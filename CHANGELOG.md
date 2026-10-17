# Changelog

Each release's notes on GitHub start with its section from this file — the
release workflow refuses to publish a version that has no section here.

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
