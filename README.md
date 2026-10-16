# Loam Synth VST

A growing collection of synthesizers, each shipped two ways: a
self-contained HTML file you can just open in a browser, and a VST3 plugin
(built via GitHub Actions) you can load in a DAW.

## Synths

- [**Loam Eight Sub**](loam-eight-sub/) — 8-voice subtractive synth with
  oscillator, filter, ADSR envelope, delay, and reverb.
  [Play it in your browser](https://vox4vulpes.github.io/loam-synth-vst/loam-eight-sub/) ·
  [VST3 plugin](loam-eight-sub/#vst3-plugin)

More to come.

## Download

Grab the latest build from [Releases](../../releases/latest):

`Loam-Synth-Bundle-<platform>.zip` has every synth, or pick a single
`<synth>-<platform>.zip`. Each holds the VST3 plugin and a standalone app.

On Windows, `Loam-Synth-Bundle-Setup-<version>.exe` installs every synth into
`Common Files\VST3\Loam Synth Bundle` and upgrades an older install in place.

To cut a new release, push a version tag (`git tag v1.1.0 && git push origin v1.1.0`);
the [Release workflow](.github/workflows/release.yml) builds every `*/vst/` synth
on Windows, macOS, and Linux and publishes the zips and the installer.


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
