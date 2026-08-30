#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
bash tools/view0_v1n2_c0_scope_verify.sh
bash tools/view0_v1n2_c0_contract_verify.sh
bash tools/view0_v1n2_c0_native_verify.sh
echo 'VIEW0_V1N2_C0_GATE=PASS'
echo 'VIEW0_V1N2_C0_DIAGNOSTIC_RULES_IMPLEMENTED=ZERO'
echo 'VIEW0_V1N2_C0_COMPLETE_HTML_CONFORMANCE_CLAIM=NO'
echo 'PASS: V1N2 C0 full zero-diagnostic foundation gate passed'
