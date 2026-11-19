/* SPDX-License-Identifier: GPL-3.0-only */
#include "fantasia_dsp.h"
#include "fantasia_zones.h"
#include "fantasia_engine.h"
#include "hal_adapter.h"

static const uint32_t G_MIDI_PHASE_INC[128] = {
    796254u, 843601u, 893765u, 946911u, 1003217u, 1062871u, 1126073u, 1193033u, 1263974u, 1339134u, 1418763u, 1503127u,
    1592507u, 1687203u, 1787529u, 1893821u, 2006434u, 2125742u, 2252146u, 2386065u, 2527948u, 2678268u, 2837526u, 3006254u,
    3185015u, 3374406u, 3575058u, 3787642u, 4012867u, 4251485u, 4504291u, 4772130u, 5055896u, 5356535u, 5675051u, 6012507u,
    6370030u, 6748811u, 7150117u, 7575285u, 8025735u, 8502970u, 9008582u, 9544261u, 10111792u, 10713070u, 11350103u, 12025015u,
    12740059u, 13497623u, 14300233u, 15150569u, 16051469u, 17005939u, 18017165u, 19088521u, 20223584u, 21426141u, 22700205u, 24050030u,
    25480119u, 26995246u, 28600467u, 30301139u, 32102938u, 34011878u, 36034330u, 38177043u, 40447168u, 42852281u, 45400411u, 48100060u,
    50960238u, 53990491u, 57200933u, 60602278u, 64205876u, 68023757u, 72068660u, 76354085u, 80894335u, 85704563u, 90800821u, 96200119u,
    101920476u, 107980983u, 114401866u, 121204555u, 128411753u, 136047513u, 144137319u, 152708170u, 161788671u, 171409126u, 181601643u, 192400238u,
    203840952u, 215961966u, 228803732u, 242409110u, 256823506u, 272095026u, 288274639u, 305416341u, 323577341u, 342818251u, 363203285u, 384800477u,
    407681904u, 431923931u, 457607465u, 484818220u, 513647012u, 544190053u, 576549277u, 610832681u, 647154683u, 685636503u, 726406571u, 769600953u,
    815363807u, 863847862u, 915214929u, 969636441u, 1027294024u, 1088380105u, 1153098554u, 1221665363u
};

static inline int32_t q15_mul(int32_t a, int32_t b)
{
    return (a * b) >> 15;
}

static inline int16_t clamp16(int32_t v)
{
    if (v > 32767) return 32767;
    if (v < -32767) return -32767;
    return (int16_t)v;
}

typedef enum {
    ENV_OFF = 0,
    ENV_ATK = 1,
    ENV_DEC = 2,
    ENV_SUS = 3,
    ENV_REL = 4
} EnvStage;

typedef struct {
    uint32_t phase_c;
    uint32_t phase_m;
    uint32_t inc_c;
    uint32_t inc_m;
    int32_t  prev_mod;
    int32_t  env_q15;
    int32_t  target_env;
    int32_t  attack_step;
    int32_t  vel_q15;
    uint16_t gate_ticks;
    uint8_t  stage;
    uint8_t  quick_rel;
    uint8_t  env_div;
} FmVoice;

typedef struct {
    uint16_t phase;
    uint16_t fm_phase;
    uint16_t pitch_inc;
    int32_t  env_q15;
    int32_t  vel_q15;
    uint8_t  active;
} PercVoice;

static FmVoice   g_party[NUM_PARTY_SLOTS];
static FmVoice   g_toll;
static PercVoice g_perc[2];

/* Atmospheric Backing Chord Pad state (Root + 3rd + 5th) */
static uint32_t g_drone_ph1 = 0u, g_drone_ph2 = 0u, g_drone_ph3 = 0u;
static uint32_t g_drone_inc1 = 0u, g_drone_inc2 = 0u, g_drone_inc3 = 0u;
static uint32_t g_drone_lfo_ph = 0u;
static int32_t  g_drone_lpf = 0;
static uint8_t  g_drone_third_en = 0u;
static uint8_t  g_drone_fifth_en = 0u;
static int32_t  g_drone_level_q15 = 0;

