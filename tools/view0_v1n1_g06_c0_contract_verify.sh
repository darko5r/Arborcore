#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
fail(){ echo "FAIL: $*" >&2; exit 1; }
contract='view/arborcore-view-core-1.contract'
rules='tests/data/view0_v1n1_g06_c0_rule_authority.tsv'
consumers='tests/data/view0_v1n1_g06_c0_consumer_policy.tsv'
link_as='tests/data/view0_v1n1_g06_c0_link_as_policy.tsv'
summary='tests/data/view0_v1n1_g06_c0_foundation_summary.txt'
for path in "$contract" "$rules" "$consumers" "$link_as" "$summary" \
  tools/c/view0_conformance/g06_c0.c tools/c/view0_conformance/g06_c0.h; do
  [[ -f "$path" ]] || fail "missing G06 C0 path: $path"
done
[[ "$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' "$contract" | head -1)" == '0.1-VIEW0-V1N1-G06-C0' ]] || fail 'contract revision drift'
[[ "$(awk -F '\t' 'NR>1{n++} END{print n+0}' "$rules")" == 17 ]] || fail 'rule row count drift'
[[ "$(awk -F '\t' 'NR>1{n++} END{print n+0}' "$consumers")" == 79 ]] || fail 'consumer policy count drift'
[[ "$(awk -F '\t' 'NR>1 && $1=="R15"{n++} END{print n+0}' "$consumers")" == 0 ]] || fail 'R15 author-facing consumer invented'
[[ "$(awk -F '\t' 'NR>1 && $1=="R8"{n++} END{print n+0}' "$consumers")" == 1 ]] || fail 'R8 supplement drift'
[[ "$(awk -F '\t' 'NR>1 && $1=="R11"{n++} END{print n+0}' "$consumers")" == 1 ]] || fail 'R11 supplement drift'
[[ "$(awk -F '\t' 'NR>1 && $5=="G05_G06_SHARED"{n++} END{print n+0}' "$consumers")" == 24 ]] || fail 'G05/G06 shared ownership count drift'
grep -Fqx $'PRELOAD_ONLY\tG05_R3\tHTML_PRELOAD_DESTINATION\tfetch,font,image,script,style,track' "$link_as" || fail 'preload policy drift'
grep -Fqx $'MODULEPRELOAD_ONLY\tG05_R3\tHTML_MODULE_PRELOAD_DESTINATION\tjson,style,text,audioworklet,paintworklet,script,serviceworker,sharedworker,worker' "$link_as" || fail 'modulepreload policy drift'
grep -Fqx $'PRELOAD_AND_MODULEPRELOAD\tG05_R3\tINTERSECTION_OF_BOTH\tstyle,script' "$link_as" || fail 'both-rel intersection drift'
grep -Fqx $'NEITHER\tG05_R3\tSUPPRESS_G06_R2_PRIOR_OWNER\t' "$link_as" || fail 'neither-rel prior-owner policy drift'
for expected in \
  'VIEW0_V1N1_G06_C0_RULE_ROWS=17' \
  'VIEW0_V1N1_G06_C0_NORMALIZED_OCCURRENCES=246' \
  'VIEW0_V1N1_G06_C0_AUTHOR_FACING_RAW_ROWS=128' \
  'VIEW0_V1N1_G06_C0_UNIQUE_CONSUMER_POLICIES=79' \
  'VIEW0_V1N1_G06_C0_SUPPLEMENTAL_CONSUMERS=2' \
  'VIEW0_V1N1_G06_C0_UNRESOLVED=0' \
  'VIEW0_V1N1_G06_C0_R15_AUTHOR_FACING_CONSUMERS=0' \
  'VIEW0_V1N1_G06_C0_HTML_SOURCE_SHA256=e31f0ee8ff647eb2f283dcdd45d4c30972ae4e8ad3fbf5e3c012bf3e63b19703' \
  'VIEW0_V1N1_G06_C0_FETCH_SOURCE_SHA256=2099e5170175b36f61ab3234849c429702552d3587d50b87149269336977eb98' \
  'VIEW0_V1N1_G06_C0_G06_DIAGNOSTIC_RULES_IMPLEMENTED=ZERO' \
  'VIEW0_V1N1_G06_C0_COMPLETE_HTML_CONFORMANCE_CLAIM=NO'; do
  grep -Fqx "$expected" "$contract" || fail "contract field drift: $expected"
  grep -Fqx "$expected" "$summary" || fail "summary field drift: $expected"
done
[[ "$(grep -Ec '^R([1-9]|1[0-7])[[:space:]]+0x00000000300600(0[1-9A-F]|1[01])[[:space:]]+ARBOR_VIEW_V1_G06_' "$rules")" == 17 ]] || fail 'rule identity series drift'
[[ "$(grep -Fc 'UINT64_C(0x00000000300600' tools/c/view0_conformance/g06_c0.c)" == 17 ]] || fail 'compiled rule identity count drift'
! grep -Eq 'ARBOR_VIEW_V1_G06_|0x00000000300600' tools/c/view0_conformance/native.c tools/include/arborcore/view0_conformance/native.h || fail 'G06 diagnostics integrated during C0'
echo 'VIEW0_V1N1_G06_C0_RULE_AUTHORITY=17_OF_17'
echo 'VIEW0_V1N1_G06_C0_UNIQUE_CONSUMER_POLICIES=79'
echo 'VIEW0_V1N1_G06_C0_R15_AUTHOR_FACING_CONSUMERS=ZERO'
echo 'VIEW0_V1N1_G06_C0_LINK_AS_A2_POLICY=PASS'
echo 'VIEW0_V1N1_G06_C0_G06_DIAGNOSTIC_RULES_IMPLEMENTED=ZERO'
echo 'PASS: G06 C0 contract, authority surface and zero-diagnostic boundary'
