#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"

fail() { echo "FAIL: $*" >&2; exit 1; }

printf '%s\n' '### VIEW0 V1N1 G03 C0-L1 — SCOPE / CONTRACT'
bash tools/view0_v1n1_g03_c0_l1_scope_verify.sh
bash tools/view0_v1n1_g03_c0_l1_contract_verify.sh

printf '%s\n' '### VIEW0 V1N1 G03 C0-L1 — RETAINED G03 C0/G02/V1N0/LOWER-LAYER REGRESSION'
bash tools/view0_v1n1_g03_c0_gate.sh
echo 'VIEW0_V1N1_G03_C0_L1_RETAINED_G03_C0=PASS'

printf '%s\n' '### VIEW0 V1N1 G03 C0-L1 — CLEAN DERIVED TEST RESET'
rm -f \
    build/view0-v1/native/lexbor_adapter.o \
    build/view0-v1/native/g03_c0_provenance.o \
    build/view0-v1/native/g03_c0_lifecycle_test.o \
    build/view0-v1/native/g03_c0_lifecycle_adversarial_test.o \
    build/view0-v1/native/g03-c0-lifecycle-test \
    build/view0-v1/native/g03-c0-lifecycle-adversarial-test \
    build/view0-v1/native/g03-c0-lifecycle-sanitize-test
for stale in \
    build/view0-v1/native/g03_c0_lifecycle_test.o \
    build/view0-v1/native/g03_c0_lifecycle_adversarial_test.o \
    build/view0-v1/native/g03-c0-lifecycle-test \
    build/view0-v1/native/g03-c0-lifecycle-adversarial-test \
    build/view0-v1/native/g03-c0-lifecycle-sanitize-test; do
    [[ ! -e "$stale" ]] || fail "G03 C0-L1 derived output survived reset: $stale"
done
echo 'VIEW0_V1N1_G03_C0_L1_DERIVED_TEST_RESET=PASS'

printf '%s\n' '### VIEW0 V1N1 G03 C0-L1 — NATIVE / ADVERSARIAL / ANALYZER'
bash tools/view0_v1n1_g03_c0_l1_native_verify.sh

printf '%s\n' '### VIEW0 V1N1 G03 C0-L1 — ASAN / UBSAN WITH SANITIZED LEXBOR'
make -s view0-v1n1-g03-c0-l1-sanitize
[[ -x build/view0-v1/native/g03-c0-lifecycle-sanitize-test ]] || fail 'fresh G03 C0-L1 sanitizer test was not rebuilt'
echo 'VIEW0_V1N1_G03_C0_L1_SANITIZE_REBUILD=PASS'

printf '%s\n' '### VIEW0 V1N1 G03 C0-L1 — FINAL DIFF / POLICY'
git diff --check
[[ -z "$(git diff --cached --name-only)" ]] || fail 'Git index changed during G03 C0-L1 gate'
current_version="$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' view/arborcore-view-core-1.contract | head -n1)"
case "$current_version" in
  0.1-VIEW0-V1N1-G03-C0-L1|0.1-VIEW0-V1N1-G03-R1A|0.1-VIEW0-V1N1-G03-R2A|0.1-VIEW0-V1N1-G03-R3A|0.1-VIEW0-V1N1-G03-R4A) ;;
  *) fail "unexpected final contract version after retained G03 C0-L1: $current_version" ;;
esac
! grep -ERq 'ARBOR_VIEW_V1_G03_|0x000000003003' tools/c/view0_conformance/lexbor_adapter.c tools/c/view0_conformance/g03_provenance.c || fail 'G03 C0-L1 mechanism files acquired rule semantics'
[[ "$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)" -eq 11 ]] || fail 'production VIEW API count changed'

echo 'VIEW0_V1N1_G03_C0_L1_GATE=PASS'
echo 'VIEW0_V1N1_G03_C0_L1_RETAINED_G03_C0=PASS'
echo 'VIEW0_V1N1_G03_C0_L1_MAX_QUALIFIED_DEPTH=4097'
echo 'VIEW0_V1N1_G03_C0_L1_ACCESSIBLE_NAME_SUPPORT=NO'
echo 'VIEW0_V1N1_G03_C0_L1_G03_RULE_IDS_IMPLEMENTED=ZERO'
echo 'VIEW0_V1N1_G03_C0_L1_G03_R1_IMPLEMENTED=NO'
echo 'VIEW0_V1N1_G03_C0_L1_STAGED_CHANGES=NO'
echo 'VIEW0_V1N1_G03_C0_L1_NEW_COMMIT_CREATED=NO'
echo 'VIEW0_V1N1_G03_C0_L1_REMOTE_WRITE_PERFORMED=NO'
echo 'NEXT_REQUIRED_STEP=RETURN_COMPLETE_G03_C0_L1_GATE_OUTPUT_FOR_REVIEW_BEFORE_ACCESSIBLE_NAME_SUPPORT_P0'
