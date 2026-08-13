# Arborcore Precision Renderer Plan

The Precision Renderer is a peer of standard HTML/CSS, not a replacement.
Arborcore supports HTML/CSS-only, Precision-Surface-only and Hybrid pages.

## R0-R3 — foundational raster qualification — QUALIFIED

Qualified RGBA16 UNORM premultiplied as the internal construction format and
Q0.32 as the coverage representation. Established row-major explicit-stride
surfaces, half-open pixel cells, conservative bounds, analytical rectangle
coverage and zero-import native/WASM semantic equivalence. The R0-R3 result was
a review recommendation rather than a frozen renderer contract.

## R4-R9 — software reference renderer — CONSTRUCTION / FREEZE CANDIDATE

R4 implements checked caller-owned RGBA16 surfaces and basic primitives. R5
defines linear-light sRGB-primary premultiplied source-over semantics and frozen
sRGB8 import/export mapping. R6 defines deterministic Q0.32 rectangle/hairline
antialiasing. R7 adds bounded deterministic line/quad/cubic path strokes. R8
requires byte-identical native/WASM golden buffers. R9 qualifies performance,
reproducibility, lower-layer regressions and emits the Reference Raster Contract
v1 freeze candidate.

The C function surface remains unfrozen construction API even when the raster
contract is frozen.

## Post-R9

- browser Precision Surface adapter: WASM linear memory -> Canvas/ImageData or
  other browser presentation surface;
- explicit RGBA16 -> RGBA8 output conversion where required;
- Hybrid DOM/Precision-Surface coordinate synchronization;
- WebGPU acceleration verified against the reference renderer;
- broader fill/stroke/path/text capabilities may extend the renderer through
  explicit compatibility/version decisions.
