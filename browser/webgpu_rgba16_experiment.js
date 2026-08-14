import { syncCanvasDeviceSize } from './precision_surface.js';
import {
    FROZEN_LINEAR16_TO_SRGB8_BUCKET12,
    FROZEN_SRGB8_TO_LINEAR16
} from './webgpu_rgba16_exact_tables.js';

export const WEBGPU_RGBA16_EXPERIMENT_VERSION = '0.1.0-candidate';
export const WEBGPU_RGBA16_EXPERIMENT_STATE = 'EXPERIMENTAL_NOT_WEBGPU_V1_CONTRACT';
export const WEBGPU_RGBA16_UPLOAD_FORMAT = 'rgba16uint';

function nowMs() {
    if (typeof performance === 'object' && performance !== null &&
        typeof performance.now === 'function') return performance.now();
    return Date.now();
}

export function validateTightRgba16Layout(width, height, strideBytes) {
    if (!Number.isSafeInteger(width) || !Number.isSafeInteger(height) ||
        !Number.isSafeInteger(strideBytes) || width <= 0 || height <= 0) {
        throw new RangeError('RGBA16 dimensions/stride must be positive safe integers');
    }
    const tightStride = width * 8;
    if (!Number.isSafeInteger(tightStride) || strideBytes !== tightStride) {
        throw new RangeError('post-W6 RGBA16 experiment requires tight 8-byte pixels');
    }
    const byteLength = tightStride * height;
    if (!Number.isSafeInteger(byteLength)) {
        throw new RangeError('RGBA16 byte length exceeds JavaScript safe-integer range');
    }
    return { width, height, strideBytes, byteLength };
}

const EXACT_RGBA16_SHADER = /* wgsl */`
@group(0) @binding(0) var source_rgba16: texture_2d<u32>;
@group(0) @binding(1) var<storage, read> bucket12: array<u32>;
@group(0) @binding(2) var<storage, read> srgb8_to_linear16: array<u32>;

fn unpremultiply16(value: u32, alpha: u32) -> u32 {
    if (alpha == 0u || value == 0u) { return 0u; }
    if (value >= alpha) { return 65535u; }
    if (alpha == 65535u) { return value; }
    let numerator = value * 65535u;
    var quotient = numerator / alpha;
    let remainder = numerator % alpha;
    let twice = remainder * 2u;
    if (twice > alpha || (twice == alpha && (quotient & 1u) != 0u)) {
        quotient = quotient + 1u;
    }
    return min(quotient, 65535u);
}

fn linear16_to_srgb8(value: u32) -> u32 {
    var code = bucket12[value >> 4u];
    if (code < 255u) {
        let lower = srgb8_to_linear16[code];
        let upper = srgb8_to_linear16[code + 1u];
        let twice_value = value * 2u;
        let midpoint_sum = lower + upper;
        if (twice_value > midpoint_sum ||
            (twice_value == midpoint_sum && (code & 1u) != 0u)) {
            code = code + 1u;
        }
    }
    return code;
}

fn alpha16_to_alpha8(alpha: u32) -> u32 {
    var quotient = alpha / 257u;
    let remainder = alpha % 257u;
    let twice = remainder * 2u;
    if (twice > 257u || (twice == 257u && (quotient & 1u) != 0u)) {
        quotient = quotient + 1u;
    }
    return min(quotient, 255u);
}

fn exact_rgba16_to_rgba8(color: vec4<u32>) -> vec4<u32> {
    let alpha = color.a;
    if (alpha == 0u) {
        return vec4<u32>(0u, 0u, 0u, 0u);
    }
    if (alpha == 65535u) {
        return vec4<u32>(
            linear16_to_srgb8(color.r),
            linear16_to_srgb8(color.g),
            linear16_to_srgb8(color.b),
            255u);
    }
    return vec4<u32>(
        linear16_to_srgb8(unpremultiply16(color.r, alpha)),
        linear16_to_srgb8(unpremultiply16(color.g, alpha)),
        linear16_to_srgb8(unpremultiply16(color.b, alpha)),
        alpha16_to_alpha8(alpha));
}

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
    let exact = exact_rgba16_to_rgba8(textureLoad(source_rgba16, pixel, 0));
    let a = f32(exact.a) / 255.0;
    let rgb = vec3<f32>(f32(exact.r), f32(exact.g), f32(exact.b)) / 255.0;
    return vec4<f32>(rgb * a, a);
}

@group(0) @binding(3) var<storage, read_write> output_rgba8: array<u32>;

@compute @workgroup_size(8, 8, 1)
fn compute_main(@builtin(global_invocation_id) gid: vec3<u32>) {
    let dimensions = textureDimensions(source_rgba16);
    if (gid.x >= dimensions.x || gid.y >= dimensions.y) { return; }
    let exact = exact_rgba16_to_rgba8(textureLoad(
        source_rgba16, vec2<i32>(i32(gid.x), i32(gid.y)), 0));
    let packed = exact.r | (exact.g << 8u) | (exact.b << 16u) | (exact.a << 24u);
    output_rgba8[gid.y * dimensions.x + gid.x] = packed;
}
`;

