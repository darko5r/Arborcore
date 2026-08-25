#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"

fail() { echo "FAIL: $*" >&2; exit 1; }

printf '%s\n' '### VIEW0 V1N1 G02 R6 — SCOPE / CONTRACT'
bash tools/view0_v1n1_g02_r6_scope_verify.sh
bash tools/view0_v1n1_g02_r6_contract_verify.sh

printf '%s\n' '### VIEW0 V1N1 G02 R6 — RETAINED R5/R4/R3/R2/R1/C0/V1N0/LOWER-LAYER REGRESSION'
bash tools/view0_v1n1_g02_r5_gate.sh

printf '%s\n' '### VIEW0 V1N1 G02 R6 — CLEAN DERIVED TEST RESET'
rm -f \
    build/view0-v1/native/g02_r6_body_singleton_test.o \
    build/view0-v1/native/g02_r6_body_singleton_adversarial_test.o \
    build/view0-v1/native/g02-r6-body-singleton-test \
    build/view0-v1/native/g02-r6-body-singleton-adversarial-test \
    build/view0-v1/native/g02-r6-body-singleton-sanitize-test
for stale in \
    build/view0-v1/native/g02_r6_body_singleton_test.o \
    build/view0-v1/native/g02_r6_body_singleton_adversarial_test.o \
    build/view0-v1/native/g02-r6-body-singleton-test \
    build/view0-v1/native/g02-r6-body-singleton-adversarial-test \
    build/view0-v1/native/g02-r6-body-singleton-sanitize-test; do
    [[ ! -e "$stale" ]] || fail "G02 R6 derived output survived clean reset: $stale"
done
echo 'VIEW0_V1N1_G02_R6_DERIVED_TEST_RESET=PASS'

printf '%s\n' '### VIEW0 V1N1 G02 R6 — NATIVE / ADVERSARIAL / ANALYZER / CLI'
bash tools/view0_v1n1_g02_r6_native_verify.sh

printf '%s\n' '### VIEW0 V1N1 G02 R6 — ASAN / UBSAN WITH SANITIZED LEXBOR'
make -s view0-v1n1-g02-r6-sanitize
[[ -x build/view0-v1/native/g02-r6-body-singleton-sanitize-test ]] || fail 'fresh G02 R6 sanitizer test was not rebuilt'
echo 'VIEW0_V1N1_G02_R6_SANITIZE_REBUILD=PASS'

printf '%s\n' '### VIEW0 V1N1 G02 R6 — FINAL DIFF / POLICY'
git diff --check
[[ -z "$(git diff --cached --name-only)" ]] || fail 'Git index changed during G02 R6 gate'

for symbol in \
    ARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED \
    ARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX \
    ARBOR_VIEW_V1_G02_DOCTYPE_LEGACY_DISCOURAGED \
    ARBOR_VIEW_V1_G02_HEAD_TITLE_CARDINALITY \
    ARBOR_VIEW_V1_G02_HEAD_BASE_CARDINALITY \
    ARBOR_VIEW_V1_G02_BODY_SINGLETON; do
    grep -Fq "$symbol" tools/c/view0_conformance/native.c || fail "G02 implementation disappeared: $symbol"
done
! grep -ERq '0x000000003002000[45]' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'retired G02 rule ID reused'
! grep -ERq 'ARBOR_VIEW_V1_G0[3-6]_' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G03-G06 implementation appeared during G02 R6'
[[ "$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' view/arborcore-view-core-1.contract | head -n1)" == '0.1-VIEW0-V1N1-G02-R6' ]] || fail 'unexpected contract version after G02 R6'

echo 'VIEW0_V1N1_G02_R6_GATE=PASS'
echo 'VIEW0_V1N1_G02_R6_G02_RULES_IMPLEMENTED=6_OF_6'
echo 'VIEW0_V1N1_G02_R6_G02_GROUP_IMPLEMENTATION_COMPLETE=YES'
echo 'VIEW0_V1N1_G02_R6_G03_G06_RULES_IMPLEMENTED=ZERO'
echo 'VIEW0_V1N1_G03_CONSTRUCTION_NOW=NO'
echo 'VIEW0_V1N1_G02_R6_STAGED_CHANGES=NO'
echo 'VIEW0_V1N1_G02_R6_NEW_COMMIT_CREATED=NO'
echo 'VIEW0_V1N1_G02_R6_REMOTE_WRITE_PERFORMED=NO'
echo 'NEXT_REQUIRED_STEP=RETURN_COMPLETE_G02_R6_CONSTRUCTION_AND_GATE_OUTPUT_FOR_REVIEW_BEFORE_G02_GROUP_FREEZE'
