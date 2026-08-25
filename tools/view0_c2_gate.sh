#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
echo '### VIEW0 C2 SCOPE / FROZEN BASE / CONTRACT'
bash tools/view0_c2_scope_verify.sh
bash tools/view0_c2_baseline_verify.sh
bash tools/view0_c2_contract_verify.sh

echo '### VIEW0 C1 REGRESSION UNDER C2 EXTENSION'
bash tools/view0_c1_gate.sh
echo 'PASS: source-review-closed VIEW0 C1 semantics preserved under C2 extension'

echo '### VIEW0 C2 NATIVE / ADVERSARIAL'
bash tools/view0_c2_native_verify.sh

echo '### VIEW0 C2 SANITIZERS'
make -s view0-c2-sanitize

echo '### VIEW0 C2 GCC ANALYZER'
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
cc -Iinclude -D_POSIX_C_SOURCE=200809L -std=c17 -O0 -g -fPIC \
  -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion \
  -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef \
  -fanalyzer -c src/c/view.c -o "$tmp/view.o"
echo 'PASS: VIEW0 C2 GCC -fanalyzer'

echo '### VIEW0 C2 FINAL DIFF / POLICY'
git diff --check
bash tools/view0_c2_scope_verify.sh >/dev/null
bash tools/view0_c2_baseline_verify.sh >/dev/null
bash tools/view0_c2_contract_verify.sh >/dev/null
echo 'VIEW0_C2_GATE=PASS'
echo 'VIEW0_C2_STAGED_CHANGES=NO'
echo 'VIEW0_C2_NEW_COMMIT_CREATED=NO'
echo 'VIEW0_C2_REMOTE_WRITE_PERFORMED=NO'
