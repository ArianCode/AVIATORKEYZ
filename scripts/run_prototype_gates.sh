#!/usr/bin/env bash
# Prototype 1 automated gates (macOS dev machine — run before freezing the RC).
#
#   ./scripts/run_prototype_gates.sh              run gates, warn on a dirty tree
#   ./scripts/run_prototype_gates.sh --strict     fail on a dirty tree (freeze check)
#
# This is the single entry point: it already runs audit_factory_paths.sh
# (which runs validate_factory_presets.py) and run_m5_smoke.sh (Release build,
# C++ unit tests, Python tests, factory validation, standalone launch).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

STRICT=0
[[ "${1:-}" == "--strict" ]] && STRICT=1

echo "== Aviation Prototype 1 RC1 — automated gates =="

# Stale worktree registrations make git plumbing fail mid-gate.
git worktree prune

DIRTY="$(git status --porcelain --untracked-files=no)"
if [[ -n "$DIRTY" ]]; then
  if [[ "$STRICT" -eq 1 ]]; then
    echo "FAIL: working tree is dirty — the RC's git SHA would not rebuild this binary."
    git status --short
    exit 1
  fi
  echo "WARN: working tree is dirty. BuildInfo will be stamped '<sha>-dirty'."
  echo "      Re-run with --strict once committed, before tagging."
fi

./scripts/audit_factory_paths.sh

echo ""
echo "== Release build + unit tests =="
./scripts/run_m5_smoke.sh

echo ""
echo "== Build identity actually compiled into the binary =="
grep -h 'kGitSha\|kPrototypeVersion\|kGitTag' build/generated/BuildInfo.h 2>/dev/null \
  || echo "WARN: build/generated/BuildInfo.h not found."
echo "(BuildInfo is captured at CMake CONFIGURE time — after committing or tagging,"
echo " delete the build tree or reconfigure, or the stamped SHA will be stale.)"

echo ""
echo "== Git identity =="
git rev-parse --short=10 HEAD
git describe --tags --always --dirty

echo ""
echo "PASS: macOS automated gates complete."
echo ""
echo "Freeze:  git tag -a prototype-1-rc1 -m 'Aviation Prototype 1 RC1'"
echo ""
echo "Then build BOTH platform artifacts from that exact tag:"
echo "  macOS   (this machine):  ./scripts/package_prototype_macos.sh"
echo "  Windows (build machine): scripts\\build_windows.bat"
echo "                           scripts\\package_prototype_windows.bat"
echo ""
echo "Then: Steinberg VST3 validator + clean-machine FL matrix (docs/PROTOTYPE1_FL_MATRIX.md)"
