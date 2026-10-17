# Loam Eight Sub

An 8-voice subtractive synthesizer that runs entirely in the browser via the
Web Audio API — no plugins, no install.

Open `index.html` in any modern browser to play it.

## Signal path

Oscillator (sine/saw/square/triangle + sub-oscillator) → shared lowpass
filter (cutoff/resonance) → per-voice ADSR envelope → delay and reverb sends
→ output.

## Controls

- **Oscillator** — waveform, fine tune, sub-oscillator mix
- **Filter** — cutoff, resonance
- **Envelope** — attack, decay, sustain, release
- **Effects** — delay (time/feedback/mix) and reverb (mix/tail size/tone),
  each a vertical unit with its own on/off switch
- **Presets** — pick a type (Lead / Bass / Pad / Pluck), then one of its
  versions; save your own sounds under **User** (kept in this browser)
- **Output** — volume

## Playing it

Flip the power switch, then play two octaves from your computer keyboard,
tracker-style: `Z`–`M` is the lower octave (sharps on `S D G H J`), `Q`–`P`
the upper one (sharps on `2 3 5 6 7 9 0`). Keys are matched by position, so
any keyboard layout works. Or click the on-screen keys; the octave buttons
shift both rows. Hold `Space` as a sustain pedal.

A MIDI keyboard works too in Chrome and Edge (Web MIDI): turn the power on,
allow MIDI access, and play — notes and the sustain pedal (CC 64) both
respond. The VST takes the same MIDI from your DAW, and its standalone app
also uses `Space` as the pedal.

**PORTA** (by the octave buttons) turns on portamento: keys inside a range
share one voice that glides between them — play a new key while holding
another and the pitch slides without a new attack; let go and it slides back
to the key still held. Keys outside the range stay polyphonic, so one hand
can hold chords while the other plays a gliding line. **GLIDE** sets the
slide time (5 ms – 2 s), and the strip under the keys sets the range: drag
either end, or click a key to move the nearer end. PORTA is a playing
setting, so presets don't change it.

## VST3 plugin

[`vst/`](vst/) has the same engine ported to C++/JUCE as a real VST3 +
standalone plugin, for use in a DAW. It's a different codebase from the web
version above (not a wrapper around it) — same signal path and parameter
ranges, driven by MIDI. Its window uses the same layout as the web panel: the
scope, the Presets module (types, versions and user presets, saved as files
under your user app-data folder in `Loam/Loam Eight Sub/Presets`), the
Oscillator / Filter / Envelope / Output modules, switchable Delay and Reverb
units, and an on-screen keyboard with the same two-row computer-key layout
(in the standalone app; most DAWs keep computer keys for themselves).

Like the rest of the repo, both the web version and the plugin are licensed
under the [GNU AGPLv3](../LICENSE).

Prebuilt binaries for Windows, macOS, and Linux are attached to each
[GitHub Actions run](../../actions/workflows/build-vst.yml) and to
[releases](../../releases). To build it yourself:

```sh
cmake -S vst -B vst/build -DCMAKE_BUILD_TYPE=Release
cmake --build vst/build --target LoamEightSub_VST3
```

Needs a C++17 compiler and, on Linux, a handful of dev packages — see
[`.github/workflows/build-vst.yml`](../.github/workflows/build-vst.yml) for
the exact list. `vst/Tests/EngineTest.cpp` is a small assert-based sanity
check for the DSP core, with no GUI dependencies:

```sh
cmake -S vst -B vst/build -DLOAM_BUILD_PLUGIN=OFF
cmake --build vst/build --target EngineTest && ./vst/build/EngineTest
```
