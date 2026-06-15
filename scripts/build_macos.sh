#!/usr/bin/env bash
# =============================================================================
#  AviatorKeyz — macOS build script (primary platform)
#
#  Usage:
#    ./scripts/build_macos.sh              Release build
#    ./scripts/build_macos.sh debug        Debug build (host-loadable, no sanitizers)
#    ./scripts/build_macos.sh relwithdebinfo
#    ./scripts/build_macos.sh install      Release + verify + install VST3
#    ./scripts/build_macos.sh sanitizer-tests
#                                          tests-only ASan/UBSan tree in build-asan/
#    ./scripts/build_macos.sh clean        Delete default build directory
#
#  Optional env:
#    BUILD_DIR=build                  CMake build tree
#    GENERATOR=Ninja                  Force generator (Ninja or Xcode)
#    AVIATORKEYZ_CODESIGN_IDENTITY=   Developer ID for release; default ad-hoc (-)
# =============================================================================

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

BUILD_DIR="${BUILD_DIR:-build}"
CONFIG="Release"
DO_INSTALL=0
ENABLE_SANITIZERS=OFF
BUILD_PLUGIN=ON
BUILD_TESTS=ON
MODE="plugin"

usage() {
  sed -n '4,18p' "$0" | sed 's/^# \{0,2\}//'
}

case "${1:-}" in
  -h|--help) usage; exit 0 ;;
  debug|Debug)
    CONFIG=Debug
    ;;
  relwithdebinfo|RelWithDebInfo)
    CONFIG=RelWithDebInfo
    ;;
  clean)
    echo "Cleaning ${BUILD_DIR}..."
    rm -rf "${BUILD_DIR}"
    echo "Done."
    exit 0
    ;;
  install)
    DO_INSTALL=1
    ;;
  sanitizer-tests)
    MODE="sanitizer-tests"
    BUILD_DIR="build-asan"
    CONFIG=Debug
    ENABLE_SANITIZERS=ON
    BUILD_PLUGIN=OFF
    BUILD_TESTS=ON
    echo "Removing stale ${BUILD_DIR} tree (tests-only sanitizer builds must not retain old plugin bundles)."
    rm -rf "${ROOT}/${BUILD_DIR}"
    ;;
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

echo "=== AviatorKeyz Build (${CONFIG}, ${GENERATOR}, sanitizers=${ENABLE_SANITIZERS}, plugin=${BUILD_PLUGIN}) ==="
echo

if [[ -f "${BUILD_DIR}/CMakeCache.txt" ]]; then
  cached_gen="$(grep '^CMAKE_GENERATOR:INTERNAL=' "${BUILD_DIR}/CMakeCache.txt" 2>/dev/null | cut -d= -f2- || true)"
  cached_san="$(grep '^AVIATORKEYZ_ENABLE_SANITIZERS:BOOL=' "${BUILD_DIR}/CMakeCache.txt" 2>/dev/null | cut -d= -f2- || true)"
  cached_plugin="$(grep '^AVIATORKEYZ_BUILD_PLUGIN:BOOL=' "${BUILD_DIR}/CMakeCache.txt" 2>/dev/null | cut -d= -f2- || true)"
  if [[ -n "$cached_gen" && "$cached_gen" != "$GENERATOR" ]]; then
    echo "Existing ${BUILD_DIR} used ${cached_gen}; removing tree for ${GENERATOR}."
    rm -rf "${BUILD_DIR}"
  elif [[ -n "$cached_san" && "$cached_san" != "$ENABLE_SANITIZERS" ]]; then
    echo "Existing ${BUILD_DIR} had AVIATORKEYZ_ENABLE_SANITIZERS=${cached_san}; removing tree."
    rm -rf "${BUILD_DIR}"
  elif [[ -n "$cached_plugin" && "$cached_plugin" != "$BUILD_PLUGIN" ]]; then
    echo "Existing ${BUILD_DIR} had AVIATORKEYZ_BUILD_PLUGIN=${cached_plugin}; removing tree."
    rm -rf "${BUILD_DIR}"
  fi
fi

echo "[1/3] Configuring CMake..."
CMAKE_ARGS=(
  -S .
  -B "${BUILD_DIR}"
  -DAVIATORKEYZ_ENABLE_SANITIZERS="${ENABLE_SANITIZERS}"
  -DAVIATORKEYZ_BUILD_PLUGIN="${BUILD_PLUGIN}"
  -DAVIATORKEYZ_BUILD_TESTS="${BUILD_TESTS}"
  -DAVIATORKEYZ_COPY_AFTER_BUILD=OFF
)
if [[ "$GENERATOR" == "Ninja" ]]; then
  CMAKE_ARGS+=(-G Ninja -DCMAKE_BUILD_TYPE="${CONFIG}")
else
  CMAKE_ARGS+=(-G Xcode)
fi
cmake "${CMAKE_ARGS[@]}"

echo
echo "[2/3] Building..."
if [[ "$MODE" == "sanitizer-tests" ]]; then
  if [[ "$GENERATOR" == "Ninja" ]]; then
    cmake --build "${BUILD_DIR}" --target AviatorKeyzSanitizerTests --parallel
  else
    cmake --build "${BUILD_DIR}" --config "${CONFIG}" --target AviatorKeyzSanitizerTests --parallel
  fi
  TEST_BIN="$(find "${BUILD_DIR}/tests" -name 'AviatorKeyzSanitizerTests' -type f 2>/dev/null | head -1 || true)"
  echo
  echo "==================================================="
  echo " SANITIZER TEST BUILD SUCCEEDED"
  if [[ -n "$TEST_BIN" ]]; then
    echo " Tests: ${TEST_BIN}"
    echo " Run:   \"${TEST_BIN}\""
  fi
  echo " NOTE: build-asan is tests-only. Never install from this tree."
  echo "==================================================="
  exit 0
fi

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
  if [[ -x "${ROOT}/scripts/finalize_production_bundle.sh" ]]; then
    "${ROOT}/scripts/finalize_production_bundle.sh" \
      "$VST3" \
      "${VST3}/Contents/MacOS/AviatorKeyz" \
      "built VST3" \
      "${AVIATORKEYZ_CODESIGN_IDENTITY:--}"
  fi
  echo
  echo "Install VST3 for DAW testing:"
  echo "  ./scripts/install_vst3.sh \"${VST3}\""
  echo
  echo "Or:"
  echo "  ./scripts/build_macos.sh install"
  echo
  echo "Fast dev loop (no DAW):"
  echo "  open \"${STANDALONE}\""
else
  echo "WARNING: VST3 not found." >&2
  echo "Check build output for the actual artefact path." >&2
  exit 1
fi

if [[ "$DO_INSTALL" -eq 1 ]]; then
  "${ROOT}/scripts/install_vst3.sh" "$VST3"
fi