/* 16-bit Galois LFSR for percussion noise */
static uint16_t g_lfsr = 0xACE1u;

/* 4-Comb + 2-Allpass Schroeder Room Reverb (~8.7 KB static SRAM) */
#define COMB0_LEN 743u
#define COMB1_LEN 887u
#define COMB2_LEN 1013u
#define COMB3_LEN 1153u
#define AP0_LEN   223u
#define AP1_LEN   337u

static int16_t  g_comb0[COMB0_LEN];
static int16_t  g_comb1[COMB1_LEN];
static int16_t  g_comb2[COMB2_LEN];
static int16_t  g_comb3[COMB3_LEN];
static int16_t  g_ap0[AP0_LEN];
static int16_t  g_ap1[AP1_LEN];
static uint16_t g_comb_idx[4];
static uint16_t g_ap_idx[2];
static int32_t  g_comb_lp[4];

/* Post-EQ ~120 Hz DC/HPF */
static int32_t  g_hp_l = 0, g_hp_r = 0;

void dsp_init(void)
{
    for (uint32_t i = 0; i < NUM_PARTY_SLOTS; i++) {
        for (uint32_t j = 0; j < sizeof(FmVoice); j++) ((uint8_t *)&g_party[i])[j] = 0;
    }
    for (uint32_t j = 0; j < sizeof(FmVoice); j++) ((uint8_t *)&g_toll)[j] = 0;
    for (uint32_t i = 0; i < 2u; i++) {
        for (uint32_t j = 0; j < sizeof(PercVoice); j++) ((uint8_t *)&g_perc[i])[j] = 0;
    }
    g_lfsr = 0xACE1u;
    g_hp_l = 0;
    g_hp_r = 0;
    for (uint32_t i = 0; i < 4u; i++) { g_comb_idx[i] = 0u; g_comb_lp[i] = 0; }
    for (uint32_t i = 0; i < 2u; i++) g_ap_idx[i] = 0u;
    for (uint32_t i = 0; i < COMB0_LEN; i++) g_comb0[i] = 0;
    for (uint32_t i = 0; i < COMB1_LEN; i++) g_comb1[i] = 0;
    for (uint32_t i = 0; i < COMB2_LEN; i++) g_comb2[i] = 0;
    for (uint32_t i = 0; i < COMB3_LEN; i++) g_comb3[i] = 0;
    for (uint32_t i = 0; i < AP0_LEN; i++) g_ap0[i] = 0;
    for (uint32_t i = 0; i < AP1_LEN; i++) g_ap1[i] = 0;
}

void dsp_drone_set(uint8_t root_midi, uint8_t third_midi, uint8_t fifth_midi, uint8_t third_en, uint8_t fifth_en, uint8_t level)
{
    if (root_midi > 127u) root_midi = 48u;
    if (third_midi > 127u) third_midi = 51u;
    if (fifth_midi > 127u) fifth_midi = 55u;
    g_drone_inc1 = G_MIDI_PHASE_INC[root_midi];
    uint32_t t_inc = G_MIDI_PHASE_INC[third_midi];
    uint32_t f_inc = G_MIDI_PHASE_INC[fifth_midi];
    g_drone_inc2 = f_inc + (f_inc / 667u); /* +0.15% detuned 5th */
    g_drone_inc3 = t_inc - (t_inc / 800u); /* -0.12% lush chorus 3rd */
    g_drone_third_en = third_en;
    g_drone_fifth_en = fifth_en;
    g_drone_level_q15 = (int32_t)level * 136; /* Full backing chord bed */
}

static void start_fm_voice(FmVoice *v, const FmPatch *p, uint8_t midi_note, uint16_t vel_q15, uint16_t gate_ticks)
{
    if (midi_note > 127u) midi_note = 60u;
    uint32_t inc = G_MIDI_PHASE_INC[midi_note];
    uint32_t den = (p->mod_ratio_den > 0u) ? p->mod_ratio_den : 1u;
    v->inc_c = inc;
    v->inc_m = (inc / den) * p->mod_ratio_num;
    v->vel_q15 = (int32_t)vel_q15;
    v->gate_ticks = gate_ticks;
    uint32_t atk = (p->attack_samples < 64u) ? 64u : p->attack_samples;
    v->attack_step = (int32_t)(32767u / atk);
    if (v->attack_step < 1) v->attack_step = 1;
    v->target_env = 32767;
    v->stage = ENV_ATK;
    v->quick_rel = 0u;
    v->env_div = 0u;
}

