import {
  PrecisionSurfaceMemoryEpoch,
  q32ToCssPxString,
  resolveDevicePixelSize,
  wasmMemoryView
} from '../../browser/precision_surface.js';

function check(condition, message) {
  if (!condition) throw new Error(message);
}

check(q32ToCssPxString(0n) === '0.000000000px', 'zero Q32 CSS');
check(q32ToCssPxString(1n << 32n) === '1.000000000px', 'one Q32 CSS');
check(q32ToCssPxString((1n << 32n) + (1n << 31n)) === '1.500000000px', 'half Q32 CSS');
check(q32ToCssPxString(-(1n << 30n)) === '-0.250000000px', 'negative quarter Q32 CSS');

const preferred = resolveDevicePixelSize({
  devicePixelContentBoxSize: [{ inlineSize: 640, blockSize: 360 }],
  contentBoxSize: [{ inlineSize: 320, blockSize: 180 }]
}, 2);
check(preferred.width === 640 && preferred.height === 360 &&
      preferred.mode === 'device-pixel-content-box', 'preferred device pixel sizing');

const fallback = resolveDevicePixelSize({
  contentBoxSize: [{ inlineSize: 320.25, blockSize: 180.25 }]
}, 2);
check(fallback.width === 641 && fallback.height === 361 &&
      fallback.mode === 'content-box-times-dpr-fallback', 'fallback DPR sizing');

const memory = new WebAssembly.Memory({ initial: 1, maximum: 3 });
const epoch = new PrecisionSurfaceMemoryEpoch(memory);
const view = epoch.view(0, 8);
view[0] = 0x5a;
const oldBuffer = memory.buffer;
memory.grow(1);
check(memory.buffer !== oldBuffer, 'memory buffer identity changes after grow');
check(epoch.refresh() === true && epoch.generation === 1, 'memory epoch refresh');
check(epoch.view(0, 1)[0] === 0x5a, 'memory data retained after grow');
check(wasmMemoryView(memory, 0, 1)[0] === 0x5a, 'fresh memory view');

console.log('PASS: B0/B3/B4 JavaScript memory, DPR, and hybrid coordinate helpers');
