/* SPDX-License-Identifier: GPL-3.0-only */
#include "castletunia_engine.h"
#include "castletunia_zones.h"
#include "castletunia_dsp.h"
#include "hal_adapter.h"

CastletuniaState g_state;

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
    g_state.ticker_timer = (g_state.settings.bpm < 90u) ? 2u : 3u; /* <= 2s, clears on beat */
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

    /* Strictly monotonic 27-key mapping per Party role (no octave wrap cliffs!) */
    static const int32_t SLOT_BASE[3] = { 48, 55, 60 }; /* Fighter=C3+, Cleric=G3+, Sorceress=C4+ */
    int32_t base = SLOT_BASE[slot % 3u];
    int32_t midi = base + (int32_t)(g_state.settings.root_semitone % 12u)
                 + oct * 12 + (int32_t)z->scale_semitones[d];

    if (midi < 24) midi = 24;
    if (midi > 108) midi = 108;
    return (uint8_t)midi;
}

static void format_pct_ticker(const char *label, uint8_t val)
{
    char msg[32];
    int i = 0;
    msg[i++] = '>'; msg[i++] = ' ';
    while (*label && i < 10) msg[i++] = *label++;
    msg[i++] = ' '; msg[i++] = '[';
    uint8_t filled = (uint8_t)((val + 5u) / 10u);
    if (filled > 10u) filled = 10u;
    for (uint8_t b = 0; b < 10u; b++) {
        msg[i++] = (b < filled) ? '=' : '.';
    }
    msg[i++] = ']'; msg[i++] = ' ';
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
    uint8_t root_off = (g_state.settings.root_semitone + z->scale_semitones[d]) % 12u;
    uint8_t third_off = z->scale_semitones[(d + 2u) % 7u];
    uint8_t fifth_off = z->scale_semitones[(d + 4u) % 7u];
    uint8_t third_iv = (uint8_t)((third_off + 12u - z->scale_semitones[d]) % 12u);
    uint8_t fifth_iv = (uint8_t)((fifth_off + 12u - z->scale_semitones[d]) % 12u);

    uint8_t root = (uint8_t)(48u + root_off);
    uint8_t third = (uint8_t)(root + third_iv + 12u); /* Upper-octave voicing so chord changes cut through */
    uint8_t fifth = (uint8_t)(root + fifth_iv + 12u);
    uint8_t fifth_en = (g_state.settings.harmony >= 20u) ? 1u : 0u;
    uint8_t third_en = (g_state.settings.harmony >= 35u) ? 1u : 0u;
    dsp_drone_set(root, third, fifth, third_en, fifth_en, g_state.settings.mix_drone);
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
    g_state.settings.drift = 36u;
    g_state.settings.density = 52u;
    g_state.settings.harmony = 56u;
    g_state.settings.weave = 24u;
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

    g_state.playing = 0u; /* Quiet on power-on until a phrase activates or a spell is cast */
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
        int diff = (int)z - (int)g_state.settings.zone_id;
        if (diff == 1 || diff == -(int)(NUM_ZONES - 1u)) {
            g_state.slide_dir = 1;
        } else if (diff == -1 || diff == (int)(NUM_ZONES - 1u)) {
            g_state.slide_dir = -1;
        } else {
            g_state.slide_dir = (diff > 0) ? 1 : -1;
        }
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
    if (g_state.slots[slot].frozen) {
        dsp_voice_quick_release(slot);
        if (g_state.next_slot == slot) {
            g_state.next_slot = find_next_unfrozen_slot((uint8_t)((slot + 1u) % NUM_PARTY_SLOTS));
        }
    }
    g_state.dirty_party = 1u;
    engine_set_ticker(g_state.slots[slot].frozen ? "> PARTY MEMBER FROZEN" : "> THAWED!");
}

