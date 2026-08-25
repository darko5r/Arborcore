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

bash tools/view0_v1n1_g02_r5_contract_verify.sh >/dev/null

grep -Fxq 'ARBORCORE_VIEW_CORE_VERSION=0.1-VIEW0-V1N1-G02-R6' "$contract" || fail 'current contract is not exact G02 R6'
for item in \
 'VIEW0_V1N1_G02_R6_CONTRACT_REVISION=0.1-VIEW0-V1N1-G02-R6' \
 'VIEW0_V1N1_G02_R6_SCOPE=BODY_SINGLETON_OVER_RETAINED_R5' \
 'VIEW0_V1N1_G02_R6_RULE_ID=0x0000000030020008' \
 'VIEW0_V1N1_G02_R6_RULE_SYMBOL=ARBOR_VIEW_V1_G02_BODY_SINGLETON' \
 'VIEW0_V1N1_G02_R6_GROUP=G02' \
 'VIEW0_V1N1_G02_R6_SOURCE_LOCATOR=GENERAL_G02_BODY_ELEMENT_WHATWG_LINES_19393_19505' \
 'VIEW0_V1N1_G02_R6_SOURCE_FINGERPRINT_SHA256=19db797aeba61408d60af08b84a74c4b443efbce418c35021c309deb6a564908' \
 'VIEW0_V1N1_G02_R6_MATRIX_SHA256=2e6ce586cbaf09853be0a2b85bac813e4a959c6cae4f3195358ca2666c33ff94' \
 'VIEW0_V1N1_G02_R6_SEVERITY=ERROR' \
 'VIEW0_V1N1_G02_R6_REQUIRED_LOGICAL_BODY_COUNT=EXACTLY_ONE' \
 'VIEW0_V1N1_G02_R6_LEGAL_OMITTED_BODY_START_TAG=CONFORMING_IF_PARSER_IMPLIES_ONE_LOGICAL_BODY' \
 'VIEW0_V1N1_G02_R6_DECISION=DOM_BODY_SINGLETON_PLUS_AUTHORED_DUPLICATE_BODY_TOKEN_EVIDENCE' \
 'VIEW0_V1N1_G02_R6_MISSING_LOGICAL_BODY_DIAGNOSTIC_ANCHOR=DOCUMENT_START_BYTE_0_LENGTH_0' \
 'VIEW0_V1N1_G02_R6_DUPLICATE_BODY_DIAGNOSTIC_ANCHOR=SECOND_AUTHORED_BODY_START_TAG_NAME_BYTE_RANGE' \
 'VIEW0_V1N1_G02_R6_DUPLICATE_BODY_ANCHOR_LENGTH=4' \
 'VIEW0_V1N1_G02_R6_PARSER_REPAIR_POLICY=DUPLICATE_AUTHORED_BODY_TAGS_REMAIN_NONCONFORMING_WHEN_DOM_REPAIRS_TO_ONE_BODY' \
 'VIEW0_V1N1_G02_R6_MISSING_BODY_POLICY=ZERO_FINAL_HTML_BODY_CHILDREN_IS_AUTHORING_ERROR' \
 'VIEW0_V1N1_G02_R6_BODY_SECOND_ELEMENT_CONTEXT_OWNERSHIP=G03_ELEMENT_CONTEXT_AND_CONTENT_MODEL' \
 'VIEW0_V1N1_G02_R6_PARSE_DIAGNOSTICS_PRESERVED=YES' \
 'VIEW0_V1N1_G02_R6_DUPLICATE_BODY_PARSER_AND_AUTHORING_DIAGNOSTICS_COEXIST=YES' \
 'VIEW0_V1N1_G02_R6_PARSE_CLEAN_FLAG_MEANS_PARSER_CLEAN_NOT_DIAGNOSTIC_FREE' \
 'VIEW0_V1N1_G02_R6_EMPTY_DOCUMENT_BODY_POLICY=NO_R6_DIAGNOSTIC_PARSER_IMPLIED_LOGICAL_BODY_RETAINED' \
 'VIEW0_V1N1_G02_R6_CAPACITY_FAILURE_ATOMICITY=RETAINED_TWO_PASS_MEASURE_THEN_PUBLISH' \
 'VIEW0_V1N1_G02_R6_C0_FACT_LAYOUT_CHANGE=NO' \
 'VIEW0_V1N1_G02_R6_C0_FACT_SIZE_X86_64=184' \
 'VIEW0_V1N1_G02_R6_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO' \
 'VIEW0_V1N1_G02_R6_PUBLIC_FUNCTION_COUNT=0' \
 'VIEW0_V1N1_G02_R6_TOTAL_PUBLIC_FUNCTION_COUNT=11' \
 'VIEW0_V1N1_G02_R6_RUNTIME_REQUEST_PATH=NO' \
 'VIEW0_V1N1_G02_R6_G02_RULES_IMPLEMENTED=6_OF_6' \
 'VIEW0_V1N1_G02_R6_G02_GROUP_IMPLEMENTATION_COMPLETE=YES' \
 'VIEW0_V1N1_G02_R6_G03_G06_ENFORCEMENT=ZERO' \
 'VIEW0_V1N1_G02_R6_RETIRED_RULE_IDS_REUSED=NO' \
 'VIEW0_V1N1_G02_R6_COMPLETE_HTML_CONFORMANCE_CLAIM=NO' \
 'VIEW0_V1N1_G02_R6_JAVA_OR_VNU_ACTIVE_DEPENDENCY=NO' \
 'VIEW0_V1N1_G02_R6_DATABASE_DEPENDENCY=NONE' \
 'VIEW0_V1N1_G02_R6_R_DEPENDENCY=NONE'; do
    grep -Fxq "$item" "$contract" || fail "missing G02 R6 contract marker: $item"
