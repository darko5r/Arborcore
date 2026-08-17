#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
cd "$ROOT"

printf '%s\n' '### AF4 SCOPE / BASELINE / CONTRACT'
bash tools/ddd_support_scope_verify.sh
bash tools/ddd_support_baseline_verify.sh
bash tools/ddd_support_contract_verify.sh

printf '%s\n' '### AF4 NATIVE / ABI'
bash tools/ddd_support_native_verify.sh
bash tools/ddd_support_abi_verify.sh

printf '%s\n' '### AF4 SANITIZERS'
make -s ddd-support-sanitize

printf '%s\n' '### AF0-AF3 REGRESSION'
make -s application-foundation-native-test
make -s application-foundation-sanitize
make -s application-capability-kernel-native-test
make -s application-capability-kernel-sanitize
make -s application-service-runtime-native-test
make -s application-service-runtime-adversarial-test
make -s application-service-runtime-sanitize
printf '%s\n' 'PASS: AF0-AF3 regression qualification'

printf '%s\n' '### AF4 GCC ANALYZER'
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
cc -Iinclude -D_POSIX_C_SOURCE=200809L \
  -std=c17 -O0 -g -fPIC -Wall -Wextra -Wpedantic -Werror \
  -Wconversion -Wsign-conversion -Wshadow -Wstrict-prototypes \
  -Wmissing-prototypes -Wformat=2 -Wundef -fanalyzer \
  -c src/c/ddd_support.c -o "$tmp/ddd_support_analyzer.o"
printf '%s\n' 'PASS: AF4 GCC -fanalyzer'

printf '%s\n' '### AF4 REPRODUCIBILITY'
bash tools/ddd_support_reproducibility_verify.sh

printf '%s\n' '### AF4 FINAL GATE'
git diff --check
printf 'AF4_R1_GATE=PASS\n'
printf 'AF4_LOWER_LAYER_RETROFIT_REQUIRED=NO\n'
printf 'AF4_STAGED_CHANGES=NO\n'
printf 'AF4_NEW_COMMIT_CREATED=NO\n'
printf 'AF4_REMOTE_WRITE_PERFORMED=NO\n'
