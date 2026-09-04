# Releasing Pandoras Box

This document describes the release gates for universal macOS AU/VST3 and
64-bit Windows VST3 builds. Do not publish Debug, ad-hoc-signed, or unsigned
bundles.

## macOS prerequisites

- macOS 11 or newer with Xcode command-line tools
- CMake 3.25 or newer
- Apple Developer Program membership
- `Developer ID Application` certificate and private key
- `Developer ID Installer` certificate and private key
- A commercial JUCE license, or distribution in full compliance with AGPLv3
- Optional local validation: `brew install --cask pluginval`

Confirm both distribution identities are available:

```bash
security find-identity -v -p codesigning
security find-identity -v -p basic
```

An `Apple Development` identity is not valid for public distribution.

## Configure notarization

Store notarization credentials in the keychain once. Do not put an Apple ID
password, app-specific password, certificate, or private key in this repository.

```bash
xcrun notarytool store-credentials "pandoras-box" \
  --apple-id "YOUR_APPLE_ID" \
  --team-id "YOUR_TEAM_ID" \
  --password "YOUR_APP_SPECIFIC_PASSWORD"
```

## Build and test without signing

```bash
cmake --preset macos-release
cmake --build --preset macos-release
ctest --preset macos-release
```

The preset builds optimized `arm64` and `x86_64` AU/VST3 binaries without
installing them into the developer's plugin folders.

## Signed and notarized package

Use the full identity names printed by `security find-identity`:

```bash
export DEVELOPER_ID_APPLICATION="Developer ID Application: FernShy (TEAMID)"
export DEVELOPER_ID_INSTALLER="Developer ID Installer: FernShy (TEAMID)"
export NOTARY_PROFILE="pandoras-box"

./scripts/release_macos.sh
```

The script:

1. builds and runs the Release tests;
2. signs both universal plugin bundles with hardened runtime and timestamping;
3. creates a signed installer that installs AU and VST3 under `/Library`;
4. submits the installer to Apple notarization and staples the result;
5. verifies signatures, Gatekeeper assessment, architectures, and package
   signature;
6. emits a versioned ZIP and SHA-256 checksum under `dist/`.

## Manual release gate

Before publishing a version:

- Run `auval -v aufx Pbox Fshy`.
- Run pluginval at strictness level 10 against the VST3 bundle.
- Install on a clean Apple Silicon Mac and, if available, an Intel Mac.
- Scan and load AU and VST3 in Ableton Live.
- Scan and load AU in Logic Pro.
- Test at 44.1, 48, 96, and 192 kHz with common block sizes.
- Test mono and stereo tracks.
- Click each eye repeatedly during playback and confirm finite, limited output.
- Save, close, reopen, and confirm the randomized sound is restored.
- Compare real-time and offline exports from the same start point with pre-roll.
- Confirm delay/reverb tails are not truncated.
- Verify bypass transitions do not click.
- Verify the installer and uninstallation instructions.

## Versioning

The release version is defined once in `CMakeLists.txt`:

```cmake
project(FernShyPandorasBoxPlugin VERSION 1.0.1 LANGUAGES C CXX)
```

Never change existing parameter IDs. Increase the patch version for compatible
fixes and increase the minor/major version for user-visible or incompatible
changes. Tag published source with the same version, for example `v1.0.0`.

## GitHub Actions secrets

The tagged/manual signing workflow expects:

- `MACOS_APPLICATION_P12`
- `MACOS_APPLICATION_P12_PASSWORD`
- `MACOS_INSTALLER_P12`
- `MACOS_INSTALLER_P12_PASSWORD`
- `MACOS_KEYCHAIN_PASSWORD`
- `DEVELOPER_ID_APPLICATION`
- `DEVELOPER_ID_INSTALLER`
- `APPLE_ID`
- `APPLE_TEAM_ID`
- `APPLE_APP_SPECIFIC_PASSWORD`

The P12 values must be base64-encoded. Repository secrets are imported into an
ephemeral CI keychain and are never written to source artifacts.

## Windows prerequisites

- 64-bit Windows 10 or newer
- Visual Studio 2022 with the **Desktop development with C++** workload
- CMake 3.25 or newer
- A trusted Windows code-signing certificate exported as a password-protected
  PFX file
- `pluginval.exe` for local validation

## Windows build and test

Run these commands from the `complete` directory in PowerShell:

```powershell
cmake --preset windows-release
cmake --build --preset windows-release
ctest --preset windows-release
```

The optimized x64 VST3 is created at:

```text
cmake-windows-release-build\FernShyPandorasBoxPlugin_artefacts\Release\VST3\Pandoras Box.vst3
```

The preset does not install or overwrite plugins on the machine.

## Signed Windows package

Create the release ZIP with Authenticode signing and pluginval validation:

```powershell
.\scripts\release_windows.ps1 `
  -PluginvalPath "C:\Tools\pluginval\pluginval.exe" `
  -CertificatePath "C:\Secure\FernShy-code-signing.pfx" `
  -CertificatePassword "PFX_PASSWORD"
```

The script builds, runs tests, validates the VST3 at pluginval strictness 10,
signs and timestamps the native VST3 binary, verifies the signature, and emits
a versioned ZIP plus a SHA-256 checksum under `dist\`.

Omitting `-CertificatePath` creates a clearly named `-unsigned.zip` for
internal testing only. Do not publish that archive. Keep PFX files and
passwords outside the repository.

## Windows manual release gate

- Install the signed VST3 under `C:\Program Files\Common Files\VST3`.
- Perform a full plugin rescan in current 64-bit versions of FL Studio and
  Ableton Live.
- Repeat the sample-rate, block-size, mono/stereo, randomization, state recall,
  offline export, tail, limiter, and bypass tests from the macOS gate.
- Check the native binary's Digital Signatures tab in Windows Explorer.
- Test the ZIP on a clean Windows machine with no developer tools installed.

## Windows GitHub Actions secrets

The tagged/manual Windows signing job expects:

- `WINDOWS_CERTIFICATE_PFX`
- `WINDOWS_CERTIFICATE_PASSWORD`

`WINDOWS_CERTIFICATE_PFX` must contain the base64-encoded PFX bytes. The
workflow decodes it only into the runner's temporary directory.
