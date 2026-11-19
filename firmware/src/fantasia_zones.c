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
            {2, 1, 20000, 3000,  200, 32640, 20000, 31800},
            {2, 1, 18000, 1500,  120, 32600, 16000, 31600},
            {2, 1, 14000, 2500,  600, 32700, 22000, 32200}
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
            {1, 1, 18000, 2000,  100, 32600, 18000, 31600},
            {2, 1, 16000, 1500,  100, 32580, 16000, 31600},
            {2, 1, 12000, 2000,  400, 32680, 20000, 32000}
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
            {2, 1, 14000, 1500, 120, 32620, 18000, 31800},
            {2, 1, 16000, 1000, 120, 32600, 16000, 31800},
            {3, 1, 14000, 1800, 500, 32680, 20000, 32200}
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
            {1, 1, 22000, 3500, 200, 32680, 20000, 31800},
            {3, 1, 20000, 2000, 100, 32600, 16000, 31600},
            {2, 1, 16000, 2500, 600, 32720, 24000, 32200}
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
            {2, 1, 20000, 2500, 100, 32600, 18000, 31600},
            {3, 1, 18000, 1500, 100, 32580, 15000, 31500},
            {2, 1, 18000, 3000, 300, 32650, 20000, 32000}
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
            {2, 1, 16000, 1500, 200, 32620, 18000, 31800},
            {3, 1, 18000, 2000, 300, 32700, 22000, 32200},
            {3, 1, 20000, 1500, 200, 32660, 18000, 32200}
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
            {1, 1, 18000, 2500, 400, 32710, 22000, 32100},
            {3, 1, 18000, 1000, 120, 32620, 16000, 31800},
            {2, 1, 14000, 2000, 600, 32680, 20000, 32200}
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
            {1, 1, 22000, 3500, 180, 32620, 18000, 31600},
            {2, 1, 18000, 2000, 300, 32700, 22000, 32200},
            {2, 1, 20000, 3000, 300, 32660, 20000, 32000}
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
            {2, 1, 24000, 4000, 100, 32620, 18000, 31600},
            {2, 1, 22000, 3000, 100, 32620, 18000, 31600},
            {2, 1, 20000, 3500, 150, 32640, 20000, 31800}
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
