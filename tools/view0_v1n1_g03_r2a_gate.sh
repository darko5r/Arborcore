#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
fail() { echo "FAIL: $*" >&2; exit 1; }

printf '%s\n' '### VIEW0 V1N1 G03 R2A — SCOPE / CONTRACT / P2 RESIDUAL OWNERSHIP'
bash tools/view0_v1n1_g03_r2a_scope_verify.sh
bash tools/view0_v1n1_g03_r2a_contract_verify.sh

printf '%s\n' '### VIEW0 V1N1 G03 R2A — RETAINED R1A / C0-L1 / C0 / G02 / V1N0 LX1 / LOWER LAYERS'
bash tools/view0_v1n1_g03_r1a_gate.sh
echo 'VIEW0_V1N1_G03_R2A_RETAINED_R1A_LX1=PASS'

printf '%s\n' '### VIEW0 V1N1 G03 R2A — CLEAN DERIVED TEST RESET'
rm -f \
    build/view0-v1/native/native.o \
    build/view0-v1/native/g03_r2a.o \
    build/view0-v1/native/main.o \
    build/view0-v1/native/g03_r2a_content_model_test.o \
    build/view0-v1/native/g03_r2a_content_model_adversarial_test.o \
    build/view0-v1/native/g03-r2a-content-model-test \
    build/view0-v1/native/g03-r2a-content-model-adversarial-test \
    build/view0-v1/native/g03-r2a-content-model-sanitize-test \
    build/view0-v1/native/arborcore-view0-html-check
for stale in \
    build/view0-v1/native/g03_r2a_content_model_test.o \
    build/view0-v1/native/g03_r2a_content_model_adversarial_test.o \
    build/view0-v1/native/g03-r2a-content-model-test \
    build/view0-v1/native/g03-r2a-content-model-adversarial-test \
    build/view0-v1/native/g03-r2a-content-model-sanitize-test; do
    [[ ! -e "$stale" ]] || fail "R2A derived output survived reset: $stale"
done
echo 'VIEW0_V1N1_G03_R2A_DERIVED_TEST_RESET=PASS'

printf '%s\n' '### VIEW0 V1N1 G03 R2A — NATIVE / ADVERSARIAL / ANALYZER / CLI'
bash tools/view0_v1n1_g03_r2a_native_verify.sh

printf '%s\n' '### VIEW0 V1N1 G03 R2A — ASAN / UBSAN WITH LX1 SANITIZED LEXBOR'
make -s view0-v1n1-g03-r2a-sanitize
[[ -x build/view0-v1/native/g03-r2a-content-model-sanitize-test ]] || fail 'fresh R2A sanitizer test was not rebuilt'
echo 'VIEW0_V1N1_G03_R2A_SANITIZE_REBUILD=PASS'

printf '%s\n' '### VIEW0 V1N1 G03 R2A — FINAL DIFF / POLICY'
git diff --check
[[ -z "$(git diff --cached --name-only)" ]] || fail 'Git index changed during R2A gate'
current_version=$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' view/arborcore-view-core-1.contract | head -n1)
case "$current_version" in
  0.1-VIEW0-V1N1-G03-R2A) retained_under_r3a=NO; retained_under_r4a=NO ;;
  0.1-VIEW0-V1N1-G03-R3A) retained_under_r3a=YES; retained_under_r4a=NO ;;
  0.1-VIEW0-V1N1-G03-R4A) retained_under_r3a=YES; retained_under_r4a=YES ;;
  *) fail "unexpected final R2A extension contract version: $current_version" ;;
esac
[[ "$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)" -eq 11 ]] || fail 'production VIEW API count changed'
if [[ "$retained_under_r3a" == NO ]]; then
    ! grep -ERq 'ARBOR_VIEW_V1_G03_(DESCENDANT_EXCLUSIONS|NOTHING_MODEL|EXPLICIT_HTML_ELEMENT_ALLOWANCE|SCALAR_VALUE_TEXT|PALPABLE_PHRASING_NONEMPTY)|0x000000003003000[3-7]' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G03 R3-R7 implementation appeared'
