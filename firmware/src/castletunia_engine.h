/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef CASTLETUNIA_ENGINE_H
#define CASTLETUNIA_ENGINE_H

#include "castletunia_types.h"

extern CastletuniaState g_state;

uint32_t engine_prng(void);
void engine_init_defaults(void);
void engine_set_ticker(const char *msg);
void engine_set_zone(uint8_t zone_id);
uint8_t engine_note_for_degree(int32_t deg, uint8_t slot);
uint8_t engine_drone_root_midi(void);

void engine_tick_ppqn(void);
void engine_on_key_down(uint8_t key_idx);
void engine_on_key_up(uint8_t key_idx);
void seed_commit_capture(void);
void seed_cancel_capture(void);

void engine_toggle_freeze(uint8_t slot);
void engine_poll_status_timers(void);
void engine_clear_slot(uint8_t slot);
void engine_clear_all_unfrozen(void);
void engine_advance_next_slot(void);

void engine_spell_haste_toggle(void);
void engine_spell_mirror_toggle(void);
void engine_spell_shatter(void);
void engine_spell_resolve(void);
void engine_spell_conjure(void);
void engine_spell_toll(void);

void engine_adjust_macro(uint8_t enc_id, int32_t delta, uint8_t glo_held);

#endif /* CASTLETUNIA_ENGINE_H */
