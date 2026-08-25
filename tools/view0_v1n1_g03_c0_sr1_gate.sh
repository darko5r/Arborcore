#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
fail(){ echo "FAIL: $*" >&2; exit 1; }
printf '%s\n' '### VIEW0 V1N1 G03 C0-SR1 — SCOPE / CONTRACT'
bash tools/view0_v1n1_g03_c0_sr1_scope_verify.sh
bash tools/view0_v1n1_g03_c0_sr1_contract_verify.sh
printf '%s\n' '### VIEW0 V1N1 G03 C0-SR1 — RETAINED R4A/R3A/R2A/R1A/C0/V1N0 LX1 CHAIN'
bash tools/view0_v1n1_g03_r4a_gate.sh
echo 'VIEW0_V1N1_G03_C0_SR1_RETAINED_R4A_CHAIN=PASS'
printf '%s\n' '### VIEW0 V1N1 G03 C0-SR1 — CLEAN DERIVED TEST RESET'
rm -f build/view0-v1/native/g03_c0_sr1_source_repair_test.o \
      build/view0-v1/native/g03_c0_sr1_source_repair_adversarial_test.o \
      build/view0-v1/native/g03-c0-sr1-source-repair-test \
      build/view0-v1/native/g03-c0-sr1-source-repair-adversarial-test \
      build/view0-v1/native/g03-c0-sr1-source-repair-sanitize-test
echo 'VIEW0_V1N1_G03_C0_SR1_DERIVED_TEST_RESET=PASS'
printf '%s\n' '### VIEW0 V1N1 G03 C0-SR1 — NATIVE / ADVERSARIAL / ANALYZER'
bash tools/view0_v1n1_g03_c0_sr1_native_verify.sh
printf '%s\n' '### VIEW0 V1N1 G03 C0-SR1 — ASAN / UBSAN WITH LX1 SANITIZED LEXBOR'
make -s view0-v1n1-g03-c0-sr1-sanitize
[[ -x build/view0-v1/native/g03-c0-sr1-source-repair-sanitize-test ]] || fail 'fresh C0-SR1 sanitizer binary missing'
echo 'VIEW0_V1N1_G03_C0_SR1_SANITIZE_REBUILD=PASS'
printf '%s\n' '### VIEW0 V1N1 G03 C0-SR1 — FINAL POLICY'
git diff --check
[[ -z "$(git diff --cached --name-only)" ]] || fail 'Git index changed during C0-SR1 gate'
[[ "$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' view/arborcore-view-core-1.contract | head -n1)" == '0.1-VIEW0-V1N1-G03-R4A' ]] || fail 'top contract changed during C0-SR1'
grep -Fxq 'VIEW0_V1N1_G03_C0_SR1_SOURCE_REPAIR_CONTEXT=QUALIFIED' view/arborcore-view-core-1.contract || fail 'C0-SR1 capability marker missing'
! grep -Rqs 'ARBOR_VIEW_V1_G03_EXPLICIT_HTML_ELEMENT_ALLOWANCE\|0x0000000030030005' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'R5 implementation leaked into support stage'
echo 'VIEW0_V1N1_G03_C0_SR1_GATE=PASS'
echo 'VIEW0_V1N1_G03_C0_SR1_ACTIVE_TOP_CONTRACT=0.1-VIEW0-V1N1-G03-R4A'
echo 'VIEW0_V1N1_G03_R5_IMPLEMENTATION_NOW=NO'
echo 'VIEW0_V1N1_STAGED_CHANGES=NO'
echo 'VIEW0_V1N1_NEW_COMMIT_CREATED=NO'
echo 'VIEW0_V1N1_REMOTE_WRITE_PERFORMED=NO'
echo 'NEXT_REQUIRED_STEP=RETURN_COMPLETE_C0_SR1_CONSTRUCTION_AND_GATE_OUTPUT_FOR_REVIEW_BEFORE_R5_SUPPORT_PLAN_REFINEMENT'
