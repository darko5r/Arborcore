/* Arborcore Browser Host Boundary v2.
 *
 * This module is deliberately a host syscall shim. Numerical policy,
 * geometry, layout validation, failure classification, lookup tables and
 * presentation state live in C/WASM. GPU arithmetic lives in WGSL.
 */

export const ARBORCORE_HOST_JS_ROLE = 'BROWSER_HOST_SYSCALL_SHIM_ONLY';
export const ARBORCORE_HOST_JS_AUTHORITY = 'ZERO';
export const AUTHORITATIVE_JS_LOGIC = 'ZERO';

const OK = 0;
const GPU_FAILURE_PROGRAMMING = 3;

function firstBoxSize(value) {
    if (!value) return null;
    if (Array.isArray(value)) return value.length ? value[0] : null;
    if (typeof value.length === 'number') return value.length ? value[0] : null;
    return value;
}

function errorText(error) {
    if (error && typeof error.message === 'string') return `${error.name || 'Error'}:${error.message}`;
    return String(error || 'unknown browser-host failure');
}

export class ArborcoreBrowserHostV2 {
    constructor(instance) {
        if (!instance || !instance.exports || !(instance.exports.memory instanceof WebAssembly.Memory)) {
            throw new TypeError('Arborcore Browser Host v2 requires a WASM instance with exported memory');
        }
        this.ex = instance.exports;
        this.memory = instance.exports.memory;
        this.encoder = new TextEncoder();
        this.decoder = new TextDecoder();
        this.ex.arbor_browser_host_v2_state_init(this.ex.arbor_browser_host_v2_state_scratch_ptr());
        this.ex.arbor_browser_host_v2_prepare_gpu_tables();
    }

    bytes(byteOffset, byteLength, Type = Uint8Array) {
        return new Type(this.memory.buffer, byteOffset, byteLength / Type.BYTES_PER_ELEMENT);
    }

    q32Css(rawValue) {
        const ptr = this.ex.arbor_browser_host_v2_css_scratch_ptr();
        const cap = this.ex.arbor_browser_host_v2_css_scratch_bytes();
        const status = this.ex.arbor_browser_host_v2_format_q32_css(
            rawValue, ptr, cap, this.ex.arbor_browser_host_v2_written_scratch_ptr());
        if (status !== OK) throw new RangeError(`WASM Q32 CSS formatting failed (${status})`);
        const data = this.bytes(ptr, cap);
        let length = 0;
        while (length < data.length && data[length] !== 0) length += 1;
        return this.decoder.decode(data.subarray(0, length));
    }

    setOverlayPosition(element, xRaw, yRaw) {
        if (!element || !element.style) throw new TypeError('overlay target must expose style');
        const left = this.q32Css(xRaw);
        const top = this.q32Css(yRaw);
        element.style.left = left;
        element.style.top = top;
        return { left, top };
    }

    resolveDevicePixelSize(entry, devicePixelRatio = 1) {
        if (!entry) throw new TypeError('ResizeObserver entry is required');
        const device = firstBoxSize(entry.devicePixelContentBoxSize);
        const content = firstBoxSize(entry.contentBoxSize);
        const deviceValid = Boolean(device && Number.isInteger(device.inlineSize) && Number.isInteger(device.blockSize));
        const cssWidth = content ? content.inlineSize : (entry.contentRect ? entry.contentRect.width : 0);
        const cssHeight = content ? content.blockSize : (entry.contentRect ? entry.contentRect.height : 0);
        const out = this.ex.arbor_browser_host_v2_size_scratch_ptr();
        const status = this.ex.arbor_browser_host_v2_resolve_device_size(
            deviceValid ? 1 : 0,
            deviceValid ? device.inlineSize : 0,
            deviceValid ? device.blockSize : 0,
            Number(cssWidth || 0),
            Number(cssHeight || 0),
            Number(devicePixelRatio),
            out);
        if (status !== OK) throw new RangeError(`WASM device-size resolution failed (${status})`);
        return {
            width: this.ex.arbor_browser_host_v2_size_width(),
            height: this.ex.arbor_browser_host_v2_size_height(),
            mode: this.ex.arbor_browser_host_v2_resolved_size_mode()
        };
    }

