#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
OUT="$ROOT/build/browser-language-boundary-v2-wasm"
MODULE="$OUT/arborcore-browser-host-v2.wasm"
command -v clang >/dev/null 2>&1 || { echo 'LBV2_WASM_RESULT=REVIEW_NO_CLANG' >&2; exit 1; }
command -v node >/dev/null 2>&1 || { echo 'LBV2_WASM_RESULT=REVIEW_NO_NODE' >&2; exit 1; }
mkdir -p "$OUT"
clang --target=wasm32 \
  -I"$ROOT/include" \
  -std=c17 -O2 -ffreestanding -fno-builtin -nostdlib \
  "$ROOT/src/c/browser_host_v2.c" \
  "$ROOT/tests/c/browser_host_v2_wasm_selftest.c" \
  -Wl,--no-entry \
  -Wl,--export=browser_host_v2_selftest \
  -Wl,--export=arbor_browser_host_v2_version \
  -Wl,--export=arbor_browser_host_v2_format_q32_css \
  -Wl,--export=arbor_browser_host_v2_resolve_device_size \
  -Wl,--export=arbor_browser_host_v2_validate_rgba8 \
  -Wl,--export=arbor_browser_host_v2_validate_rgba16 \
  -Wl,--export=arbor_browser_host_v2_classify_gpu_failure \
  -Wl,--export=arbor_browser_host_v2_state_init \
  -Wl,--export=arbor_browser_host_v2_state_webgpu_ready \
  -Wl,--export=arbor_browser_host_v2_state_failure \
  -Wl,--export=arbor_browser_host_v2_state_destroy \
  -Wl,--export=arbor_browser_host_v2_prepare_gpu_tables \
  -Wl,--export=arbor_browser_host_v2_bucket12_ptr \
  -Wl,--export=arbor_browser_host_v2_bucket12_bytes \
  -Wl,--export=arbor_browser_host_v2_forward_lut_ptr \
  -Wl,--export=arbor_browser_host_v2_forward_lut_bytes \
  -Wl,--export=arbor_browser_host_v2_size_width \
  -Wl,--export=arbor_browser_host_v2_size_height \
  -Wl,--export=arbor_browser_host_v2_resolved_size_mode \
  -Wl,--export=arbor_browser_host_v2_layout_byte_length \
  -Wl,--export=arbor_browser_host_v2_rgba8_output_bytes \
  -Wl,--export=arbor_browser_host_v2_dispatch_x \
  -Wl,--export=arbor_browser_host_v2_dispatch_y \
  -Wl,--export=arbor_browser_host_v2_state_present_mode \
  -Wl,--export=arbor_browser_host_v2_state_failure_class \
  -Wl,--export=arbor_browser_host_v2_state_generation \
  -Wl,--export=arbor_browser_host_v2_css_scratch_ptr \
  -Wl,--export=arbor_browser_host_v2_written_scratch_ptr \
  -Wl,--export=arbor_browser_host_v2_written_value \
  -Wl,--export=arbor_browser_host_v2_css_scratch_bytes \
  -Wl,--export=arbor_browser_host_v2_failure_scratch_ptr \
  -Wl,--export=arbor_browser_host_v2_failure_scratch_bytes \
  -Wl,--export=arbor_browser_host_v2_size_scratch_ptr \
  -Wl,--export=arbor_browser_host_v2_layout_scratch_ptr \
  -Wl,--export=arbor_browser_host_v2_state_scratch_ptr \
  -Wl,--export-memory -Wl,--strip-all \
  -o "$MODULE"
node - "$MODULE" <<'NODE'
const fs = require('fs');
const crypto = require('crypto');
const path = process.argv[2];
const bytes = fs.readFileSync(path);
const moduleObject = new WebAssembly.Module(bytes);
const imports = WebAssembly.Module.imports(moduleObject);
if (imports.length !== 0) throw new Error(`imports=${imports.length}`);
const instance = new WebAssembly.Instance(moduleObject, {});
const ex = instance.exports;
if (ex.browser_host_v2_selftest() !== 0) throw new Error('C/WASM selftest failed');
if (ex.arbor_browser_host_v2_version() !== 0x00020000) throw new Error('version mismatch');
ex.arbor_browser_host_v2_prepare_gpu_tables();
function hash(ptr, size) {
  return crypto.createHash('sha256').update(Buffer.from(ex.memory.buffer, ptr, size)).digest('hex');
}
const bucket = hash(ex.arbor_browser_host_v2_bucket12_ptr(), ex.arbor_browser_host_v2_bucket12_bytes());
const forward = hash(ex.arbor_browser_host_v2_forward_lut_ptr(), ex.arbor_browser_host_v2_forward_lut_bytes());
console.log(`LBV2_WASM_IMPORT_COUNT=${imports.length}`);
console.log(`LBV2_BUCKET_U32_SHA256=${bucket}`);
console.log(`LBV2_FORWARD_U32_SHA256=${forward}`);
NODE
node "$ROOT/tests/js/browser_language_boundary_v2_equivalence.mjs" "$MODULE"
module_sha="$(sha256sum "$MODULE" | awk '{print $1}')"
# capture deterministic semantic table identities from the just-verified module
read -r bucket_sha forward_sha < <(node - "$MODULE" <<'NODE'
const fs=require('fs'), crypto=require('crypto');
const m=new WebAssembly.Module(fs.readFileSync(process.argv[2]));
const e=new WebAssembly.Instance(m,{}).exports;e.arbor_browser_host_v2_prepare_gpu_tables();
const h=(p,n)=>crypto.createHash('sha256').update(Buffer.from(e.memory.buffer,p,n)).digest('hex');
process.stdout.write(`${h(e.arbor_browser_host_v2_bucket12_ptr(),e.arbor_browser_host_v2_bucket12_bytes())} ${h(e.arbor_browser_host_v2_forward_lut_ptr(),e.arbor_browser_host_v2_forward_lut_bytes())}\n`);
NODE
)
cat > "$OUT/result.env" <<EVIDENCE
LBV2_WASM_RESULT=PASS
LBV2_WASM_IMPORT_COUNT=0
LBV2_WASM_MODULE_SHA256=$module_sha
LBV2_BUCKET_U32_SHA256=$bucket_sha
LBV2_FORWARD_U32_SHA256=$forward_sha
EVIDENCE
cat "$OUT/result.env"
echo 'PASS: Browser Host Boundary v2 zero-import C/WASM authority'
