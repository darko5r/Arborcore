#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
contract='view/arborcore-view-core-1.contract'
doc='docs/VIEW_CORE_VIEW0.md'
header='tools/include/arborcore/view0_conformance/native.h'
native='tools/c/view0_conformance/native.c'
main='tools/c/view0_conformance/main.c'
adapter='tools/c/view0_conformance/lexbor_adapter.c'

fail() { echo "FAIL: $*" >&2; exit 1; }

bash tools/view0_v1n1_g02_r2_contract_verify.sh >/dev/null

grep -Eq '^ARBORCORE_VIEW_CORE_VERSION=0\.1-VIEW0-V1N1-G02-R([3-9]|[1-9][0-9]+)$' "$contract" || fail 'current V1N1 G02 R3-or-higher contract version missing'
for item in \
 'VIEW0_V1N1_G02_R3_CONTRACT_REVISION=0.1-VIEW0-V1N1-G02-R3' \
 'VIEW0_V1N1_G02_R3_SCOPE=DOCTYPE_LEGACY_DISCOURAGED_WARNING_ONLY_OVER_RETAINED_R2' \
 'VIEW0_V1N1_G02_R3_RULE_ID=0x0000000030020003' \
 'VIEW0_V1N1_G02_R3_RULE_SYMBOL=ARBOR_VIEW_V1_G02_DOCTYPE_LEGACY_DISCOURAGED' \
 'VIEW0_V1N1_G02_R3_GROUP=G02' \
 'VIEW0_V1N1_G02_R3_SOURCE_LOCATOR=GENERAL_G02_DOCTYPE_WHATWG_LINES_138306_138357' \
 'VIEW0_V1N1_G02_R3_SOURCE_FINGERPRINT_SHA256=359f9af97934ed0ffa64a2c1376764e2e3adca53d6798caa7044bb834e511333' \
 'VIEW0_V1N1_G02_R3_MATRIX_SHA256=2e6ce586cbaf09853be0a2b85bac813e4a959c6cae4f3195358ca2666c33ff94' \
 'VIEW0_V1N1_G02_R3_SEVERITY=WARNING' \
 'VIEW0_V1N1_G02_R3_NORMATIVE_SHOULD=LEGACY_STRING_SHOULD_NOT_BE_USED_UNLESS_GENERATOR_CANNOT_OUTPUT_SHORT_FORM' \
 'VIEW0_V1N1_G02_R3_TRIGGER=R2_SYNTAX_VALID_LEGACY_COMPAT_FORM_ONLY' \
 'VIEW0_V1N1_G02_R3_LEGACY_VALUE=about:legacy-compat' \
 'VIEW0_V1N1_G02_R3_DIAGNOSTIC_ANCHOR=LEGACY_SYSTEM_ID_CONTENT_BYTE_RANGE' \
 'VIEW0_V1N1_G02_R3_DIAGNOSTIC_ORIGIN=ARBORCORE_AUTHORING' \
 'VIEW0_V1N1_G02_R3_R2_SYNTAX_ERROR_SUPPRESSES_WARNING=YES' \
 'VIEW0_V1N1_G02_R3_PARSE_DIAGNOSTICS_PRESERVED=YES' \
 'VIEW0_V1N1_G02_R3_PARSE_CLEAN_FLAG_UNCHANGED_BY_WARNING=YES' \
 'VIEW0_V1N1_G02_R3_WARNING_ONLY_CLI_EXIT_STATUS=ZERO_WHEN_NO_ERROR_DIAGNOSTICS' \
 'VIEW0_V1N1_G02_R3_CAPACITY_FAILURE_ATOMICITY=RETAINED_TWO_PASS_MEASURE_THEN_PUBLISH' \
 'VIEW0_V1N1_G02_R3_DIRECT_ARBORCORE_HEAP_ALLOCATION=ZERO' \
 'VIEW0_V1N1_G02_R3_PUBLIC_FUNCTION_COUNT=0' \
 'VIEW0_V1N1_G02_R3_TOTAL_PUBLIC_FUNCTION_COUNT=11' \
 'VIEW0_V1N1_G02_R3_RUNTIME_REQUEST_PATH=NO' \
 'VIEW0_V1N1_G02_R3_G02_RULES_IMPLEMENTED=3_OF_6' \
 'VIEW0_V1N1_G02_R3_G03_G06_ENFORCEMENT=ZERO' \
 'VIEW0_V1N1_G02_R3_RETIRED_RULE_IDS_REUSED=NO' \
 'VIEW0_V1N1_G02_R3_COMPLETE_HTML_CONFORMANCE_CLAIM=NO' \
 'VIEW0_V1N1_G02_R3_JAVA_OR_VNU_ACTIVE_DEPENDENCY=NO' \
 'VIEW0_V1N1_G02_R3_DATABASE_DEPENDENCY=NONE' \
 'VIEW0_V1N1_G02_R3_R_DEPENDENCY=NONE'; do
    grep -Fxq "$item" "$contract" || fail "missing G02 R3 contract marker: $item"
