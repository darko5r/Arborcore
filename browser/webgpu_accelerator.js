import {
    PrecisionSurfaceMemoryEpoch,
    PrecisionSurfacePresenter,
    syncCanvasDeviceSize
} from './precision_surface.js';

export const WEBGPU_ACCELERATOR_VERSION = '1.0-candidate';
export const WEBGPU_UPLOAD_FORMAT = 'rgba8unorm';
export const WEBGPU_ALPHA_MODE = 'premultiplied';

export function alignTo(value, alignment) {
    if (!Number.isSafeInteger(value) || !Number.isSafeInteger(alignment) ||
        value < 0 || alignment <= 0) {
        throw new RangeError('alignTo requires a non-negative safe integer and positive alignment');
    }
    return Math.ceil(value / alignment) * alignment;
}

export function validateTightRgba8Layout(width, height, strideBytes) {
    if (!Number.isSafeInteger(width) || !Number.isSafeInteger(height) ||
        !Number.isSafeInteger(strideBytes) || width <= 0 || height <= 0) {
        throw new RangeError('RGBA8 dimensions/stride must be positive safe integers');
    }
    const tightStride = width * 4;
    if (!Number.isSafeInteger(tightStride) || strideBytes !== tightStride) {
        throw new RangeError('WebGPU transport requires tight RGBA8 rows');
    }
    const byteLength = tightStride * height;
    if (!Number.isSafeInteger(byteLength)) {
        throw new RangeError('RGBA8 byte length exceeds JavaScript safe-integer range');
    }
    return { width, height, strideBytes, byteLength };
}

function globalGpu() {
    return typeof navigator === 'object' && navigator !== null ? navigator.gpu : undefined;
}

function gpuConstantsAvailable() {
    return typeof GPUTextureUsage === 'object' &&
        typeof GPUBufferUsage === 'object' &&
        typeof GPUMapMode === 'object';
}

export function webGpuApiAvailable() {
    const gpu = globalGpu();
    return Boolean(gpu && typeof gpu.requestAdapter === 'function' &&
        typeof gpu.getPreferredCanvasFormat === 'function' && gpuConstantsAvailable());
}

function adapterInfoRecord(adapter) {
    const info = adapter && adapter.info ? adapter.info : {};
    return {
        vendor: String(info.vendor || ''),
        architecture: String(info.architecture || ''),
        device: String(info.device || ''),
        description: String(info.description || ''),
        isFallbackAdapter: Boolean(info.isFallbackAdapter)
    };
}

function sortedFeatureNames(features) {
    return Array.from(features || [], value => String(value)).sort();
}

function selectedLimits(limits) {
    if (!limits) return {};
    const names = [
        'maxTextureDimension2D',
        'maxBindGroups',
        'maxBindingsPerBindGroup',
        'maxBufferSize',
        'maxStorageBufferBindingSize',
        'maxComputeWorkgroupStorageSize',
        'maxComputeInvocationsPerWorkgroup'
    ];
    const result = {};
    for (const name of names) {
        if (typeof limits[name] === 'number') result[name] = limits[name];
    }
    return result;
}

const PRESENT_SHADER = /* wgsl */`
@group(0) @binding(0) var source_texture: texture_2d<f32>;

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
};

@vertex
fn vertex_main(@builtin(vertex_index) index: u32) -> VertexOutput {
    var positions = array<vec2<f32>, 3>(
        vec2<f32>(-1.0, -1.0),
        vec2<f32>( 3.0, -1.0),
        vec2<f32>(-1.0,  3.0)
    );
    var output: VertexOutput;
    output.position = vec4<f32>(positions[index], 0.0, 1.0);
    return output;
}

@fragment
fn fragment_main(input: VertexOutput) -> @location(0) vec4<f32> {
    let pixel = vec2<i32>(i32(input.position.x), i32(input.position.y));
    let color = textureLoad(source_texture, pixel, 0);
    return vec4<f32>(color.rgb * color.a, color.a);
}
`;

