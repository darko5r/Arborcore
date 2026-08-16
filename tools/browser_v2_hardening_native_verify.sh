#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
OUT="$ROOT/build/browser-v2-hardening-native"
mkdir -p "$OUT"
CC="${CC:-cc}"
"$CC" -I"$ROOT/include" -std=c17 -O2 -Wall -Wextra -Werror -pedantic \
  "$ROOT/src/c/browser_hardening_v2.c" \
  "$ROOT/tests/c/browser_hardening_v2_test.c" \
  -o "$OUT/browser-hardening-v2-test"
"$OUT/browser-hardening-v2-test"
echo 'BV2H_NATIVE_RESULT=PASS'
