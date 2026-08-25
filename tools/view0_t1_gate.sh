#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"

echo '### VIEW0 T1 SCOPE / FROZEN BASE / CONTRACT'
bash tools/view0_t1_scope_verify.sh
bash tools/view0_t1_baseline_verify.sh
bash tools/view0_t1_contract_verify.sh

echo '### VIEW0 C1/C2/C3/C4 REGRESSION UNDER T1 EXTENSION'
bash tools/view0_c4_gate.sh
echo 'PASS: source-reviewed VIEW0 C1/C2/C3/C4 behavior and real NASM ABI qualification preserved under T1'

echo '### VIEW0 T1 NATIVE / ADVERSARIAL'
bash tools/view0_t1_native_verify.sh

echo '### VIEW0 T1 SANITIZERS'
make -s view0-t1-sanitize

echo '### VIEW0 T1 GCC ANALYZER'
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
for src in src/c/view.c tests/c/view0_t1_test.c tests/c/view0_t1_adversarial_test.c; do
  cc -Iinclude -D_POSIX_C_SOURCE=200809L -std=c17 -O0 -g -fPIC \
    -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion \
    -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef \
    -fanalyzer -c "$src" -o "$tmp/$(basename "$src" .c).o"
done
echo 'PASS: VIEW0 T1 implementation and harnesses GCC -fanalyzer'

echo '### VIEW0 T1 FINAL DIFF / POLICY'
git diff --check
bash tools/view0_t1_scope_verify.sh >/dev/null
bash tools/view0_t1_baseline_verify.sh >/dev/null
bash tools/view0_t1_contract_verify.sh >/dev/null
echo 'VIEW0_T1_GATE=PASS'
echo 'VIEW0_T1_STAGED_CHANGES=NO'
echo 'VIEW0_T1_NEW_COMMIT_CREATED=NO'
echo 'VIEW0_T1_REMOTE_WRITE_PERFORMED=NO'
