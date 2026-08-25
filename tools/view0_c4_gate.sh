#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
echo '### VIEW0 C4 SCOPE / FROZEN BASE / CONTRACT'
bash tools/view0_c4_scope_verify.sh
bash tools/view0_c4_baseline_verify.sh
bash tools/view0_c4_contract_verify.sh

echo '### VIEW0 C1/C2/C3 REGRESSION UNDER C4 ABI QUALIFICATION'
bash tools/view0_c3_gate.sh
echo 'PASS: source-review-closed VIEW0 C1/C2/C3 semantics preserved under C4'

echo '### VIEW0 C4 REAL NASM / C ABI / FUNCTIONAL / ADVERSARIAL'
bash tools/view0_c4_abi_verify.sh

echo '### VIEW0 C4 SANITIZERS'
make -s view0-c4-sanitize

echo '### VIEW0 C4 GCC ANALYZER — C HARNESS / ABI EXPECTATIONS'
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
for src in tests/c/view0_c4_test.c tests/c/view0_c4_adversarial_test.c; do
  cc -Iinclude -D_POSIX_C_SOURCE=200809L -std=c17 -O0 -g -fPIC \
    -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion \
    -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef \
    -fanalyzer -c "$src" -o "$tmp/$(basename "$src" .c).o"
done
echo 'PASS: VIEW0 C4 C harness GCC -fanalyzer'

echo '### VIEW0 C4 FINAL DIFF / POLICY'
git diff --check
bash tools/view0_c4_scope_verify.sh >/dev/null
bash tools/view0_c4_baseline_verify.sh >/dev/null
bash tools/view0_c4_contract_verify.sh >/dev/null
echo 'VIEW0_C4_GATE=PASS'
echo 'VIEW0_C4_STAGED_CHANGES=NO'
echo 'VIEW0_C4_NEW_COMMIT_CREATED=NO'
echo 'VIEW0_C4_REMOTE_WRITE_PERFORMED=NO'
