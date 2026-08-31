#!/usr/bin/env bash
set -euo pipefail
ROOT=${ARBORCORE_ROOT:-$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)}
cd "$ROOT"
bash tools/view0_v1n3_c0_scope_verify.sh
bash tools/view0_v1n3_c0_contract_verify.sh
bash tools/view0_v1n3_c0_native_verify.sh
for gate in ecma g12 g13 g14 g15 g16; do bash "tools/view0_v1n3_${gate}_gate.sh"; done
echo "VIEW0_V1N3_C1_GATE=PASS"
echo "VIEW0_V1N3_C1_COMPLETE_HTML_CONFORMANCE_CLAIM=NO"
echo "PASS: VIEW0 V1N3 C1 isolated candidate source gate"

