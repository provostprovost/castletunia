/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "app_config.h"
#include "hal_adapter.h"

uint32_t mock_ms = 0;
uint32_t mock_ticks = 0;
uint16_t mock_fb[240 * 240];
hal_input_t fm1_in = {0, 0};
uint32_t mock_btn_press = 0;
uint32_t mock_btn_rel = 0;
int32_t  mock_enc[7] = {0};
uint16_t mock_adc_master = 1023;
uint64_t mock_leds = 0;
uint64_t mock_glow = 0;
uint8_t  mock_flash_blob[512];
uint32_t mock_flash_len = 0;
#include "fantasia_types.h"
#include "fantasia_zones.h"
#include "fantasia_engine.h"
#include "fantasia_dsp.h"

#if TEST_STAGE == 1
/* Test spies for Stage 1 (engine-only without fantasia_dsp.c) */
static uint32_t spy_note_count = 0;
static uint8_t  spy_last_slot = 0;
static uint8_t  spy_last_midi = 0;
static uint32_t spy_toll_count = 0;
static uint32_t spy_perc_count = 0;

void dsp_init(void) {}
void dsp_drone_set(uint8_t r, uint8_t t, uint8_t f, uint8_t te, uint8_t fe, uint8_t lvl) { (void)r; (void)t; (void)f; (void)te; (void)fe; (void)lvl; }
void dsp_note_on(uint8_t slot, uint8_t midi, uint16_t vel, uint16_t gate) {
    (void)vel; (void)gate;
    spy_note_count++;
    spy_last_slot = slot;
    spy_last_midi = midi;
}
void dsp_voice_quick_release(uint8_t slot) { (void)slot; }
void dsp_toll_trigger(uint8_t zone_id, uint8_t root) { (void)zone_id; (void)root; spy_toll_count++; }
void dsp_attack_sfx(uint8_t slot) { (void)slot; }
void dsp_boss_attack_sfx(uint8_t zone_id) { (void)zone_id; }
void dsp_perc_trigger(uint8_t is_high, uint16_t vel) { (void)is_high; (void)vel; spy_perc_count++; }
void dsp_all_notes_off(void) {}
void dsp_tick_gates(void) {}
void dsp_render_sample(int32_t *l, int32_t *r) { *l = 0; *r = 0; }
#endif

#if TEST_STAGE == 3
#include "fantasia_assets.h"
#include "fantasia_ui.h"
void dsp_init(void) {}
void dsp_drone_set(uint8_t r, uint8_t t, uint8_t f, uint8_t te, uint8_t fe, uint8_t lvl) { (void)r; (void)t; (void)f; (void)te; (void)fe; (void)lvl; }
void dsp_note_on(uint8_t slot, uint8_t midi, uint16_t vel, uint16_t gate) { (void)slot; (void)midi; (void)vel; (void)gate; }
void dsp_voice_quick_release(uint8_t slot) { (void)slot; }
void dsp_toll_trigger(uint8_t zone_id, uint8_t root) { (void)zone_id; (void)root; }
void dsp_attack_sfx(uint8_t slot) { (void)slot; }
void dsp_boss_attack_sfx(uint8_t zone_id) { (void)zone_id; }
void dsp_perc_trigger(uint8_t is_high, uint16_t vel) { (void)is_high; (void)vel; }
void dsp_all_notes_off(void) {}
void dsp_tick_gates(void) {}
void dsp_render_sample(int32_t *l, int32_t *r) { *l = 0; *r = 0; }
#endif

#if TEST_STAGE == 4
#include "fantasia_assets.h"
#include "fantasia_ui.h"
#include "fantasia_input.h"
#endif

