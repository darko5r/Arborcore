#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
OUT="$ROOT/build/browser-v2-hardening-repro"
mkdir -p "$OUT"
FILES=(
  Makefile
  browser/arborcore-browser-language-boundary-2.contract
  browser/arborcore-browser-v2-hardening-optimization-1.contract
  browser/arborcore_host.js
  browser/shaders/rgba16_exact_convert.wgsl
  browser/shaders/rgba8_present.wgsl
  docs/BROWSER_LANGUAGE_BOUNDARY_V2.md
  docs/BROWSER_V2_DIAGNOSTICS.md
  docs/BROWSER_V2_HARDENING_OPTIMIZATION.md
  include/arborcore/browser_hardening_v2.h
  include/arborcore/browser_host_v2.h
  src/c/browser_hardening_v2.c
  src/c/browser_host_v2.c
  tests/browser/browser_v2_hardening_diagnostics.html
  tests/c/browser_hardening_v2_test.c
  tests/c/browser_hardening_v2_wasm_selftest.c
  tests/data/browser_v2_dpr_vectors.json
  tests/data/browser_v2_opt3_performance_evidence.json
  tests/js/browser_language_boundary_v2_experiment.mjs
  tests/js/browser_v2_hardening_runtime.mjs
  tools/browser_v2_hardening_contract_verify.sh
  tools/browser_v2_hardening_gate.sh
  tools/browser_v2_hardening_inventory.sh
  tools/browser_v2_hardening_js_audit.py
  tools/browser_v2_hardening_live_runner.mjs
  tools/browser_v2_hardening_live_verify.sh
  tools/browser_v2_hardening_native_verify.sh
  tools/browser_v2_hardening_performance_verify.py
  tools/browser_v2_hardening_reproducibility_verify.sh
  tools/browser_v2_hardening_wasm_verify.sh
  tools/browser_v2_opt3_qualification_verify.py
)
for f in "${FILES[@]}"; do [[ -f "$ROOT/$f" ]] || { echo "FAIL: missing reproducibility input $f" >&2; exit 1; }; done
printf '%s\n' "${FILES[@]}" | LC_ALL=C sort > "$OUT/files.txt"
make_tar(){ local dest="$1"; (cd "$ROOT" && tar --sort=name --format=ustar --mtime='UTC 1970-01-01' --owner=0 --group=0 --numeric-owner -cf "$dest" -T "$OUT/files.txt"); }
make_tar "$OUT/a.tar"; make_tar "$OUT/b.tar"
cmp "$OUT/a.tar" "$OUT/b.tar"
sha="$(sha256sum "$OUT/a.tar" | awk '{print $1}')"
count="$(wc -l < "$OUT/files.txt")"
[[ "$count" -eq 31 ]] || { echo "FAIL: BV2H reproducibility archive file count is $count, expected 31" >&2; exit 1; }
grep -qx 'Makefile' "$OUT/files.txt"
printf 'BV2H_REPRODUCIBILITY_RESULT=PASS\nBV2H_FROZEN_ARCHIVE_SHA256=%s\nBV2H_FROZEN_ARCHIVE_FILE_COUNT=%s\nBV2H_REPRODUCIBILITY_ARCHIVE_INCLUDES_MAKEFILE=YES\n' "$sha" "$count" | tee "$OUT/result.env"
echo 'PASS: BV2H source/evidence archive is byte-reproducible and includes Makefile integration'
