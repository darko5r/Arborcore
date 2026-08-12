# Arborcore Geometry Precision Plan

This plan begins above the frozen Assembly ABI and the qualified C runtime
bridge. It does not replace HTML/CSS.

Arborcore permanently supports three peer rendering modes:

1. **HTML/CSS mode** — standard DOM, CSS, accessibility, selection, forms and SEO.
2. **Precision Surface mode** — deterministic fixed-point geometry and explicit
   raster/device-pixel control.
3. **Hybrid mode** — HTML/CSS and one or more Precision Surfaces in the same
   page, including layered composition.

## G0 — numerical requirements and candidate contracts

Define coordinate spaces, working range/precision requirements, checked
arithmetic, rounding vocabulary, overflow behavior and the browser/WASM
representation boundary. Compare Q16.16, Q26.6, Q32.32 and Q24.40 without
turning any candidate into public API.

## G1 — fixed-point qualification and selection recommendation

Property-test the candidates and compare checked add/multiply/divide plus a
simple affine workload on the qualified host. Produce an evidence-backed
selection recommendation. The result remains `UNFROZEN_EXPERIMENT` until
reviewed.

## G2 — checked geometry

After G1 review, define production points, sizes, rectangles, lines, bounds,
intersection, union, translation and explicit rounding operations using the
selected representation. Arithmetic remains checked and transactional.

## G3 — transforms and clipping

Define affine transforms, inverse transforms, scale/rotation, clipping and
well-specified overflow behavior. Rotation/trigonometric approximation strategy
must itself be qualified rather than silently making host floating point
normative.

## G4 — logical/device pixel mapping and numerical-contract freeze

Make world -> layout -> surface -> CSS pixel -> device pixel conversion
explicit. `devicePixelRatio`, viewport transforms and browser zoom are inputs to
the mapping rather than hidden assumptions. Freeze the selected fixed-point
representation, rounding/overflow semantics and conversion rules before
implementing the rasterizer.

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
is not executed directly in the browser sandbox. Authoritative Precision
Surface fixed-point values stay in WebAssembly integer/linear-memory form rather
than being round-tripped through JavaScript `Number`.
