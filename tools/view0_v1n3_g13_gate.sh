#!/usr/bin/env bash
set -euo pipefail
ROOT=${ARBORCORE_ROOT:-$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)}
cd "$ROOT"
bash tools/view0_v1n3_g13_scope_verify.sh
bash tools/view0_v1n3_g13_contract_verify.sh
bash tools/view0_v1n3_g13_native_verify.sh
echo "VIEW0_V1N3_G13_GATE=PASS"

