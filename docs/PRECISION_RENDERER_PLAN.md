# Arborcore Precision Renderer Plan

The Precision Renderer is a peer of standard HTML/CSS, not a replacement.
Arborcore supports HTML/CSS-only, Precision-Surface-only and Hybrid pages.

## R0-R3 — foundational raster qualification — QUALIFIED

Qualified RGBA16 UNORM premultiplied as the internal construction format and
Q0.32 as the coverage representation. Established row-major explicit-stride
surfaces, half-open pixel cells, conservative bounds, analytical rectangle
coverage and zero-import native/WASM semantic equivalence. The R0-R3 result was
a review recommendation rather than a frozen renderer contract.

## R4-R9 — software reference renderer — FROZEN REFERENCE CONTRACT v1

R4-R9 established the frozen byte-level Reference Raster Contract v1: checked
caller-owned RGBA16 surfaces, linear-light sRGB-primary premultiplied source-over,
Q0.32 analytical coverage, deterministic hairline/path rasterization, and
byte-identical native/WASM golden output. The current C function surface remains
construction-stage even though raster semantics are frozen.

The C function surface remains unfrozen construction API even when the raster
contract is frozen.

## Browser Precision Surface B0-B6 — FROZEN DELIVERY CONTRACT v1

B0-B6 froze the browser delivery boundary above Reference Raster Contract v1:
zero-import WASM memory/view lifetime rules, deterministic RGBA16-to-sRGB8
export, Canvas/ImageData presentation, DPR/resize synchronization, Hybrid DOM
coordinate serialization, real Firefox/Chrome execution and browser delivery
performance. The browser C and JavaScript function surfaces remain
`UNFROZEN_CONSTRUCTION`; delivery semantics are frozen independently of those
evolving APIs. See `docs/BROWSER_PRECISION_SURFACE_B0_B6.md`.

## Post-B6

WebGPU may be developed as an accelerator only. It must reproduce the frozen
Reference Raster and Browser Precision Surface v1 delivery semantics. HTML/CSS
remains a parallel independent first-class rendering path.