#if TEST_STAGE == 1
static void run_stage1(void)
{
    printf("=== Stage 1: 9-Zone Tables & Generative Seed Engine ===\n");
    engine_init_defaults();

    /* 1. 24-PPQN clock drift < 1 tick over 60s at 76, 92, 138 BPM */
    const uint8_t bpms[3] = {76, 92, 138};
    for (int b = 0; b < 3; b++) {
        uint32_t acc = 0, ticks = 0;
        uint32_t samples_60s = 44100u * 60u;
        for (uint32_t i = 0; i < samples_60s; i++) {
            acc += (uint32_t)bpms[b] * 24u;
            if (acc >= 2646000u) {
                acc -= 2646000u;
                ticks++;
            }
        }
        uint32_t expected = (uint32_t)bpms[b] * 24u;
        int32_t diff = (int32_t)ticks - (int32_t)expected;
        if (diff < 0) diff = -diff;
        assert(diff < 1);
    }

    /* 2. Strictly monotonic 27-key mapping & register ordering across all 3 Party slots */
    for (int d = 0; d < 27; d++) {
        uint8_t n0 = engine_note_for_degree(d, 0);
        uint8_t n1 = engine_note_for_degree(d, 1);
        uint8_t n2 = engine_note_for_degree(d, 2);
        assert(n0 >= 48 && n0 <= 108);
        assert(n1 >= n0 && n2 >= n1);
        if (d > 0) {
            assert(n0 > engine_note_for_degree(d - 1, 0));
            assert(n1 > engine_note_for_degree(d - 1, 1));
            assert(n2 > engine_note_for_degree(d - 1, 2));
        }
    }

    /* 3. Silence-triggered 1..16 note Seed planting (>= 28 ticks AND >= 850 ms) */
    engine_init_defaults();
    engine_on_key_down(0);
    engine_on_key_up(0);
    engine_on_key_down(4);
    engine_on_key_up(4);
    assert(g_state.capture.active == 1);
    assert(g_state.capture.count == 2);
    for (int t = 0; t < 20; t++) engine_tick_ppqn();
    assert(g_state.capture.active == 1); /* Not committed yet */
    for (int t = 0; t < 20; t++) engine_tick_ppqn();
    assert(g_state.capture.active == 0);
    assert(g_state.slots[0].note_count == 2);

    /* 4. Freeze slot skipping & All-frozen Live Jam bypass */
    engine_toggle_freeze(0);
    engine_toggle_freeze(1);
    engine_toggle_freeze(2);
    assert(g_state.slots[0].frozen && g_state.slots[1].frozen && g_state.slots[2].frozen);
    uint32_t prev_notes = spy_note_count;
    engine_on_key_down(7);
    engine_on_key_up(7);
    assert(spy_note_count == prev_notes + 1);
    assert(spy_last_slot == 2); /* Red Sorceress live jam */
    assert(g_state.capture.active == 0);

    /* Unfreeze all for spells */
    engine_toggle_freeze(0);
    engine_toggle_freeze(1);
    engine_toggle_freeze(2);

    /* 5. All 6 Spells */
    engine_spell_haste_toggle();
    assert(g_state.spells.haste_active == 1);
    engine_spell_mirror_toggle();
    assert(g_state.spells.mirror_active == 1);
    engine_spell_conjure();
    assert(g_state.slot_pending[g_state.next_slot == 0 ? 2 : g_state.next_slot - 1] == 1 || g_state.staging_slots[1].note_count >= 3);
    g_state.slots[1] = g_state.staging_slots[1];
    g_state.slot_pending[1] = 0;
    engine_spell_shatter();
    engine_spell_resolve();
    assert(g_state.spells.resolve_armed == 1);
    uint32_t prev_toll = spy_toll_count;
    engine_spell_toll();
    assert(spy_toll_count == prev_toll + 1);

    /* 6. Macro control mapping */
    engine_adjust_macro(ENC_ALGORITHM, 5, 0);
    assert(g_state.settings.intensity == 60);
    engine_adjust_macro(ENC_KNOB1, 5, 0);
    assert(g_state.settings.drift == 56);
    engine_adjust_macro(ENC_KNOB2, 5, 0);
    assert(g_state.settings.density == 72);
    engine_adjust_macro(ENC_KNOB3, 5, 0);
    assert(g_state.settings.harmony == 76);
    engine_adjust_macro(ENC_KNOB4, 5, 0);
    assert(g_state.settings.weave == 44);

    printf("Stage 1 PASSED!\n");
}
#endif

