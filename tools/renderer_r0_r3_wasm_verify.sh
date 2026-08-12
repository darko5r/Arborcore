#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
OUT="$ROOT/build/renderer-r0-r3/wasm"
HEADER="$ROOT/experiments/renderer/raster_foundation_candidates.h"
SRC="$ROOT/tests/c/renderer_foundation_wasm_selftest.c"
WASM="$OUT/renderer-foundation.wasm"
RESULT="$OUT/result.env"

mkdir -p "$OUT"
if ! command -v clang >/dev/null 2>&1; then
    echo "R0_R3_WASM_RUNTIME_RESULT=REVIEW_NO_CLANG"
    exit 2
fi
if ! command -v wasm-ld >/dev/null 2>&1; then
    echo "R0_R3_WASM_RUNTIME_RESULT=REVIEW_NO_WASM_LD"
    exit 2
fi
if ! command -v node >/dev/null 2>&1; then
    echo "R0_R3_WASM_RUNTIME_RESULT=REVIEW_NO_NODE_RUNTIME"
    exit 2
fi

clang --target=wasm32 -std=c17 -O2 -Wall -Wextra -Wpedantic -Werror \
  -I"$ROOT/experiments/renderer" -ffreestanding -fno-builtin -nostdlib \
  -Wl,--no-entry -Wl,--export=renderer_foundation_wasm_selftest \
  -Wl,--strip-all "$SRC" -o "$WASM"

node - "$WASM" "$RESULT" <<'NODE'
const fs=require('fs');
const [wasmPath,resultPath]=process.argv.slice(2);
const bytes=fs.readFileSync(wasmPath);
const mod=new WebAssembly.Module(bytes);
const imports=WebAssembly.Module.imports(mod);
if(imports.length!==0){
  console.error(`unexpected wasm imports: ${JSON.stringify(imports)}`);
  process.exit(2);
}
const inst=new WebAssembly.Instance(mod,{});
const rc=inst.exports.renderer_foundation_wasm_selftest();
if(rc!==0){ console.error(`wasm selftest failed: ${rc}`); process.exit(3); }
fs.writeFileSync(resultPath,`R0_R3_WASM_RUNTIME_RESULT=PASS\nWASM_IMPORT_COUNT=0\nWASM_SELFTEST_RESULT=0\n`);
console.log('wasm_import_count=0');
console.log('wasm_selftest_result=0');
console.log('R0_R3_WASM_RUNTIME_RESULT=PASS');
NODE
sha="$(sha256sum "$WASM" | awk '{print $1}')"
echo "RENDERER_FOUNDATION_WASM_SHA256=$sha" >> "$RESULT"
echo "renderer_foundation_wasm_sha256=$sha"
