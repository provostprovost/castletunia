# Castletunia

**Generative Dungeon-Synth & 16-Bit RPG Soundtrack Firmware for the M-VAVE FM-1**

Built on [zvenson/dinghy](https://github.com/zvenson/dinghy) v0.5 (GPL-3.0-only).

Castletunia converts the M-VAVE FM-1 pocket synthesizer into a tactile, zero-menu generative dungeon-synth and 16-bit RPG soundtrack instrument. Upon power-on, it boots immediately into a living musical environment ("Living Room Tone") with 9 Fantasy Zones, silence-triggered inferred Seed planting on the 27-key scale-quantized keyboard, 7 one-knob-per-function macro encoders, 6 live Spell buttons, and a 240x240 RGB565 SNES-style living diorama & RPG party HUD.

## Architecture

- **Pure 32-Bit Fixed-Point DSP** (`firmware/src/castletunia_dsp.c`):
  - Living Room Ambient Drone (Root + detuned 5th, 0.18 Hz triangle LFO, 1-pole LPF)
  - 3-Member Party 2-Op FM Engine (Fighter Bass, White Mage Harmony, Red Sorceress Lead) with 64-sample anti-click micro-ramps and envelope-squared modulator decay
  - Dedicated 1-Voice Toll Bell/Gong FM Flourish
  - Zone-Specific 2-Channel FM Percussion Kit (Low body + High LFSR-noise transient)
  - 4-Comb + 2-Allpass Schroeder Room Reverb + 120 Hz DC/HPF + Q15 polynomial soft-clipper
- **9 Fantasy Zones** (`firmware/src/castletunia_zones.c`):
  - `0: THE CRYPT` (Aeolian, 92 BPM)
  - `1: THE TAVERN` (Mixolydian, 104 BPM)
  - `2: THE GLADE` (Dorian, 96 BPM)
  - `3: THE CASTLE` (Harmonic Minor, 108 BPM)
  - `4: THE SANDS` (Phrygian Dominant, 100 BPM)
  - `5: THE SPIRE` (Lydian, 86 BPM)
  - `6: THE TUNDRA` (Aeolian, 76 BPM)
  - `7: CORONATION` (Ionian, 116 BPM)
  - `8: BOSS` (Phrygian, 138 BPM)
- **Dual-Path Asset Pipeline & Differential SNES UI** (`tools/gen_assets.py`, `firmware/src/castletunia_ui.c`):
  - Supports authored 120x80 PNG dioramas (`assets/zones/zone_0..8.png`) or deterministic procedural 3-layer dithered pixel art packed into `firmware/src/castletunia_assets.h`.

## Building & Testing

### Host TDD Suite (No Hardware Required)

```sh
python3 tools/gen_assets.py

# Stage 1: 9-Zone Tables & Generative Seed Engine
gcc -DSTANDALONE_TEST -O2 -Wall -Wextra -Ifirmware/src firmware/src/castletunia_zones.c firmware/src/castletunia_engine.c tests/test_castletunia.c -DTEST_STAGE=1 -o build/test_s1 && ./build/test_s1

# Stage 2: Pure 32-Bit Fixed-Point DSP Engine
gcc -DSTANDALONE_TEST -O2 -Wall -Wextra -Ifirmware/src firmware/src/castletunia_zones.c firmware/src/castletunia_engine.c firmware/src/castletunia_dsp.c tests/test_castletunia.c -DTEST_STAGE=2 -o build/test_s2 -lm && ./build/test_s2

# Stage 3: Dual-Path Asset Generator & Differential SNES UI
gcc -DSTANDALONE_TEST -O2 -Wall -Wextra -Ifirmware/src firmware/src/castletunia_zones.c firmware/src/castletunia_engine.c firmware/src/castletunia_ui.c tests/test_castletunia.c -DTEST_STAGE=3 -o build/test_s3 && ./build/test_s3

# Stage 4: Panel Input, LEDs & Full-System Integration
gcc -DSTANDALONE_TEST -O2 -Wall -Wextra -Ifirmware/src firmware/src/castletunia_zones.c firmware/src/castletunia_engine.c firmware/src/castletunia_dsp.c firmware/src/castletunia_input.c firmware/src/castletunia_ui.c tests/test_castletunia.c -DTEST_STAGE=4 -o build/test_s4 && ./build/test_s4

# Dinghy Host Emulator (renders WAV + PNG screenshots)
sh tests/run_host.sh
```

### Hardware Firmware Build

Requires the JieLi pi32v2 toolchain (`./tools/get_toolchain.sh`) and `AC79NN_SDK_V1.2.1_2023-12-13`:

```sh
./build.sh
python3 tools/fm1_install.py build/dinghy.fwsc
```

Hold **OCT-** while powering on to enter USB rescue mode.

## License

GPL-3.0-only. See [LICENSE](LICENSE) and [LICENSING.md](LICENSING.md).