export class WebGpuPrecisionSurfacePresenter {
    constructor(canvas, memory, options = {}) {
        if (!canvas || !(memory instanceof WebAssembly.Memory)) {
            throw new TypeError('Canvas and WebAssembly.Memory are required');
        }
        this.canvas = canvas;
        this.memory = memory;
        this.memoryEpoch = new PrecisionSurfaceMemoryEpoch(memory);
        this.powerPreference = options.powerPreference || 'high-performance';
        this.alphaMode = options.alphaMode || WEBGPU_ALPHA_MODE;
        this.onStateChange = typeof options.onStateChange === 'function' ? options.onStateChange : null;
        this.state = 'new';
        this.reason = '';
        this.adapter = null;
        this.device = null;
        this.context = null;
        this.canvasFormat = '';
        this.pipeline = null;
        this.sourceTexture = null;
        this.sourceView = null;
        this.bindGroup = null;
        this.sourceWidth = 0;
        this.sourceHeight = 0;
        this.generation = 0;
        this.lastLostInfo = null;
    }

    _setState(state, reason = '') {
        this.state = state;
        this.reason = reason;
        if (this.onStateChange) {
            this.onStateChange({ state, reason, presenter: this });
        }
    }

    capabilityRecord() {
        return {
            apiAvailable: webGpuApiAvailable(),
            state: this.state,
            reason: this.reason,
            canvasFormat: this.canvasFormat,
            adapterInfo: adapterInfoRecord(this.adapter),
            adapterFeatures: sortedFeatureNames(this.adapter && this.adapter.features),
            deviceFeatures: sortedFeatureNames(this.device && this.device.features),
            limits: selectedLimits(this.device && this.device.limits),
            generation: this.generation
        };
    }

    async initialize() {
        if (!webGpuApiAvailable()) {
            this._setState('unavailable', 'navigator.gpu unavailable');
            return false;
        }
        this._releaseGpuObjects(false);
        this._setState('initializing');
        const gpu = globalGpu();
        let adapter;
        try {
            adapter = await gpu.requestAdapter({ powerPreference: this.powerPreference });
        } catch (error) {
            this._setState('unavailable', `requestAdapter failed: ${String(error)}`);
            return false;
        }
        if (!adapter) {
            this._setState('unavailable', 'requestAdapter returned null');
            return false;
        }

        let device;
        try {
            device = await adapter.requestDevice();
        } catch (error) {
            this.adapter = adapter;
            this._setState('unavailable', `requestDevice failed: ${String(error)}`);
            return false;
        }

        const context = this.canvas.getContext('webgpu');
        if (!context) {
            try { device.destroy(); } catch (_) {}
            this.adapter = adapter;
            this._setState('unavailable', 'GPUCanvasContext unavailable');
            return false;
        }

        const format = gpu.getPreferredCanvasFormat();
        this.adapter = adapter;
        this.device = device;
        this.context = context;
        this.canvasFormat = format;
        this.generation += 1;
        const generation = this.generation;

        device.lost.then(info => {
            if (this.generation !== generation || this.device !== device) return;
            this.lastLostInfo = {
                reason: String(info && info.reason ? info.reason : ''),
                message: String(info && info.message ? info.message : '')
            };
            this._setState('lost', `${this.lastLostInfo.reason}:${this.lastLostInfo.message}`);
        }).catch(error => {
            if (this.generation === generation && this.device === device) {
                this._setState('lost', `device.lost rejected: ${String(error)}`);
            }
        });

        try {
            this._configureContext();
            const shader = device.createShaderModule({
                label: 'Arborcore frozen-B1 WebGPU presentation shader',
                code: PRESENT_SHADER
            });
            this.pipeline = await device.createRenderPipelineAsync({
                label: 'Arborcore WebGPU presentation pipeline',
                layout: 'auto',
                vertex: { module: shader, entryPoint: 'vertex_main' },
                fragment: {
                    module: shader,
                    entryPoint: 'fragment_main',
                    targets: [{ format }]
                },
                primitive: { topology: 'triangle-list' }
            });
        } catch (error) {
            this._releaseGpuObjects(true);
            this._setState('unavailable', `WebGPU pipeline initialization failed: ${String(error)}`);
            return false;
        }

        this._setState('ready');
        return true;
    }

