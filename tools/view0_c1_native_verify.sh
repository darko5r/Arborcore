#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
make -s view0-c1-core-test
make -s view0-c1-adversarial-test
symbols=$(nm -g --defined-only build/libarborcore_view.a | awk '$2 ~ /^[TDBR]$/ {print $3}' | LC_ALL=C sort -u)
for fn in \
  arbor_view_measure_add \
  arbor_view_output_begin \
  arbor_view_output_append \
  arbor_view_output_commit \
  arbor_view_output_abort; do
  printf '%s\n' "$symbols" | grep -Fxq "$fn" || { echo "FAIL: missing VIEW0 C1 exported symbol: $fn" >&2; exit 1; }
done
echo 'VIEW0_C1_REQUIRED_EXPORTED_SYMBOL_COUNT=5'
echo 'PASS: VIEW0 C1 native core/adversarial behavior and required symbols preserved'
