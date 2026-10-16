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
