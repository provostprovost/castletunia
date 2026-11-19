/* SPDX-License-Identifier: GPL-3.0-only */
#include "fantasia_engine.h"
#include "fantasia_zones.h"
#include "fantasia_dsp.h"
#include "hal_adapter.h"

FantasiaState g_state;

static void copy_str(char *dst, const char *src, uint32_t max_len)
{
    uint32_t i = 0;
    while (src[i] && i + 1u < max_len) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

uint32_t engine_prng(void)
{
    uint32_t x = g_state.prng_state;
    if (x == 0u) x = 0x13579BDFu;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    g_state.prng_state = x;
    return x;
}

static uint8_t clamp_u8(int32_t v, int32_t lo, int32_t hi)
{
    if (v < lo) return (uint8_t)lo;
    if (v > hi) return (uint8_t)hi;
    return (uint8_t)v;
}

static uint8_t slot_is_all_frozen(void)
{
    return (uint8_t)(g_state.slots[0].frozen && g_state.slots[1].frozen && g_state.slots[2].frozen);
}

static uint8_t find_next_unfrozen_slot(uint8_t start)
{
    for (uint8_t i = 0; i < NUM_PARTY_SLOTS; i++) {
        uint8_t s = (uint8_t)((start + i) % NUM_PARTY_SLOTS);
        if (!g_state.slots[s].frozen) return s;
    }
    return start % NUM_PARTY_SLOTS;
}

void engine_set_ticker(const char *msg)
{
    copy_str(g_state.ticker, msg, sizeof(g_state.ticker));
    g_state.ticker_timer = 90u;
    g_state.dirty_ticker = 1u;
}

uint8_t engine_drone_root_midi(void)
{
    /* C3 (48) .. B3 (59) so tonal center is unmistakable on 28mm speaker */
    return (uint8_t)(48u + (g_state.settings.root_semitone % 12u));
}

uint8_t engine_note_for_degree(int32_t deg, uint8_t slot)
{
    const ZoneDef *z = zone_get(g_state.settings.zone_id);
    int32_t oct = 0;
    int32_t d = deg;
    while (d < 0) { d += 7; oct--; }
    while (d >= 7) { d -= 7; oct++; }

    int32_t midi = 36 + (int32_t)(g_state.settings.root_semitone % 12u)
                 + oct * 12 + (int32_t)z->scale_semitones[d];

    /* Party Register Clamping: Slot 0 (36..55), Slot 1 (55..72), Slot 2 (67..88) */
    int32_t lo = 36, hi = 55;
    if (slot == 1u) { lo = 55; hi = 72; }
    else if (slot >= 2u) { lo = 67; hi = 88; }

    while (midi < lo) midi += 12;
    while (midi > hi) midi -= 12;
    if (midi < 0) midi = 0;
    if (midi > 127) midi = 127;
    return (uint8_t)midi;
}

static void format_pct_ticker(const char *label, uint8_t val)
{
    char msg[32];
    int i = 0;
    msg[i++] = '>'; msg[i++] = ' ';
    while (*label && i < 22) msg[i++] = *label++;
    msg[i++] = ':'; msg[i++] = ' ';
    if (val >= 100u) msg[i++] = (char)('0' + (val / 100u));
    if (val >= 10u)  msg[i++] = (char)('0' + ((val / 10u) % 10u));
    msg[i++] = (char)('0' + (val % 10u));
    msg[i++] = '%';
    msg[i] = '\0';
    engine_set_ticker(msg);
}

static void update_drone_pitch(void)
{
    const ZoneDef *z = zone_get(g_state.settings.zone_id);
    uint8_t d = g_state.active_chord_deg % 7u;
    uint8_t root = (uint8_t)(48u + ((g_state.settings.root_semitone + z->scale_semitones[d]) % 12u));
    uint8_t fifth = (uint8_t)(root + 7u);
    uint8_t fifth_en = (g_state.settings.harmony >= 20u) ? 1u : 0u;
    dsp_drone_set(root, fifth, fifth_en, g_state.settings.mix_drone);
}

void engine_init_defaults(void)
{
    for (uint32_t i = 0; i < sizeof(g_state); i++) {
        ((uint8_t *)&g_state)[i] = 0;
    }
    g_state.settings.magic = SETTINGS_MAGIC;
    g_state.settings.version = 1u;
    g_state.settings.zone_id = 0u;
    g_state.settings.root_semitone = 2u; /* D */
    g_state.settings.bpm = G_ZONES[0].default_bpm;
    g_state.settings.intensity = 40u;
    g_state.settings.drift = 32u;
    g_state.settings.density = 52u;
    g_state.settings.harmony = 52u;
    g_state.settings.weave = 45u;
    g_state.settings.mix_drone = 88u;
    g_state.settings.mix_slots[0] = 108u;
    g_state.settings.mix_slots[1] = 100u;
    g_state.settings.mix_slots[2] = 104u;
    g_state.settings.mix_perc = 96u;
    g_state.settings.reverb_override = 0u;

    for (uint8_t s = 0; s < NUM_PARTY_SLOTS; s++) {
        g_state.slots[s].base_length = 16u;
        g_state.slots[s].weave_length = (s == 0u) ? 5u : (s == 1u ? 7u : 11u);
        g_state.slots[s].fade_gain_q8 = 255u;
        for (uint8_t n = 0; n < MAX_SEED_NOTES; n++) {
            g_state.slots[s].scale_deg[n] = -1;
        }
    }

    g_state.playing = 1u;
    g_state.prng_state = 0x5EED16B1u;
    g_state.dirty_diorama = 1u;
    g_state.dirty_header = 1u;
    g_state.dirty_party = 1u;
    engine_set_ticker("> ENTERING THE CRYPT");
    update_drone_pitch();
}

void engine_set_zone(uint8_t zone_id)
{
    uint8_t z = (uint8_t)(zone_id % NUM_ZONES);
    if (z != g_state.settings.zone_id) {
        g_state.slide_prev_zone = g_state.settings.zone_id;
        g_state.slide_frames = 12u;
        g_state.settings.zone_id = z;
        g_state.settings.bpm = G_ZONES[z].default_bpm;
        g_state.dirty_diorama = 1u;
        g_state.dirty_header = 1u;
        char msg[32];
        msg[0] = '>'; msg[1] = ' ';
        copy_str(msg + 2, G_ZONES[z].name, sizeof(msg) - 2u);
        engine_set_ticker(msg);
        update_drone_pitch();
    }
}

void engine_advance_next_slot(void)
{
    g_state.next_slot = find_next_unfrozen_slot((uint8_t)((g_state.next_slot + 1u) % NUM_PARTY_SLOTS));
    g_state.dirty_party = 1u;
}

void engine_toggle_freeze(uint8_t slot)
{
    if (slot >= NUM_PARTY_SLOTS) return;
    g_state.slots[slot].frozen ^= 1u;
    if (g_state.slots[slot].frozen && g_state.next_slot == slot) {
        g_state.next_slot = find_next_unfrozen_slot((uint8_t)((slot + 1u) % NUM_PARTY_SLOTS));
    }
    g_state.dirty_party = 1u;
    engine_set_ticker(g_state.slots[slot].frozen ? "> PARTY MEMBER FROZEN" : "> THAWED!");
}

void engine_clear_slot(uint8_t slot)
{
    if (slot >= NUM_PARTY_SLOTS || g_state.slots[slot].frozen) return;
    SeedSlot empty;
    for (uint32_t i = 0; i < sizeof(empty); i++) ((uint8_t *)&empty)[i] = 0;
    empty.base_length = 16u;
    empty.weave_length = (slot == 0u) ? 5u : (slot == 1u ? 7u : 11u);
    empty.fade_gain_q8 = 255u;
    for (uint8_t n = 0; n < MAX_SEED_NOTES; n++) empty.scale_deg[n] = -1;
    g_state.staging_slots[slot] = empty;
    g_state.slot_pending[slot] = 1u;
    g_state.dirty_party = 1u;
    engine_set_ticker("> SLOT CLEARED!");
}

void engine_clear_all_unfrozen(void)
{
    for (uint8_t s = 0; s < NUM_PARTY_SLOTS; s++) {
        if (!g_state.slots[s].frozen) {
            engine_clear_slot(s);
        }
    }
    engine_set_ticker("> CLEARED!");
}

static void apply_drift_to_slot(SeedSlot *sl, uint8_t drift)
{
    if (sl->frozen || sl->note_count == 0u) return;
    uint32_t r = engine_prng();
    uint8_t idx = (uint8_t)(r % sl->note_count);
    int32_t deg = sl->scale_deg[idx];
    if (deg < 0) deg = 0;

    if (drift <= 35u) {
        if (r & 0x100u) {
            deg = (deg >= 7) ? (deg - 7) : (deg + 7);
        } else if (sl->note_count > 1u) {
            deg = -1; /* Ghost rest */
        }
    } else if (drift <= 70u) {
        int32_t delta = ((r >> 8) & 1u) ? 1 : -1;
        if ((r >> 9) & 1u) delta *= 2;
        deg += delta;
        if (deg < 0) deg = 0;
        if (deg > 26) deg = 26;
    } else {
        deg = (int32_t)((r >> 8) % 14u);
        uint8_t step = (uint8_t)((sl->step_pos[idx] + (((r >> 12) & 1u) ? 1u : 15u)) % sl->base_length);
        sl->step_pos[idx] = step;
    }
    sl->scale_deg[idx] = (int8_t)deg;
}

void seed_cancel_capture(void)
{
    g_state.capture.active = 0u;
    g_state.capture.count = 0u;
}

void seed_commit_capture(void)
{
    if (!g_state.capture.active || g_state.capture.count == 0u) {
        g_state.capture.active = 0u;
        return;
    }

    uint8_t s = g_state.next_slot;
    if (g_state.slots[s].frozen) {
        s = find_next_unfrozen_slot(s);
    }
    if (g_state.slots[s].frozen) {
        g_state.capture.active = 0u;
        g_state.capture.count = 0u;
        return;
    }

    SeedSlot ns;
    for (uint32_t i = 0; i < sizeof(ns); i++) ((uint8_t *)&ns)[i] = 0;
    for (uint8_t n = 0; n < MAX_SEED_NOTES; n++) ns.scale_deg[n] = -1;

    uint8_t count = g_state.capture.count;
    if (count > MAX_SEED_NOTES) count = MAX_SEED_NOTES;

    uint16_t used_steps = 0u;
    uint32_t max_step = 0u;
    for (uint8_t i = 0; i < count; i++) {
        uint32_t step = (i == 0u && g_state.capture.rel_tick[0] < 6u)
                      ? 0u
                      : ((g_state.capture.rel_tick[i] + 3u) / 6u) % 16u;
        /* Nudge to next free 16th step so no played note is ever dropped */
        for (uint8_t tries = 0; tries < 16u && (used_steps & (1u << step)); tries++) {
            step = (step + 1u) % 16u;
        }
        used_steps |= (uint16_t)(1u << step);
        if (step > max_step) max_step = step;
        ns.scale_deg[i] = (int8_t)g_state.capture.scale_deg[i];
        ns.step_pos[i] = (uint8_t)step;
    }

    ns.note_count = count;
    ns.base_length = (count <= 3u && max_step < 8u) ? 16u : 32u;
    static const uint8_t WEAVE_LENS[3][2] = { {7u, 11u}, {11u, 13u}, {13u, 17u} };
    ns.weave_length = WEAVE_LENS[s][engine_prng() & 1u];
    ns.playhead = 0u;
    ns.frozen = 0u;
    ns.fade_gain_q8 = (g_state.settings.intensity > 75u) ? 255u : 0u;
    ns.anim_timer = 12u;

    g_state.staging_slots[s] = ns;
    g_state.slot_pending[s] = 1u;

    /* Cross-Pollination on other unfrozen slots */
    for (uint8_t other = 0; other < NUM_PARTY_SLOTS; other++) {
        if (other != s && !g_state.slots[other].frozen) {
            g_state.cross_accent_bars[other] = 2u;
        }
    }

    g_state.next_slot = find_next_unfrozen_slot((uint8_t)((s + 1u) % NUM_PARTY_SLOTS));
    g_state.capture.active = 0u;
    g_state.capture.count = 0u;
    g_state.silence_ticks = 0u;
    g_state.silence_ms = 0u;
    g_state.attack_timer[s] = 15u;
    g_state.boss_hit_timer = 12u;
    dsp_attack_sfx(s);
    g_state.dirty_diorama = 1u;
    g_state.dirty_party = 1u;
    static const char *const ATK_MSG[3] = {
        "> FIGHTER: SLASH!", "> CLERIC: HOLY!", "> SORCERESS: FLARE!"
    };
    engine_set_ticker(ATK_MSG[s]);
}

void engine_on_key_down(uint8_t key_idx)
{
    if (key_idx >= 27u) return;
    g_state.held_keys_mask |= (1u << key_idx);
    g_state.silence_ticks = 0u;
    g_state.silence_ms = 0u;

    /* Live Jam Bypass if all 3 Party slots are frozen */
    if (slot_is_all_frozen()) {
        uint8_t midi = engine_note_for_degree((int32_t)key_idx, 2u);
        dsp_note_on(2u, midi, 32767u, 16u);
        return;
    }

    uint8_t target_slot = g_state.next_slot;
    if (g_state.slots[target_slot].frozen) {
        target_slot = find_next_unfrozen_slot(target_slot);
    }

    uint8_t midi = engine_note_for_degree((int32_t)key_idx, target_slot);
    dsp_note_on(target_slot, midi, 32767u, 16u);

    if (!g_state.capture.active) {
        g_state.capture.active = 1u;
        g_state.capture.start_tick = g_state.total_ticks;
        g_state.capture.scale_deg[0] = key_idx;
        g_state.capture.rel_tick[0] = 0u;
        g_state.capture.count = 1u;
    } else if (g_state.capture.count < MAX_SEED_NOTES) {
        uint8_t idx = g_state.capture.count;
        g_state.capture.scale_deg[idx] = key_idx;
        g_state.capture.rel_tick[idx] = g_state.total_ticks - g_state.capture.start_tick;
        g_state.capture.count++;
    }

    if (g_state.capture.count >= MAX_SEED_NOTES) {
        seed_commit_capture();
    }
}

void engine_on_key_up(uint8_t key_idx)
{
    if (key_idx >= 27u) return;
    g_state.held_keys_mask &= ~(1u << key_idx);
}

void engine_spell_haste_toggle(void)
{
    g_state.spells.haste_active ^= 1u;
    g_state.dirty_party = 1u;
    engine_set_ticker(g_state.spells.haste_active ? "> CAST HASTE!" : "> HASTE FADED");
}

void engine_spell_mirror_toggle(void)
{
    g_state.spells.mirror_active ^= 1u;
    g_state.dirty_diorama = 1u;
    engine_set_ticker(g_state.spells.mirror_active ? "> CAST MIRROR!" : "> MIRROR FADED");
}

void engine_spell_shatter(void)
{
    for (uint8_t s = 0; s < NUM_PARTY_SLOTS; s++) {
        if (!g_state.slots[s].frozen && g_state.slots[s].note_count > 0u) {
            if ((engine_prng() % 100u) < 75u) {
                SeedSlot tmp = g_state.slots[s];
                apply_drift_to_slot(&tmp, 80u);
                tmp.anim_timer = 10u;
                g_state.staging_slots[s] = tmp;
                g_state.slot_pending[s] = 1u;
            }
        }
    }
    g_state.shake_timer = 8u;
    g_state.shake_amp = 2u;
    g_state.dirty_diorama = 1u;
    g_state.dirty_party = 1u;
    engine_set_ticker("> CAST SHATTER!");
}

void engine_spell_resolve(void)
{
    g_state.spells.resolve_armed = 1u;
    g_state.gold_flash_timer = 10u;
    engine_set_ticker("> CAST RESOLVE!");
}

void engine_spell_conjure(void)
{
    uint8_t s = g_state.next_slot;
    if (g_state.slots[s].frozen) s = find_next_unfrozen_slot(s);
    if (g_state.slots[s].frozen) {
        engine_set_ticker("> ALL PARTY FROZEN");
        return;
    }

    SeedSlot ns;
    for (uint32_t i = 0; i < sizeof(ns); i++) ((uint8_t *)&ns)[i] = 0;
    for (uint8_t n = 0; n < MAX_SEED_NOTES; n++) ns.scale_deg[n] = -1;

    uint8_t count = (uint8_t)(3u + (engine_prng() % 4u)); /* 3..6 notes */
    static const uint8_t STEPS[7] = {2u, 4u, 6u, 8u, 10u, 12u, 14u};
    static const int8_t CHORD_DEGS[3] = {0, 2, 4};

    uint8_t used_mask = 0u;
    ns.scale_deg[0] = (int8_t)(g_state.active_chord_deg % 7u);
    ns.step_pos[0] = 0u;

    uint8_t passing_used = 0u;
    for (uint8_t i = 1u; i < count; i++) {
        uint8_t pick;
        do {
            pick = (uint8_t)(engine_prng() % 7u);
        } while (used_mask & (1u << pick));
        used_mask |= (1u << pick);

        int8_t deg = (int8_t)(g_state.active_chord_deg + CHORD_DEGS[engine_prng() % 3u]);
        if (!passing_used && (engine_prng() % 100u) < 30u) {
            deg += ((engine_prng() & 1u) ? 1 : 3);
            passing_used = 1u;
        }
        ns.scale_deg[i] = (int8_t)(deg % 27);
        ns.step_pos[i] = STEPS[pick];
    }

    ns.note_count = count;
    ns.base_length = 16u;
    ns.weave_length = (s == 0u) ? 5u : (s == 1u ? 7u : 11u);
    ns.fade_gain_q8 = 255u;
    ns.anim_timer = 12u;

    g_state.staging_slots[s] = ns;
    g_state.slot_pending[s] = 1u;
    g_state.next_slot = find_next_unfrozen_slot((uint8_t)((s + 1u) % NUM_PARTY_SLOTS));
    g_state.attack_timer[s] = 15u;
    g_state.boss_hit_timer = 12u;
    dsp_attack_sfx(s);
    g_state.dirty_diorama = 1u;
    g_state.dirty_party = 1u;
    engine_set_ticker("> CONJURED!");
}

void engine_spell_toll(void)
{
    dsp_toll_trigger(g_state.settings.zone_id, engine_drone_root_midi());
    g_state.toll_ring_timer = 14u;
    g_state.dirty_diorama = 1u;
    engine_set_ticker("> CAST TOLL!");
}

void engine_adjust_macro(uint8_t enc_id, int32_t delta, uint8_t glo_held)
{
    if (delta == 0) return;
    if (glo_held) {
        switch (enc_id) {
            case ENC_SELECT: {
                int32_t r = ((int32_t)g_state.settings.root_semitone + delta) % 12;
                if (r < 0) r += 12;
                g_state.settings.root_semitone = (uint8_t)r;
                g_state.dirty_header = 1u;
                update_drone_pitch();
                static const char *const NAMES[12] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
                char msg[24] = "> ROOT: ";
                copy_str(msg + 8, NAMES[r], sizeof(msg) - 8u);
                engine_set_ticker(msg);
                break;
            }
            case ENC_PRESETS:
                g_state.settings.mix_drone = clamp_u8((int32_t)g_state.settings.mix_drone + delta * 4, 0, 127);
                update_drone_pitch();
                engine_set_ticker("> DRONE LEVEL");
                break;
            case ENC_ALGORITHM:
                g_state.settings.reverb_override = clamp_u8((int32_t)g_state.settings.reverb_override + delta * 4, 0, 127);
                engine_set_ticker("> REVERB LEVEL");
                break;
            case ENC_KNOB1:
                g_state.settings.mix_slots[0] = clamp_u8((int32_t)g_state.settings.mix_slots[0] + delta * 4, 0, 127);
                engine_set_ticker("> FIGHTER VOL");
                break;
            case ENC_KNOB2:
                g_state.settings.mix_slots[1] = clamp_u8((int32_t)g_state.settings.mix_slots[1] + delta * 4, 0, 127);
                engine_set_ticker("> CLERIC VOL");
                break;
            case ENC_KNOB3:
                g_state.settings.mix_slots[2] = clamp_u8((int32_t)g_state.settings.mix_slots[2] + delta * 4, 0, 127);
                engine_set_ticker("> SORCERESS VOL");
                break;
            case ENC_KNOB4:
                g_state.settings.mix_perc = clamp_u8((int32_t)g_state.settings.mix_perc + delta * 4, 0, 127);
                engine_set_ticker("> PERC VOL");
                break;
            default:
                break;
        }
        return;
    }

    switch (enc_id) {
        case ENC_SELECT: {
            int32_t z = ((int32_t)g_state.settings.zone_id + delta) % (int32_t)NUM_ZONES;
            if (z < 0) z += (int32_t)NUM_ZONES;
            engine_set_zone((uint8_t)z);
            break;
        }
        case ENC_PRESETS:
            g_state.settings.bpm = clamp_u8((int32_t)g_state.settings.bpm + delta * 2, 55, 160);
            g_state.dirty_header = 1u;
            break;
        case ENC_ALGORITHM:
            g_state.settings.intensity = clamp_u8((int32_t)g_state.settings.intensity + delta * 4, 0, 100);
            format_pct_ticker("INTENSITY", g_state.settings.intensity);
            break;
        case ENC_KNOB1:
            g_state.settings.drift = clamp_u8((int32_t)g_state.settings.drift + delta * 4, 0, 100);
            format_pct_ticker("DRIFT", g_state.settings.drift);
            break;
        case ENC_KNOB2:
            g_state.settings.density = clamp_u8((int32_t)g_state.settings.density + delta * 4, 0, 100);
            format_pct_ticker("DENSITY", g_state.settings.density);
            break;
        case ENC_KNOB3:
            g_state.settings.harmony = clamp_u8((int32_t)g_state.settings.harmony + delta * 4, 0, 100);
            update_drone_pitch();
            format_pct_ticker("HARMONY", g_state.settings.harmony);
            break;
        case ENC_KNOB4:
            g_state.settings.weave = clamp_u8((int32_t)g_state.settings.weave + delta * 4, 0, 100);
            format_pct_ticker("WEAVE", g_state.settings.weave);
            break;
        default:
            break;
    }
}

static void trigger_slot_step(uint8_t s, uint8_t step)
{
    SeedSlot *sl = &g_state.slots[s];
    if (sl->note_count == 0u) return;

    uint8_t len = (g_state.settings.weave > 40u) ? sl->weave_length : sl->base_length;
    if (len == 0u) len = 16u;
    uint8_t pos = (uint8_t)((g_state.bar_count * 16u + step) % len);

    /* Wrap detection -> Drift & Cross-pollination decay */
    if (pos == 0u) {
        if (g_state.settings.drift >= 10u && !sl->frozen) {
            if ((engine_prng() % 100u) < g_state.settings.drift) {
                apply_drift_to_slot(sl, g_state.settings.drift);
            }
        }
        if (g_state.cross_accent_bars[s] > 0u) {
            g_state.cross_accent_bars[s]--;
        }
    }

    if (sl->fade_gain_q8 < 255u) {
        uint16_t fg = (uint16_t)sl->fade_gain_q8 + 16u;
        sl->fade_gain_q8 = (fg > 255u) ? 255u : (uint8_t)fg;
    }

    int32_t deg = -1;
    uint8_t vel_pct = 100u;

    if (sl->note_count == 1u) {
        /* Single-Note Seed Evolution */
        int8_t base_deg = sl->scale_deg[0];
        if (base_deg >= 0) {
            uint8_t other_occ = 0u;
            for (uint8_t o = 0; o < NUM_PARTY_SLOTS; o++) {
                if (o != s && g_state.slots[o].note_count > 0u) other_occ = 1u;
            }
            if (pos == 0u) {
                deg = base_deg;
            } else if (pos == 8u && (other_occ || g_state.settings.density > 50u)) {
                deg = base_deg + 4;
            } else if (pos == 12u && g_state.settings.density > 75u) {
                deg = base_deg + 7;
                vel_pct = 75u;
            }
        }
    } else {
        for (uint8_t i = 0; i < sl->note_count; i++) {
            uint8_t target_pos = sl->step_pos[i];
            if (g_state.spells.mirror_active) {
                target_pos = (uint8_t)((len - 1u - (target_pos % len)) % len);
            }
            if (target_pos == pos) {
                /* Density thinning (0..35%) while anchoring step 0 */
                if (g_state.settings.density <= 35u && pos != 0u) {
                    if ((engine_prng() % 100u) > (uint32_t)(g_state.settings.density * 2u + 25u)) {
                        continue;
                    }
                }
                deg = sl->scale_deg[i];
                break;
            }
        }
        /* Density blooming (65..100%) on empty 16ths */
        if (deg < 0 && g_state.settings.density >= 65u) {
            if ((engine_prng() % 100u) < (uint32_t)(g_state.settings.density - 55u)) {
                static const int8_t ORN[3] = {2, 4, 7};
                int8_t base = sl->scale_deg[0];
                if (base >= 0) deg = base + ORN[engine_prng() % 3u];
            }
        }
    }

    if (deg < 0) return;

    /* 4-Bar A/B/A/C Phrase Evolution */
    uint8_t form_bar = g_state.bar_count & 3u;
    if (form_bar == 1u && (step & 3u) == 2u) {
        deg += 2; /* Melodic answer */
    } else if (form_bar == 3u && step >= 12u) {
        deg += 4; /* Turnaround cadence lift */
    }

    /* Harmony chord-tone gravity / voice-leading */
    if (s == 0u && g_state.settings.harmony >= 15u) {
        deg = (deg % 7) + (int32_t)g_state.active_chord_deg;
    } else if (s == 1u && g_state.settings.harmony > 45u) {
        static const int8_t ARP[4] = {0, 2, 4, 6};
        uint8_t max_i = (g_state.settings.harmony > 75u) ? 4u : 3u;
        deg = (int32_t)g_state.active_chord_deg + ARP[pos % max_i];
    } else if (s == 2u && g_state.settings.harmony > 65u && (pos & 1u)) {
        deg += ((pos & 2u) ? 4 : 6); /* 5th/7th harmonic color */
    }

    if (g_state.spells.mirror_active) {
        deg = (28 - deg) % 27;
        if (deg < 0) deg += 27;
        if (s == 1u && g_state.settings.weave > 60u && g_state.settings.harmony > 50u) {
            /* Snap inverted White Mage note to nearest chord tone */
            int32_t c0 = (int32_t)g_state.active_chord_deg;
            int32_t c1 = c0 + 2;
            int32_t c2 = c0 + 4;
            int32_t d0 = deg - c0; if (d0 < 0) d0 = -d0;
            int32_t d1 = deg - c1; if (d1 < 0) d1 = -d1;
            int32_t d2 = deg - c2; if (d2 < 0) d2 = -d2;
            deg = (d0 <= d1 && d0 <= d2) ? c0 : ((d1 <= d2) ? c1 : c2);
        }
    }

    /* Cross-pollination boost */
    if (g_state.cross_accent_bars[s] > 0u) {
        vel_pct = 120u;
        if ((engine_prng() % 100u) < 35u) deg += 7;
    }

    uint8_t midi = engine_note_for_degree(deg, s);
    uint32_t vel_q15 = ((uint32_t)g_state.settings.mix_slots[s] * 256u * vel_pct) / 100u;
    vel_q15 = (vel_q15 * (uint32_t)sl->fade_gain_q8) >> 8;
    if (vel_q15 > 32767u) vel_q15 = 32767u;

    uint16_t gate = (g_state.spells.haste_active && s >= 1u) ? 4u : 8u;
    dsp_note_on(s, midi, (uint16_t)vel_q15, gate);
    sl->playhead = pos;
    sl->anim_timer = 8u;
    g_state.dirty_party = 1u;

    if (midi >= 53u && midi < 80u) {
        g_state.active_note_leds |= (1u << (midi - 53u));
    }
}

void engine_tick_ppqn(void)
{
    g_state.total_ticks++;
    dsp_tick_gates();

    /* Silence-triggered capture commit check */
    if (g_state.capture.active) {
        for (uint8_t i = 0; i < g_state.capture.count; i++) {
            /* Keep relative tick updated only on key_down */
        }
        if (g_state.held_keys_mask == 0u) {
            g_state.silence_ticks++;
            /* Approximate ms per tick = 60000 / (bpm * 24) */
            uint32_t denom = (uint32_t)g_state.settings.bpm * 24u;
            if (denom > 0u) {
                g_state.silence_ms += (60000u / denom);
            }
            if (g_state.silence_ticks >= 36u && g_state.silence_ms >= 900u) {
                seed_commit_capture();
            }
        }
    }

    if (!g_state.playing) return;

    uint8_t t = g_state.tick_in_16th;
    uint8_t step = g_state.step_16th;

    /* Swing calculation for Weave 21..60% */
    uint8_t swing_ticks = 0u;
    if (g_state.settings.weave >= 21u && g_state.settings.weave <= 60u) {
        swing_ticks = (uint8_t)((g_state.settings.weave - 20u) / 13u);
    }

    uint8_t fire_main = 0u;
    uint8_t fire_haste = 0u;
    if (swing_ticks == 0u) {
        fire_main = (t == 0u);
        fire_haste = (t == 0u || t == 3u);
    } else {
        if ((step & 1u) == 0u) {
            fire_main = (t == 0u);
        } else {
            fire_main = (t == swing_ticks);
        }
        fire_haste = (t == 0u || t == 3u);
    }

    if (fire_main) {
        /* Update active chord degree & retune Drone on chord changes */
        const ZoneDef *z = zone_get(g_state.settings.zone_id);
        uint8_t bar_idx = g_state.bar_count & 3u;
        uint8_t prev_chord = g_state.active_chord_deg;
        if (g_state.spells.resolve_armed) {
            g_state.active_chord_deg = (step < 8u) ? 4u : 0u;
        } else if (g_state.settings.harmony >= 70u) {
            g_state.active_chord_deg = z->high_prog[bar_idx];
        } else if (g_state.settings.harmony >= 15u) {
            g_state.active_chord_deg = z->mid_prog[bar_idx];
        } else {
            g_state.active_chord_deg = 0u;
        }
        if (g_state.active_chord_deg != prev_chord) {
            update_drone_pitch();
        }

        if (step == 0u) g_state.beat1_flash = 4u;

        /* Slot 0 always at 1x */
        trigger_slot_step(0u, step);
        if (!g_state.spells.haste_active) {
            trigger_slot_step(1u, step);
            trigger_slot_step(2u, step);
        }

        /* Percussion evaluation */
        uint8_t inten = g_state.settings.intensity;
        if (inten >= 26u && g_state.settings.mix_perc > 0u) {
            uint16_t low_mask = 0u, high_mask = 0u;
            if (inten <= 50u) {
                low_mask = z->low_mask_march & 0x0101u;
            } else if (inten <= 75u) {
                low_mask = z->low_mask_march;
                high_mask = z->high_mask_march;
            } else {
                low_mask = z->low_mask_battle;
                high_mask = z->high_mask_battle;
                if ((g_state.bar_count & 3u) == 3u && step >= 12u) {
                    high_mask |= (uint16_t)(1u << step);
                }
            }
            uint16_t vel = (uint16_t)((uint32_t)g_state.settings.mix_perc * 160u);
            if (low_mask & (1u << step)) dsp_perc_trigger(0u, vel);
            if (high_mask & (1u << step)) dsp_perc_trigger(1u, vel);
            if (inten > 75u && (step & 3u) == 0u) {
                g_state.shake_timer = 2u;
                g_state.shake_amp = 1u;
            }
        }
    }

    if (g_state.spells.haste_active && fire_haste) {
        uint8_t hstep = (uint8_t)((step * 2u + (t >= 3u ? 1u : 0u)) & 15u);
        trigger_slot_step(1u, hstep);
        trigger_slot_step(2u, hstep);
    }

    g_state.tick_in_16th++;
    if (g_state.tick_in_16th >= 6u) {
        g_state.tick_in_16th = 0u;
        g_state.step_16th++;
        if (g_state.step_16th >= 16u) {
            g_state.step_16th = 0u;
            g_state.bar_count++;
            if (g_state.spells.resolve_armed) {
                g_state.spells.resolve_armed = 0u;
            }
        }
    }
}
