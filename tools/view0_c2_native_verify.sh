#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
make -s view0-c2-core-test
make -s view0-c2-adversarial-test
make -s view0-c1-library
symbols=$(nm -g --defined-only build/libarborcore_view.a | awk '$2 ~ /^[TDBR]$/ && $3 ~ /^arbor_view_/ {print $3}' | LC_ALL=C sort)
for sym in \
  arbor_view_html_text_append \
  arbor_view_html_text_measure \
  arbor_view_measure_add \
  arbor_view_output_abort \
  arbor_view_output_append \
  arbor_view_output_begin \
  arbor_view_output_commit; do
  printf '%s\n' "$symbols" | grep -Fxq "$sym" || { echo "FAIL: required C2 VIEW symbol missing: $sym" >&2; exit 1; }
done
count=$(printf '%s\n' "$symbols" | sed '/^$/d' | wc -l)
[[ "$count" -ge 7 ]]
echo 'VIEW0_C2_REQUIRED_EXPORTED_SYMBOL_COUNT=7'
echo "VIEW0_C2_CUMULATIVE_EXPORTED_SYMBOL_COUNT=$count"
echo 'PASS: VIEW0 C2 core/adversarial behavior and required symbols preserved under cumulative VIEW0 extension'
