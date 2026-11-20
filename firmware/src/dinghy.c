/* SPDX-License-Identifier: GPL-3.0-only */
/* Castletunia: Generative Dungeon-Synth & 16-bit RPG Soundtrack Firmware for M-VAVE FM-1
 * Built on zvenson/dinghy v0.5 (GPL-3.0-only). */

#include "app_config.h"

#define APP_NAME    "CASTLETUNIA"
#ifndef FELUCCA_ID
#define FELUCCA_ID  "FM-1_961"
#endif
#define STORE_MAGIC CASTLETUNIA_STORE_MAGIC

static void app_idle(void);
#define LCD_IDLE app_idle

#ifndef STANDALONE_TEST
#include "platform.c"
#endif

#include "hal_adapter.h"
#include "castletunia_types.h"
#include "castletunia_zones.h"
#include "castletunia_engine.h"
#include "castletunia_dsp.h"
#include "castletunia_input.h"
#include "castletunia_assets.h"
#include "castletunia_ui.h"

#include "castletunia_zones.c"
#include "castletunia_engine.c"
#include "castletunia_dsp.c"
#include "castletunia_input.c"
#include "castletunia_ui.c"

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
    input_init();
#ifndef DINGHY_HOST
    ui_render_splash();
    {
        uint32_t splash_start = hal_ms();
        hal_platform_poll();
        (void)hal_input_edges(0);
        uint32_t init_keys = fm1_in.notes;
        while ((hal_ms() - splash_start) < 3000u) {
            hal_platform_poll();
            if (hal_input_edges(0) != 0u || (fm1_in.notes & ~init_keys) != 0u) {
                break;
            }
        }
        /* Pick a random starting stage using splash timing + encoder detent states */
        uint32_t ent = hal_ms() ^ (fm1_in.frames << 7) ^ fm1_in.notes;
        for (uint8_t p = 0; p < 7u; p++) ent ^= ((uint32_t)fm1_in.enc_rest[p] << (p * 3u));
        engine_set_zone((uint8_t)(ent % NUM_ZONES));
        g_state.slide_frames = 0u;
    }
#endif
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
