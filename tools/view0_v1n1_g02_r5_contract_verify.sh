#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
contract='view/arborcore-view-core-1.contract'
doc='docs/VIEW_CORE_VIEW0.md'
header='tools/include/arborcore/view0_conformance/native.h'
native='tools/c/view0_conformance/native.c'
adapter='tools/c/view0_conformance/lexbor_adapter.c'

fail() { echo "FAIL: $*" >&2; exit 1; }

bash tools/view0_v1n1_g02_r4_contract_verify.sh >/dev/null

grep -Eq '^ARBORCORE_VIEW_CORE_VERSION=0\.1-VIEW0-V1N1-G02-R(5|6)$' "$contract" || fail 'current contract is not G02 R5 or admitted R6 extension'
for item in \
 'VIEW0_V1N1_G02_R5_CONTRACT_REVISION=0.1-VIEW0-V1N1-G02-R5' \
 'VIEW0_V1N1_G02_R5_SCOPE=HEAD_BASE_CARDINALITY_OVER_RETAINED_R4' \
 'VIEW0_V1N1_G02_R5_RULE_ID=0x0000000030020007' \
 'VIEW0_V1N1_G02_R5_RULE_SYMBOL=ARBOR_VIEW_V1_G02_HEAD_BASE_CARDINALITY' \
 'VIEW0_V1N1_G02_R5_GROUP=G02' \
 'VIEW0_V1N1_G02_R5_SOURCE_LOCATOR=GENERAL_G02_HEAD_ELEMENT_WHATWG_LINES_16132_16200' \
 'VIEW0_V1N1_G02_R5_SOURCE_FINGERPRINT_SHA256=f8584c1719db70e2264af3348e382ca2287a5aa35d74372c778dcae60783ae15' \
 'VIEW0_V1N1_G02_R5_MATRIX_SHA256=2e6ce586cbaf09853be0a2b85bac813e4a959c6cae4f3195358ca2666c33ff94' \
 'VIEW0_V1N1_G02_R5_SEVERITY=ERROR' \
 'VIEW0_V1N1_G02_R5_REQUIRED_HEAD_BASE_CHILD_COUNT=AT_MOST_ONE' \
 'VIEW0_V1N1_G02_R5_ZERO_BASE_CONFORMING=YES' \
 'VIEW0_V1N1_G02_R5_DECISION=DOM_HEAD_DIRECT_BASE_CHILD_COUNT' \
 'VIEW0_V1N1_G02_R5_DUPLICATE_BASE_DIAGNOSTIC_ANCHOR=SECOND_AUTHORED_BASE_START_TAG_NAME_BYTE_RANGE' \
 'VIEW0_V1N1_G02_R5_DUPLICATE_BASE_ANCHOR_LENGTH=4' \
 'VIEW0_V1N1_G02_R5_PARSER_REPAIR_POLICY=COUNT_FINAL_HEAD_CHILDREN_RETAIN_SOURCE_DUPLICATE_ANCHOR' \
 'VIEW0_V1N1_G02_R5_G03_GENERIC_CONTENT_MODEL_DUPLICATE_REPORTING=DEFERRED_AND_SUPPRESSED_BY_SPECIFIC_G02_PRECEDENCE' \
 'VIEW0_V1N1_G02_R5_PARSE_DIAGNOSTICS_PRESERVED=YES' \
 'VIEW0_V1N1_G02_R5_PARSE_CLEAN_FLAG_MEANS_PARSER_CLEAN_NOT_DIAGNOSTIC_FREE' \
 'VIEW0_V1N1_G02_R5_CAPACITY_FAILURE_ATOMICITY=RETAINED_TWO_PASS_MEASURE_THEN_PUBLISH' \
 'VIEW0_V1N1_G02_R5_C0_FACT_LAYOUT_CHANGE=NO' \
 'VIEW0_V1N1_G02_R5_C0_FACT_SIZE_X86_64=184' \
 'VIEW0_V1N1_G02_R5_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO' \
 'VIEW0_V1N1_G02_R5_PUBLIC_FUNCTION_COUNT=0' \
 'VIEW0_V1N1_G02_R5_TOTAL_PUBLIC_FUNCTION_COUNT=11' \
 'VIEW0_V1N1_G02_R5_RUNTIME_REQUEST_PATH=NO' \
 'VIEW0_V1N1_G02_R5_G02_RULES_IMPLEMENTED=5_OF_6' \
 'VIEW0_V1N1_G02_R5_G03_G06_ENFORCEMENT=ZERO' \
 'VIEW0_V1N1_G02_R5_RETIRED_RULE_IDS_REUSED=NO' \
 'VIEW0_V1N1_G02_R5_COMPLETE_HTML_CONFORMANCE_CLAIM=NO' \
 'VIEW0_V1N1_G02_R5_JAVA_OR_VNU_ACTIVE_DEPENDENCY=NO' \
 'VIEW0_V1N1_G02_R5_DATABASE_DEPENDENCY=NONE' \
 'VIEW0_V1N1_G02_R5_R_DEPENDENCY=NONE'; do
    grep -Fxq "$item" "$contract" || fail "missing G02 R5 contract marker: $item"
