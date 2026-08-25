#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
make -s view0-v1-render-artifacts
cc -Iinclude -D_POSIX_C_SOURCE=200809L -std=c17 -O2 -g \
  -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion \
  -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef \
  -fanalyzer -fsyntax-only tests/c/view0_v1_render_artifacts.c
symbols=$(nm -g --defined-only build/libarborcore_view.a | awk '$2 ~ /^[TDBR]$/ && $3 ~ /^arbor_view_/ {print $3}' | LC_ALL=C sort)
count=$(printf '%s\n' "$symbols" | sed '/^$/d' | wc -l)
[[ "$count" -eq 11 ]] || { echo "FAIL: V1 unexpectedly changed production VIEW symbols: $count" >&2; exit 1; }
echo 'VIEW0_V1_GENERATOR_STRICT_COMPILE=PASS'
echo 'VIEW0_V1_GENERATOR_GCC_FANALYZER=PASS'
echo 'VIEW0_V1_PRODUCTION_VIEW_SYMBOL_COUNT=11'
echo 'PASS: V1 artifact generator is qualified without production API growth'
