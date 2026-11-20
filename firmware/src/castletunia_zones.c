/* SPDX-License-Identifier: GPL-3.0-only */
#include "castletunia_zones.h"

const ZoneDef G_ZONES[NUM_ZONES] = {
    /* Zone 0: THE CRYPT - Classic Dark DS */
    {
        .name = "THE CRYPT",
        .subgenre_id = 0,
        .drone_mode = DRONE_ORGAN,
        .default_bpm = 92,
        .scale_semitones = {0, 2, 3, 5, 7, 8, 10},
        .mid_prog = {0, 5, 6, 0},
        .high_prog = {0, 5, 1, 4},
        .reverb_mix = 96,
        .reverb_fb = 26800,
        .party_patches = {
            {2, 1, 18000,  6000, 160, 32660, 22000, 31900, VOICE_ORGAN_CHOIR}, /* Crypt Fighter: warm dark organ */
            {4, 1, 14000,     0, 100, 32550,  8000, 32350, VOICE_FM},          /* Crypt Cleric: soft cathedral bell */
            {1, 2,  9000,  1500, 900, 32680, 14000, 32300, VOICE_ORGAN_CHOIR}  /* Crypt Sorceress: sub-octave haunt choir */
        },
        .perc_low  = { 800,  200, 32000,  2000, 1,  8000, 32758},
        .perc_high = {2000, 1000, 30000, 28000, 4, 16000, 32735},
        .low_mask_march  = 0x0101u,
        .high_mask_march = 0x1010u,
        .low_mask_battle = 0x1111u,
        .high_mask_battle= 0x5555u,
        .toll_patch = {7, 2, 42000, 0, 64, 32745, 0, 32740, VOICE_FM}
    },
    /* Zone 1: THE TAVERN - Comfy Medieval */
    {
        .name = "THE TAVERN",
        .subgenre_id = 1,
        .drone_mode = DRONE_HURDY,
        .default_bpm = 104,
        .scale_semitones = {0, 2, 4, 5, 7, 9, 10},
        .mid_prog = {0, 6, 3, 0},
        .high_prog = {0, 6, 3, 4},
        .reverb_mix = 48,
        .reverb_fb = 21500,
        .party_patches = {
            {1, 1, 28000,  8000,  64, 32580,  7500, 31600, VOICE_PLUCK},    /* Tavern Fighter: plucky warm lute */
            {3, 1, 24000,     0,  64, 32600,  6500, 32100, VOICE_PLUCK},    /* Tavern Cleric: plucky medieval harp */
            {2, 1, 10000,     0, 600, 32680, 22000, 32200, VOICE_WOODWIND}  /* Tavern Sorceress: mellow wooden flute */
        },
        .perc_low  = {1000,  300, 31500,  1500, 1,  4000, 32752},
        .perc_high = {3000, 1500, 29000, 24000, 3, 12000, 32728},
        .low_mask_march  = 0x0909u,
        .high_mask_march = 0x4444u,
        .low_mask_battle = 0x2929u,
        .high_mask_battle= 0xAAAAu,
        .toll_patch = {5, 2, 42000, 0, 64, 32745, 0, 32740, VOICE_FM}
    },
    /* Zone 6: THE TUNDRA - Winter Synth */
    {
        .name = "THE TUNDRA",
        .subgenre_id = 6,
        .drone_mode = DRONE_GLASS,
        .default_bpm = 60,
        .scale_semitones = {0, 2, 3, 5, 7, 8, 10},
        .mid_prog = {0, 4, 5, 2},
        .high_prog = {0, 4, 5, 6},
        .reverb_mix = 115,
        .reverb_fb = 27800,
        .party_patches = {
            {1, 1, 12000,  2000, 300, 32680, 22000, 32000, VOICE_FM},       /* Tundra Fighter: deep warm sub-bass */
            {8, 1, 22000,     0,  80, 32560,  7000, 32450, VOICE_FM},       /* Tundra Cleric: high icicle glockenspiel */
            {2, 1,  8000,     0, 950, 32720, 26000, 32500, VOICE_WOODWIND}  /* Tundra Sorceress: pure frost whistle */
        },
        .perc_low  = { 750,  150, 32300,  1200, 1,  4000, 32757},
        .perc_high = {3200, 1600, 27500, 18000, 4, 18000, 32735},
        .low_mask_march  = 0x0001u,
        .high_mask_march = 0x1000u,
        .low_mask_battle = 0x0101u,
        .high_mask_battle= 0x1010u,
        .toll_patch = {7, 2, 42000, 0, 64, 32745, 0, 32740, VOICE_FM}
    },
    /* Zone 4: THE SANDS - Desert / Tomb */
    {
        .name = "THE SANDS",
        .subgenre_id = 4,
        .drone_mode = DRONE_HURDY,
        .default_bpm = 100,
        .scale_semitones = {0, 1, 4, 5, 7, 8, 10},
        .mid_prog = {0, 1, 6, 0},
        .high_prog = {0, 1, 3, 6},
        .reverb_mix = 78,
        .reverb_fb = 24800,
        .party_patches = {
            {2, 1, 22000, 12000,  80, 32590,  8000, 31600, VOICE_PLUCK},    /* Sands Fighter: plucky desert sitar/oud */
            {5, 1, 24000,     0,  80, 32640, 22000, 32350, VOICE_FM},       /* Sands Cleric: desert finger-cymbal */
            {3, 2, 16000,  4000, 500, 32690, 22000, 32300, VOICE_WOODWIND}  /* Sands Sorceress: nasal desert horn */
        },
        .perc_low  = {1200,  400, 31000,  3000, 2, 10000, 32753},
        .perc_high = {3500, 1800, 28500, 22000, 3, 10000, 32726},
        .low_mask_march  = 0x0141u,
        .high_mask_march = 0x1010u,
        .low_mask_battle = 0x0541u,
        .high_mask_battle= 0x5454u,
        .toll_patch = {5, 2, 42000, 0, 64, 32745, 0, 32740, VOICE_FM}
    },
    /* Zone 2: THE GLADE - Woodland Synth */
    {
        .name = "THE GLADE",
        .subgenre_id = 2,
        .drone_mode = DRONE_STRINGS,
        .default_bpm = 82,
        .scale_semitones = {0, 2, 3, 5, 7, 9, 10},
        .mid_prog = {0, 3, 6, 0},
        .high_prog = {0, 3, 5, 6},
        .reverb_mix = 72,
        .reverb_fb = 24500,
        .party_patches = {
            {2, 1, 18000,  4000, 200, 32620, 20000, 31700, VOICE_WOODWIND}, /* Glade Fighter: hollow reed/clarinet */
            {5, 2, 22000,     0,  80, 32640, 20000, 32200, VOICE_FM},       /* Glade Cleric: woody marimba */
            {3, 2, 12000,  1500, 850, 32700, 25000, 32400, VOICE_WOODWIND}  /* Glade Sorceress: airy pan-flute */
        },
        .perc_low  = { 900,  250, 31800,  1000, 1,  3000, 32750},
        .perc_high = {4000, 2000, 28000, 20000, 2,  8000, 32720},
        .low_mask_march  = 0x0411u,
        .high_mask_march = 0x2080u,
        .low_mask_battle = 0x1451u,
        .high_mask_battle= 0x54A4u,
        .toll_patch = {5, 2, 42000, 0, 64, 32745, 0, 32740, VOICE_FM}
    },
    /* Zone 3: THE CASTLE - Vampyric Baroque */
    {
        .name = "THE CASTLE",
        .subgenre_id = 3,
        .drone_mode = DRONE_ORGAN,
        .default_bpm = 90,
        .scale_semitones = {0, 2, 3, 5, 7, 8, 11},
        .mid_prog = {0, 5, 3, 4},
        .high_prog = {0, 3, 6, 4},
        .reverb_mix = 84,
        .reverb_fb = 25800,
        .party_patches = {
            {1, 1, 30000, 14000,  64, 32600,  9000, 31700, VOICE_PLUCK},       /* Castle Fighter: bright harpsichord */
            {3, 1, 24000,  3000, 200, 32680, 24000, 32350, VOICE_ORGAN_CHOIR}, /* Castle Cleric: gothic choir */
            {2, 1, 18000,  6000, 750, 32710, 24000, 32350, VOICE_ANALOG_SAW}   /* Castle Sorceress: rich vampyric strings */
        },
        .perc_low  = { 850,  180, 32200,   800, 1,  6000, 32756},
        .perc_high = {2500, 1200, 29500, 26000, 4, 14000, 32722},
        .low_mask_march  = 0x0101u,
        .high_mask_march = 0x1110u,
        .low_mask_battle = 0x4111u,
        .high_mask_battle= 0x5555u,
        .toll_patch = {11, 4, 42000, 0, 64, 32745, 0, 32740, VOICE_FM}
    },
    /* Zone 7: CORONATION - Royal Fanfare */
    {
        .name = "CORONATION",
        .subgenre_id = 7,
        .drone_mode = DRONE_BRASS,
        .default_bpm = 104,
        .scale_semitones = {0, 2, 4, 5, 7, 9, 11},
        .mid_prog = {0, 4, 5, 3},
        .high_prog = {0, 4, 5, 3},
        .reverb_mix = 64,
        .reverb_fb = 23500,
        .party_patches = {
            {1, 1, 24000,  6000, 140, 32650, 24000, 31800, VOICE_ANALOG_SAW}, /* Coronation Fighter: royal trumpet */
            {2, 1, 20000,  4000, 180, 32640, 22000, 31900, VOICE_ANALOG_SAW}, /* Coronation Cleric: french horn brass */
            {1, 1, 18000,  3500, 220, 32660, 23000, 32000, VOICE_ANALOG_SAW}  /* Coronation Sorceress: brass fanfare lead */
        },
        .perc_low  = { 880,  220, 32100,   900, 1,  5000, 32755},
        .perc_high = {2800, 1400, 29000, 25000, 3, 15000, 32738},
        .low_mask_march  = 0x0505u,
        .high_mask_march = 0x5050u,
        .low_mask_battle = 0x1515u,
        .high_mask_battle= 0xF5F5u,
        .toll_patch = {5, 2, 42000, 0, 64, 32745, 0, 32740, VOICE_FM}
    },
    /* Zone 5: THE SPIRE - Crystal / Arcane */
    {
        .name = "THE SPIRE",
        .subgenre_id = 5,
        .drone_mode = DRONE_GLASS,
        .default_bpm = 68,
        .scale_semitones = {0, 2, 4, 6, 7, 9, 11},
        .mid_prog = {0, 1, 4, 0},
        .high_prog = {0, 1, 4, 5},
        .reverb_mix = 110,
        .reverb_fb = 27600,
        .party_patches = {
            {2, 1, 18000,  6000, 140, 32580, 15000, 31800, VOICE_ANALOG_SAW}, /* Spire Fighter: resonant synth bass */
            {3, 1, 22000,     0, 100, 32660, 22000, 32400, VOICE_FM},         /* Spire Cleric: crystal chime */
            {2, 1, 12000,  1500, 800, 32700, 18000, 32400, VOICE_ANALOG_SAW}  /* Spire Sorceress: cosmic pad */
        },
        .perc_low  = { 700,  100, 32500,   500, 1,  2000, 32760},
        .perc_high = {5000, 3000, 27000, 18000, 5, 20000, 32745},
        .low_mask_march  = 0x0101u,
        .high_mask_march = 0x1010u,
        .low_mask_battle = 0x1111u,
        .high_mask_battle= 0x4444u,
        .toll_patch = {7, 2, 42000, 0, 64, 32745, 0, 32740, VOICE_FM}
    },
    /* Zone 8: BOSS - 16-Bit Battle */
    {
        .name = "BOSS",
        .subgenre_id = 8,
        .drone_mode = DRONE_DARK_SAW,
        .default_bpm = 138,
        .scale_semitones = {0, 1, 3, 6, 7, 8, 10}, /* Dark Locrian/tritone horror scale */
        .mid_prog = {0, 1, 5, 4},
        .high_prog = {0, 1, 4, 1},
        .reverb_mix = 52,
        .reverb_fb = 22500,
        .party_patches = {
            {1, 1, 22000, 14000, 100, 32580, 17000, 31600, VOICE_ANALOG_SAW},  /* Boss Fighter: shred bass */
            {7, 3, 26000,  6000,  80, 32650, 23000, 32350, VOICE_FM},          /* Boss Cleric: horror bell */
            {3, 2, 24000, 12000, 120, 32680, 24000, 32200, VOICE_ORGAN_CHOIR}  /* Boss Sorceress: menacing dark organ lead */
        },
        .perc_low  = { 950,  120, 32600,  4000, 2, 14000, 32754},
        .perc_high = {3200, 1600, 28200, 31000, 4, 22000, 32748},
        .low_mask_march  = 0x1111u,
        .high_mask_march = 0x5555u,
        .low_mask_battle = 0x5555u,
        .high_mask_battle= 0xFFFFu,
        .toll_patch = {11, 4, 42000, 0, 64, 32745, 0, 32740, VOICE_FM}
    }
};

const ZoneDef *zone_get(uint8_t zone_id)
{
    if (zone_id >= NUM_ZONES) zone_id = 0;
    return &G_ZONES[zone_id];
}
