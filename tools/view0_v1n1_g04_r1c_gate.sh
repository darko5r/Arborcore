#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
fail(){ echo "FAIL: $*" >&2; exit 1; }

echo '### VIEW0 V1N1 G04 R1C — SCOPE / RETAINED R1A-R1B / CONTRACT'
bash tools/view0_v1n1_g04_r1c_scope_verify.sh
bash tools/view0_v1n1_g04_r1a_scope_verify.sh
bash tools/view0_v1n1_g04_r1b_scope_verify.sh
bash tools/view0_v1n1_g04_r1a_contract_verify.sh
bash tools/view0_v1n1_g04_r1b_contract_verify.sh
bash tools/view0_v1n1_g04_r1c_contract_verify.sh

echo '### VIEW0 V1N1 G04 R1C — CLEAN DERIVED NATIVE/TEST RESET'
rm -f \
 build/view0-v1/native/native.o build/view0-v1/native/lexbor_adapter.o build/view0-v1/native/g04_r1a.o build/view0-v1/native/main.o \
 build/view0-v1/native/g04_r1a_transparent_parent_model_test.o \
 build/view0-v1/native/g04_r1a_transparent_parent_model_adversarial_test.o \
 build/view0-v1/native/g04_r1a_global_failure_atomicity_test.o \
 build/view0-v1/native/g04_r1b_transparent_parent_model_test.o \
 build/view0-v1/native/g04_r1b_transparent_parent_model_adversarial_test.o \
 build/view0-v1/native/g04_r1c_noscript_transparent_test.o \
 build/view0-v1/native/g04_r1c_noscript_transparent_adversarial_test.o \
 build/view0-v1/native/g04-r1a-transparent-parent-model-test \
 build/view0-v1/native/g04-r1a-transparent-parent-model-adversarial-test \
 build/view0-v1/native/g04-r1a-global-failure-atomicity-test \
 build/view0-v1/native/g04-r1b-transparent-parent-model-test \
 build/view0-v1/native/g04-r1b-transparent-parent-model-adversarial-test \
 build/view0-v1/native/g04-r1a-transparent-parent-model-sanitize-test \
 build/view0-v1/native/g04-r1b-transparent-parent-model-sanitize-test \
 build/view0-v1/native/g04-r1c-noscript-transparent-test \
 build/view0-v1/native/g04-r1c-noscript-transparent-adversarial-test \
 build/view0-v1/native/g04-r1c-noscript-transparent-sanitize-test \
 build/view0-v1/native/arborcore-view0-html-check
for stale in \
 build/view0-v1/native/g04_r1a_transparent_parent_model_test.o \
 build/view0-v1/native/g04_r1a_transparent_parent_model_adversarial_test.o \
 build/view0-v1/native/g04_r1a_global_failure_atomicity_test.o \
 build/view0-v1/native/g04_r1b_transparent_parent_model_test.o \
 build/view0-v1/native/g04_r1b_transparent_parent_model_adversarial_test.o \
 build/view0-v1/native/g04-r1a-transparent-parent-model-test \
 build/view0-v1/native/g04-r1a-transparent-parent-model-adversarial-test \
 build/view0-v1/native/g04-r1a-global-failure-atomicity-test \
 build/view0-v1/native/g04-r1b-transparent-parent-model-test \
 build/view0-v1/native/g04-r1b-transparent-parent-model-adversarial-test; do
    [[ ! -e "$stale" ]] || fail "retained G04 derived reset failed: $stale"
done
echo 'VIEW0_V1N1_G04_R1C_DERIVED_RESET=PASS'
echo 'VIEW0_V1N1_G04_R1C_SR1_RETAINED_G04_TEST_RESET=PASS'

echo '### VIEW0 V1N1 G04 R1C — RETAINED + R1C NATIVE QUALIFICATION'
bash tools/view0_v1n1_g04_r1c_native_verify.sh

echo '### VIEW0 V1N1 G04 R1C — ASAN / UBSAN'
make -s view0-v1n1-g04-r1c-sanitize
[[ -x build/view0-v1/native/g04-r1c-noscript-transparent-sanitize-test ]] || fail 'R1C sanitizer binary missing'
echo 'VIEW0_V1N1_G04_R1C_SANITIZE_REBUILD=PASS'

git diff --check
[[ -z "$(git diff --cached --name-only)" ]] || fail 'Git index changed during G04 R1C gate'
current_version="$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' view/arborcore-view-core-1.contract | head -1)"; [[ "$current_version" == '0.1-VIEW0-V1N1-G04-R1C' || "$current_version" == '0.1-VIEW0-V1N1-G04-R2' ]] || fail 'top contract drift'
[[ "$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)" -eq 11 ]] || fail 'production VIEW API count changed'
if [[ "$current_version" == '0.1-VIEW0-V1N1-G04-R2' ]]; then [[ -f tools/c/view0_conformance/g04_r2a.c && -f tools/c/view0_conformance/g04_r2a.h ]] || fail 'R2 extension missing'; else [[ ! -e tools/c/view0_conformance/g04_r2a.c && ! -e tools/c/view0_conformance/g04_r2a.h ]] || fail 'G04 R2 appeared before R2'; fi
expected_setters=1; [[ "$current_version" != '0.1-VIEW0-V1N1-G04-R2' ]] || expected_setters=2; [[ "$(grep -Fc 'lxb_html_parser_scripting_set(parser, false);' tools/c/view0_conformance/lexbor_adapter.c)" -eq "$expected_setters" ]] || fail 'explicit scripting-disabled parser bind drift'

echo 'VIEW0_V1N1_G04_R1C_GATE=PASS'
echo 'VIEW0_V1N1_G04_R1C_CHECKER_SCRIPTING_MODE=DISABLED'
echo 'VIEW0_V1N1_G04_R1C_NOSCRIPT_STANDARD_ELEMENT_CLOSURE=PASS'
echo 'VIEW0_V1N1_G04_R1C_STANDARD_ELEMENT_R1_COVERAGE=COMPLETE_FOR_FROZEN_SCRIPTING_DISABLED_CHECKER_MODE'
echo 'VIEW0_V1N1_G04_R1C_REMAINING_R1_EXTERNAL_DEPENDENCY=G13_CUSTOM_IDENTITY_ONLY'
echo 'VIEW0_V1N1_G04_R1C_R1_STANDARD_ELEMENT_IMPLEMENTATION_COMPLETE=YES'
echo 'VIEW0_V1N1_G04_R1C_R1_IMPLEMENTATION_COMPLETE=NO_G13_EXTERNAL_DEPENDENCY'
echo 'VIEW0_V1N1_G04_R1C_G04_R2_IMPLEMENTED=NO'
echo 'VIEW0_V1N1_G04_R1C_G04_GROUP_FREEZE=NO'
echo 'VIEW0_V1N1_G04_R1C_COMPLETE_HTML_CONFORMANCE_CLAIM=NO'
echo 'VIEW0_V1N1_G04_R1C_STAGED_CHANGES=NO'
echo 'VIEW0_V1N1_G04_R1C_NEW_COMMIT_CREATED=NO'
echo 'VIEW0_V1N1_G04_R1C_REMOTE_WRITE_PERFORMED=NO'
echo 'PASS: G04 R1C full gate passed'
