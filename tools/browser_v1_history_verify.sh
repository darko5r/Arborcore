#!/usr/bin/env bash
set -euo pipefail

ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"

PRECISION_FREEZE="80395372243fcfb2995ed03beca8a3af3e873964"
PRECISION_TREE="b571df944b366f6ee1cdbbf3c55c12d507ba41f9"
PRECISION_PATH="browser/precision_surface.js"
PRECISION_BLOB="e5595c063ff438a63efc9dfb7c324c60d0099e3f"
PRECISION_SHA="e6e228daaf3cd220a727e5622fdca5bf24f8ff1fc67816e76adcf35f39ccf13e"
PRECISION_SOURCE_SHA="a970816b979dae1021853bc649f2b37ae8703403376f893d8ca4ab678725cfd3"
PRECISION_CONTRACT_SHA="7bd78690444e7925913010e2187cab80dfb5631dc97e3b126047ede2cda5f4b7"

WEBGPU_FREEZE="66574f02102c0b5bdcc97590ddfb3b30e298cf97"
WEBGPU_TREE="7b93cfa50f3a0d70b9e8934c187099ab29185a4b"
WEBGPU_PATH="browser/webgpu_accelerator.js"
WEBGPU_BLOB="fc9694d4456236551067e45a0b5ca2019cbb2cde"
WEBGPU_SHA="b42d48d8f30a9c3cd11c63c68219effe4ad7126d511f0233583fd1a1a59e38bb"
WEBGPU_SOURCE_SHA="351cfe3e3240661a3251b25a0ecbd61dd724edfd5fbbb39dab8795199784e666"
WEBGPU_CONTRACT_SHA="a0d95467817504dbdc4db38c22b10c210b208e0604857ab752aa2730e97a144b"

FIXTURE="tests/data/browser_v1_precision_vectors.json"

command -v git >/dev/null
command -v tar >/dev/null
command -v sha256sum >/dev/null

git cat-file -e "$PRECISION_FREEZE^{commit}"
git cat-file -e "$WEBGPU_FREEZE^{commit}"

precision_tree="$(git rev-parse "$PRECISION_FREEZE^{tree}")"
webgpu_tree="$(git rev-parse "$WEBGPU_FREEZE^{tree}")"

[[ "$precision_tree" == "$PRECISION_TREE" ]]
[[ "$webgpu_tree" == "$WEBGPU_TREE" ]]

precision_blob="$(git rev-parse "$PRECISION_FREEZE:$PRECISION_PATH")"
webgpu_blob="$(git rev-parse "$WEBGPU_FREEZE:$WEBGPU_PATH")"

[[ "$precision_blob" == "$PRECISION_BLOB" ]]
[[ "$webgpu_blob" == "$WEBGPU_BLOB" ]]

precision_sha="$(
  git show "$PRECISION_FREEZE:$PRECISION_PATH" |
  sha256sum |
  awk '{print $1}'
)"

webgpu_sha="$(
  git show "$WEBGPU_FREEZE:$WEBGPU_PATH" |
  sha256sum |
  awk '{print $1}'
)"

[[ "$precision_sha" == "$PRECISION_SHA" ]]
[[ "$webgpu_sha" == "$WEBGPU_SHA" ]]

precision_contract_sha="$(
  sha256sum browser/arborcore-browser-surface-1.contract |
  awk '{print $1}'
)"

webgpu_contract_sha="$(
  sha256sum browser/arborcore-browser-webgpu-1.contract |
  awk '{print $1}'
)"

[[ "$precision_contract_sha" == "$PRECISION_CONTRACT_SHA" ]]
[[ "$webgpu_contract_sha" == "$WEBGPU_CONTRACT_SHA" ]]

tmp="$(
  mktemp -d \
    "${TMPDIR:-/tmp}/arborcore-browser-v1-history.XXXXXX"
)"

trap 'rm -rf "$tmp"' EXIT

precision_paths=(
  include/arborcore/browser_surface.h
  src/c/browser_surface.c
  browser/precision_surface.js
  browser/linear16_srgb8_bucket12.h
  browser/arborcore-browser-surface-1.contract
)

mkdir -p "$tmp/precision"

git archive \
  "$PRECISION_FREEZE" \
  -- "${precision_paths[@]}" |
tar -x -C "$tmp/precision"

precision_source_sha="$(
  (
    cd "$tmp/precision"

    {
      printf '%s\0' "${precision_paths[@]}" |
      LC_ALL=C sort -z |
      xargs -0 sha256sum
    } |
    sha256sum |
    awk '{print $1}'
  )
)"

[[ "$precision_source_sha" == "$PRECISION_SOURCE_SHA" ]]

webgpu_paths=(
  browser/arborcore-browser-webgpu-1.contract
  browser/webgpu_accelerator.js
)

mkdir -p "$tmp/webgpu"

git archive \
  "$WEBGPU_FREEZE" \
  -- "${webgpu_paths[@]}" |
tar -x -C "$tmp/webgpu"

webgpu_source_sha="$(
  (
    cd "$tmp/webgpu"

    {
      printf '%s\0' "${webgpu_paths[@]}" |
      LC_ALL=C sort -z |
      xargs -0 sha256sum
    } |
    sha256sum |
    awk '{print $1}'
  )
)"

[[ "$webgpu_source_sha" == "$WEBGPU_SOURCE_SHA" ]]

[[ -s "$FIXTURE" ]]

grep -q \
  '"freezeCommit": "80395372243fcfb2995ed03beca8a3af3e873964"' \
  "$FIXTURE"

grep -q \
  '"gitBlob": "e5595c063ff438a63efc9dfb7c324c60d0099e3f"' \
  "$FIXTURE"

grep -q \
  '"sha256": "e6e228daaf3cd220a727e5622fdca5bf24f8ff1fc67816e76adcf35f39ccf13e"' \
  "$FIXTURE"

printf 'PRECISION_V1_FREEZE_COMMIT=%s\n' "$PRECISION_FREEZE"
printf 'PRECISION_V1_TREE=%s\n' "$precision_tree"
printf 'PRECISION_V1_BLOB=%s\n' "$precision_blob"
printf 'PRECISION_V1_JS_SHA256=%s\n' "$precision_sha"
printf 'PRECISION_V1_SOURCE_SHA256=%s\n' "$precision_source_sha"
printf 'PRECISION_V1_CONTRACT_SHA256=%s\n' "$precision_contract_sha"

printf 'WEBGPU_V1_FREEZE_COMMIT=%s\n' "$WEBGPU_FREEZE"
printf 'WEBGPU_V1_TREE=%s\n' "$webgpu_tree"
printf 'WEBGPU_V1_BLOB=%s\n' "$webgpu_blob"
printf 'WEBGPU_V1_JS_SHA256=%s\n' "$webgpu_sha"
printf 'WEBGPU_V1_SOURCE_SHA256=%s\n' "$webgpu_source_sha"
printf 'WEBGPU_V1_CONTRACT_SHA256=%s\n' "$webgpu_contract_sha"

echo 'PRECISION_V1_HISTORY_VERIFIED=PASS'
echo 'WEBGPU_V1_HISTORY_VERIFIED=PASS'
echo 'PASS: retired Browser/WebGPU v1 implementation identities are recoverable from frozen Git history'
