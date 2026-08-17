#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
OUT=build/http0-repro; rm -rf "$OUT"; mkdir -p "$OUT"
paths=(Makefile http/arborcore-http-message-semantics-1.contract docs/HTTP_MESSAGE_SEMANTICS_HTTP0.md include/arborcore/http.h src/asm/http_header.asm src/asm/http_response_v2.asm src/c/http.c tests/asm/http0_abi_test.asm tests/c/http0_header_test.c tests/c/http0_response_test.c tests/c/http0_adversarial_test.c tests/c/http0_integration_test.c bench/http0_response_bench.c tools/http0_baseline_verify.sh tools/http0_contract_verify.sh tools/http0_native_verify.sh tools/http0_abi_verify.sh tools/http0_scope_verify.sh tools/http0_reproducibility_verify.sh tools/http0_benchmark_run.sh tools/http0_gate.sh)
printf 'path\tmode\tbytes\tlines\tsha256\n' >"$OUT/candidate-manifest.tsv"
for p in "${paths[@]}"; do mode=100644; [[ -x "$p" ]] && mode=100755; printf '%s\t%s\t%s\t%s\t%s\n' "$p" "$mode" "$(wc -c <"$p")" "$(wc -l <"$p")" "$(sha256sum "$p" | awk '{print $1}')" >>"$OUT/candidate-manifest.tsv"; done
make_tar(){ local dest=$1 staging; staging=$(mktemp -d); for p in "${paths[@]}"; do mkdir -p "$staging/$(dirname "$p")"; cp "$p" "$staging/$p"; if [[ -x "$p" ]]; then chmod 755 "$staging/$p"; else chmod 644 "$staging/$p"; fi; touch -d '@0' "$staging/$p"; done; (cd "$staging" && LC_ALL=C tar --sort=name --format=ustar --owner=0 --group=0 --numeric-owner --mtime='@0' -cf "$dest" "${paths[@]}"); rm -rf "$staging"; }
make_tar "$PWD/$OUT/a.tar"; make_tar "$PWD/$OUT/b.tar"; cmp "$OUT/a.tar" "$OUT/b.tar"; cp "$OUT/a.tar" "$OUT/candidate-source.tar"
cat >"$OUT/result.env" <<EOF_RESULT
HTTP0_MANIFEST_SHA256=$(sha256sum "$OUT/candidate-manifest.tsv" | awk '{print $1}')
HTTP0_ARCHIVE_SHA256=$(sha256sum "$OUT/candidate-source.tar" | awk '{print $1}')
HTTP0_ARCHIVE_FILE_COUNT=${#paths[@]}
EOF_RESULT
cat "$OUT/result.env"; echo 'PASS: HTTP0 normalized-mode deterministic candidate archive'
