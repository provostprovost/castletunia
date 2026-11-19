/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef FANTASIA_DSP_H
#define FANTASIA_DSP_H

#include <stdint.h>
#include "fantasia_types.h"

void dsp_init(void);
void dsp_drone_set(uint8_t root_midi, uint8_t fifth_midi, uint8_t fifth_enable, uint8_t level);
void dsp_note_on(uint8_t slot, uint8_t midi_note, uint16_t vel_q15, uint16_t gate_ticks);
void dsp_voice_quick_release(uint8_t slot);
void dsp_toll_trigger(uint8_t zone_id, uint8_t root_midi);
void dsp_perc_trigger(uint8_t is_high, uint16_t vel_q15);
void dsp_all_notes_off(void);
void dsp_tick_gates(void);
void dsp_render_sample(int32_t *out_l, int32_t *out_r);

#endif /* FANTASIA_DSP_H */
