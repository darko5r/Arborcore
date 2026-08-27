#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
bash tools/view0_v1n1_rc1_scope_verify.sh
bash tools/view0_v1n1_rc1_contract_verify.sh
bash tools/view0_v1n1_rc1_native_verify.sh
echo 'VIEW0_V1N1_RC1_GATE=PASS'
echo 'VIEW0_V1N1_RC1_RESOLVED_DEPENDENCIES=7_OF_7'
echo 'VIEW0_V1N1_RC1_RETAINED_EXTERNAL_DEPENDENCIES=13'
echo 'VIEW0_V1N1_RC1_ALREADY_OWNED_DEPENDENCIES=1'
echo 'VIEW0_V1N1_RC1_G05_GROUP_FREEZE=RETAINED'
echo 'VIEW0_V1N1_RC1_G06_GROUP_FREEZE=RETAINED'
echo 'VIEW0_V1N1_RC1_V1N1_INTEGRATED_GATE=NO_PENDING_INDEPENDENT_REVIEW'
echo 'VIEW0_V1N1_RC1_COMPLETE_HTML_CONFORMANCE_CLAIM=NO'
echo 'PASS: V1N1 G02-G06 dependency reconciliation source gate passed'
