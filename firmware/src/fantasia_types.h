/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef FANTASIA_TYPES_H
#define FANTASIA_TYPES_H

#include <stdint.h>
#include "app_config.h"

typedef struct {
    uint8_t  mod_ratio_num;   /* Modulator ratio numerator (1..8) */
    uint8_t  mod_ratio_den;   /* Modulator ratio denominator (1..8) */
    uint16_t mod_index_q15;   /* Base FM modulation index (0..65535) */
    uint16_t feedback_q15;    /* Op2 self-feedback (0..32767) */
    uint16_t attack_samples;  /* Attack envelope duration in samples (64..2000) */
    uint16_t decay_q15;       /* Q15 decay multiplier per sample (30000..32767) */
    uint16_t sustain_q15;     /* Q15 sustain level (0..32767) */
    uint16_t release_q15;     /* Q15 release decay multiplier (28000..32700) */
} FmPatch;

typedef struct {
    uint16_t pitch_start_inc; /* Initial pitch increment */
    uint16_t pitch_end_inc;   /* Target pitch increment */
    uint16_t pitch_sweep_q15; /* Q15 pitch sweep rate per sample */
    uint16_t noise_mix_q15;   /* Q15 LFSR noise mix level (0..32767) */
    uint8_t  fm_ratio;        /* Percussion FM modulator ratio */
    uint16_t fm_index_q15;    /* Q15 percussion FM modulation index */
    uint16_t decay_q15;       /* Q15 amplitude decay multiplier (32715..32761) */
} PercPatch;

typedef struct {
    int8_t   scale_deg[MAX_SEED_NOTES]; /* 0..26 relative scale degree, -1 = rest */
    uint8_t  step_pos[MAX_SEED_NOTES];  /* 0..15 step position */
    uint8_t  note_count;                /* 0 = empty, 1..8 = active */
    uint8_t  base_length;               /* 4, 8, or 16 steps */
    uint8_t  weave_length;              /* 5, 7, 11, or 13 steps */
    uint8_t  playhead;                  /* Current step index */
    uint8_t  frozen;                    /* 1 = locked from Drift & overwrite */
    uint8_t  fade_gain_q8;              /* 0..255 smooth fade-in/out */
    uint8_t  anim_timer;                /* Frames remaining for sprite hop/cast */
} SeedSlot;

typedef struct {
    uint8_t  scale_deg[MAX_SEED_NOTES];
    uint32_t rel_tick[MAX_SEED_NOTES];
    uint32_t start_tick;
    uint8_t  count;
    uint8_t  active;
} CaptureBuf;

typedef struct {
    uint8_t  haste_active;
    uint8_t  mirror_active;
    uint8_t  resolve_armed;
    uint8_t  toll_trigger;
    uint8_t  conjure_trigger;
} SpellState;

/* Compact scalar settings (<= 64 bytes) */
typedef struct {
    uint8_t  magic;                      /* SETTINGS_MAGIC (0xFA) */
    uint8_t  version;                    /* 1 */
    uint8_t  zone_id;                    /* 0..8 */
    uint8_t  root_semitone;              /* 0..11 (default 2 = D) */
    uint8_t  bpm;                        /* 55..160 */
    uint8_t  intensity;                  /* 0..100 */
    uint8_t  drift;                      /* 0..100 */
    uint8_t  density;                    /* 0..100 */
    uint8_t  harmony;                    /* 0..100 */
    uint8_t  weave;                      /* 0..100 */
    uint8_t  mix_drone;                  /* 0..127 */
    uint8_t  mix_slots[NUM_PARTY_SLOTS]; /* 0..127 */
    uint8_t  mix_perc;                   /* 0..127 */
    uint8_t  reverb_override;            /* 0..127 (0 = use Zone default) */
} FantasiaSettings;

typedef struct {
    uint8_t  magic;                      /* SETTINGS_MAGIC (0xFA) */
    uint8_t  version;                    /* 1 */
    uint8_t  zone_id;                    /* 0..8 */
    uint8_t  root_semitone;              /* 0..11 (default 2 = D) */
    uint8_t  bpm;                        /* 55..160 */
    uint8_t  intensity;                  /* 0..100 */
    uint8_t  drift;                      /* 0..100 */
    uint8_t  density;                    /* 0..100 */
    uint8_t  harmony;                    /* 0..100 */
    uint8_t  weave;                      /* 0..100 */
    uint8_t  mix_drone;                  /* 0..127 */
    uint8_t  mix_slots[NUM_PARTY_SLOTS]; /* 0..127 */
    uint8_t  mix_perc;                   /* 0..127 */
    uint8_t  reverb_override;            /* 0..127 */
    SeedSlot saved_slots[NUM_PARTY_SLOTS];
} FantasiaSaveBlob;

typedef struct {
    const char *name;
    uint8_t     subgenre_id;
    uint8_t     default_bpm;
    uint8_t     scale_semitones[7];
    uint8_t     mid_prog[4];
    uint8_t     high_prog[4];
    uint8_t     reverb_mix;
    uint16_t    reverb_fb;
    FmPatch     party_patches[3];
    PercPatch   perc_low;
    PercPatch   perc_high;
    uint16_t    low_mask_march;
    uint16_t    high_mask_march;
    uint16_t    low_mask_battle;
    uint16_t    high_mask_battle;
    FmPatch     toll_patch;
} ZoneDef;

typedef struct {
    SeedSlot          slots[NUM_PARTY_SLOTS];
    SeedSlot          staging_slots[NUM_PARTY_SLOTS];
    volatile uint8_t  slot_pending[NUM_PARTY_SLOTS];
    uint8_t           cross_accent_bars[NUM_PARTY_SLOTS];
    CaptureBuf        capture;
    SpellState        spells;
    FantasiaSaveBlob  settings;
    uint8_t           next_slot;
    uint8_t           ui_dirty;
    uint8_t           cpu_load_pct;

    /* Live runtime state */
    uint8_t           playing;
    uint32_t          prng_state;
    uint32_t          total_ticks;
    uint8_t           tick_in_16th;
    uint8_t           step_16th;
    uint8_t           bar_count;
    uint8_t           active_chord_deg;
    uint32_t          silence_ticks;
    uint32_t          silence_ms;
    uint32_t          last_silence_ms;
    uint32_t          held_keys_mask;

    /* LED & UI feedback state */
    volatile uint32_t active_note_leds;
    uint8_t           beat1_flash;
    uint8_t           dirty_diorama;
    uint8_t           dirty_header;
    uint8_t           dirty_party;
    uint8_t           dirty_ticker;
    char              ticker[32];
    uint8_t           ticker_timer;
    uint8_t           shake_timer;
    uint8_t           shake_amp;
    uint8_t           toll_ring_timer;
    uint8_t           gold_flash_timer;
    uint8_t           slide_frames;
    uint8_t           slide_prev_zone;
} FantasiaState;

#endif /* FANTASIA_TYPES_H */
