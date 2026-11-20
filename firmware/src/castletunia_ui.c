/* SPDX-License-Identifier: GPL-3.0-only */
#include "castletunia_ui.h"
#include "castletunia_engine.h"
#include "castletunia_zones.h"
#include "castletunia_assets.h"
#include "hal_adapter.h"

#define DIORAMA_W 120
#define DIORAMA_H 80
#define SCREEN_W  240
#define SCREEN_H  240
#define NUM_PARTICLES 6

typedef struct {
    int16_t x;
    int16_t y;
    int8_t  vx;
    int8_t  vy;
} AmbientParticle;

/* Small static scanline & sprite scratch (~4.3 KB total) */
static uint16_t g_scanline[SCREEN_W];       /* 480 B */
static uint16_t g_sprite_buf[42 * 45];      /* 3780 B */
static uint16_t g_patch_buf[4 * 4];         /* 32 B */
static AmbientParticle g_particles[NUM_PARTICLES];
static uint8_t g_hud_frame_drawn = 0;
static uint8_t g_last_haste = 0;
static uint8_t g_boss_phase = 0;

static uint16_t hud_bg_color(int y)
{
    int t = y - 160;
    if (t < 0) t = 0;
    if (t > 79) t = 79;
    uint8_t r = (uint8_t)(8 + ((24 - 8) * (79 - t)) / 79);
    uint8_t g = (uint8_t)(16 + ((52 - 16) * (79 - t)) / 79);
    uint8_t b = (uint8_t)(64 + ((148 - 64) * (79 - t)) / 79);
    return (uint16_t)(((r & 0xF8u) << 8) | ((g & 0xFCu) << 3) | (b >> 3));
}

static uint16_t sample_diorama_px(uint8_t zone_id, int sx, int sy, uint8_t mirror)
{
    if (sx < 0) sx = 0;
    if (sx >= DIORAMA_W) sx = DIORAMA_W - 1;
    if (sy < 0) sy = 0;
    if (sy >= DIORAMA_H) sy = DIORAMA_H - 1;
    if (mirror) sx = (DIORAMA_W - 1) - sx;

    uint32_t idx = (uint32_t)sy * DIORAMA_W + (uint32_t)sx;
    uint8_t packed = castletunia_assets.zone_maps[zone_id % 9u][idx >> 1];
    uint8_t pal_idx = (idx & 1u) ? (packed & 0x0Fu) : ((packed >> 4) & 0x0Fu);
    return castletunia_assets.zone_pals[zone_id % 9u][pal_idx];
}

static int boss_hover_dy(uint8_t phase)
{
    static const int8_t BOB[16] = {-3, -3, -2, -1, 0, 1, 2, 3, 3, 3, 2, 1, 0, -1, -2, -3};
    return (int)BOB[(phase >> 1) & 15u];
}

static uint16_t boss_atk_tint(uint8_t z)
{
    static const uint16_t TINTS[9] = {
        0x781Fu, /* 0 Crypt: spectral purple */
        0xFC00u, /* 1 Tavern: amber ale */
        0x07E4u, /* 2 Glade: emerald overgrow */
        0xF800u, /* 3 Castle: crimson blood moon */
        0xFD20u, /* 4 Sands (Anubis): golden sandstorm */
        0x07FFu, /* 5 Spire: cyan void warp */
        0x867Fu, /* 6 Tundra: ice frost */
        0xFFE0u, /* 7 Coronation: radiant gold */
        0xF810u  /* 8 Chaos: magenta meteor */
    };
    return TINTS[zone_get(z)->subgenre_id % 9u];
}

static int boss_atk_ripple_dx(int y)
{
    static const int8_t SIN16[16] = {0, 2, 4, 5, 6, 5, 4, 2, 0, -2, -4, -5, -6, -5, -4, -2};
    if (g_state.boss_atk_timer == 0u) return 0;
    int s = (int)SIN16[((y >> 2) + (int)g_state.boss_atk_timer) & 15];
    return (s * (int)(g_state.boss_atk_timer + 7u)) >> 5;
}

static uint16_t apply_boss_atk_fx(uint16_t c, uint8_t z)
{
    if (g_state.boss_atk_timer == 0u) return c;
    uint16_t tint = boss_atk_tint(z);
    if (g_state.boss_atk_timer > 28u && (g_state.boss_atk_timer & 2u)) {
        c ^= 0x39E7u;
    }
    return (uint16_t)(((c & 0xF7DEu) >> 1) + ((tint & 0xF7DEu) >> 1));
}