void dsp_note_on(uint8_t slot, uint8_t midi_note, uint16_t vel_q15, uint16_t gate_ticks)
{
    if (slot >= NUM_PARTY_SLOTS) return;
    const ZoneDef *z = zone_get(g_state.settings.zone_id);
    start_fm_voice(&g_party[slot], &z->party_patches[slot], midi_note, vel_q15, gate_ticks);
}

void dsp_voice_quick_release(uint8_t slot)
{
    if (slot >= NUM_PARTY_SLOTS) return;
    if (g_party[slot].stage != ENV_OFF) {
        g_party[slot].stage = ENV_REL;
        g_party[slot].quick_rel = 1u;
        g_party[slot].gate_ticks = 0u;
    }
}

void dsp_toll_trigger(uint8_t zone_id, uint8_t root_midi)
{
    const ZoneDef *z = zone_get(zone_id);
    uint8_t note = (uint8_t)(root_midi + 24u);
    if (note > 108u) note = 84u;
    start_fm_voice(&g_toll, &z->toll_patch, note, 28000u, 36u);
}

void dsp_attack_sfx(uint8_t slot)
{
    static const FmPatch ATK_SFX[3] = {
        {7, 2, 36000, 10000, 64, 31500, 0, 29000}, /* Fighter: crisp metallic sword slash */
        {5, 2, 28000,  2000, 64, 32350, 0, 31000}, /* White Mage: bright Holy bell chime */
        {3, 1, 44000,  8000, 64, 32100, 0, 30000}  /* Red Sorceress: arcane Flare burst */
    };
    static const uint8_t NOTES[3] = { 62u, 79u, 74u };
    uint8_t s = slot % 3u;
    start_fm_voice(&g_toll, &ATK_SFX[s], NOTES[s], 26000u, 10u);
}

void dsp_boss_attack_sfx(uint8_t zone_id)
{
    static const FmPatch BOSS_SFX[9] = {
        {1, 2, 50000,  4000, 64, 32600, 4, 24000}, /* 0 Crypt (Lich): deep doom toll */
        {9, 2, 46000, 14000, 64, 31600, 0, 29000}, /* 1 Tavern (Mimic): crunching brawl snap */
        {4, 3, 38000,  8000, 64, 32300, 3, 27000}, /* 2 Glade (Treant): woody overgrow chime */
        {3, 2, 52000, 10000, 64, 32400, 5, 28000}, /* 3 Castle (Vampire): dark blood moon organ */
        {11, 2, 42000, 12000, 64, 32100, 6, 26000}, /* 4 Sands (Djinn): metallic sandstorm */
        {7, 5, 56000,  6000, 64, 32450, 8, 25000}, /* 5 Spire (Beholder): alien warp beam */
        {5, 4, 48000,  3000, 64, 32550, 2, 26000}, /* 6 Tundra (Wyrm): crystalline frost shatter */
        {2, 1, 34000,  2000, 64, 32450, 2, 29000}, /* 7 Coronation (Seraph): radiant judgment brass */
        {13, 3, 60000, 16000, 64, 32500, 0, 22000} /* 8 Chaos (Demon): meteor impact */
    };
    static const uint8_t NOTES[9] = { 42u, 55u, 64u, 48u, 67u, 73u, 81u, 72u, 38u };
    uint8_t z = zone_id % 9u;
    start_fm_voice(&g_toll, &BOSS_SFX[z], NOTES[z], 30000u, 20u);
}

void dsp_perc_trigger(uint8_t is_high, uint16_t vel_q15)
{
    uint8_t ch = is_high ? 1u : 0u;
    const ZoneDef *z = zone_get(g_state.settings.zone_id);
    const PercPatch *pp = is_high ? &z->perc_high : &z->perc_low;
    g_perc[ch].phase = 0u;
    g_perc[ch].fm_phase = 0u;
    g_perc[ch].pitch_inc = pp->pitch_start_inc;
    g_perc[ch].env_q15 = 32767;
    g_perc[ch].vel_q15 = (int32_t)vel_q15;
    g_perc[ch].active = 1u;
}

