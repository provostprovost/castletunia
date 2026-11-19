/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef HAL_ADAPTER_H
#define HAL_ADAPTER_H

#include <stdint.h>
#include <stddef.h>

/* Pure integer 32-bit phase -> Q15 sine (-32767..32767), parabolic approximation (~0.1% THD) */
static inline int16_t hal_sine_q15(uint32_t phase)
{
    int32_t x = (int32_t)phase >> 16;
    int32_t ax = x < 0 ? -x : x;
    int32_t y = (x * (32768 - ax)) >> 13;
    int32_t ay = y < 0 ? -y : y;
    y += ((((y * ay) >> 15) - y) * 7373) >> 15;
    if (y > 32767) y = 32767;
    if (y < -32767) y = -32767;
    return (int16_t)y;
}

#ifdef STANDALONE_TEST

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern uint32_t mock_ms;
extern uint32_t mock_ticks;
#define FM1_TICKS_PER_US 24u

static inline uint32_t hal_ms(void) { return mock_ms; }
static inline uint32_t hal_ticks(void) { return mock_ticks; }
static inline void hal_platform_poll(void) {}

extern uint16_t mock_fb[240 * 240];
static inline void hal_lcd_blit_rect(int x, int y, int w, int h, const uint16_t *buf)
{
    for (int j = 0; j < h; j++) {
        int yy = y + j;
        if (yy < 0 || yy >= 240) continue;
        for (int i = 0; i < w; i++) {
            int xx = x + i;
            if (xx < 0 || xx >= 240) continue;
            mock_fb[yy * 240 + xx] = buf[j * w + i];
        }
    }
}

static inline void hal_lcd_fill(int x, int y, int w, int h, uint16_t c)
{
    for (int j = 0; j < h; j++) {
        int yy = y + j;
        if (yy < 0 || yy >= 240) continue;
        for (int i = 0; i < w; i++) {
            int xx = x + i;
            if (xx < 0 || xx >= 240) continue;
            mock_fb[yy * 240 + xx] = c;
        }
    }
}

static inline void hal_draw_text(int x, int y, int w, const char *txt, uint16_t fg, uint16_t bg, int align)
{
    (void)x; (void)y; (void)w; (void)txt; (void)fg; (void)bg; (void)align;
}

typedef struct {
    uint32_t notes;
    uint32_t buttons;
} hal_input_t;

extern hal_input_t fm1_in;
extern uint32_t mock_btn_press;
extern uint32_t mock_btn_rel;
extern int32_t  mock_enc[7];
extern uint16_t mock_adc_master;
extern uint64_t mock_leds;
extern uint64_t mock_glow;

static inline uint32_t hal_input_edges(uint32_t *rel)
{
    uint32_t p = mock_btn_press;
    mock_btn_press = 0;
    if (rel) {
        *rel = mock_btn_rel;
        mock_btn_rel = 0;
    }
    return p;
}

static inline int32_t hal_enc_take(int enc)
{
    if (enc < 0 || enc >= 7) return 0;
    int32_t v = mock_enc[enc];
    mock_enc[enc] = 0;
    return v;
}

static inline uint16_t hal_adc_master(void) { return mock_adc_master; }

static inline void hal_led_key(uint32_t id, int on)
{
    if (id < 64u) {
        if (on) mock_leds |= ((uint64_t)1 << id);
        else mock_leds &= ~((uint64_t)1 << id);
    }
}

static inline void hal_led_glow(uint32_t id, int on)
{
    if (id < 64u) {
        if (on) mock_glow |= ((uint64_t)1 << id);
        else mock_glow &= ~((uint64_t)1 << id);
    }
}

extern uint8_t mock_flash_blob[512];
extern uint32_t mock_flash_len;

static inline int hal_settings_save(const void *ptr, uint32_t sz)
{
    if (sz > sizeof(mock_flash_blob)) return -1;
    memcpy(mock_flash_blob, ptr, sz);
    mock_flash_len = sz;
    return 0;
}

