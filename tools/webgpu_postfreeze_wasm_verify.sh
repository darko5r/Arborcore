#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
OUT="$ROOT/build/browser-webgpu-postfreeze-wasm"
MODULE="$OUT/arborcore-browser-postfreeze.wasm"
SRC="$ROOT/tests/c/webgpu_postfreeze_wasm_selftest.c"
command -v clang >/dev/null 2>&1 || { echo 'OPT1_WASM_RESULT=REVIEW_NO_CLANG' >&2; exit 1; }
command -v node >/dev/null 2>&1 || { echo 'OPT1_WASM_RESULT=REVIEW_NO_NODE' >&2; exit 1; }
mkdir -p "$OUT"

clang --target=wasm32 \
  -I"$ROOT/include" \
  -std=c17 -O2 -ffreestanding -fno-builtin -nostdlib \
  "$ROOT/src/c/browser_surface.c" \
  "$ROOT/src/wasm/geometry_int128_builtins.c" \
  "$SRC" \
  -Wl,--no-entry \
  -Wl,--export=postfreeze_wasm_prepare_vectors \
  -Wl,--export=postfreeze_wasm_prepare_perf \
  -Wl,--export=postfreeze_wasm_export_perf \
  -Wl,--export=postfreeze_wasm_vector_rgba16_ptr \
  -Wl,--export=postfreeze_wasm_vector_rgba16_size \
  -Wl,--export=postfreeze_wasm_vector_rgba8_ptr \
  -Wl,--export=postfreeze_wasm_vector_rgba8_size \
  -Wl,--export=postfreeze_wasm_perf_rgba16_ptr \
  -Wl,--export=postfreeze_wasm_perf_rgba16_size \
  -Wl,--export=postfreeze_wasm_perf_rgba8_ptr \
  -Wl,--export=postfreeze_wasm_perf_rgba8_size \
  -Wl,--export=postfreeze_wasm_vector_width \
  -Wl,--export=postfreeze_wasm_vector_height \
  -Wl,--export=postfreeze_wasm_vector_rgba16_stride \
  -Wl,--export=postfreeze_wasm_vector_rgba8_stride \
  -Wl,--export=postfreeze_wasm_perf_width \
  -Wl,--export=postfreeze_wasm_perf_height \
  -Wl,--export=postfreeze_wasm_perf_rgba16_stride \
  -Wl,--export=postfreeze_wasm_perf_rgba8_stride \
  -Wl,--export-memory -Wl,--strip-all \
  -o "$MODULE"

node - "$MODULE" <<'NODE'
const fs = require('fs');
const crypto = require('crypto');
const modulePath = process.argv[2];
const bytes = fs.readFileSync(modulePath);
const moduleObject = new WebAssembly.Module(bytes);
const imports = WebAssembly.Module.imports(moduleObject);
if (imports.length !== 0) throw new Error(`postfreeze WASM imports=${imports.length}`);
const instance = new WebAssembly.Instance(moduleObject, {});
const ex = instance.exports;
if (ex.postfreeze_wasm_prepare_vectors() !== 0) throw new Error('vector preparation failed');
if (ex.postfreeze_wasm_prepare_perf() !== 0) throw new Error('performance preparation failed');
function hash(ptr, size) {
  return crypto.createHash('sha256').update(Buffer.from(ex.memory.buffer, ptr, size)).digest('hex');
}
const vector16 = hash(ex.postfreeze_wasm_vector_rgba16_ptr(), ex.postfreeze_wasm_vector_rgba16_size());
const vector8 = hash(ex.postfreeze_wasm_vector_rgba8_ptr(), ex.postfreeze_wasm_vector_rgba8_size());
const perf16 = hash(ex.postfreeze_wasm_perf_rgba16_ptr(), ex.postfreeze_wasm_perf_rgba16_size());
const perf8 = hash(ex.postfreeze_wasm_perf_rgba8_ptr(), ex.postfreeze_wasm_perf_rgba8_size());
if (ex.postfreeze_wasm_export_perf() !== 0) throw new Error('repeat performance export failed');
const perf8Repeat = hash(ex.postfreeze_wasm_perf_rgba8_ptr(), ex.postfreeze_wasm_perf_rgba8_size());
if (perf8 !== perf8Repeat) throw new Error('B1 performance export is not deterministic');
console.log(`OPT1_WASM_IMPORT_COUNT=${imports.length}`);
console.log(`OPT3_VECTOR_RGBA16_SHA256=${vector16}`);
console.log(`OPT3_VECTOR_RGBA8_SHA256=${vector8}`);
console.log(`OPT1_PERF_RGBA16_SHA256=${perf16}`);
console.log(`OPT1_PERF_RGBA8_SHA256=${perf8}`);
NODE

module_sha="$(sha256sum "$MODULE" | awk '{print $1}')"
node_output="$(node - "$MODULE" <<'NODE'
const fs = require('fs');
const crypto = require('crypto');
const moduleObject = new WebAssembly.Module(fs.readFileSync(process.argv[2]));
const ex = new WebAssembly.Instance(moduleObject, {}).exports;
if (ex.postfreeze_wasm_prepare_vectors() !== 0 || ex.postfreeze_wasm_prepare_perf() !== 0) process.exit(1);
function h(ptr, size) { return crypto.createHash('sha256').update(Buffer.from(ex.memory.buffer, ptr, size)).digest('hex'); }
console.log(`OPT3_VECTOR_RGBA16_SHA256=${h(ex.postfreeze_wasm_vector_rgba16_ptr(), ex.postfreeze_wasm_vector_rgba16_size())}`);
console.log(`OPT3_VECTOR_RGBA8_SHA256=${h(ex.postfreeze_wasm_vector_rgba8_ptr(), ex.postfreeze_wasm_vector_rgba8_size())}`);
console.log(`OPT1_PERF_RGBA16_SHA256=${h(ex.postfreeze_wasm_perf_rgba16_ptr(), ex.postfreeze_wasm_perf_rgba16_size())}`);
console.log(`OPT1_PERF_RGBA8_SHA256=${h(ex.postfreeze_wasm_perf_rgba8_ptr(), ex.postfreeze_wasm_perf_rgba8_size())}`);
NODE
)"
cat > "$OUT/result.env" <<EVIDENCE
OPT1_WASM_RESULT=PASS
OPT1_WASM_IMPORT_COUNT=0
OPT1_WASM_MODULE_SHA256=$module_sha
$node_output
OPT3_VECTOR_POLICY=OPAQUE_LINEAR16_EXHAUSTIVE_PLUS_16384_MIXED_ALPHA_VECTORS
OPT1_PERF_SURFACE=640X360_MIXED_ALPHA
EVIDENCE
cat "$OUT/result.env"
echo 'PASS: post-W6 zero-import WASM B1 oracle and performance fixture'
