#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
OUT="$ROOT/build/browser-webgpu-postfreeze-repro"
mkdir -p "$OUT"
files=(
  Makefile
  browser/arborcore-browser-webgpu-postfreeze-optimization-1.contract
  browser/webgpu_postfreeze_optimizer.js
  browser/webgpu_rgba16_exact_tables.js
  browser/webgpu_rgba16_experiment.js
  docs/BROWSER_WEBGPU_POSTFREEZE_OPT0_OPT5.md
  tests/browser/webgpu_postfreeze_browser_test.html
  tests/c/webgpu_postfreeze_wasm_selftest.c
  tests/js/browser_webgpu_postfreeze_unit_test.mjs
  tools/webgpu_postfreeze_benchmark_verify.sh
  tools/webgpu_postfreeze_contract_verify.sh
  tools/webgpu_postfreeze_opt0_opt5_gate.sh
  tools/webgpu_postfreeze_real_browser_runner.mjs
  tools/webgpu_postfreeze_real_browser_verify.sh
  tools/webgpu_postfreeze_reproducibility_verify.sh
  tools/webgpu_postfreeze_table_verify.mjs
  tools/webgpu_postfreeze_wasm_verify.sh
)
make_archive() {
  local output="$1"
  (cd "$ROOT" && printf '%s\0' "${files[@]}" | LC_ALL=C sort -z | \
    tar --null -T - --format=posix --sort=name --mtime='UTC 2026-08-14' \
      --owner=0 --group=0 --numeric-owner \
      --pax-option=delete=atime,delete=ctime -cf "$output")
}
make_archive "$OUT/a.tar"
make_archive "$OUT/b.tar"
cmp "$OUT/a.tar" "$OUT/b.tar"
sha="$(sha256sum "$OUT/a.tar" | awk '{print $1}')"
cat > "$OUT/result.env" <<EVIDENCE
POSTFREEZE_OPT_REPRODUCIBILITY_RESULT=PASS
POSTFREEZE_OPT_ARCHIVE_SHA256=$sha
POSTFREEZE_OPT_ARCHIVE_FILE_COUNT=${#files[@]}
EVIDENCE
cat "$OUT/result.env"
echo 'PASS: independent OPT0-OPT5 source archives are byte-for-byte reproducible'
