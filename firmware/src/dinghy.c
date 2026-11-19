/* SPDX-License-Identifier: GPL-3.0-only */
/* Fantasia: Generative Dungeon-Synth & 16-bit RPG Soundtrack Firmware for M-VAVE FM-1
 * Built on zvenson/dinghy v0.5 (GPL-3.0-only). */

#include "app_config.h"

#define APP_NAME    "FANTASIA"
#ifndef FELUCCA_ID
#define FELUCCA_ID  "FM-1_961"
#endif
#define STORE_MAGIC FANTASIA_STORE_MAGIC

static void app_idle(void);
#define LCD_IDLE app_idle

#ifndef STANDALONE_TEST
#include "platform.c"
#endif

#include "hal_adapter.h"
#include "fantasia_types.h"
#include "fantasia_zones.h"
#include "fantasia_engine.h"
#include "fantasia_dsp.h"
#include "fantasia_input.h"
#include "fantasia_assets.h"
#include "fantasia_ui.h"

#include "fantasia_zones.c"
#include "fantasia_engine.c"
#include "fantasia_dsp.c"
#include "fantasia_input.c"
#include "fantasia_ui.c"

static uint32_t g_sample_acc = 0u;
static uint8_t  g_idle_ok = 0u;

static void app_idle(void)
{
    if (g_idle_ok) {
        /* Background idle hook while LCD SPI DMA is active */
    }
}

static void app_audio(int32_t *out, uint32_t n)
{
    for (uint8_t s = 0; s < NUM_PARTY_SLOTS; s++) {
        if (g_state.slot_pending[s]) {
            dsp_voice_quick_release(s);
            g_state.slots[s] = g_state.staging_slots[s];
            g_state.slot_pending[s] = 0u;
        }
    }

    for (uint32_t i = 0; i < n; i++) {
        g_sample_acc += (uint32_t)g_state.settings.bpm * 24u;
        if (g_sample_acc >= 2646000u) {
            g_sample_acc -= 2646000u;
            engine_tick_ppqn();
        }
        int32_t l = 0, r = 0;
        dsp_render_sample(&l, &r);
        out[i * 2u]      += l;
        out[i * 2u + 1u] += r;
    }
}

static void app_main(void)
{
    dsp_init();
    engine_init_defaults();
    if (hal_settings_load(&g_state.settings, sizeof(FantasiaSaveBlob)) == 0 &&
        g_state.settings.magic == SETTINGS_MAGIC) {
        for (uint8_t s = 0; s < NUM_PARTY_SLOTS; s++) {
            g_state.slots[s] = g_state.settings.saved_slots[s];
        }
        const ZoneDef *z = zone_get(g_state.settings.zone_id);
        uint8_t r = engine_drone_root_midi();
        dsp_drone_set(r,
                      (uint8_t)(r + z->scale_semitones[2]),
                      (uint8_t)(r + z->scale_semitones[4]),
                      (g_state.settings.harmony >= 35u) ? 1u : 0u,
                      (g_state.settings.harmony >= 20u) ? 1u : 0u,
                      g_state.settings.mix_drone);
    }
    input_init();
    ui_init();

    uint32_t last_ui_ms = 0u;
    for (;;) {
        hal_platform_poll();
        input_process();
        uint32_t now = hal_ms();
        if ((now - last_ui_ms) >= UI_REDRAW_MS) {
            last_ui_ms = now;
            g_idle_ok = 1u;
            ui_render();
            g_idle_ok = 0u;
        }
    }
}
