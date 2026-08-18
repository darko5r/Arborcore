#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"

echo '### HTTP1 SCOPE / BASELINE / CONTROLLED AF1 RETROFIT / CONTRACT'
bash tools/http1_scope_verify.sh
bash tools/http1_baseline_verify.sh
bash tools/http1_af1_retrofit_verify.sh
bash tools/http1_contract_verify.sh

echo '### HTTP1 NATIVE / REAL SOCKET / ABI'
bash tools/http1_native_verify.sh
bash tools/http1_abi_verify.sh

echo '### HTTP1 SANITIZERS'
make -s http1-sanitize

echo '### HTTP1 GCC ANALYZER'
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
for src in src/c/http_mvc.c src/c/application_foundation.c; do
  cc -Iinclude -D_POSIX_C_SOURCE=200809L -std=c17 -O0 -g -fPIC \
    -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion \
    -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef \
    -fanalyzer -c "$src" -o "$tmp/$(basename "$src" .c).o"
done
echo 'PASS: HTTP1 + controlled AF1 retrofit GCC -fanalyzer'

echo '### FROZEN C RUNTIME / LEGACY SERVER REGRESSION'
make -s c-runtime-check
make -s c-runtime-sanitize
make -s server-test
./build/server-test
echo 'PASS: frozen C runtime + legacy server regression'

echo '### AF1 -> AF4 DEPENDENT REQUALIFICATION'
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
echo 'PASS: AF1 semantic retrofit dependent AF2-AF4 requalification'

echo '### MVC0 COMPLETE REGRESSION ON RETROFITTED AF1'
bash tools/mvc0_native_verify.sh
bash tools/mvc0_abi_verify.sh
make -s mvc0-sanitize
echo 'PASS: MVC0 complete regression without source/API reopen'

echo '### HTTP0 COMPLETE REGRESSION'
bash tools/http0_native_verify.sh
bash tools/http0_abi_verify.sh
make -s http0-sanitize
echo 'PASS: HTTP0 complete regression without source reopen'

echo '### HTTP1 DIAGNOSTIC BENCHMARK'
bash tools/http1_benchmark_run.sh

echo '### HTTP1 REPRODUCIBILITY'
bash tools/http1_reproducibility_verify.sh

echo '### HTTP1 FINAL POLICY / DIFF'
git diff --check
bash tools/http1_scope_verify.sh >/dev/null
bash tools/http1_baseline_verify.sh >/dev/null
bash tools/http1_contract_verify.sh >/dev/null
echo 'HTTP1_GATE=PASS'
echo 'HTTP1_ASSEMBLY_ABI_V1_REOPEN=NO'
echo 'HTTP1_HTTP0_REOPEN=NO'
echo 'HTTP1_MVC0_REOPEN=NO'
echo 'HTTP1_AF1_SEMANTIC_RETROFIT=QUALIFIED'
echo 'HTTP1_STAGED_CHANGES=NO'
echo 'HTTP1_NEW_COMMIT_CREATED=NO'
echo 'HTTP1_REMOTE_WRITE_PERFORMED=NO'
