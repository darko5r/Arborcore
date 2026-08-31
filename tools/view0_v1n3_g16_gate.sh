#!/usr/bin/env bash
set -euo pipefail
ROOT=${ARBORCORE_ROOT:-$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)}
cd "$ROOT"
bash tools/view0_v1n3_g16_scope_verify.sh
bash tools/view0_v1n3_g16_contract_verify.sh
bash tools/view0_v1n3_g16_native_verify.sh
echo "VIEW0_V1N3_G16_GATE=PASS"

