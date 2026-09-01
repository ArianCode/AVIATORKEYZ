#!/usr/bin/env bash
# =============================================================================
#  Aviation — Prototype 1 RC1 macOS packaging (.pkg installer)
#
#  Produces, from a clean universal Release build of the current commit:
#    dist/Aviation_Prototype1_RC1_macOS.pkg
#    dist/Aviation_Prototype1_RC1_macOS/VERSION.txt
#    dist/SHA256SUMS.txt
#
#  Installs:
#    Aviation.vst3      -> /Library/Audio/Plug-Ins/VST3
#    Aviation.component -> /Library/Audio/Plug-Ins/Components   (only with --with-au)
#
#  Usage:
#    ./scripts/package_prototype_macos.sh                 unsigned local pkg, VST3 only
#    ./scripts/package_prototype_macos.sh --skip-build    reuse build-release/
#    ./scripts/package_prototype_macos.sh --with-au       also install the AU component
#
#  AU is OFF by default: KNOWN_ISSUES.txt scopes Prototype 1 to VST3, and shipping an
#  unvalidated second format widens the test matrix for no prototype benefit.
#
#  Signing / notarization — all credentials come from the environment, never
#  from the repository. Any subset may be omitted; the script reports what was
#  skipped and marks it as an outstanding distribution gate.
#
#    DEVELOPER_ID_APP="Developer ID Application: Name (TEAMID)"
#    DEVELOPER_ID_INSTALLER="Developer ID Installer: Name (TEAMID)"
#    NOTARY_PROFILE=aviation           # xcrun notarytool store-credentials profile
#      ...or...
#    APPLE_ID=... APP_PASSWORD=... TEAM_ID=...
# =============================================================================
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

BUILD_DIR="${BUILD_DIR:-build-release}"
DIST="$ROOT/dist"
PKG_NAME="Aviation_Prototype1_RC1_macOS.pkg"
OUT_DIR="$DIST/Aviation_Prototype1_RC1_macOS"
PKG_IDENTIFIER="com.aviation.aviation-prototype1"
PROTOTYPE_VERSION="0.1.0-rc1"
SKIP_BUILD=0
WITH_AU=0
GATES=()

for arg in "$@"; do
  case "$arg" in
    --skip-build) SKIP_BUILD=1 ;;
    --with-au) WITH_AU=1 ;;
    -h|--help) sed -n '2,30p' "$0" | sed 's/^# \{0,2\}//'; exit 0 ;;
    *) echo "Unknown argument: $arg" >&2; exit 1 ;;
  esac
done

note_gate() { GATES+=("$1"); echo "GATE OPEN: $1"; }

# --- Identity ---------------------------------------------------------------
GIT_SHA="$(git rev-parse --short=10 HEAD 2>/dev/null || echo unknown)"
GIT_TAG="$(git describe --tags --always --dirty 2>/dev/null || echo untagged)"
if [[ -n "$(git status --porcelain --untracked-files=no 2>/dev/null)" ]]; then
  echo "ERROR: working tree is dirty. Commit before cutting an RC —"
  echo "       the recorded SHA must rebuild the exact binary the client runs."
  exit 1
fi

echo "=== Aviation Prototype 1 RC1 — macOS packaging ==="
echo "Commit: $GIT_SHA   Describe: $GIT_TAG"

# --- Build ------------------------------------------------------------------
if [[ "$SKIP_BUILD" -eq 0 ]]; then
  echo ""
  echo "[1/6] Clean universal Release build (arm64 + x86_64)..."
  rm -rf "$BUILD_DIR"
  cmake -S . -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DAVIATORKEYZ_MAC_UNIVERSAL=ON \
    -DAVIATORKEYZ_REQUIRE_CLEAN_TREE=ON \
    -DAVIATORKEYZ_BUILD_PLUGIN=ON \
    -DAVIATORKEYZ_BUILD_TESTS=OFF \
    -DAVIATORKEYZ_COPY_AFTER_BUILD=OFF
  cmake --build "$BUILD_DIR" --parallel
fi

