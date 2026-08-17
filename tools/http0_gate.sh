#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
echo '### HTTP0 SCOPE / BASELINE / CONTRACT'; bash tools/http0_scope_verify.sh; bash tools/http0_baseline_verify.sh; bash tools/http0_contract_verify.sh
echo '### HTTP0 NATIVE / REAL NASM ABI'; bash tools/http0_native_verify.sh; bash tools/http0_abi_verify.sh
echo '### HTTP0 SANITIZERS'; make -s http0-sanitize
echo '### HTTP0 GCC ANALYZER'; mkdir -p build/http0-analyzer
cc -Iinclude -D_POSIX_C_SOURCE=200809L -std=c17 -O0 -g -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef -fanalyzer -c src/c/http.c -o build/http0-analyzer/http.o
echo 'PASS: HTTP0 GCC -fanalyzer'
echo '### FROZEN C RUNTIME / LEGACY SERVER REGRESSION'
make -s c-runtime-check
make -s c-runtime-sanitize
make -s server-test
build/server-test
echo 'PASS: frozen C runtime + legacy status-only server regression'
echo '### AF0-AF4 / MVC0 REGRESSION'
make -s application-foundation-native-test
make -s application-foundation-sanitize
make -s application-capability-kernel-native-test
make -s application-capability-kernel-sanitize
make -s application-service-runtime-native-test
make -s application-service-runtime-adversarial-test
make -s application-service-runtime-sanitize
make -s ddd-support-native-test
make -s ddd-support-adversarial-test
make -s ddd-support-sanitize
bash tools/mvc0_native_verify.sh
bash tools/mvc0_abi_verify.sh
make -s mvc0-sanitize
echo 'PASS: AF0-AF4 and MVC0 regression qualification'
echo '### HTTP0 DIAGNOSTIC BENCHMARK'; bash tools/http0_benchmark_run.sh
echo '### HTTP0 REPRODUCIBILITY'; bash tools/http0_reproducibility_verify.sh
echo '### HTTP0 FINAL POLICY / DIFF'; git diff --check; bash tools/http0_scope_verify.sh >/dev/null; bash tools/http0_baseline_verify.sh >/dev/null; git diff --cached --quiet || { echo 'FAIL: HTTP0 gate unexpectedly staged changes' >&2; exit 1; }
echo 'HTTP0_GATE=PASS'; echo 'HTTP0_LOWER_LAYER_RETROFIT_REQUIRED=NO'; echo 'HTTP0_AF1_REOPEN_REQUIRED=NO'; echo 'HTTP0_MVC0_REOPEN_REQUIRED=NO'; echo 'HTTP0_STAGED_CHANGES=NO'; echo 'HTTP0_NEW_COMMIT_CREATED=NO'; echo 'HTTP0_REMOTE_WRITE_PERFORMED=NO'
