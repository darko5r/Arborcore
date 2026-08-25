#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"

fail() { echo "FAIL: $*" >&2; exit 1; }

printf '%s\n' '### VIEW0 V1N1 G03 R1A — SCOPE / CONTRACT / 62-CONTEXT COVERAGE'
bash tools/view0_v1n1_g03_r1a_scope_verify.sh
bash tools/view0_v1n1_g03_r1a_contract_verify.sh

printf '%s\n' '### VIEW0 V1N1 G03 R1A — RETAINED G03 C0-L1 / G03 C0 / G02 / V1N0 / LOWER-LAYER REGRESSION'
bash tools/view0_v1n1_g03_c0_l1_gate.sh
echo 'VIEW0_V1N1_G03_R1A_RETAINED_G03_C0_L1=PASS'

printf '%s\n' '### VIEW0 V1N1 G03 R1A — CLEAN DERIVED TEST RESET'
rm -f \
    build/view0-v1/native/native.o \
    build/view0-v1/native/g03_r1a.o \
    build/view0-v1/native/g03_r1a_element_context_test.o \
    build/view0-v1/native/g03_r1a_element_context_adversarial_test.o \
    build/view0-v1/native/g03-r1a-element-context-test \
    build/view0-v1/native/g03-r1a-element-context-adversarial-test \
    build/view0-v1/native/g03-r1a-element-context-sanitize-test
for stale in \
    build/view0-v1/native/g03_r1a_element_context_test.o \
    build/view0-v1/native/g03_r1a_element_context_adversarial_test.o \
    build/view0-v1/native/g03-r1a-element-context-test \
    build/view0-v1/native/g03-r1a-element-context-adversarial-test \
    build/view0-v1/native/g03-r1a-element-context-sanitize-test; do
    [[ ! -e "$stale" ]] || fail "R1A derived output survived reset: $stale"
done
echo 'VIEW0_V1N1_G03_R1A_DERIVED_TEST_RESET=PASS'

printf '%s\n' '### VIEW0 V1N1 G03 R1A — NATIVE / ADVERSARIAL / ANALYZER'
bash tools/view0_v1n1_g03_r1a_native_verify.sh

printf '%s\n' '### VIEW0 V1N1 G03 R1A — ASAN / UBSAN WITH SANITIZED LEXBOR'
make -s view0-v1n1-g03-r1a-sanitize
[[ -x build/view0-v1/native/g03-r1a-element-context-sanitize-test ]] || fail 'fresh R1A sanitizer test was not rebuilt'
echo 'VIEW0_V1N1_G03_R1A_SANITIZE_REBUILD=PASS'

printf '%s\n' '### VIEW0 V1N1 G03 R1A — FINAL DIFF / POLICY'
git diff --check
[[ -z "$(git diff --cached --name-only)" ]] || fail 'Git index changed during R1A gate'
current_version=$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' view/arborcore-view-core-1.contract | head -n1)
case "$current_version" in
  0.1-VIEW0-V1N1-G03-R1A) extension_r2a=NO; extension_r3a=NO; extension_r4a=NO ;;
  0.1-VIEW0-V1N1-G03-R2A) extension_r2a=YES; extension_r3a=NO; extension_r4a=NO ;;
  0.1-VIEW0-V1N1-G03-R3A) extension_r2a=YES; extension_r3a=YES; extension_r4a=NO ;;
  0.1-VIEW0-V1N1-G03-R4A) extension_r2a=YES; extension_r3a=YES; extension_r4a=YES ;;
  *) fail "unexpected final contract version: $current_version" ;;
esac
[[ "$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)" -eq 11 ]] || fail 'production VIEW API count changed'
if [[ "$extension_r2a" == NO ]]; then
    ! grep -ERq 'ARBOR_VIEW_V1_G03_(CONTENT_MODEL|DESCENDANT_EXCLUSIONS|NOTHING_MODEL|EXPLICIT_HTML_ELEMENT_ALLOWANCE|SCALAR_VALUE_TEXT|PALPABLE_PHRASING_NONEMPTY)|0x000000003003000[2-7]' \
        tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G03 R2-R7 implementation appeared'
