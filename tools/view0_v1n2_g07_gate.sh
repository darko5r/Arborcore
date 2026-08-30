#!/usr/bin/env bash
set -euo pipefail

ROOT=${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}
cd "$ROOT"

echo '### VIEW0 V1N2 G07 — FIVE STATIC LINK-SEMANTICS RULES'
bash tools/view0_v1n2_g07_scope_verify.sh
bash tools/view0_v1n2_g07_contract_verify.sh
bash tools/view0_v1n2_g07_native_verify.sh
printf '%s\n' \
    'VIEW0_V1N2_G07_GATE=PASS' \
    'VIEW0_V1N2_G07_RULES_IMPLEMENTED=5_OF_5' \
    'VIEW0_V1N2_G07_C0_FOUNDATION=RETAINED' \
    'VIEW0_V1N2_G07_G08_G11=NOT_IMPLEMENTED' \
    'VIEW0_V1N2_G07_COMPLETE_HTML_CONFORMANCE_CLAIM=NO' \
    'PASS: VIEW0 V1N2 G07 source gate passed'
