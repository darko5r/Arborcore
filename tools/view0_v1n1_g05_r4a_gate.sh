#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
A1="${ARBOR_A1_BUNDLE:-/root/Downloads/Arborcore-VIEW0-V1-A1-wave-a-support-authority-candidate-1563a1cddfd0fae52a8042dc84543c302e4c07d9b3fe47dc541961b19d09d30e.tar.gz}"
R3="${ARBOR_G05_R3A_BUNDLE:-/root/Downloads/Arborcore-VIEW0-V1N1-G05-R3A-construction-result-candidate-bcc20446e48c093bb0f1f53d886b5e5e25644a9b261823e084e7cfc308fa3e4a.tar.gz}"
WA0="${ARBOR_WA0_BUNDLE:-/root/Downloads/Arborcore-VIEW0-V1-WA0-implementation-surface-candidate-6254ba06fd35d26d9404a5dee96305c57f6b66ba87e9963a02186bdade56d2f7.tar.gz}"
GF1="${ARBOR_G04_GF1_BUNDLE:-/root/Downloads/Arborcore-VIEW0-V1N1-G04-GF1-group-freeze-candidate-889994f122452dd8b9acce0825fc3d29d2b93644916bd34dc13dc08932db039b.tar.gz}"
C0_SR1="${ARBOR_G05_C0_SR1_BUNDLE:-/root/Downloads/Arborcore-VIEW0-V1N1-G05-C0-SR1-construction-result-candidate-a0a03c65bd7d23089df87ce45fe16445361606539fa62c07933dddb3c3d54201.tar.gz}"
echo '### VIEW0 V1N1 G05 R4A — BODY WINDOW EVENT ATTRIBUTE APPLICABILITY FULL GATE'
bash tools/view0_v1n1_g05_r4a_scope_verify.sh
bash tools/view0_v1n1_g05_c0_scope_verify.sh
ARBOR_A1_BUNDLE="$A1" ARBOR_WA0_BUNDLE="$WA0" ARBOR_G04_GF1_BUNDLE="$GF1" bash tools/view0_v1n1_g05_c0_contract_verify.sh
bash tools/view0_v1n1_g05_c0_sr1_scope_verify.sh
ARBOR_A1_BUNDLE="$A1" bash tools/view0_v1n1_g05_c0_sr1_contract_verify.sh
bash tools/view0_v1n1_g05_r1a_scope_verify.sh
ARBOR_A1_BUNDLE="$A1" bash tools/view0_v1n1_g05_r1a_contract_verify.sh
ARBOR_A1_BUNDLE="$A1" bash tools/view0_v1n1_g05_r2a_contract_verify.sh
ARBOR_A1_BUNDLE="$A1" bash tools/view0_v1n1_g05_c0_sr1_contract_verify.sh
bash tools/view0_v1n1_g05_r2a_scope_verify.sh
ARBOR_A1_BUNDLE="$A1" bash tools/view0_v1n1_g05_r2a_contract_verify.sh
bash tools/view0_v1n1_g05_r3a_scope_verify.sh
ARBOR_A1_BUNDLE="$A1" ARBOR_G05_C0_SR1_BUNDLE="$C0_SR1" bash tools/view0_v1n1_g05_r3a_contract_verify.sh
ARBOR_A1_BUNDLE="$A1" ARBOR_G05_R3A_BUNDLE="$R3" bash tools/view0_v1n1_g05_r4a_contract_verify.sh
bash tools/view0_v1n1_g05_r4a_native_verify.sh
git diff --check
echo 'VIEW0_V1N1_G05_R4A_GATE=PASS'
echo 'VIEW0_V1N1_G05_R4A_BODY_WINDOW_EVENT_ROWS=18'
echo 'VIEW0_V1N1_G05_R4A_R1_IMPLEMENTED=YES'
echo 'VIEW0_V1N1_G05_R4A_R2_IMPLEMENTED=YES'
echo 'VIEW0_V1N1_G05_R4A_R3_IMPLEMENTED=YES'
echo 'VIEW0_V1N1_G05_R4A_R4_IMPLEMENTED=YES'
echo 'VIEW0_V1N1_G05_R4A_G05_GROUP_FREEZE=NO_PENDING_INDEPENDENT_REVIEW'
echo 'VIEW0_V1N1_G05_R4A_VALUE_SEMANTICS=DEFER_G16_EVENT_HANDLER_JAVASCRIPT'
echo 'VIEW0_V1N1_G05_R4A_COMPLETE_HTML_CONFORMANCE_CLAIM=NO'
echo 'PASS: G05 R4A full gate passed'
