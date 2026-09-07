#!/usr/bin/env bash
# Pre-packaging audit: no developer-machine paths in Source/
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

echo "== Factory / path audit (Prototype 1) =="

FAIL=0

audit() {
  local label="$1"
  local pattern="$2"
  if rg -q "$pattern" Source/ 2>/dev/null; then
    echo "FAIL: $label"
    rg "$pattern" Source/ || true
    FAIL=1
  else
    echo "OK:   $label"
  fi
}

audit "No Mac home paths" '/Users/|/home/|Downloads/|\.cursor/'
audit "No cwd/repo Resources loads" 'getCurrentWorkingDirectory|getParentDirectory.*Resources'

# Source-level greps only prove intent. Scan the BUILT binary too: a leaked absolute
# path is what makes a plugin load on the dev machine and go silent on the client's.
echo ""
echo "== Built binary string scan =="
BIN=""
for candidate in \
  "build/Aviation_artefacts/Release/VST3/Aviation.vst3/Contents/MacOS/Aviation" \
  "build-release/Aviation_artefacts/Release/VST3/Aviation.vst3/Contents/MacOS/Aviation"; do
  if [[ -f "$candidate" ]]; then BIN="$candidate"; break; fi
done

if [[ -z "$BIN" ]]; then
  echo "SKIP: no built VST3 binary found — run ./scripts/build_macos.sh first."
else
  echo "Binary: $BIN"
  LEAKS="$(strings -a "$BIN" 2>/dev/null \
    | grep -aE '(/Users/[^S]|/Volumes/|C:\\Users\\|\.cursor/|/Downloads/)' \
    | sort -u || true)"
  if [[ -n "$LEAKS" ]]; then
    echo "FAIL: developer-machine paths embedded in the shipped binary:"
    echo "$LEAKS" | head -20
    FAIL=1
  else
    echo "OK:   no developer-machine absolute paths in the binary"
  fi
fi

echo ""
echo "== Factory preset validation =="
python3 scripts/validate_factory_presets.py

echo ""
echo "== Embedded factory architecture =="
echo "Factory WAV + preset XML + cockpit UI embed via juce_add_binary_data (CMakeLists.txt)."
echo "Runtime load: FactoryResources -> BinaryData -> SampleLibrary::loadFromMemory"

if [[ "$FAIL" -ne 0 ]]; then
  echo ""
  echo "AUDIT FAILED — fix path dependencies before packaging."
  exit 1
fi

echo ""
echo "PASS: factory path audit"
