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
ownership='tests/data/view0_v1n1_g06_wave_ownership.txt'
for path in "$contract" "$rules" "$consumers" "$link_as" "$ownership" \
  tools/c/view0_conformance/g06.c tools/c/view0_conformance/g06.h \
  tools/c/view0_conformance/g06_c0.c tools/c/view0_conformance/g06_c0.h; do
  [[ -f "$path" ]] || fail "missing G06 wave path: $path"
done
[[ "$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' "$contract" | head -1)" == \
    '0.1-VIEW0-V1N1-G06-R17A-SR1' ]] || fail 'contract revision drift'
[[ "$(awk -F '\t' 'NR>1{n++} END{print n+0}' "$rules")" == 17 ]] ||
  fail 'rule row count drift'
[[ "$(awk -F '\t' 'NR>1{n++} END{print n+0}' "$consumers")" == 79 ]] ||
  fail 'consumer policy count drift'
[[ "$(awk -F '\t' 'NR>1 && $1=="R15"{n++} END{print n+0}' "$consumers")" == 0 ]] ||
  fail 'R15 author-facing consumer invented'
tmp=$(mktemp -d /tmp/arborcore-g06-wave-contract.XXXXXX)
trap 'find "$tmp" -type f -delete; find "$tmp" -depth -type d -empty -delete' EXIT
awk -F '\t' 'NR>1{n[$1]++} END{for(i=1;i<=17;i++)printf "R%d\t%d\n",i,n["R" i]+0}' \
  "$consumers" > "$tmp/counts"
printf '%s\n' \
  $'R1\t22' $'R2\t12' $'R3\t2' $'R4\t9' $'R5\t7' $'R6\t3' \
  $'R7\t3' $'R8\t1' $'R9\t3' $'R10\t3' $'R11\t1' $'R12\t1' \
  $'R13\t3' $'R14\t1' $'R15\t0' $'R16\t5' $'R17\t3' > "$tmp/expected"
cmp -s "$tmp/expected" "$tmp/counts" || fail 'per-rule consumer policy partition drift'
grep -Fqx $'PRELOAD_ONLY\tG05_R3\tHTML_PRELOAD_DESTINATION\tfetch,font,image,script,style,track' "$link_as" || fail 'preload policy drift'
grep -Fqx $'MODULEPRELOAD_ONLY\tG05_R3\tHTML_MODULE_PRELOAD_DESTINATION\tjson,style,text,audioworklet,paintworklet,script,serviceworker,sharedworker,worker' "$link_as" || fail 'modulepreload policy drift'
grep -Fqx $'PRELOAD_AND_MODULEPRELOAD\tG05_R3\tINTERSECTION_OF_BOTH\tstyle,script' "$link_as" || fail 'both-rel intersection drift'
grep -Fqx $'NEITHER\tG05_R3\tSUPPRESS_G06_R2_PRIOR_OWNER\t' "$link_as" || fail 'neither-rel prior-owner policy drift'
for expected in \
  'VIEW0_V1N1_G06_R17A_RULE_IDENTITIES=17_OF_17' \
  'VIEW0_V1N1_G06_R17A_UNIQUE_CONSUMER_POLICIES=79' \
  'VIEW0_V1N1_G06_R17A_R15_AUTHOR_FACING_CONSUMERS=ZERO' \
  'VIEW0_V1N1_G06_R17A_SIGNED_INTEGER_RANGE=UNBOUNDED_AUTHORING_SYNTAX_C0_CONVERSION_RANGE_NOT_DIAGNOSTIC' \
  'VIEW0_V1N1_G06_R17A_TIME_YEAR_ONLY_BRANCH=ADMIT_NORMATIVE_WITHOUT_INVENTED_RULE' \
  'VIEW0_V1N1_G06_R17A_TIME_INVALID_UNION_DIAGNOSTIC=ONE_DETERMINISTIC_LEXICAL_SHAPE_OWNER' \
  'VIEW0_V1N1_G06_R17A_TIME_ELEMENT_DESCENDANT_OWNER=G03_PRIOR_OWNER_NO_G06_DUPLICATE' \
  'VIEW0_V1N1_G06_R17A_G05_PRESENCE_PLACEMENT_OWNER=RETAINED_NO_DUPLICATE' \
  'VIEW0_V1N1_G06_R17A_ACCEPT_DUPLICATE_MATCH=ASCII_CASE_INSENSITIVE_REJECT' \
  'VIEW0_V1N1_G06_R17A_BOUNDED_WORKSPACE_EXHAUSTION=MECHANISM_FAILURE_NOT_AUTHOR_DIAGNOSTIC' \
  'VIEW0_V1N1_G06_R17A_TOTAL_PUBLIC_FUNCTION_COUNT=11' \
  'VIEW0_V1N1_G06_R17A_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO' \
  'VIEW0_V1N1_G06_R17A_G05_GROUP_FREEZE=RETAINED' \
  'VIEW0_V1N1_G06_R17A_G06_GROUP_FREEZE=NO_PENDING_INDEPENDENT_REVIEW' \
  'VIEW0_V1N1_G06_R17A_COMPLETE_HTML_CONFORMANCE_CLAIM=NO' \
  'VIEW0_V1N1_G06_R17A_SR1_SCOPE=PORTABLE_VERIFIER_REGEX_AND_EXACT_DEPENDENT_OBJECT_RESET_NO_RULE_SEMANTIC_CHANGE' \
  'VIEW0_V1N1_G06_R17A_SR1_PREDECESSOR_ARCHIVE_SHA256=8480338a3ed7e78f7d24a66673f702968145b6bf912b7ce1132fdfd89f238ac9' \
  'VIEW0_V1N1_G06_R17A_SR1_GNU_GREP_STRAY_BACKSLASH_WARNINGS=ZERO_REQUIRED' \
  'VIEW0_V1N1_G06_R17A_SR1_DERIVED_RESET=G06_OBJECTS_TESTS_AND_SHARED_NATIVE_OBJECT' \
  'VIEW0_V1N1_G06_R17A_SR1_G06_GROUP_FREEZE=NO_PENDING_INDEPENDENT_REVIEW'; do
  grep -Fqx "$expected" "$contract" || fail "contract field drift: $expected"