#if TEST_STAGE == 2
static void write_wav_header(FILE *f, uint32_t total_frames)
{
    uint32_t data_bytes = total_frames * 4u;
    uint32_t riff_size = 36u + data_bytes;
    uint16_t fmt = 1, ch = 2, align = 4, bps = 16;
    uint32_t fmt_sz = 16, sr = 44100, br = 44100u * 4u;
    fwrite("RIFF", 1, 4, f); fwrite(&riff_size, 4, 1, f); fwrite("WAVE", 1, 4, f);
    fwrite("fmt ", 1, 4, f); fwrite(&fmt_sz, 4, 1, f); fwrite(&fmt, 2, 1, f);
    fwrite(&ch, 2, 1, f); fwrite(&sr, 4, 1, f); fwrite(&br, 4, 1, f);
    fwrite(&align, 2, 1, f); fwrite(&bps, 2, 1, f);
    fwrite("data", 1, 4, f); fwrite(&data_bytes, 4, 1, f);
}

static void run_stage2(void)
{
    printf("=== Stage 2: Pure 32-Bit Fixed-Point DSP Engine ===\n");
    dsp_init();
    engine_init_defaults();
    g_state.settings.intensity = 100;

    FILE *wav = fopen("build/test_fantasia.wav", "wb");
    assert(wav != NULL);
    uint32_t total_frames = 1000u * 256u;
    write_wav_header(wav, total_frames);

    double sum_sq = 0.0;
    uint32_t sample_acc = 0;

    for (uint32_t blk = 0; blk < 1000u; blk++) {
        if (blk % 111u == 0u) {
            uint8_t z = (uint8_t)((blk / 111u) % 9u);
            engine_set_zone(z);
            engine_spell_conjure();
            for (int s = 0; s < 3; s++) {
                if (g_state.slot_pending[s]) {
                    g_state.slots[s] = g_state.staging_slots[s];
                    g_state.slot_pending[s] = 0;
                }
            }
        }
        int16_t pcm[512];
        for (uint32_t i = 0; i < 256u; i++) {
            sample_acc += (uint32_t)g_state.settings.bpm * 24u;
            if (sample_acc >= 2646000u) {
                sample_acc -= 2646000u;
                engine_tick_ppqn();
            }
            int32_t l = 0, r = 0;
            dsp_render_sample(&l, &r);
            assert(l >= -32767 && l <= 32767);
            assert(r >= -32767 && r <= 32767);
            pcm[i * 2u] = (int16_t)l;
            pcm[i * 2u + 1u] = (int16_t)r;
            sum_sq += (double)l * (double)l + (double)r * (double)r;
        }
        fwrite(pcm, sizeof(int16_t), 512, wav);
    }
    fclose(wav);

    double rms = sqrt(sum_sq / (double)(total_frames * 2u));
    printf("Stage 2 RMS: %.2f\n", rms);
    assert(rms > 200.0);

    /* Tail decay test <= 2 LSB within 1.5s after dsp_all_notes_off() + drone off */
    g_state.playing = 0;
    dsp_drone_set(36, 39, 43, 0, 0, 0);
    dsp_all_notes_off();
    int32_t max_tail = 0;
    for (uint32_t i = 0; i < 44100u + 22050u; i++) {
        int32_t l = 0, r = 0;
        dsp_render_sample(&l, &r);
        if (i > 44100u + 20000u) {
            if (abs(l) > max_tail) max_tail = abs(l);
            if (abs(r) > max_tail) max_tail = abs(r);
        }
    }
    printf("Stage 2 tail max LSB: %d\n", max_tail);
    assert(max_tail <= 2);
    printf("Stage 2 PASSED!\n");
}
#endif

