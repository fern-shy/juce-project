# Pandoras Box

Pandoras Box is a chaos-oriented audio effect by FernShy. Three interactive
eyes randomize hidden DSP parameters, effect routing, or both. Eight host
macros control the intensity of Time, Breath, Order, Chaos, Space, Reflection,
Fracture, and Wrath.

## Formats and requirements

- Audio Unit (AU) on macOS
- VST3 on macOS and 64-bit Windows
- macOS 11 or newer (Apple Silicon and Intel)
- Windows 10 or newer (x64)
- CMake 3.25 or newer
- Xcode command-line tools on macOS or Visual Studio 2022 with C++ on Windows

The project uses JUCE 8.0.12 and C++23.

## Development build

From the `complete` directory:

```bash
cmake --preset default
cmake --build --preset default
```

Development builds install into the current user's plugin folders:

- AU: `~/Library/Audio/Plug-Ins/Components`
- VST3: `~/Library/Audio/Plug-Ins/VST3`

## Release build and tests

```bash
cd complete
cmake --preset macos-release
cmake --build --preset macos-release
ctest --preset macos-release
```

Release artifacts are created under
`complete/cmake-macos-release-build/FernShyPandorasBoxPlugin_artefacts/Release`.
The release preset does not install or overwrite local development plugins.

For a Windows x64 VST3:

```powershell
cd complete
cmake --preset windows-release
cmake --build --preset windows-release
ctest --preset windows-release
```

The Windows artifact is created under
`complete\cmake-windows-release-build\FernShyPandorasBoxPlugin_artefacts\Release`.

See `complete/RELEASING.md` for signing, notarization, packaging, validation,
and clean-machine testing instructions.

## State and rendering

Randomized settings and routing are regenerated from seeds stored in plugin
state. Saving and reopening a session therefore restores the same hidden
parameter set. Delay, reverb, and comb-filter memory is not serialized, so a
render should begin before the desired section or include pre-roll when an
existing tail must be reproduced.

## License

Pandoras Box source code is released under the
[MIT License](LICENSE.md). Third-party components retain their own licenses;
see [`complete/THIRD_PARTY_LICENSES.md`](complete/THIRD_PARTY_LICENSES.md).

JUCE usage and distribution additionally require compliance with either the
commercial JUCE license or AGPLv3, as applicable to the distributor.