done

grep -Fxq '#define ARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED UINT64_C(0x0000000030020001)' "$header" || fail 'retained G02 R1 macro missing'
grep -Fxq '#define ARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX UINT64_C(0x0000000030020002)' "$header" || fail 'retained G02 R2 macro missing'
grep -Fxq '#define ARBOR_VIEW_V1_G02_DOCTYPE_LEGACY_DISCOURAGED UINT64_C(0x0000000030020003)' "$header" || fail 'stable G02 R3 rule macro missing'
grep -Fq 'g02_doctype_legacy_compat_anchor(' "$native" || fail 'G02 R3 legacy-form evidence predicate missing'
grep -Fq 'diagnostic->rule_id = ARBOR_VIEW_V1_G02_DOCTYPE_LEGACY_DISCOURAGED;' "$native" || fail 'G02 R3 diagnostic publication missing'
grep -Fq 'ARBOR_VIEW0_NATIVE_SEVERITY_WARNING' "$native" || fail 'G02 R3 WARNING severity missing'
grep -Fq 'const int exit_code = has_error != 0 ? 1 : 0;' "$main" || fail 'warning-only CLI exit policy missing'
! grep -Fq 'ARBOR_VIEW_V1_G02_DOCTYPE_LEGACY_DISCOURAGED' "$adapter" || fail 'G02 R3 semantic rule leaked into Lexbor adapter'

for symbol in \
 ARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED \
 ARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX \
 ARBOR_VIEW_V1_G02_DOCTYPE_LEGACY_DISCOURAGED; do
    grep -Fq "$symbol" "$header" || fail "retained G02 R3 symbol missing from header: $symbol"
    grep -Fq "$symbol" "$native" || fail "retained G02 R3 implementation missing: $symbol"
done
! grep -ERq '0x000000003002000[45]' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'retired G02 rule ID reused'
! grep -ERq 'ARBOR_VIEW_V1_G0[3-6]_' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G03-G06 enforcement appeared during G02 R3'

count=$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)
[[ "$count" -eq 11 ]] || fail "G02 R3 changed production VIEW API count: $count"
! grep -Rq 'lxb_' include/arborcore/view.h || fail 'Lexbor type leaked into production VIEW header'
! grep -Fq 'lxb_' "$header" || fail 'Lexbor type leaked across private Arborcore header'

grep -Fq '## V1N1 G02 R3: legacy DOCTYPE compatibility warning' "$doc"
grep -Fq 'implements **3 of 6** active G02 diagnostics' "$doc"
grep -Fq 'warning-only' "$doc"

echo 'VIEW0_V1N1_G02_R3_PUBLIC_FUNCTION_COUNT=0'
echo 'VIEW0_TOTAL_PUBLIC_FUNCTION_COUNT=11'
echo 'VIEW0_V1N1_G02_R3_RULE_ID=0x0000000030020003'
echo 'VIEW0_V1N1_G02_R3_EXTENSION_AWARE_RETENTION=YES'
echo 'PASS: G02 R3 frozen author SHOULD, WARNING semantics and rule ownership retained under higher G02 extension'
