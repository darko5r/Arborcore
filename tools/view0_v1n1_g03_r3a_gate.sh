#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
fail() { echo "FAIL: $*" >&2; exit 1; }

printf '%s\n' '### VIEW0 V1N1 G03 R3A — SCOPE / CONTRACT / F1-R7 DESCENDANT-EXCLUSION OWNERSHIP'
bash tools/view0_v1n1_g03_r3a_scope_verify.sh
bash tools/view0_v1n1_g03_r3a_contract_verify.sh

printf '%s\n' '### VIEW0 V1N1 G03 R3A — RETAINED R2A / R1A / C0-L1 / C0 / G02 / V1N0 LX1 / LOWER LAYERS'
bash tools/view0_v1n1_g03_r2a_gate.sh
echo 'VIEW0_V1N1_G03_R3A_RETAINED_R2A_R1A_LX1=PASS'

printf '%s\n' '### VIEW0 V1N1 G03 R3A — CLEAN DERIVED TEST RESET'
rm -f \
    build/view0-v1/native/native.o \
    build/view0-v1/native/g03_r1a.o \
    build/view0-v1/native/g03_r3a.o \
    build/view0-v1/native/main.o \
    build/view0-v1/native/g03_r3a_descendant_exclusions_test.o \
    build/view0-v1/native/g03_r3a_descendant_exclusions_adversarial_test.o \
    build/view0-v1/native/g03-r3a-descendant-exclusions-test \
    build/view0-v1/native/g03-r3a-descendant-exclusions-adversarial-test \
    build/view0-v1/native/g03-r3a-descendant-exclusions-sanitize-test \
    build/view0-v1/native/arborcore-view0-html-check
for stale in \
    build/view0-v1/native/g03_r3a_descendant_exclusions_test.o \
    build/view0-v1/native/g03_r3a_descendant_exclusions_adversarial_test.o \
    build/view0-v1/native/g03-r3a-descendant-exclusions-test \
    build/view0-v1/native/g03-r3a-descendant-exclusions-adversarial-test \
    build/view0-v1/native/g03-r3a-descendant-exclusions-sanitize-test; do
    [[ ! -e "$stale" ]] || fail "R3A derived output survived reset: $stale"
done
echo 'VIEW0_V1N1_G03_R3A_DERIVED_TEST_RESET=PASS'

printf '%s\n' '### VIEW0 V1N1 G03 R3A — NATIVE / ADVERSARIAL / ANALYZER / CLI'
bash tools/view0_v1n1_g03_r3a_native_verify.sh

printf '%s\n' '### VIEW0 V1N1 G03 R3A — ASAN / UBSAN WITH LX1 SANITIZED LEXBOR'
make -s view0-v1n1-g03-r3a-sanitize
[[ -x build/view0-v1/native/g03-r3a-descendant-exclusions-sanitize-test ]] || fail 'fresh R3A sanitizer test was not rebuilt'
echo 'VIEW0_V1N1_G03_R3A_SANITIZE_REBUILD=PASS'

printf '%s\n' '### VIEW0 V1N1 G03 R3A — FINAL DIFF / POLICY'
git diff --check
[[ -z "$(git diff --cached --name-only)" ]] || fail 'Git index changed during R3A gate'
current_version=$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' view/arborcore-view-core-1.contract | head -n1)
case "$current_version" in
  0.1-VIEW0-V1N1-G03-R3A|0.1-VIEW0-V1N1-G03-R4A) ;;
  *) fail "unexpected final R3A extension contract version: $current_version" ;;
esac
[[ "$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)" -eq 11 ]] || fail 'production VIEW API count changed'
if [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R3A' ]]; then
    ! grep -ERq 'ARBOR_VIEW_V1_G03_(NOTHING_MODEL|EXPLICIT_HTML_ELEMENT_ALLOWANCE|SCALAR_VALUE_TEXT|PALPABLE_PHRASING_NONEMPTY)|0x000000003003000[4-7]' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G03 R4-R7 implementation appeared'
else
    ! grep -ERq 'ARBOR_VIEW_V1_G03_(EXPLICIT_HTML_ELEMENT_ALLOWANCE|SCALAR_VALUE_TEXT|PALPABLE_PHRASING_NONEMPTY)|0x000000003003000[5-7]' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G03 R5-R7 implementation appeared under R4A'
fi
[[ "$(sha256sum build/view0-v1/native/lexbor-src/source/lexbor/html/tree/insertion_mode/in_body.c | awk '{print $1}')" == '28b8b1d15329f5f387005982a9a2788a16f66696505f740249f548e400be22ef' ]] || fail 'canonical Lexbor cache changed'
[[ "$(sha256sum build/view0-v1/native/lexbor-compat-src/source/lexbor/html/tree/insertion_mode/in_body.c | awk '{print $1}')" == '142a2f08ea17ab803a91bf6c6af348a35ac1bf787b2c2d43016de79424e991d8' ]] || fail 'LX1 derived compatibility source changed'

echo 'VIEW0_V1N1_G03_R3A_GATE=PASS'
echo 'VIEW0_V1N1_G03_R3A_RETAINED_R2A_R1A_LX1=PASS'
echo 'VIEW0_V1N1_G03_R3A_RULE_ID=0x0000000030030003'
echo 'VIEW0_V1N1_G03_R3A_ELEMENT_DEFINITION_COUNT=20'
echo 'VIEW0_V1N1_G03_R3A_STATIC_PARENT_COUNT=13'
echo 'VIEW0_V1N1_G03_R3A_PARTIAL_PARENT_COUNT=6'
echo 'VIEW0_V1N1_G03_R3A_DEFERRED_PARENT_COUNT=1'
echo 'VIEW0_V1N1_G03_R3A_EXPLICIT_DEFERRED_BRANCH_FAMILY_COUNT=5'
echo 'VIEW0_V1N1_G03_R3A_IMPLEMENTATION_COMPLETE=NO'
if [[ "$current_version" == '0.1-VIEW0-V1N1-G03-R3A' ]]; then
    echo 'VIEW0_V1N1_G03_R3A_G03_R4_R7_RULE_IDS_IMPLEMENTED=ZERO'
else
    echo 'VIEW0_V1N1_G03_R3A_RETAINED_UNDER_G03_R4A=PASS'
    echo 'VIEW0_V1N1_G03_R3A_G03_R5_R7_RULE_IDS_IMPLEMENTED=ZERO'
fi
echo 'VIEW0_V1N1_G03_R3A_G03_GROUP_FREEZE=NO'
echo 'VIEW0_V1N1_G03_R3A_COMPLETE_HTML_CONFORMANCE_CLAIM=NO'
echo 'VIEW0_V1N1_G03_R3A_STAGED_CHANGES=NO'
echo 'VIEW0_V1N1_G03_R3A_NEW_COMMIT_CREATED=NO'
echo 'VIEW0_V1N1_G03_R3A_REMOTE_WRITE_PERFORMED=NO'
echo 'NEXT_REQUIRED_STEP=RETURN_COMPLETE_G03_R3A_CONSTRUCTION_AND_GATE_OUTPUT_FOR_REVIEW_BEFORE_NEXT_G03_BOUNDARY'
