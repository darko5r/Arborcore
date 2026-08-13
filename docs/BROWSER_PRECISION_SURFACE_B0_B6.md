# Browser Precision Surface B0-B6 — Frozen Delivery Contract v1

B0-B6 delivers the frozen Reference Raster Contract v1 into a real browser
without making browser pixels authoritative. The canonical renderer remains the
RGBA16 UNORM little-endian, linear-light, premultiplied software reference
raster. Browser presentation is an explicit conversion/presentation boundary.

## B0 — WASM memory boundary

The reference WASM module has zero imports and exports its memory. JavaScript
must reacquire typed-array views whenever `memory.buffer` identity changes.
Memory growth is forbidden during a synchronous presentation operation. No
cached JavaScript typed array is authoritative.

## B1 — deterministic RGBA8 export

`arbor_browser_export_rgba8` converts a structurally valid frozen RGBA16 raster
to tight or explicitly-strided non-premultiplied sRGB8 bytes by reusing the
frozen `arbor_rgba16_to_srgb8` mapping. The 16x16 reference scene has canonical
RGBA8 export SHA-256:

`a24be39e38b4fb654e79a2161177cf3670c354695f0b2fa13be677e8d1624dfd`.

The frozen authoritative RGBA16 SHA-256 remains:

`fda03aa982372e8bb181ecf4e65910478bd8ad66ae08cf12cc1a5f89288673ba`.

## B2 — Canvas/ImageData reference presentation

The baseline uses `ImageData` with sRGB / `rgba-unorm8` and
`CanvasRenderingContext2D.putImageData()`. The Canvas bitmap is a presentation
sink only. Readback is not a raster oracle because browser color-space and
premultiplication conversions can be lossy for translucent pixels. B2 therefore
uses a separate fully opaque 2x2 probe for byte-exact Canvas transport checking.

`rgba-float16` ImageData support is recorded as a capability diagnostic only; it
does not replace the canonical RGBA16 UNORM raster.

## B3 — resize and device pixels

`ResizeObserver` `device-pixel-content-box` is preferred because it reports an
integral device-pixel content size before transforms. If unavailable, B3 uses
the content box multiplied by `devicePixelRatio` and rounds to the nearest
positive integer. This fallback affects presentation sizing only; it does not
replace Q32.32 geometry as raster authority.

## B4 — Hybrid DOM/Precision coordinates

Authoritative Q32.32 coordinates cross into CSS through JavaScript `BigInt`.
The reference serializer emits nine decimal CSS-pixel places with nearest-even
rounding. DOM/CSS remains a parallel independent rendering path; Hybrid mode
synchronizes placement without feeding browser floating point back into the
reference raster contract.

## B5 — real-browser qualification

The browser gate launches both installed Firefox and Google Chrome headlessly
against a localhost test server. Each browser must prove:

- zero-import WASM instantiation;
- frozen RGBA16 golden hash;
- deterministic RGBA8 export hash;
- memory-buffer replacement and fresh-view recovery after `memory.grow`;
- opaque Canvas/ImageData byte transport;
- DPR selection helpers;
- deterministic Hybrid Q32.32 CSS serialization.

## B6 — performance and lower-layer regression

The first browser-export performance gate requires a mixed-alpha 640x360
RGBA16-to-RGBA8 export to fit inside one 60 Hz frame budget (16.666667 ms) on
the qualification host. Opaque 1280x720 conversion and Canvas `putImageData`
are retained as diagnostic baselines for future causal optimization.

B6 then reruns frozen Renderer, Geometry, C Runtime Bridge and Assembly checks.
The browser C and JavaScript function surfaces remain `UNFROZEN_CONSTRUCTION`;
the successful B0-B6 admission gate freezes the browser **delivery semantics** as
Browser Precision Surface Contract v1 without freezing those evolving APIs.

## Post-B6

Browser Precision Surface Contract v1 is the frozen Canvas/ImageData reference
delivery boundary. WebGPU may now be introduced only as an accelerator. GPU
output must be checked against the frozen software reference and frozen browser
export/presentation semantics rather than becoming an independent rendering
truth. Standard HTML/CSS remains an independent first-class rendering path.

### B1 export hot-path qualification

The qualification-host B6 review exposed an avoidable per-pixel ELF/PIC call and
binary-search cost in the first export implementation. The repaired browser
bridge does not modify the frozen renderer. Instead it owns an exact browser
export accelerator derived from the frozen `srgb8_linear16_lut.h` transfer
mapping.

A 4096-byte bucket index stores the exact nearest-even sRGB8 result at the
start of each 16-value linear16 bucket. Runtime performs at most one exact
midpoint correction against the frozen forward table. Opaque pixels bypass
unpremultiplication entirely. Partially transparent pixels retain the same
integer nearest-even unpremultiplication rule.

This is a representation/performance optimization only. Native B1 tests verify
all 65,536 opaque linear16 values and one deterministic valid premultiplied
sample for every possible 16-bit alpha against the frozen
`arbor_rgba16_to_srgb8` reference function. The canonical RGBA8 golden hash is
unchanged. The production browser bridge therefore has no runtime renderer
symbol dependency for export while remaining semantically checked against the
frozen Reference Raster Contract v1.
