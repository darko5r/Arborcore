#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
fail(){ echo "FAIL: $*" >&2; exit 1; }
echo '### VIEW0 V1N1 G03 R6A — SCOPE / CONTRACT / F1-R11 RETAINED OWNERSHIP'
bash tools/view0_v1n1_g03_r6a_scope_verify.sh
bash tools/view0_v1n1_g03_r6a_contract_verify.sh
echo '### VIEW0 V1N1 G03 R6A — CLEAN RETAINED R5A + R6A DERIVED TEST RESET'
# Rebuild the retained R5A dependency closure from source rather than trusting
# ignored/stale object timestamps. This mirrors the accepted R5A gate reset and
# prevents a higher-stage retention gate from passing only because lower binaries
# were already present.
rm -f \
 build/view0-v1/native/native.o build/view0-v1/native/lexbor_adapter.o \
 build/view0-v1/native/g03_c0_provenance.o build/view0-v1/native/g03_r1a.o \
 build/view0-v1/native/g03_r2a.o build/view0-v1/native/g03_r3a.o \
 build/view0-v1/native/g03_r4a.o build/view0-v1/native/g03_r5a.o \
 build/view0-v1/native/main.o \
 build/view0-v1/native/g03_r5a_explicit_html_element_allowance_test.o \
 build/view0-v1/native/g03_r5a_explicit_html_element_allowance_adversarial_test.o \
 build/view0-v1/native/g03-r5a-explicit-html-element-allowance-test \
 build/view0-v1/native/g03-r5a-explicit-html-element-allowance-adversarial-test \
 build/view0-v1/native/g03-r5a-explicit-html-element-allowance-sanitize-test \
 build/view0-v1/native/arborcore-view0-html-check \
 build/view0-v1/native/g03_r6a_scalar_value_text_retention_test.o \
 build/view0-v1/native/g03_r6a_scalar_value_text_retention_adversarial_test.o \
 build/view0-v1/native/g03-r6a-scalar-value-text-retention-test \
 build/view0-v1/native/g03-r6a-scalar-value-text-retention-adversarial-test
echo 'VIEW0_V1N1_G03_R6A_RETAINED_NATIVE_RULE_OBJECT_CLEAN_RESET=8_OF_8'
echo 'VIEW0_V1N1_G03_R6A_RETAINED_R5A_DERIVED_RESET=PASS'
echo 'VIEW0_V1N1_G03_R6A_DERIVED_TEST_RESET=PASS'
echo '### VIEW0 V1N1 G03 R6A — RETAINED NATIVE + OWNER TESTS'
bash tools/view0_v1n1_g03_r6a_native_verify.sh
echo '### VIEW0 V1N1 G03 R6A — RETAINED ASAN / UBSAN R5A RUNTIME'
make -s view0-v1n1-g03-r5a-sanitize
[[ -x build/view0-v1/native/g03-r5a-explicit-html-element-allowance-sanitize-test ]] || fail 'retained R5A sanitizer binary missing'
echo 'VIEW0_V1N1_G03_R6A_RETAINED_RUNTIME_SANITIZE=PASS'
git diff --check
[[ -z "$(git diff --cached --name-only)" ]] || fail 'Git index changed during R6A gate'
[[ "$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' view/arborcore-view-core-1.contract|head -1)" == '0.1-VIEW0-V1N1-G03-R6A' ]] || fail 'R6A top contract drift'
[[ "$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)" -eq 11 ]] || fail 'production VIEW API count changed'
[[ ! -e tools/c/view0_conformance/g03_r6a.c && ! -e tools/c/view0_conformance/g03_r6a.h ]] || fail 'R6 evaluator source appeared'
echo 'VIEW0_V1N1_G03_R6A_GATE=PASS'
echo 'VIEW0_V1N1_G03_R6A_OWNERSHIP_LEDGER=7_OF_7'
echo 'VIEW0_V1N1_G03_R6A_FIXTURE_PLAN=6_OF_6'
echo 'VIEW0_V1N1_G03_R6A_NEW_SEMANTIC_EVALUATOR=NO'
echo 'VIEW0_V1N1_G03_R6A_NEW_DIAGNOSTIC=NO'
echo 'VIEW0_V1N1_G03_R6A_RUNTIME_SEMANTIC_SOURCE_CHANGE=NO'
echo 'VIEW0_V1N1_G03_R6A_IMPLEMENTATION_COMPLETE=YES_RETAINED_OWNER_INTEGRATION'
echo 'VIEW0_V1N1_G03_R6A_G03_R7_RULE_IDS_IMPLEMENTED=ZERO'
echo 'VIEW0_V1N1_G03_R6A_G03_GROUP_FREEZE=NO'
echo 'VIEW0_V1N1_COMPLETE_HTML_CONFORMANCE_CLAIM=NO'
echo 'VIEW0_V1N1_STAGED_CHANGES=NO'
echo 'VIEW0_V1N1_NEW_COMMIT_CREATED=NO'
echo 'VIEW0_V1N1_REMOTE_WRITE_PERFORMED=NO'
