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

bash tools/view0_v1n1_g02_r3_contract_verify.sh >/dev/null

grep -Eq '^ARBORCORE_VIEW_CORE_VERSION=0\.1-VIEW0-V1N1-G02-R(4|5|6)$' "$contract" || fail 'current contract is not retained G02 R4 or admitted higher G02 extension'
for item in \
 'VIEW0_V1N1_G02_R4_CONTRACT_REVISION=0.1-VIEW0-V1N1-G02-R4' \
 'VIEW0_V1N1_G02_R4_SCOPE=HEAD_TITLE_CARDINALITY_STANDALONE_DOCUMENT_ONLY_OVER_RETAINED_R3' \
 'VIEW0_V1N1_G02_R4_RULE_ID=0x0000000030020006' \
 'VIEW0_V1N1_G02_R4_RULE_SYMBOL=ARBOR_VIEW_V1_G02_HEAD_TITLE_CARDINALITY' \
 'VIEW0_V1N1_G02_R4_GROUP=G02' \
 'VIEW0_V1N1_G02_R4_SOURCE_LOCATOR=GENERAL_G02_HEAD_ELEMENT_WHATWG_LINES_16132_16200' \
 'VIEW0_V1N1_G02_R4_SOURCE_FINGERPRINT_SHA256=f8584c1719db70e2264af3348e382ca2287a5aa35d74372c778dcae60783ae15' \
 'VIEW0_V1N1_G02_R4_MATRIX_SHA256=2e6ce586cbaf09853be0a2b85bac813e4a959c6cae4f3195358ca2666c33ff94' \
 'VIEW0_V1N1_G02_R4_SEVERITY=ERROR' \
 'VIEW0_V1N1_G02_R4_DOCUMENT_MODE=STANDALONE_DOCUMENT' \
 'VIEW0_V1N1_G02_R4_REQUIRED_HEAD_TITLE_CHILD_COUNT=EXACTLY_ONE' \
 'VIEW0_V1N1_G02_R4_SRCDOC_HIGHER_LEVEL_TITLE_EXCEPTION=DEFERRED_UNTIL_EXPLICIT_FUTURE_MODE' \
 'VIEW0_V1N1_G02_R4_DECISION=DOM_HEAD_DIRECT_TITLE_CHILD_COUNT' \
 'VIEW0_V1N1_G02_R4_MISSING_TITLE_DIAGNOSTIC_ANCHOR=DOCUMENT_START_BYTE_0_LENGTH_0' \
 'VIEW0_V1N1_G02_R4_DUPLICATE_TITLE_DIAGNOSTIC_ANCHOR=SECOND_AUTHORED_TITLE_START_TAG_NAME_BYTE_RANGE' \
 'VIEW0_V1N1_G02_R4_DUPLICATE_TITLE_ANCHOR_LENGTH=5' \
 'VIEW0_V1N1_G02_R4_PARSER_REPAIR_POLICY=COUNT_FINAL_HEAD_CHILDREN_RETAIN_SOURCE_DUPLICATE_ANCHOR' \
 'VIEW0_V1N1_G02_R4_TITLE_OUTSIDE_HEAD_DOES_NOT_SATISFY_RULE=YES' \
 'VIEW0_V1N1_G02_R4_G03_GENERIC_CONTENT_MODEL_DUPLICATE_REPORTING=DEFERRED_AND_SUPPRESSED_BY_SPECIFIC_G02_PRECEDENCE' \
 'VIEW0_V1N1_G02_R4_PARSE_DIAGNOSTICS_PRESERVED=YES' \
 'VIEW0_V1N1_G02_R4_PARSE_CLEAN_FLAG_MEANS_PARSER_CLEAN_NOT_DIAGNOSTIC_FREE' \
 'VIEW0_V1N1_G02_R4_EMPTY_DOCUMENT_ACCUMULATES_R1_AND_R4=YES' \
 'VIEW0_V1N1_G02_R4_CAPACITY_FAILURE_ATOMICITY=RETAINED_TWO_PASS_MEASURE_THEN_PUBLISH' \
 'VIEW0_V1N1_G02_R4_C0_FACT_LAYOUT_CHANGE=NO' \
 'VIEW0_V1N1_G02_R4_C0_FACT_SIZE_X86_64=184' \
 'VIEW0_V1N1_G02_R4_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO' \
 'VIEW0_V1N1_G02_R4_PUBLIC_FUNCTION_COUNT=0' \
 'VIEW0_V1N1_G02_R4_TOTAL_PUBLIC_FUNCTION_COUNT=11' \
 'VIEW0_V1N1_G02_R4_RUNTIME_REQUEST_PATH=NO' \
 'VIEW0_V1N1_G02_R4_G02_RULES_IMPLEMENTED=4_OF_6' \
 'VIEW0_V1N1_G02_R4_G03_G06_ENFORCEMENT=ZERO' \
 'VIEW0_V1N1_G02_R4_RETIRED_RULE_IDS_REUSED=NO' \
 'VIEW0_V1N1_G02_R4_COMPLETE_HTML_CONFORMANCE_CLAIM=NO' \
 'VIEW0_V1N1_G02_R4_JAVA_OR_VNU_ACTIVE_DEPENDENCY=NO' \
 'VIEW0_V1N1_G02_R4_DATABASE_DEPENDENCY=NONE' \
 'VIEW0_V1N1_G02_R4_R_DEPENDENCY=NONE'; do
    grep -Fxq "$item" "$contract" || fail "missing G02 R4 contract marker: $item"