    _configureContext() {
        this.context.configure({
            device: this.device,
            format: this.canvasFormat,
            colorSpace: 'srgb',
            alphaMode: this.alphaMode
        });
    }

    _releaseSourceTexture() {
        if (this.sourceTexture) {
            try { this.sourceTexture.destroy(); } catch (_) {}
        }
        this.sourceTexture = null;
        this.sourceView = null;
        this.bindGroup = null;
        this.sourceWidth = 0;
        this.sourceHeight = 0;
    }

    _releaseGpuObjects(destroyDevice) {
        this._releaseSourceTexture();
        if (this.context) {
            try { this.context.unconfigure(); } catch (_) {}
        }
        if (destroyDevice && this.device) {
            try { this.device.destroy(); } catch (_) {}
        }
        this.pipeline = null;
        this.context = null;
        this.device = null;
        this.adapter = null;
        this.canvasFormat = '';
    }

    _ensureSourceTexture(width, height) {
        if (this.sourceTexture && this.sourceWidth === width && this.sourceHeight === height) return;
        this._releaseSourceTexture();
        this.sourceTexture = this.device.createTexture({
            label: 'Arborcore frozen B1 RGBA8 transport texture',
            size: { width, height, depthOrArrayLayers: 1 },
            format: WEBGPU_UPLOAD_FORMAT,
            mipLevelCount: 1,
            sampleCount: 1,
            dimension: '2d',
            usage: GPUTextureUsage.COPY_DST |
                GPUTextureUsage.COPY_SRC |
                GPUTextureUsage.TEXTURE_BINDING
        });
        this.sourceView = this.sourceTexture.createView();
        this.bindGroup = this.device.createBindGroup({
            layout: this.pipeline.getBindGroupLayout(0),
            entries: [{ binding: 0, resource: this.sourceView }]
        });
        this.sourceWidth = width;
        this.sourceHeight = height;
    }

    presentRgba8(byteOffset, width, height, strideBytes) {
        if (this.state !== 'ready' || !this.device || !this.context || !this.pipeline) {
            throw new Error(`WebGPU presenter is not ready (${this.state})`);
        }
        const layout = validateTightRgba8Layout(width, height, strideBytes);
        const changed = syncCanvasDeviceSize(this.canvas, width, height);
        if (changed) this._configureContext();
        this._ensureSourceTexture(width, height);
        const source = this.memoryEpoch.view(byteOffset, layout.byteLength, Uint8Array);
        this.device.queue.writeTexture(
            { texture: this.sourceTexture },
            source,
            { offset: 0, bytesPerRow: strideBytes, rowsPerImage: height },
            { width, height, depthOrArrayLayers: 1 });

        const encoder = this.device.createCommandEncoder({ label: 'Arborcore WebGPU present encoder' });
        const pass = encoder.beginRenderPass({
            colorAttachments: [{
                view: this.context.getCurrentTexture().createView(),
                clearValue: { r: 0, g: 0, b: 0, a: 0 },
                loadOp: 'clear',
                storeOp: 'store'
            }]
        });
        pass.setPipeline(this.pipeline);
        pass.setBindGroup(0, this.bindGroup);
        pass.draw(3);
        pass.end();
        this.device.queue.submit([encoder.finish()]);
        return {
            mode: 'webgpu',
            width,
            height,
            sourceFormat: WEBGPU_UPLOAD_FORMAT,
            canvasFormat: this.canvasFormat,
            generation: this.generation
        };
    }

    async awaitWorkDone() {
        if (!this.device || this.state !== 'ready') {
            throw new Error(`WebGPU presenter is not ready (${this.state})`);
        }
        await this.device.queue.onSubmittedWorkDone();
    }