static void render_battle_sprites(void)
{
    uint8_t z = g_state.settings.zone_id % 9u;
    uint8_t mirror = g_state.spells.mirror_active;

    /* 1. Boss on left (normal) or right facing left (mirror) */
    int boss_lane_w = (g_state.boss_atk_timer > 0u) ? 116 : 96;
    int boss_lane_x = mirror ? (g_state.boss_atk_timer > 0u ? 108 : 128) : 16;
    int bx = mirror ? 128 : 16;
    if (g_state.boss_atk_timer > 0u) {
        int atk_lunge = (g_state.boss_atk_timer > 24u) ? 18 : ((g_state.boss_atk_timer > 12u) ? 12 : 5);
        bx += mirror ? -atk_lunge : atk_lunge;
    } else if (g_state.boss_hit_timer > 0u) {
        bx += (g_state.boss_hit_timer & 1u) ? -3 : 3;
    }
    int by_base = 34;
    int by = by_base + boss_hover_dy(g_boss_phase);

    for (int strip = 0; strip < 104; strip += 4) {
        for (int r = 0; r < 4; r++) {
            int sy_scr = (by_base - 4) + strip + r;
            int rip = boss_atk_ripple_dx(sy_scr);
            for (int c = 0; c < boss_lane_w; c++) {
                int sx_scr = boss_lane_x + c;
                uint16_t bg = apply_boss_atk_fx(
                    sample_diorama_px(z, (sx_scr + rip) >> 1, sy_scr >> 1, mirror), z);
                int b_x = sx_scr - bx;
                int b_y = sy_scr - by;
                if (b_x >= 0 && b_x < 96 && b_y >= 0 && b_y < 96) {
                    int b_sx = mirror ? (95 - b_x) : b_x;
                    uint32_t idx = (uint32_t)b_y * 96u + (uint32_t)b_sx;
                    uint8_t packed = castletunia_assets.boss_sprites[z][idx >> 1];
                    uint8_t ci = (idx & 1u) ? (packed & 0x0Fu) : ((packed >> 4) & 0x0Fu);
                    if (ci != 0u) {
                        if (g_state.boss_hit_timer == 9u || g_state.boss_hit_timer == 5u) {
                            bg = 0xFFFFu;
                        } else if (g_state.boss_atk_timer > 18u && (g_state.boss_atk_timer & 2u)) {
                            bg = castletunia_assets.boss_pals[z][ci] ^ 0x39E7u;
                        } else {
                            bg = castletunia_assets.boss_pals[z][ci];
                        }
                    }
                }
                /* Diagonal slash / spell flash on boss hit */
                if (g_state.boss_hit_timer > 3u) {
                    int slash_x = 24 + (10 - (int)g_state.boss_hit_timer) * 6;
                    if (c >= slash_x && c <= slash_x + 3 && sy_scr >= 56 && sy_scr <= 104 &&
                        ((c - slash_x) == ((sy_scr - 56) / 12) || (c - slash_x) == 0)) {
                        bg = 0xFE60u;
                    }
                }
                g_sprite_buf[r * boss_lane_w + c] = bg;
            }
        }
        hal_lcd_blit_rect(boss_lane_x, (by_base - 4) + strip, boss_lane_w, 4, g_sprite_buf);
    }

    /* 2. Staggered FFVI Party Column on right (normal) or left facing right (mirror) */
    static const int BASE_X[3]   = {174, 182, 190};
    static const int BASE_X_M[3] = {24, 16, 8};
    static const int BASE_Y[3]   = {24, 68, 112};
    int party_recoil = (g_state.boss_atk_timer > 20u) ? 4 : ((g_state.boss_atk_timer > 8u) ? 2 : 0);
    for (uint8_t s = 0; s < NUM_PARTY_SLOTS; s++) {
        uint8_t atk_t = g_state.attack_timer[s];
        uint8_t st = g_state.slots[s].status;
        uint8_t pending = (st != g_state.slots[s].audio_status) ? 1u : 0u;
        uint8_t flash_alt = pending && (((hal_ms() >> 7) & 1u) != 0u);
        const uint8_t *spr = (st == STATUS_SLOW) ? castletunia_assets.sloth_sprite : castletunia_assets.party_sprites[s];
        const uint16_t *base_pal = (st == STATUS_SLOW)
                                 ? castletunia_assets.sloth_pal
                                 : (atk_t > 0u ? castletunia_assets.party_atk_pals[s] : castletunia_assets.party_pals[s]);
        uint16_t status_pal[16];
        const uint16_t *pal = base_pal;
        if (flash_alt) {
            for (int pi = 0; pi < 16; pi++) {
                uint16_t c = base_pal[pi];
                status_pal[pi] = (c == 0u) ? 0u : (uint16_t)(((c & 0xF7DEu) >> 1) + ((0xFFFFu & 0xF7DEu) >> 1));
            }
            pal = status_pal;
        } else if (g_state.slots[s].frozen || st == STATUS_FREEZE) {
            pal = G_FREEZE_PALETTE;
        } else if (st == STATUS_POISON || st == STATUS_BERSERK) {
            uint16_t tint = (st == STATUS_POISON) ? 0xC81Fu : 0xF800u;
            for (int pi = 0; pi < 16; pi++) {
                uint16_t c = base_pal[pi];
                status_pal[pi] = (c == 0u) ? 0u : (uint16_t)(((c & 0xF7DEu) >> 1) + ((tint & 0xF7DEu) >> 1));
            }
            pal = status_pal;
        }
        int lunge = 0;
        if (atk_t > 0u) {
            uint8_t frame = (atk_t > 10u) ? 0u : ((atk_t > 5u) ? 1u : 2u);
            if (st != STATUS_SLOW) spr = castletunia_assets.party_atk[s][frame];
            lunge = (frame == 0u) ? -10 : ((frame == 1u) ? -28 : -16);
        } else if (party_recoil > 0) {
            lunge = party_recoil;
        }
        if (mirror) lunge = -lunge;
        int hop = (g_state.slots[s].anim_timer > 0u && atk_t == 0u) ? -2 : 0;
        int base_x = mirror ? BASE_X_M[s] : BASE_X[s];
        int lane_x = mirror ? (base_x - 4) : (base_x - 32); /* 78px wide lane covers lunge + recoil */
        int lane_y = BASE_Y[s] - 2;
        int sx0 = base_x + lunge;

        for (int strip = 0; strip < 44; strip += 4) {
            for (int r = 0; r < 4; r++) {
                int sy_scr = lane_y + strip + r;
                int rip = boss_atk_ripple_dx(sy_scr);
                for (int c = 0; c < 78; c++) {
                    int px_scr = lane_x + c;
                    uint16_t bg = apply_boss_atk_fx(
                        sample_diorama_px(z, (px_scr + rip) >> 1, sy_scr >> 1, mirror), z);
                    int sp_x = px_scr - sx0;
                    int sp_y = sy_scr - (BASE_Y[s] + hop);
                    if (sp_x >= 0 && sp_x < 42 && sp_y >= 0 && sp_y < 42) {
                        int sp_sx = mirror ? (41 - sp_x) : sp_x;
                        uint32_t idx = (uint32_t)sp_y * 42u + (uint32_t)sp_sx;
                        uint8_t packed = spr[idx >> 1];
                        uint8_t ci = (idx & 1u) ? (packed & 0x0Fu) : ((packed >> 4) & 0x0Fu);
                        if (ci != 0u) bg = pal[ci];
                    }
                    g_sprite_buf[r * 78 + c] = bg;
                }
            }
            hal_lcd_blit_rect(lane_x, lane_y + strip, 78, 4, g_sprite_buf);
        }
    }
}