elif [[ "$extension_r3a" == NO ]]; then
    ! grep -ERq 'ARBOR_VIEW_V1_G03_(DESCENDANT_EXCLUSIONS|NOTHING_MODEL|EXPLICIT_HTML_ELEMENT_ALLOWANCE|SCALAR_VALUE_TEXT|PALPABLE_PHRASING_NONEMPTY)|0x000000003003000[3-7]' \
        tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G03 R3-R7 implementation appeared under R2A'
elif [[ "$extension_r4a" == NO ]]; then
    ! grep -ERq 'ARBOR_VIEW_V1_G03_(NOTHING_MODEL|EXPLICIT_HTML_ELEMENT_ALLOWANCE|SCALAR_VALUE_TEXT|PALPABLE_PHRASING_NONEMPTY)|0x000000003003000[4-7]' \
        tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G03 R4-R7 implementation appeared under R3A'
else
    ! grep -ERq 'ARBOR_VIEW_V1_G03_(EXPLICIT_HTML_ELEMENT_ALLOWANCE|SCALAR_VALUE_TEXT|PALPABLE_PHRASING_NONEMPTY)|0x000000003003000[5-7]' \
        tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G03 R5-R7 implementation appeared under R4A'
fi

echo 'VIEW0_V1N1_G03_R1A_GATE=PASS'
echo 'VIEW0_V1N1_G03_R1A_RETAINED_G03_C0_L1=PASS'
echo 'VIEW0_V1N1_G03_R1A_RULE_ID=0x0000000030030001'
echo 'VIEW0_V1N1_G03_R1A_CONTEXT_ALTERNATIVE_COUNT=62'
echo 'VIEW0_V1N1_G03_R1A_IMPLEMENTED_STABLE_CONTEXT_COUNT=58'
echo 'VIEW0_V1N1_G03_R1A_DELEGATED_G02_CONTEXT_COUNT=2'
echo 'VIEW0_V1N1_G03_R1A_PARTIAL_DEFERRED_CONTEXT_COUNT=1'
echo 'VIEW0_V1N1_G03_R1A_MODE_UNREACHABLE_CONTEXT_COUNT=1'
echo 'VIEW0_V1N1_G03_R1A_DEFERRED_BRANCH=MAIN_FORM_ACCESSIBLE_NAME'
echo 'VIEW0_V1N1_G03_R1A_IMPLEMENTATION_COMPLETE=NO'
if [[ "$extension_r2a" == NO ]]; then
    echo 'VIEW0_V1N1_G03_R1A_G03_R2_R7_RULE_IDS_IMPLEMENTED=ZERO'
elif [[ "$extension_r3a" == NO ]]; then
    echo 'VIEW0_V1N1_G03_R1A_RETAINED_UNDER_G03_R2A=PASS'
    echo 'VIEW0_V1N1_G03_R1A_G03_R3_R7_RULE_IDS_IMPLEMENTED=ZERO'
elif [[ "$extension_r4a" == NO ]]; then
    echo 'VIEW0_V1N1_G03_R1A_RETAINED_UNDER_G03_R3A=PASS'
    echo 'VIEW0_V1N1_G03_R1A_PRIVATE_OFFSET_COLLECTION=PASS'
    echo 'VIEW0_V1N1_G03_R1A_G03_R4_R7_RULE_IDS_IMPLEMENTED=ZERO'
else
    echo 'VIEW0_V1N1_G03_R1A_RETAINED_UNDER_G03_R4A=PASS'
    echo 'VIEW0_V1N1_G03_R1A_PRIVATE_OFFSET_COLLECTION=PASS'
    echo 'VIEW0_V1N1_G03_R1A_G03_R5_R7_RULE_IDS_IMPLEMENTED=ZERO'
fi
echo 'VIEW0_V1N1_G03_R1A_G03_GROUP_FREEZE=NO'
echo 'VIEW0_V1N1_G03_R1A_COMPLETE_HTML_CONFORMANCE_CLAIM=NO'
echo 'VIEW0_V1N1_G03_R1A_STAGED_CHANGES=NO'
echo 'VIEW0_V1N1_G03_R1A_NEW_COMMIT_CREATED=NO'
echo 'VIEW0_V1N1_G03_R1A_REMOTE_WRITE_PERFORMED=NO'
echo 'NEXT_REQUIRED_STEP=RETURN_COMPLETE_G03_R1A_CONSTRUCTION_AND_GATE_OUTPUT_FOR_REVIEW_BEFORE_G03_R2'
