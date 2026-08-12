# Arborcore Geometry Precision Plan

This plan begins above the frozen Assembly ABI and the qualified C runtime
bridge. It does not replace HTML/CSS.

Arborcore permanently supports three peer rendering modes:

1. **HTML/CSS mode** — standard DOM, CSS, accessibility, selection, forms and SEO.
2. **Precision Surface mode** — deterministic fixed-point geometry and explicit
   raster/device-pixel control.
3. **Hybrid mode** — HTML/CSS and one or more Precision Surfaces in the same
   page, including layered composition.

## G0 — numerical contract

Evaluate fixed-point candidates such as 16.16 and 32.32 against range,
overflow, multiplication/division cost, transform precision, WebAssembly
behavior and actual rendering needs. Do not select a format solely because it
has more fractional bits.

## G1 — checked geometry

Define points, sizes, rectangles, lines, bounds, intersection, union,
translation and explicit rounding modes with checked arithmetic.

## G2 — transforms and clipping

Define affine transforms, inverse transforms, scale/rotation, clipping and
well-specified overflow behavior.

## G3 — logical/device pixel mapping

Make world -> layout -> surface -> CSS pixel -> device pixel conversion
explicit. `devicePixelRatio`, viewport transforms and browser zoom are inputs to
the mapping rather than hidden assumptions.

## G4 — freeze Geometry Numerical Contract

Freeze the chosen fixed-point representation and rounding/overflow semantics
before implementing the rasterizer.

## Precision renderer

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
is not executed directly in the browser sandbox.
