#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
OUT="$ROOT/build/browser-webgpu-repro"
command -v tar >/dev/null 2>&1 || { echo 'W5_WEBGPU_REPRO_RESULT=REVIEW_NO_TAR' >&2; exit 1; }
rm -rf "$OUT"
mkdir -p "$OUT/a" "$OUT/b"
files=(
  Makefile
  browser/arborcore-browser-webgpu-1.contract
  browser/webgpu_accelerator.js
  docs/BROWSER_WEBGPU_W0_W6.md
  tests/browser/webgpu_accelerator_browser_test.html
  tests/js/browser_webgpu_unit_test.mjs
  tools/webgpu_benchmark_verify.sh
  tools/webgpu_w0_host_verify.sh
  tools/webgpu_contract_verify.sh
  tools/webgpu_real_browser_runner.mjs
  tools/webgpu_real_browser_verify.sh
  tools/webgpu_isolated_browser_runner.mjs
  tools/webgpu_isolated_browser_verify.sh
  tools/webgpu_reproducibility_verify.sh
  tools/webgpu_w1_w6_gate.sh
)
for dir in a b; do
  (
    cd "$ROOT"
    LC_ALL=C tar --sort=name --mtime='@0' --owner=0 --group=0 --numeric-owner \
      -cf "$OUT/$dir/arborcore-webgpu-w1-w6.tar" "${files[@]}"
  )
done
cmp "$OUT/a/arborcore-webgpu-w1-w6.tar" "$OUT/b/arborcore-webgpu-w1-w6.tar"
sha="$(sha256sum "$OUT/a/arborcore-webgpu-w1-w6.tar" | awk '{print $1}')"
cat > "$OUT/result.env" <<EVIDENCE
W5_WEBGPU_REPRO_RESULT=PASS
WEBGPU_W1_W6_ARCHIVE_SHA256=$sha
EVIDENCE
printf 'webgpu_repro_sha256=%s\n' "$sha"
echo 'PASS: independent WebGPU W1-W6 source archives are byte-for-byte reproducible'
