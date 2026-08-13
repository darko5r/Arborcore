# Precision Renderer R4-R9 — Reference Software Rasterizer

R4-R9 promotes the qualified R0-R3 recommendations into a portable production
reference renderer above frozen Geometry Numerical Contract v1.

## Contract boundary

The reference renderer is independent of Linux and the Assembly ABI. It depends
only on the portable Q32.32 Geometry API. The C renderer function surface remains
construction-stage and unfrozen; R9 freezes raster semantics and golden bytes,
not C symbol names.

## R4 — surfaces and primitives

- caller-owned row-major surfaces
- canonical little-endian R16/G16/B16/A16 channel storage
- explicit stride and checked buffer geometry
- analytical Q0.32 rectangle coverage
- filled rectangles and one-device-pixel hairlines

## R5 — color and compositing

The authoritative working representation is linear-light sRGB-primary RGBA16
UNORM with premultiplied alpha. sRGB8 import/export uses a frozen 256-entry
encoded-to-linear table; runtime rendering performs no floating-point transfer
function evaluation. Coverage and Porter-Duff source-over reductions use
nearest-even integer rounding.

RGBA8 remains a browser/display export representation, not the reference working
buffer.

## R6 — deterministic antialiasing

Axis-aligned rectangles retain analytical area coverage with no supersampling.
Hairlines use a deterministic fixed-point Wu-style midpoint rule and Q0.32
coverage. The algorithm is part of the reference contract; future accelerated
backends must reproduce its observable output where reference equivalence is
required.

## R7 — paths and curves

The first reference path surface is a one-device-pixel hairline path with move,
line, quadratic, cubic and close commands. Curves use deterministic de Casteljau
half-subdivision, second-difference L-infinity flatness, 1/256-device-pixel
construction tolerance and maximum recursion depth 12. Path command count is
bounded at 4096.

## R8 — golden native/WASM identity

A fixed 16x16 reference scene exercises:

- transparent clear;
- overlapping fractional RGBA16 rectangles;
- source-over compositing;
- an antialiased line;
- quadratic and cubic path segments.

Native and zero-import WASM execute the same renderer source and must emit an
identical 2048-byte RGBA16 buffer. The frozen golden SHA-256 is retained in
`renderer/arborcore-renderer-1.contract`.

## R9 — qualification

The final gate requires:

- lower-layer identity preservation;
- warning-clean native renderer tests;
- ASan/UBSan;
- contract/source/dependency verification;
- native/WASM golden-byte equality;
- deterministic `libarborcore_renderer.a` reconstruction;
- same-host clear and fractional-rectangle performance qualification;
- line/path diagnostic performance baselines;
- frozen Geometry regression;
- qualified C Runtime Bridge regression;
- full frozen Assembly regression.

The initial hot-path policy is fixed before target-host evidence: production
clear and fractional rectangle rendering must each be no more than 50% slower
than an equivalent same-host reference OR remain within their fixed absolute
budgets (500 ns clear, 4000 ns rectangle). Line/path metrics establish the first
retained diagnostic baseline and do not yet form causal rejection thresholds.

## Browser architecture

HTML/CSS remains a fully independent first-class path. Precision Surface uses the
reference renderer semantics. Hybrid pages may combine both. The WASM reference
renderer remains authoritative for Precision Surface; JavaScript `Number` is not
an authoritative geometry or coverage representation.

A future WebGPU backend is an accelerator and must be validated against this
software reference rather than defining different semantics.


## Native hardening and clear hot-path qualification

Native compiler stack-protector references such as `__stack_chk_fail` are classified separately from the renderer's semantic lower-layer dependency surface. The only semantic dependencies remain the five frozen Geometry functions; unexpected external dependencies remain a hard gate failure. This preserves toolchain hardening rather than disabling it to satisfy dependency accounting.

`arbor_renderer_clear` validates the caller-owned surface and premultiplied color once, packs the canonical little-endian RGBA16 pixel into 64 bits, and writes rows in an alignment-safe unrolled loop. Row padding is never modified. The R9 clear/rectangle comparison uses runtime, nonconstant width/height/stride shared by the reference and production paths so the reference implementation cannot gain compile-time surface-specialization that the public production function cannot receive across the archive boundary.

### R9 rectangle hot-path qualification note

The production rectangle loop uses private, compiler-inlineable coverage and compositing helpers. Public renderer entry points remain available as the construction C surface, but the per-pixel loop must not call interposable public `arbor_rgba16_apply_coverage`, `arbor_rgba16_source_over`, or `arbor_renderer_rect_coverage` symbols. This avoids ELF/PIC interposition overhead without changing byte-level raster semantics. The R9 benchmark supplies runtime/nonconstant shared surface, rectangle, and color inputs to both reference and production paths.
