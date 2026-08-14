import assert from 'node:assert/strict';
import {
  WEBGPU_ACCELERATOR_VERSION,
  WEBGPU_ALPHA_MODE,
  WEBGPU_UPLOAD_FORMAT,
  alignTo,
  validateTightRgba8Layout,
  webGpuApiAvailable
} from '../../browser/webgpu_accelerator.js';

assert.equal(WEBGPU_ACCELERATOR_VERSION, '1.0-candidate');
assert.equal(WEBGPU_UPLOAD_FORMAT, 'rgba8unorm');
assert.equal(WEBGPU_ALPHA_MODE, 'premultiplied');
assert.equal(alignTo(0, 256), 0);
assert.equal(alignTo(1, 256), 256);
assert.equal(alignTo(256, 256), 256);
assert.equal(alignTo(257, 256), 512);
assert.deepEqual(validateTightRgba8Layout(16, 16, 64), {
  width: 16,
  height: 16,
  strideBytes: 64,
  byteLength: 1024
});
assert.throws(() => alignTo(-1, 256), RangeError);
assert.throws(() => alignTo(1, 0), RangeError);
assert.throws(() => validateTightRgba8Layout(16, 16, 68), RangeError);
assert.throws(() => validateTightRgba8Layout(0, 16, 0), RangeError);
assert.equal(typeof webGpuApiAvailable(), 'boolean');

console.log('PASS: W1-W3 WebGPU JavaScript invariants');