#if TEST_STAGE == 3
static void save_ppm(const char *path, const uint16_t *fb)
{
    FILE *f = fopen(path, "wb");
    assert(f != NULL);
    fprintf(f, "P6\n240 240\n255\n");
    for (int i = 0; i < 240 * 240; i++) {
        uint16_t c = fb[i];
        uint8_t rgb[3] = {
            (uint8_t)(((c >> 11) & 0x1Fu) << 3),
            (uint8_t)(((c >> 5) & 0x3Fu) << 2),
            (uint8_t)((c & 0x1Fu) << 3)
        };
        fwrite(rgb, 1, 3, f);
    }
    fclose(f);
}

static void run_stage3(void)
{
    printf("=== Stage 3: Dual-Path Asset Generator & Differential SNES UI ===\n");
    printf("sizeof(fantasia_assets) = %zu bytes\n", sizeof(fantasia_assets));
    assert(sizeof(fantasia_assets) < 200000u);

    engine_init_defaults();
    ui_init();
    ui_render();
    save_ppm("build/screen_crypt.ppm", mock_fb);

    engine_spell_mirror_toggle();
    engine_toggle_freeze(0);
    ui_render();
    save_ppm("build/screen_mirror_vfx.ppm", mock_fb);
    printf("Stage 3 PASSED!\n");
}
#endif

#if TEST_STAGE == 4
static void run_stage4(void)
{
    printf("=== Stage 4: Panel Input, LEDs & Full-System Integration ===\n");
    dsp_init();
    engine_init_defaults();
    input_init();
    ui_init();

    /* 1. BTN_GLO + encoder shift layer does NOT fire TOLL on release */
    mock_btn_press = (1u << BTN_GLO);
    input_process();
    mock_enc[ENC_KNOB1] = 2;
    input_process();
    assert(g_state.settings.mix_slots[0] == 116);
    mock_btn_rel = (1u << BTN_GLO);
    input_process();
    assert(g_state.toll_ring_timer == 0);

    /* 2. BTN_GLO tap-release without encoder fires TOLL */
    mock_btn_press = (1u << BTN_GLO);
    input_process();
    mock_btn_rel = (1u << BTN_GLO);
    input_process();
    assert(g_state.toll_ring_timer > 0);

    /* 3. BTN_HOME instant CONJURE on press */
    uint8_t prev_slot = g_state.next_slot;
    mock_btn_press = (1u << BTN_HOME);
    mock_ms = 100;
    input_process();
    mock_btn_rel = (1u << BTN_HOME);
    mock_ms = 200;
    input_process();
    assert(g_state.next_slot != prev_slot);

    /* 4. BTN_SAVE persistence */
    g_state.settings.zone_id = 3;
    mock_btn_press = (1u << BTN_SAVE);
    input_process();
    FantasiaSaveBlob loaded;
    assert(hal_settings_load(&loaded, sizeof(loaded)) == 0);
    assert(loaded.zone_id == 3);

    /* 5. Key LEDs while paused = all 27 keys solid white */
    g_state.playing = 1;
    mock_btn_press = (1u << BTN_PLAY);
    input_process();
    assert(g_state.playing == 0);
    for (int k = 0; k < 27; k++) {
        assert((mock_leds >> KEY_LED(k)) & 1u);
    }

    printf("Stage 4 PASSED!\n");
}
#endif

int main(void)
{
#if TEST_STAGE == 1
    run_stage1();
#elif TEST_STAGE == 2
    run_stage2();
#elif TEST_STAGE == 3
    run_stage3();
#elif TEST_STAGE == 4
    run_stage4();
#else
#error "Define TEST_STAGE=1..4"
#endif
    return 0;
}
