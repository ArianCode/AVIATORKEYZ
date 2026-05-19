#!/usr/bin/env bash
# =============================================================================
#  AviatorKeyz — macOS build script (primary platform)
#
#  Usage:
#    ./scripts/build_macos.sh              Release build
#    ./scripts/build_macos.sh debug        Debug build
#    ./scripts/build_macos.sh install      Release + copy VST3 to user folder
#    ./scripts/build_macos.sh clean        Delete build directory
#
#  Optional env:
#    BUILD_DIR=build   CMAKE build tree
#    GENERATOR=Ninja   Force generator (Ninja or Xcode)
# =============================================================================

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

BUILD_DIR="${BUILD_DIR:-build}"
CONFIG="Release"
DO_INSTALL=0

usage() {
  sed -n '4,12p' "$0" | sed 's/^# \{0,2\}//'
}

case "${1:-}" in
  -h|--help) usage; exit 0 ;;
  debug|Debug) CONFIG=Debug ;;
  clean)
    echo "Cleaning ${BUILD_DIR}..."
    rm -rf "${BUILD_DIR}"
    echo "Done."
    exit 0
    ;;
  install) DO_INSTALL=1 ;;
  "") ;;
  *)
    echo "Unknown argument: $1" >&2
    usage >&2
    exit 1
    ;;
esac

pick_generator() {
  if [[ -n "${GENERATOR:-}" ]]; then
    echo "$GENERATOR"
    return
  fi
  if command -v ninja >/dev/null 2>&1; then
    echo "Ninja"
  else
    echo "Xcode"
  fi
}

GENERATOR="$(pick_generator)"

echo "=== AviatorKeyz Build (${CONFIG}, ${GENERATOR}) ==="
echo

if [[ -f "${BUILD_DIR}/CMakeCache.txt" ]]; then
  cached_gen="$(grep '^CMAKE_GENERATOR:INTERNAL=' "${BUILD_DIR}/CMakeCache.txt" 2>/dev/null | cut -d= -f2- || true)"
  if [[ -n "$cached_gen" && "$cached_gen" != "$GENERATOR" ]]; then
    echo "Existing ${BUILD_DIR} used ${cached_gen}; removing tree for ${GENERATOR}."
    rm -rf "${BUILD_DIR}"
  fi
fi

echo "[1/3] Configuring CMake..."
if [[ "$GENERATOR" == "Ninja" ]]; then
  cmake -S . -B "${BUILD_DIR}" -G Ninja -DCMAKE_BUILD_TYPE="${CONFIG}"
else
  cmake -S . -B "${BUILD_DIR}" -G Xcode
fi

echo
echo "[2/3] Building..."
if [[ "$GENERATOR" == "Ninja" ]]; then
  cmake --build "${BUILD_DIR}" --parallel
else
  cmake --build "${BUILD_DIR}" --config "${CONFIG}" --parallel
fi

locate_artefact() {
  local name="$1"
  shift
  local candidate
  for candidate in "$@"; do
    if [[ -e "$candidate" ]]; then
      echo "$candidate"
      return 0
    fi
  done
  return 1
}

VST3="$(locate_artefact vst3 \
  "${BUILD_DIR}/AviatorKeyz_artefacts/${CONFIG}/VST3/AviatorKeyz.vst3" \
  "${BUILD_DIR}/AviatorKeyz_artefacts/VST3/AviatorKeyz.vst3" \
  "${BUILD_DIR}/AviatorKeyz_artefacts/Release/VST3/AviatorKeyz.vst3" || true)"
STANDALONE="$(locate_artefact app \
  "${BUILD_DIR}/AviatorKeyz_artefacts/${CONFIG}/Standalone/AviatorKeyz.app" \
  "${BUILD_DIR}/AviatorKeyz_artefacts/Standalone/AviatorKeyz.app" \
  "${BUILD_DIR}/AviatorKeyz_artefacts/Release/Standalone/AviatorKeyz.app" || true)"

echo
echo "[3/3] Locating artefacts..."
if [[ -n "$VST3" ]]; then
  echo
  echo "==================================================="
  echo " BUILD SUCCEEDED"
  echo " VST3:       ${VST3}"
  if [[ -d "$STANDALONE" ]]; then
    echo " Standalone: ${STANDALONE}"
  fi
  echo "==================================================="
  echo
  echo "Install VST3 for DAW testing:"
  echo "  cp -R \"${VST3}\" \"\$HOME/Library/Audio/Plug-Ins/VST3/\""
  echo
  echo "Fast dev loop (no DAW):"
  echo "  open \"${STANDALONE}\""
else
  echo "WARNING: VST3 not found at: ${VST3}" >&2
  echo "Check build output for the actual artefact path." >&2
  exit 1
fi

if [[ "$DO_INSTALL" -eq 1 ]]; then
  DEST="${HOME}/Library/Audio/Plug-Ins/VST3"
  mkdir -p "${DEST}"
  rm -rf "${DEST}/AviatorKeyz.vst3"
  cp -R "${VST3}" "${DEST}/"
  echo "Installed to ${DEST}/AviatorKeyz.vst3"
fi
