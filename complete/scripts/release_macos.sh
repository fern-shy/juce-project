#!/bin/zsh
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/cmake-macos-release-build"
ARTEFACT_DIR="$BUILD_DIR/FernShyPandorasBoxPlugin_artefacts/Release"
DIST_DIR="$ROOT_DIR/dist"

SIGNED=false
if [[ -n "${DEVELOPER_ID_APPLICATION:-}${DEVELOPER_ID_INSTALLER:-}${NOTARY_PROFILE:-}" ]]; then
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
  SIGNED=true
else
  print -u2 "No Developer ID configured: building an unsigned, un-notarized package."
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
[[ "$SIGNED" == true ]] || PRODUCT="$PRODUCT-unsigned"
WORK_DIR="$DIST_DIR/.stage-$PRODUCT"
PACKAGE_ROOT="$WORK_DIR/package-root"
COMPONENTS_PLIST="$WORK_DIR/components.plist"
OUTPUT_DIR="$DIST_DIR/$PRODUCT"
PKG="$OUTPUT_DIR/$PRODUCT.pkg"

rm -rf "$WORK_DIR" "$OUTPUT_DIR" "$DIST_DIR/$PRODUCT.zip" "$DIST_DIR/$PRODUCT.pkg"
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
  if [[ "$SIGNED" == true ]]; then
    codesign --force --options runtime --timestamp \
      --sign "$DEVELOPER_ID_APPLICATION" "$binary"
    codesign --force --options runtime --timestamp \
      --sign "$DEVELOPER_ID_APPLICATION" "$bundle"
  else
    codesign --force --sign - "$binary"
    codesign --force --sign - "$bundle"
  fi
  codesign --verify --deep --strict --verbose=2 "$bundle"

  archs="$(lipo -archs "$binary")"
  [[ "$archs" == *arm64* && "$archs" == *x86_64* ]] || {
    print -u2 "Artifact is not universal: $bundle ($archs)"
    exit 1
  }
done

# Relocatable bundles make Installer update any other copy with the same
# bundle ID (e.g. a development build) instead of installing to /Library.
pkgbuild --analyze --root "$PACKAGE_ROOT" "$COMPONENTS_PLIST"
index=0
while /usr/libexec/PlistBuddy -c "Print :$index" "$COMPONENTS_PLIST" >/dev/null 2>&1; do
  /usr/libexec/PlistBuddy -c "Set :$index:BundleIsRelocatable false" "$COMPONENTS_PLIST"
  index=$((index + 1))
done

pkgbuild_args=(
  --root "$PACKAGE_ROOT"
  --component-plist "$COMPONENTS_PLIST"
  --install-location "/"
  --identifier "com.fernshy.pandorasbox.pkg"
  --version "$VERSION"
)
[[ "$SIGNED" == true ]] && pkgbuild_args+=(--sign "$DEVELOPER_ID_INSTALLER")
pkgbuild "${pkgbuild_args[@]}" "$PKG"

if [[ "$SIGNED" == true ]]; then
  xcrun notarytool submit "$PKG" \
    --keychain-profile "$NOTARY_PROFILE" \
    --wait
  xcrun stapler staple "$PKG"
  xcrun stapler validate "$PKG"

  pkgutil --check-signature "$PKG"
  spctl --assess --type install --verbose=2 "$PKG"
fi

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
cp "$PKG" "$DIST_DIR/$PRODUCT.pkg"
rm -rf "$WORK_DIR"

print "Release ready:"
print "  $DIST_DIR/$PRODUCT.pkg"
print "  $DIST_DIR/$PRODUCT.zip"
[[ "$SIGNED" == true ]] || print -u2 "Unsigned: macOS will ask users to approve it in System Settings > Privacy & Security."