done
for expected in \
  'VIEW0_V1N1_G06_WAVE_RULE_IDENTITIES=17' \
  'VIEW0_V1N1_G06_WAVE_UNIQUE_CONSUMER_POLICIES=79' \
  'VIEW0_V1N1_G06_WAVE_NORMALIZED_OCCURRENCES=246' \
  'VIEW0_V1N1_G06_WAVE_UNRESOLVED=0' \
  'VIEW0_V1N1_G06_WAVE_R15_AUTHOR_FACING_CONSUMERS=0' \
  'VIEW0_V1N1_G06_WAVE_COMPLETE_HTML_CONFORMANCE_CLAIM=NO'; do
  grep -Fqx "$expected" "$ownership" || fail "ownership field drift: $expected"
done
[[ "$(grep -Ec '^#define ARBOR_VIEW_V1_G06_[A-Z0-9_]+ UINT64_C\(0x00000000300600(0[1-9a-f]|1[01])\)$' tools/include/arborcore/view0_conformance/native.h)" == 17 ]] || fail 'public development rule identity count drift'
[[ "$(grep -Ec '^        ".*",?$' tools/c/view0_conformance/g06.c | tr -d ' ')" -ge 17 ]] || fail 'diagnostic message table drift'
! grep -Fq 'UINT16_C(15)' tools/c/view0_conformance/g06.c || fail 'R15 diagnostic publication invented'
[[ "$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)" == 11 ]] || fail 'production VIEW public API count drift'
! grep -Eq '\b(malloc|calloc|realloc|free)\s*\(' tools/c/view0_conformance/g06.c || fail 'direct G06 heap allocation'
echo 'VIEW0_V1N1_G06_WAVE_RULE_IDENTITIES=17_OF_17'
echo 'VIEW0_V1N1_G06_WAVE_UNIQUE_CONSUMER_POLICIES=79'
echo 'VIEW0_V1N1_G06_WAVE_R15_AUTHOR_FACING_CONSUMERS=ZERO'
echo 'VIEW0_V1N1_G06_WAVE_LINK_AS_A2_POLICY=PASS'
echo 'VIEW0_V1N1_G06_WAVE_G05_PRIOR_OWNER_BOUNDARY=PASS'
echo 'VIEW0_V1N1_G06_WAVE_CONTRACT_VERIFY=PASS'
echo 'PASS: G06 R1-R17 exact authority, ownership and no-overclaim contract'
