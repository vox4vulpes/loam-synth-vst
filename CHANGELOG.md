# Changelog

Each release's notes on GitHub start with its section from this file — the
release workflow refuses to publish a version that has no section here.

## [1.4.1]

### Added
- **Loam DaBoom** (web): 小鷺 · Rhythm Composer, an eight-voice drum machine
  in the spirit of the 808/909 — kick, snare, clap, closed and open hats, tom,
  rim and cowbell, all synthesised — with a 16-step sequencer (accents, swing,
  patterns A–D), pads, computer-keyboard and MIDI play, 808 / 909 / Lo-Fi
  kits and a master section. The VST3 version follows.
- **DaBoom: your own kits.** Each voice's name above its pad picks which
  version to use (e.g. KICK 909 with SNARE Lo-Fi); tweak, name and **SAVE**
  it as your kit, load it from the KIT menu, **DELETE** it when done.
- **DaBoom: pattern library.** Load a factory groove (House, Boom Bap,
  Breakbeat, Trap, Techno, Garage, Lazy Groove, Half-Time) or one of yours
  into the current pattern; **SAVE** the current pattern under a name.
  Grooves are kept separately from kits, so any groove plays on any kit.
- Loam Eight Sub gets the same signature: 小鷺 (kosagi, the little egret)
  in its subtitle and the egret beside it — click it.

### Fixed
- Knob caps catch the light straight from the top, at 12 o'clock — the
  knob's 50% position — and on the web the dark ring around each knob now
  matches its travel, with the gap at the bottom (it was turned 90°).
- VST: the sliding DELAY plate no longer covers the left end of the Delay
  on/off switch.

## [1.4.0]

### Added
- **Pitch and mod wheels.** Two wheels beside the keys, in the VST and the web
  synth, that follow a MIDI keyboard's wheels and can be dragged with the
  mouse. The pitch wheel springs back to the centre; **+** and **−** around it
  set its range, ±1 to ±12 semitones (default ±2), shown on the wheel. The
  button above the mod wheel steps its mode: **VIB** (vibrato, up to ±60
  cents at 5.5 Hz), **FILT** (opens the filter up to 4 octaves) or **CUST**.
- **CUST mod wheel: put any knob on the wheel.** In CUST every knob gets a
  small button: click once to assign it (green), again to reverse it (red),
  a third time to free it. An assigned knob shows a red and a green dot on its
  ring — drag them to set where the wheel takes it, from red (wheel down) to
  green (wheel up). One wheel can drive any number of knobs.
- **Presets set up the wheels.** Every preset — factory and user — carries
  its mod mode, bend range and CUST assignments with their dots; the VST also
  keeps them with the project. Factory sounds default to VIB, ±2 and nothing
  assigned, and a few come with their own: Lead · Detuned Saw bends ±12,
  Bass · Acid and Pad · Dark Drone use FILT, and Pad · Classic puts Cutoff
  (1800 → 8000 Hz) and Reverb Mix (45 → 90%) on the wheel.
- Every C on the on-screen keyboard shows its octave (C3, C4, C5...).
- **Web: the keyboard always fits** — no more sideways scrolling. Click
  **OCTAVE** to switch to the full range, C1–C8, at real key widths in a
  scrolling strip; the keys the computer keyboard plays are lettered and
  tinted, and the octave buttons move them. Click OCTAVE again to go back.
- **Effects pages**, flipped with the ‹ › arrows in the Effects title:
  **TIME** (Delay, Reverb) and the new **DRIVE** page with **Saturation**
  (soft, warm) and **Distortion** (hard, asymmetric clip), each with Drive,
  Tone and Mix and its own on/off switch. They sit after the filter and
  before delay and reverb. Factory presets leave them off; user presets save
  them. The Effects module turns red on the Drive page.
- On the Drive page, click a **TONE** label to switch its filter between
  lowpass (**TONE <**, cuts the fizz) and highpass (**TONE >**, cuts the low
  end of the drive). Both are 12 dB/oct Butterworth filters, identical in the
  VST and on the web.
- **Ping-pong delay.** Hover the DELAY title and its plate slides up under the
  panel, uncovering a recess that shows the delay's type, **NORMAL** or
  **PING-PONG**; click to switch. In ping-pong the echoes bounce left, right,
  left... and a table-tennis paddle appears in the unit's corner. Saved with
  user presets; factory presets use normal.
- **Reverb TONE** switches the same way: **TONE <** reverbs only the lows and
  mids (a warm, dark tail), **TONE >** only the highs (air on top, a clean low
  end). The dry sound is untouched.

### Changed
- Delay's **MIX** knob is now on top (MIX, TIME, FEEDBK), like Reverb's.
- The VST's reverb TONE is now a filter on the reverb, the same as the web
  version, instead of the reverb's internal damping, so reverb tails sound the
  same in both; they may sound slightly different from 1.3.

## [1.3.0]

### Added
- **Sustain pedal** in the VST and the web synth. Notes released while the
  pedal is down keep sounding until it lifts; re-striking a held note
  retriggers it. The VST takes MIDI CC 64 from the DAW; `Space` works as the
  pedal on the web and in the standalone app. A SUSTAIN light by the keyboard
  shows the pedal state.
- **MIDI keyboards on the web** (Chrome, Edge): notes, sustain pedal and
  all-notes-off through Web MIDI, with the connected device shown by the
  keyboard. Access is requested when the power goes on.
- **Portamento (PORTA)**, switchable like an effect: notes inside a chosen key
  range share one voice that glides between them (legato), with an adjustable
  glide time. Notes outside the range stay polyphonic, so one hand can play
  chords while the other plays a gliding line. The range is picked on a strip
  under the keyboard while PORTA is on.

## [1.2.1]

### Changed
- The standalone app icon is crisp in Windows Explorer: icons are rendered
  from SVG at every size instead of being scaled down from one bitmap.
- Each single synth's Windows zip now includes its own installer.
- All installers share `Common Files\VST3\Loam` and `Program Files\Loam`, so
  installing the bundle and a single synth never shows a plugin twice;
  installs from 1.1–1.2 are moved there on upgrade.
- Download names: bundle files start with `1Bundle-` so they list first;
  single synths are `Loam-Synth-<Synth>-<version>-<platform>.zip`.

## [1.2.0]

### Added
- The VST now matches the web synth: the same layout, preset types (Lead,
  Bass, Pad, Pluck) with four versions each, user presets saved as files,
  on/off switches for Delay and Reverb, an oscilloscope, OCT and NOTE
  readouts, and a two-octave on-screen keyboard played from the computer
  keyboard (`Z`–`M` and `Q`–`P`, any keyboard layout).
- Standalone app icon.

### Changed
- The web synth uses the same arrangement as the VST window; its scope
  locks to a rising zero crossing so a steady note stands still.
- Release file names include the version.

## [1.1.0]

### Added
- Windows installer that puts the plugins in the VST3 folder and upgrades an
  older install in place.
- License: GNU AGPLv3.

## [1.0.0]

### Added
- First release: Loam Eight Sub, an 8-voice subtractive synth with filter,
  ADSR envelope, delay and reverb, as a VST3 plugin and standalone app for
  Windows, macOS and Linux, with a custom panel matching the web version.
