/* SPDX-License-Identifier: GPL-3.0-only */
#include "castletunia_input.h"
#include "castletunia_engine.h"
#include "castletunia_dsp.h"
#include "hal_adapter.h"

static uint32_t g_prev_notes = 0u;
static uint8_t  g_glo_held = 0u;
static uint8_t  g_glo_enc_moved = 0u;

static uint8_t  g_rec_down = 0u;
static uint8_t  g_rec_hold_fired = 0u;
static uint32_t g_rec_down_ms = 0u;

void input_init(void)
{
    g_prev_notes = 0u;
    g_glo_held = 0u;
    g_glo_enc_moved = 0u;
    g_rec_down = 0u;
    g_rec_hold_fired = 0u;
}

static uint8_t status_led_val(uint8_t st, uint32_t t_ms)
{
    if (st == STATUS_FREEZE)  return 1u;
    if (st == STATUS_SLOW)    return (uint8_t)((t_ms >> 8) & 1u);
    if (st == STATUS_POISON)  return (uint8_t)((t_ms >> 7) & 1u);
    if (st == STATUS_BERSERK) return (uint8_t)((t_ms >> 5) & 1u);
    return 0u;
}

static uint32_t g_key_led_until[27];

static void update_leds(uint32_t now)
{
    uint32_t note_leds = g_state.active_note_leds;
    g_state.active_note_leds = 0u;
    if (note_leds != 0u) {
        for (uint32_t k = 0; k < 27u; k++) {
            if ((note_leds >> k) & 1u) g_key_led_until[k] = now + 100u;
        }
    }

    uint8_t buildup_pulse = (g_state.boss_buildup_steps > 0u) && (((now >> 6) & 1u) != 0u);

    for (uint32_t k = 0; k < 27u; k++) {
        uint8_t is_root = (k == 0u || k == 7u || k == 14u || k == 21u);
        uint8_t held    = (uint8_t)((g_prev_notes >> k) & 1u);
        uint8_t bright  = held;

        if (g_state.playing) {
            uint8_t note_on = ((int32_t)(g_key_led_until[k] - now) > 0);
            if (note_on || (is_root && (g_state.beat1_flash > 0u || buildup_pulse))) {
                bright = 1u;
            }
        }

        if (bright) {
            hal_led_key(KEY_LED(k), 1);
            hal_led_glow(KEY_LED(k), 0);
        } else if (is_root) {
            hal_led_key(KEY_LED(k), 0);
            hal_led_glow(KEY_LED(k), 1);
        } else {
            hal_led_key(KEY_LED(k), 0);
            hal_led_glow(KEY_LED(k), 0);
        }
    }

    if (g_state.beat1_flash > 0u) g_state.beat1_flash--;

    hal_led_key(BTN_FX,  status_led_val(g_state.slots[0].status, now));
    hal_led_key(BTN_SEL, status_led_val(g_state.slots[1].status, now));
    hal_led_key(BTN_ENV, status_led_val(g_state.slots[2].status, now));
    hal_led_key(BTN_LFO, g_state.spells.haste_active ? (int)((now >> 6) & 1u) : 0);
    hal_led_key(BTN_EDIT, g_state.spells.mirror_active);
    hal_led_key(BTN_SEQ, g_state.spells.resolve_armed);
    hal_led_key(BTN_SAVE, g_state.stage_locked);
    hal_led_key(BTN_PLAY, g_state.playing ? (g_state.beat1_flash > 0u ? 0 : 1) : 0);
}

void input_process(void)
{
    uint32_t now = hal_ms();
    engine_poll_status_timers();

    /* 1. 27-Key Scanner */
    uint32_t keys = fm1_in.notes;
    uint32_t down = keys & ~g_prev_notes;
    uint32_t up = g_prev_notes & ~keys;
    g_prev_notes = keys;

    for (uint8_t k = 0; k < 27u; k++) {
        if ((down >> k) & 1u) engine_on_key_down(k);
        if ((up >> k) & 1u) engine_on_key_up(k);
    }

    /* 2. 7 Rotary Encoders + BTN_GLO Shift Layer */
    for (uint8_t e = 0; e < 7u; e++) {
        int32_t d = hal_enc_take(e);
        if (d != 0) {
            if (g_glo_held) g_glo_enc_moved = 1u;
            engine_adjust_macro(e, d, g_glo_held);
        }
    }

    /* 3. 14 Backlit Function Buttons */
    uint32_t rel = 0u;
    uint32_t press = hal_input_edges(&rel);

    if (press & (1u << BTN_OCT_DOWN)) engine_adjust_macro(ENC_SELECT, -1, 1);
    if (press & (1u << BTN_OCT_UP))   engine_adjust_macro(ENC_SELECT, +1, 1);
    if (press & (1u << BTN_FX))       engine_toggle_freeze(0u);
    if (press & (1u << BTN_SEL))      engine_toggle_freeze(1u);
    if (press & (1u << BTN_ENV))      engine_toggle_freeze(2u);
    if (press & (1u << BTN_LFO))      engine_spell_haste_toggle();
    if (press & (1u << BTN_EDIT))     engine_spell_mirror_toggle();
    if (press & (1u << BTN_ARP))      engine_spell_shatter();
    if (press & (1u << BTN_SEQ))      engine_spell_resolve();

    if (press & (1u << BTN_PLAY)) {
        g_state.playing ^= 1u;
        if (!g_state.playing) {
            dsp_all_notes_off();
            engine_set_ticker("> PAUSED");
        } else {
            engine_set_ticker("> PLAYING");
        }
    }

    if (press & (1u << BTN_SAVE)) {
        engine_toggle_stage_lock();
    }

    /* BTN_GLO: Hold+Encoder Shift vs Tap-Release SPELL: TOLL */
    if (press & (1u << BTN_GLO)) {
        g_glo_held = 1u;
        g_glo_enc_moved = 0u;
    }
    if (rel & (1u << BTN_GLO)) {
        g_glo_held = 0u;
        if (!g_glo_enc_moved) {
            engine_spell_toll();
        }
    }

    /* BTN_HOME: Skip to next Party member without starting playback */
    if (press & (1u << BTN_HOME)) {
        engine_advance_next_slot();
    }

    /* BTN_REC: Cancel capture, Tap clear next_slot, Hold 600ms clear all unfrozen */
    if (press & (1u << BTN_REC)) {
        if (g_state.capture.active) {
            seed_cancel_capture();
            engine_set_ticker("> CAPTURE CANCELED");
        } else {
            g_rec_down = 1u;
            g_rec_hold_fired = 0u;
            g_rec_down_ms = now;
        }
    }
    if (g_rec_down && !g_rec_hold_fired && (now - g_rec_down_ms) >= BTN_HOLD_MS) {
        g_rec_hold_fired = 1u;
        engine_clear_all_unfrozen();
    }
    if (rel & (1u << BTN_REC)) {
        if (g_rec_down && !g_rec_hold_fired) {
            engine_clear_slot(g_state.next_slot);
        }
        g_rec_down = 0u;
    }

    update_leds(now);
}
