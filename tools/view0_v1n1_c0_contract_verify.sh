#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
contract='view/arborcore-view-core-1.contract'
doc='docs/VIEW_CORE_VIEW0.md'
header='tools/include/arborcore/view0_conformance/native.h'

fail() { echo "FAIL: $*" >&2; exit 1; }

bash tools/view0_v1_contract_verify.sh >/dev/null

grep -Eq '^ARBORCORE_VIEW_CORE_VERSION=0\.1-VIEW0-V1N1-[A-Z0-9-]+$' "$contract" || fail 'current VIEW contract is not a V1N1 extension'
for item in \
 'VIEW0_V1N1_F1_R2_CORRECTED_FREEZE=ACCEPTED' \
 'VIEW0_V1N1_F1_R2_ACTIVE_RULE_COUNT=36' \
 'VIEW0_V1N1_F1_R2_G02_ACTIVE_RULE_COUNT=6' \
 'VIEW0_V1N1_F1_R2_RETIRED_RESERVED_RULE_ID_COUNT=2' \
 'VIEW0_V1N1_F1_R2_RETIRED_RULE_ID_1=0x0000000030020004' \
 'VIEW0_V1N1_F1_R2_RETIRED_RULE_ID_2=0x0000000030020005' \
 'VIEW0_V1N1_F1_R2_MATRIX_SHA256=2e6ce586cbaf09853be0a2b85bac813e4a959c6cae4f3195358ca2666c33ff94' \
 'VIEW0_V1N1_F1_R2_FREEZE_BUNDLE_SHA256=7ba406265319e4fc37f3bd93389867315ac83f097497875f4cd9f988a9dda5e4' \
 'VIEW0_V1N1_F1_R2_OPTIONAL_TAGS_SHA256=8f6299138f9229e56c84f4d64f66f4ae63bf7317871f05759ce61e4d7428a93b' \
 'VIEW0_V1N1_C0_CONTRACT_REVISION=0.1-VIEW0-V1N1-C0' \
 'VIEW0_V1N1_C0_SCOPE=PRIVATE_LEXBOR_INDEPENDENT_DOCUMENT_FACTS_FOUNDATION' \
 'VIEW0_V1N1_C0_PUBLIC_FUNCTION_COUNT=0' \
 'VIEW0_V1N1_C0_TOTAL_PUBLIC_FUNCTION_COUNT=11' \
 'VIEW0_V1N1_C0_RUNTIME_REQUEST_PATH=NO' \
 'VIEW0_V1N1_C0_AUTHORING_RULE_GROUPS_IMPLEMENTED=ZERO' \
 'VIEW0_V1N1_C0_G02_RULES_IMPLEMENTED=ZERO' \
 'VIEW0_V1N1_C0_COMPLETE_HTML_CONFORMANCE_CLAIM=NO_FOUNDATION_ONLY' \
 'VIEW0_V1N1_C0_DOCUMENT_FACT_SIZE_X86_64=184' \
 'VIEW0_V1N1_C0_DOCUMENT_FACT_STORAGE=CALLER_LOCAL_VALUE_DATA' \
 'VIEW0_V1N1_C0_DOCUMENT_FACT_POINTER_FIELDS=ZERO' \
 'VIEW0_V1N1_C0_INPUT_SLICE_REFERENCE=OFFSETS_AND_LENGTHS_ONLY' \
 'VIEW0_V1N1_C0_OUTPUT_ALIAS_POLICY=INPUT_DIAGNOSTICS_COUNTS_AND_FACTS_MUTUALLY_DISJOINT' \
 'VIEW0_V1N1_C0_SOURCE_OFFSET_SENTINEL=UINT64_MAX' \
 'VIEW0_V1N1_C0_RAW_SOURCE_TAG_AGGREGATES=EVIDENCE_NOT_RULE_OUTCOME' \
 'VIEW0_V1N1_C0_LEXBOR_TYPES_CROSS_PRIVATE_HEADER=NO' \
 'VIEW0_V1N1_C0_CALLBACK_CONTEXT_LIFETIME=SYNCHRONOUS_SINGLE_PARSE_CALL' \
 'VIEW0_V1N1_C0_CALLBACK_FORWARDING=ORIGINAL_TREE_CALLBACK_PRESERVED' \
 'VIEW0_V1N1_C0_FAILURE_ATOMICITY=PARSE_COUNTS_DOCUMENT_FACTS_AND_DIAGNOSTICS_UNPUBLISHED_ON_FAILURE' \
 'VIEW0_V1N1_C0_V1N0_REGRESSION_VERIFIER=EXTENSION_AWARE' \
 'VIEW0_V1N1_C0_EXTENSION_AWARE_RETENTION=YES' \
 'VIEW0_V1N1_C0_G02_ENFORCEMENT=NOT_YET' \
 'VIEW0_V1N1_C0_RETIRED_RULE_IDS_REUSED=NO' \
 'VIEW0_V1N1_C0_JAVA_OR_VNU_ACTIVE_DEPENDENCY=NO' \
 'VIEW0_V1N1_C0_PRODUCTION_VIEW_HEADER_CHANGE=NO' \
 'VIEW0_V1N1_C0_PRODUCTION_VIEW_SOURCE_CHANGE=NO' \
 'VIEW0_V1N1_C0_DATABASE_DEPENDENCY=NONE' \
 'VIEW0_V1N1_C0_R_DEPENDENCY=NONE'; do
    grep -Fxq "$item" "$contract" || fail "missing C0 contract marker: $item"
