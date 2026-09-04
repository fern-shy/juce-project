#!/bin/zsh
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/cmake-macos-release-build"
ARTEFACT_DIR="$BUILD_DIR/FernShyPandorasBoxPlugin_artefacts/Release"
DIST_DIR="$ROOT_DIR/dist"

: "${DEVELOPER_ID_APPLICATION:?Set DEVELOPER_ID_APPLICATION to the full Developer ID Application identity}"
: "${DEVELOPER_ID_INSTALLER:?Set DEVELOPER_ID_INSTALLER to the full Developer ID Installer identity}"
: "${NOTARY_PROFILE:?Set NOTARY_PROFILE to a notarytool keychain profile name}"

if [[ "$(security find-identity -v -p codesigning)" != *"$DEVELOPER_ID_APPLICATION"* ]]; then
  print -u2 "Developer ID Application identity not found in the keychain"
  exit 1
fi

if [[ "$(security find-identity -v -p basic)" != *"$DEVELOPER_ID_INSTALLER"* ]]; then
  print -u2 "Developer ID Installer identity not found in the keychain"
  exit 1
fi

cmake --preset macos-release -S "$ROOT_DIR"
cmake --build --preset macos-release
ctest --preset macos-release

AU="$ARTEFACT_DIR/AU/Pandoras Box.component"
VST3="$ARTEFACT_DIR/VST3/Pandoras Box.vst3"

[[ -d "$AU" ]] || { print -u2 "Missing AU artifact: $AU"; exit 1; }
[[ -d "$VST3" ]] || { print -u2 "Missing VST3 artifact: $VST3"; exit 1; }

VERSION="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' \
  "$AU/Contents/Info.plist")"
PRODUCT="PandorasBox-${VERSION}-macOS-universal"
WORK_DIR="$DIST_DIR/.stage-$PRODUCT"
PACKAGE_ROOT="$WORK_DIR/package-root"
OUTPUT_DIR="$DIST_DIR/$PRODUCT"
PKG="$OUTPUT_DIR/$PRODUCT.pkg"

rm -rf "$WORK_DIR" "$OUTPUT_DIR"
mkdir -p \
  "$PACKAGE_ROOT/Library/Audio/Plug-Ins/Components" \
  "$PACKAGE_ROOT/Library/Audio/Plug-Ins/VST3" \
  "$OUTPUT_DIR"

ditto "$AU" "$PACKAGE_ROOT/Library/Audio/Plug-Ins/Components/Pandoras Box.component"
ditto "$VST3" "$PACKAGE_ROOT/Library/Audio/Plug-Ins/VST3/Pandoras Box.vst3"

SIGNED_AU="$PACKAGE_ROOT/Library/Audio/Plug-Ins/Components/Pandoras Box.component"
SIGNED_VST3="$PACKAGE_ROOT/Library/Audio/Plug-Ins/VST3/Pandoras Box.vst3"

for bundle in "$SIGNED_AU" "$SIGNED_VST3"; do
  binary="$bundle/Contents/MacOS/Pandoras Box"
  codesign --force --options runtime --timestamp \
    --sign "$DEVELOPER_ID_APPLICATION" "$binary"
  codesign --force --options runtime --timestamp \
    --sign "$DEVELOPER_ID_APPLICATION" "$bundle"
  codesign --verify --deep --strict --verbose=2 "$bundle"

  archs="$(lipo -archs "$binary")"
  [[ "$archs" == *arm64* && "$archs" == *x86_64* ]] || {
    print -u2 "Artifact is not universal: $bundle ($archs)"
    exit 1
  }
done

pkgbuild \
  --root "$PACKAGE_ROOT" \
  --install-location "/" \
  --identifier "com.fernshy.pandorasbox.pkg" \
  --version "$VERSION" \
  --sign "$DEVELOPER_ID_INSTALLER" \
  "$PKG"

xcrun notarytool submit "$PKG" \
  --keychain-profile "$NOTARY_PROFILE" \
  --wait
xcrun stapler staple "$PKG"
xcrun stapler validate "$PKG"

pkgutil --check-signature "$PKG"
spctl --assess --type install --verbose=2 "$PKG"

cp "$ROOT_DIR/../LICENSE.md" "$OUTPUT_DIR/LICENSE.md"
cp "$ROOT_DIR/THIRD_PARTY_LICENSES.md" "$OUTPUT_DIR/THIRD_PARTY_LICENSES.md"
cp "$ROOT_DIR/INTER_FONT_LICENSE.md" "$OUTPUT_DIR/INTER_FONT_LICENSE.md"
cp "$ROOT_DIR/assets/README.txt" "$OUTPUT_DIR/README.txt"

(
  cd "$OUTPUT_DIR"
  shasum -a 256 "$(basename "$PKG")" > SHA256SUMS.txt
)

ditto -c -k --sequesterRsrc --keepParent \
  "$OUTPUT_DIR" "$DIST_DIR/$PRODUCT.zip"

print "Release ready:"
print "  $PKG"
print "  $DIST_DIR/$PRODUCT.zip"
