#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
ARBORCORE_ROOT="$ROOT" bash tools/renderer_r4_r9_lower_layer_verify.sh
renderer="$({ printf '%s\0' \
  include/arborcore/renderer.h \
  src/c/renderer.c \
  renderer/srgb8_linear16_lut.h \
  src/wasm/renderer_memory_builtins.c \
  renderer/arborcore-renderer-1.contract \
  | sort -z | xargs -0 sha256sum; } | sha256sum | awk '{print $1}')"
renderer_contract="$(sha256sum renderer/arborcore-renderer-1.contract | awk '{print $1}')"
printf 'renderer_source_sha256=%s\n' "$renderer"
printf 'renderer_contract_sha256=%s\n' "$renderer_contract"
[[ "$renderer" == "fe943ed7c52b75f8dd5eecd41f290ac9e06c5201ffeee59a00f713c267418783" ]]
[[ "$renderer_contract" == "1db52f652d50943753bc18a3cf487741e7ad5a494614d64fac32a2f693a5aab7" ]]
echo "PASS: B0-B6 browser delivery remains above frozen Reference Raster Contract v1"