export class DirectRgba16WebGpuExperiment {
    constructor(canvas, memory) {
        if (!canvas || !(memory instanceof WebAssembly.Memory)) {
            throw new TypeError('Canvas and WebAssembly.Memory are required');
        }
        this.canvas = canvas;
        this.memory = memory;
        this.adapter = null;
        this.device = null;
        this.context = null;
        this.canvasFormat = '';
        this.sourceTexture = null;
        this.sourceView = null;
        this.sourceWidth = 0;
        this.sourceHeight = 0;
        this.bucketBuffer = null;
        this.forwardBuffer = null;
        this.renderPipeline = null;
        this.computePipeline = null;
        this.renderBindGroup = null;
        this.outputBuffer = null;
        this.readbackBuffer = null;
        this.computeBindGroup = null;
        this.outputPixels = 0;
        this._cachedInputBuffer = null;
        this._cachedInputOffset = -1;
        this._cachedInputLength = -1;
        this._cachedInputView = null;
        this._renderAttachment = {
            view: null,
            clearValue: { r: 0, g: 0, b: 0, a: 0 },
            loadOp: 'clear',
            storeOp: 'store'
        };
        this._renderDescriptor = { colorAttachments: [this._renderAttachment] };
        this._submitList = [null];
        this.state = 'new';
    }

    async initializeFromPresenter(presenter) {
        if (!presenter || presenter.state !== 'ready' || !presenter.device || !presenter.adapter) {
            throw new Error('ready WebGPU v1 presenter is required for the RGBA16 experiment');
        }
        this.adapter = presenter.adapter;
        this.device = presenter.device;
        this.canvasFormat = presenter.canvasFormat;
        this.context = this.canvas.getContext('webgpu');
        if (!this.context) throw new Error('GPUCanvasContext unavailable for RGBA16 experiment');
        this.context.configure({
            device: this.device,
            format: this.canvasFormat,
            colorSpace: 'srgb',
            alphaMode: 'premultiplied'
        });
        this.bucketBuffer = this.device.createBuffer({
            label: 'Arborcore post-W6 frozen bucket12 table',
            size: FROZEN_LINEAR16_TO_SRGB8_BUCKET12.byteLength,
            usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_DST
        });
        this.forwardBuffer = this.device.createBuffer({
            label: 'Arborcore post-W6 frozen sRGB8->linear16 table',
            size: FROZEN_SRGB8_TO_LINEAR16.byteLength,
            usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_DST
        });
        this.device.queue.writeBuffer(this.bucketBuffer, 0, FROZEN_LINEAR16_TO_SRGB8_BUCKET12);
        this.device.queue.writeBuffer(this.forwardBuffer, 0, FROZEN_SRGB8_TO_LINEAR16);
        const shader = this.device.createShaderModule({
            label: 'Arborcore post-W6 exact RGBA16 conversion experiment',
            code: EXACT_RGBA16_SHADER
        });
        this.renderPipeline = await this.device.createRenderPipelineAsync({
            label: 'Arborcore post-W6 direct RGBA16 presentation experiment',
            layout: 'auto',
            vertex: { module: shader, entryPoint: 'vertex_main' },
            fragment: {
                module: shader,
                entryPoint: 'fragment_main',
                targets: [{ format: this.canvasFormat }]
            },
            primitive: { topology: 'triangle-list' }
        });
        this.computePipeline = await this.device.createComputePipelineAsync({
            label: 'Arborcore post-W6 exact RGBA16->RGBA8 compute experiment',
            layout: 'auto',
            compute: { module: shader, entryPoint: 'compute_main' }
        });
        this.state = 'ready';
        return true;
    }

