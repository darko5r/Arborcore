#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
OUT="$ROOT/build/browser-language-boundary-v2-repro"
mkdir -p "$OUT"
FILES=(
  Makefile docs/SOURCES.md docs/BROWSER_LANGUAGE_BOUNDARY_V2.md
  browser/arborcore-browser-language-boundary-2.contract browser/arborcore_host.js
  browser/shaders/rgba8_present.wgsl browser/shaders/rgba16_exact_convert.wgsl
  include/arborcore/browser_host_v2.h src/c/browser_host_v2.c
  tests/c/browser_host_v2_test.c tests/c/browser_host_v2_wasm_selftest.c
  tests/js/browser_language_boundary_v2_equivalence.mjs tests/js/browser_language_boundary_v2_experiment.mjs
  tests/browser/browser_language_boundary_v2_test.html
  tools/browser_language_boundary_v2_js_audit.py tools/browser_language_boundary_v2_wasm_verify.sh
  tools/browser_language_boundary_v2_live_runner.mjs tools/browser_language_boundary_v2_live_verify.sh
  tools/browser_language_boundary_v2_contract_verify.sh tools/browser_language_boundary_v2_reproducibility_verify.sh
  tools/browser_language_boundary_v2_gate.sh
)
make_tar() {
  local dest="$1"
  tar --sort=name --mtime='@0' --owner=0 --group=0 --numeric-owner \
    --mode='u+rwX,go+rX,go-w' -C "$ROOT" -cf "$dest" "${FILES[@]}"
}
make_tar "$OUT/a.tar"
make_tar "$OUT/b.tar"
cmp "$OUT/a.tar" "$OUT/b.tar"
sha="$(sha256sum "$OUT/a.tar" | awk '{print $1}')"
cat > "$OUT/result.env" <<EVIDENCE
LBV2_REPRODUCIBILITY_RESULT=PASS
LBV2_ARCHIVE_SHA256=$sha
LBV2_ARCHIVE_FILE_COUNT=${#FILES[@]}
EVIDENCE
cat "$OUT/result.env"
echo 'PASS: Browser Language Boundary v2 source archive reproducibility'
