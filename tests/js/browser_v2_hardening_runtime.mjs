/* BV2H diagnostic/instrumentation driver.
 * TEST-ONLY: browser API observation and timing collection.
 * Authoritative lifecycle transitions, counters and statistical aggregation live in C/WASM.
 */

export const BV2H_TEST_JS_ROLE = 'DIAGNOSTIC_OBSERVER_AND_BROWSER_HOST_DRIVER_ONLY';
export const BV2H_AUTHORITATIVE_JS_LOGIC = 'ZERO';

export const LifecycleEvent = Object.freeze({
  PROBE_BEGIN: 0,
  WEBGPU_READY: 1,
  PLATFORM_FALLBACK: 2,
  DEVICE_LOST: 3,
  RECOVER_BEGIN: 4,
  RECOVER_SUCCESS: 5,
  RECOVER_FALLBACK: 6,
  DESTROY: 7,
  RUNTIME_FALLBACK: 8
});

export const TimingStage = Object.freeze({
  VALIDATE: 0,
  SOURCE_VIEW: 1,
  UPLOAD: 2,
  ENCODE_SUBMIT: 3,
  QUEUE_COMPLETION_LATENCY: 4,
  SYNCHRONIZED_PRESENT: 5,
  HOST_ENQUEUE: 6
});

export const Metric = Object.freeze({
  PRESENTATIONS: 0,
  SOURCE_VIEW_CREATIONS: 1,
  SOURCE_VIEW_REUSES: 2,
  TEXTURE_CREATIONS: 3,
  TEXTURE_REUSES: 4,
  BIND_GROUP_CREATIONS: 5,
  BIND_GROUP_REUSES: 6,
  PIPELINE_CREATIONS: 7,
  PIPELINE_REUSES: 8,
  MEMORY_BUFFER_CHANGES: 9,
  DEVICE_LOSSES: 10,
  RECOVERIES: 11,
  FALLBACKS: 12
});

const lifecycleNames = [
  'COLD', 'PROBING', 'WEBGPU_READY', 'FALLBACK_READY', 'DEVICE_LOST', 'RECOVERING', 'DESTROYED'
];
const timingNames = [
  'validate', 'sourceView', 'uploadCall', 'encodeSubmit', 'queueCompletionLatency', 'synchronizedPresent', 'hostEnqueue'
];
const metricNames = [
  'presentations', 'sourceViewCreations', 'sourceViewReuses', 'textureCreations', 'textureReuses',
  'bindGroupCreations', 'bindGroupReuses', 'pipelineCreations', 'pipelineReuses', 'memoryBufferChanges',
  'deviceLosses', 'recoveries', 'fallbacks'
];

function nowMs() { return performance.now(); }
function elapsedNs(startMs, endMs) {
  const delta = endMs >= startMs ? endMs - startMs : 0;
  return BigInt(Math.round(delta * 1000000));
}
function numeric64(value) {
  return typeof value === 'bigint' ? Number(value) : Number(value || 0);
}

export function measureTimerResolutionNs(iterations = 8192) {
  let previous = nowMs();
  let minimum = Number.POSITIVE_INFINITY;
  for (let i = 0; i < iterations; ++i) {
    const current = nowMs();
    const delta = current - previous;
    if (delta > 0 && delta < minimum) minimum = delta;
    previous = current;
  }
  if (!Number.isFinite(minimum)) return 0;
  return Math.max(1, Math.round(minimum * 1000000));
}

export class BrowserHardeningV2Authority {
  constructor(instance) {
    this.ex = instance.exports;
    this.ex.arbor_browser_hardening_v2_reset();
  }

  transition(event) {
    return this.ex.arbor_browser_hardening_v2_transition(event);
  }

  metric(metric) {
    const status = this.ex.arbor_browser_hardening_v2_metric_increment(metric);
    if (status !== 0) throw new Error(`BV2H metric increment rejected (${metric}, ${status})`);
  }

  timing(stage, startMs, endMs) {
    const status = this.ex.arbor_browser_hardening_v2_timing_record(stage, elapsedNs(startMs, endMs));
    if (status !== 0) throw new Error(`BV2H timing sample rejected (${stage}, ${status})`);
  }

  lifecycle() {
    const state = this.ex.arbor_browser_hardening_v2_lifecycle_state_value();
    return {
      state,
      name: lifecycleNames[state] || `UNKNOWN_${state}`,
      generation: this.ex.arbor_browser_hardening_v2_lifecycle_generation()
    };
  }

  metrics() {
    const out = {};
    metricNames.forEach((name, index) => {
      out[name] = numeric64(this.ex.arbor_browser_hardening_v2_metric_value(index));
    });
    return out;
  }

  timingSummary() {
    const out = {};
    timingNames.forEach((name, stage) => {
      const count = this.ex.arbor_browser_hardening_v2_timing_count(stage);
      out[name] = {
        count,
        minNs: numeric64(this.ex.arbor_browser_hardening_v2_timing_min(stage)),
        meanNs: numeric64(this.ex.arbor_browser_hardening_v2_timing_mean(stage)),
        p50Ns: numeric64(this.ex.arbor_browser_hardening_v2_timing_percentile_permille(stage, 500)),
        p95Ns: numeric64(this.ex.arbor_browser_hardening_v2_timing_percentile_permille(stage, 950)),
        p99Ns: numeric64(this.ex.arbor_browser_hardening_v2_timing_percentile_permille(stage, 990)),
        maxNs: numeric64(this.ex.arbor_browser_hardening_v2_timing_max(stage))
      };
    });
    return out;
  }
}

