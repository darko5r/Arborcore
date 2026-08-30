#!/usr/bin/env bash
set -euo pipefail
ROOT=${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}
cd "$ROOT"
contract=view/arborcore-view-core-1.contract
for marker in \
 VIEW0_V1N2_G11_C1_RULE_IDENTITIES_IMPLEMENTED=2_OF_2 \
 VIEW0_V1N2_G11_C1_OWNED_DIAGNOSTIC_IDENTITIES=1 \
 VIEW0_V1N2_G11_C1_DETAILS_NAME_COMPARISON=EXACT_CASE_SENSITIVE \
 VIEW0_V1N2_G11_C1_G05_DIALOG_TABINDEX_OWNER=RETAINED_NO_DUPLICATE \
 VIEW0_V1N2_G11_C1_G06_OPEN_CLOSEDBY_SYNTAX_OWNER=RETAINED_NO_DUPLICATE \
 VIEW0_V1N2_G11_C1_DIALOG_POPOVER_ATTRIBUTE=ALLOWED_NO_RUNTIME_SHOWING_STATE_INFERENCE \
 VIEW0_V1N2_G11_C1_RUNTIME_TOGGLE_FOCUS_CLOSE_WATCHER_INERT_ACTIVATION=EXCLUDED \
 VIEW0_V1N2_G11_C1_COMPLETE_HTML_CONFORMANCE_CLAIM=NO; do
    grep -Fx -- "$marker" "$contract" >/dev/null
done
for ordinal in 1 2; do
    grep -F "ARBOR_VIEW0_NATIVE_RULE_ID_V1N2_G11_R$ordinal" \
        tools/include/arborcore/view0_conformance/native.h >/dev/null
done
grep -F 'arbor_view0_native_v1n2_g11_support_calloc' \
    tools/c/view0_conformance/g11.c >/dev/null
grep -F 'arbor_view0_native_v1n2_anchor v1n2_anchors' \
    tools/c/view0_conformance/native.c >/dev/null
printf '%s\n' \
 'VIEW0_V1N2_G11_RULE_IDENTITIES=2_OF_2' \
 'VIEW0_V1N2_G11_DETAILS_STATIC_AUTHORING=IMPLEMENTED' \
 'VIEW0_V1N2_G11_DIALOG_PRIOR_OWNER_BOUNDARY=G05_G06_RETAINED' \
 'VIEW0_V1N2_G11_RUNTIME_INTERACTION_BOUNDARY=EXPLICIT' \
 'VIEW0_V1N2_G11_CONTRACT_VERIFY=PASS' \
 'PASS: V1N2 G11 exact authority, ownership and no-overclaim contract'