static void restore_diorama_4x4(uint8_t zone_id, int px, int py, uint8_t mirror)
{
    for (int j = 0; j < 4; j++) {
        int yy = py + j;
        if (yy < 0 || yy >= 160) continue;
        for (int i = 0; i < 4; i++) {
            int xx = px + i;
            if (xx < 0 || xx >= 240) {
                g_patch_buf[j * 4 + i] = 0;
            } else {
                g_patch_buf[j * 4 + i] = sample_diorama_px(zone_id, xx >> 1, yy >> 1, mirror);
            }
        }
    }
    hal_lcd_blit_rect(px, py, 4, 4, g_patch_buf);
}

static void render_diorama_full(void)
{
    uint8_t z = g_state.settings.zone_id;
    uint8_t mirror = g_state.spells.mirror_active;
    int shake_x = 0;
    if (g_state.shake_timer > 0u) {
        shake_x = (g_state.shake_timer & 1u) ? (int)g_state.shake_amp : -(int)g_state.shake_amp;
    }

    int slide_off = 0;
    if (g_state.slide_frames > 0u) {
        slide_off = ((int)g_state.slide_frames * 240) / 12;
    }

    for (int y = 0; y < 160; y++) {
        int sy = y >> 1;
        int rip = boss_atk_ripple_dx(y);
        for (int x = 0; x < 240; x++) {
            int vx = x + shake_x + rip;
            uint16_t c;
            if (slide_off > 0) {
                if (g_state.slide_dir >= 0) {
                    int src_x = vx + (240 - slide_off);
                    if (src_x < 240) {
                        c = sample_diorama_px(g_state.slide_prev_zone, src_x >> 1, sy, mirror);
                    } else {
                        c = sample_diorama_px(z, (src_x - 240) >> 1, sy, mirror);
                    }
                } else {
                    int src_x = vx - (240 - slide_off);
                    if (src_x < 0) {
                        c = sample_diorama_px(z, (src_x + 240) >> 1, sy, mirror);
                    } else {
                        c = sample_diorama_px(g_state.slide_prev_zone, src_x >> 1, sy, mirror);
                    }
                }
            } else {
                c = apply_boss_atk_fx(sample_diorama_px(z, vx >> 1, sy, mirror), z);
            }
            g_scanline[x] = c;
        }

        if (g_state.toll_ring_timer > 0u) {
            int rad = (15 - (int)g_state.toll_ring_timer) * 6;
            int dy = y - 80;
            for (int x = 0; x < 240; x++) {
                int dx = x - 120;
                int d2 = dx * dx + dy * dy;
                if (d2 >= (rad - 2) * (rad - 2) && d2 <= (rad + 2) * (rad + 2)) {
                    g_scanline[x] = 0xFFFFu;
                }
            }
        }

        hal_lcd_blit_rect(0, y, 240, 1, g_scanline);
    }
}