done

for item in \
 '#define ARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED UINT64_C(0x0000000030020001)' \
 '#define ARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX UINT64_C(0x0000000030020002)' \
 '#define ARBOR_VIEW_V1_G02_DOCTYPE_LEGACY_DISCOURAGED UINT64_C(0x0000000030020003)' \
 '#define ARBOR_VIEW_V1_G02_HEAD_TITLE_CARDINALITY UINT64_C(0x0000000030020006)' \
 '#define ARBOR_VIEW_V1_G02_HEAD_BASE_CARDINALITY UINT64_C(0x0000000030020007)'; do
    grep -Fxq "$item" "$header" || fail "missing retained/current G02 macro: $item"
done

grep -Fq 'g02_head_base_cardinality_check(' "$native" || fail 'G02 R5 base-cardinality evaluator missing'
grep -Fq 'diagnostic->rule_id = ARBOR_VIEW_V1_G02_HEAD_BASE_CARDINALITY;' "$native" || fail 'G02 R5 diagnostic publication missing'
grep -Fq 'facts->dom_head_base_child_count <= 1u' "$native" || fail 'G02 R5 DOM head-base decision missing'
grep -Fq 'facts->source_second_base_start_tag_offset' "$native" || fail 'G02 R5 second-base source anchor missing'
! grep -Fq 'ARBOR_VIEW_V1_G02_HEAD_BASE_CARDINALITY' "$adapter" || fail 'G02 R5 semantic rule leaked into Lexbor adapter'

active_symbols=$(grep -ERho 'ARBOR_VIEW_V1_G02_[A-Z0-9_]+' "$header" "$native" | LC_ALL=C sort -u)
required_symbols=$'ARBOR_VIEW_V1_G02_DOCTYPE_LEGACY_DISCOURAGED\nARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED\nARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX\nARBOR_VIEW_V1_G02_HEAD_BASE_CARDINALITY\nARBOR_VIEW_V1_G02_HEAD_TITLE_CARDINALITY'
allowed_symbols=$'ARBOR_VIEW_V1_G02_BODY_SINGLETON\nARBOR_VIEW_V1_G02_DOCTYPE_LEGACY_DISCOURAGED\nARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED\nARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX\nARBOR_VIEW_V1_G02_HEAD_BASE_CARDINALITY\nARBOR_VIEW_V1_G02_HEAD_TITLE_CARDINALITY'
while IFS= read -r symbol; do
    [[ -n "$symbol" ]] || continue
    grep -Fxq "$symbol" <<< "$active_symbols" || fail "G02 R5 semantic symbol disappeared: $symbol"
done <<< "$required_symbols"
while IFS= read -r symbol; do
    [[ -n "$symbol" ]] || continue
    grep -Fxq "$symbol" <<< "$allowed_symbols" || fail "unexpected active G02 semantic symbol under R5 contract: $symbol"
done <<< "$active_symbols"
! grep -ERq '0x000000003002000[45]' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'retired G02 rule ID reused'
! grep -ERq 'ARBOR_VIEW_V1_G0[3-6]_' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G03-G06 enforcement appeared during G02 R5'

count=$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)
[[ "$count" -eq 11 ]] || fail "G02 R5 changed production VIEW API count: $count"
! grep -Rq 'lxb_' include/arborcore/view.h || fail 'Lexbor type leaked into production VIEW header'
! grep -Fq 'lxb_' "$header" || fail 'Lexbor type leaked across private Arborcore header'

grep -Fq '## V1N1 G02 R5: head/base maximum cardinality' "$doc"
grep -Fq 'implements **5 of 6**' "$doc"
grep -Fq 'dom_head_base_child_count' "$doc"

echo 'VIEW0_V1N1_G02_R5_PUBLIC_FUNCTION_COUNT=0'
echo 'VIEW0_TOTAL_PUBLIC_FUNCTION_COUNT=11'
echo 'VIEW0_V1N1_G02_R5_RULE_ID=0x0000000030020007'
echo 'VIEW0_V1N1_G02_R5_G02_RULES_IMPLEMENTED=5_OF_6'
echo 'PASS: G02 R5 frozen head/base maximum-cardinality, C0-fact consumption and no-scope-creep contract'