void dsp_all_notes_off(void)
{
    for (uint8_t s = 0; s < NUM_PARTY_SLOTS; s++) {
        g_party[s].gate_ticks = 0u;
        if (g_party[s].stage != ENV_OFF) {
            g_party[s].stage = ENV_REL;
            g_party[s].quick_rel = 1u;
        }
    }
    if (g_toll.stage != ENV_OFF) {
        g_toll.stage = ENV_REL;
        g_toll.quick_rel = 1u;
    }
}

void dsp_tick_gates(void)
{
    for (uint8_t s = 0; s < NUM_PARTY_SLOTS; s++) {
        if (g_party[s].gate_ticks > 0u) {
            g_party[s].gate_ticks--;
            if (g_party[s].gate_ticks == 0u && g_party[s].stage != ENV_OFF) {
                g_party[s].stage = ENV_REL;
            }
        }
    }
    if (g_toll.gate_ticks > 0u) {
        g_toll.gate_ticks--;
        if (g_toll.gate_ticks == 0u && g_toll.stage != ENV_OFF) {
            g_toll.stage = ENV_REL;
        }
    }
}

static int32_t render_fm_voice(FmVoice *v, const FmPatch *patch, int32_t grit_boost_q15)
{
    if (v->stage == ENV_OFF) return 0;

    switch (v->stage) {
        case ENV_ATK:
            v->env_q15 += v->attack_step;
            if (v->env_q15 >= v->target_env) {
                v->env_q15 = v->target_env;
                v->target_env = 0; /* Clear target_env after 64-sample micro-ramp */
                v->stage = ENV_DEC;
            }
            break;
        case ENV_DEC:
            if ((++v->env_div & 31u) == 0u) {
                v->env_q15 = q15_mul(v->env_q15, patch->decay_q15);
                if (v->env_q15 <= (int32_t)patch->sustain_q15) {
                    v->env_q15 = (int32_t)patch->sustain_q15;
                    v->stage = (v->env_q15 < 8) ? ENV_OFF : ENV_SUS;
                }
            }
            break;
        case ENV_SUS:
            break;
        case ENV_REL:
        default: {
            if ((++v->env_div & 31u) == 0u) {
                uint16_t rel = v->quick_rel ? 29500u : patch->release_q15;
                v->env_q15 = q15_mul(v->env_q15, rel);
                if (v->env_q15 < 8) {
                    v->env_q15 = 0;
                    v->stage = ENV_OFF;
                    return 0;
                }
            }
            break;
        }
    }

    v->phase_c += v->inc_c;
    v->phase_m += v->inc_m;

    int32_t fb = (v->prev_mod * (int32_t)patch->feedback_q15) >> 15;
    int16_t mod = hal_sine_q15(v->phase_m + ((uint32_t)fb << 15));
    v->prev_mod = mod;

    int32_t env2 = (v->env_q15 * v->env_q15) >> 15;
    int32_t mod_idx = (int32_t)patch->mod_index_q15 + grit_boost_q15;
    if (mod_idx > 65535) mod_idx = 65535;
    int32_t idx = (mod_idx * env2) >> 15;
    int16_t car = hal_sine_q15(v->phase_c + (uint32_t)((mod * idx) >> 1));

    int32_t out = q15_mul(car, v->env_q15);
    return q15_mul(out, v->vel_q15);
}

