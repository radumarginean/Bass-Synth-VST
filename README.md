# BassForge

**BassForge** is a bass-focused software synthesizer, built with
[JUCE](https://juce.com) and shipped as **VST3**, **CLAP**, **AU** (macOS) and a
**Standalone** app (Linux / macOS / Windows).

Its DSP architecture is inspired by two excellent open-source synthesizers:

- **[Surge XT](https://github.com/surge-synthesizer/surge)** — its band-limited
  wavetable oscillators, unison voicing, and flexible modulation matrix.
- **[Vaporizer2](https://github.com/VASTDynamics/Vaporizer2)** — its
  dual-oscillator + sub-oscillator layout driven hard into a non-linear ladder
  filter, the recipe behind fat, aggressive bass tones.

BassForge is an **original, from-scratch implementation** — no source code is
copied from either project. It re-implements the *ideas* those synths popularised
in a compact engine tuned specifically for low end.

---

## Features

| Section | What it does |
| --- | --- |
| **Osc 1 / Osc 2** | Anti-aliased Saw / Square / Triangle / Sine (PolyBLEP) plus an 8-frame band-limited **wavetable** mode with morphable position. Per-osc octave / semi / fine, level, pulse width. Osc 2 can **hard-sync** to Osc 1. |
| **Sub Oscillator** | Dedicated sine/square sub, one or two octaves down — the foundation of the bass. |
| **Noise** | White-noise layer for transients and texture. |
| **Unison** | 1–7 voices per oscillator with detune spread and equal-power stereo width. |
| **Drive** | Pre-filter saturation: Soft (tanh), Hard clip, Wavefolder, Bitcrush. |
| **Filter** | Non-linear **Moog-style transistor ladder** (Huovilainen model, 2× oversampled) with LP24 / LP12 / HP24 / Bandpass modes, resonance, drive, key-tracking and a dedicated filter envelope. |
| **Envelopes** | Two analog-style exponential ADSRs (amp + filter). |
| **LFOs** | Two LFOs (Sine / Tri / Saw / Square / S&H), free-running or tempo-synced to the host. |
| **Mod Matrix** | 6 assignable slots: sources (LFO1/2, Filter Env, Amp Env, Velocity, Mod Wheel, Key Track) → destinations (osc pitch/level, cutoff, resonance, PW, WT position, amplitude, pan). |
| **Voicing** | 16-voice Poly, plus **Mono** and **Legato** modes with **glide/portamento** and a held-note stack. |
| **Presets** | 10 factory patches (Deep Sub, Reese Bass, Acid 303, Growl Wobble, Pluck, Hoover Stab, Hard Sync, WT Morph, Bitcrush Dirt, Init) with a browsable preset bar and **save/load** of user patches to `.bfpreset` files. |
| **Visualizer** | Built-in **oscilloscope + FFT spectrum** analyzer (click to cycle Both / Scope / Spectrum). |
| **Output** | DC blocker, master volume, soft-clip safety limiter, and an output meter in the UI. |

The full parameter set is exposed to the host for automation via
`AudioProcessorValueTreeState`, and patches are saved/restored with the session.

---

## Building

You need **CMake ≥ 3.22** and a C++17 compiler. JUCE is fetched automatically
via CMake `FetchContent` (or point `-DBASSFORGE_JUCE_PATH=/path/to/JUCE` at a
local checkout).

### Linux dependencies

```bash
sudo apt-get install -y libasound2-dev libx11-dev libxext-dev \
    libxinerama-dev libxrandr-dev libxcursor-dev \
    libfreetype6-dev libfontconfig1-dev
```

### Configure & build

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Artifacts land in:

```
build/BassForge_artefacts/Release/VST3/BassForge.vst3
build/BassForge_artefacts/Release/CLAP/BassForge.clap
build/BassForge_artefacts/Release/Standalone/BassForge
# (AU is produced only on macOS: .../AU/BassForge.component)
```

Copy the `.vst3` into your plugin folder (Linux: `~/.vst3`, macOS:
`~/Library/Audio/Plug-Ins/VST3`, Windows: `%COMMONPROGRAMFILES%\VST3`), and the
`.clap` into your CLAP folder (Linux: `~/.clap`, macOS:
`~/Library/Audio/Plug-Ins/CLAP`, Windows: `%COMMONPROGRAMFILES%\CLAP`).

**Format toggles:** CLAP is fetched from
[clap-juce-extensions](https://github.com/free-audio/clap-juce-extensions) and
built by default; pass `-DBASSFORGE_BUILD_CLAP=OFF` to skip it. **AU** is added
automatically only on Apple platforms.

### Testing / auditioning

```bash
cmake --build build --target bf_smoke_test   # renders MIDI through the engine,
./build/bf_smoke_test_artefacts/Release/bf_smoke_test   # asserts output is sane

cmake --build build --target bf_render_demo  # renders a bass riff to a WAV
./build/bf_render_demo_artefacts/Release/bf_render_demo demo.wav
```

---

## Project layout

```
source/
  PluginProcessor.*        AudioProcessor, parameter → Patch snapshot, output stage, scope tap
  PluginEditor.*           Data-driven UI (sections of auto-attached controls)
  Presets.h                Factory patches + PresetManager (apply/reset via APVTS)
  dsp/
    Wavetable.h            Band-limited, multi-frame, mip-mapped wavetable
    Oscillator.h           PolyBLEP classic shapes + wavetable reader + hard sync
    LadderFilter.h         Non-linear Moog ladder (Huovilainen, 2× oversampled)
    Envelope.h             Analog-style exponential ADSR
    LFO.h                  Sine/Tri/Saw/Square/S&H modulation source
    BassVoice.*            One voice: unison oscs, sub, noise, filters, envs, LFOs, mod matrix
    SynthEngine.h          Voice allocation, MIDI, poly/mono/legato, glide
    Patch.h                Flat per-block parameter snapshot (no atomics in the audio loop)
    Parameters.h           APVTS layout + parameter IDs + mod-matrix vocabulary
  gui/
    BassForgeLookAndFeel.h Dark, neon-accented theme + custom rotary
    ParamControl.h         Self-attaching knob/combo/toggle for any parameter
    SectionPanel.h         Titled grid of controls
    PresetBar.h            Preset browser + prev/next + save/load
    Visualizer.h           Oscilloscope + FFT spectrum analyzer
tests/  smoke_test.cpp     End-to-end engine sanity test
tools/  render_demo.cpp    Offline WAV renderer
```

---

## Licensing

The BassForge source in this repository is released under **CC0 1.0** (public
domain dedication) — see [`LICENSE`](LICENSE).

Note that a compiled plugin also links **JUCE**, which is dual-licensed
(GPLv3 or a commercial license). Distributing a binary built against JUCE must
comply with JUCE's terms. Surge XT and Vaporizer2 are GPLv3 projects; BassForge
does **not** include or derive from their code, so their licenses do not attach
to this repository.

## Credits

Architecture and sound-design inspiration from the Surge Synth Team (Surge XT)
and VAST Dynamics (Vaporizer2). Built on JUCE.
