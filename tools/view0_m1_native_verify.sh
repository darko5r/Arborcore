#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
make -s view0-m1-utf8-test
make -s view0-m1-adversarial-test
make -s view0-m1-integration-test
make -s view0-c1-library
symbols=$(nm -g --defined-only build/libarborcore_view.a | awk '$2 ~ /^[TDBR]$/ && $3 ~ /^arbor_view_/ {print $3}' | LC_ALL=C sort)
expected=$(cat <<'SYMS'
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
arbor_view_utf8_validate
SYMS
)
[[ "$symbols" == "$expected" ]] || {
  echo 'FAIL: VIEW0 M1 cumulative exported symbol set differs' >&2
  printf '%s\n' '--- expected ---' "$expected" '--- actual ---' "$symbols" >&2
  exit 1
}
echo 'VIEW0_M1_EXPORTED_SYMBOL_COUNT=11'
echo 'VIEW0_M1_REAL_SOCKET_TEMPLATE_NATIVE_C_NASM=PASS'
echo 'PASS: VIEW0 M1 UTF-8 validation and real MVC0/HTTP1 HTML integration qualification'