static int32_t render_perc_voice(PercVoice *p, const PercPatch *pp)
{
    if (!p->active) return 0;

    p->pitch_inc = (uint16_t)(pp->pitch_end_inc + q15_mul((int32_t)p->pitch_inc - (int32_t)pp->pitch_end_inc, pp->pitch_sweep_q15));
    p->phase = (uint16_t)(p->phase + p->pitch_inc);
    p->fm_phase = (uint16_t)(p->fm_phase + (uint16_t)(p->pitch_inc * pp->fm_ratio));

    int16_t mod = hal_sine_q15((uint32_t)p->fm_phase << 16);
    uint32_t car_ph = ((uint32_t)p->phase << 16) + (uint32_t)(mod * (int32_t)pp->fm_index_q15);
    int16_t tone = hal_sine_q15(car_ph);

    uint16_t lsb = g_lfsr & 1u;
    g_lfsr >>= 1;
    if (lsb) g_lfsr ^= 0xB400u;
    int32_t noise = (g_lfsr & 1u) ? 24000 : -24000;

    int32_t dry = q15_mul(tone, 32767 - (int32_t)pp->noise_mix_q15)
                + q15_mul(noise, (int32_t)pp->noise_mix_q15);

    p->env_q15 = q15_mul(p->env_q15, pp->decay_q15);
    if (p->env_q15 < 8) {
        p->env_q15 = 0;
        p->active = 0u;
        return 0;
    }

    return q15_mul(q15_mul(dry, p->env_q15), p->vel_q15);
}

static int32_t comb_step(int16_t *buf, uint32_t len, uint16_t *idx, int32_t *lp, int32_t in, uint16_t fb_q15)
{
    uint16_t i = *idx;
    int32_t d = buf[i];
    *lp += (d - *lp) / 2; /* 1-pole damping LPF (0.5) */
    int32_t w = in + ((*lp * (int32_t)fb_q15) / 32768);
    buf[i] = clamp16(w);
    i++;
    if (i >= len) i = 0u;
    *idx = i;
    return d;
}

static int32_t allpass_step(int16_t *buf, uint32_t len, uint16_t *idx, int32_t in)
{
    uint16_t i = *idx;
    int16_t d = buf[i];
    int32_t v = in + (d / 2);
    buf[i] = clamp16(v);
    int32_t out = d - (v / 2);
    i++;
    if (i >= len) i = 0u;
    *idx = i;
    return out;
}

static int16_t soft_clip_q15(int32_t x)
{
    if (x > 32767) x = 32767;
    if (x < -32767) x = -32767;
    int32_t x2 = q15_mul(x, x);
    int32_t x3 = q15_mul(x2, x);
    return (int16_t)(x - (x3 >> 2));
}