VST3="$BUILD_DIR/Aviation_artefacts/Release/VST3/Aviation.vst3"
AU="$BUILD_DIR/Aviation_artefacts/Release/AU/Aviation.component"
[[ -d "$VST3" ]] || { echo "ERROR: $VST3 not found."; exit 1; }

echo ""
echo "[2/6] Architecture check..."
lipo -info "$VST3/Contents/MacOS/Aviation"
if ! lipo -info "$VST3/Contents/MacOS/Aviation" | grep -q "x86_64"; then
  note_gate "VST3 is not universal — Intel Macs cannot load it"
fi

# --- Sign the plugin bundles ------------------------------------------------
echo ""
echo "[3/6] Signing plugin bundles..."
sign_bundle() {
  local bundle="$1"
  # Sign the BUNDLE, not the inner Mach-O: notarization and Gatekeeper evaluate
  # the bundle's signature, and hardened runtime must be enabled at this level.
  codesign --force --timestamp --options runtime --sign "$DEVELOPER_ID_APP" "$bundle"
  codesign --verify --strict --verbose=2 "$bundle"
}

if [[ -n "${DEVELOPER_ID_APP:-}" ]]; then
  sign_bundle "$VST3"
  [[ "$WITH_AU" -eq 1 && -d "$AU" ]] && sign_bundle "$AU"
  echo "Signed with: $DEVELOPER_ID_APP"
else
  note_gate "VST3 not signed (DEVELOPER_ID_APP unset) — client will hit Gatekeeper"
fi

# --- Stage + build the pkg --------------------------------------------------
echo ""
echo "[4/6] Building installer package..."
rm -rf "$DIST"
STAGE="$DIST/stage"
mkdir -p "$STAGE/Library/Audio/Plug-Ins/VST3" "$OUT_DIR"
ditto "$VST3" "$STAGE/Library/Audio/Plug-Ins/VST3/Aviation.vst3"
if [[ "$WITH_AU" -eq 1 && -d "$AU" ]]; then
  mkdir -p "$STAGE/Library/Audio/Plug-Ins/Components"
  ditto "$AU" "$STAGE/Library/Audio/Plug-Ins/Components/Aviation.component"
  echo "Including AU component."
fi

pkgbuild --root "$STAGE" \
         --install-location "/" \
         --identifier "$PKG_IDENTIFIER" \
         --version "$PROTOTYPE_VERSION" \
         "$DIST/AviationComponent.pkg"

cat > "$DIST/distribution.xml" <<XML
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="1">
    <title>Aviation — Prototype 1 RC1</title>
    <options customize="never" require-scripts="false" hostArchitectures="arm64,x86_64"/>
    <volume-check>
        <allowed-os-versions><os-version min="10.15"/></allowed-os-versions>
    </volume-check>
    <choices-outline><line choice="default"/></choices-outline>
    <choice id="default"><pkg-ref id="$PKG_IDENTIFIER"/></choice>
    <pkg-ref id="$PKG_IDENTIFIER" version="$PROTOTYPE_VERSION" onConclusion="none">AviationComponent.pkg</pkg-ref>
</installer-gui-script>
XML

PRODUCTBUILD_ARGS=(--distribution "$DIST/distribution.xml" --package-path "$DIST")
if [[ -n "${DEVELOPER_ID_INSTALLER:-}" ]]; then
  PRODUCTBUILD_ARGS+=(--sign "$DEVELOPER_ID_INSTALLER" --timestamp)
else
  note_gate "PKG not signed (DEVELOPER_ID_INSTALLER unset)"
fi
productbuild "${PRODUCTBUILD_ARGS[@]}" "$DIST/$PKG_NAME"
rm -f "$DIST/AviationComponent.pkg"
rm -rf "$STAGE"

# --- Notarize ---------------------------------------------------------------
echo ""
echo "[5/6] Notarization..."
NOTARY_ARGS=()
if [[ -n "${NOTARY_PROFILE:-}" ]]; then
  NOTARY_ARGS=(--keychain-profile "$NOTARY_PROFILE")
elif [[ -n "${APPLE_ID:-}" && -n "${APP_PASSWORD:-}" && -n "${TEAM_ID:-}" ]]; then
  NOTARY_ARGS=(--apple-id "$APPLE_ID" --password "$APP_PASSWORD" --team-id "$TEAM_ID")
