#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
contract=view/arborcore-view-core-1.contract
header=include/arborcore/view.h
source=src/c/view.c
asm=tests/asm/view0_c4_abi_test.asm
doc=docs/VIEW_CORE_VIEW0.md
for item in \
 'VIEW0_C4_CONTRACT_REVISION=0.1-VIEW0-C4' \
 'VIEW0_C4_SCOPE=REAL_NASM_TO_C_VIEW_API_SYSV_QUALIFICATION' \
 'VIEW0_C4_PUBLIC_FUNCTION_COUNT=0' \
 'VIEW0_C4_TOTAL_PUBLIC_FUNCTION_COUNT=7' \
 'VIEW0_C4_PRODUCTION_VIEW_SOURCE_CHANGE=NO' \
 'VIEW0_C4_PRODUCTION_VIEW_HEADER_CHANGE=NO' \
 'VIEW0_C4_ASSEMBLY_ABI_V1_REOPEN=NO' \
 'VIEW0_C4_ASM_ROLE=TEST_ONLY_APPLICATION_STYLE_CONSUMER' \
 'VIEW0_C4_CALLING_CONVENTION=SYSTEM_V_AMD64' \
 'VIEW0_C4_ARBOR_STATUS_RETURN=RAX_RDX_TWO_INTEGER_EIGHTBYTES' \
 'VIEW0_C4_ARBOR_SPAN_ARGUMENT=BY_VALUE_TWO_INTEGER_EIGHTBYTES' \
 'VIEW0_C4_CALL_STACK_ALIGNMENT=16_BYTE_BEFORE_CALL' \
 'VIEW0_C4_CALLEE_SAVED_GPRS=RBX_RBP_R12_R13_R14_R15_PRESERVED' \
 'VIEW0_C4_VIEW_API_SYMBOLS_CONSUMED=7' \
 'VIEW0_C4_DIRECT_ASSEMBLY_RANGE_HELPERS=RANGE_END_CHECKED_RANGE_OVERLAPS' \
 'VIEW0_C4_SOURCE_FITTING_FUTURE_BODY_ALIAS=REJECT_BEFORE_BODY_WRITE' \
 'VIEW0_C4_PRIOR_SAME_ARENA_SOURCE=ALLOW_NONOVERLAPPING_FUTURE_OUTPUT' \
 'VIEW0_C4_CAPACITY_PRECEDENCE=C1_ENOSPC_BEFORE_SPECULATIVE_FUTURE_ALIAS' \
 'VIEW0_C4_BODY_PUBLICATION=C1_COMMIT_ONLY' \
 'VIEW0_C4_BEGIN_ABORT_FROM_REAL_NASM=QUALIFIED' \
 'VIEW0_C4_PRODUCTION_ASSEMBLY_SYMBOL_ADDITION=ZERO' \
 'VIEW0_C4_USER_FACING_HTTP_ADMISSION=NOT_YET' \
 'VIEW0_C4_DATABASE_DEPENDENCY=NONE'; do
  grep -Fxq "$item" "$contract" || { echo "FAIL: missing C4 contract marker: $item" >&2; exit 1; }
done
count=$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' "$header")
[[ "$count" -ge 7 ]] || { echo "FAIL: cumulative VIEW public surface lost C4-required functions; got $count" >&2; exit 1; }
for sym in arbor_view_measure_add arbor_view_html_text_measure arbor_view_output_begin arbor_view_output_append arbor_view_html_text_append arbor_view_output_commit arbor_view_output_abort; do
  grep -Fq "extern $sym" "$asm" || { echo "FAIL: C4 NASM does not consume $sym" >&2; exit 1; }
done
grep -Fq 'extern range_end_checked' "$asm"
grep -Fq 'extern range_overlaps' "$asm"
grep -Fq 'real hand-written NASM' "$doc"
grep -Fq 'C4 does not weaken C3' "$doc"
echo 'VIEW0_C4_PUBLIC_FUNCTION_COUNT=0'
echo "VIEW0_TOTAL_PUBLIC_FUNCTION_COUNT=$count"
echo 'VIEW0_C4_REQUIRED_VIEW_C_API_PRESENT=YES'
echo 'PASS: VIEW0 C4 real NASM qualification adds no production VIEW or Assembly ABI surface'