function accountResourceReuse(authority, presenter, before) {
  if (before.texture && presenter.sourceTexture === before.texture) authority.metric(Metric.TEXTURE_REUSES);
  else authority.metric(Metric.TEXTURE_CREATIONS);

  if (before.bindGroup && presenter.bindGroup === before.bindGroup) authority.metric(Metric.BIND_GROUP_REUSES);
  else authority.metric(Metric.BIND_GROUP_CREATIONS);

  if (before.pipeline && presenter.pipeline === before.pipeline) authority.metric(Metric.PIPELINE_REUSES);
  else authority.metric(Metric.PIPELINE_CREATIONS);

  if (before.buffer && presenter.cachedBuffer !== before.buffer) authority.metric(Metric.MEMORY_BUFFER_CHANGES);
}

export async function measuredPresentRgba8(authority, presenter, byteOffset, width, height, strideBytes) {
  const totalStart = nowMs();

  let start = nowMs();
  const byteLength = presenter.host.rgba8Bytes(width, height, strideBytes);
  let end = nowMs();
  authority.timing(TimingStage.VALIDATE, start, end);

  presenter.host.syncCanvas(presenter.canvas, width, height);
  const before = {
    texture: presenter.sourceTexture,
    bindGroup: presenter.bindGroup,
    pipeline: presenter.pipeline,
    buffer: presenter.cachedBuffer,
    viewCreations: presenter.stats.sourceViewCreations,
    viewReuses: presenter.stats.sourceViewReuses
  };

  start = nowMs();
  presenter.ensureSourceTexture(width, height);
  const source = presenter.sourceBytes(byteOffset, byteLength);
  end = nowMs();
  authority.timing(TimingStage.SOURCE_VIEW, start, end);

  if (presenter.stats.sourceViewCreations > before.viewCreations) authority.metric(Metric.SOURCE_VIEW_CREATIONS);
  if (presenter.stats.sourceViewReuses > before.viewReuses) authority.metric(Metric.SOURCE_VIEW_REUSES);
  accountResourceReuse(authority, presenter, before);

  start = nowMs();
  presenter.device.queue.writeTexture(
    { texture: presenter.sourceTexture },
    source,
    { offset: 0, bytesPerRow: strideBytes, rowsPerImage: height },
    { width, height, depthOrArrayLayers: 1 });
  end = nowMs();
  authority.timing(TimingStage.UPLOAD, start, end);

  start = nowMs();
  const encoder = presenter.device.createCommandEncoder({ label: 'Arborcore BV2H measured present' });
  const pass = encoder.beginRenderPass({
    colorAttachments: [{
      view: presenter.context.getCurrentTexture().createView(),
      clearValue: { r: 0, g: 0, b: 0, a: 0 },
      loadOp: 'clear',
      storeOp: 'store'
    }]
  });
  pass.setPipeline(presenter.pipeline);
  pass.setBindGroup(0, presenter.bindGroup);
  pass.draw(3);
  pass.end();
  presenter.device.queue.submit([encoder.finish()]);
  end = nowMs();
  authority.timing(TimingStage.ENCODE_SUBMIT, start, end);
  const submitEnd = end;
  authority.timing(TimingStage.HOST_ENQUEUE, totalStart, submitEnd);

  start = nowMs();
  await presenter.device.queue.onSubmittedWorkDone();
  end = nowMs();
  authority.timing(TimingStage.QUEUE_COMPLETION_LATENCY, start, end);

  presenter.stats.presentations += 1;
  authority.metric(Metric.PRESENTATIONS);
  authority.timing(TimingStage.SYNCHRONIZED_PRESENT, totalStart, end);
}

export function testDprVectors(host, vectors) {
  return vectors.map(vector => {
    const device = vector.deviceWidth ? [{ inlineSize: vector.deviceWidth, blockSize: vector.deviceHeight }] : null;
    const entry = {
      devicePixelContentBoxSize: device,
      contentBoxSize: [{ inlineSize: vector.cssWidth, blockSize: vector.cssHeight }],
      contentRect: { width: vector.cssWidth, height: vector.cssHeight }
    };
    const actual = host.resolveDevicePixelSize(entry, vector.dpr);
    return {
      name: vector.name,
      expected: { width: vector.expectedWidth, height: vector.expectedHeight, mode: vector.expectedMode },
      actual,
      pass: actual.width === vector.expectedWidth && actual.height === vector.expectedHeight && actual.mode === vector.expectedMode
    };
  });
}

export async function adapterDiagnostics(presenter) {
  if (!presenter.adapter || !presenter.device) return null;
  let info = null;
  try {
    if (presenter.adapter.info) info = { ...presenter.adapter.info };
    else if (typeof presenter.adapter.requestAdapterInfo === 'function') info = { ...(await presenter.adapter.requestAdapterInfo()) };
  } catch (_) {}
  const limitKeys = [
    'maxTextureDimension2D', 'maxBindGroups', 'maxBufferSize', 'maxStorageBufferBindingSize',
    'maxComputeWorkgroupsPerDimension', 'maxComputeInvocationsPerWorkgroup'
  ];
  const limits = {};
  for (const key of limitKeys) {
    const value = presenter.device.limits?.[key];
    if (value !== undefined) limits[key] = typeof value === 'bigint' ? value.toString() : value;
  }
  return {
    info,
    adapterFeatures: Array.from(presenter.adapter.features || []).sort(),
    deviceFeatures: Array.from(presenter.device.features || []).sort(),
    limits,
    canvasFormat: presenter.canvasFormat
  };
}
