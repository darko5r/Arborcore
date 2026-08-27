#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
echo '### VIEW0 V1N1 G05 C0-SR1 — INPUT-STATE AUTHORITY CORRECTION GATE'
bash tools/view0_v1n1_g05_c0_sr1_scope_verify.sh
bash tools/view0_v1n1_g05_c0_sr1_contract_verify.sh
bash tools/view0_v1n1_g05_c0_sr1_native_verify.sh
git diff --check
echo 'VIEW0_V1N1_G05_C0_SR1_GATE=PASS'
echo 'VIEW0_V1N1_G05_C0_SR1_R1_IMPLEMENTED=YES'
echo 'VIEW0_V1N1_G05_C0_SR1_R2_IMPLEMENTED=YES'
echo 'VIEW0_V1N1_G05_C0_SR1_R3_IMPLEMENTED=NO'
echo 'VIEW0_V1N1_G05_C0_SR1_R4_IMPLEMENTED=NO'
echo 'VIEW0_V1N1_G05_C0_SR1_SEMANTICS_CHANGE=FOUNDATION_METADATA_ONLY_R1_R2_NO_CHANGE'
echo 'VIEW0_V1N1_G05_C0_SR1_COMPLETE_HTML_CONFORMANCE_CLAIM=NO'
echo 'PASS: G05 C0-SR1 authority correction gate passed'
