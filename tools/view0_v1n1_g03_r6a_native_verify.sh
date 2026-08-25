#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
fail(){ echo "FAIL: $*" >&2; exit 1; }
echo '### RETAINED R5A NATIVE / C0-SR1 / R1-R4 / G02 / ANALYZER / STACK / CLI'
bash tools/view0_v1n1_g03_r5a_native_verify.sh
echo '### R6A RETAINED-OWNER FUNCTIONAL / ADVERSARIAL'
make -s view0-v1n1-g03-r6a-test
make -s view0-v1n1-g03-r6a-adversarial-test
[[ -x build/view0-v1/native/g03-r6a-scalar-value-text-retention-test ]] || fail 'R6A functional binary missing'
[[ -x build/view0-v1/native/g03-r6a-scalar-value-text-retention-adversarial-test ]] || fail 'R6A adversarial binary missing'
echo 'VIEW0_V1N1_G03_R6A_NATIVE_OWNER_RETENTION=PASS'
echo 'PASS: G03 R6A native qualification retains existing owner diagnostics and adds no R6 diagnostic'
