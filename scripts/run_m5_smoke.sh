#!/usr/bin/env bash
# M5 automated smoke checks (host certification prep).
# Manual DAW steps: see docs/M5_CERTIFICATION.md and TESTPLAN.md T-M5-*.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

echo "== AviatorKeyz M5 smoke (automated) =="

./scripts/build_macos.sh

echo ""
echo "== C++ unit tests =="
cmake --build build --target AviatorKeyzTests
TEST_BIN="build/tests/AviatorKeyzTests_artefacts/Release/AviatorKeyzTests"
if [[ ! -x "$TEST_BIN" ]]; then
  TEST_BIN="build/tests/AviatorKeyzTests_artefacts/Debug/AviatorKeyzTests"
fi
"$TEST_BIN"

echo ""
echo "== Python tests =="
python3 -m unittest discover -s tests -p 'test_*.py' -v

echo ""
echo "== Factory content =="
python3 scripts/validate_factory_presets.py

STANDALONE="build/Aviation_artefacts/Release/Standalone/Aviation.app"
if [[ ! -d "$STANDALONE" ]]; then
  STANDALONE="build/Aviation_artefacts/Debug/Standalone/Aviation.app"
fi

if [[ -d "$STANDALONE" ]]; then
  echo ""
  echo "== Standalone launch (5s) =="
  open -a "$STANDALONE" || open "$STANDALONE"
  sleep 5
  if pgrep -f "Aviation" >/dev/null 2>&1; then
    echo "OK — Standalone process running"
    pkill -f "Aviation.app" 2>/dev/null || true
  else
    echo "WARN — Standalone may have exited; check Console.app"
  fi
fi

echo ""
echo "Automated M5 smoke complete."
echo "Manual: multi-instance DAW test, CPU profile, 48h soak, codesign + notarize (docs/CODE_SIGNING.md)."
