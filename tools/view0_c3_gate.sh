#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
echo '### VIEW0 C3 SCOPE / FROZEN BASE / CONTRACT'
bash tools/view0_c3_scope_verify.sh
bash tools/view0_c3_baseline_verify.sh
bash tools/view0_c3_contract_verify.sh

echo '### VIEW0 C1/C2 REGRESSION UNDER C3 COMPOSITION'
bash tools/view0_c2_gate.sh
echo 'PASS: source-review-closed VIEW0 C1/C2 semantics preserved under C3'

echo '### VIEW0 C3 NATIVE / ADVERSARIAL'
bash tools/view0_c3_native_verify.sh

echo '### VIEW0 C3 SANITIZERS'
make -s view0-c3-sanitize

echo '### VIEW0 C3 GCC ANALYZER — COMPILED-VIEW PATTERN'
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
for src in tests/c/view0_c3_test.c tests/c/view0_c3_adversarial_test.c; do
  cc -Iinclude -D_POSIX_C_SOURCE=200809L -std=c17 -O0 -g -fPIC \
    -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion \
    -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef \
    -fanalyzer -c "$src" -o "$tmp/$(basename "$src" .c).o"
done
echo 'PASS: VIEW0 C3 application-defined compiled-view pattern GCC -fanalyzer'

echo '### VIEW0 C3 FINAL DIFF / POLICY'
git diff --check
bash tools/view0_c3_scope_verify.sh >/dev/null
bash tools/view0_c3_baseline_verify.sh >/dev/null
bash tools/view0_c3_contract_verify.sh >/dev/null
echo 'VIEW0_C3_GATE=PASS'
echo 'VIEW0_C3_STAGED_CHANGES=NO'
echo 'VIEW0_C3_NEW_COMMIT_CREATED=NO'
echo 'VIEW0_C3_REMOTE_WRITE_PERFORMED=NO'