static void render_particles(void)
{
    uint8_t z = g_state.settings.zone_id;
    uint8_t mirror = g_state.spells.mirror_active;
    uint16_t pcol = castletunia_assets.zone_pals[z % 9u][14];
    for (int i = 0; i < NUM_PARTICLES; i++) {
        restore_diorama_4x4(z, g_particles[i].x, g_particles[i].y, mirror);
        g_particles[i].x += g_particles[i].vx;
        g_particles[i].y += g_particles[i].vy;
        if (g_particles[i].x < 4) g_particles[i].x = 136;
        if (g_particles[i].x > 138) g_particles[i].x = 6;
        if (g_particles[i].y < 4) g_particles[i].y = 152;
        if (g_particles[i].y > 154) g_particles[i].y = 6;
        for (int j = 0; j < 4; j++) g_patch_buf[j] = pcol;
        hal_lcd_blit_rect(g_particles[i].x, g_particles[i].y, 2, 2, g_patch_buf);
    }
}

static void render_hud_border(void)
{
    uint16_t border = (g_state.spells.haste_active || g_state.gold_flash_timer > 0u) ? 0xFE60u : 0xFFFFu;
    for (int y = 160; y < 240; y++) {
        uint16_t bg = hud_bg_color(y);
        for (int x = 0; x < 240; x++) {
            if (y == 160 || y == 161 || y == 238 || y == 239 || x == 0 || x == 1 || x == 238 || x == 239 || y == 177) {
                g_scanline[x] = border;
            } else {
                g_scanline[x] = bg;
            }
        }
        hal_lcd_blit_rect(0, y, 240, 1, g_scanline);
    }
}