void engine_clear_slot(uint8_t slot)
{
    if (slot >= NUM_PARTY_SLOTS) return;
    /* If target slot is already empty or frozen, find the most recently active unfrozen slot */
    if (g_state.slots[slot].frozen || g_state.slots[slot].note_count == 0u) {
        for (uint8_t i = 1u; i <= NUM_PARTY_SLOTS; i++) {
            uint8_t prev = (uint8_t)((slot + NUM_PARTY_SLOTS - i) % NUM_PARTY_SLOTS);
            if (!g_state.slots[prev].frozen && g_state.slots[prev].note_count > 0u) {
                slot = prev;
                break;
            }
        }
    }
    if (g_state.slots[slot].frozen) return;

    SeedSlot empty;
    for (uint32_t i = 0; i < sizeof(empty); i++) ((uint8_t *)&empty)[i] = 0;
    empty.base_length = 16u;
    empty.weave_length = (slot == 0u) ? 7u : (slot == 1u ? 11u : 13u);
    empty.fade_gain_q8 = 255u;
    for (uint8_t n = 0; n < MAX_SEED_NOTES; n++) empty.scale_deg[n] = -1;
    g_state.slots[slot] = empty;
    g_state.staging_slots[slot] = empty;
    g_state.slot_pending[slot] = 1u;
    dsp_voice_quick_release(slot);
    g_state.next_slot = slot;
    g_state.dirty_party = 1u;
    static const char *const CLR_MSG[3] = {
        "> CLEARED FIGHTER!", "> CLEARED CLERIC!", "> CLEARED SORCERESS!"
    };
    engine_set_ticker(CLR_MSG[slot]);
}

void engine_clear_all_unfrozen(void)
{
    for (uint8_t s = 0; s < NUM_PARTY_SLOTS; s++) {
        if (!g_state.slots[s].frozen) {
            SeedSlot empty;
            for (uint32_t i = 0; i < sizeof(empty); i++) ((uint8_t *)&empty)[i] = 0;
            empty.base_length = 16u;
            empty.weave_length = (s == 0u) ? 7u : (s == 1u ? 11u : 13u);
            empty.fade_gain_q8 = 255u;
            for (uint8_t n = 0; n < MAX_SEED_NOTES; n++) empty.scale_deg[n] = -1;
            g_state.slots[s] = empty;
            g_state.staging_slots[s] = empty;
            g_state.slot_pending[s] = 1u;
            dsp_voice_quick_release(s);
        }
    }
    g_state.next_slot = find_next_unfrozen_slot(0u);
    g_state.dirty_party = 1u;
    engine_set_ticker("> CLEARED!");
}

static int32_t snap_to_chord_gravity(int32_t deg, int32_t chord_root)
{
    /* Pull scale degree 1 step closer to nearest triad tone (root, +2, +4) */
    int32_t rel = (deg - chord_root) % 7;
    if (rel < 0) rel += 7;
    if (rel == 1) return deg - 1;      /* 2nd -> root */
    if (rel == 3) return deg - 1;      /* 4th -> 3rd */
    if (rel == 5) return deg - 1;      /* 6th -> 5th */
    if (rel == 6) return deg + 1;      /* 7th -> octave root */
    return deg;
}

