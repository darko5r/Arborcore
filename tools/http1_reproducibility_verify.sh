#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
out=build/http1-repro; rm -rf "$out"; mkdir -p "$out"
paths=(
Makefile
application/arborcore-application-ddd-mvc-foundation-1.contract
docs/APPLICATION_DDD_MVC_FOUNDATION.md
src/c/application_foundation.c
tests/c/application_foundation_test.c
tools/application_foundation_contract_verify.sh
http/arborcore-http-mvc-adapter-1.contract
docs/HTTP_MVC_ADAPTER_HTTP1.md
include/arborcore/http_mvc.h
src/c/http_mvc.c
tests/asm/http1_abi_test.asm
tests/c/http1_core_test.c
tests/c/http1_adversarial_test.c
tests/c/http1_integration_test.c
tests/c/http1_socket_test.c
bench/http1_adapter_bench.c
tools/http1_af1_retrofit_verify.sh
tools/http1_baseline_verify.sh
tools/http1_contract_verify.sh
tools/http1_native_verify.sh
tools/http1_abi_verify.sh
tools/http1_scope_verify.sh
tools/http1_reproducibility_verify.sh
tools/http1_benchmark_run.sh
tools/http1_gate.sh
)
printf 'path\tmode\tbytes\tlines\tsha256\n' >"$out/candidate-manifest.tsv"
for p in "${paths[@]}"; do
  mode=100644; [[ -x "$p" ]] && mode=100755
  printf '%s\t%s\t%s\t%s\t%s\n' "$p" "$mode" "$(wc -c <"$p")" "$(wc -l <"$p")" "$(sha256sum "$p"|awk '{print $1}')" >>"$out/candidate-manifest.tsv"
done
for n in a b; do
  tar --sort=name --mtime='UTC 1970-01-01' --owner=0 --group=0 --numeric-owner --format=ustar \
    -cf "$out/$n.tar" "${paths[@]}"
done
cmp -s "$out/a.tar" "$out/b.tar"
cp "$out/a.tar" "$out/candidate-source.tar"
MSHA=$(sha256sum "$out/candidate-manifest.tsv"|awk '{print $1}')
ASHA=$(sha256sum "$out/candidate-source.tar"|awk '{print $1}')
printf 'HTTP1_MANIFEST_SHA256=%s\nHTTP1_ARCHIVE_SHA256=%s\nHTTP1_ARCHIVE_FILE_COUNT=25\n' "$MSHA" "$ASHA" | tee "$out/result.env"
echo 'PASS: HTTP1 normalized-mode deterministic candidate archive'
