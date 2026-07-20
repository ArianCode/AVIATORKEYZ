#!/usr/bin/env bash
# Install a verified VST3 bundle to the user plugin folder.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SOURCE_BUNDLE="${1:?Usage: $0 <path/to/Aviation.vst3>}"
DEST="${HOME}/Library/Audio/Plug-Ins/VST3/Aviation.vst3"
FINALIZE="${ROOT}/scripts/finalize_production_bundle.sh"
VERIFY="${ROOT}/scripts/verify_plugin_binary.sh"

if [[ ! -d "$SOURCE_BUNDLE" ]]; then
  echo "ERROR: Source bundle not found: $SOURCE_BUNDLE" >&2
  exit 1
fi

SOURCE_EXE="${SOURCE_BUNDLE}/Contents/MacOS/Aviation"
if [[ ! -f "$SOURCE_EXE" ]]; then
  echo "ERROR: Source executable not found: $SOURCE_EXE" >&2
  exit 1
fi

echo "== Installing Aviation VST3 =="
echo "Source: ${SOURCE_BUNDLE}"

# 1. Source bundle must already be finalized (sign + sanitizer check).
if [[ -x "$FINALIZE" ]]; then
  "$FINALIZE" "$SOURCE_BUNDLE" "$SOURCE_EXE" "pre-install source VST3" "${AVIATORKEYZ_CODESIGN_IDENTITY:--}"
fi

echo "-- source executable SHA-256 --"
shasum -a 256 "$SOURCE_EXE"

echo "-- installing --"
mkdir -p "$(dirname "$DEST")"
rm -rf "$DEST"
cp -R "$SOURCE_BUNDLE" "$DEST"

INSTALLED_EXE="${DEST}/Contents/MacOS/Aviation"

echo "-- installed executable SHA-256 --"
shasum -a 256 "$INSTALLED_EXE"

echo "-- SHA-256 comparison --"
if cmp -s "$SOURCE_EXE" "$INSTALLED_EXE"; then
  echo "OK: installed executable byte-identical to source (signatures preserved)"
else
  echo "WARN: installed executable differs from source; verifying installed copy independently"
fi

echo "-- installed bundle mtime --"
stat -f "%Sm %N" "$INSTALLED_EXE"

echo "-- post-install bundle verification --"
if [[ -x "$FINALIZE" ]]; then
  "$FINALIZE" "$DEST" "$INSTALLED_EXE" "installed VST3" "${AVIATORKEYZ_CODESIGN_IDENTITY:--}"
elif [[ -x "$VERIFY" ]]; then
  "$VERIFY" "$INSTALLED_EXE" "installed VST3"
fi

echo "Installed to ${DEST}"