static void apply_drift_to_slot(SeedSlot *sl, uint8_t drift)
{
    if (sl->frozen || sl->note_count == 0u) return;
    uint32_t r = engine_prng();
    uint8_t idx = (uint8_t)(r % sl->note_count);
    int32_t deg = sl->scale_deg[idx];
    if (deg < 0) deg = (int32_t)(g_state.active_chord_deg % 7u);

    /* 1. Self-Organizing Rhythmic & Harmonic Gravity (pulls phrase toward musical grid & chord) */
    uint8_t st = sl->step_pos[idx];
    if ((st & 1u) != 0u && (r & 3u) != 0u) {
        /* Pull odd 16th steps onto clean 8th-note grid if unoccupied */
        uint8_t target_st = (uint8_t)((st + 1u) & 14u);
        uint8_t occ = 0u;
        for (uint8_t k = 0; k < sl->note_count; k++) {
            if (k != idx && sl->step_pos[k] == target_st) occ = 1u;
        }
        if (!occ) sl->step_pos[idx] = target_st;
    }

    int32_t c_root = (int32_t)(g_state.active_chord_deg % 7u);
    deg = snap_to_chord_gravity(deg, c_root);

    /* 2. Stepwise Motif-Preserving Variation (avoids jagged random leaps) */
    if (drift <= 45u) {
        if ((r & 3u) == 0u) {
            deg += ((r & 4u) ? 1 : -1);
        }
    } else {
        int32_t step_dir = ((r & 4u) ? 1 : -1);
        deg = snap_to_chord_gravity(deg + step_dir, c_root);
    }

    if (deg < 0) deg = 0;
    if (deg > 21) deg = 14 + (deg % 7);
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
    uint32_t span = (count > 1u) ? g_state.capture.rel_tick[count - 1u] : 0u;
    for (uint8_t i = 0; i < count; i++) {
        uint32_t step;
        if (count == 1u) {
            step = 0u;
        } else if (span < 24u) {
            /* Rapid phrase: place cleanly on consecutive 8th/16th steps */
            step = ((uint32_t)i * ((count <= 4u) ? 2u : 1u)) % 16u;
        } else {
            /* Snap to nearest 16th-note (6 ticks = 1/4 beat, preserving beats & half-beats) */
            step = ((g_state.capture.rel_tick[i] + 3u) / 6u) % 16u;
        }
        /* Nudge collisions to the next free 16th (+1) instead of jumping +3 */
        for (uint8_t tries = 0; tries < 16u && (used_steps & (1u << step)); tries++) {
            step = (step + 1u) % 16u;
        }
        used_steps |= (uint16_t)(1u << step);
        if (step > max_step) max_step = step;
        ns.scale_deg[i] = (int8_t)g_state.capture.scale_deg[i];
        ns.step_pos[i] = (uint8_t)step;
    }

    ns.note_count = count;
    ns.base_length = 16u;
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

    g_state.slots[s] = ns;
    g_state.staging_slots[s] = ns;
    g_state.slot_pending[s] = 0u;
    g_state.next_slot = find_next_unfrozen_slot((uint8_t)((s + 1u) % NUM_PARTY_SLOTS));
    g_state.capture.active = 0u;
    g_state.capture.count = 0u;
    g_state.silence_ticks = 0u;
    g_state.silence_ms = 0u;
    g_state.attack_timer[s] = 15u;
    uint8_t was_paused = !g_state.playing;
    g_state.playing = 1u;
    if (was_paused) {
        /* Starting from pause: align step_16th so the entered melody starts immediately on beat 1 */
        static const uint8_t SLOT_OFF_ALIGN[3] = {0u, 2u, 4u};
        g_state.tick_in_16th = 0u;
        g_state.step_16th = (uint8_t)((16u - SLOT_OFF_ALIGN[s % 3u]) % 16u);
    }
    /* Launch exact number of flying music notes at Boss (no chime SFX) */
    uint8_t mir = g_state.spells.mirror_active;
    for (uint8_t i = 0; i < count && i < MAX_SEED_NOTES; i++) {
        int16_t y0 = (int16_t)(38 + (int)s * 44);
        g_state.note_fx[i].x = mir ? 64 : 172;
        g_state.note_fx[i].y = y0;
        g_state.note_fx[i].vx = mir ? 7 : -7;
        g_state.note_fx[i].vy = (int16_t)((72 - y0) / 15);
        g_state.note_fx[i].slot = s;
        g_state.note_fx[i].delay = (uint8_t)(i * 3u);
        g_state.note_fx[i].active = 1u;
    }
    g_state.boss_hit_timer = 12u;
    g_state.dirty_diorama = 1u;
    g_state.dirty_party = 1u;
    static const char *const ATK_MSG[3] = {
        "> FIGHTER: SLASH!", "> CLERIC: HOLY!", "> SORCERESS: FLARE!"
    };
    engine_set_ticker(ATK_MSG[s]);

    /* Arm Boss Counter-Attack 3-beat drum buildup according to each Boss's turn cooldown */
    if (g_state.slots[0].note_count > 0u && g_state.slots[1].note_count > 0u && g_state.slots[2].note_count > 0u) {
        static const uint8_t BOSS_CD[9] = { 4u, 4u, 5u, 3u, 3u, 4u, 4u, 4u, 5u };
        uint8_t sg = zone_get(g_state.settings.zone_id)->subgenre_id % 9u;
        g_state.boss_turn_cnt++;
        if (g_state.boss_turn_cnt >= BOSS_CD[sg]) {
            g_state.boss_turn_cnt = 0u;
            g_state.boss_buildup_steps = 12u;
        }
    }
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
    dsp_note_on(target_slot, midi, 32767u, 72u);

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
    if (g_state.held_keys_mask == 0u && g_state.capture.active) {
        uint8_t target_slot = g_state.next_slot;
        if (g_state.slots[target_slot].frozen) {
            target_slot = find_next_unfrozen_slot(target_slot);
        }
        dsp_voice_quick_release(target_slot);
    }
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
    g_state.playing = 1u;
    dsp_shatter_sfx();
    for (uint8_t s = 0; s < NUM_PARTY_SLOTS; s++) {
        if (!g_state.slots[s].frozen && g_state.slots[s].note_count > 0u) {
            SeedSlot tmp = g_state.slots[s];
            for (uint8_t k = 0; k < 3u; k++) {
                apply_drift_to_slot(&tmp, 80u);
            }
            uint8_t len = tmp.base_length ? tmp.base_length : 16u;
            for (uint8_t i = 1u; i < tmp.note_count; i++) {
                tmp.step_pos[i] = (uint8_t)((tmp.step_pos[i] + 3u) % len);
            }
            tmp.anim_timer = 10u;
            g_state.staging_slots[s] = tmp;
            g_state.slot_pending[s] = 1u;
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
    uint8_t was_paused = !g_state.playing;
    g_state.playing = 1u;
    if (was_paused) {
        g_state.tick_in_16th = 0u;
        g_state.step_16th = 0u;
        g_state.bar_count = 0u;
    }
    /* Immediately play first note of the entered tune and launch count flying notes at Boss */
    if (ns.note_count > 0u && ns.scale_deg[0] >= 0) {
        uint8_t m0 = engine_note_for_degree(ns.scale_deg[0], s);
        dsp_note_on(s, m0, 32767u, 12u);
    }
    for (uint8_t i = 0; i < count && i < MAX_SEED_NOTES; i++) {
        int16_t y0 = (int16_t)(38 + (int)s * 44);
        g_state.note_fx[i].x = 172;
        g_state.note_fx[i].y = y0;
        g_state.note_fx[i].vx = -7;
        g_state.note_fx[i].vy = (int16_t)((72 - y0) / 15);
        g_state.note_fx[i].slot = s;
        g_state.note_fx[i].delay = (uint8_t)(i * 3u);
        g_state.note_fx[i].active = 1u;
    }
    g_state.boss_hit_timer = 12u;
    dsp_attack_sfx(s);
    g_state.dirty_diorama = 1u;
    g_state.dirty_party = 1u;
    engine_set_ticker("> CONJURED!");
}

void engine_spell_toll(void)
{
    g_state.playing = 1u;
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
    if (sl->frozen || sl->note_count == 0u) return;
    /* Never steal/overwrite the voice of the Party member currently recording */
    if ((g_state.capture.active || g_state.held_keys_mask != 0u) && s == g_state.next_slot) return;

    /* Stagger Party slots on beat/half-beat (Fighter=0, Cleric=+2 [half-beat], Sorceress=+4 [beat 2]) */
    static const uint8_t SLOT_OFFSET[3] = {0u, 2u, 4u};
    uint8_t len = (g_state.settings.weave > 65u) ? sl->weave_length : sl->base_length;
    if (len == 0u) len = 16u;
    uint8_t pos = (uint8_t)((g_state.bar_count * 16u + step + SLOT_OFFSET[s % 3u]) % len);

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

    uint8_t boss_active = (g_state.boss_curse_bars > 0u) ? 1u : 0u;
    uint8_t zc = zone_get(g_state.settings.zone_id)->subgenre_id % 9u;
    uint8_t eff_density = g_state.settings.density;
    if (boss_active && (zc == 2u || zc == 7u || zc == 8u)) {
        eff_density = 100u; /* Overgrow / Judgment / Meteor density surge */
    }
    uint8_t eff_mirror = g_state.spells.mirror_active ^ ((boss_active && zc == 3u) ? 1u : 0u);

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
            } else if (pos == 6u && (other_occ || eff_density > 45u)) {
                deg = base_deg + 4;
            } else if (pos == 11u && eff_density > 55u) {
                deg = base_deg + 7;
                vel_pct = 78u;
            } else if (pos == 14u && eff_density > 70u) {
                deg = base_deg + 2;
                vel_pct = 72u;
            }
        }
    } else {
        for (uint8_t i = 0; i < sl->note_count; i++) {
            uint8_t target_pos = sl->step_pos[i];
            if (eff_mirror) {
                target_pos = (uint8_t)((len - 1u - (target_pos % len)) % len);
            }
            if (target_pos == pos) {
                /* Density thinning (0..35%) while anchoring step 0 */
                if (eff_density <= 35u && pos != 0u) {
                    if ((engine_prng() % 100u) > (uint32_t)(eff_density * 2u + 25u)) {
                        continue;
                    }
                }
                deg = sl->scale_deg[i];
                break;
            }
        }
        /* Density blooming (65..100%) adds gentle stepwise passing tones (or lush Overgrow cascades for Treant) */
        if (deg < 0 && eff_density >= 65u) {
            uint8_t chance = (boss_active && zc == 2u) ? 55u : (uint8_t)(eff_density - 58u);
            if ((engine_prng() % 100u) < chance) {
                static const int8_t ORN[3] = {1, -1, 2};
                static const int8_t OVERGROW[3] = {2, 4, 7};
                int8_t base = sl->scale_deg[0];
                if (base >= 0) {
                    deg = base + ((boss_active && zc == 2u) ? OVERGROW[engine_prng() % 3u] : ORN[engine_prng() % 3u]);
                    vel_pct = (boss_active && zc == 2u) ? 85u : 65u;
                }
            }
        }
    }

    if (deg < 0) return;

    /* Guided Band Accompaniment & 4-Bar RPG Evolution (A / B / A / C) */
    uint8_t form_bar = g_state.bar_count & 3u;
    int32_t c_root = (int32_t)g_state.active_chord_deg;

    if (s == 0u) {
        /* Fighter (Bass): 1-note seeds walk; multi-note phrases keep user melody on A/A, follow chord on B/C */
        if (sl->note_count == 1u) {
            if (step == 0u) {
                deg = c_root;
            } else if (g_state.settings.harmony >= 15u) {
                static const int8_t BASS_WALK[4] = {0, 4, 2, 6};
                uint8_t w_idx = (uint8_t)(((step >> 2) + form_bar) & 3u);
                deg = c_root + BASS_WALK[w_idx];
            }
        } else if (form_bar == 1u || (form_bar == 3u && step >= 12u)) {
            deg = snap_to_chord_gravity(deg, c_root);
        }
    } else if (s == 1u) {
        /* Cleric: 1-note seeds arpeggiate; multi-note phrases answer/cadence with chord on B/C */
        static const int8_t ARP_SHAPES[4][4] = {
            {0, 2, 4, 7}, /* Bar 0: Rising Prelude */
            {7, 4, 2, 0}, /* Bar 1: Falling Cascade */
            {0, 4, 2, 4}, /* Bar 2: Alberti Wave */
            {0, 2, 4, 6}  /* Bar 3: Seventh Turnaround */
        };
        if (sl->note_count == 1u && g_state.settings.harmony >= 25u) {
            uint8_t shape = (uint8_t)((form_bar + (g_state.settings.drift > 50u ? (engine_prng() & 1u) : 0u)) & 3u);
            deg = c_root + ARP_SHAPES[shape][(step >> 1) & 3u];
        } else if (form_bar == 1u) {
            deg = snap_to_chord_gravity(deg + 1, c_root);
        } else if (form_bar == 3u && step >= 12u) {
            deg = snap_to_chord_gravity(deg, c_root);
        }
    } else {
        /* Sorceress (Lead): Plays motif on Bars 0 & 2 (Call), stepwise Answer on Bar 1, Cadence on Bar 3 */
        if (form_bar == 1u) {
            deg = snap_to_chord_gravity(deg + 1, c_root);
        } else if (form_bar == 3u) {
            deg = (step >= 12u) ? (c_root + 4) : snap_to_chord_gravity(deg + 2, c_root);
        } else if (g_state.settings.harmony >= 45u) {
            deg = snap_to_chord_gravity(deg, c_root);
        }
    }

    if (eff_mirror) {
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
    if (boss_active) {
        if (zc == 0u) deg -= 7;                  /* Crypt Doom: sub-octave drop */
        else if (zc == 5u || zc == 6u) deg += 7; /* Spire/Tundra: crystal octave surge */
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

    /* Map scale degree (folding octaves into 0..26) so LEDs match the scale keyboard */
    int32_t led_k = deg;
    while (led_k < 0) led_k += 7;
    while (led_k >= 27) led_k -= 7;
    g_state.active_note_leds |= (1u << (uint32_t)led_k);
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
            if (!g_state.playing) {
                if (g_state.silence_ticks >= 28u && g_state.silence_ms >= 850u) {
                    seed_commit_capture();
                }
            } else {
                /* Wait for the next quarter-note beat so character transitions stay locked to the groove */
                if (g_state.silence_ticks >= 24u && g_state.silence_ms >= 750u &&
                    g_state.tick_in_16th == 0u && (g_state.step_16th & 3u) == 0u) {
                    seed_commit_capture();
                }
            }
        }
    }

    if (!g_state.playing) {
        if ((g_state.total_ticks % 24u) == 0u && g_state.ticker_timer > 0u) {
            g_state.ticker_timer--;
            if (g_state.ticker_timer == 0u) g_state.dirty_ticker = 1u;
        }
        return;
    }

    uint8_t t = g_state.tick_in_16th;
    uint8_t step = g_state.step_16th;

    /* Swing calculation for Weave 21..60% (or Barrel Mimic Boss Brawl) */
    uint8_t swing_ticks = 0u;
    uint8_t eff_weave = (g_state.boss_curse_bars > 0u && (zone_get(g_state.settings.zone_id)->subgenre_id % 9u) == 1u)
                      ? 58u : g_state.settings.weave;
    if (eff_weave >= 15u && eff_weave <= 65u) {
        swing_ticks = (uint8_t)(1u + (eff_weave - 15u) / 15u);
        if (swing_ticks > 3u) swing_ticks = 3u;
    }
    uint8_t eff_haste = g_state.spells.haste_active ||
                        (g_state.boss_curse_bars > 0u && ((zone_get(g_state.settings.zone_id)->subgenre_id % 9u) == 5u || (zone_get(g_state.settings.zone_id)->subgenre_id % 9u) == 8u));

    uint8_t fire_main = 0u;
    uint8_t pair_tick = (uint8_t)((step & 1u) * 6u + t);
    uint8_t fire_haste = (pair_tick == 0u || pair_tick == 4u || pair_tick == 8u); /* 1.5x tempo */
    if (swing_ticks == 0u) {
        fire_main = (t == 0u);
    } else {
        if ((step & 1u) == 0u) {
            fire_main = (t == 0u);
        } else {
            fire_main = (t == swing_ticks);
        }
    }

    if (fire_main) {
        /* Update active chord degree & retune Drone on chord changes */
        const ZoneDef *z = zone_get(g_state.settings.zone_id);
        uint8_t bar_idx = g_state.bar_count & 3u;
        uint8_t prev_chord = g_state.active_chord_deg;
        if (g_state.spells.resolve_armed) {
            g_state.active_chord_deg = (step < 8u) ? 4u : 0u;
        } else if (g_state.boss_curse_bars > 0u && ((z->subgenre_id == 4u) || (z->subgenre_id == 8u) || (z->subgenre_id == 0u))) {
            g_state.active_chord_deg = 1u; /* Dramatic bII Phrygian/Doom tension */
        } else if (g_state.boss_curse_bars > 0u && z->subgenre_id == 7u) {
            g_state.active_chord_deg = (step < 8u) ? 3u : 4u; /* Seraph IV->V fanfare */
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
        if ((step & 3u) == 0u && g_state.ticker_timer > 0u) {
            g_state.ticker_timer--;
            if (g_state.ticker_timer == 0u) g_state.dirty_ticker = 1u;
        }

        /* Slot 0 always at 1x */
        trigger_slot_step(0u, step);
        if (!eff_haste) {
            trigger_slot_step(1u, step);
            trigger_slot_step(2u, step);
        }

        /* Percussion evaluation (sensitive to Intensity, Density, and Boss Counter-Attack) */
        uint8_t inten = g_state.settings.intensity;
        uint8_t dens = g_state.settings.density;
        if (g_state.boss_curse_bars > 0u) {
            uint8_t zc = zone_get(g_state.settings.zone_id)->subgenre_id % 9u;
            if (zc == 1u || zc == 4u || zc == 7u || zc == 8u) inten = 95u; /* Battle fills */
        }
        if (inten >= 15u && g_state.settings.mix_perc > 0u) {
            uint16_t low_mask = 0u, high_mask = 0u;
            if (inten <= 45u) {
                low_mask = (dens >= 30u) ? z->low_mask_march : (z->low_mask_march & 0x0101u);
                if (dens >= 40u) high_mask = z->high_mask_march;
            } else if (inten <= 75u) {
                low_mask = z->low_mask_march;
                high_mask = z->high_mask_march;
                if (dens >= 70u) high_mask |= 0x4444u;
            } else {
                low_mask = z->low_mask_battle;
                high_mask = z->high_mask_battle;
                if ((g_state.bar_count & 3u) == 3u && step >= 12u) {
                    high_mask |= (uint16_t)(1u << step);
                }
            }
            if (dens < 25u) high_mask = 0u;
            uint16_t vel = (uint16_t)((uint32_t)g_state.settings.mix_perc * 200u);
            if (low_mask & (1u << step)) dsp_perc_trigger(0u, vel);
            if (high_mask & (1u << step)) dsp_perc_trigger(1u, vel);
            if (inten > 75u && (step & 3u) == 0u) {
                g_state.shake_timer = 2u;
                g_state.shake_amp = 1u;
            }
        }

        /* Boss Counter-Attack 3-beat drum roll buildup -> beat-timed chord sting & lunge */
        if (g_state.boss_buildup_steps > 0u) {
            if (g_state.boss_buildup_steps == 10u) {
                engine_set_ticker("> BOSS CHARGING!");
            }
            uint16_t bvel = (uint16_t)(16000u + (uint32_t)(12u - g_state.boss_buildup_steps) * 1400u);
            if (g_state.boss_buildup_steps > 8u) {
                if ((step & 1u) == 0u) dsp_perc_trigger(0u, bvel);
            } else {
                dsp_perc_trigger(1u, bvel);
                if ((step & 1u) == 0u) dsp_perc_trigger(0u, bvel);
            }
            if (g_state.boss_buildup_steps <= 4u) {
                g_state.shake_timer = 2u;
                g_state.shake_amp = 1u;
            }
            g_state.boss_buildup_steps--;
            if (g_state.boss_buildup_steps == 0u) {
                g_state.boss_curse_bars = 1u;
                g_state.boss_atk_timer = 24u;
                g_state.shake_timer = 10u;
                g_state.shake_amp = 2u;
                g_state.active_chord_deg = (z->subgenre_id == 7u) ? 4u : 1u;
                update_drone_pitch();
                dsp_boss_attack_sfx(g_state.settings.zone_id);
                dsp_perc_trigger(0u, 32767u);
                dsp_perc_trigger(1u, 32767u);
                static const char *const BOSS_ATK_MSG[9] = {
                    "> LICH: DOOM!", "> MIMIC: BRAWL!", "> TREANT: OVERGROW!",
                    "> VAMPIRE: BLOOD MOON!", "> ANUBIS: SANDSTORM!", "> BEHOLDER: WARP!",
                    "> WYRM: BLIZZARD!", "> SERAPH: JUDGMENT!", "> DEMON: METEOR!"
                };
                engine_set_ticker(BOSS_ATK_MSG[z->subgenre_id % 9u]);
            }
        }
    }

    if (g_state.boss_buildup_steps > 0u && g_state.boss_buildup_steps <= 4u && t == 3u) {
        dsp_perc_trigger(1u, 30000u); /* Rapid 32nd snare roll into Boss chord sting */
    }

    if (eff_haste && fire_haste) {
        uint8_t hstep = (uint8_t)(((step >> 1) * 3u + (pair_tick >> 2)) & 15u);
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
            if (g_state.boss_curse_bars > 0u) {
                g_state.boss_curse_bars--;
                if (g_state.boss_curse_bars == 0u) {
                    g_state.dirty_diorama = 1u;
                }
            }
        }
    }
}
