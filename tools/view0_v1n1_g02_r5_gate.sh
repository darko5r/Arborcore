#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"

fail() { echo "FAIL: $*" >&2; exit 1; }

printf '%s\n' '### VIEW0 V1N1 G02 R5 — SCOPE / CONTRACT'
bash tools/view0_v1n1_g02_r5_scope_verify.sh
bash tools/view0_v1n1_g02_r5_contract_verify.sh

printf '%s\n' '### VIEW0 V1N1 G02 R5 — RETAINED R4/R3/R2/R1/C0/V1N0/LOWER-LAYER REGRESSION'
bash tools/view0_v1n1_g02_r4_gate.sh

printf '%s\n' '### VIEW0 V1N1 G02 R5 — CLEAN DERIVED TEST RESET'
rm -f \
    build/view0-v1/native/g02_r5_head_base_cardinality_test.o \
    build/view0-v1/native/g02_r5_head_base_cardinality_adversarial_test.o \
    build/view0-v1/native/g02-r5-head-base-cardinality-test \
    build/view0-v1/native/g02-r5-head-base-cardinality-adversarial-test \
    build/view0-v1/native/g02-r5-head-base-cardinality-sanitize-test
for stale in \
    build/view0-v1/native/g02_r5_head_base_cardinality_test.o \
    build/view0-v1/native/g02_r5_head_base_cardinality_adversarial_test.o \
    build/view0-v1/native/g02-r5-head-base-cardinality-test \
    build/view0-v1/native/g02-r5-head-base-cardinality-adversarial-test \
    build/view0-v1/native/g02-r5-head-base-cardinality-sanitize-test; do
    [[ ! -e "$stale" ]] || fail "G02 R5 derived output survived clean reset: $stale"
done
echo 'VIEW0_V1N1_G02_R5_DERIVED_TEST_RESET=PASS'

printf '%s\n' '### VIEW0 V1N1 G02 R5 — NATIVE / ADVERSARIAL / ANALYZER / CLI'
bash tools/view0_v1n1_g02_r5_native_verify.sh

printf '%s\n' '### VIEW0 V1N1 G02 R5 — ASAN / UBSAN WITH SANITIZED LEXBOR'
make -s view0-v1n1-g02-r5-sanitize
[[ -x build/view0-v1/native/g02-r5-head-base-cardinality-sanitize-test ]] || fail 'fresh G02 R5 sanitizer test was not rebuilt'
echo 'VIEW0_V1N1_G02_R5_SANITIZE_REBUILD=PASS'

printf '%s\n' '### VIEW0 V1N1 G02 R5 — FINAL DIFF / POLICY'
git diff --check
[[ -z "$(git diff --cached --name-only)" ]] || fail 'Git index changed during G02 R5 gate'

for symbol in \
    ARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED \
    ARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX \
    ARBOR_VIEW_V1_G02_DOCTYPE_LEGACY_DISCOURAGED \
    ARBOR_VIEW_V1_G02_HEAD_TITLE_CARDINALITY \
    ARBOR_VIEW_V1_G02_HEAD_BASE_CARDINALITY; do
    grep -Fq "$symbol" tools/c/view0_conformance/native.c || fail "G02 implementation disappeared: $symbol"
done
! grep -ERq '0x000000003002000[45]' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'retired G02 rule ID reused'
current_version=$(sed -n 's/^ARBORCORE_VIEW_CORE_VERSION=//p' view/arborcore-view-core-1.contract | head -n1)
if [[ "$current_version" == '0.1-VIEW0-V1N1-G02-R5' ]]; then
    ! grep -ERq 'ARBOR_VIEW_V1_G02_BODY_SINGLETON|0x0000000030020008' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G02 R6 implemented prematurely'
elif [[ "$current_version" != '0.1-VIEW0-V1N1-G02-R6' ]]; then
    fail "unexpected higher-stage contract under retained R5 gate: $current_version"
fi
! grep -ERq 'ARBOR_VIEW_V1_G0[3-6]_' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G03-G06 implementation appeared during G02 R5'

echo 'VIEW0_V1N1_G02_R5_GATE=PASS'
echo 'VIEW0_V1N1_G02_R5_EXTENSION_AWARE_RETENTION=YES'
echo 'VIEW0_V1N1_G02_R5_G02_RULES_IMPLEMENTED=5_OF_6'
echo 'VIEW0_V1N1_G02_R5_G03_G06_RULES_IMPLEMENTED=ZERO'
echo 'VIEW0_V1N1_G02_R5_STAGED_CHANGES=NO'
echo 'VIEW0_V1N1_G02_R5_NEW_COMMIT_CREATED=NO'
echo 'VIEW0_V1N1_G02_R5_REMOTE_WRITE_PERFORMED=NO'
echo 'NEXT_REQUIRED_STEP=RETURN_COMPLETE_G02_R5_GATE_OUTPUT_FOR_REVIEW_BEFORE_G02_R6_BODY_SINGLETON'
