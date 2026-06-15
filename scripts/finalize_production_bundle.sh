#!/usr/bin/env bash
# Final ad-hoc (or configured) deep sign after JUCE bundle assembly, then verify.
set -euo pipefail

BUNDLE="${1:?Usage: $0 <bundle-path> <executable-path> <label> [codesign-identity]}"
EXECUTABLE="${2:?missing executable path}"
LABEL="${3:?missing label}"
IDENTITY="${4:-${AVIATORKEYZ_CODESIGN_IDENTITY:--}}"

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
VERIFY="${ROOT}/scripts/verify_plugin_binary.sh"

if [[ ! -d "$BUNDLE" ]]; then
  echo "ERROR: Bundle not found: $BUNDLE" >&2
  exit 1
fi

if [[ ! -f "$EXECUTABLE" ]]; then
  echo "ERROR: Executable not found: $EXECUTABLE" >&2
  exit 1
fi

if [[ "$IDENTITY" == "-" || -z "$IDENTITY" ]]; then
  IDENTITY="-"
fi

echo "== Finalizing production bundle: ${LABEL} =="
echo "Bundle:     ${BUNDLE}"
echo "Executable: ${EXECUTABLE}"
echo "Identity:   ${IDENTITY}"

if [[ -f "${BUNDLE}/Contents/Resources/moduleinfo.json" ]]; then
  echo "OK: moduleinfo.json present before final sign"
else
  echo "NOTE: moduleinfo.json not present (expected for AU/Standalone; VST3 should have it)"
fi

echo "-- codesign (deep) --"
codesign --force --deep --sign "${IDENTITY}" "${BUNDLE}"

echo "-- codesign verify (strict) --"
codesign --verify --deep --strict --verbose=4 "${BUNDLE}"

echo "-- codesign details --"
codesign -dv --verbose=4 "${BUNDLE}" 2>&1 | head -20

if [[ -x "$VERIFY" ]]; then
  "$VERIFY" "$EXECUTABLE" "${LABEL}"
else
  echo "WARN: ${VERIFY} not executable; skipping sanitizer verification" >&2
fi

echo "PASS: bundle finalized for ${LABEL}"
