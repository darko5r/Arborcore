#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
fail(){ echo "FAIL: $*" >&2; exit 1; }

echo '### VIEW0 V1N1 G04 R1B — SCOPE / RETAINED R1A / CONTRACT'
bash tools/view0_v1n1_g04_r1b_scope_verify.sh
bash tools/view0_v1n1_g04_r1a_scope_verify.sh
bash tools/view0_v1n1_g04_r1a_contract_verify.sh
bash tools/view0_v1n1_g04_r1b_contract_verify.sh

echo '### VIEW0 V1N1 G04 R1B — CLEAN DERIVED NATIVE/TEST RESET'
rm -f \
 build/view0-v1/native/native.o build/view0-v1/native/lexbor_adapter.o build/view0-v1/native/g04_r1a.o build/view0-v1/native/main.o \
 build/view0-v1/native/g04_r1a_transparent_parent_model_test.o \
 build/view0-v1/native/g04_r1a_transparent_parent_model_adversarial_test.o \
 build/view0-v1/native/g04_r1a_global_failure_atomicity_test.o \
 build/view0-v1/native/g04_r1b_transparent_parent_model_test.o \
 build/view0-v1/native/g04_r1b_transparent_parent_model_adversarial_test.o \
 build/view0-v1/native/g04-r1a-transparent-parent-model-test \
 build/view0-v1/native/g04-r1a-transparent-parent-model-adversarial-test \
 build/view0-v1/native/g04-r1a-global-failure-atomicity-test \
 build/view0-v1/native/g04-r1b-transparent-parent-model-test \
 build/view0-v1/native/g04-r1b-transparent-parent-model-adversarial-test \
 build/view0-v1/native/g04-r1b-transparent-parent-model-sanitize-test \
 build/view0-v1/native/arborcore-view0-html-check
echo 'VIEW0_V1N1_G04_R1B_DERIVED_RESET=PASS'

echo '### VIEW0 V1N1 G04 R1B — RETAINED + R1B NATIVE QUALIFICATION'
bash tools/view0_v1n1_g04_r1b_native_verify.sh

echo '### VIEW0 V1N1 G04 R1B — ASAN / UBSAN'
make -s view0-v1n1-g04-r1b-sanitize
[[ -x build/view0-v1/native/g04-r1b-transparent-parent-model-sanitize-test ]] || fail 'G04 R1B sanitizer binary missing'
echo 'VIEW0_V1N1_G04_R1B_SANITIZE_REBUILD=PASS'

git diff --check
[[ -z "$(git diff --cached --name-only)" ]] || fail 'Git index changed during G04 R1B gate'
current_version="$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' view/arborcore-view-core-1.contract | head -1)"; [[ "$current_version" == '0.1-VIEW0-V1N1-G04-R1B' || "$current_version" == '0.1-VIEW0-V1N1-G04-R1C' || "$current_version" == '0.1-VIEW0-V1N1-G04-R2' ]] || fail "top contract drift: $current_version"
[[ "$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)" -eq 11 ]] || fail 'production VIEW API count changed'
[[ -f tools/c/view0_conformance/g04_r1a.c && -f tools/c/view0_conformance/g04_r1a.h ]] || fail 'G04 R1 evaluator missing'
if [[ "$current_version" == '0.1-VIEW0-V1N1-G04-R2' ]]; then [[ -f tools/c/view0_conformance/g04_r2a.c && -f tools/c/view0_conformance/g04_r2a.h ]] || fail 'R2 extension missing'; else [[ ! -e tools/c/view0_conformance/g04_r2a.c && ! -e tools/c/view0_conformance/g04_r2a.h ]] || fail 'G04 R2 appeared before R2'; fi
rg -F 'source_attribute' tools/include/arborcore/view0_conformance/native.h tools/c/view0_conformance/lexbor_adapter.c tools/c/view0_conformance/g04_r1a.c >/dev/null || fail 'source attribute observer missing'
rg -F 'source_text' tools/include/arborcore/view0_conformance/native.h tools/c/view0_conformance/lexbor_adapter.c tools/c/view0_conformance/g04_r1a.c >/dev/null || fail 'source text observer missing'

echo 'VIEW0_V1N1_G04_R1B_GATE=PASS'
echo 'VIEW0_V1N1_G04_R1B_FROZEN_MATRIX_FIXTURES=2_OF_2'
echo 'VIEW0_V1N1_G04_R1B_OPTION_BRANCH_CLOSURE=PASS'
echo 'VIEW0_V1N1_G04_R1B_SELECT_SOURCE_TEXT_CLOSURE=PASS'
echo 'VIEW0_V1N1_G04_R1B_CHARACTER_REFERENCE_TEXT=PASS'
echo 'VIEW0_V1N1_G04_R1B_OBSERVER_FAILURE_ATOMICITY=ATTRIBUTE_TEXT_PASS'
if [[ "$current_version" == '0.1-VIEW0-V1N1-G04-R1C' ]]; then echo 'VIEW0_V1N1_G04_R1B_RETAINED_UNDER_R1C=PASS_G13_ONLY'; elif [[ "$current_version" == '0.1-VIEW0-V1N1-G04-R2' ]]; then echo 'VIEW0_V1N1_G04_R1B_RETAINED_UNDER_R2=PASS_G13_ONLY'; else echo 'VIEW0_V1N1_G04_R1B_REMAINING_R1_RESIDUALS=NOSCRIPT_G13'; fi
echo 'VIEW0_V1N1_G04_R1B_G04_R2_IMPLEMENTED=NO'
echo 'VIEW0_V1N1_G04_R1B_G04_GROUP_FREEZE=NO'
if [[ "$current_version" == '0.1-VIEW0-V1N1-G04-R1C' || "$current_version" == '0.1-VIEW0-V1N1-G04-R2' ]]; then echo 'VIEW0_V1N1_G04_R1B_R1_IMPLEMENTATION_COMPLETE=SUPERSEDED_BY_R1C_G13_EXTERNAL_DEPENDENCY_ONLY'; else echo 'VIEW0_V1N1_G04_R1B_R1_IMPLEMENTATION_COMPLETE=NO_NOSCRIPT_G13_RESIDUAL'; fi
echo 'VIEW0_V1N1_G04_R1B_COMPLETE_HTML_CONFORMANCE_CLAIM=NO'
echo 'VIEW0_V1N1_G04_R1B_STAGED_CHANGES=NO'
echo 'VIEW0_V1N1_G04_R1B_NEW_COMMIT_CREATED=NO'
echo 'VIEW0_V1N1_G04_R1B_REMOTE_WRITE_PERFORMED=NO'
echo 'PASS: G04 R1B full gate passed'
