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

bash tools/view0_v1n1_g02_r1_contract_verify.sh >/dev/null

grep -Eq '^ARBORCORE_VIEW_CORE_VERSION=0\.1-VIEW0-V1N1-G02-R[2-9][0-9]*$' "$contract" || fail 'current V1N1 G02 R2-or-higher contract version missing'
for item in \
 'VIEW0_V1N1_G02_R2_CONTRACT_REVISION=0.1-VIEW0-V1N1-G02-R2' \
 'VIEW0_V1N1_G02_R2_SCOPE=DOCTYPE_SYNTAX_ONLY_OVER_RETAINED_R1' \
 'VIEW0_V1N1_G02_R2_RULE_ID=0x0000000030020002' \
 'VIEW0_V1N1_G02_R2_RULE_SYMBOL=ARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX' \
 'VIEW0_V1N1_G02_R2_GROUP=G02' \
 'VIEW0_V1N1_G02_R2_SOURCE_LOCATOR=GENERAL_G02_DOCTYPE_WHATWG_LINES_138306_138357' \
 'VIEW0_V1N1_G02_R2_SOURCE_FINGERPRINT_SHA256=359f9af97934ed0ffa64a2c1376764e2e3adca53d6798caa7044bb834e511333' \
 'VIEW0_V1N1_G02_R2_MATRIX_SHA256=2e6ce586cbaf09853be0a2b85bac813e4a959c6cae4f3195358ca2666c33ff94' \
 'VIEW0_V1N1_G02_R2_SEVERITY=ERROR' \
 'VIEW0_V1N1_G02_R2_SHORT_FORM=ASCII_CI_DOCTYPE_ASCII_WS_PLUS_ASCII_CI_HTML_ASCII_WS_STAR_GT' \
 'VIEW0_V1N1_G02_R2_LEGACY_FORM=SHORT_PREFIX_PLUS_ASCII_WS_PLUS_ASCII_CI_SYSTEM_ASCII_WS_PLUS_MATCHED_QUOTED_ABOUT_LEGACY_COMPAT' \
 'VIEW0_V1N1_G02_R2_LEGACY_VALUE_CASE_SENSITIVE=YES' \
 'VIEW0_V1N1_G02_R2_ASCII_WHITESPACE=HT_LF_FF_CR_SPACE' \
 'VIEW0_V1N1_G02_R2_MISSING_DOCTYPE_OWNED_BY_R1=YES' \
 'VIEW0_V1N1_G02_R2_LEGACY_WARNING_OWNED_BY_R3_NOT_IMPLEMENTED=YES' \
 'VIEW0_V1N1_G02_R2_DIAGNOSTIC_ANCHOR=FIRST_NONCONFORMING_DOCTYPE_COMPONENT_BYTE_RANGE' \
 'VIEW0_V1N1_G02_R2_DIAGNOSTIC_ORIGIN=ARBORCORE_AUTHORING' \
 'VIEW0_V1N1_G02_R2_SOURCE_EVALUATION=BORROWED_INPUT_PLUS_C0_FIRST_DOCTYPE_SOURCE_ANCHOR' \
 'VIEW0_V1N1_G02_R2_LEXBOR_ACCEPTANCE_IS_AUTHORING_AUTHORITY=NO' \
 'VIEW0_V1N1_G02_R2_PARSE_DIAGNOSTICS_PRESERVED=YES' \
 'VIEW0_V1N1_G02_R2_CAPACITY_FAILURE_ATOMICITY=RETAINED_TWO_PASS_MEASURE_THEN_PUBLISH' \
 'VIEW0_V1N1_G02_R2_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO' \
 'VIEW0_V1N1_G02_R2_PUBLIC_FUNCTION_COUNT=0' \
 'VIEW0_V1N1_G02_R2_TOTAL_PUBLIC_FUNCTION_COUNT=11' \
 'VIEW0_V1N1_G02_R2_RUNTIME_REQUEST_PATH=NO' \
 'VIEW0_V1N1_G02_R2_G02_RULES_IMPLEMENTED=2_OF_6' \
 'VIEW0_V1N1_G02_R2_G03_G06_ENFORCEMENT=ZERO' \
 'VIEW0_V1N1_G02_R2_RETIRED_RULE_IDS_REUSED=NO' \
 'VIEW0_V1N1_G02_R2_COMPLETE_HTML_CONFORMANCE_CLAIM=NO' \
 'VIEW0_V1N1_G02_R2_JAVA_OR_VNU_ACTIVE_DEPENDENCY=NO' \
 'VIEW0_V1N1_G02_R2_DATABASE_DEPENDENCY=NONE' \
 'VIEW0_V1N1_G02_R2_R_DEPENDENCY=NONE'; do
    grep -Fxq "$item" "$contract" || fail "missing G02 R2 contract marker: $item"
done

grep -Fxq '#define ARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED UINT64_C(0x0000000030020001)' "$header" || fail 'retained G02 R1 rule macro missing'
grep -Fxq '#define ARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX UINT64_C(0x0000000030020002)' "$header" || fail 'stable G02 R2 rule macro missing'
grep -Fq 'diagnostic->rule_id = ARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX;' "$native" || fail 'G02 R2 rule publication missing'
grep -Fq 'g02_doctype_syntax_check(' "$native" || fail 'G02 R2 native syntax evaluator missing'
grep -Fq 'ascii_whitespace(' "$native" || fail 'G02 R2 ASCII-whitespace evaluator missing'
grep -Fq 'about:legacy-compat' "$native" || fail 'G02 R2 exact legacy literal missing'
! grep -Fq 'ARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX' "$adapter" || fail 'G02 R2 semantic rule leaked into Lexbor adapter'

grep -Fq 'ARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED' "$header" || fail 'retained G02 R1 rule symbol missing'
grep -Fq 'ARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX' "$header" || fail 'retained G02 R2 rule symbol missing'
grep -Fq 'ARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX' "$native" || fail 'retained G02 R2 implementation missing'
! grep -ERq '0x000000003002000[45]' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'retired G02 rule ID reused'
! grep -ERq 'ARBOR_VIEW_V1_G0[3-6]_' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G03-G06 enforcement appeared during retained G02 R2'

count=$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)
[[ "$count" -eq 11 ]] || fail "G02 R2 changed production VIEW API count: $count"
! grep -Rq 'lxb_' include/arborcore/view.h || fail 'Lexbor type leaked into production VIEW header'
! grep -Fq 'lxb_' "$header" || fail 'Lexbor type leaked across private Arborcore header'

grep -Fq '## V1N1 G02 R2: DOCTYPE authoring syntax' "$doc"
grep -Fq 'implements **2 of 6**' "$doc"
grep -Fq 'about:legacy-compat' "$doc"

echo 'VIEW0_V1N1_G02_R2_PUBLIC_FUNCTION_COUNT=0'
echo 'VIEW0_TOTAL_PUBLIC_FUNCTION_COUNT=11'
echo 'VIEW0_V1N1_G02_R2_RULE_ID=0x0000000030020002'
echo 'VIEW0_V1N1_G02_R2_EXTENSION_AWARE_RETENTION=YES'
echo 'PASS: G02 R2 exact frozen DOCTYPE grammar and rule ownership retained under higher G02 extension'