    syncCanvas(canvas, width, height) {
        const changed = canvas.width !== width || canvas.height !== height;
        if (changed) {
            canvas.width = width;
            canvas.height = height;
        }
        return changed;
    }

    rgba8Bytes(width, height, strideBytes) {
        const status = this.ex.arbor_browser_host_v2_validate_rgba8(
            width, height, strideBytes, this.ex.arbor_browser_host_v2_layout_scratch_ptr());
        if (status !== OK) throw new RangeError(`WASM RGBA8 layout rejected (${status})`);
        return this.ex.arbor_browser_host_v2_layout_byte_length();
    }

    rgba16Bytes(width, height, strideBytes) {
        const status = this.ex.arbor_browser_host_v2_validate_rgba16(
            width, height, strideBytes, this.ex.arbor_browser_host_v2_layout_scratch_ptr());
        if (status !== OK) throw new RangeError(`WASM RGBA16 layout rejected (${status})`);
        return this.ex.arbor_browser_host_v2_layout_byte_length();
    }

    classifyGpuFailure(error) {
        const ptr = this.ex.arbor_browser_host_v2_failure_scratch_ptr();
        const cap = this.ex.arbor_browser_host_v2_failure_scratch_bytes();
        const view = this.bytes(ptr, cap);
        const encoded = this.encoder.encode(errorText(error));
        const length = encoded.length < cap ? encoded.length : cap;
        view.set(encoded.subarray(0, length), 0);
        return this.ex.arbor_browser_host_v2_classify_gpu_failure(ptr, length);
    }

    stateRecord() {
        return {
            presentMode: this.ex.arbor_browser_host_v2_state_present_mode(),
            failureClass: this.ex.arbor_browser_host_v2_state_failure_class(),
            generation: this.ex.arbor_browser_host_v2_state_generation()
        };
    }

    markWebGpuReady() {
        this.ex.arbor_browser_host_v2_state_webgpu_ready(this.ex.arbor_browser_host_v2_state_scratch_ptr());
    }

    markGpuFailure(failureClass) {
        this.ex.arbor_browser_host_v2_state_failure(this.ex.arbor_browser_host_v2_state_scratch_ptr(), failureClass);
    }

    markDestroyed() {
        this.ex.arbor_browser_host_v2_state_destroy(this.ex.arbor_browser_host_v2_state_scratch_ptr());
    }

    async shaderSource(url) {
        const response = await fetch(url, { cache: 'no-store' });
        if (!response.ok) throw new Error(`shader fetch failed (${response.status})`);
        return response.text();
    }

    gpuTableViews() {
        return {
            bucket12: this.bytes(
                this.ex.arbor_browser_host_v2_bucket12_ptr(),
                this.ex.arbor_browser_host_v2_bucket12_bytes()),
            forward: this.bytes(
                this.ex.arbor_browser_host_v2_forward_lut_ptr(),
                this.ex.arbor_browser_host_v2_forward_lut_bytes())
        };
    }
}

export class CanvasPresenterV2 {
    constructor(canvas, host) {
        this.canvas = canvas;
        this.host = host;
        this.context = canvas.getContext('2d', { alpha: true, colorSpace: 'srgb' });
        if (!this.context) throw new Error('Canvas 2D context unavailable');
    }

    presentRgba8(byteOffset, width, height, strideBytes) {
        const byteLength = this.host.rgba8Bytes(width, height, strideBytes);
        this.host.syncCanvas(this.canvas, width, height);
        const view = this.host.bytes(byteOffset, byteLength, Uint8ClampedArray);
        let imageData;
        try {
            imageData = new ImageData(view, width, height, { colorSpace: 'srgb', pixelFormat: 'rgba-unorm8' });
        } catch (_) {
            imageData = new ImageData(view, width, height, { colorSpace: 'srgb' });
        }
        this.context.putImageData(imageData, 0, 0);
        return imageData;
    }
}

export class WebGpuPresenterV2 {
    constructor(canvas, host, shaderUrl = '/browser/shaders/rgba8_present.wgsl') {
        this.canvas = canvas;
        this.host = host;
        this.shaderUrl = shaderUrl;
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
        this.cachedBuffer = null;
        this.cachedOffset = -1;
        this.cachedLength = -1;
        this.cachedView = null;
        this.stats = { sourceViewCreations: 0, sourceViewReuses: 0, presentations: 0 };
    }

