#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"

fail() { echo "FAIL: $*" >&2; exit 1; }

printf '%s\n' '### VIEW0 V1N1 G02 R2 — SCOPE / CONTRACT'
bash tools/view0_v1n1_g02_r2_scope_verify.sh
bash tools/view0_v1n1_g02_r2_contract_verify.sh

printf '%s\n' '### VIEW0 V1N1 G02 R2 — RETAINED R1/C0/V1N0/LOWER-LAYER REGRESSION'
bash tools/view0_v1n1_g02_r1_gate.sh

printf '%s\n' '### VIEW0 V1N1 G02 R2 — CLEAN DERIVED TEST RESET'
rm -f \
    build/view0-v1/native/g02_r2_doctype_syntax_test.o \
    build/view0-v1/native/g02_r2_doctype_syntax_adversarial_test.o \
    build/view0-v1/native/g02-r2-doctype-syntax-test \
    build/view0-v1/native/g02-r2-doctype-syntax-adversarial-test \
    build/view0-v1/native/g02-r2-doctype-syntax-sanitize-test
for stale in \
    build/view0-v1/native/g02_r2_doctype_syntax_test.o \
    build/view0-v1/native/g02_r2_doctype_syntax_adversarial_test.o \
    build/view0-v1/native/g02-r2-doctype-syntax-test \
    build/view0-v1/native/g02-r2-doctype-syntax-adversarial-test \
    build/view0-v1/native/g02-r2-doctype-syntax-sanitize-test; do
    [[ ! -e "$stale" ]] || fail "G02 R2 derived output survived clean reset: $stale"
done
echo 'VIEW0_V1N1_G02_R2_DERIVED_TEST_RESET=PASS'

printf '%s\n' '### VIEW0 V1N1 G02 R2 — NATIVE / ADVERSARIAL / ANALYZER / CLI'
bash tools/view0_v1n1_g02_r2_native_verify.sh

printf '%s\n' '### VIEW0 V1N1 G02 R2 — ASAN / UBSAN WITH SANITIZED LEXBOR'
make -s view0-v1n1-g02-r2-sanitize
[[ -x build/view0-v1/native/g02-r2-doctype-syntax-sanitize-test ]] || fail 'fresh G02 R2 sanitizer test was not rebuilt'
echo 'VIEW0_V1N1_G02_R2_SANITIZE_REBUILD=PASS'

printf '%s\n' '### VIEW0 V1N1 G02 R2 — FINAL DIFF / POLICY'
git diff --check
[[ -z "$(git diff --cached --name-only)" ]] || fail 'Git index changed during G02 R2 gate'

grep -Fq 'ARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED' tools/c/view0_conformance/native.c || fail 'G02 R1 implementation disappeared'
grep -Fq 'ARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX' tools/c/view0_conformance/native.c || fail 'G02 R2 implementation disappeared'
! grep -ERq '0x000000003002000[45]' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'retired G02 rule ID reused'
! grep -ERq 'ARBOR_VIEW_V1_G0[3-6]_' tools/c/view0_conformance tools/include/arborcore/view0_conformance || fail 'G03-G06 implementation appeared during retained G02 R2'

echo 'VIEW0_V1N1_G02_R2_GATE=PASS'
echo 'VIEW0_V1N1_G02_R2_EXTENSION_AWARE_RETENTION=YES'
echo 'VIEW0_V1N1_G02_R2_G03_G06_RULES_IMPLEMENTED=ZERO'
echo 'VIEW0_V1N1_G02_R2_STAGED_CHANGES=NO'
echo 'VIEW0_V1N1_G02_R2_NEW_COMMIT_CREATED=NO'
echo 'VIEW0_V1N1_G02_R2_REMOTE_WRITE_PERFORMED=NO'
echo 'NEXT_REQUIRED_STEP=RETURN_COMPLETE_G02_R2_GATE_OUTPUT_FOR_REVIEW_BEFORE_G02_R3_DOCTYPE_LEGACY_DISCOURAGED'