void dsp_render_sample(int32_t *out_l, int32_t *out_r)
{
    const ZoneDef *z = zone_get(g_state.settings.zone_id);

    /* 1. Atmospheric Backing Chord Pad (Root + 3rd + 5th, Density-sensitive breathing) */
    g_drone_ph1 += g_drone_inc1;
    g_drone_ph2 += g_drone_inc2;
    g_drone_ph3 += g_drone_inc3;
    uint32_t lfo_rate = 17530u + (uint32_t)g_state.settings.density * 350u;
    g_drone_lfo_ph += lfo_rate;
    int32_t lfo_tri = (int16_t)(g_drone_lfo_ph >> 16);
    if (lfo_tri < 0) lfo_tri = -lfo_tri;
    int32_t lfo_gain = 21000 + ((lfo_tri * 11767) >> 15);

    int32_t d1 = (hal_sine_q15(g_drone_ph1) + (hal_sine_q15(g_drone_ph1 << 1) >> 2)) >> 1;
    int32_t d2 = g_drone_fifth_en ? (hal_sine_q15(g_drone_ph2) >> 1) : 0;
    int32_t d3 = g_drone_third_en ? (hal_sine_q15(g_drone_ph3) >> 1) : 0;
    int32_t chord_sum = (d1 + d2 + d3) >> 1;
    int32_t drone_raw = q15_mul(chord_sum, lfo_gain);
    g_drone_lpf += q15_mul(drone_raw - g_drone_lpf, 5000 + (int32_t)g_state.settings.harmony * 30);
    int32_t drone = g_state.playing ? q15_mul(g_drone_lpf, g_drone_level_q15) : 0;

    /* 2. Three-Member Party FM Engine + Toll Voice */
    int32_t grit = 0;
    if (g_state.settings.intensity > 50u) {
        grit = (int32_t)(g_state.settings.intensity - 50u) * 240;
    }
    if (g_state.boss_curse_bars > 0u) {
        uint8_t zc = g_state.settings.zone_id % 9u;
        if (zc == 3u || zc == 8u || zc == 1u) grit += 12000;
    }
    /* Sorceress (slot 2) gets gentle magical vibrato shimmer */
    if (g_party[2].stage != ENV_OFF) {
        int32_t vib = ((int32_t)(g_drone_lfo_ph >> 16) >> 8); /* -128..127 */
        g_party[2].phase_c += (uint32_t)(vib * 400);
    }
    int32_t v0 = render_fm_voice(&g_party[0], &z->party_patches[0], grit);
    int32_t v1 = render_fm_voice(&g_party[1], &z->party_patches[1], grit >> 2);
    int32_t v2 = render_fm_voice(&g_party[2], &z->party_patches[2], grit >> 1);
    int32_t vt = render_fm_voice(&g_toll, &z->toll_patch, 0);

    /* 3. Zone-Specific 2-Channel FM Percussion */
    int32_t pl = render_perc_voice(&g_perc[0], &z->perc_low);
    int32_t ph = render_perc_voice(&g_perc[1], &z->perc_high);

    /* Stereo mix: Louder backing bed, Fighter 2.0x for 28mm speaker, Cleric/Sorceress 0.75x to prevent speaker blowout */
    int32_t p_mix = pl + (ph >> 1);
    int32_t bg_mix = drone + (drone >> 2) + p_mix;
    int32_t pv[3] = { v0 << 1, (v1 * 3) >> 2, (v2 * 3) >> 2 };
    if (g_state.capture.active || g_state.held_keys_mask != 0u) {
        bg_mix >>= 2;
        uint8_t act = g_state.next_slot % 3u;
        for (uint8_t s = 0; s < 3u; s++) {
            pv[s] = (s == act) ? (pv[s] + (pv[s] >> 1)) : (pv[s] >> 2);
        }
    }
    int32_t dry_l = bg_mix + pv[0] + pv[1] + pv[2] + (vt >> 1);
    int32_t dry_r = dry_l;

    /* 4. Schroeder 4-Comb + 2-Allpass Room Reverb */
    int32_t rev_in = (dry_l + dry_r) >> 2;
    int32_t c_l = comb_step(g_comb0, COMB0_LEN, &g_comb_idx[0], &g_comb_lp[0], rev_in, z->reverb_fb)
                + comb_step(g_comb1, COMB1_LEN, &g_comb_idx[1], &g_comb_lp[1], rev_in, z->reverb_fb);
    int32_t c_r = comb_step(g_comb2, COMB2_LEN, &g_comb_idx[2], &g_comb_lp[2], rev_in, z->reverb_fb)
                + comb_step(g_comb3, COMB3_LEN, &g_comb_idx[3], &g_comb_lp[3], rev_in, z->reverb_fb);

    int32_t ap_l = allpass_step(g_ap0, AP0_LEN, &g_ap_idx[0], c_l >> 1);
    int32_t ap_r = allpass_step(g_ap1, AP1_LEN, &g_ap_idx[1], c_r >> 1);

    uint16_t rev_mix = g_state.settings.reverb_override ? (uint16_t)(g_state.settings.reverb_override << 8) : (uint16_t)(z->reverb_mix << 8);
    if (g_state.boss_curse_bars > 0u) {
        uint8_t zc = g_state.settings.zone_id % 9u;
        if (zc == 0u || zc == 5u || zc == 6u) rev_mix = 32000u; /* Cathedral/Arcane/Blizzard reverb surge */
    }
    int32_t mix_l = dry_l + q15_mul(ap_l, (int32_t)rev_mix);
    int32_t mix_r = dry_r + q15_mul(ap_r, (int32_t)rev_mix);

    /* 5. ~45 Hz DC/HPF (Q8 sub-LSB precision so it decays cleanly to 0) + Q15 Soft-Clipper */
    g_hp_l += ((mix_l << 8) - g_hp_l) / 156;
    mix_l -= (g_hp_l >> 8);
    g_hp_r += ((mix_r << 8) - g_hp_r) / 156;
    mix_r -= (g_hp_r >> 8);

    *out_l = soft_clip_q15(mix_l);
    *out_r = soft_clip_q15(mix_r);
}
