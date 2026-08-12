# Precision Renderer R0-R3 Foundation Qualification

R0-R3 is an experimental qualification layer above the frozen Geometry
Numerical Contract v1. It does not change Assembly, CR0-CR8, or Q32.32
geometry semantics and does not replace HTML/CSS.

## R0 — color and coverage candidates

The internal color precision floor is 16 bits per channel. RGBA8 remains a
browser/display export reference; RGBA16 and RGBA32 are internal candidates.
R0 recommends RGBA16 when its sequential fill cost stays within 2.5x RGBA8,
because it meets the precision floor at half the storage of RGBA32. Color-space
transfer and final blending semantics are deliberately deferred to R5.

Coverage candidates are Q0.24 and Q0.32. Both meet the 24-bit coverage floor.
Q0.32 aligns exactly with the frozen 32 fractional geometry bits and retains
coverage smaller than Q0.24 can represent. It is recommended when it stays
within 15% or 2 ns absolute median cost of Q0.24.

## R1 — surface memory

Candidate precision surfaces are row-major with explicit stride, explicit
bytes-per-pixel, checked dimensions and checked buffer size. Pixel offsets are
checked before access. R0-R3 uses RGBA16's 8-byte pixel size as the leading
internal surface candidate.

## R2 — raster bounds

Each integer pixel `(x,y)` owns the half-open cell `[x,x+1) x [y,y+1)`. A
Q32.32 device rectangle is conservatively mapped to integer raster bounds using
floor on left/top and ceil on right/bottom, then clipped to the surface.
Fractional geometry is not rounded away.

## R3 — analytical rectangle coverage

Axis-aligned rectangle coverage is computed analytically from exact Q32.32
interval intersections. There is no supersampling. Q0.32 full coverage is
represented by `2^32` in a 64-bit integer so both 0 and 1.0 are exact.

The same semantic vectors run natively and in a zero-import wasm32 module.
JavaScript `Number` is not authoritative.

R0-R3 ends with a recommendation only. R4-R9 may adopt the selected foundation,
but the raster/color/compositing contract is not frozen here.
