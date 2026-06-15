#!/usr/bin/env bash
# Fail if a production plugin executable links sanitizer runtimes or symbols.
set -euo pipefail

BINARY="${1:?Usage: $0 <plugin-executable> [label]}"
LABEL="${2:-$BINARY}"

if [[ ! -f "$BINARY" ]]; then
  echo "ERROR: Binary not found: $BINARY" >&2
  exit 1
fi

fail=0

# Runtime/symbol patterns only — avoid matching benign source-path strings.
DYLIB_PATTERN='libclang_rt\.(asan|ubsan|tsan)'
SYMBOL_PATTERN='__asan_|__ubsan_|__tsan_|___asan_|___ubsan_|___tsan_|__sanitizer_'
LOAD_CMD_PATTERN='libclang_rt\.(asan|ubsan|tsan)|/sanitizer/'
STRINGS_PATTERN='libclang_rt\.(asan|ubsan|tsan)|@rpath/libclang_rt\.(asan|ubsan|tsan)'

check_failed() {
  local check_name="$1"
  shift
  echo "FAIL [${check_name}]:" >&2
  "$@" >&2 || true
  fail=1
}

echo "== Verifying production plugin binary: ${LABEL} =="
echo "Path: ${BINARY}"

echo "-- file --"
file "$BINARY"

echo "-- check: linked dynamic libraries (otool -L) --"
if otool -L "$BINARY" 2>/dev/null | grep -Eiq "$DYLIB_PATTERN"; then
  check_failed "otool -L" grep -Ei "$DYLIB_PATTERN" <<< "$(otool -L "$BINARY" 2>/dev/null || true)"
else
  echo "OK: no sanitizer runtime in linked libraries"
fi

echo "-- check: undefined/imported symbols (nm -u) --"
if nm -u "$BINARY" 2>/dev/null | grep -Eiq "$SYMBOL_PATTERN"; then
  check_failed "nm -u" grep -Ei "$SYMBOL_PATTERN" <<< "$(nm -u "$BINARY" 2>/dev/null || true)"
else
  echo "OK: no sanitizer undefined symbols"
fi

echo "-- check: load commands (otool -l) --"
if otool -l "$BINARY" 2>/dev/null | grep -Eiq "$LOAD_CMD_PATTERN"; then
  check_failed "otool -l" grep -Ei "$LOAD_CMD_PATTERN" <<< "$(otool -l "$BINARY" 2>/dev/null || true)"
else
  echo "OK: no sanitizer load commands"
fi

echo "-- check: fallback runtime strings (strings) --"
if strings "$BINARY" 2>/dev/null | grep -Eiq "$STRINGS_PATTERN"; then
  check_failed "strings" grep -Ei "$STRINGS_PATTERN" <<< "$(strings "$BINARY" 2>/dev/null || true)"
else
  echo "OK: no sanitizer runtime strings"
fi

if [[ "$fail" -ne 0 ]]; then
  echo "Production plugin binary verification FAILED." >&2
  exit 1
fi

echo "PASS: production plugin binary is sanitizer-free."
