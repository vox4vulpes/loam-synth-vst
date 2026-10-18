# Loam Synth VST

A growing collection of synthesizers, each shipped two ways: a
self-contained HTML file you can just open in a browser, and a VST3 plugin
(built via GitHub Actions) you can load in a DAW.

## Synths

- [**Loam Eight Sub**](https://github.com/vox4vulpes/loam-eight-sub) — 8-voice
  subtractive synth with oscillator, filter, ADSR envelope, drive, delay, and reverb.
  [Play it in your browser](https://vox4vulpes.github.io/loam-eight-sub/) ·
  [Releases](https://github.com/vox4vulpes/loam-eight-sub/releases)

- [**Loam DaBoom**](https://github.com/vox4vulpes/loam-daboom) — 小鷺 · Rhythm
  Composer: an eight-voice 808/909-style drum machine with a 16-step sequencer,
  all synthesised.
  [Play it in your browser](https://vox4vulpes.github.io/loam-daboom/) ·
  [Releases](https://github.com/vox4vulpes/loam-daboom/releases)

- [**Loam Elf-cHarmor**](https://github.com/vox4vulpes/loam-elf-charmor) — 小鷺 ·
  Additive Synthesizer: up to 256 harmonics per voice with morphing timbres,
  pluck, a spectral filter and unison.
  [Play it in your browser](https://vox4vulpes.github.io/loam-elf-charmor/) ·
  [Releases](https://github.com/vox4vulpes/loam-elf-charmor/releases)

Each synth lives in its own repository with its own versions and releases;
this one gathers them as git submodules (clone with `--recurse-submodules`)
and ships them together as the bundle.

More to come.

## Download

Grab the latest bundle from [Releases](../../releases/latest):
`1Bundle-Loam-Synth-<version>-<platform>.zip` has every synth, each as a VST3
plugin and a standalone app. Single synths are in their own repositories'
releases (links above).

On Windows, `1Bundle-Loam-Synth-Setup-<version>.exe` installs every synth into
`Common Files\VST3\Loam` and upgrades an older install in place; the single
synths' installers share the same folder.

What changed in each version is in [CHANGELOG.md](CHANGELOG.md) and at the top
of every release's notes.

To cut a bundle release, move the submodules to the synth versions to ship
(`git submodule update --remote`, then commit), add a `## [x.y.z]` section to
`CHANGELOG.md` and push a version tag (`git tag v1.4.1 && git push origin v1.4.1`);
the [Release workflow](.github/workflows/release.yml) builds every synth with a
`vst/` folder on Windows, macOS, and Linux and publishes the bundle zips and installer.


## License

Everything here — the web synths, the VST plugins, the installer and build
scripts — is licensed under the [GNU AGPLv3](LICENSE). You can use, study,
change and share it freely; if you distribute a modified version (or run a
modified web synth for others online), you must publish its source under the
same license. The plugins are built on [JUCE](https://juce.com), whose free
open-source terms also require AGPLv3.

Copyright (c) 2026 vox4vulpes

## От Автора:

Я заставил Claude написать парочку синтезаторов для себя, возможно кто-то найдёт их полезными в своих проектах. 
В планах реализация самых разнообразных типов VST плагинов, от синтезаторов до эффектов.
