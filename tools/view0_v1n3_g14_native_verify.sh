#!/usr/bin/env bash
set -euo pipefail
ROOT=${ARBORCORE_ROOT:-$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)}
cd "$ROOT"
make view0-v1n3-g14-test
echo "VIEW0_V1N3_G14_NATIVE_VERIFY=PASS"

