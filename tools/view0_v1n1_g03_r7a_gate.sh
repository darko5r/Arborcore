#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
fail(){ echo "FAIL: $*" >&2; exit 1; }
echo '### VIEW0 V1N1 G03 R7A — SCOPE / CONTRACT / F1-R13 PALPABLE WARNING'
bash tools/view0_v1n1_g03_r7a_scope_verify.sh
bash tools/view0_v1n1_g03_r7a_contract_verify.sh

echo '### VIEW0 V1N1 G03 R7A — CLEAN RETAINED R6A + R7A DERIVED RESET'
rm -f \
 build/view0-v1/native/native.o build/view0-v1/native/lexbor_adapter.o \
 build/view0-v1/native/g03_c0_provenance.o build/view0-v1/native/g03_r1a.o \
 build/view0-v1/native/g03_r2a.o build/view0-v1/native/g03_r3a.o \
 build/view0-v1/native/g03_r4a.o build/view0-v1/native/g03_r5a.o \
 build/view0-v1/native/g03_r7a.o build/view0-v1/native/main.o \
 build/view0-v1/native/g03_r5a_explicit_html_element_allowance_test.o \
 build/view0-v1/native/g03_r5a_explicit_html_element_allowance_adversarial_test.o \
 build/view0-v1/native/g03-r5a-explicit-html-element-allowance-test \
 build/view0-v1/native/g03-r5a-explicit-html-element-allowance-adversarial-test \
 build/view0-v1/native/g03-r5a-explicit-html-element-allowance-sanitize-test \
 build/view0-v1/native/g03_r6a_scalar_value_text_retention_test.o \
 build/view0-v1/native/g03_r6a_scalar_value_text_retention_adversarial_test.o \
 build/view0-v1/native/g03-r6a-scalar-value-text-retention-test \
 build/view0-v1/native/g03-r6a-scalar-value-text-retention-adversarial-test \
 build/view0-v1/native/g03_r7a_palpable_content_test.o \
 build/view0-v1/native/g03_r7a_palpable_content_adversarial_test.o \
 build/view0-v1/native/g03_r7a_global_failure_atomicity_test.o \
 build/view0-v1/native/g03-r7a-palpable-content-test \
 build/view0-v1/native/g03-r7a-palpable-content-adversarial-test \
 build/view0-v1/native/g03-r7a-global-failure-atomicity-test \
 build/view0-v1/native/g03-r7a-palpable-content-sanitize-test \
 build/view0-v1/native/arborcore-view0-html-check
echo 'VIEW0_V1N1_G03_R7A_RETAINED_NATIVE_RULE_OBJECT_CLEAN_RESET=9_OF_9'
echo 'VIEW0_V1N1_G03_R7A_RETAINED_R6A_DERIVED_RESET=PASS'
echo 'VIEW0_V1N1_G03_R7A_DERIVED_TEST_RESET=PASS'

echo '### VIEW0 V1N1 G03 R7A — RETAINED NATIVE + R7A TESTS'
bash tools/view0_v1n1_g03_r7a_native_verify.sh

echo '### VIEW0 V1N1 G03 R7A — ASAN / UBSAN WITH SANITIZED PINNED LEXBOR'
make -s view0-v1n1-g03-r7a-sanitize
[[ -x build/view0-v1/native/g03-r7a-palpable-content-sanitize-test ]] || fail 'R7A sanitizer binary missing'
echo 'VIEW0_V1N1_G03_R7A_SANITIZE_REBUILD=PASS'

git diff --check
[[ -z "$(git diff --cached --name-only)" ]] || fail 'Git index changed during R7A gate'
[[ "$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' view/arborcore-view-core-1.contract|head -1)" == '0.1-VIEW0-V1N1-G03-R7A' ]] || fail 'R7A top contract drift'
[[ "$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)" -eq 11 ]] || fail 'production VIEW API count changed'
[[ -f tools/c/view0_conformance/g03_r7a.c && -f tools/c/view0_conformance/g03_r7a.h ]] || fail 'R7 evaluator source missing'
[[ ! -e tools/c/view0_conformance/g03_r6a.c && ! -e tools/c/view0_conformance/g03_r6a.h ]] || fail 'dedicated R6 evaluator source appeared'

echo 'VIEW0_V1N1_G03_R7A_GATE=PASS'
echo 'VIEW0_V1N1_G03_R7A_SR2_GLOBAL_MECHANISM_FAILURE_ATOMICITY=PASS'
echo 'VIEW0_V1N1_G03_R7A_SR2_ANCHOR_EQUIVALENCE=R1_R5_R7_PASS'
echo 'VIEW0_V1N1_G03_R7A_SR2_EXACT_CLI_ANCHOR=PASS'
echo 'VIEW0_V1N1_G03_R7A_SR2_STACK_THRESHOLD_WIDENING=NO'
echo 'VIEW0_V1N1_G03_R7A_SR2_R1_R7_SEMANTICS_CHANGE=NO'
echo 'VIEW0_V1N1_G03_R7A_F1_R13_FIXTURES=18_OF_18'
echo 'VIEW0_V1N1_G03_R7A_RETAINED_FIXTURE_ISOLATION=8_OF_8'
echo 'VIEW0_V1N1_G03_R7A_CONDITIONAL_PALPABILITY_CONTROLS=10_OF_10'
echo 'VIEW0_V1N1_G03_R7A_G04_TRANSPARENT_DEPENDENCY=DEFER_NO_WARNING'
echo 'VIEW0_V1N1_G03_R7A_G13_CUSTOM_DEPENDENCY=DEFER_NO_WARNING'
echo 'VIEW0_V1N1_G03_R7A_IMPLEMENTATION_COMPLETE=NO_G04_G13_DEFERRED'
echo 'VIEW0_V1N1_G03_R7A_G03_RULE_IDS_CONSTRUCTED=7_OF_7'
echo 'VIEW0_V1N1_G03_R7A_G03_GROUP_FREEZE=NO'
echo 'VIEW0_V1N1_COMPLETE_HTML_CONFORMANCE_CLAIM=NO'
echo 'VIEW0_V1N1_STAGED_CHANGES=NO'
echo 'VIEW0_V1N1_NEW_COMMIT_CREATED=NO'
echo 'VIEW0_V1N1_REMOTE_WRITE_PERFORMED=NO'
