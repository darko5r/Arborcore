# Arborcore Precision Renderer Plan

The Precision Renderer is a peer of standard HTML/CSS, not a replacement.
Arborcore supports HTML/CSS-only, Precision-Surface-only and Hybrid pages.

## R0-R3 — foundational raster qualification — CONSTRUCTION

Qualify internal channel precision, coverage precision, explicit surface memory,
half-open pixel cells, conservative raster bounds and analytical rectangle
coverage. Native/WASM semantic equivalence is mandatory.

## R4 — basic primitives

Reference CPU/WASM rasterization for filled rectangles, lines and basic shapes.

## R5 — color and compositing

Freeze color-space conversion, premultiplied-alpha semantics, blend equations
and rounding. RGBA16 is the leading internal representation from R0-R3, but R5
owns the normative compositing decision.

## R6 — deterministic antialiasing

Extend exact/analytical coverage semantics to edges while preserving a software
reference oracle.

## R7 — paths and curves

Deterministic path flattening or direct curve coverage with bounded error.

## R8 — native/WASM raster identity

Golden raster buffers and SHA-256 identity for shared semantic vectors.

## R9 — renderer qualification

Performance, memory, determinism and final software-reference renderer contract.
A later WebGPU backend may accelerate these semantics but does not define them.
