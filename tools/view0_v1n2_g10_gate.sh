#!/usr/bin/env bash
set -euo pipefail
ROOT=${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}
cd "$ROOT"
echo '### VIEW0 V1N2 G10 — THIRTEEN STATIC FORM-SEMANTICS RULES'
bash tools/view0_v1n2_g10_scope_verify.sh
bash tools/view0_v1n2_g10_contract_verify.sh
bash tools/view0_v1n2_g10_native_verify.sh
printf '%s\n' \
 'VIEW0_V1N2_G10_GATE=PASS' \
 'VIEW0_V1N2_G10_RULES_IMPLEMENTED=13_OF_13' \
 'VIEW0_V1N2_G10_G09_GROUP_FREEZE=RETAINED' \
 'VIEW0_V1N2_G10_C0_FOUNDATION=RETAINED' \
 'VIEW0_V1N2_G10_G11=NOT_IMPLEMENTED' \
 'VIEW0_V1N2_G10_COMPLETE_HTML_CONFORMANCE_CLAIM=NO' \
 'PASS: VIEW0 V1N2 G10 source gate passed'
