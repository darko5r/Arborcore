#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
BASE='1e6b04632d971ed1d26bcf74654250f784fcca64'; BRANCH='http0-message-metadata-semantics'
expected=$(mktemp); actual=$(mktemp); trap 'rm -f "$expected" "$actual"' EXIT
cat >"$expected" <<'EOF_PATHS'
Makefile
bench/http0_response_bench.c
docs/HTTP_MESSAGE_SEMANTICS_HTTP0.md
http/arborcore-http-message-semantics-1.contract
include/arborcore/http.h
src/asm/http_header.asm
src/asm/http_response_v2.asm
src/c/http.c
tests/asm/http0_abi_test.asm
tests/c/http0_adversarial_test.c
tests/c/http0_header_test.c
tests/c/http0_integration_test.c
tests/c/http0_response_test.c
tools/http0_abi_verify.sh
tools/http0_baseline_verify.sh
tools/http0_benchmark_run.sh
tools/http0_contract_verify.sh
tools/http0_gate.sh
tools/http0_native_verify.sh
tools/http0_reproducibility_verify.sh
tools/http0_scope_verify.sh
EOF_PATHS
LC_ALL=C sort -o "$expected" "$expected"
[[ "$(git branch --show-current)" == "$BRANCH" ]]
[[ "$(git rev-parse HEAD)" == "$BASE" ]]
{ git diff --name-only "$BASE" --; git ls-files --others --exclude-standard; } | sed '/^$/d' | LC_ALL=C sort -u >"$actual"
diff -u "$expected" "$actual"
echo "HTTP0_CANDIDATE_PATH_COUNT=$(wc -l <"$actual")"; cat "$actual"
echo 'HTTP0_LOWER_LAYER_RETROFIT_REQUIRED=NO'
echo 'PASS: HTTP0 construction scope exact; frozen lower/Application/MVC/browser layers untouched'
