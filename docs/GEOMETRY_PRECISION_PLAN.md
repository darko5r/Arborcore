# Arborcore Geometry Precision Plan

This plan begins above the frozen Assembly ABI and the qualified C runtime
bridge. It does not replace HTML/CSS.

Arborcore permanently supports three peer rendering modes:

1. **HTML/CSS mode** — standard DOM, CSS, accessibility, selection, forms and SEO.
2. **Precision Surface mode** — deterministic fixed-point geometry and explicit
   raster/device-pixel control.
3. **Hybrid mode** — HTML/CSS and one or more Precision Surfaces in the same
   page, including layered composition.

## G0 — numerical requirements and candidate contracts — COMPLETE

Defined coordinate precision requirements and compared Q16.16, Q26.6, Q32.32
and Q24.40 without exposing an API.

## G1 — fixed-point qualification — COMPLETE

Q16.16 failed required integer range and Q26.6 failed required fractional
precision. Q32.32 and Q24.40 qualified. Same-host evidence recommended Q32.32:
it provided 256x more integer range and remained inside the fixed performance
review envelope. The recommendation was retained as an unfrozen experiment.

## G2 — checked geometry — CONSTRUCTION

Production Q32.32 scalar arithmetic, explicit rounding, points, sizes,
rectangles, lines, bounds, intersection, union and translation. Operations are
checked and transactional.

## G3 — transforms and clipping — CONSTRUCTION

Production affine composition/inversion, point and rectangle transforms,
clipping and deterministic fixed-point CORDIC rotation coefficient generation.
No host floating-point trigonometry is normative.

## G4 — logical/device mapping and numerical contract — CONSTRUCTION

Exact rational DPR/zoom composition, logical-to-device Q32.32 mapping, distinct
logical/device coordinate types, native/WASM equivalence qualification and the
Geometry Numerical Contract v1 freeze candidate.

## Precision renderer — NEXT

The reference renderer will be a deterministic CPU implementation capable of
producing explicit RGBA buffers with defined clipping, coverage, blending and
antialiasing. Golden raster buffers can therefore be qualified by byte identity
and SHA-256.

A later WebGPU backend may accelerate the same geometry contract. The software
renderer remains the correctness oracle; GPU equivalence claims must be bounded
to what tests actually establish.

## Browser delivery

Browser-side precision execution will use C/WebAssembly plus Canvas/ImageData
and/or WebGPU. Native x86-64 Assembly remains the Linux server/native core and
is not executed directly in the browser sandbox. Authoritative Precision
Surface fixed-point values stay in WebAssembly integer/linear-memory form rather
than being round-tripped through JavaScript `Number`.