done

for item in \
 '#define ARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED UINT64_C(0x0000000030020001)' \
 '#define ARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX UINT64_C(0x0000000030020002)' \
 '#define ARBOR_VIEW_V1_G02_DOCTYPE_LEGACY_DISCOURAGED UINT64_C(0x0000000030020003)' \
 '#define ARBOR_VIEW_V1_G02_HEAD_TITLE_CARDINALITY UINT64_C(0x0000000030020006)' \
 '#define ARBOR_VIEW_V1_G02_HEAD_BASE_CARDINALITY UINT64_C(0x0000000030020007)' \
 '#define ARBOR_VIEW_V1_G02_BODY_SINGLETON UINT64_C(0x0000000030020008)'; do
    grep -Fxq "$item" "$header" || fail "missing G02 macro: $item"
done

grep -Fq 'g02_body_singleton_check(' "$native" || fail 'G02 R6 body-singleton evaluator missing'
grep -Fq 'diagnostic->rule_id = ARBOR_VIEW_V1_G02_BODY_SINGLETON;' "$native" || fail 'G02 R6 diagnostic publication missing'
grep -Fq 'facts->source_body_start_tag_count > 1u' "$native" || fail 'G02 R6 authored duplicate-body decision missing'
grep -Fq 'facts->source_second_body_start_tag_offset' "$native" || fail 'G02 R6 second-body source anchor missing'
grep -Fq 'facts->dom_html_body_element_count == 1u' "$native" || fail 'G02 R6 logical body acceptance missing'
grep -Fq 'facts->dom_html_body_element_count == 0u' "$native" || fail 'G02 R6 missing logical body decision missing'
! grep -Fq 'ARBOR_VIEW_V1_G02_BODY_SINGLETON' "$adapter" || fail 'G02 R6 semantic rule leaked into Lexbor adapter'

active_symbols=$(grep -ERho 'ARBOR_VIEW_V1_G02_[A-Z0-9_]+' "$header" "$native" | LC_ALL=C sort -u)
required_symbols=$'ARBOR_VIEW_V1_G02_BODY_SINGLETON\nARBOR_VIEW_V1_G02_DOCTYPE_LEGACY_DISCOURAGED\nARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED\nARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX\nARBOR_VIEW_V1_G02_HEAD_BASE_CARDINALITY\nARBOR_VIEW_V1_G02_HEAD_TITLE_CARDINALITY'
[[ "$active_symbols" == "$required_symbols" ]] || fail "unexpected active G02 R6 semantic symbols: $active_symbols"
! grep -ERq '0x000000003002000[45]' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'retired G02 rule ID reused'
! grep -ERq 'ARBOR_VIEW_V1_G0[3-6]_' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G03-G06 enforcement appeared during G02 R6'

count=$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)
[[ "$count" -eq 11 ]] || fail "G02 R6 changed production VIEW API count: $count"
! grep -Rq 'lxb_' include/arborcore/view.h || fail 'Lexbor type leaked into production VIEW header'
! grep -Fq 'lxb_' "$header" || fail 'Lexbor type leaked across private Arborcore header'

grep -Fq '## V1N1 G02 R6: logical body singleton and parser-repair provenance' "$doc"
grep -Fq 'implements **6 of 6**' "$doc"
grep -Fq 'source_second_body_start_tag_offset' "$doc"

echo 'VIEW0_V1N1_G02_R6_PUBLIC_FUNCTION_COUNT=0'
echo 'VIEW0_TOTAL_PUBLIC_FUNCTION_COUNT=11'
echo 'VIEW0_V1N1_G02_R6_RULE_ID=0x0000000030020008'
echo 'VIEW0_V1N1_G02_R6_G02_RULES_IMPLEMENTED=6_OF_6'
echo 'PASS: G02 R6 frozen logical-body singleton, source/DOM repair evidence and complete-G02 no-scope-creep contract'
