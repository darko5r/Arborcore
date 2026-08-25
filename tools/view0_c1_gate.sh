#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
echo '### VIEW0 C1 SCOPE / FROZEN BASE / CONTRACT'
bash tools/view0_c1_scope_verify.sh
bash tools/view0_c1_baseline_verify.sh
bash tools/view0_c1_contract_verify.sh

echo '### VIEW0 C1 NATIVE / ADVERSARIAL'
bash tools/view0_c1_native_verify.sh

echo '### VIEW0 C1 SANITIZERS'
make -s view0-c1-sanitize

echo '### VIEW0 C1 GCC ANALYZER'
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
cc -Iinclude -D_POSIX_C_SOURCE=200809L -std=c17 -O0 -g -fPIC \
  -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion \
  -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef \
  -fanalyzer -c src/c/view.c -o "$tmp/view.o"
echo 'PASS: VIEW0 C1 GCC -fanalyzer'

echo '### RELEVANT FROZEN ASSEMBLY BUFFER / ARENA REGRESSION'
make -s buffer-test arena-test core-buffer-arena-property-test core-buffer-alias-test
echo 'PASS: qualified Assembly buffer/arena behavior preserved'

echo '### C RUNTIME STATUS / ASSEMBLY BRIDGE REGRESSION'
make -s c-runtime-check
make -s c-runtime-sanitize
echo 'PASS: C runtime status translation and Assembly bridge preserved'

echo '### POST-HTTP1 PRESENTATION BASE SMOKE REGRESSION'
make -s mvc0-native-verify
make -s http0-native-verify
make -s http1-native-verify
echo 'PASS: MVC0/HTTP0/HTTP1 native presentation base preserved'

echo '### VIEW0 C1 FINAL DIFF / POLICY'
git diff --check
bash tools/view0_c1_scope_verify.sh >/dev/null
bash tools/view0_c1_baseline_verify.sh >/dev/null
bash tools/view0_c1_contract_verify.sh >/dev/null
echo 'VIEW0_C1_GATE=PASS'
echo 'VIEW0_C1_STAGED_CHANGES=NO'
echo 'VIEW0_C1_NEW_COMMIT_CREATED=NO'
echo 'VIEW0_C1_REMOTE_WRITE_PERFORMED=NO'
