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
#define NUM_PARTICLES 8

typedef struct {
    int16_t x;
    int16_t y;
    int8_t  vx;
    int8_t  vy;
} AmbientParticle;

/* Small static scanline & sprite scratch (~3 KB total) */
static uint16_t g_scanline[SCREEN_W];       /* 480 B */
static uint16_t g_sprite_buf[32 * 36];      /* 2304 B */
static uint16_t g_patch_buf[4 * 4];         /* 32 B */
static AmbientParticle g_particles[NUM_PARTICLES];
static uint8_t g_hud_frame_drawn = 0;
static uint8_t g_last_haste = 0;
static uint8_t g_boss_phase = 0;

static uint16_t sample_diorama_px(uint8_t zone_id, int sx, int sy, uint8_t mirror);

static int boss_hover_dy(uint8_t phase)
{
    static const int8_t BOB[16] = {-3, -3, -2, -1, 0, 1, 2, 3, 3, 3, 2, 1, 0, -1, -2, -3};
    return (int)BOB[(phase >> 1) & 15u];
}

static void render_boss_overlay(void)
{
    uint8_t z = g_state.settings.zone_id % 9u;
    uint8_t mirror = g_state.spells.mirror_active;
    int bx = 88;
    int by_base = 44;
    int dy_bob = boss_hover_dy(g_boss_phase);
    int by = by_base + dy_bob;

    /* Render 64x72 box (covers bob range -3..+3) in 8-row strips into g_sprite_buf (64*8 = 512 uint16_t) */
    for (int strip = 0; strip < 72; strip += 8) {
        for (int r = 0; r < 8; r++) {
            int sy_scr = (by_base - 4) + strip + r;
            for (int c = 0; c < 64; c++) {
                int sx_scr = bx + c;
                uint16_t bg = sample_diorama_px(z, sx_scr >> 1, sy_scr >> 1, mirror);
                int b_y = sy_scr - by;
                if (b_y >= 0 && b_y < 64) {
                    int b_sx = mirror ? (31 - (c >> 1)) : (c >> 1);
                    int b_sy = b_y >> 1;
                    uint32_t idx = (uint32_t)b_sy * 32u + (uint32_t)b_sx;
                    uint8_t packed = fantasia_assets.boss_sprites[z][idx >> 1];
                    uint8_t ci = (idx & 1u) ? (packed & 0x0Fu) : ((packed >> 4) & 0x0Fu);
                    if (ci != 0u) bg = fantasia_assets.boss_pals[z][ci];
                }
                g_sprite_buf[r * 64 + c] = bg;
            }
        }
        hal_lcd_blit_rect(bx, (by_base - 4) + strip, 64, 8, g_sprite_buf);
    }
}

static uint16_t hud_bg_color(int y)
{
    /* Royal-blue FFVI gradient for y=160..239 */
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

        /* Optional Toll shockwave ring */
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
        if (g_particles[i].x < 4) g_particles[i].x = 232;
        if (g_particles[i].x > 234) g_particles[i].x = 6;
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
    const ZoneDef *z = zone_get(g_state.settings.zone_id);
    char bpm_str[12];
    uint8_t b = g_state.settings.bpm;
    int i = 0;
    if (b >= 100u) bpm_str[i++] = (char)('0' + (b / 100u));
    bpm_str[i++] = (char)('0' + ((b / 10u) % 10u));
    bpm_str[i++] = (char)('0' + (b % 10u));
    bpm_str[i++] = ' '; bpm_str[i++] = 'B'; bpm_str[i++] = 'P'; bpm_str[i++] = 'M'; bpm_str[i] = '\0';

    hal_draw_text(8, 162, 140, z->name, 0xFFFFu, hud_bg_color(166), 0);
    hal_draw_text(152, 162, 80, bpm_str, 0xFE60u, hud_bg_color(166), 2);
}

