import assert from 'node:assert/strict';
import {
  OptimizedRgba8WebGpuPresenter,
  WEBGPU_POSTFREEZE_OPTIMIZER_VERSION,
  median
} from '../../browser/webgpu_postfreeze_optimizer.js';
import {
  WEBGPU_RGBA16_EXPERIMENT_STATE,
  WEBGPU_RGBA16_EXPERIMENT_VERSION,
  validateTightRgba16Layout
} from '../../browser/webgpu_rgba16_experiment.js';
import {
  FROZEN_LINEAR16_TO_SRGB8_BUCKET12,
  FROZEN_SRGB8_TO_LINEAR16,
  exactAlpha16ToAlpha8,
  exactLinear16ToSrgb8,
  exactRgba16ToRgba8,
  exactUnpremultiply16
} from '../../browser/webgpu_rgba16_exact_tables.js';

assert.equal(WEBGPU_POSTFREEZE_OPTIMIZER_VERSION, '0.1.0-candidate');
assert.equal(WEBGPU_RGBA16_EXPERIMENT_VERSION, '0.1.0-candidate');
assert.equal(WEBGPU_RGBA16_EXPERIMENT_STATE, 'EXPERIMENTAL_NOT_WEBGPU_V1_CONTRACT');
assert.equal(FROZEN_LINEAR16_TO_SRGB8_BUCKET12.length, 4096);
assert.equal(FROZEN_SRGB8_TO_LINEAR16.length, 256);
assert.deepEqual(validateTightRgba16Layout(16, 16, 128), {
  width: 16,
  height: 16,
  strideBytes: 128,
  byteLength: 2048
});
assert.throws(() => validateTightRgba16Layout(16, 16, 64), RangeError);
assert.equal(median([5, 1, 3]), 3);
assert.throws(() => median([]), RangeError);
assert.equal(exactLinear16ToSrgb8(0), 0);
assert.equal(exactLinear16ToSrgb8(65535), 255);
assert.equal(exactAlpha16ToAlpha8(0), 0);
assert.equal(exactAlpha16ToAlpha8(65535), 255);
assert.equal(exactUnpremultiply16(0, 123), 0);
assert.equal(exactUnpremultiply16(123, 123), 65535);
assert.deepEqual(exactRgba16ToRgba8(65535, 65535, 65535, 0), [0, 0, 0, 0]);
assert.deepEqual(exactRgba16ToRgba8(65535, 65535, 65535, 65535), [255, 255, 255, 255]);
assert.equal(typeof OptimizedRgba8WebGpuPresenter, 'function');
console.log('PASS: post-W6 optimizer and exact-conversion JavaScript invariants');
