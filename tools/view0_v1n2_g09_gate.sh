#!/usr/bin/env bash
set -euo pipefail
ROOT=${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}
cd "$ROOT"
echo '### VIEW0 V1N2 G09 — SIX STATIC TABLE-SEMANTICS RULES'
bash tools/view0_v1n2_g09_scope_verify.sh
bash tools/view0_v1n2_g09_contract_verify.sh
bash tools/view0_v1n2_g09_native_verify.sh
printf '%s\n' \
 'VIEW0_V1N2_G09_GATE=PASS' \
 'VIEW0_V1N2_G09_RULES_IMPLEMENTED=6_OF_6' \
 'VIEW0_V1N2_G09_G08_GROUP_FREEZE=RETAINED' \
 'VIEW0_V1N2_G09_C0_FOUNDATION=RETAINED' \
 'VIEW0_V1N2_G09_G10_G11=NOT_IMPLEMENTED' \
 'VIEW0_V1N2_G09_COMPLETE_HTML_CONFORMANCE_CLAIM=NO' \
 'PASS: VIEW0 V1N2 G09 source gate passed'