done

for item in \
 '#define ARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED UINT64_C(0x0000000030020001)' \
 '#define ARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX UINT64_C(0x0000000030020002)' \
 '#define ARBOR_VIEW_V1_G02_DOCTYPE_LEGACY_DISCOURAGED UINT64_C(0x0000000030020003)' \
 '#define ARBOR_VIEW_V1_G02_HEAD_TITLE_CARDINALITY UINT64_C(0x0000000030020006)'; do
    grep -Fxq "$item" "$header" || fail "missing retained/current G02 macro: $item"
done

grep -Fq 'g02_head_title_cardinality_check(' "$native" || fail 'G02 R4 title-cardinality evaluator missing'
grep -Fq 'diagnostic->rule_id = ARBOR_VIEW_V1_G02_HEAD_TITLE_CARDINALITY;' "$native" || fail 'G02 R4 diagnostic publication missing'
grep -Fq 'facts->dom_head_title_child_count == 1u' "$native" || fail 'G02 R4 DOM head-title cardinality decision missing'
grep -Fq 'facts->source_second_title_start_tag_offset' "$native" || fail 'G02 R4 duplicate source anchor missing'
! grep -Fq 'ARBOR_VIEW_V1_G02_HEAD_TITLE_CARDINALITY' "$adapter" || fail 'G02 R4 semantic rule leaked into Lexbor adapter'

active_symbols=$(grep -ERho 'ARBOR_VIEW_V1_G02_[A-Z0-9_]+' "$header" "$native" | LC_ALL=C sort -u)
required_symbols=$'ARBOR_VIEW_V1_G02_DOCTYPE_LEGACY_DISCOURAGED\nARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED\nARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX\nARBOR_VIEW_V1_G02_HEAD_TITLE_CARDINALITY'
allowed_symbols=$'ARBOR_VIEW_V1_G02_BODY_SINGLETON\nARBOR_VIEW_V1_G02_DOCTYPE_LEGACY_DISCOURAGED\nARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED\nARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX\nARBOR_VIEW_V1_G02_HEAD_BASE_CARDINALITY\nARBOR_VIEW_V1_G02_HEAD_TITLE_CARDINALITY'
while IFS= read -r symbol; do
    [[ -n "$symbol" ]] || continue
    grep -Fxq "$symbol" <<< "$active_symbols" || fail "retained G02 R4 semantic symbol disappeared: $symbol"
done <<< "$required_symbols"
while IFS= read -r symbol; do
    [[ -n "$symbol" ]] || continue
    grep -Fxq "$symbol" <<< "$allowed_symbols" || fail "unexpected active G02 semantic symbol under retained R4 contract: $symbol"
done <<< "$active_symbols"
! grep -ERq '0x000000003002000[45]' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'retired G02 rule ID reused'
! grep -ERq 'ARBOR_VIEW_V1_G0[3-6]_' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G03-G06 enforcement appeared during G02 R4'

count=$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)
[[ "$count" -eq 11 ]] || fail "G02 R4 changed production VIEW API count: $count"
! grep -Rq 'lxb_' include/arborcore/view.h || fail 'Lexbor type leaked into production VIEW header'
! grep -Fq 'lxb_' "$header" || fail 'Lexbor type leaked across private Arborcore header'

grep -Fq '## V1N1 G02 R4: standalone head/title cardinality' "$doc"
grep -Fq 'implements **4 of 6**' "$doc"
grep -Fq 'dom_head_title_child_count' "$doc"

echo 'VIEW0_V1N1_G02_R4_PUBLIC_FUNCTION_COUNT=0'
echo 'VIEW0_TOTAL_PUBLIC_FUNCTION_COUNT=11'
echo 'VIEW0_V1N1_G02_R4_RULE_ID=0x0000000030020006'
echo 'VIEW0_V1N1_G02_R4_EXTENSION_AWARE_RETENTION=YES'
echo 'PASS: G02 R4 frozen standalone title-cardinality and C0-fact semantics retained under higher G02 extension'