    _sourceBytes(byteOffset, byteLength) {
        const buffer = this.memory.buffer;
        if (byteOffset < 0 || byteLength < 0 || byteOffset + byteLength > buffer.byteLength) {
            throw new RangeError('RGBA16 experiment memory view is out of bounds');
        }
        if (this._cachedInputView && this._cachedInputBuffer === buffer &&
            this._cachedInputOffset === byteOffset && this._cachedInputLength === byteLength) {
            return this._cachedInputView;
        }
        this._cachedInputBuffer = buffer;
        this._cachedInputOffset = byteOffset;
        this._cachedInputLength = byteLength;
        this._cachedInputView = new Uint8Array(buffer, byteOffset, byteLength);
        return this._cachedInputView;
    }

    _releaseSizedResources() {
        for (const resource of [this.sourceTexture, this.outputBuffer, this.readbackBuffer]) {
            if (resource) {
                try { resource.destroy(); } catch (_) {}
            }
        }
        this.sourceTexture = null;
        this.sourceView = null;
        this.renderBindGroup = null;
        this.outputBuffer = null;
        this.readbackBuffer = null;
        this.computeBindGroup = null;
        this.sourceWidth = 0;
        this.sourceHeight = 0;
        this.outputPixels = 0;
    }

    _ensureSizedResources(width, height) {
        if (this.sourceTexture && this.sourceWidth === width && this.sourceHeight === height) return;
        this._releaseSizedResources();
        this.sourceTexture = this.device.createTexture({
            label: 'Arborcore post-W6 RGBA16 experimental upload texture',
            size: { width, height, depthOrArrayLayers: 1 },
            format: WEBGPU_RGBA16_UPLOAD_FORMAT,
            mipLevelCount: 1,
            sampleCount: 1,
            dimension: '2d',
            usage: GPUTextureUsage.COPY_DST | GPUTextureUsage.TEXTURE_BINDING
        });
        this.sourceView = this.sourceTexture.createView();
        this.renderBindGroup = this.device.createBindGroup({
            layout: this.renderPipeline.getBindGroupLayout(0),
            entries: [
                { binding: 0, resource: this.sourceView },
                { binding: 1, resource: { buffer: this.bucketBuffer } },
                { binding: 2, resource: { buffer: this.forwardBuffer } }
            ]
        });
        const pixels = width * height;
        const outputBytes = pixels * 4;
        this.outputBuffer = this.device.createBuffer({
            label: 'Arborcore post-W6 RGBA8 exact-conversion output',
            size: outputBytes,
            usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_SRC
        });
        this.readbackBuffer = this.device.createBuffer({
            label: 'Arborcore post-W6 RGBA8 exact-conversion readback',
            size: outputBytes,
            usage: GPUBufferUsage.COPY_DST | GPUBufferUsage.MAP_READ
        });
        this.computeBindGroup = this.device.createBindGroup({
            layout: this.computePipeline.getBindGroupLayout(0),
            entries: [
                { binding: 0, resource: this.sourceView },
                { binding: 1, resource: { buffer: this.bucketBuffer } },
                { binding: 2, resource: { buffer: this.forwardBuffer } },
                { binding: 3, resource: { buffer: this.outputBuffer } }
            ]
        });
        this.sourceWidth = width;
        this.sourceHeight = height;
        this.outputPixels = pixels;
    }

    _upload(byteOffset, width, height, strideBytes) {
        const layout = validateTightRgba16Layout(width, height, strideBytes);
        this._ensureSizedResources(width, height);
        const source = this._sourceBytes(byteOffset, layout.byteLength);
        this.device.queue.writeTexture(
            { texture: this.sourceTexture },
            source,
            { offset: 0, bytesPerRow: strideBytes, rowsPerImage: height },
            { width, height, depthOrArrayLayers: 1 });
        return layout;
    }