    async initialize() {
        if (!navigator.gpu) return false;
        try {
            this.adapter = await navigator.gpu.requestAdapter({ powerPreference: 'high-performance' });
            if (!this.adapter) throw new Error('requestAdapter returned null');
            this.device = await this.adapter.requestDevice();
            this.context = this.canvas.getContext('webgpu');
            if (!this.context) throw new Error('GPUCanvasContext unavailable');
            this.canvasFormat = navigator.gpu.getPreferredCanvasFormat();
            this.context.configure({
                device: this.device,
                format: this.canvasFormat,
                colorSpace: 'srgb',
                alphaMode: 'premultiplied'
            });
            const shader = this.device.createShaderModule({
                label: 'Arborcore Browser Host v2 RGBA8 presentation',
                code: await this.host.shaderSource(this.shaderUrl)
            });
            this.pipeline = await this.device.createRenderPipelineAsync({
                label: 'Arborcore Browser Host v2 presentation',
                layout: 'auto',
                vertex: { module: shader, entryPoint: 'vertex_main' },
                fragment: { module: shader, entryPoint: 'fragment_main', targets: [{ format: this.canvasFormat }] },
                primitive: { topology: 'triangle-list' }
            });
            this.device.lost.then(info => {
                const failure = this.host.classifyGpuFailure(new Error(`device lost:${info.reason}:${info.message}`));
                this.host.markGpuFailure(failure);
            });
            this.host.markWebGpuReady();
            return true;
        } catch (error) {
            const failure = this.host.classifyGpuFailure(error);
            this.host.markGpuFailure(failure);
            if (failure === GPU_FAILURE_PROGRAMMING) throw error;
            return false;
        }
    }

    ensureSourceTexture(width, height) {
        if (this.sourceTexture && this.sourceWidth === width && this.sourceHeight === height) return;
        if (this.sourceTexture) this.sourceTexture.destroy();
        this.sourceTexture = this.device.createTexture({
            label: 'Arborcore Browser Host v2 frozen-B1 RGBA8 transport',
            size: { width, height, depthOrArrayLayers: 1 },
            format: 'rgba8unorm',
            usage: GPUTextureUsage.COPY_DST | GPUTextureUsage.COPY_SRC | GPUTextureUsage.TEXTURE_BINDING
        });
        this.sourceView = this.sourceTexture.createView();
        this.bindGroup = this.device.createBindGroup({
            layout: this.pipeline.getBindGroupLayout(0),
            entries: [{ binding: 0, resource: this.sourceView }]
        });
        this.sourceWidth = width;
        this.sourceHeight = height;
    }

    sourceBytes(byteOffset, byteLength) {
        const buffer = this.host.memory.buffer;
        if (this.cachedView && this.cachedBuffer === buffer &&
            this.cachedOffset === byteOffset && this.cachedLength === byteLength) {
            this.stats.sourceViewReuses += 1;
            return this.cachedView;
        }
        this.cachedBuffer = buffer;
        this.cachedOffset = byteOffset;
        this.cachedLength = byteLength;
        this.cachedView = new Uint8Array(buffer, byteOffset, byteLength);
        this.stats.sourceViewCreations += 1;
        return this.cachedView;
    }

    presentRgba8(byteOffset, width, height, strideBytes) {
        const byteLength = this.host.rgba8Bytes(width, height, strideBytes);
        this.host.syncCanvas(this.canvas, width, height);
        this.ensureSourceTexture(width, height);
        this.device.queue.writeTexture(
            { texture: this.sourceTexture },
            this.sourceBytes(byteOffset, byteLength),
            { offset: 0, bytesPerRow: strideBytes, rowsPerImage: height },
            { width, height, depthOrArrayLayers: 1 });
        const encoder = this.device.createCommandEncoder({ label: 'Arborcore Browser Host v2 present' });
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
        this.stats.presentations += 1;
    }

    destroy() {
        if (this.sourceTexture) this.sourceTexture.destroy();
        if (this.context) this.context.unconfigure();
        if (this.device) this.device.destroy();
        this.host.markDestroyed();
    }
}
