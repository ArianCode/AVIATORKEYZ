#!/usr/bin/env bash
# Pre-release checks: factory content, optional build artifact, sanitizer-free plugin binary.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

echo "== AviatorKeyz release verification =="

python3 scripts/validate_factory_presets.py

find_vst3() {
  local candidates=(
    "$ROOT/build/Aviation_artefacts/VST3/Aviation.vst3"
    "$ROOT/build/Aviation_artefacts/Release/VST3/Aviation.vst3"
    "$ROOT/build/Aviation_artefacts/RelWithDebInfo/VST3/Aviation.vst3"
    "$ROOT/build/Aviation_artefacts/Debug/VST3/Aviation.vst3"
  )
  for path in "${candidates[@]}"; do
    if [[ -d "$path" ]]; then
      echo "$path"
      return 0
    fi
  done
  return 1
}

if VST3="$(find_vst3)"; then
  echo "VST3 bundle: $VST3"
  if [[ -x "$ROOT/scripts/finalize_production_bundle.sh" ]]; then
    "$ROOT/scripts/finalize_production_bundle.sh" \
      "$VST3" \
      "$VST3/Contents/MacOS/Aviation" \
      "release candidate VST3" \
      "${AVIATORKEYZ_CODESIGN_IDENTITY:--}"
  fi
else
  echo "WARN: VST3 not built yet. Run: ./scripts/build_macos.sh"
fi

echo ""
echo "Manual (M5): macOS DAW scan, project save/reopen, 48h soak, codesign + notarization."
echo "Secondary: Windows build via scripts/build_windows.bat — see docs/CODE_SIGNING.md and TESTPLAN.md"