static void render_header(void)
{
    static const char *const ROOT_NAMES[12] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
    static const char *const MODE_NAMES[9] = {"AEOL", "MIXO", "DOR", "HARM", "PHRY", "LYD", "AEOL", "ION", "PHRY"};
    const ZoneDef *z = zone_get(g_state.settings.zone_id);

    char key_str[16];
    const char *rn = ROOT_NAMES[g_state.settings.root_semitone % 12u];
    const char *mn = MODE_NAMES[g_state.settings.zone_id % 9u];
    int k = 0;
    while (*rn) key_str[k++] = *rn++;
    key_str[k++] = ' ';
    while (*mn) key_str[k++] = *mn++;
    key_str[k] = '\0';

    char bpm_str[10];
    uint8_t b = g_state.settings.bpm;
    int i = 0;
    if (b >= 100u) bpm_str[i++] = (char)('0' + (b / 100u));
    bpm_str[i++] = (char)('0' + ((b / 10u) % 10u));
    bpm_str[i++] = (char)('0' + (b % 10u));
    bpm_str[i++] = ' '; bpm_str[i++] = 'B'; bpm_str[i++] = 'P'; bpm_str[i++] = 'M'; bpm_str[i] = '\0';

    hal_draw_text(6, 162, 92, z->name, 0xFFFFu, hud_bg_color(166), 0);
    hal_draw_text(100, 162, 74, key_str, 0x5E7Fu, hud_bg_color(166), 1);
    hal_draw_text(176, 162, 58, bpm_str, 0xFE60u, hud_bg_color(166), 2);
}

static void render_party_row(uint8_t s, int row_y)
{
    static const char *const NAMES[3] = {"FIGHTER", "CLERIC", "SORCERESS"};
    /* Gloved pointer */
    for (int j = 0; j < 14; j++) {
        uint16_t bg = hud_bg_color(row_y + j);
        for (int i = 0; i < 16; i++) {
            uint16_t col = bg;
            if (g_state.next_slot == s) {
                uint32_t idx = (uint32_t)j * 16u + (uint32_t)i;
                uint8_t packed = castletunia_assets.cursor_sprite[idx >> 1];
                uint8_t ci = (idx & 1u) ? (packed & 0x0Fu) : ((packed >> 4) & 0x0Fu);
                if (ci != 0u) col = castletunia_assets.sprite_pal[ci];
            }
            g_sprite_buf[j * 16 + i] = col;
        }
    }
    hal_lcd_blit_rect(6, row_y, 16, 14, g_sprite_buf);

    uint8_t st = g_state.slots[s].status;
    uint16_t st_col = (st == STATUS_FREEZE || g_state.slots[s].frozen) ? 0x5E7Fu
                    : ((st == STATUS_SLOW) ? 0x07FFu
                    : ((st == STATUS_POISON) ? 0xC81Fu
                    : ((st == STATUS_BERSERK) ? 0xF800u : 0xFFFFu)));
    uint16_t name_col = (st != STATUS_NONE || g_state.slots[s].frozen) ? st_col
                      : (g_state.next_slot == s ? 0xFE60u : 0xFFFFu);
    hal_draw_text(24, row_y, 82, NAMES[s], name_col, hud_bg_color(row_y), 0);

    /* Live 8-segment RPG Voice Activity Meter (pulses on note trigger) */
    uint8_t level = g_state.slots[s].anim_timer; /* 0..8 */
    if (level > 8u) level = 8u;
    for (int j = 0; j < 6; j++) {
        for (int i = 0; i < 64; i++) {
            uint8_t seg = (uint8_t)(i >> 3); /* 0..7 */
            uint8_t in_gap = ((i & 7) == 7);
            uint16_t c = 0x1084u;
            if (in_gap) {
                c = hud_bg_color(row_y + 4 + j);
            } else if (g_state.slots[s].note_count > 0u && seg < level) {
                c = (st != STATUS_NONE || g_state.slots[s].frozen) ? st_col : (seg >= 6 ? 0xFFFFu : 0xFE60u);
            } else if (g_state.slots[s].note_count > 0u && seg == 0u) {
                c = g_state.slots[s].frozen ? 0x2B55u : 0x4210u;
            }
            g_sprite_buf[j * 64 + i] = c;
        }
    }
    hal_lcd_blit_rect(110, row_y + 4, 64, 6, g_sprite_buf);

    char badge[8];
    uint16_t bcol;
    if (g_state.slots[s].frozen || st == STATUS_FREEZE) {
        badge[0]='F'; badge[1]='R'; badge[2]='Z'; badge[3]='\0';
        bcol = 0x5E7Fu;
    } else if (st == STATUS_SLOW) {
        badge[0]='S'; badge[1]='L'; badge[2]='W'; badge[3]='\0';
        bcol = 0x07FFu;
    } else if (st == STATUS_POISON) {
        badge[0]='P'; badge[1]='S'; badge[2]='N'; badge[3]='\0';
        bcol = 0xC81Fu;
    } else if (st == STATUS_BERSERK) {
        badge[0]='B'; badge[1]='S'; badge[2]='K'; badge[3]='\0';
        bcol = 0xF800u;
    } else if (g_state.spells.haste_active && s >= 1u) {
        badge[0]='H'; badge[1]='S'; badge[2]='T'; badge[3]='\0';
        bcol = 0xFE60u;
    } else if (g_state.slots[s].note_count > 0u) {
        badge[0]='\0';
        bcol = 0x87F0u;
    } else {
        badge[0]='R'; badge[1]='D'; badge[2]='Y'; badge[3]='\0';
        bcol = 0xAD55u;
    }
    hal_draw_text(180, row_y, 54, badge, bcol, hud_bg_color(row_y), 2);
}