    async readbackUploadedRgba8() {
        if (!this.device || !this.sourceTexture || this.state !== 'ready') {
            throw new Error('No ready WebGPU upload texture is available for readback');
        }
        const bytesPerPixel = 4;
        const tightRow = this.sourceWidth * bytesPerPixel;
        const copyRow = alignTo(tightRow, 256);
        const bufferSize = copyRow * this.sourceHeight;
        const buffer = this.device.createBuffer({
            label: 'Arborcore WebGPU qualification readback',
            size: bufferSize,
            usage: GPUBufferUsage.COPY_DST | GPUBufferUsage.MAP_READ
        });
        try {
            const encoder = this.device.createCommandEncoder({ label: 'Arborcore WebGPU readback encoder' });
            encoder.copyTextureToBuffer(
                { texture: this.sourceTexture },
                { buffer, offset: 0, bytesPerRow: copyRow, rowsPerImage: this.sourceHeight },
                { width: this.sourceWidth, height: this.sourceHeight, depthOrArrayLayers: 1 });
            this.device.queue.submit([encoder.finish()]);
            await buffer.mapAsync(GPUMapMode.READ);
            const mapped = new Uint8Array(buffer.getMappedRange());
            const result = new Uint8Array(tightRow * this.sourceHeight);
            for (let y = 0; y < this.sourceHeight; ++y) {
                result.set(mapped.subarray(y * copyRow, y * copyRow + tightRow), y * tightRow);
            }
            buffer.unmap();
            return result;
        } finally {
            try { buffer.destroy(); } catch (_) {}
        }
    }

    async recover() {
        this._releaseGpuObjects(false);
        this._setState('recovering');
        return this.initialize();
    }

    destroy() {
        this.generation += 1;
        this._releaseGpuObjects(true);
        this._setState('destroyed');
    }
}

export class AdaptivePrecisionSurfacePresenter {
    constructor(webGpuCanvas, fallbackCanvas, memory, options = {}) {
        if (!webGpuCanvas || !fallbackCanvas || webGpuCanvas === fallbackCanvas) {
            throw new TypeError('WebGPU and fallback presentation require distinct canvases');
        }
        this.webGpuCanvas = webGpuCanvas;
        this.fallbackCanvas = fallbackCanvas;
        this.memory = memory;
        this.fallback = new PrecisionSurfacePresenter(fallbackCanvas, memory);
        this.mode = 'fallback';
        this.lastWebGpuFailure = '';
        this.webgpu = new WebGpuPrecisionSurfacePresenter(webGpuCanvas, memory, {
            ...options,
            onStateChange: event => {
                if (event.state === 'lost' || event.state === 'unavailable') {
                    this.lastWebGpuFailure = event.reason;
                    this._activate('fallback');
                }
                if (typeof options.onStateChange === 'function') options.onStateChange(event);
            }
        });
        this._activate('fallback');
    }

    _activate(mode) {
        this.mode = mode;
        if ('hidden' in this.webGpuCanvas) this.webGpuCanvas.hidden = mode !== 'webgpu';
        if ('hidden' in this.fallbackCanvas) this.fallbackCanvas.hidden = mode === 'webgpu';
    }

    async initialize() {
        const ready = await this.webgpu.initialize();
        this._activate(ready ? 'webgpu' : 'fallback');
        return {
            mode: this.mode,
            webgpu: this.webgpu.capabilityRecord()
        };
    }

    presentRgba8(byteOffset, width, height, strideBytes) {
        if (this.mode === 'webgpu' && this.webgpu.state === 'ready') {
            try {
                return this.webgpu.presentRgba8(byteOffset, width, height, strideBytes);
            } catch (error) {
                this.lastWebGpuFailure = String(error);
                this._activate('fallback');
            }
        }
        const imageData = this.fallback.presentRgba8(byteOffset, width, height, strideBytes);
        return { mode: 'canvas2d-fallback', width, height, imageData };
    }

    async recoverWebGpu() {
        const ready = await this.webgpu.recover();
        this._activate(ready ? 'webgpu' : 'fallback');
        return ready;
    }

    destroy() {
        this.webgpu.destroy();
        this._activate('fallback');
    }
}
