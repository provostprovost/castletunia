/* SPDX-License-Identifier: GPL-3.0-only */
#include "fantasia_ui.h"
#include "fantasia_engine.h"
#include "fantasia_zones.h"
#include "fantasia_assets.h"
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
    uint8_t packed = fantasia_assets.zone_maps[zone_id % 9u][idx >> 1];
    uint8_t pal_idx = (idx & 1u) ? (packed & 0x0Fu) : ((packed >> 4) & 0x0Fu);
    return fantasia_assets.zone_pals[zone_id % 9u][pal_idx];
}

static int boss_hover_dy(uint8_t phase)
{
    static const int8_t BOB[16] = {-3, -3, -2, -1, 0, 1, 2, 3, 3, 3, 2, 1, 0, -1, -2, -3};
    return (int)BOB[(phase >> 1) & 15u];
}

static void render_battle_sprites(void)
{
    uint8_t z = g_state.settings.zone_id % 9u;
    uint8_t mirror = g_state.spells.mirror_active;

    /* 1. Boss on left/center (x=16..111, y=34..129) */
    int bx = 16;
    if (g_state.boss_hit_timer > 0u) {
        bx += (g_state.boss_hit_timer & 1u) ? -3 : 3;
    }
    int by_base = 34;
    int by = by_base + boss_hover_dy(g_boss_phase);

    for (int strip = 0; strip < 104; strip += 4) {
        for (int r = 0; r < 4; r++) {
            int sy_scr = (by_base - 4) + strip + r;
            for (int c = 0; c < 96; c++) {
                int sx_scr = 16 + c;
                uint16_t bg = sample_diorama_px(z, sx_scr >> 1, sy_scr >> 1, mirror);
                int b_x = sx_scr - bx;
                int b_y = sy_scr - by;
                if (b_x >= 0 && b_x < 96 && b_y >= 0 && b_y < 96) {
                    int b_sx = mirror ? (95 - b_x) : b_x;
                    uint32_t idx = (uint32_t)b_y * 96u + (uint32_t)b_sx;
                    uint8_t packed = fantasia_assets.boss_sprites[z][idx >> 1];
                    uint8_t ci = (idx & 1u) ? (packed & 0x0Fu) : ((packed >> 4) & 0x0Fu);
                    if (ci != 0u) {
                        bg = (g_state.boss_hit_timer == 9u || g_state.boss_hit_timer == 5u)
                           ? 0xFFFFu : fantasia_assets.boss_pals[z][ci];
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
                g_sprite_buf[r * 96 + c] = bg;
            }
        }
        hal_lcd_blit_rect(16, (by_base - 4) + strip, 96, 4, g_sprite_buf);
    }

    /* 2. Staggered FFVI Party Column on right (ABOVE the blue menu bar) */
    static const int BASE_X[3] = {174, 182, 190};
    static const int BASE_Y[3] = {24, 68, 112};
    for (uint8_t s = 0; s < NUM_PARTY_SLOTS; s++) {
        uint8_t atk_t = g_state.attack_timer[s];
        const uint16_t *pal = g_state.slots[s].frozen
                            ? G_FREEZE_PALETTE
                            : (atk_t > 0u ? fantasia_assets.party_atk_pals[s] : fantasia_assets.party_pals[s]);
        const uint8_t *spr = fantasia_assets.party_sprites[s];
        int lunge = 0;
        if (atk_t > 0u) {
            uint8_t frame = (atk_t > 10u) ? 0u : ((atk_t > 5u) ? 1u : 2u);
            spr = fantasia_assets.party_atk[s][frame];
            lunge = (frame == 0u) ? -10 : ((frame == 1u) ? -28 : -16);
        }
        int hop = (g_state.slots[s].anim_timer > 0u && atk_t == 0u) ? -2 : 0;
        int lane_x = BASE_X[s] - 32; /* 74px wide lane covers lunge */
        int lane_y = BASE_Y[s] - 2;
        int sx0 = BASE_X[s] + lunge;

        for (int strip = 0; strip < 44; strip += 4) {
            for (int r = 0; r < 4; r++) {
                int sy_scr = lane_y + strip + r;
                for (int c = 0; c < 74; c++) {
                    int px_scr = lane_x + c;
                    uint16_t bg = sample_diorama_px(z, px_scr >> 1, sy_scr >> 1, mirror);
                    int sp_x = px_scr - sx0;
                    int sp_y = sy_scr - (BASE_Y[s] + hop);
                    if (sp_x >= 0 && sp_x < 42 && sp_y >= 0 && sp_y < 42) {
                        uint32_t idx = (uint32_t)sp_y * 42u + (uint32_t)sp_x;
                        uint8_t packed = spr[idx >> 1];
                        uint8_t ci = (idx & 1u) ? (packed & 0x0Fu) : ((packed >> 4) & 0x0Fu);
                        if (ci != 0u) bg = pal[ci];
                    }
                    g_sprite_buf[r * 74 + c] = bg;
                }
            }
            hal_lcd_blit_rect(lane_x, lane_y + strip, 74, 4, g_sprite_buf);
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
        for (int x = 0; x < 240; x++) {
            int vx = x + shake_x;
            uint16_t c;
            if (slide_off > 0) {
                int src_x = vx + (240 - slide_off);
                if (src_x < 240) {
                    c = sample_diorama_px(g_state.slide_prev_zone, src_x >> 1, sy, mirror);
                } else {
                    c = sample_diorama_px(z, (src_x - 240) >> 1, sy, mirror);
                }
            } else {
                c = sample_diorama_px(z, vx >> 1, sy, mirror);
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
    uint16_t pcol = fantasia_assets.zone_pals[z % 9u][14];
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
                uint8_t packed = fantasia_assets.cursor_sprite[idx >> 1];
                uint8_t ci = (idx & 1u) ? (packed & 0x0Fu) : ((packed >> 4) & 0x0Fu);
                if (ci != 0u) col = fantasia_assets.sprite_pal[ci];
            }
            g_sprite_buf[j * 16 + i] = col;
        }
    }
    hal_lcd_blit_rect(6, row_y, 16, 14, g_sprite_buf);

    uint16_t name_col = g_state.slots[s].frozen ? 0x5E7Fu : (g_state.next_slot == s ? 0xFE60u : 0xFFFFu);
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
                c = g_state.slots[s].frozen ? 0x5E7Fu : (seg >= 6 ? 0xFFFFu : 0xFE60u);
            } else if (g_state.slots[s].note_count > 0u && seg == 0u) {
                c = g_state.slots[s].frozen ? 0x2B55u : 0x4210u;
            }
            g_sprite_buf[j * 64 + i] = c;
        }
    }
    hal_lcd_blit_rect(110, row_y + 4, 64, 6, g_sprite_buf);

    char badge[8];
    uint16_t bcol;
    if (g_state.slots[s].frozen) {
        badge[0]='F'; badge[1]='R'; badge[2]='Z'; badge[3]='\0';
        bcol = 0x5E7Fu;
    } else if (g_state.slots[s].note_count > 0u) {
        uint8_t len = (g_state.settings.weave > 40u) ? g_state.slots[s].weave_length : g_state.slots[s].base_length;
        badge[0] = (g_state.settings.weave > 40u) ? 'W' : 'L';
        if (len >= 10u) {
            badge[1] = (char)('0' + (len / 10u));
            badge[2] = (char)('0' + (len % 10u));
            badge[3] = '\0';
        } else {
            badge[1] = (char)('0' + len);
            badge[2] = '\0';
        }
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

    if (g_state.dirty_diorama || g_state.slide_frames > 0u || g_state.shake_timer > 0u || g_state.toll_ring_timer > 0u) {
        render_diorama_full();
        if (g_state.slide_frames == 0u) render_battle_sprites();
        g_state.dirty_diorama = 0u;
        if (g_state.slide_frames > 0u) g_state.slide_frames--;
        if (g_state.shake_timer > 0u) g_state.shake_timer--;
        if (g_state.toll_ring_timer > 0u) g_state.toll_ring_timer--;
    } else {
        render_particles();
        g_boss_phase++;
        render_battle_sprites();
    }

    if (g_state.boss_hit_timer > 0u) g_state.boss_hit_timer--;
    for (uint8_t s = 0; s < NUM_PARTY_SLOTS; s++) {
        if (g_state.attack_timer[s] > 0u) g_state.attack_timer[s]--;
        if (g_state.slots[s].anim_timer > 0u) {
            g_state.slots[s].anim_timer--;
            g_state.dirty_party = 1u;
        }
    }
    if (g_state.ticker_timer > 0u) {
        g_state.ticker_timer--;
        if (g_state.ticker_timer == 0u) g_state.dirty_ticker = 1u;
    } else {
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
        for (uint8_t s = 0; s < NUM_PARTY_SLOTS; s++) {
            render_party_row(s, ROW_Y[s]);
        }
        g_state.dirty_party = 0u;
    }

    if (g_state.dirty_ticker) {
        render_ticker();
        g_state.dirty_ticker = 0u;
    }
}
