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

bash tools/view0_v1n1_c0_contract_verify.sh >/dev/null

grep -Eq '^ARBORCORE_VIEW_CORE_VERSION=0\.1-VIEW0-V1N1-' "$contract" || fail 'current V1N1 contract version missing'
for item in \
 'VIEW0_V1N1_G02_R1_CONTRACT_REVISION=0.1-VIEW0-V1N1-G02-R1' \
 'VIEW0_V1N1_G02_R1_SCOPE=DOCTYPE_REQUIRED_ONLY' \
 'VIEW0_V1N1_G02_R1_RULE_ID=0x0000000030020001' \
 'VIEW0_V1N1_G02_R1_RULE_SYMBOL=ARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED' \
 'VIEW0_V1N1_G02_R1_GROUP=G02' \
 'VIEW0_V1N1_G02_R1_SOURCE_LOCATOR=GENERAL_G02_DOCTYPE_WHATWG_LINES_138306_138357' \
 'VIEW0_V1N1_G02_R1_SOURCE_FINGERPRINT_SHA256=359f9af97934ed0ffa64a2c1376764e2e3adca53d6798caa7044bb834e511333' \
 'VIEW0_V1N1_G02_R1_MATRIX_SHA256=2e6ce586cbaf09853be0a2b85bac813e4a959c6cae4f3195358ca2666c33ff94' \
 'VIEW0_V1N1_G02_R1_SEVERITY=ERROR' \
 'VIEW0_V1N1_G02_R1_DIAGNOSTIC_ANCHOR=DOCUMENT_START_BYTE_0_LENGTH_0' \
 'VIEW0_V1N1_G02_R1_DIAGNOSTIC_ORIGIN=ARBORCORE_AUTHORING' \
 'VIEW0_V1N1_G02_R1_STANDALONE_DOCUMENT_MODE=YES' \
 'VIEW0_V1N1_G02_R1_MISSING_DOCTYPE_DECISION=DOM_DOCUMENT_DOCTYPE_NODE_COUNT_ZERO' \
 'VIEW0_V1N1_G02_R1_PARSE_DIAGNOSTICS_PRESERVED=YES' \
 'VIEW0_V1N1_G02_R1_PARSE_CLEAN_FLAG_MEANS_PARSER_CLEAN_NOT_DIAGNOSTIC_FREE' \
 'VIEW0_V1N1_G02_R1_CAPACITY_FAILURE_ATOMICITY=TWO_PASS_MEASURE_THEN_PUBLISH' \
 'VIEW0_V1N1_G02_R1_LEXBOR_MEASUREMENT_PASS=PRIVATE_NO_DIAGNOSTIC_PUBLICATION' \
 'VIEW0_V1N1_G02_R1_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO' \
 'VIEW0_V1N1_G02_R1_PUBLIC_FUNCTION_COUNT=0' \
 'VIEW0_V1N1_G02_R1_TOTAL_PUBLIC_FUNCTION_COUNT=11' \
 'VIEW0_V1N1_G02_R1_RUNTIME_REQUEST_PATH=NO' \
 'VIEW0_V1N1_G02_R1_G02_RULES_IMPLEMENTED=1_OF_6' \
 'VIEW0_V1N1_G02_R1_G03_G06_ENFORCEMENT=ZERO' \
 'VIEW0_V1N1_G02_R1_RETIRED_RULE_IDS_REUSED=NO' \
 'VIEW0_V1N1_G02_R1_COMPLETE_HTML_CONFORMANCE_CLAIM=NO' \
 'VIEW0_V1N1_G02_R1_JAVA_OR_VNU_ACTIVE_DEPENDENCY=NO' \
 'VIEW0_V1N1_G02_R1_DATABASE_DEPENDENCY=NONE' \
 'VIEW0_V1N1_G02_R1_R_DEPENDENCY=NONE'; do
    grep -Fxq "$item" "$contract" || fail "missing G02 R1 contract marker: $item"
done

grep -Fxq '#define ARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED UINT64_C(0x0000000030020001)' "$header" || fail 'stable G02 R1 rule macro missing'
grep -Fq 'ARBOR_VIEW0_NATIVE_ORIGIN_AUTHORING = 4' "$header" || fail 'authoring diagnostic origin missing'
grep -Fq 'arbor_view0_native_lexbor_measure(' "$header" || fail 'private measurement boundary missing'
grep -Fq 'diagnostic->rule_id = ARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED;' "$native" || fail 'G02 R1 rule publication missing'
grep -Fq 'measured_facts.dom_doctype_node_count == 0u ? 1u : 0u' "$native" || fail 'G02 R1 document-doctype decision missing'
grep -Fq 'arbor_view0_native_lexbor_measure(' "$native" || fail 'G02 R1 measurement prepass missing'
! grep -Fq 'ARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED' "$adapter" || fail 'G02 semantic rule leaked into Lexbor adapter'

# R1 remains mandatory under admitted higher G02 extensions. Retired IDs and
# G03-G06 authoring rules remain absent from implementation files.
grep -Fq 'ARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED' "$header" || fail 'G02 R1 semantic symbol no longer retained'
grep -Fq 'ARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED' "$native" || fail 'G02 R1 implementation no longer retained'
! grep -ERq '0x000000003002000[45]' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'retired G02 rule ID reused'
! grep -ERq 'ARBOR_VIEW_V1_G0[3-6]_' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G03-G06 enforcement appeared during retained G02 R1'

count=$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)
[[ "$count" -eq 11 ]] || fail "G02 R1 changed production VIEW API count: $count"
! grep -Rq 'lxb_' include/arborcore/view.h || fail 'Lexbor type leaked into production VIEW header'
! grep -Fq 'lxb_' "$header" || fail 'Lexbor type leaked across private Arborcore header'

grep -Fq '## V1N1 G02 R1: required document DOCTYPE' "$doc"
grep -Fq 'implements **1 of 6** active G02 diagnostics' "$doc"
grep -Fq 'failure-atomicity contract' "$doc"

echo 'VIEW0_V1N1_G02_R1_PUBLIC_FUNCTION_COUNT=0'
echo 'VIEW0_TOTAL_PUBLIC_FUNCTION_COUNT=11'
echo 'VIEW0_V1N1_G02_R1_RULE_ID=0x0000000030020001'
echo 'VIEW0_V1N1_G02_R1_EXTENSION_AWARE_RETENTION=YES'
echo 'PASS: G02 R1 corrected-freeze rule identity and semantics retained under admitted higher G02 extension'
