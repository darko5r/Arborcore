import fs from 'node:fs';
import assert from 'node:assert/strict';
import { q32ToCssPxString, resolveDevicePixelSize } from '../../browser/precision_surface.js';

const modulePath = process.argv[2];
const bytes = fs.readFileSync(modulePath);
const moduleObject = new WebAssembly.Module(bytes);
assert.equal(WebAssembly.Module.imports(moduleObject).length, 0);
const ex = new WebAssembly.Instance(moduleObject, {}).exports;
const decoder = new TextDecoder();

function v2Css(raw) {
  const ptr = ex.arbor_browser_host_v2_css_scratch_ptr();
  const cap = ex.arbor_browser_host_v2_css_scratch_bytes();
  const status = ex.arbor_browser_host_v2_format_q32_css(
    raw, ptr, cap, ex.arbor_browser_host_v2_written_scratch_ptr());
  assert.equal(status, 0);
  const data = new Uint8Array(ex.memory.buffer, ptr, cap);
  let n = 0; while (n < data.length && data[n] !== 0) n++;
  return decoder.decode(data.subarray(0, n));
}

const q32Vectors = [
  0n, 1n, -1n,
  0x80000000n, -0x80000000n,
  0x100000000n, -0x100000000n,
  0x123456789abn, -0x123456789abn,
  0x7fffffffffffffffn, -0x8000000000000000n
];
for (const raw of q32Vectors) assert.equal(v2Css(raw), q32ToCssPxString(raw), `Q32 ${raw}`);

function v2Size(entry, dpr) {
  const d = entry.devicePixelContentBoxSize?.[0] || null;
  const c = entry.contentBoxSize?.[0] || null;
  const status = ex.arbor_browser_host_v2_resolve_device_size(
    d ? 1 : 0, d?.inlineSize || 0, d?.blockSize || 0,
    c?.inlineSize || entry.contentRect?.width || 0,
    c?.blockSize || entry.contentRect?.height || 0,
    dpr, ex.arbor_browser_host_v2_size_scratch_ptr());
  assert.equal(status, 0);
  return { width: ex.arbor_browser_host_v2_size_width(), height: ex.arbor_browser_host_v2_size_height() };
}
const entries = [
  [{ devicePixelContentBoxSize: [{ inlineSize: 640, blockSize: 360 }] }, 2],
  [{ contentBoxSize: [{ inlineSize: 100.25, blockSize: 50.25 }] }, 2],
  [{ contentRect: { width: 319.5, height: 179.5 } }, 1],
  [{ contentRect: { width: 10.1, height: 10.9 } }, 1.5]
];
for (const [entry, dpr] of entries) {
  const old = resolveDevicePixelSize(entry, dpr);
  const next = v2Size(entry, dpr);
  assert.equal(next.width, old.width);
  assert.equal(next.height, old.height);
}
console.log('PASS: Browser Language Boundary v2 C/WASM numerical behavior matches frozen browser v1 vectors');
