#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
fail(){ echo "FAIL: $*" >&2; exit 1; }
contract='view/arborcore-view-core-1.contract'
rules='tests/data/view0_v1n2_c0_rule_authority.tsv'
authority='tests/data/view0_v1n2_c0_external_authority.tsv'
for path in "$contract" "$rules" "$authority" \
  tools/c/view0_conformance/v1n2_c0.c \
  tools/c/view0_conformance/v1n2_c0.h; do
  [[ -f "$path" ]] || fail "missing V1N2 C0 path: $path"
done
[[ "$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' "$contract" | head -1)" == \
  '0.1-VIEW0-V1N2-C0' ]] || fail 'contract revision drift'
printf '%s  %s\n' \
  'cb4513a71510a2dd43c577900321f27fea9970c45ee27ba9914afe9a7a30724c' "$rules" \
  '11f25a3dffc249aec72ae17e9e1621592d0152d5d259dd41c9561c3edb621ba7' "$authority" \
  | sha256sum -c -
[[ "$(awk -F '\t' 'NR>1{n++} END{print n+0}' "$rules")" == 38 ]] || fail 'rule row count drift'
[[ "$(awk -F '\t' 'NR>1{n++} END{print n+0}' "$authority")" == 12 ]] || fail 'authority row count drift'
[[ "$(awk -F '\t' 'NR>1{n[$3]++} END{printf "%d/%d/%d/%d/%d",n["G07"],n["G08"],n["G09"],n["G10"],n["G11"]}' "$rules")" == '5/12/6/13/2' ]] || fail 'group count drift'
[[ "$(awk -F '\t' 'NR>1{n[$10]++} END{printf "%d/%d/%d",n["STATIC_DOCUMENT_ONLY"],n["STATIC_HTML_INTEGRATION_ONLY"],n["STATIC_DETERMINISTIC_SUBSET_ONLY"]}' "$rules")" == '36/1/1' ]] || fail 'admission partition drift'
[[ "$(grep -Ec 'UINT64_C\(0x00000000300(7|8|9|a|b)00' tools/c/view0_conformance/v1n2_c0.c)" == 38 ]] || fail 'compiled identity count drift'
! grep -Eq 'ARBOR_VIEW_V1_G(07|08|09|10|11)_|0x00000000300(7|8|9|a|b)00' \
  tools/c/view0_conformance/native.c tools/include/arborcore/view0_conformance/native.h \
  || fail 'V1N2 diagnostics integrated during zero-diagnostic C0'
for expected in \
  'VIEW0_V1N2_C0_RULE_IDENTITIES=38_OF_38' \
  'VIEW0_V1N2_C0_GROUP_COUNTS=G07_5_G08_12_G09_6_G10_13_G11_2' \
  'VIEW0_V1N2_C0_AUTHORITY_ROWS=12' \
  'VIEW0_V1N2_C0_SHARED_G07_G11_ANCHOR_ARENAS=1' \
  'VIEW0_V1N2_C0_DIAGNOSTIC_RULES_IMPLEMENTED=ZERO' \
  'VIEW0_V1N2_C0_PRODUCTION_VIEW_API_CHANGE=NO' \
  'VIEW0_V1N2_C0_TOTAL_PUBLIC_FUNCTION_COUNT=11' \
  'VIEW0_V1N2_C0_PHASED_STACK_BOUND_BYTES=900000' \
  'VIEW0_V1N2_C0_STACK_THRESHOLD_WIDENING=NO' \
  'VIEW0_V1N2_C0_COMPLETE_HTML_CONFORMANCE_CLAIM=NO'; do
  grep -Fqx "$expected" "$contract" || fail "contract field drift: $expected"
done
echo 'VIEW0_V1N2_C0_CONTRACT_VERIFY=PASS'
echo 'VIEW0_V1N2_C0_RULE_IDENTITIES=38_OF_38'
echo 'VIEW0_V1N2_C0_DIAGNOSTIC_RULES_IMPLEMENTED=ZERO'
echo 'PASS: V1N2 C0 exact authority, resource and zero-diagnostic boundary'
