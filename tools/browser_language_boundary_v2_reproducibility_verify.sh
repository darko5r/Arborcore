#!/usr/bin/env bash
set -euo pipefail

ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
OUT="$ROOT/build/browser-language-boundary-v2-repro"

FROZEN_COMMIT="f3cc06fc721e75fa5059ecb3b32ab54f2e2ab2fb"
EXPECTED_FROZEN_ARCHIVE="cf0205662e314a674cc2a17df2b148f09b2381c56f9801a774ac5463c59fee71"
EXPECTED_FROZEN_COUNT="21"

QUALIFICATION_FILES=(
  Makefile
  docs/SOURCES.md
  docs/BROWSER_LANGUAGE_BOUNDARY_V2.md
  browser/arborcore-browser-language-boundary-2.contract
  browser/arborcore_host.js
  browser/shaders/rgba8_present.wgsl
  browser/shaders/rgba16_exact_convert.wgsl
  include/arborcore/browser_host_v2.h
  src/c/browser_host_v2.c
  tests/c/browser_host_v2_test.c
  tests/c/browser_host_v2_wasm_selftest.c
  tests/js/browser_language_boundary_v2_equivalence.mjs
  tests/js/browser_language_boundary_v2_experiment.mjs
  tests/data/browser_v1_precision_vectors.json
  tests/browser/browser_language_boundary_v2_test.html
  tools/browser_language_boundary_v2_js_audit.py
  tools/browser_language_boundary_v2_wasm_verify.sh
  tools/browser_language_boundary_v2_live_runner.mjs
  tools/browser_language_boundary_v2_live_verify.sh
  tools/browser_language_boundary_v2_contract_verify.sh
  tools/browser_language_boundary_v2_reproducibility_verify.sh
  tools/browser_v1_history_verify.sh
  tools/browser_language_boundary_v2_gate.sh
)

command -v git >/dev/null
command -v tar >/dev/null
command -v sha256sum >/dev/null
command -v cmp >/dev/null

rm -rf "$OUT"
mkdir -p "$OUT"

tmp="$(
  mktemp -d \
    "${TMPDIR:-/tmp}/arborcore-lbv2-frozen-tree.XXXXXX"
)"

cleanup() {
  rm -rf "$tmp"
}

trap cleanup EXIT

mkdir -p "$tmp/tree"

git -C "$ROOT" archive "$FROZEN_COMMIT" |
  tar -x -C "$tmp/tree"

frozen_script="$tmp/tree/tools/browser_language_boundary_v2_reproducibility_verify.sh"

[[ -f "$frozen_script" ]]

ARBORCORE_ROOT="$tmp/tree" \
  bash "$frozen_script" \
  > "$OUT/frozen-verifier.log"

frozen_result="$tmp/tree/build/browser-language-boundary-v2-repro/result.env"
frozen_tar="$tmp/tree/build/browser-language-boundary-v2-repro/a.tar"

[[ -s "$frozen_result" ]]
[[ -s "$frozen_tar" ]]

frozen_sha="$(
  sha256sum "$frozen_tar" |
  awk '{print $1}'
)"

frozen_reported_sha="$(
  awk -F= \
    '$1=="LBV2_ARCHIVE_SHA256" {print $2}' \
    "$frozen_result"
)"

frozen_reported_count="$(
  awk -F= \
    '$1=="LBV2_ARCHIVE_FILE_COUNT" {print $2}' \
    "$frozen_result"
)"

[[ "$frozen_sha" == "$EXPECTED_FROZEN_ARCHIVE" ]]
[[ "$frozen_reported_sha" == "$EXPECTED_FROZEN_ARCHIVE" ]]
[[ "$frozen_reported_count" == "$EXPECTED_FROZEN_COUNT" ]]

make_current_tar() {
  local dest="$1"

  tar \
    --sort=name \
    --mtime='@0' \
    --owner=0 \
    --group=0 \
    --numeric-owner \
    --mode='u+rwX,go+rX,go-w' \
    -C "$ROOT" \
    -cf "$dest" \
    "${QUALIFICATION_FILES[@]}"
}

make_current_tar "$OUT/a.tar"
make_current_tar "$OUT/b.tar"

cmp "$OUT/a.tar" "$OUT/b.tar"

qualification_sha="$(
  sha256sum "$OUT/a.tar" |
  awk '{print $1}'
)"

cat > "$OUT/result.env" <<EVIDENCE
LBV2_REPRODUCIBILITY_RESULT=PASS
LBV2_ARCHIVE_SHA256=$frozen_sha
LBV2_ARCHIVE_FILE_COUNT=$frozen_reported_count
LBV2_FROZEN_ARCHIVE_SHA256=$frozen_sha
LBV2_FROZEN_ARCHIVE_FILE_COUNT=$frozen_reported_count
LBV2_QUALIFICATION_ARCHIVE_SHA256=$qualification_sha
LBV2_QUALIFICATION_ARCHIVE_FILE_COUNT=${#QUALIFICATION_FILES[@]}
LBV2_QUALIFICATION_ARCHIVE_ROLE=POST_FREEZE_CURRENT_QUALIFICATION
EVIDENCE

cat "$OUT/result.env"

echo 'PASS: exact frozen LBv2 archive reproduced by its own frozen verifier'
echo 'PASS: current post-freeze LBv2 qualification archive is reproducible'
