import {
    WebGpuPrecisionSurfacePresenter,
    WEBGPU_UPLOAD_FORMAT,
    validateTightRgba8Layout
} from './webgpu_accelerator.js';
import { syncCanvasDeviceSize } from './precision_surface.js';

export const WEBGPU_POSTFREEZE_OPTIMIZER_VERSION = '0.1.0-candidate';
export const WEBGPU_POSTFREEZE_BASELINE = 'WEBGPU_ACCELERATOR_CONTRACT_V1';

function nowMs() {
    if (typeof performance === 'object' && performance !== null &&
        typeof performance.now === 'function') {
        return performance.now();
    }
    return Date.now();
}

export function median(values) {
    if (!Array.isArray(values) || values.length === 0) {
        throw new RangeError('median requires a non-empty array');
    }
    const copy = [...values].sort((a, b) => a - b);
    return copy[Math.floor(copy.length / 2)];
}

export class OptimizedRgba8WebGpuPresenter extends WebGpuPrecisionSurfacePresenter {
    constructor(canvas, memory, options = {}) {
        super(canvas, memory, options);
        this._cachedSourceBuffer = null;
        this._cachedSourceOffset = -1;
        this._cachedSourceLength = -1;
        this._cachedSourceView = null;
        this._submitList = [null];
        this._renderAttachment = {
            view: null,
            clearValue: { r: 0, g: 0, b: 0, a: 0 },
            loadOp: 'clear',
            storeOp: 'store'
        };
        this._renderPassDescriptor = {
            colorAttachments: [this._renderAttachment]
        };
        this._stats = {
            sourceViewCreations: 0,
            sourceViewReuses: 0,
            memoryEpochChanges: 0,
            sourceTextureReconfigures: 0,
            presentations: 0
        };
    }

    _resetSourceViewCache() {
        this._cachedSourceBuffer = null;
        this._cachedSourceOffset = -1;
        this._cachedSourceLength = -1;
        this._cachedSourceView = null;
    }

    _releaseGpuObjects(destroyDevice) {
        this._resetSourceViewCache();
        super._releaseGpuObjects(destroyDevice);
    }

    _ensureSourceTexture(width, height) {
        const changed = !this.sourceTexture || this.sourceWidth !== width || this.sourceHeight !== height;
        super._ensureSourceTexture(width, height);
        if (changed) this._stats.sourceTextureReconfigures += 1;
    }

    _sourceView(byteOffset, byteLength) {
        const changed = this.memoryEpoch.refresh();
        if (changed) {
            this._stats.memoryEpochChanges += 1;
            this._resetSourceViewCache();
        }
        const buffer = this.memory.buffer;
        if (byteOffset < 0 || byteLength < 0 || byteOffset + byteLength > buffer.byteLength) {
            throw new RangeError('WASM memory view is out of bounds');
        }
        if (this._cachedSourceView && this._cachedSourceBuffer === buffer &&
            this._cachedSourceOffset === byteOffset && this._cachedSourceLength === byteLength) {
            this._stats.sourceViewReuses += 1;
            return this._cachedSourceView;
        }
        this._cachedSourceBuffer = buffer;
        this._cachedSourceOffset = byteOffset;
        this._cachedSourceLength = byteLength;
        this._cachedSourceView = new Uint8Array(buffer, byteOffset, byteLength);
        this._stats.sourceViewCreations += 1;
        return this._cachedSourceView;
    }

    statsRecord() {
        return { ...this._stats };
    }

    _presentCore(byteOffset, width, height, strideBytes, profile) {
        if (this.state !== 'ready' || !this.device || !this.context || !this.pipeline) {
            throw new Error(`WebGPU presenter is not ready (${this.state})`);
        }
        const totalStart = profile ? nowMs() : 0;
        const layout = validateTightRgba8Layout(width, height, strideBytes);
        const changed = syncCanvasDeviceSize(this.canvas, width, height);
        if (changed) this._configureContext();
        this._ensureSourceTexture(width, height);

        const viewStart = profile ? nowMs() : 0;
        const source = this._sourceView(byteOffset, layout.byteLength);
        const viewEnd = profile ? nowMs() : 0;

        const uploadStart = profile ? nowMs() : 0;
        this.device.queue.writeTexture(
            { texture: this.sourceTexture },
            source,
            { offset: 0, bytesPerRow: strideBytes, rowsPerImage: height },
            { width, height, depthOrArrayLayers: 1 });
        const uploadEnd = profile ? nowMs() : 0;

        const encodeStart = profile ? nowMs() : 0;
        const encoder = this.device.createCommandEncoder({ label: 'Arborcore post-W6 optimized present encoder' });
        this._renderAttachment.view = this.context.getCurrentTexture().createView();
        const pass = encoder.beginRenderPass(this._renderPassDescriptor);
        pass.setPipeline(this.pipeline);
        pass.setBindGroup(0, this.bindGroup);
        pass.draw(3);
        pass.end();
        const commandBuffer = encoder.finish();
        const encodeEnd = profile ? nowMs() : 0;

        const submitStart = profile ? nowMs() : 0;
        this._submitList[0] = commandBuffer;
        this.device.queue.submit(this._submitList);
        this._submitList[0] = null;
        const submitEnd = profile ? nowMs() : 0;
        this._stats.presentations += 1;

        const result = {
            mode: 'webgpu',
            width,
            height,
            sourceFormat: WEBGPU_UPLOAD_FORMAT,
            canvasFormat: this.canvasFormat,
            generation: this.generation,
            optimizerVersion: WEBGPU_POSTFREEZE_OPTIMIZER_VERSION
        };
        if (profile) {
            result.profile = {
                viewAcquireMs: viewEnd - viewStart,
                writeTextureCallMs: uploadEnd - uploadStart,
                encodeMs: encodeEnd - encodeStart,
                submitCallMs: submitEnd - submitStart,
                totalSubmitPathMs: submitEnd - totalStart
            };
        }
        return result;
    }

    presentRgba8(byteOffset, width, height, strideBytes) {
        return this._presentCore(byteOffset, width, height, strideBytes, false);
    }

    presentRgba8Profiled(byteOffset, width, height, strideBytes) {
        return this._presentCore(byteOffset, width, height, strideBytes, true);
    }
}
