#!/usr/bin/env bash
set -euo pipefail

# =============================================================================
#  AviatorKeyz — Run Standalone (macOS)
#  Usage:
#    ./scripts/run_standalone_macos.sh
# =============================================================================

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
APP_PATH="${ROOT_DIR}/build/AviatorKeyz_artefacts/Release/Standalone/AviatorKeyz.app"

if [[ ! -d "${APP_PATH}" ]]; then
  echo "Standalone app not found at:"
  echo "  ${APP_PATH}"
  echo
  echo "Build it first:"
  echo "  ./scripts/build_macos.sh"
  exit 1
fi

echo "Launching standalone:"
echo "  ${APP_PATH}"
open "${APP_PATH}"