fi

if [[ ${#NOTARY_ARGS[@]} -gt 0 && -n "${DEVELOPER_ID_INSTALLER:-}" ]]; then
  xcrun notarytool submit "$DIST/$PKG_NAME" "${NOTARY_ARGS[@]}" --wait
  xcrun stapler staple "$DIST/$PKG_NAME"
  xcrun stapler validate "$DIST/$PKG_NAME"
  spctl --assess --type install -vv "$DIST/$PKG_NAME" || true
  pkgutil --check-signature "$DIST/$PKG_NAME"
else
  note_gate "PKG not notarized — client must right-click > Open, or clear quarantine"
fi

# --- VERSION.txt + checksum -------------------------------------------------
echo ""
echo "[6/6] VERSION.txt + SHA-256..."
mkdir -p "$OUT_DIR"
mkdir -p "$OUT_DIR/DOCS" "$OUT_DIR/TEST"
# macOS-specific front matter; the Windows README/INSTALL are not shipped here.
cp -f release/prototype1/README_FIRST_MACOS.txt "$OUT_DIR/README_FIRST.txt"
cp -f release/prototype1/DOCS/INSTALL_MACOS.txt "$OUT_DIR/DOCS/"
cp -f release/prototype1/DOCS/KNOWN_ISSUES.txt "$OUT_DIR/DOCS/"
cp -f release/prototype1/DOCS/FEEDBACK_TEMPLATE.txt "$OUT_DIR/DOCS/"
cp -f release/prototype1/TEST/* "$OUT_DIR/TEST/" 2>/dev/null || true

{
  echo "AVIATION — PROTOTYPE 1 RC1 (macOS)"
  echo ""
  echo "Prototype version:     $PROTOTYPE_VERSION"
  echo "Git commit:            $GIT_SHA"
  echo "Git describe:          $GIT_TAG"
  echo "Built:                 $(date -u '+%Y-%m-%d %H:%M UTC')"
  echo "Platform:              macOS"
  echo "Architectures:         $(lipo -archs "$VST3/Contents/MacOS/Aviation" 2>/dev/null || echo unknown)"
  echo "Minimum OS:            macOS 10.15"
  echo "Formats:               VST3$([[ "$WITH_AU" -eq 1 && -d "$AU" ]] && echo ' + AU')"
  echo "Install location:      /Library/Audio/Plug-Ins/VST3"
  echo "Manufacturer / code:   Avkz / Avk1"
  echo "JUCE:                  8.0.9"
  echo "Configuration:         Release"
  echo "Signed (app):          ${DEVELOPER_ID_APP:-NO}"
  echo "Signed (installer):    ${DEVELOPER_ID_INSTALLER:-NO}"
  echo "Notarized:             $(xcrun stapler validate "$DIST/$PKG_NAME" >/dev/null 2>&1 && echo YES || echo NO)"
  echo "Status:                CONFIDENTIAL PROTOTYPE — NOT FOR REDISTRIBUTION"
} > "$OUT_DIR/VERSION.txt"

cat "$OUT_DIR/VERSION.txt"

( cd "$DIST" && shasum -a 256 "$PKG_NAME" > SHA256SUMS.txt )
cp -f "$DIST/SHA256SUMS.txt" "$OUT_DIR/SHA256SUMS.txt"

echo ""
echo "==================================================="
echo " PACKAGE COMPLETE"
echo " PKG:  $DIST/$PKG_NAME"
echo " Docs: $OUT_DIR"
echo " SHA:  $(cat "$DIST/SHA256SUMS.txt")"
echo "==================================================="

if [[ ${#GATES[@]} -gt 0 ]]; then
  echo ""
  echo "OUTSTANDING DISTRIBUTION GATES (${#GATES[@]}):"
  for g in "${GATES[@]}"; do echo "  - $g"; done
  echo ""
  echo "The .pkg is installable locally but is NOT yet frictionless for an external client."
fi

echo ""
echo "Do not modify the .pkg after the SHA-256 above is recorded. Any code change becomes RC2."
