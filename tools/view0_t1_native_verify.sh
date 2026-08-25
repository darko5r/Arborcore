#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
make -s view0-t1-core-test
make -s view0-t1-adversarial-test
make -s view0-c1-library
symbols=$(nm -g --defined-only build/libarborcore_view.a | awk '$2 ~ /^[TDBR]$/ && $3 ~ /^arbor_view_/ {print $3}' | LC_ALL=C sort)
required=$(cat <<'SYMS'
arbor_view_html_template_measure
arbor_view_html_template_prepare
arbor_view_html_template_render
arbor_view_html_text_append
arbor_view_html_text_measure
arbor_view_measure_add
arbor_view_output_abort
arbor_view_output_append
arbor_view_output_begin
arbor_view_output_commit
SYMS
)
while IFS= read -r symbol; do
  [[ -n "$symbol" ]] || continue
  grep -Fxq "$symbol" <<< "$symbols" || { echo "FAIL: missing required T1 symbol under extension: $symbol" >&2; exit 1; }
done <<< "$required"
count=$(printf '%s\n' "$symbols" | sed '/^$/d' | wc -l)
echo 'VIEW0_T1_REQUIRED_EXPORTED_SYMBOL_COUNT=10'
echo "VIEW0_T1_CUMULATIVE_EXPORTED_SYMBOL_COUNT=$count"
echo 'PASS: VIEW0 T1 prepared-template core/adversarial behavior and required symbols preserved under cumulative VIEW0 extension'
