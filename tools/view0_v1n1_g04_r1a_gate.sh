#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
fail(){ echo "FAIL: $*" >&2; exit 1; }
echo '### VIEW0 V1N1 G04 R1A — SCOPE / CONTRACT'
bash tools/view0_v1n1_g04_r1a_scope_verify.sh
bash tools/view0_v1n1_g04_r1a_contract_verify.sh

echo '### VIEW0 V1N1 G04 R1A — CLEAN DERIVED NATIVE/TEST RESET'
rm -f \
 build/view0-v1/native/native.o build/view0-v1/native/g04_r1a.o build/view0-v1/native/main.o \
 build/view0-v1/native/g04_r1a_transparent_parent_model_test.o \
 build/view0-v1/native/g04_r1a_transparent_parent_model_adversarial_test.o \
 build/view0-v1/native/g04_r1a_global_failure_atomicity_test.o \
 build/view0-v1/native/g04-r1a-transparent-parent-model-test \
 build/view0-v1/native/g04-r1a-transparent-parent-model-adversarial-test \
 build/view0-v1/native/g04-r1a-global-failure-atomicity-test \
 build/view0-v1/native/g04-r1a-transparent-parent-model-sanitize-test \
 build/view0-v1/native/arborcore-view0-html-check
echo 'VIEW0_V1N1_G04_R1A_DERIVED_RESET=PASS'

echo '### VIEW0 V1N1 G04 R1A — RETAINED + NEW NATIVE QUALIFICATION'
bash tools/view0_v1n1_g04_r1a_native_verify.sh

echo '### VIEW0 V1N1 G04 R1A — ASAN / UBSAN'
make -s view0-v1n1-g04-r1a-sanitize
[[ -x build/view0-v1/native/g04-r1a-transparent-parent-model-sanitize-test ]] || fail 'G04 sanitizer binary missing'
echo 'VIEW0_V1N1_G04_R1A_SANITIZE_REBUILD=PASS'

git diff --check
[[ -z "$(git diff --cached --name-only)" ]] || fail 'Git index changed during G04 R1A gate'
current_version="$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' view/arborcore-view-core-1.contract | head -1)"
[[ "$current_version" == '0.1-VIEW0-V1N1-G04-R1A' || "$current_version" == '0.1-VIEW0-V1N1-G04-R1B' || "$current_version" == '0.1-VIEW0-V1N1-G04-R1C' || "$current_version" == '0.1-VIEW0-V1N1-G04-R2' ]] || fail "top contract drift: $current_version"
[[ "$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)" -eq 11 ]] || fail 'production VIEW API count changed'
[[ -f tools/c/view0_conformance/g04_r1a.c && -f tools/c/view0_conformance/g04_r1a.h ]] || fail 'G04 R1 evaluator missing'
if [[ "$current_version" == '0.1-VIEW0-V1N1-G04-R2' ]]; then [[ -f tools/c/view0_conformance/g04_r2a.c && -f tools/c/view0_conformance/g04_r2a.h ]] || fail 'R2 extension missing'; else [[ ! -e tools/c/view0_conformance/g04_r2a.c && ! -e tools/c/view0_conformance/g04_r2a.h ]] || fail 'G04 R2 appeared before R2'; fi
echo 'VIEW0_V1N1_G04_R1A_GATE=PASS'
echo 'VIEW0_V1N1_G04_R1A_FROZEN_MATRIX_FIXTURES=2_OF_2'
echo 'VIEW0_V1N1_G04_R1A_ITERATIVE_TRANSPARENT_RESOLUTION=PASS'
echo 'VIEW0_V1N1_G04_R1A_SOURCE_REPAIR_ANCHOR=PASS'
echo 'VIEW0_V1N1_G04_R1A_PRIOR_OWNER_SUPPRESSION=PASS'
echo 'VIEW0_V1N1_G04_R1A_GLOBAL_MECHANISM_FAILURE_ATOMICITY=PASS'
if [[ "$current_version" == '0.1-VIEW0-V1N1-G04-R1A' ]]; then
  echo 'VIEW0_V1N1_G04_R1A_DYNAMIC_DEFERRALS=NOSCRIPT_OPTION_G13_PASS'
  echo 'VIEW0_V1N1_G04_R1A_SELECT_TEXT_RESIDUAL=PARTIAL_NO_WARNING'
  echo 'VIEW0_V1N1_G04_R1A_IMPLEMENTATION_COMPLETE=NO_NOSCRIPT_OPTION_G13_AND_SELECT_TEXT_PARTIAL'
elif [[ "$current_version" == '0.1-VIEW0-V1N1-G04-R1B' ]]; then
  echo 'VIEW0_V1N1_G04_R1A_RETAINED_BY_R1B=YES'
  echo 'VIEW0_V1N1_G04_R1A_OPTION_AND_SELECT_RESIDUALS=SUPERSEDED_BY_R1B'
elif [[ "$current_version" == '0.1-VIEW0-V1N1-G04-R1C' ]]; then
  echo 'VIEW0_V1N1_G04_R1A_RETAINED_BY_R1C=YES'
  echo 'VIEW0_V1N1_G04_R1A_NOSCRIPT_OPTION_AND_SELECT_RESIDUALS=SUPERSEDED_BY_R1B_R1C'
else
  echo 'VIEW0_V1N1_G04_R1A_RETAINED_BY_R2=YES'
fi
echo 'VIEW0_V1N1_G04_R1A_G04_R2_IMPLEMENTED=NO'
echo 'VIEW0_V1N1_G04_R1A_G04_GROUP_FREEZE=NO'
echo 'VIEW0_V1N1_G04_R1A_COMPLETE_HTML_CONFORMANCE_CLAIM=NO'
echo 'VIEW0_V1N1_G04_R1A_STAGED_CHANGES=NO'
echo 'VIEW0_V1N1_G04_R1A_NEW_COMMIT_CREATED=NO'
echo 'VIEW0_V1N1_G04_R1A_REMOTE_WRITE_PERFORMED=NO'