static void render_party_slot(uint8_t s, int base_x)
{
    const uint16_t *pal = g_state.slots[s].frozen ? G_FREEZE_PALETTE : fantasia_assets.sprite_pal;
    int hop = (g_state.slots[s].anim_timer > 0u) ? -3 : 0;
    int base_y = 182;

    /* 32x36 region (32x32 sprite + 4px hop clearance) */
    for (int j = 0; j < 36; j++) {
        uint16_t bg = hud_bg_color(base_y - 3 + j);
        for (int i = 0; i < 32; i++) {
            g_sprite_buf[j * 32 + i] = bg;
        }
    }

    for (int sy = 0; sy < 32; sy++) {
        for (int sx = 0; sx < 32; sx++) {
            uint32_t idx = (uint32_t)sy * 32u + (uint32_t)sx;
            uint8_t packed = fantasia_assets.party_sprites[s][idx >> 1];
            uint8_t ci = (idx & 1u) ? (packed & 0x0Fu) : ((packed >> 4) & 0x0Fu);
            if (ci == 0u) continue;
            uint16_t col = pal[ci];
            int dy = 3 + hop + sy;
            g_sprite_buf[dy * 32 + sx] = col;
        }
    }
    hal_lcd_blit_rect(base_x, base_y - 3, 32, 36, g_sprite_buf);

    /* Gloved pointer 16px left of next_slot */
    int ptr_x = base_x - 16;
    for (int j = 0; j < 16; j++) {
        uint16_t bg = hud_bg_color(190 + j);
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
    hal_lcd_blit_rect(ptr_x, 190, 16, 16, g_sprite_buf);

    /* 32x4 step activity bar at y=215..218 */
    uint8_t len = (g_state.settings.weave > 60u) ? g_state.slots[s].weave_length : g_state.slots[s].base_length;
    if (len == 0u) len = 16u;
    uint16_t active_mask = 0u;
    for (uint8_t n = 0; n < g_state.slots[s].note_count; n++) {
        if (g_state.slots[s].scale_deg[n] >= 0) {
            active_mask |= (uint16_t)(1u << (g_state.slots[s].step_pos[n] & 15u));
        }
    }
    for (int j = 0; j < 4; j++) {
        for (int i = 0; i < 32; i++) {
            uint8_t step = (uint8_t)(i >> 1);
            uint16_t c = 0x1084u;
            if (step == g_state.slots[s].playhead && g_state.slots[s].note_count > 0u) {
                c = 0xFFFFu;
            } else if (active_mask & (1u << step)) {
                c = g_state.slots[s].frozen ? 0x5E7Fu : 0xFE60u;
            }
            g_sprite_buf[j * 32 + i] = c;
        }
    }
    hal_lcd_blit_rect(base_x, 215, 32, 4, g_sprite_buf);
}

static void render_ticker(void)
{
    hal_draw_text(8, 223, 224, g_state.ticker, 0xFFFFu, hud_bg_color(223), 0);
}

void ui_init(void)
{
    for (int i = 0; i < NUM_PARTICLES; i++) {
        g_particles[i].x = (int16_t)(16 + i * 28);
        g_particles[i].y = (int16_t)(20 + (i * 19) % 120);
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
        if (g_state.slide_frames == 0u) render_boss_overlay();
        g_state.dirty_diorama = 0u;
        if (g_state.slide_frames > 0u) g_state.slide_frames--;
        if (g_state.shake_timer > 0u) g_state.shake_timer--;
        if (g_state.toll_ring_timer > 0u) g_state.toll_ring_timer--;
    } else {
        render_particles();
        g_boss_phase++;
        render_boss_overlay();
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
        static const int SLOT_X[3] = {64, 112, 160};
        for (uint8_t s = 0; s < NUM_PARTY_SLOTS; s++) {
            render_party_slot(s, SLOT_X[s]);
            if (g_state.slots[s].anim_timer > 0u) g_state.slots[s].anim_timer--;
        }
        g_state.dirty_party = 0u;
    }

    if (g_state.dirty_ticker) {
        render_ticker();
        g_state.dirty_ticker = 0u;
    }
}
