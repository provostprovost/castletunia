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
            {1, 1, 28000, 14000, 200, 32620, 22000, 31600}, /* Fighter: gritty brass/saw */
            {4, 1, 20000,     0, 100, 32550,  9000, 32350}, /* Cleric: pure glassy bell */
            {3, 2, 16000,  3500, 800, 32700, 26000, 32400}  /* Sorceress: singing arcane lead */
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
            {1, 1, 26000, 12000, 160, 32600, 21000, 31600},
            {4, 1, 18000,     0, 100, 32540,  8500, 32300},
            {3, 2, 15000,  3000, 700, 32680, 25000, 32350}
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
            {1, 1, 25000, 11000, 180, 32620, 20000, 31700},
            {5, 1, 19000,     0, 100, 32560,  9000, 32350},
            {3, 2, 14000,  3200, 850, 32700, 25000, 32400}
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
            {1, 1, 29000, 15000, 200, 32640, 22000, 31600},
            {4, 1, 21000,     0, 100, 32560,  9500, 32350},
            {3, 2, 17000,  4000, 800, 32710, 26000, 32400}
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
            {1, 1, 27000, 13000, 160, 32620, 21000, 31600},
            {5, 1, 20000,     0, 100, 32550,  9000, 32300},
            {3, 2, 16000,  3500, 750, 32690, 25000, 32350}
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
            {1, 1, 26000, 12000, 200, 32620, 21000, 31700},
            {4, 1, 22000,     0, 120, 32580, 10000, 32400},
            {3, 2, 15000,  3000, 900, 32720, 26000, 32450}
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
            {1, 1, 26000, 12000, 220, 32640, 22000, 31800},
            {5, 1, 21000,     0, 100, 32580,  9500, 32400},
            {3, 2, 14000,  2800, 950, 32720, 26000, 32450}
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
            {1, 1, 28000, 14000, 180, 32620, 22000, 31600},
            {4, 1, 20000,     0, 100, 32560,  9000, 32350},
            {3, 2, 16000,  3500, 800, 32700, 26000, 32400}
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
            {1, 1, 30000, 15000, 160, 32620, 23000, 31600},
            {4, 1, 22000,     0, 100, 32560, 10000, 32350},
            {3, 2, 18000,  4000, 750, 32700, 26000, 32400}
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
