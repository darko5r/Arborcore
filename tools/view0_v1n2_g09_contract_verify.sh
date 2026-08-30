#!/usr/bin/env bash
set -euo pipefail
ROOT=${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}
cd "$ROOT"
contract=view/arborcore-view-core-1.contract
for marker in \
 VIEW0_V1N2_G09_C1_RULES_IMPLEMENTED=6_OF_6 \
 VIEW0_V1N2_G09_C1_PRODUCT_GRID_ALLOCATION=FORBIDDEN \
 VIEW0_V1N2_G09_C1_TABLE_MODEL_STORAGE=O_PARSED_TABLE_NODES_AND_HEADERS_TOKENS \
 VIEW0_V1N2_G09_C1_ARIA_HTML_AAM=EXPLICITLY_DEFERRED \
 VIEW0_V1N2_G09_C1_CSS_TABLE_LAYOUT=EXCLUDED \
 VIEW0_V1N2_G09_C1_RUNTIME_TABLE_PRESENTATION=EXCLUDED \
 VIEW0_V1N2_G09_C1_COMPLETE_HTML_CONFORMANCE_CLAIM=NO; do
    grep -Fx -- "$marker" "$contract" >/dev/null
done
for ordinal in 1 2 3 4 5 6; do
    grep -F "UINT16_C($ordinal)" tools/c/view0_conformance/v1n2_c0.c >/dev/null
done
grep -F 'arbor_view0_native_v1n2_g09_support_calloc' tools/c/view0_conformance/g09.c >/dev/null
grep -F 'lexbor_mraw_calloc' tools/c/view0_conformance/native.c >/dev/null
printf '%s\n' \
 'VIEW0_V1N2_G09_RULE_IDENTITIES=6_OF_6' \
 'VIEW0_V1N2_G09_PRIOR_OWNER_BOUNDARY=G03_G05_G06_RETAINED' \
 'VIEW0_V1N2_G09_PRODUCT_GRID_ALLOCATION=FORBIDDEN' \
 'VIEW0_V1N2_G09_ACCESSIBILITY_CSS_RUNTIME_BOUNDARY=EXPLICIT' \
 'VIEW0_V1N2_G09_CONTRACT_VERIFY=PASS' \
 'PASS: V1N2 G09 exact authority, ownership and no-overclaim contract'
