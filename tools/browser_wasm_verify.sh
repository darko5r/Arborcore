#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
OUT="$ROOT/build/browser-b0-b6-wasm"
MODULE="$OUT/arborcore-browser-reference.wasm"
NATIVE_RGBA8="$OUT/native-golden.rgba8"
EXPECTED_RGBA16="fda03aa982372e8bb181ecf4e65910478bd8ad66ae08cf12cc1a5f89288673ba"
EXPECTED_RGBA8="a24be39e38b4fb654e79a2161177cf3670c354695f0b2fa13be677e8d1624dfd"
command -v clang >/dev/null 2>&1 || { echo 'B0_B1_WASM_RESULT=REVIEW_NO_CLANG' >&2; exit 1; }
command -v node >/dev/null 2>&1 || { echo 'B0_B1_WASM_RESULT=REVIEW_NO_NODE' >&2; exit 1; }
mkdir -p "$OUT"
"$ROOT/build/browser-export-golden-native" > "$NATIVE_RGBA8"
native_rgba8_sha="$(sha256sum "$NATIVE_RGBA8" | awk '{print $1}')"
[[ "$native_rgba8_sha" == "$EXPECTED_RGBA8" ]]

clang --target=wasm32 \
  -I"$ROOT/include" -I"$ROOT/tests/c" \
  -std=c17 -O2 -ffreestanding -fno-builtin -nostdlib \
  "$ROOT/src/c/geometry.c" \
  "$ROOT/src/c/renderer.c" \
  "$ROOT/src/c/browser_surface.c" \
  "$ROOT/src/wasm/geometry_int128_builtins.c" \
  "$ROOT/src/wasm/renderer_memory_builtins.c" \
  "$ROOT/tests/c/browser_wasm_selftest.c" \
  -Wl,--no-entry \
  -Wl,--export=browser_wasm_prepare \
  -Wl,--export=browser_wasm_rgba16_ptr \
  -Wl,--export=browser_wasm_rgba16_size \
  -Wl,--export=browser_wasm_rgba8_ptr \
  -Wl,--export=browser_wasm_rgba8_size \
  -Wl,--export=browser_wasm_opaque_rgba8_ptr \
  -Wl,--export=browser_wasm_opaque_rgba8_size \
  -Wl,--export=browser_wasm_width \
  -Wl,--export=browser_wasm_height \
  -Wl,--export=browser_wasm_rgba8_stride \
  -Wl,--export=browser_wasm_opaque_width \
  -Wl,--export=browser_wasm_opaque_height \
  -Wl,--export=browser_wasm_opaque_rgba8_stride \
  -Wl,--export-memory -Wl,--strip-all \
  -o "$MODULE"

node - "$MODULE" "$EXPECTED_RGBA16" "$EXPECTED_RGBA8" <<'NODE'
const fs = require('fs');
const crypto = require('crypto');
const modulePath = process.argv[2];
const expected16 = process.argv[3];
const expected8 = process.argv[4];
const bytes = fs.readFileSync(modulePath);
const moduleObject = new WebAssembly.Module(bytes);
const imports = WebAssembly.Module.imports(moduleObject);
if (imports.length !== 0) throw new Error(`WASM imports=${imports.length}`);
const instance = new WebAssembly.Instance(moduleObject, {});
const ex = instance.exports;
if (ex.browser_wasm_prepare() !== 0) throw new Error('browser_wasm_prepare failed');
function hash(ptr, size) {
  return crypto.createHash('sha256').update(Buffer.from(ex.memory.buffer, ptr, size)).digest('hex');
}
const rgba16 = hash(ex.browser_wasm_rgba16_ptr(), ex.browser_wasm_rgba16_size());
const rgba8 = hash(ex.browser_wasm_rgba8_ptr(), ex.browser_wasm_rgba8_size());
if (rgba16 !== expected16) throw new Error(`RGBA16 hash ${rgba16}`);
if (rgba8 !== expected8) throw new Error(`RGBA8 hash ${rgba8}`);
const ptr = ex.browser_wasm_rgba8_ptr();
const before = new Uint8Array(ex.memory.buffer, ptr, 1)[0];
const oldBuffer = ex.memory.buffer;
ex.memory.grow(1);
if (ex.memory.buffer === oldBuffer) throw new Error('memory buffer identity unchanged after grow');
const after = new Uint8Array(ex.memory.buffer, ptr, 1)[0];
if (after !== before) throw new Error('memory content changed after grow');
console.log(`wasm_import_count=${imports.length}`);
console.log(`wasm_rgba16_sha256=${rgba16}`);
console.log(`wasm_rgba8_sha256=${rgba8}`);
console.log(`wasm_memory_buffer_identity_changed=true`);
console.log(`old_buffer_byte_length_after_grow=${oldBuffer.byteLength}`);
NODE
module_sha="$(sha256sum "$MODULE" | awk '{print $1}')"
cat > "$OUT/result.env" <<EVIDENCE
B0_B1_WASM_RESULT=PASS
WASM_IMPORT_COUNT=0
WASM_RGBA16_SHA256=$EXPECTED_RGBA16
WASM_RGBA8_SHA256=$EXPECTED_RGBA8
WASM_MEMORY_BUFFER_IDENTITY_CHANGED=PASS
BROWSER_WASM_MODULE_SHA256=$module_sha
EVIDENCE
cat "$OUT/result.env"
echo 'PASS: B0/B1 zero-import WASM memory/export qualification'
