#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"

fail() { echo "FAIL: $*" >&2; exit 1; }

printf '%s\n' '### VIEW0 V1N1 G03 C0 — SCOPE / CONTRACT'
bash tools/view0_v1n1_g03_c0_scope_verify.sh
bash tools/view0_v1n1_g03_c0_contract_verify.sh

printf '%s\n' '### VIEW0 V1N1 G03 C0 — RETAINED G02 GF1/R6 FUNCTIONAL REGRESSION'
make -s \
    view0-v1n1-g02-r1-test view0-v1n1-g02-r1-adversarial-test \
    view0-v1n1-g02-r2-test view0-v1n1-g02-r2-adversarial-test \
    view0-v1n1-g02-r3-test view0-v1n1-g02-r3-adversarial-test \
    view0-v1n1-g02-r4-test view0-v1n1-g02-r4-adversarial-test \
    view0-v1n1-g02-r5-test view0-v1n1-g02-r5-adversarial-test \
    view0-v1n1-g02-r6-test view0-v1n1-g02-r6-adversarial-test
bash tools/view0_v1n1_g02_r6_native_verify.sh
make -s view0-v1n1-g02-r6-sanitize

echo 'VIEW0_V1N1_G03_C0_RETAINED_G02_GF1=PASS'

printf '%s\n' '### VIEW0 V1N1 G03 C0 — RETAINED V1N0 / LOWER-LAYER REGRESSION'
bash tools/view0_v1_native_foundation_gate.sh

echo 'VIEW0_V1N1_G03_C0_RETAINED_V1N0_LOWER_LAYERS=PASS'

printf '%s\n' '### VIEW0 V1N1 G03 C0 — CLEAN DERIVED TEST RESET'
rm -f \
    build/view0-v1/native/g03_c0_provenance.o \
    build/view0-v1/native/g03_c0_observation_test.o \
    build/view0-v1/native/g03_c0_observation_adversarial_test.o \
    build/view0-v1/native/g03_c0_nowrap_test.o \
    build/view0-v1/native/g03-c0-observation-test \
    build/view0-v1/native/g03-c0-observation-adversarial-test \
    build/view0-v1/native/g03-c0-nowrap-test \
    build/view0-v1/native/g03-c0-observation-sanitize-test
for stale in \
    build/view0-v1/native/g03_c0_provenance.o \
    build/view0-v1/native/g03_c0_observation_test.o \
    build/view0-v1/native/g03_c0_observation_adversarial_test.o \
    build/view0-v1/native/g03_c0_nowrap_test.o \
    build/view0-v1/native/g03-c0-observation-test \
    build/view0-v1/native/g03-c0-observation-adversarial-test \
    build/view0-v1/native/g03-c0-nowrap-test \
    build/view0-v1/native/g03-c0-observation-sanitize-test; do
    [[ ! -e "$stale" ]] || fail "G03 C0 derived output survived clean reset: $stale"
done
echo 'VIEW0_V1N1_G03_C0_DERIVED_TEST_RESET=PASS'

printf '%s\n' '### VIEW0 V1N1 G03 C0 — NATIVE / ADVERSARIAL / ANALYZER'
bash tools/view0_v1n1_g03_c0_native_verify.sh

printf '%s\n' '### VIEW0 V1N1 G03 C0 — ASAN / UBSAN WITH SANITIZED LEXBOR'
make -s view0-v1n1-g03-c0-sanitize
[[ -x build/view0-v1/native/g03-c0-observation-sanitize-test ]] || fail 'fresh G03 C0 sanitizer test was not rebuilt'
echo 'VIEW0_V1N1_G03_C0_SANITIZE_REBUILD=PASS'

printf '%s\n' '### VIEW0 V1N1 G03 C0 — FINAL DIFF / POLICY'
git diff --check
[[ -z "$(git diff --cached --name-only)" ]] || fail 'Git index changed during G03 C0 gate'
current_version="$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' view/arborcore-view-core-1.contract | head -n1)"
case "$current_version" in
  0.1-VIEW0-V1N1-G03-C0|0.1-VIEW0-V1N1-G03-C0-L1|0.1-VIEW0-V1N1-G03-R1A|0.1-VIEW0-V1N1-G03-R2A|0.1-VIEW0-V1N1-G03-R3A|0.1-VIEW0-V1N1-G03-R4A) ;;
  *) fail "unexpected contract version after retained G03 C0: $current_version" ;;
esac
! grep -ERq 'ARBOR_VIEW_V1_G03_|0x000000003003' tools/c/view0_conformance/lexbor_adapter.c tools/c/view0_conformance/g03_provenance.c || fail 'G03 C0 mechanism files acquired rule semantics'

for symbol in \
    ARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED \
    ARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX \
    ARBOR_VIEW_V1_G02_DOCTYPE_LEGACY_DISCOURAGED \
    ARBOR_VIEW_V1_G02_HEAD_TITLE_CARDINALITY \
    ARBOR_VIEW_V1_G02_HEAD_BASE_CARDINALITY \
    ARBOR_VIEW_V1_G02_BODY_SINGLETON; do
    grep -Fq "$symbol" tools/c/view0_conformance/native.c || fail "frozen G02 symbol disappeared: $symbol"
done

count=$(grep -Ec '^arbor_status arbor_view_[a-z0-9_]+\(' include/arborcore/view.h)
[[ "$count" -eq 11 ]] || fail "production VIEW API count changed: $count"

echo 'VIEW0_V1N1_G03_C0_GATE=PASS'
echo 'VIEW0_V1N1_G03_C0_EXTENSION_AWARE_RETENTION=YES'
echo 'VIEW0_V1N1_G03_C0_G02_GROUP_RETAINED=PASS'
echo 'VIEW0_V1N1_G03_C0_G03_RULE_IDS_IMPLEMENTED=ZERO'
echo 'VIEW0_V1N1_G03_C0_G03_R1_IMPLEMENTED=NO'
echo 'VIEW0_V1N1_G03_R1_CONSTRUCTION_NOW=NO'
echo 'VIEW0_V1N1_G03_C0_STAGED_CHANGES=NO'
echo 'VIEW0_V1N1_G03_C0_NEW_COMMIT_CREATED=NO'
echo 'VIEW0_V1N1_G03_C0_REMOTE_WRITE_PERFORMED=NO'
echo 'NEXT_REQUIRED_STEP=RETURN_COMPLETE_G03_C0_CONSTRUCTION_AND_GATE_OUTPUT_FOR_REVIEW_BEFORE_G03_R1_ELEMENT_CONTEXT'
