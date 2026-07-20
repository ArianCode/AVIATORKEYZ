#!/usr/bin/env bash
# Release codesign + notarization template (requires Apple Developer credentials).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

: "${DEVELOPER_ID:?Set DEVELOPER_ID to 'Developer ID Application: Name (TEAMID)'}"
: "${APPLE_ID:?Set APPLE_ID for notarytool}"
: "${APP_PASSWORD:?Set APP_PASSWORD (app-specific password)}"
: "${TEAM_ID:?Set TEAM_ID}"

VST3="build/Aviation_artefacts/Release/VST3/Aviation.vst3"
STANDALONE="build/Aviation_artefacts/Release/Standalone/Aviation.app"

if [[ ! -d "$VST3" ]]; then
  echo "Build Release first: ./scripts/build_macos.sh"
  exit 1
fi

echo "Signing VST3..."
codesign --force --options runtime --sign "$DEVELOPER_ID" \
  "$VST3/Contents/MacOS/Aviation"

if [[ -d "$STANDALONE" ]]; then
  codesign --force --options runtime --deep --sign "$DEVELOPER_ID" "$STANDALONE"
fi

codesign --verify --deep --strict --verbose=2 "$VST3"

ZIP="$ROOT/build/Aviation-notarize.zip"
ditto -c -k --keepParent "$VST3" "$ZIP"

echo "Submitting for notarization..."
xcrun notarytool submit "$ZIP" --apple-id "$APPLE_ID" --password "$APP_PASSWORD" \
  --team-id "$TEAM_ID" --wait

xcrun stapler staple "$VST3"
echo "Done — stapled VST3 at $VST3"
