#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"; fail(){ echo "FAIL: $*" >&2; exit 1; }
echo '### VIEW0 V1N1 G03 R4A — SCOPE / CONTRACT / F1-R8 NOTHING-MODEL OWNERSHIP'
bash tools/view0_v1n1_g03_r4a_scope_verify.sh
bash tools/view0_v1n1_g03_r4a_contract_verify.sh
echo '### VIEW0 V1N1 G03 R4A — RETAINED R3A / R2A / R1A / C0-L1 / C0 / G02 / V1N0 LX1 / LOWER LAYERS'
bash tools/view0_v1n1_g03_r3a_gate.sh
echo 'VIEW0_V1N1_G03_R4A_RETAINED_R3A_R2A_R1A_LX1=PASS'
echo '### VIEW0 V1N1 G03 R4A — CLEAN DERIVED TEST RESET'
rm -f build/view0-v1/native/native.o build/view0-v1/native/g03_r4a.o build/view0-v1/native/main.o build/view0-v1/native/g03_r4a_nothing_model_test.o build/view0-v1/native/g03_r4a_nothing_model_adversarial_test.o build/view0-v1/native/g03-r4a-nothing-model-test build/view0-v1/native/g03-r4a-nothing-model-adversarial-test build/view0-v1/native/g03-r4a-nothing-model-sanitize-test build/view0-v1/native/arborcore-view0-html-check
echo 'VIEW0_V1N1_G03_R4A_DERIVED_TEST_RESET=PASS'
echo '### VIEW0 V1N1 G03 R4A — NATIVE / ADVERSARIAL / ANALYZER / CLI'
bash tools/view0_v1n1_g03_r4a_native_verify.sh
echo '### VIEW0 V1N1 G03 R4A — ASAN / UBSAN WITH LX1 SANITIZED LEXBOR'
make -s view0-v1n1-g03-r4a-sanitize
[[ -x build/view0-v1/native/g03-r4a-nothing-model-sanitize-test ]] || fail 'fresh R4A sanitizer test missing'
echo 'VIEW0_V1N1_G03_R4A_SANITIZE_REBUILD=PASS'
echo '### VIEW0 V1N1 G03 R4A — FINAL DIFF / POLICY'
git diff --check
[[ -z "$(git diff --cached --name-only)" ]] || fail 'Git index changed during R4A gate'
[[ "$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' view/arborcore-view-core-1.contract | head -1)" == '0.1-VIEW0-V1N1-G03-R4A' ]] || fail 'unexpected final R4A contract version'
[[ "$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)" -eq 11 ]] || fail 'production VIEW API count changed'
! grep -ERq 'ARBOR_VIEW_V1_G03_(EXPLICIT_HTML_ELEMENT_ALLOWANCE|SCALAR_VALUE_TEXT|PALPABLE_PHRASING_NONEMPTY)|0x000000003003000[5-7]' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G03 R5-R7 implementation appeared'
echo 'VIEW0_V1N1_G03_R4A_GATE=PASS'
echo 'VIEW0_V1N1_G03_R4A_RETAINED_R3A_R2A_R1A_LX1=PASS'
echo 'VIEW0_V1N1_G03_R4A_RULE_ID=0x0000000030030004'
echo 'VIEW0_V1N1_G03_R4A_SUBJECT_DEFINITION_COUNT=18'
echo 'VIEW0_V1N1_G03_R4A_VACUOUS_SUBJECT_COUNT=14'
echo 'VIEW0_V1N1_G03_R4A_IMPLEMENT_NOW_SUBJECT_COUNT=3'
echo 'VIEW0_V1N1_G03_R4A_DEFERRED_SUBJECT_COUNT=1'
echo 'VIEW0_V1N1_G03_R4A_IMPLEMENTATION_COMPLETE=NO'
echo 'VIEW0_V1N1_G03_R4A_G03_R5_R7_RULE_IDS_IMPLEMENTED=ZERO'
echo 'VIEW0_V1N1_G03_R4A_G03_GROUP_FREEZE=NO'
echo 'VIEW0_V1N1_G03_R4A_COMPLETE_HTML_CONFORMANCE_CLAIM=NO'
echo 'VIEW0_V1N1_G03_R4A_STAGED_CHANGES=NO'
echo 'VIEW0_V1N1_G03_R4A_NEW_COMMIT_CREATED=NO'
echo 'VIEW0_V1N1_G03_R4A_REMOTE_WRITE_PERFORMED=NO'
echo 'NEXT_REQUIRED_STEP=RETURN_COMPLETE_G03_R4A_CONSTRUCTION_AND_GATE_OUTPUT_FOR_REVIEW_BEFORE_NEXT_G03_BOUNDARY'
