#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"

echo '### VIEW0 M1 SCOPE / FROZEN BASE / CONTRACT'
bash tools/view0_m1_scope_verify.sh
bash tools/view0_m1_baseline_verify.sh
bash tools/view0_m1_contract_verify.sh

echo '### VIEW0 C1-C4/T1 REGRESSION UNDER M1 EXTENSION'
bash tools/view0_t1_gate.sh
echo 'PASS: source-reviewed C1-C4 and T1 semantics preserved under M1'

echo '### VIEW0 M1 UTF-8 / REAL HTTP INTEGRATION'
bash tools/view0_m1_native_verify.sh

echo '### VIEW0 M1 SANITIZERS'
make -s view0-m1-sanitize

echo '### VIEW0 M1 GCC ANALYZER'
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
for src in src/c/view.c tests/c/view0_m1_utf8_test.c tests/c/view0_m1_adversarial_test.c tests/c/view0_m1_integration_test.c; do
  cc -Iinclude -D_POSIX_C_SOURCE=200809L -std=c17 -O0 -g -fPIC \
    -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion \
    -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef \
    -fanalyzer -c "$src" -o "$tmp/$(basename "$src" .c).o"
done
echo 'PASS: VIEW0 M1 implementation and harnesses GCC -fanalyzer'

echo '### VIEW0 M1 HTTP1 PRESENTATION BASE REGRESSION'
make -s http1-native-verify
echo 'PASS: frozen HTTP1 presentation semantics preserved under VIEW integration'

echo '### VIEW0 M1 FINAL DIFF / POLICY'
git diff --check
bash tools/view0_m1_scope_verify.sh >/dev/null
bash tools/view0_m1_baseline_verify.sh >/dev/null
bash tools/view0_m1_contract_verify.sh >/dev/null
echo 'VIEW0_M1_GATE=PASS'
echo 'VIEW0_M1_STAGED_CHANGES=NO'
echo 'VIEW0_M1_NEW_COMMIT_CREATED=NO'
echo 'VIEW0_M1_REMOTE_WRITE_PERFORMED=NO'
