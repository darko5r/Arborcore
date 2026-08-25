#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"

fail() { echo "FAIL: $*" >&2; exit 1; }

printf '%s\n' '### VIEW0 V1N1 C0 — SCOPE / CORRECTED F1 CONTRACT'
bash tools/view0_v1n1_c0_scope_verify.sh
bash tools/view0_v1n1_c0_contract_verify.sh

printf '%s\n' '### VIEW0 V1N1 C0 — COMPLETE V1N0 + LOWER-LAYER REGRESSION'
bash tools/view0_v1_native_foundation_gate.sh

printf '%s\n' '### VIEW0 V1N1 C0 — CLEAN C0 DERIVED TEST RESET'
rm -f \
    build/view0-v1/native/c0_facts_test.o \
    build/view0-v1/native/c0_facts_adversarial_test.o \
    build/view0-v1/native/c0-facts-test \
    build/view0-v1/native/c0-facts-adversarial-test \
    build/view0-v1/native/c0-facts-sanitize-test
for stale in \
    build/view0-v1/native/c0_facts_test.o \
    build/view0-v1/native/c0_facts_adversarial_test.o \
    build/view0-v1/native/c0-facts-test \
    build/view0-v1/native/c0-facts-adversarial-test \
    build/view0-v1/native/c0-facts-sanitize-test; do
    [[ ! -e "$stale" ]] || fail "C0 derived output survived clean reset: $stale"
done
echo 'VIEW0_V1N1_C0_DERIVED_TEST_RESET=PASS'

printf '%s\n' '### VIEW0 V1N1 C0 — NATIVE FACTS / ADVERSARIAL / ANALYZER'
bash tools/view0_v1n1_c0_native_verify.sh

printf '%s\n' '### VIEW0 V1N1 C0 — ASAN / UBSAN WITH SANITIZED LEXBOR'
make -s view0-v1n1-c0-sanitize
[[ -x build/view0-v1/native/c0-facts-sanitize-test ]] || fail 'fresh C0 sanitizer test was not rebuilt'
echo 'VIEW0_V1N1_C0_SANITIZE_REBUILD=PASS'

printf '%s\n' '### VIEW0 V1N1 C0 — FINAL DIFF / POLICY'
git diff --check
[[ -z "$(git diff --cached --name-only)" ]] || fail 'Git index changed during C0 gate'

if grep -ERq 'ARBOR_VIEW_V1_G02_|0x000000003002000[0-9A-Fa-f]' \
    tools/c/view0_conformance/lexbor_adapter.c \
    tests/c/view0_v1n1_c0_facts_test.c \
    tests/c/view0_v1n1_c0_facts_adversarial_test.c; then
    fail 'C0 facts substrate acquired rule semantics under higher extension'
fi

echo 'VIEW0_V1N1_C0_GATE=PASS'
echo 'VIEW0_V1N1_C0_EXTENSION_AWARE_RETENTION=YES'
echo 'VIEW0_V1N1_C0_FACTS_SUBSTRATE_RULE_SEMANTICS=ZERO'
echo 'VIEW0_V1N1_C0_STAGED_CHANGES=NO'
echo 'VIEW0_V1N1_C0_NEW_COMMIT_CREATED=NO'
echo 'VIEW0_V1N1_C0_REMOTE_WRITE_PERFORMED=NO'
echo 'NEXT_REQUIRED_STEP=RETURN_COMPLETE_C0_GATE_OUTPUT_FOR_REVIEW_BEFORE_G02_CONSTRUCTION'
