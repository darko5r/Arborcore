#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
fail(){ echo "FAIL: $*" >&2; exit 1; }
contract='view/arborcore-view-core-1.contract'
matrix='tests/data/view0_v1n1_rc1_dependency_reconciliation.tsv'
[[ -f "$contract" && -f "$matrix" ]] || fail 'RC1 contract surface missing'
[[ "$(sed -n '1p' "$contract")" == 'ARBORCORE_VIEW_CORE_VERSION=0.1-VIEW0-V1N1-RC1' ]] || fail 'RC1 version'
for line in \
  'VIEW0_V1N1_RC1_DEPENDENCY_ROWS=21' \
  'VIEW0_V1N1_RC1_RESOLVED_V1N1_ROWS=7' \
  'VIEW0_V1N1_RC1_RETAINED_EXTERNAL_ROWS=13' \
  'VIEW0_V1N1_RC1_ALREADY_OWNED_ROWS=1' \
  'VIEW0_V1N1_RC1_RETIRED_DEFERRAL_FLAGS=SEVEN_ZERO_PUBLICATION' \
  'VIEW0_V1N1_RC1_G06_INVALID_SIZE_DIAGNOSTIC_OWNER=RETAINED_NO_DUPLICATE' \
  'VIEW0_V1N1_RC1_TOTAL_PUBLIC_FUNCTION_COUNT=11' \
  'VIEW0_V1N1_RC1_G05_GROUP_FREEZE=RETAINED' \
  'VIEW0_V1N1_RC1_G06_GROUP_FREEZE=RETAINED' \
  'VIEW0_V1N1_RC1_COMPLETE_HTML_CONFORMANCE_CLAIM=NO'; do
  grep -Fqx "$line" "$contract" || fail "contract line: $line"
done
[[ "$(awk -F '\t' 'NR>1{n++} END{print n+0}' "$matrix")" -eq 21 ]] || fail 'matrix rows'
[[ "$(awk -F '\t' 'NR>1&&$2=="RESOLVE_V1N1"{n++} END{print n+0}' "$matrix")" -eq 7 ]] || fail 'resolve rows'
[[ "$(awk -F '\t' 'NR>1&&$2=="RETAIN_EXTERNAL"{n++} END{print n+0}' "$matrix")" -eq 13 ]] || fail 'retain rows'
[[ "$(awk -F '\t' 'NR>1&&$2=="ALREADY_OWNED"{n++} END{print n+0}' "$matrix")" -eq 1 ]] || fail 'owned rows'
grep -Fq 'arbor_view0_native_g05_c0_input_state_from_type' tools/c/view0_conformance/g03_r3a.c || fail 'shared input classifier not consumed'
grep -Fq 'arbor_view0_native_g06_c0_nonnegative_integer' tools/c/view0_conformance/g03_r2a.c || fail 'G06 parser not consumed by R2'
grep -Fq 'arbor_view0_native_g06_c0_nonnegative_integer' tools/c/view0_conformance/g03_r3a.c || fail 'G06 parser not consumed by R3'
grep -Fq 'arbor_view0_native_g04_select_transparent_div_is_r7_subject' tools/c/view0_conformance/g03_r7a.c || fail 'G04 containing-model helper not consumed by R7'
for retired in \
  G03_R2_DEFERRED_NOSCRIPT G03_R2_DEFERRED_SELECT_SIZE \
  G03_R3_DEFERRED_INPUT_TYPE G03_R3_DEFERRED_CANVAS_INPUT_STATE \
  G03_R3_DEFERRED_CANVAS_SELECT_SIZE G03_R3_DEFERRED_NOSCRIPT \
  G03_R7_DEFERRED_G04_TRANSPARENT; do
  ! grep -Eq "set_deferred\([^;]*ARBOR_VIEW0_NATIVE_RESULT_FLAG_${retired}" \
    tools/c/view0_conformance/g03_r2a.c \
    tools/c/view0_conformance/g03_r3a.c \
    tools/c/view0_conformance/g03_r7a.c || fail "retired publication: $retired"
done
echo 'VIEW0_V1N1_RC1_DEPENDENCY_ROWS=21'
echo 'VIEW0_V1N1_RC1_RESOLVED_V1N1_ROWS=7'
echo 'VIEW0_V1N1_RC1_RETAINED_EXTERNAL_ROWS=13'
echo 'VIEW0_V1N1_RC1_ALREADY_OWNED_ROWS=1'
echo 'VIEW0_V1N1_RC1_RETIRED_DEFERRAL_FLAGS=ZERO_PUBLICATION'
echo 'VIEW0_V1N1_RC1_CONTRACT_VERIFY=PASS'
echo 'PASS: exact RC0 disposition matrix and RC1 ownership contract bound'