    presentRgba16(byteOffset, width, height, strideBytes, profile = false) {
        if (this.state !== 'ready') throw new Error('RGBA16 experiment is not ready');
        const totalStart = profile ? nowMs() : 0;
        const layout = validateTightRgba16Layout(width, height, strideBytes);
        const changed = syncCanvasDeviceSize(this.canvas, width, height);
        if (changed) {
            this.context.configure({
                device: this.device,
                format: this.canvasFormat,
                colorSpace: 'srgb',
                alphaMode: 'premultiplied'
            });
        }
        this._ensureSizedResources(width, height);
        const viewStart = profile ? nowMs() : 0;
        const source = this._sourceBytes(byteOffset, layout.byteLength);
        const viewEnd = profile ? nowMs() : 0;
        const uploadStart = profile ? nowMs() : 0;
        this.device.queue.writeTexture(
            { texture: this.sourceTexture }, source,
            { offset: 0, bytesPerRow: strideBytes, rowsPerImage: height },
            { width, height, depthOrArrayLayers: 1 });
        const uploadEnd = profile ? nowMs() : 0;
        const encodeStart = profile ? nowMs() : 0;
        const encoder = this.device.createCommandEncoder({ label: 'Arborcore post-W6 RGBA16 present encoder' });
        this._renderAttachment.view = this.context.getCurrentTexture().createView();
        const pass = encoder.beginRenderPass(this._renderDescriptor);
        pass.setPipeline(this.renderPipeline);
        pass.setBindGroup(0, this.renderBindGroup);
        pass.draw(3);
        pass.end();
        const command = encoder.finish();
        const encodeEnd = profile ? nowMs() : 0;
        const submitStart = profile ? nowMs() : 0;
        this._submitList[0] = command;
        this.device.queue.submit(this._submitList);
        this._submitList[0] = null;
        const submitEnd = profile ? nowMs() : 0;
        const result = { mode: 'webgpu-rgba16-experiment', width, height };
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

    async convertRgba16ToRgba8Readback(byteOffset, width, height, strideBytes) {
        if (this.state !== 'ready') throw new Error('RGBA16 experiment is not ready');
        this._upload(byteOffset, width, height, strideBytes);
        const encoder = this.device.createCommandEncoder({ label: 'Arborcore post-W6 exact conversion encoder' });
        const pass = encoder.beginComputePass({ label: 'Arborcore post-W6 exact conversion pass' });
        pass.setPipeline(this.computePipeline);
        pass.setBindGroup(0, this.computeBindGroup);
        pass.dispatchWorkgroups(Math.ceil(width / 8), Math.ceil(height / 8), 1);
        pass.end();
        encoder.copyBufferToBuffer(this.outputBuffer, 0, this.readbackBuffer, 0, this.outputPixels * 4);
        this.device.queue.submit([encoder.finish()]);
        await this.readbackBuffer.mapAsync(GPUMapMode.READ);
        try {
            const words = new Uint32Array(this.readbackBuffer.getMappedRange());
            const out = new Uint8Array(this.outputPixels * 4);
            for (let i = 0; i < this.outputPixels; ++i) {
                const value = words[i];
                const j = i * 4;
                out[j] = value & 0xff;
                out[j + 1] = (value >>> 8) & 0xff;
                out[j + 2] = (value >>> 16) & 0xff;
                out[j + 3] = (value >>> 24) & 0xff;
            }
            return out;
        } finally {
            this.readbackBuffer.unmap();
        }
    }

    async awaitWorkDone() {
        if (!this.device || this.state !== 'ready') throw new Error('RGBA16 experiment is not ready');
        await this.device.queue.onSubmittedWorkDone();
    }

    destroy() {
        this._releaseSizedResources();
        for (const resource of [this.bucketBuffer, this.forwardBuffer]) {
            if (resource) {
                try { resource.destroy(); } catch (_) {}
            }
        }
        if (this.context) {
            try { this.context.unconfigure(); } catch (_) {}
        }
        this.bucketBuffer = null;
        this.forwardBuffer = null;
        this.renderPipeline = null;
        this.computePipeline = null;
        this.context = null;
        this.state = 'destroyed';
    }
}
