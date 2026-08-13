#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
OUT="$ROOT/build/renderer-r8-wasm"
MODULE="$OUT/arborcore-renderer-reference-v1.wasm"
NATIVE="$OUT/native-golden.rgba16"
WASM_BYTES="$OUT/wasm-golden.rgba16"
EXPECTED="$(awk -F= '$1=="GOLDEN_RGBA16_SHA256" {print $2}' "$ROOT/renderer/arborcore-renderer-1.contract")"

command -v clang >/dev/null 2>&1 || { echo "R8_WASM_RESULT=REVIEW_NO_CLANG" >&2; exit 1; }
command -v node >/dev/null 2>&1 || { echo "R8_WASM_RESULT=REVIEW_NO_NODE" >&2; exit 1; }
mkdir -p "$OUT"

"$ROOT/build/renderer-golden-native" > "$NATIVE"

clang --target=wasm32 \
  -I"$ROOT/include" -I"$ROOT/tests/c" \
  -std=c17 -O2 -ffreestanding -fno-builtin -nostdlib \
  "$ROOT/src/c/geometry.c" \
  "$ROOT/src/c/renderer.c" \
  "$ROOT/src/wasm/geometry_int128_builtins.c" \
  "$ROOT/src/wasm/renderer_memory_builtins.c" \
  "$ROOT/tests/c/renderer_wasm_selftest.c" \
  -Wl,--no-entry \
  -Wl,--export=renderer_wasm_render \
  -Wl,--export=renderer_wasm_data_ptr \
  -Wl,--export=renderer_wasm_data_size \
  -Wl,--export-memory \
  -Wl,--strip-all \
  -o "$MODULE"

node - "$MODULE" "$WASM_BYTES" <<'NODE'
const fs = require('fs');
const modulePath = process.argv[2];
const outputPath = process.argv[3];
const bytes = fs.readFileSync(modulePath);
const moduleObject = new WebAssembly.Module(bytes);
const imports = WebAssembly.Module.imports(moduleObject);
if (imports.length !== 0) {
  console.error(`FAIL: renderer WASM module has ${imports.length} imports`);
  process.exit(2);
}
const instance = new WebAssembly.Instance(moduleObject, {});
const result = instance.exports.renderer_wasm_render();
if (result !== 0) {
  console.error(`FAIL: renderer_wasm_render=${result}`);
  process.exit(result || 3);
}
const ptr = instance.exports.renderer_wasm_data_ptr();
const size = instance.exports.renderer_wasm_data_size();
const data = Buffer.from(instance.exports.memory.buffer, ptr, size);
fs.writeFileSync(outputPath, data);
console.log(`wasm_import_count=${imports.length}`);
console.log(`wasm_render_result=${result}`);
console.log(`wasm_golden_bytes=${size}`);
NODE

cmp "$NATIVE" "$WASM_BYTES"
native_sha="$(sha256sum "$NATIVE" | awk '{print $1}')"
wasm_sha="$(sha256sum "$WASM_BYTES" | awk '{print $1}')"
module_sha="$(sha256sum "$MODULE" | awk '{print $1}')"
[[ "$native_sha" == "$EXPECTED" ]]
[[ "$wasm_sha" == "$EXPECTED" ]]

cat > "$OUT/result.env" <<EVIDENCE
R8_WASM_GOLDEN_RESULT=PASS
WASM_IMPORT_COUNT=0
NATIVE_GOLDEN_SHA256=$native_sha
WASM_GOLDEN_SHA256=$wasm_sha
RENDERER_WASM_MODULE_SHA256=$module_sha
GOLDEN_BYTES=$(stat -c %s "$NATIVE")
EVIDENCE
cat "$OUT/result.env"
echo "PASS: R8 native/WASM renderer golden buffers are byte-for-byte identical"
