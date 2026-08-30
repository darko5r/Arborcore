#!/usr/bin/env bash
set -euo pipefail
ROOT=${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}
cd "$ROOT"
echo '### VIEW0 V1N2 G08 — TWELVE EMBEDDED-CONTENT RULE IDENTITIES'
bash tools/view0_v1n2_g08_scope_verify.sh
bash tools/view0_v1n2_g08_contract_verify.sh
bash tools/view0_v1n2_g08_native_verify.sh
printf '%s\n' \
 'VIEW0_V1N2_G08_GATE=PASS' \
 'VIEW0_V1N2_G08_RULES_IMPLEMENTED=12_OF_12' \
 'VIEW0_V1N2_G08_G07_GROUP_FREEZE=RETAINED' \
 'VIEW0_V1N2_G08_C0_FOUNDATION=RETAINED' \
 'VIEW0_V1N2_G08_G09_G11=NOT_IMPLEMENTED' \
 'VIEW0_V1N2_G08_COMPLETE_HTML_CONFORMANCE_CLAIM=NO' \
 'PASS: VIEW0 V1N2 G08 source gate passed'
