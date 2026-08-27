#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
bash tools/view0_v1n1_g06_wave_scope_verify.sh
bash tools/view0_v1n1_g06_wave_contract_verify.sh
bash tools/view0_v1n1_g06_wave_native_verify.sh
echo 'VIEW0_V1N1_G06_WAVE_GATE=PASS'
echo 'VIEW0_V1N1_G06_WAVE_RULES_IMPLEMENTED=17_OF_17_WITH_R15_ZERO_CONSUMER'
echo 'VIEW0_V1N1_G06_WAVE_G05_GROUP_FREEZE=RETAINED'
echo 'VIEW0_V1N1_G06_WAVE_G06_GROUP_FREEZE=NO_PENDING_INDEPENDENT_REVIEW'
echo 'VIEW0_V1N1_G06_WAVE_COMPLETE_HTML_CONFORMANCE_CLAIM=NO'
echo 'PASS: G06 R1-R17 internally checkpointed full source gate passed'
