#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
echo '### VIEW0 V1N1 G05 R1A — RETAINED GATE'
bash tools/view0_v1n1_g05_r1a_scope_verify.sh
bash tools/view0_v1n1_g05_r1a_contract_verify.sh
bash tools/view0_v1n1_g05_r1a_native_verify.sh
git diff --check
version=$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' view/arborcore-view-core-1.contract|head -1)
echo 'VIEW0_V1N1_G05_R1A_GATE=PASS'; echo 'VIEW0_V1N1_G05_R1A_R1_IMPLEMENTED=YES'
if [[ "$version" == *R2A* ]]; then echo 'VIEW0_V1N1_G05_R1A_RETAINED_UNDER_R2=PASS'; else echo 'VIEW0_V1N1_G05_R1A_R2_IMPLEMENTED=NO'; fi
echo 'VIEW0_V1N1_G05_R1A_G05_GROUP_FREEZE=NO'; echo 'VIEW0_V1N1_G05_R1A_COMPLETE_HTML_CONFORMANCE_CLAIM=NO'; echo 'PASS: G05 R1A retained gate passed'
