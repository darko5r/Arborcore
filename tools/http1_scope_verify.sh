#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
BASE='5caeedb4d7c2b92bee829e27dc615efd5a658cac'
expected=$(mktemp); actual=$(mktemp); trap 'rm -f "$expected" "$actual"' EXIT
cat >"$expected" <<'E'
Makefile
application/arborcore-application-ddd-mvc-foundation-1.contract
bench/http1_adapter_bench.c
docs/APPLICATION_DDD_MVC_FOUNDATION.md
docs/HTTP_MVC_ADAPTER_HTTP1.md
http/arborcore-http-mvc-adapter-1.contract
include/arborcore/http_mvc.h
src/c/application_foundation.c
src/c/http_mvc.c
tests/asm/http1_abi_test.asm
tests/c/application_foundation_test.c
tests/c/http1_adversarial_test.c
tests/c/http1_core_test.c
tests/c/http1_integration_test.c
tests/c/http1_socket_test.c
tools/application_foundation_contract_verify.sh
tools/http1_abi_verify.sh
tools/http1_af1_retrofit_verify.sh
tools/http1_baseline_verify.sh
tools/http1_benchmark_run.sh
tools/http1_contract_verify.sh
tools/http1_gate.sh
tools/http1_native_verify.sh
tools/http1_reproducibility_verify.sh
tools/http1_scope_verify.sh
E
{
  git diff --name-only "$BASE" --
  git ls-files --others --exclude-standard
} | sed '/^$/d' | LC_ALL=C sort -u >"$actual"
LC_ALL=C sort -o "$expected" "$expected"
diff -u "$expected" "$actual"
echo "HTTP1_CANDIDATE_PATH_COUNT=$(wc -l <"$actual")"
cat "$actual"
echo 'PASS: HTTP1 construction scope exact'
