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
- **Effects** — delay (time/feedback/mix), reverb (mix/tail size/tone)
- **Output** — volume, preset dropdown (Lead / Bass / Pad / Pluck)

## Playing it

Flip the power switch, then play with `A`–`;` on your keyboard (mapped like
a piano keyboard) or click the on-screen keys. Octave shift buttons sit next
to the keybed.

## VST3 plugin

[`vst/`](vst/) has the same engine ported to C++/JUCE as a real VST3 +
standalone plugin, for use in a DAW. It's a different codebase from the web
version above (not a wrapper around it) — same signal path and parameter
ranges, driven by MIDI instead of a browser keyboard, with JUCE's generic
parameter editor for a GUI for now.

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
