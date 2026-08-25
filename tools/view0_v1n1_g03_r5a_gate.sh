#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
fail(){ echo "FAIL: $*" >&2; exit 1; }
echo '### VIEW0 V1N1 G03 R5A — SCOPE / CONTRACT / ACCEPTED F1-R10 ACTUAL-FOSTER OWNERSHIP'
bash tools/view0_v1n1_g03_r5a_scope_verify.sh
bash tools/view0_v1n1_g03_r5a_contract_verify.sh
echo '### VIEW0 V1N1 G03 R5A — CLEAN DERIVED TEST RESET'
rm -f \
  build/view0-v1/native/native.o build/view0-v1/native/g03_r2a.o \
  build/view0-v1/native/g03_r3a.o build/view0-v1/native/g03_r4a.o \
  build/view0-v1/native/g03_r5a.o build/view0-v1/native/main.o \
  build/view0-v1/native/g03_r5a_explicit_html_element_allowance_test.o \
  build/view0-v1/native/g03_r5a_explicit_html_element_allowance_adversarial_test.o \
  build/view0-v1/native/g03-r5a-explicit-html-element-allowance-test \
  build/view0-v1/native/g03-r5a-explicit-html-element-allowance-adversarial-test \
  build/view0-v1/native/g03-r5a-explicit-html-element-allowance-sanitize-test \
  build/view0-v1/native/arborcore-view0-html-check
echo 'VIEW0_V1N1_G03_R5A_DERIVED_TEST_RESET=PASS'
echo '### VIEW0 V1N1 G03 R5A — NATIVE / RETAINED REGRESSION / ANALYZER / STACK / CLI'
bash tools/view0_v1n1_g03_r5a_native_verify.sh
echo '### VIEW0 V1N1 G03 R5A — ASAN / UBSAN WITH SANITIZED PINNED LEXBOR'
make -s view0-v1n1-g03-r5a-sanitize
[[ -x build/view0-v1/native/g03-r5a-explicit-html-element-allowance-sanitize-test ]] || fail 'fresh R5A sanitizer test missing'
echo 'VIEW0_V1N1_G03_R5A_SANITIZE_REBUILD=PASS'
echo '### VIEW0 V1N1 G03 R5A — FINAL DIFF / POLICY'
git diff --check
[[ -z "$(git diff --cached --name-only)" ]] || fail 'Git index changed during R5A gate'
[[ "$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' view/arborcore-view-core-1.contract | head -1)" == '0.1-VIEW0-V1N1-G03-R5A' ]] || fail 'unexpected final R5A contract version'
[[ "$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)" -eq 11 ]] || fail 'production VIEW API count changed'
! grep -Eq 'ARBOR_VIEW_V1_G03_(SCALAR_VALUE_TEXT|PALPABLE_PHRASING_NONEMPTY)|0x000000003003000[67]' tools/include/arborcore/view0_conformance/native.h || fail 'G03 R6/R7 implementation appeared'
echo 'VIEW0_V1N1_G03_R5A_GATE=PASS'
echo 'VIEW0_V1N1_G03_R5A_IMPLEMENTABLE_CLASS=FOSTER_PARENTED_SOURCE_STANDARD_START_TAG'
echo 'VIEW0_V1N1_G03_R5A_ACTUAL_FOSTER_TARGET_SET=TABLE_TBODY_TFOOT_THEAD_TR'
echo 'VIEW0_V1N1_G03_R5A_FOSTER_FLAG_ALONE_ACTUAL_FOSTER_PROOF=NO'
echo 'VIEW0_V1N1_G03_R5A_REPAIR_CLASS_LEDGER=9_OF_9'
echo 'VIEW0_V1N1_G03_R5A_DEFERRED_CLASS_COUNT=4'
echo 'VIEW0_V1N1_G03_R5A_CUSTOM_ELEMENTS=G13'
echo 'VIEW0_V1N1_G03_R5A_IMPLEMENTATION_COMPLETE=NO'
echo 'VIEW0_V1N1_G03_R5A_G03_R6_R7_RULE_IDS_IMPLEMENTED=ZERO'
echo 'VIEW0_V1N1_G03_R5A_G03_GROUP_FREEZE=NO'
echo 'VIEW0_V1N1_G03_R5A_COMPLETE_HTML_CONFORMANCE_CLAIM=NO'
echo 'VIEW0_V1N1_G03_R5A_STAGED_CHANGES=NO'
echo 'VIEW0_V1N1_G03_R5A_NEW_COMMIT_CREATED=NO'
echo 'VIEW0_V1N1_G03_R5A_REMOTE_WRITE_PERFORMED=NO'
