/* Test-only OPT3 driver. Not part of the production Browser Host Boundary v2. */
export class DirectRgba16ExperimentV2 {
    constructor(canvas, host, shaderUrl = '/browser/shaders/rgba16_exact_convert.wgsl') {
        this.canvas = canvas;
        this.host = host;
        this.shaderUrl = shaderUrl;
        this.device = null;
        this.context = null;
        this.canvasFormat = '';
        this.sourceTexture = null;
        this.sourceWidth = 0;
        this.sourceHeight = 0;
        this.bucketBuffer = null;
        this.forwardBuffer = null;
        this.renderPipeline = null;
        this.computePipeline = null;
        this.renderBindGroup = null;
        this.computeBindGroup = null;
        this.outputBuffer = null;
    }

    async initializeFromPresenter(presenter) {
        this.device = presenter.device;
        this.canvasFormat = presenter.canvasFormat;
        this.context = this.canvas.getContext('webgpu');
        if (!this.context) throw new Error('GPUCanvasContext unavailable');
        this.context.configure({ device: this.device, format: this.canvasFormat, colorSpace: 'srgb', alphaMode: 'premultiplied' });
        const tables = this.host.gpuTableViews();
        this.bucketBuffer = this.device.createBuffer({ size: tables.bucket12.byteLength, usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_DST });
        this.forwardBuffer = this.device.createBuffer({ size: tables.forward.byteLength, usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_DST });
        this.device.queue.writeBuffer(this.bucketBuffer, 0, tables.bucket12);
        this.device.queue.writeBuffer(this.forwardBuffer, 0, tables.forward);
        const shader = this.device.createShaderModule({ code: await this.host.shaderSource(this.shaderUrl) });
        this.renderPipeline = await this.device.createRenderPipelineAsync({
            layout: 'auto',
            vertex: { module: shader, entryPoint: 'vertex_main' },
            fragment: { module: shader, entryPoint: 'fragment_main', targets: [{ format: this.canvasFormat }] },
            primitive: { topology: 'triangle-list' }
        });
        this.computePipeline = await this.device.createComputePipelineAsync({
            layout: 'auto', compute: { module: shader, entryPoint: 'compute_main' }
        });
        return true;
    }

    ensureResources(width, height) {
        if (this.sourceTexture && this.sourceWidth === width && this.sourceHeight === height) return;
        if (this.sourceTexture) this.sourceTexture.destroy();
        if (this.outputBuffer) this.outputBuffer.destroy();
        this.sourceTexture = this.device.createTexture({
            size: { width, height, depthOrArrayLayers: 1 },
            format: 'rgba16uint',
            usage: GPUTextureUsage.COPY_DST | GPUTextureUsage.TEXTURE_BINDING
        });
        const sourceView = this.sourceTexture.createView();
        this.outputBuffer = this.device.createBuffer({
            size: this.host.ex.arbor_browser_host_v2_rgba8_output_bytes(),
            usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_SRC
        });
        this.renderBindGroup = this.device.createBindGroup({
            layout: this.renderPipeline.getBindGroupLayout(0),
            entries: [
                { binding: 0, resource: sourceView },
                { binding: 1, resource: { buffer: this.bucketBuffer } },
                { binding: 2, resource: { buffer: this.forwardBuffer } }
            ]
        });
        this.computeBindGroup = this.device.createBindGroup({
            layout: this.computePipeline.getBindGroupLayout(0),
            entries: [
                { binding: 0, resource: sourceView },
                { binding: 1, resource: { buffer: this.bucketBuffer } },
                { binding: 2, resource: { buffer: this.forwardBuffer } },
                { binding: 3, resource: { buffer: this.outputBuffer } }
            ]
        });
        this.sourceWidth = width;
        this.sourceHeight = height;
    }

    presentRgba16(byteOffset, width, height, strideBytes) {
        const byteLength = this.host.rgba16Bytes(width, height, strideBytes);
        this.host.syncCanvas(this.canvas, width, height);
        this.ensureResources(width, height);
        this.device.queue.writeTexture(
            { texture: this.sourceTexture },
            this.host.bytes(byteOffset, byteLength),
            { offset: 0, bytesPerRow: strideBytes, rowsPerImage: height },
            { width, height, depthOrArrayLayers: 1 });
        const encoder = this.device.createCommandEncoder();
        const compute = encoder.beginComputePass();
        compute.setPipeline(this.computePipeline);
        compute.setBindGroup(0, this.computeBindGroup);
        compute.dispatchWorkgroups(
            this.host.ex.arbor_browser_host_v2_dispatch_x(),
            this.host.ex.arbor_browser_host_v2_dispatch_y(),
            1);
        compute.end();
        const render = encoder.beginRenderPass({
            colorAttachments: [{ view: this.context.getCurrentTexture().createView(), loadOp: 'clear', storeOp: 'store', clearValue: { r: 0, g: 0, b: 0, a: 0 } }]
        });
        render.setPipeline(this.renderPipeline);
        render.setBindGroup(0, this.renderBindGroup);
        render.draw(3);
        render.end();
        this.device.queue.submit([encoder.finish()]);
    }

    async readbackRgba8() {
        if (!this.outputBuffer) throw new Error('RGBA16 experiment has no output buffer');
        const size = this.host.ex.arbor_browser_host_v2_rgba8_output_bytes();
        const readback = this.device.createBuffer({
            size, usage: GPUBufferUsage.COPY_DST | GPUBufferUsage.MAP_READ
        });
        try {
            const encoder = this.device.createCommandEncoder();
            encoder.copyBufferToBuffer(this.outputBuffer, 0, readback, 0, size);
            this.device.queue.submit([encoder.finish()]);
            await readback.mapAsync(GPUMapMode.READ);
            const result = new Uint8Array(size);
            result.set(new Uint8Array(readback.getMappedRange()));
            readback.unmap();
            return result;
        } finally {
            readback.destroy();
        }
    }

    destroy() {
        if (this.sourceTexture) this.sourceTexture.destroy();
        if (this.outputBuffer) this.outputBuffer.destroy();
        if (this.bucketBuffer) this.bucketBuffer.destroy();
        if (this.forwardBuffer) this.forwardBuffer.destroy();
        if (this.context) this.context.unconfigure();
    }
}