elif [[ "$retained_under_r4a" == NO ]]; then
    ! grep -ERq 'ARBOR_VIEW_V1_G03_(NOTHING_MODEL|EXPLICIT_HTML_ELEMENT_ALLOWANCE|SCALAR_VALUE_TEXT|PALPABLE_PHRASING_NONEMPTY)|0x000000003003000[4-7]' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G03 R4-R7 implementation appeared under R3A'
else
    ! grep -ERq 'ARBOR_VIEW_V1_G03_(EXPLICIT_HTML_ELEMENT_ALLOWANCE|SCALAR_VALUE_TEXT|PALPABLE_PHRASING_NONEMPTY)|0x000000003003000[5-7]' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G03 R5-R7 implementation appeared under R4A'
fi
[[ "$(sha256sum build/view0-v1/native/lexbor-src/source/lexbor/html/tree/insertion_mode/in_body.c | awk '{print $1}')" == '28b8b1d15329f5f387005982a9a2788a16f66696505f740249f548e400be22ef' ]] || fail 'canonical Lexbor cache changed'
[[ "$(sha256sum build/view0-v1/native/lexbor-compat-src/source/lexbor/html/tree/insertion_mode/in_body.c | awk '{print $1}')" == '142a2f08ea17ab803a91bf6c6af348a35ac1bf787b2c2d43016de79424e991d8' ]] || fail 'LX1 derived compatibility source changed'

echo 'VIEW0_V1N1_G03_R2A_GATE=PASS'
echo 'VIEW0_V1N1_G03_R2A_RETAINED_R1A_LX1=PASS'
echo 'VIEW0_V1N1_G03_R2A_RULE_ID=0x0000000030030002'
echo 'VIEW0_V1N1_G03_R2A_PREDICATE_COUNT=28'
echo 'VIEW0_V1N1_G03_R2A_IMPLEMENT_NOW_COUNT=18'
echo 'VIEW0_V1N1_G03_R2A_EXPLICIT_DEFER_COUNT=6'
echo 'VIEW0_V1N1_G03_R2A_DELEGATED_OWNER_COUNT=4'
echo 'VIEW0_V1N1_G03_R2A_IMPLEMENTATION_COMPLETE=NO'
if [[ "$retained_under_r3a" == NO ]]; then
    echo 'VIEW0_V1N1_G03_R2A_G03_R3_R7_RULE_IDS_IMPLEMENTED=ZERO'
elif [[ "$retained_under_r4a" == NO ]]; then
    echo 'VIEW0_V1N1_G03_R2A_RETAINED_UNDER_G03_R3A=PASS'
    echo 'VIEW0_V1N1_G03_R2A_G03_R4_R7_RULE_IDS_IMPLEMENTED=ZERO'
else
    echo 'VIEW0_V1N1_G03_R2A_RETAINED_UNDER_G03_R4A=PASS'
    echo 'VIEW0_V1N1_G03_R2A_G03_R5_R7_RULE_IDS_IMPLEMENTED=ZERO'
fi
echo 'VIEW0_V1N1_G03_R2A_G03_GROUP_FREEZE=NO'
echo 'VIEW0_V1N1_G03_R2A_COMPLETE_HTML_CONFORMANCE_CLAIM=NO'
echo 'VIEW0_V1N1_G03_R2A_STAGED_CHANGES=NO'
echo 'VIEW0_V1N1_G03_R2A_NEW_COMMIT_CREATED=NO'
echo 'VIEW0_V1N1_G03_R2A_REMOTE_WRITE_PERFORMED=NO'
echo 'NEXT_REQUIRED_STEP=RETURN_COMPLETE_G03_R2A_CONSTRUCTION_AND_GATE_OUTPUT_FOR_REVIEW_BEFORE_NEXT_G03_BOUNDARY'
