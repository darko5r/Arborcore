const Q32_DENOMINATOR = 1n << 32n;
const CSS_DECIMAL_SCALE = 1000000000n;

function roundNearestEvenUnsigned(numerator, denominator) {
    const q = numerator / denominator;
    const r = numerator % denominator;
    const twice = r * 2n;
    if (twice > denominator || (twice === denominator && (q & 1n) !== 0n)) {
        return q + 1n;
    }
    return q;
}

export function q32ToCssPxString(rawValue) {
    if (typeof rawValue !== 'bigint') {
        throw new TypeError('Q32.32 CSS conversion requires BigInt input');
    }
    const negative = rawValue < 0n;
    const magnitude = negative ? -rawValue : rawValue;
    let integerPart = magnitude >> 32n;
    const fractionRaw = magnitude & 0xffffffffn;
    let fraction = roundNearestEvenUnsigned(
        fractionRaw * CSS_DECIMAL_SCALE,
        Q32_DENOMINATOR);
    if (fraction === CSS_DECIMAL_SCALE) {
        integerPart += 1n;
        fraction = 0n;
    }
    const sign = negative && (integerPart !== 0n || fraction !== 0n) ? '-' : '';
    return `${sign}${integerPart}.${fraction.toString().padStart(9, '0')}px`;
}

export function setHybridOverlayPosition(element, xRaw, yRaw) {
    if (!element || !element.style) {
        throw new TypeError('Hybrid overlay target must expose style');
    }
    const left = q32ToCssPxString(xRaw);
    const top = q32ToCssPxString(yRaw);
    element.style.left = left;
    element.style.top = top;
    return { left, top };
}

function firstBoxSize(value) {
    if (!value) return null;
    if (Array.isArray(value)) return value.length ? value[0] : null;
    if (typeof value.length === 'number') return value.length ? value[0] : null;
    return value;
}

function positiveFinite(value) {
    return Number.isFinite(value) && value > 0;
}

export function resolveDevicePixelSize(entry, devicePixelRatio = 1) {
    if (!entry) {
        throw new TypeError('ResizeObserver entry is required');
    }
    const device = firstBoxSize(entry.devicePixelContentBoxSize);
    if (device && Number.isInteger(device.inlineSize) && Number.isInteger(device.blockSize) &&
        device.inlineSize > 0 && device.blockSize > 0) {
        return {
            width: device.inlineSize,
            height: device.blockSize,
            mode: 'device-pixel-content-box'
        };
    }

    const content = firstBoxSize(entry.contentBoxSize);
    let cssWidth;
    let cssHeight;
    if (content && positiveFinite(content.inlineSize) && positiveFinite(content.blockSize)) {
        cssWidth = content.inlineSize;
        cssHeight = content.blockSize;
    } else if (entry.contentRect && positiveFinite(entry.contentRect.width) && positiveFinite(entry.contentRect.height)) {
        cssWidth = entry.contentRect.width;
        cssHeight = entry.contentRect.height;
    } else {
        throw new RangeError('ResizeObserver entry has no positive content size');
    }
    if (!positiveFinite(devicePixelRatio)) {
        throw new RangeError('devicePixelRatio must be positive and finite');
    }
    return {
        width: Math.max(1, Math.floor((cssWidth * devicePixelRatio) + 0.5)),
        height: Math.max(1, Math.floor((cssHeight * devicePixelRatio) + 0.5)),
        mode: 'content-box-times-dpr-fallback'
    };
}

export function syncCanvasDeviceSize(canvas, width, height) {
    if (!canvas || !Number.isInteger(width) || !Number.isInteger(height) || width <= 0 || height <= 0) {
        throw new TypeError('Canvas and positive integral device dimensions are required');
    }
    const changed = canvas.width !== width || canvas.height !== height;
    if (changed) {
        canvas.width = width;
        canvas.height = height;
    }
    return changed;
}

export function wasmMemoryView(memory, byteOffset, byteLength, Type = Uint8Array) {
    if (!(memory instanceof WebAssembly.Memory)) {
        throw new TypeError('WebAssembly.Memory is required');
    }
    if (!Number.isSafeInteger(byteOffset) || !Number.isSafeInteger(byteLength) ||
        byteOffset < 0 || byteLength < 0 || byteOffset + byteLength > memory.buffer.byteLength) {
        throw new RangeError('WASM memory view is out of bounds');
    }
    const bytesPerElement = Type.BYTES_PER_ELEMENT || 1;
    if ((byteOffset % bytesPerElement) !== 0 || (byteLength % bytesPerElement) !== 0) {
        throw new RangeError('WASM memory view alignment/length mismatch');
    }
    return new Type(memory.buffer, byteOffset, byteLength / bytesPerElement);
}

export class PrecisionSurfaceMemoryEpoch {
    constructor(memory) {
        if (!(memory instanceof WebAssembly.Memory)) {
            throw new TypeError('WebAssembly.Memory is required');
        }
        this.memory = memory;
        this.buffer = memory.buffer;
        this.generation = 0;
    }

    refresh() {
        const next = this.memory.buffer;
        if (next !== this.buffer) {
            this.buffer = next;
            this.generation += 1;
            return true;
        }
        return false;
    }

    view(byteOffset, byteLength, Type = Uint8Array) {
        this.refresh();
        return wasmMemoryView(this.memory, byteOffset, byteLength, Type);
    }
}

export class PrecisionSurfacePresenter {
    constructor(canvas, memory) {
        if (!canvas || !(memory instanceof WebAssembly.Memory)) {
            throw new TypeError('Canvas and WebAssembly.Memory are required');
        }
        const context = canvas.getContext('2d', { alpha: true, colorSpace: 'srgb' });
        if (!context) {
            throw new Error('Canvas 2D context unavailable');
        }
        this.canvas = canvas;
        this.memory = memory;
        this.context = context;
        this.memoryEpoch = new PrecisionSurfaceMemoryEpoch(memory);
    }

    presentRgba8(byteOffset, width, height, strideBytes) {
        if (!Number.isInteger(width) || !Number.isInteger(height) ||
            !Number.isInteger(strideBytes) || width <= 0 || height <= 0 ||
            strideBytes !== width * 4) {
            throw new RangeError('Reference ImageData path requires positive tight RGBA8 rows');
        }
        syncCanvasDeviceSize(this.canvas, width, height);
        const bytes = width * height * 4;
        const view = this.memoryEpoch.view(byteOffset, bytes, Uint8ClampedArray);
        let imageData;
        try {
            imageData = new ImageData(view, width, height, {
                colorSpace: 'srgb',
                pixelFormat: 'rgba-unorm8'
            });
        } catch (error) {
            imageData = new ImageData(view, width, height, { colorSpace: 'srgb' });
        }
        this.context.putImageData(imageData, 0, 0);
        return imageData;
    }
}
