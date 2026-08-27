#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
echo '### VIEW0 V1N1 G05 R2A — FULL GATE'
bash tools/view0_v1n1_g05_r2a_scope_verify.sh
bash tools/view0_v1n1_g05_r2a_contract_verify.sh
bash tools/view0_v1n1_g05_r2a_native_verify.sh
git diff --check
echo 'VIEW0_V1N1_G05_R2A_GATE=PASS'; echo 'VIEW0_V1N1_G05_R2A_R1_IMPLEMENTED=YES'; echo 'VIEW0_V1N1_G05_R2A_R2_IMPLEMENTED=YES'; echo 'VIEW0_V1N1_G05_R2A_R3_IMPLEMENTED=NO'; echo 'VIEW0_V1N1_G05_R2A_R4_IMPLEMENTED=NO'; echo 'VIEW0_V1N1_G05_R2A_G05_GROUP_FREEZE=NO'; echo 'VIEW0_V1N1_G05_R2A_COMPLETE_HTML_CONFORMANCE_CLAIM=NO'; echo 'PASS: G05 R2A full gate passed'