static void render_ticker(void)
{
    if (g_state.ticker_timer > 0u) {
        hal_draw_text(8, 223, 224, g_state.ticker, 0xFFFFu, hud_bg_color(223), 0);
        return;
    }
    static const char *const DEG_ROMAN[7] = {"I", "II", "III", "IV", "V", "VI", "VII"};
    char buf[32];
    const char *deg = DEG_ROMAN[g_state.active_chord_deg % 7u];
    int i = 0;
    buf[i++]='C'; buf[i++]='H'; buf[i++]=':';
    while (*deg) buf[i++] = *deg++;
    buf[i++]=' ';
    const char *lbl[3] = {"DRF:", "DNS:", "HRM:"};
    uint8_t vals[3] = {g_state.settings.drift, g_state.settings.density, g_state.settings.harmony};
    for (int m = 0; m < 3; m++) {
        buf[i++]=' ';
        const char *p = lbl[m];
        while (*p) buf[i++] = *p++;
        uint8_t v = vals[m];
        if (v >= 100u) buf[i++] = (char)('0' + (v / 100u));
        if (v >= 10u)  buf[i++] = (char)('0' + ((v / 10u) % 10u));
        buf[i++] = (char)('0' + (v % 10u));
    }
    buf[i] = '\0';
    hal_draw_text(8, 223, 224, buf, 0x5E7Fu, hud_bg_color(223), 0);
}

void ui_render_splash(void)
{
    for (int y = 0; y < 240; y++) {
        uint32_t row_off = (uint32_t)y * 240u;
        for (int x = 0; x < 240; x++) {
            uint8_t ci = castletunia_assets.splash_data[row_off + (uint32_t)x];
            g_scanline[x] = castletunia_assets.splash_pal[ci];
        }
        hal_lcd_blit_rect(0, y, 240, 1, g_scanline);
    }
    hal_draw_text(8, 224, 224, "BRIAN PROVOST  v1.1.0", 0xFE60u, 0x0021u, 1);
}

void ui_init(void)
{
    for (int i = 0; i < NUM_PARTICLES; i++) {
        g_particles[i].x = (int16_t)(12 + i * 20);
        g_particles[i].y = (int16_t)(16 + (i * 23) % 120);
        g_particles[i].vx = (i & 1) ? 1 : -1;
        g_particles[i].vy = 1;
    }
    g_hud_frame_drawn = 0;
    g_state.dirty_diorama = 1u;
    g_state.dirty_header = 1u;
    g_state.dirty_party = 1u;
    g_state.dirty_ticker = 1u;
}

