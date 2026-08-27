#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
A1="${ARBOR_A1_BUNDLE:-/root/Downloads/Arborcore-VIEW0-V1-A1-wave-a-support-authority-candidate-1563a1cddfd0fae52a8042dc84543c302e4c07d9b3fe47dc541961b19d09d30e.tar.gz}"
C0_SR1="${ARBOR_G05_C0_SR1_BUNDLE:-/root/Downloads/Arborcore-VIEW0-V1N1-G05-C0-SR1-construction-result-candidate-a0a03c65bd7d23089df87ce45fe16445361606539fa62c07933dddb3c3d54201.tar.gz}"
echo '### VIEW0 V1N1 G05 R3A — CONDITIONAL ATTRIBUTE APPLICABILITY FULL GATE'
bash tools/view0_v1n1_g05_r3a_scope_verify.sh
ARBOR_A1_BUNDLE="$A1" bash tools/view0_v1n1_g05_r1a_contract_verify.sh
ARBOR_A1_BUNDLE="$A1" bash tools/view0_v1n1_g05_r2a_contract_verify.sh
bash tools/view0_v1n1_g05_c0_sr1_scope_verify.sh
ARBOR_A1_BUNDLE="$A1" bash tools/view0_v1n1_g05_c0_sr1_contract_verify.sh
ARBOR_A1_BUNDLE="$A1" ARBOR_G05_C0_SR1_BUNDLE="$C0_SR1" bash tools/view0_v1n1_g05_r3a_contract_verify.sh
bash tools/view0_v1n1_g05_r3a_native_verify.sh
git diff --check
echo 'VIEW0_V1N1_G05_R3A_GATE=PASS'
echo 'VIEW0_V1N1_G05_R3A_CONDITIONAL_CLAUSE_ROWS=43'
echo 'VIEW0_V1N1_G05_R3A_INPUT_STATE_CLAUSE_ROWS=25'
echo 'VIEW0_V1N1_G05_R3A_DISTINCT_INPUT_STATES=22'
echo 'VIEW0_V1N1_G05_R3A_R1_IMPLEMENTED=YES'
echo 'VIEW0_V1N1_G05_R3A_R2_IMPLEMENTED=YES'
echo 'VIEW0_V1N1_G05_R3A_R3_IMPLEMENTED=YES'
echo 'VIEW0_V1N1_G05_R3A_R4_IMPLEMENTED=NO'
echo 'VIEW0_V1N1_G05_R3A_G05_GROUP_FREEZE=NO'
echo 'VIEW0_V1N1_G05_R3A_COMPLETE_HTML_CONFORMANCE_CLAIM=NO'
echo 'PASS: G05 R3A full gate passed'
