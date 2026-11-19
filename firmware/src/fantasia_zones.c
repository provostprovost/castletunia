/* SPDX-License-Identifier: GPL-3.0-only */
#include "fantasia_zones.h"

const ZoneDef G_ZONES[NUM_ZONES] = {
    /* Zone 0: THE CRYPT - Classic Dark DS */
    {
        .name = "THE CRYPT",
        .subgenre_id = 0,
        .default_bpm = 92,
        .scale_semitones = {0, 2, 3, 5, 7, 8, 10},
        .mid_prog = {0, 5, 6, 0},
        .high_prog = {0, 5, 1, 4},
        .reverb_mix = 96,
        .reverb_fb = 26800,
        .party_patches = {
            {2, 1, 24000, 10000, 220, 32640, 22000, 31800}, /* Crypt Fighter: dark pipe organ bass */
            {4, 1, 20000,     0, 100, 32550,  8000, 32400}, /* Crypt Cleric: cathedral bell */
            {1, 2, 14000,  2000, 900, 32710, 26000, 32450}  /* Crypt Sorceress: eerie sub-octave haunt */
        },
        .perc_low  = { 800,  200, 32000,  2000, 1,  8000, 32758},
        .perc_high = {2000, 1000, 30000, 28000, 4, 16000, 32735},
        .low_mask_march  = 0x0101u,
        .high_mask_march = 0x1010u,
        .low_mask_battle = 0x1111u,
        .high_mask_battle= 0x5555u,
        .toll_patch = {7, 2, 42000, 0, 64, 32745, 0, 32740}
    },
    /* Zone 1: THE TAVERN - Comfy Medieval */
    {
        .name = "THE TAVERN",
        .subgenre_id = 1,
        .default_bpm = 104,
        .scale_semitones = {0, 2, 4, 5, 7, 9, 10},
        .mid_prog = {0, 6, 3, 0},
        .high_prog = {0, 6, 3, 4},
        .reverb_mix = 48,
        .reverb_fb = 21500,
        .party_patches = {
            {1, 1, 22000,  6000,  80, 32350,  5000, 31200}, /* Tavern Fighter: warm plucked lute */
            {3, 1, 16000,     0,  80, 32420,  6000, 32100}, /* Tavern Cleric: plucked harp */
            {2, 1, 10000,     0, 600, 32680, 24000, 32200}  /* Tavern Sorceress: mellow wooden flute */
        },
        .perc_low  = {1000,  300, 31500,  1500, 1,  4000, 32752},
        .perc_high = {3000, 1500, 29000, 24000, 3, 12000, 32728},
        .low_mask_march  = 0x0909u,
        .high_mask_march = 0x4444u,
        .low_mask_battle = 0x2929u,
        .high_mask_battle= 0xAAAAu,
        .toll_patch = {5, 2, 42000, 0, 64, 32745, 0, 32740}
    },
    /* Zone 2: THE GLADE - Woodland Synth */
    {
        .name = "THE GLADE",
        .subgenre_id = 2,
        .default_bpm = 96,
        .scale_semitones = {0, 2, 3, 5, 7, 9, 10},
        .mid_prog = {0, 3, 6, 0},
        .high_prog = {0, 3, 5, 6},
        .reverb_mix = 72,
        .reverb_fb = 24500,
        .party_patches = {
            {2, 1, 18000,  4000, 200, 32620, 20000, 31700}, /* Glade Fighter: hollow reed/clarinet */
            {5, 2, 22000,     0,  80, 32300,  4000, 31900}, /* Glade Cleric: woody marimba */
            {3, 2, 12000,  1500, 850, 32700, 25000, 32400}  /* Glade Sorceress: airy pan-flute */
        },
        .perc_low  = { 900,  250, 31800,  1000, 1,  3000, 32750},
        .perc_high = {4000, 2000, 28000, 20000, 2,  8000, 32720},
        .low_mask_march  = 0x0411u,
        .high_mask_march = 0x2080u,
        .low_mask_battle = 0x1451u,
        .high_mask_battle= 0x54A4u,
        .toll_patch = {5, 2, 42000, 0, 64, 32745, 0, 32740}
    },
    /* Zone 3: THE CASTLE - Vampyric Baroque */
    {
        .name = "THE CASTLE",
        .subgenre_id = 3,
        .default_bpm = 108,
        .scale_semitones = {0, 2, 3, 5, 7, 8, 11},
        .mid_prog = {0, 5, 3, 4},
        .high_prog = {0, 5, 1, 4},
        .reverb_mix = 84,
        .reverb_fb = 25800,
        .party_patches = {
            {1, 1, 32000, 18000,  80, 32450, 10000, 31500}, /* Castle Fighter: bright harpsichord */
            {3, 1, 20000,  4000, 300, 32660, 18000, 32350}, /* Castle Cleric: gothic choir */
            {2, 1, 22000,  8000, 750, 32710, 26000, 32400}  /* Castle Sorceress: rich vampyric strings */
        },
        .perc_low  = { 850,  180, 32200,   800, 1,  6000, 32756},
        .perc_high = {2500, 1200, 29500, 26000, 4, 14000, 32722},
        .low_mask_march  = 0x0101u,
        .high_mask_march = 0x1110u,
        .low_mask_battle = 0x4111u,
        .high_mask_battle= 0x5555u,
        .toll_patch = {11, 4, 42000, 0, 64, 32745, 0, 32740}
    },
    /* Zone 4: THE SANDS - Desert / Tomb */
    {
        .name = "THE SANDS",
        .subgenre_id = 4,
        .default_bpm = 100,
        .scale_semitones = {0, 1, 4, 5, 7, 8, 10},
        .mid_prog = {0, 1, 6, 0},
        .high_prog = {0, 1, 3, 6},
        .reverb_mix = 78,
        .reverb_fb = 24800,
        .party_patches = {
            {7, 4, 28000, 12000, 100, 32480, 12000, 31600}, /* Sands Fighter: metallic sitar/oud buzz */
            {6, 1, 24000,     0,  80, 32400,  6000, 32200}, /* Sands Cleric: desert finger-cymbal */
            {3, 2, 20000,  6000, 500, 32690, 25000, 32350}  /* Sands Sorceress: nasal desert horn */
        },
        .perc_low  = {1200,  400, 31000,  3000, 2, 10000, 32753},
        .perc_high = {3500, 1800, 28500, 22000, 3, 10000, 32726},
        .low_mask_march  = 0x0841u,
        .high_mask_march = 0x2222u,
        .low_mask_battle = 0x4849u,
        .high_mask_battle= 0xB6B6u,
        .toll_patch = {5, 2, 42000, 0, 64, 32745, 0, 32740}
    },
    /* Zone 5: THE SPIRE - Crystal / Arcane */
    {
        .name = "THE SPIRE",
        .subgenre_id = 5,
        .default_bpm = 86,
        .scale_semitones = {0, 2, 4, 6, 7, 9, 11},
        .mid_prog = {0, 1, 5, 0},
        .high_prog = {0, 1, 4, 5},
        .reverb_mix = 110,
        .reverb_fb = 27600,
        .party_patches = {
            {5, 2, 26000, 10000, 180, 32600, 18000, 31700}, /* Spire Fighter: resonant FM laser bass */
            {7, 1, 24000,     0, 100, 32580, 10000, 32450}, /* Spire Cleric: crystal glass chime */
            {4, 3, 18000,  4000, 950, 32720, 26000, 32450}  /* Spire Sorceress: sci-fi cosmic pad */
        },
        .perc_low  = { 700,  100, 32500,   500, 1,  2000, 32760},
        .perc_high = {5000, 3000, 27000, 18000, 5, 20000, 32745},
        .low_mask_march  = 0x0101u,
        .high_mask_march = 0x0808u,
        .low_mask_battle = 0x0909u,
        .high_mask_battle= 0x4949u,
        .toll_patch = {7, 2, 42000, 0, 64, 32745, 0, 32740}
    },
    /* Zone 6: THE TUNDRA - Winter Synth */
    {
        .name = "THE TUNDRA",
        .subgenre_id = 6,
        .default_bpm = 76,
        .scale_semitones = {0, 2, 3, 5, 7, 8, 10},
        .mid_prog = {0, 4, 5, 2},
        .high_prog = {0, 4, 5, 6},
        .reverb_mix = 115,
        .reverb_fb = 27800,
        .party_patches = {
            {1, 1, 12000,  2000, 300, 32680, 22000, 32000}, /* Tundra Fighter: deep warm sub-bass */
            {8, 1, 22000,     0,  80, 32560,  7000, 32450}, /* Tundra Cleric: high icicle glockenspiel */
            {2, 1,  8000,     0, 950, 32720, 26000, 32500}  /* Tundra Sorceress: pure frost whistle */
        },
        .perc_low  = { 750,  150, 32300,  1200, 1,  4000, 32757},
        .perc_high = {4500, 2200, 27500, 29000, 4, 18000, 32742},
        .low_mask_march  = 0x0001u,
        .high_mask_march = 0x0404u,
        .low_mask_battle = 0x0101u,
        .high_mask_battle= 0x2424u,
        .toll_patch = {7, 2, 42000, 0, 64, 32745, 0, 32740}
    },
    /* Zone 7: CORONATION - Royal Fanfare */
    {
        .name = "CORONATION",
        .subgenre_id = 7,
        .default_bpm = 116,
        .scale_semitones = {0, 2, 4, 5, 7, 9, 11},
        .mid_prog = {0, 4, 5, 3},
        .high_prog = {0, 4, 5, 1},
        .reverb_mix = 64,
        .reverb_fb = 23500,
        .party_patches = {
            {1, 1, 30000, 16000, 140, 32640, 24000, 31700}, /* Coronation Fighter: bright royal trumpet */
            {5, 2, 22000,     0, 100, 32560, 11000, 32400}, /* Coronation Cleric: tubular bells */
            {3, 2, 22000,  6000, 400, 32700, 26000, 32350}  /* Coronation Sorceress: brass fanfare lead */
        },
        .perc_low  = { 880,  220, 32100,   900, 1,  5000, 32755},
        .perc_high = {2800, 1400, 29000, 25000, 3, 15000, 32738},
        .low_mask_march  = 0x0505u,
        .high_mask_march = 0x5050u,
        .low_mask_battle = 0x1515u,
        .high_mask_battle= 0xF5F5u,
        .toll_patch = {5, 2, 42000, 0, 64, 32745, 0, 32740}
    },
    /* Zone 8: BOSS - 16-Bit Battle */
    {
        .name = "BOSS",
        .subgenre_id = 8,
        .default_bpm = 138,
        .scale_semitones = {0, 1, 3, 5, 7, 8, 10},
        .mid_prog = {0, 1, 5, 4},
        .high_prog = {0, 1, 4, 1},
        .reverb_mix = 40,
        .reverb_fb = 20000,
        .party_patches = {
            {1, 1, 36000, 22000, 100, 32620, 24000, 31500}, /* Boss Fighter: overdriven shred bass */
            {4, 1, 26000,  4000,  80, 32500, 10000, 32200}, /* Boss Cleric: sharp metallic synth-bell */
            {3, 1, 24000,  8000, 120, 32680, 26000, 32100}  /* Boss Sorceress: piercing 16-bit square lead */
        },
        .perc_low  = { 950,  120, 32600,  4000, 2, 14000, 32754},
        .perc_high = {3200, 1600, 28200, 31000, 4, 22000, 32748},
        .low_mask_march  = 0x1111u,
        .high_mask_march = 0x5555u,
        .low_mask_battle = 0x5555u,
        .high_mask_battle= 0xFFFFu,
        .toll_patch = {11, 4, 42000, 0, 64, 32745, 0, 32740}
    }
};

const ZoneDef *zone_get(uint8_t zone_id)
{
    if (zone_id >= NUM_ZONES) zone_id = 0;
    return &G_ZONES[zone_id];
}