static void render_note_projectiles(void)
{
    static const uint8_t NOTE_GLYPH[7] = {
        0x1Cu, /* ..###.. */
        0x14u, /* ..#.#.. */
        0x12u, /* ..#..#. */
        0x10u, /* ..#.... */
        0x70u, /* .###... */
        0x70u, /* .###... */
        0x20u  /* ..#.... */
    };
    static const uint16_t SLOT_COLS[3] = { 0xFE60u, 0x07FFu, 0xF81Fu };
    uint8_t z = g_state.settings.zone_id % 9u;
    uint8_t mirror = g_state.spells.mirror_active;

    for (uint8_t i = 0; i < MAX_SEED_NOTES; i++) {
        NoteProjectile *np = &g_state.note_fx[i];
        if (!np->active) continue;
        if (np->delay > 0u) {
            np->delay--;
            continue;
        }
        int ox = (int)np->x;
        int oy = (int)np->y;
        /* Erase previous 7x7 note footprint cleanly without wiping the whole diorama */
        for (int r = 0; r < 7; r++) {
            int yy = oy + r;
            for (int c = 0; c < 7; c++) {
                int xx = ox + c;
                g_sprite_buf[r * 7 + c] = (xx >= 0 && xx < 240 && yy >= 0 && yy < 160)
                    ? apply_boss_atk_fx(sample_diorama_px(z, xx >> 1, yy >> 1, mirror), z)
                    : 0u;
            }
        }
        if (ox >= 0 && ox <= 233 && oy >= 0 && oy <= 153) {
            hal_lcd_blit_rect(ox, oy, 7, 7, g_sprite_buf);
        }

        np->x += np->vx;
        np->y += np->vy;
        if ((np->vx < 0 && np->x <= 72) || (np->vx > 0 && np->x >= 164)) {
            np->active = 0u;
            g_state.boss_hit_timer = 5u;
            continue;
        }

        int px = (int)np->x;
        int py = (int)np->y;
        uint16_t col = SLOT_COLS[np->slot % 3u];
        for (int r = 0; r < 7; r++) {
            int yy = py + r;
            for (int c = 0; c < 7; c++) {
                int xx = px + c;
                uint16_t bg = (xx >= 0 && xx < 240 && yy >= 0 && yy < 160)
                            ? apply_boss_atk_fx(sample_diorama_px(z, xx >> 1, yy >> 1, mirror), z)
                            : 0u;
                if ((NOTE_GLYPH[r] >> (6 - c)) & 1u) bg = col;
                g_sprite_buf[r * 7 + c] = bg;
            }
        }
        if (px >= 0 && px <= 233 && py >= 0 && py <= 153) {
            hal_lcd_blit_rect(px, py, 7, 7, g_sprite_buf);
        }
    }
}

void ui_render(void)
{
    if (!g_hud_frame_drawn || g_state.spells.haste_active != g_last_haste) {
        g_last_haste = g_state.spells.haste_active;
        render_hud_border();
        g_hud_frame_drawn = 1;
        g_state.dirty_header = 1u;
        g_state.dirty_party = 1u;
        g_state.dirty_ticker = 1u;
    }

    if (g_state.dirty_diorama || g_state.slide_frames > 0u || g_state.shake_timer > 0u ||
        g_state.toll_ring_timer > 0u || g_state.boss_atk_timer > 0u) {
        render_diorama_full();
        if (g_state.slide_frames == 0u) {
            render_battle_sprites();
            render_note_projectiles();
        }
        g_state.dirty_diorama = 0u;
        if (g_state.slide_frames > 0u) {
            g_state.slide_frames--;
            if (g_state.slide_frames == 0u) g_state.dirty_diorama = 1u;
        }
        if (g_state.shake_timer > 0u) g_state.shake_timer--;
        if (g_state.toll_ring_timer > 0u) g_state.toll_ring_timer--;
        if (g_state.boss_atk_timer > 0u) {
            g_state.boss_atk_timer--;
            if (g_state.boss_atk_timer == 0u) g_state.dirty_diorama = 1u;
        }
    } else {
        render_particles();
        g_boss_phase++;
        render_battle_sprites();
        render_note_projectiles();
    }

    if (g_state.boss_hit_timer > 0u) g_state.boss_hit_timer--;
    for (uint8_t s = 0; s < NUM_PARTY_SLOTS; s++) {
        if (g_state.attack_timer[s] > 0u) g_state.attack_timer[s]--;
        if (g_state.slots[s].anim_timer > 0u) {
            g_state.slots[s].anim_timer--;
            g_state.dirty_party = 1u;
        }
    }
    if (g_state.ticker_timer == 0u) {
        g_state.dirty_ticker = 1u;
    }

    if (g_state.gold_flash_timer > 0u) {
        g_state.gold_flash_timer--;
        if (g_state.gold_flash_timer == 0u) render_hud_border();
    }

    if (g_state.dirty_header) {
        render_header();
        g_state.dirty_header = 0u;
    }

    if (g_state.dirty_party) {
        static const int ROW_Y[3] = {179, 193, 207};
        uint8_t any_pending = 0u;
        for (uint8_t s = 0; s < NUM_PARTY_SLOTS; s++) {
            render_party_row(s, ROW_Y[s]);
            if (g_state.slots[s].status != g_state.slots[s].audio_status) any_pending = 1u;
        }
        if (!any_pending) g_state.dirty_party = 0u;
    }

    if (g_state.dirty_ticker) {
        render_ticker();
        g_state.dirty_ticker = 0u;
    }
}