static inline int hal_settings_load(void *ptr, uint32_t sz)
{
    if (mock_flash_len != sz || sz == 0) return -1;
    memcpy(ptr, mock_flash_blob, sz);
    return 0;
}

#define ENC_SELECT    0
#define ENC_ALGORITHM 1
#define ENC_KNOB1     2
#define ENC_KNOB2     3
#define ENC_KNOB3     4
#define ENC_KNOB4     5
#define ENC_PRESETS   6

#define BTN_OCT_DOWN  0
#define BTN_OCT_UP    1
#define BTN_FX        2
#define BTN_SEL       3
#define BTN_ENV       4
#define BTN_LFO       5
#define BTN_EDIT      6
#define BTN_GLO       7
#define BTN_HOME      8
#define BTN_SAVE      9
#define BTN_ARP       10
#define BTN_SEQ       11
#define BTN_PLAY      12
#define BTN_REC       13
#define KEY_LED(k)    (14u + (uint32_t)(k))

#else /* Dinghy hardware or DINGHY_HOST */

static inline uint32_t hal_ms(void) { return fm1_ms; }

#ifdef DINGHY_HOST
#define FM1_TICKS_PER_US 24u
static inline uint32_t hal_ticks(void) { return 0; }
static inline uint16_t hal_adc_master(void) { return 1023; }
#else
static inline uint32_t hal_ticks(void) { return fm1_ticks(); }
static inline uint16_t hal_adc_master(void)
{
    int v = fm1_adc_read(FM1_ADC_MASTER);
    return (v < 0) ? 1023u : (uint16_t)v;
}
#endif

static inline void hal_platform_poll(void) { platform_poll(); }
static inline uint32_t hal_input_edges(uint32_t *rel) { return fm1_input_edges(rel); }
static inline int32_t hal_enc_take(int enc) { return fm1_enc_take((uint32_t)enc); }
static inline void hal_led_key(uint32_t id, int on) { fm1_led_key(id, on); }
static inline void hal_led_glow(uint32_t id, int on) { fm1_led_key_glow(id, on); }
static inline int hal_settings_save(const void *ptr, uint32_t sz) { return settings_save(ptr, sz); }
static inline int hal_settings_load(void *ptr, uint32_t sz) { return settings_load(ptr, sz); }
static inline void hal_lcd_fill(int x, int y, int w, int h, uint16_t c)
{
    lcd_fill((uint32_t)x, (uint32_t)y, (uint32_t)w, (uint32_t)h, c);
}

static uint16_t g_hal_swap_buf[240 * 8];
static inline void hal_lcd_blit_rect(int x, int y, int w, int h, const uint16_t *buf)
{
    int row = 0;
    if (w <= 0 || h <= 0 || w > 240) return;
    while (row < h) {
        int chunk = h - row;
        if (chunk > 8) chunk = 8;
        int n = w * chunk;
        for (int i = 0; i < n; i++) {
            uint16_t c = buf[row * w + i];
            g_hal_swap_buf[i] = (uint16_t)((c >> 8) | (c << 8));
        }
        lcd_blit((uint32_t)x, (uint32_t)(y + row), (uint32_t)w, (uint32_t)chunk, g_hal_swap_buf);
        row += chunk;
    }
}

static inline void hal_draw_text(int x, int y, int w, const char *txt, uint16_t fg, uint16_t bg, int align)
{
    int32_t tw = text_w(&FONT_S, txt), tx = 0;
    if (align == 1) tx = (w - tw) / 2;
    else if (align == 2) tx = w - tw;
    cv_begin((uint32_t)w, FONT_S.h, bg);
    cv_text(tx, 0, &FONT_S, txt, fg);
    cv_blit((uint32_t)x, (uint32_t)y);
    lcd_sync();
}

#endif /* STANDALONE_TEST */

#endif /* HAL_ADAPTER_H */
