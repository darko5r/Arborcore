#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
fail(){ echo "FAIL: $*" >&2; exit 1; }

echo '### VIEW0 V1N1 G04 R2 — SCOPE / RETAINED R1 / CONTRACT'
bash tools/view0_v1n1_g04_r2_scope_verify.sh
bash tools/view0_v1n1_g04_r1a_scope_verify.sh
bash tools/view0_v1n1_g04_r1b_scope_verify.sh
bash tools/view0_v1n1_g04_r1c_scope_verify.sh
bash tools/view0_v1n1_g04_r1a_contract_verify.sh
bash tools/view0_v1n1_g04_r1b_contract_verify.sh
bash tools/view0_v1n1_g04_r1c_contract_verify.sh
bash tools/view0_v1n1_g04_r2_contract_verify.sh

echo '### VIEW0 V1N1 G04 R2 — CLEAN DERIVED NATIVE/TEST RESET'
rm -f \
 build/view0-v1/native/native.o build/view0-v1/native/lexbor_adapter.o \
 build/view0-v1/native/g04_r1a.o build/view0-v1/native/g04_r2a.o build/view0-v1/native/main.o \
 build/view0-v1/native/g04_r1a_transparent_parent_model_test.o \
 build/view0-v1/native/g04_r1a_transparent_parent_model_adversarial_test.o \
 build/view0-v1/native/g04_r1a_global_failure_atomicity_test.o \
 build/view0-v1/native/g04_r1b_transparent_parent_model_test.o \
 build/view0-v1/native/g04_r1b_transparent_parent_model_adversarial_test.o \
 build/view0-v1/native/g04_r1c_noscript_transparent_test.o \
 build/view0-v1/native/g04_r1c_noscript_transparent_adversarial_test.o \
 build/view0-v1/native/g04_r2_parentless_flow_test.o \
 build/view0-v1/native/g04_r2_parentless_flow_adversarial_test.o \
 build/view0-v1/native/g04_r2_global_failure_atomicity_test.o \
 build/view0-v1/native/g04-r1a-transparent-parent-model-test \
 build/view0-v1/native/g04-r1a-transparent-parent-model-adversarial-test \
 build/view0-v1/native/g04-r1a-global-failure-atomicity-test \
 build/view0-v1/native/g04-r1b-transparent-parent-model-test \
 build/view0-v1/native/g04-r1b-transparent-parent-model-adversarial-test \
 build/view0-v1/native/g04-r1c-noscript-transparent-test \
 build/view0-v1/native/g04-r1c-noscript-transparent-adversarial-test \
 build/view0-v1/native/g04-r2-parentless-flow-test \
 build/view0-v1/native/g04-r2-parentless-flow-adversarial-test \
 build/view0-v1/native/g04-r2-global-failure-atomicity-test \
 build/view0-v1/native/g04-r1a-transparent-parent-model-sanitize-test \
 build/view0-v1/native/g04-r1b-transparent-parent-model-sanitize-test \
 build/view0-v1/native/g04-r1c-noscript-transparent-sanitize-test \
 build/view0-v1/native/g04-r2-parentless-flow-sanitize-test \
 build/view0-v1/native/arborcore-view0-html-check
for stale in \
 build/view0-v1/native/g04-r1a-transparent-parent-model-test \
 build/view0-v1/native/g04-r1b-transparent-parent-model-test \
 build/view0-v1/native/g04-r1c-noscript-transparent-test \
 build/view0-v1/native/g04-r2-parentless-flow-test \
 build/view0-v1/native/g04-r2-parentless-flow-adversarial-test \
 build/view0-v1/native/g04-r2-global-failure-atomicity-test; do [[ ! -e "$stale" ]] || fail "derived reset failed: $stale"; done
echo 'VIEW0_V1N1_G04_R2_DERIVED_RESET=PASS'
echo 'VIEW0_V1N1_G04_R2_RETAINED_G04_TEST_RESET=PASS'

echo '### VIEW0 V1N1 G04 R2 — RETAINED + R2 NATIVE QUALIFICATION'
bash tools/view0_v1n1_g04_r2_native_verify.sh

echo '### VIEW0 V1N1 G04 R2 — ASAN / UBSAN'
make -s view0-v1n1-g04-r2-sanitize
[[ -x build/view0-v1/native/g04-r2-parentless-flow-sanitize-test ]] || fail 'R2 sanitizer binary missing'
echo 'VIEW0_V1N1_G04_R2_SANITIZE_REBUILD=PASS'

git diff --check
[[ -z "$(git diff --cached --name-only)" ]] || fail 'Git index changed during G04 R2 gate'
[[ "$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' view/arborcore-view-core-1.contract | head -1)" == '0.1-VIEW0-V1N1-G04-R2' ]] || fail 'top contract drift'
[[ "$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)" -eq 11 ]] || fail 'production VIEW API count changed'
[[ "$(grep -Fc 'lxb_html_parse_fragment_chunk_begin(' tools/c/view0_conformance/lexbor_adapter.c)" -eq 1 ]] || fail 'fragment parser entry drift'
[[ "$(grep -Fc 'lxb_html_parser_scripting_set(parser, false);' tools/c/view0_conformance/lexbor_adapter.c)" -eq 2 ]] || fail 'document+fragment scripting bind drift'

echo 'VIEW0_V1N1_G04_R2_GATE=PASS'
echo 'VIEW0_V1N1_G04_R2_FRAGMENT_MODE=EXPLICIT_DEVELOPMENT_TOOL_ONLY'
echo 'VIEW0_V1N1_G04_R2_FRAGMENT_CONTEXT=BODY_HTML_NAMESPACE'
echo 'VIEW0_V1N1_G04_R2_STANDARD_ELEMENT_IMPLEMENTATION_COMPLETE=YES'
echo 'VIEW0_V1N1_G04_R2_IMPLEMENTATION_COMPLETE=NO_G13_EXTERNAL_DEPENDENCY'
echo 'VIEW0_V1N1_G04_R2_G04_STANDARD_ELEMENT_IMPLEMENTATION_COMPLETE=YES_R1_DOCUMENT_AND_R2_FRAGMENT_MODES'
echo 'VIEW0_V1N1_G04_R2_G04_IMPLEMENTATION_COMPLETE=NO_G13_EXTERNAL_DEPENDENCY'
echo 'VIEW0_V1N1_G04_R2_G04_GROUP_FREEZE=NO_PENDING_INDEPENDENT_REVIEW'
echo 'VIEW0_V1N1_G04_R2_COMPLETE_HTML_CONFORMANCE_CLAIM=NO'
echo 'VIEW0_V1N1_G04_R2_STAGED_CHANGES=NO'
echo 'VIEW0_V1N1_G04_R2_NEW_COMMIT_CREATED=NO'
echo 'VIEW0_V1N1_G04_R2_REMOTE_WRITE_PERFORMED=NO'
echo 'PASS: G04 R2 full gate passed'