done

count=$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)
[[ "$count" -eq 11 ]] || fail "C0 changed production VIEW API count: $count"
! grep -Rq 'lxb_' include/arborcore/view.h || fail 'Lexbor type leaked into production VIEW header'
! grep -Fq 'lxb_' "$header" || fail 'Lexbor type leaked across private Arborcore header'

fact_field_count=$(awk '
    /typedef struct arbor_view0_native_document_facts/ { capture=1; next }
    capture && /} arbor_view0_native_document_facts;/ { capture=0 }
    capture && /^[[:space:]]+uint64_t / { count++ }
    END { print count+0 }
' "$header")
[[ "$fact_field_count" -eq 23 ]] || fail "document-facts field count drift: $fact_field_count"
if awk '
    /typedef struct arbor_view0_native_document_facts/ { capture=1; next }
    capture && /} arbor_view0_native_document_facts;/ { capture=0 }
    capture && /\*/ { found=1 }
    END { exit found ? 0 : 1 }
' "$header"; then
    fail 'document-facts structure contains a pointer field'
fi

if grep -ERq 'ARBOR_VIEW_V1_G02_|0x000000003002000[0-9A-Fa-f]' \
    tools/c/view0_conformance/lexbor_adapter.c \
    tests/c/view0_v1n1_c0_facts_test.c \
    tests/c/view0_v1n1_c0_facts_adversarial_test.c; then
    fail 'C0 facts adapter/tests must remain rule-semantic free under higher extension'
fi

grep -Fq '## V1N1 C0: private document-facts construction foundation' "$doc"
grep -Fq 'C0 does **not** implement those six G02 authoring rules.' "$doc"
grep -Fq 'Raw source-tag' "$doc"
grep -Fq '184 bytes' "$doc"

echo 'VIEW0_V1N1_C0_PUBLIC_FUNCTION_COUNT=0'
echo 'VIEW0_TOTAL_PUBLIC_FUNCTION_COUNT=11'
echo 'VIEW0_V1N1_C0_FACT_FIELD_COUNT=23'
echo 'VIEW0_V1N1_C0_EXTENSION_AWARE_RETENTION=YES'
echo 'PASS: C0 corrected-freeze identities and private facts boundary preserved under higher rule-layer extension'
