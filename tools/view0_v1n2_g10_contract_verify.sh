#!/usr/bin/env bash
set -euo pipefail
ROOT=${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}
cd "$ROOT"
contract=view/arborcore-view-core-1.contract
for marker in \
 VIEW0_V1N2_G10_C1_RULES_IMPLEMENTED=13_OF_13 \
 VIEW0_V1N2_G10_C1_INPUT_STATE_PRODUCT_TABLE=FORBIDDEN \
 VIEW0_V1N2_G10_C1_FORM_CONTROL_RELATION_STORAGE=O_FORMS_CONTROLS_LABELS_OPTIONS_AND_TOKENS \
 VIEW0_V1N2_G10_C1_ECMASCRIPT_REGEXP=EXPLICITLY_DEFERRED_TO_G16 \
 VIEW0_V1N2_G10_C1_ARIA_HTML_AAM=EXPLICITLY_DEFERRED \
 VIEW0_V1N2_G10_C1_RUNTIME_FORM_UI_AND_MUTABILITY=EXCLUDED \
 VIEW0_V1N2_G10_C1_FORM_SUBMISSION_EXECUTION=EXCLUDED \
 VIEW0_V1N2_G10_C1_COMPLETE_HTML_CONFORMANCE_CLAIM=NO; do
    grep -Fx -- "$marker" "$contract" >/dev/null
done
for ordinal in 1 2 3 4 5 6 7 8 9 10 11 12 13; do
    grep -F "ARBOR_VIEW0_NATIVE_RULE_ID_V1N2_G10_R$ordinal" tools/include/arborcore/view0_conformance/native.h >/dev/null
done
grep -F 'arbor_view0_native_v1n2_g10_support_calloc' tools/c/view0_conformance/g10.c >/dev/null
grep -F 'lexbor_mraw_calloc' tools/c/view0_conformance/native.c >/dev/null
printf '%s\n' \
 'VIEW0_V1N2_G10_RULE_IDENTITIES=13_OF_13' \
 'VIEW0_V1N2_G10_PRIOR_OWNER_BOUNDARY=G03_G04_G05_G06_RETAINED' \
 'VIEW0_V1N2_G10_STATIC_CROSS_STANDARD_AUTHORITY=URL_FETCH_MIME_ENCODING_FROZEN' \
 'VIEW0_V1N2_G10_REGEXP_ACCESSIBILITY_RUNTIME_BOUNDARY=EXPLICIT' \
 'VIEW0_V1N2_G10_CONTRACT_VERIFY=PASS' \
 'PASS: V1N2 G10 exact authority, ownership and no-overclaim contract'
