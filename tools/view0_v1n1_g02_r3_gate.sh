#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"

fail() { echo "FAIL: $*" >&2; exit 1; }

printf '%s\n' '### VIEW0 V1N1 G02 R3 — SCOPE / CONTRACT'
bash tools/view0_v1n1_g02_r3_scope_verify.sh
bash tools/view0_v1n1_g02_r3_contract_verify.sh

printf '%s\n' '### VIEW0 V1N1 G02 R3 — RETAINED R2/R1/C0/V1N0/LOWER-LAYER REGRESSION'
bash tools/view0_v1n1_g02_r2_gate.sh

printf '%s\n' '### VIEW0 V1N1 G02 R3 — CLEAN DERIVED TEST RESET'
rm -f \
    build/view0-v1/native/g02_r3_doctype_legacy_discouraged_test.o \
    build/view0-v1/native/g02_r3_doctype_legacy_discouraged_adversarial_test.o \
    build/view0-v1/native/g02-r3-doctype-legacy-discouraged-test \
    build/view0-v1/native/g02-r3-doctype-legacy-discouraged-adversarial-test \
    build/view0-v1/native/g02-r3-doctype-legacy-discouraged-sanitize-test
for stale in \
    build/view0-v1/native/g02_r3_doctype_legacy_discouraged_test.o \
    build/view0-v1/native/g02_r3_doctype_legacy_discouraged_adversarial_test.o \
    build/view0-v1/native/g02-r3-doctype-legacy-discouraged-test \
    build/view0-v1/native/g02-r3-doctype-legacy-discouraged-adversarial-test \
    build/view0-v1/native/g02-r3-doctype-legacy-discouraged-sanitize-test; do
    [[ ! -e "$stale" ]] || fail "G02 R3 derived output survived clean reset: $stale"
done
echo 'VIEW0_V1N1_G02_R3_DERIVED_TEST_RESET=PASS'

printf '%s\n' '### VIEW0 V1N1 G02 R3 — NATIVE / ADVERSARIAL / ANALYZER / CLI'
bash tools/view0_v1n1_g02_r3_native_verify.sh

printf '%s\n' '### VIEW0 V1N1 G02 R3 — ASAN / UBSAN WITH SANITIZED LEXBOR'
make -s view0-v1n1-g02-r3-sanitize
[[ -x build/view0-v1/native/g02-r3-doctype-legacy-discouraged-sanitize-test ]] || fail 'fresh G02 R3 sanitizer test was not rebuilt'
echo 'VIEW0_V1N1_G02_R3_SANITIZE_REBUILD=PASS'

printf '%s\n' '### VIEW0 V1N1 G02 R3 — FINAL DIFF / POLICY'
git diff --check
[[ -z "$(git diff --cached --name-only)" ]] || fail 'Git index changed during G02 R3 gate'

active_symbols=$(grep -ERho 'ARBOR_VIEW_V1_G02_[A-Z0-9_]+' \
    tools/c/view0_conformance/native.c \
    tools/include/arborcore/view0_conformance/native.h | LC_ALL=C sort -u)
required_symbols=$'ARBOR_VIEW_V1_G02_DOCTYPE_LEGACY_DISCOURAGED\nARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED\nARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX'
allowed_symbols=$'ARBOR_VIEW_V1_G02_BODY_SINGLETON\nARBOR_VIEW_V1_G02_DOCTYPE_LEGACY_DISCOURAGED\nARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED\nARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX\nARBOR_VIEW_V1_G02_HEAD_BASE_CARDINALITY\nARBOR_VIEW_V1_G02_HEAD_TITLE_CARDINALITY'
while IFS= read -r symbol; do
    [[ -n "$symbol" ]] || continue
    grep -Fxq "$symbol" <<< "$active_symbols" || fail "retained G02 R3 semantic symbol disappeared: $symbol"
done <<< "$required_symbols"
while IFS= read -r symbol; do
    [[ -n "$symbol" ]] || continue
    grep -Fxq "$symbol" <<< "$allowed_symbols" || fail "unexpected active G02 semantic symbol under retained R3 gate: $symbol"
done <<< "$active_symbols"
! grep -ERq '0x000000003002000[45]' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'retired G02 rule ID reused'
! grep -ERq 'ARBOR_VIEW_V1_G0[3-6]_' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G03-G06 implementation appeared during G02 R3'

echo 'VIEW0_V1N1_G02_R3_GATE=PASS'
echo 'VIEW0_V1N1_G02_R3_EXTENSION_AWARE_RETENTION=YES'
echo 'VIEW0_V1N1_G02_R3_G03_G06_RULES_IMPLEMENTED=ZERO'
echo 'VIEW0_V1N1_G02_R3_STAGED_CHANGES=NO'
echo 'VIEW0_V1N1_G02_R3_NEW_COMMIT_CREATED=NO'
echo 'VIEW0_V1N1_G02_R3_REMOTE_WRITE_PERFORMED=NO'
echo 'NEXT_REQUIRED_STEP=RETURN_COMPLETE_G02_R3_GATE_OUTPUT_FOR_REVIEW_BEFORE_G02_R4_HEAD_TITLE_CARDINALITY'
