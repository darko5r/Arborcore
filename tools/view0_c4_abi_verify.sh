#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
make -s build/view0-c4/view0_c4_abi_test.o view0-c4-core-test view0-c4-adversarial-test
OBJ=build/view0-c4/view0_c4_abi_test.o
for sym in view0_c4_asm_render_html_text view0_c4_asm_begin_abort view0_c4_asm_call_render_preserve; do
  nm -g --defined-only "$OBJ" | awk '{print $3}' | grep -Fxq "$sym" || { echo "FAIL: missing C4 NASM symbol $sym" >&2; exit 1; }
done
for sym in arbor_view_measure_add arbor_view_html_text_measure arbor_view_output_begin arbor_view_output_append arbor_view_html_text_append arbor_view_output_commit arbor_view_output_abort; do
  nm -u "$OBJ" | awk '{print $2}' | grep -Fxq "$sym" || { echo "FAIL: C4 NASM object missing C VIEW dependency $sym" >&2; exit 1; }
done
for sym in range_end_checked range_overlaps; do
  nm -u "$OBJ" | awk '{print $2}' | grep -Fxq "$sym" || { echo "FAIL: C4 NASM object missing frozen range dependency $sym" >&2; exit 1; }
done
readelf -W -S "$OBJ" | grep -q '\.note\.GNU-stack' || { echo 'FAIL: C4 NASM object lacks GNU-stack note' >&2; exit 1; }
./build/view0-c4-test >/dev/null
./build/view0-c4-adversarial-test >/dev/null
echo "VIEW0_C4_REAL_NASM_ABI_OBJECT_SHA256=$(sha256sum "$OBJ" | awk '{print $1}')"
echo "VIEW0_C4_REAL_NASM_ABI_TEXT_BYTES=$(size -A "$OBJ" | awk '$1==".text"{print $2}')"
echo 'VIEW0_C4_ASSEMBLY_TO_C_VIEW_C_API_ABI=PASS'
echo 'VIEW0_C4_ARBOR_STATUS_RAX_RDX=PASS'
echo 'VIEW0_C4_ARBOR_SPAN_INTEGER_ARGUMENT_CLASS=PASS'
echo 'VIEW0_C4_SYSV_STACK_ALIGNMENT=PASS'
echo 'VIEW0_C4_SYSV_CALLEE_SAVED_GPRS=PASS'
echo 'VIEW0_C4_ALL_SEVEN_VIEW_C_FUNCTIONS_CONSUMED=PASS'
echo 'PASS: VIEW0 C4 real NASM renders through the existing VIEW C API under SysV AMD64'
