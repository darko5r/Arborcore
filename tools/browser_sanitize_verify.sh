#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
CC="${CC:-cc}"
OUT="$ROOT/build/browser-sanitize"
mkdir -p "$OUT"
"$CC" -I"$ROOT/include" -I"$ROOT/tests/c" -D_POSIX_C_SOURCE=200809L \
  -std=c17 -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined \
  -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion -Wshadow \
  -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef \
  "$ROOT/tests/c/browser_surface_test.c" \
  "$ROOT/src/c/browser_surface.c" "$ROOT/src/c/renderer.c" "$ROOT/src/c/geometry.c" \
  -o "$OUT/browser-surface-sanitize"
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 "$OUT/browser-surface-sanitize" >/dev/null
echo 'PASS: B0/B1 ASan/UBSan browser export qualification'
