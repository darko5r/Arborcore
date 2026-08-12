# Geometry Precision G2-G4

G2-G4 turns the G0-G1 Q32.32 recommendation into the first production
Geometry Precision implementation. The phase freezes the **numerical
semantics**, not the evolving higher-level C framework API.

## Layer boundary

The geometry subsystem is deliberately portable. `include/arborcore/geometry.h`
does not include the Linux/x86-64 Assembly ABI and `src/c/geometry.c` does not
call the Assembly core. This lets the same fixed-point implementation compile
natively and for wasm32 while the frozen Assembly ABI remains the native
server/runtime foundation.

The existing CR0-CR8 files remain unchanged:

- `include/arborcore/arborcore.h`
- `include/arborcore/assembly_abi.h`
- `src/c/status.c`
- `src/c/security.c`
- `src/c/request.c`
- `src/c/route.c`
- `src/c/runtime.c`

## G2: checked Q32.32 geometry

`arbor_coord` is signed Q32.32 stored in `int64_t`. Add/subtract are exact when
representable. Multiply/divide use 128-bit intermediates and nearest-even
rounding. Conversion exposes explicit toward-zero, floor, ceil and nearest-even
modes. Failure is transactional: output objects are not modified when an
operation reports invalid input, overflow or a singular transform.

Production types include points, sizes, rectangles, lines and insets.
Rectangles have nonnegative dimensions and half-open point-membership semantics.
Intersection, union, translation and line bounds are checked.

## G3: affine transforms, clipping and deterministic rotation

Affine transforms use six Q32.32 coefficients in the conventional 3x3 form.
Composition `left o right` applies `right` first. Point transformation fuses two
Q32.32 products plus translation and rounds once per output axis.

Inversion uses an exact Q64.64 determinant in a signed 128-bit intermediate,
then nearest-even reduction back to Q32.32. Singular matrices fail explicitly.

Arbitrary rotation coefficients are generated without host floating point.
Angles are Q32.32 **turns** (`1.0 == 360 degrees`) and a fixed 31-step integer
CORDIC implementation generates sine/cosine. Cardinal quarter-turns are exact.
The fixed CORDIC constants are part of the production source identity.

Clipping is rectangle intersection with the same half-open geometry semantics.

## G4: logical to device mapping

DPR, browser zoom and analogous scale factors enter Precision Surface as exact
positive reduced rationals. Scale composition performs cross-reduction before
multiplication and rejects unrepresentable ratios.

Logical coordinates remain Q32.32 after device mapping. Scaling is
nearest-even and an optional device-space Q32.32 origin is added afterward.
Rectangle mapping transforms edges rather than prematurely rounding widths.
The rasterizer will receive fractional physical coordinates and decide actual
pixel coverage later.

For example, `24.375` logical pixels at DPR `2/1` remains `48.75` device pixels;
it is not rounded to physical pixel 49 during geometry.

## Browser/WASM boundary

The portable geometry header does not depend on native pointer size or Assembly
layouts. wasm32 stores each Q32.32 coordinate as WebAssembly `i64`. The G4 gate
links the production geometry implementation into a freestanding WASM module
and executes a numerical self-test through Node.js or another supported WASM
runtime when available.

Because current Clang wasm32 environments may lower `__int128` multiply/divide
to compiler-runtime helpers, Arborcore supplies a tiny freestanding wasm-only
128-bit helper implementation for the exact operations required by geometry.
It is not used by the native library.

JavaScript `Number` is explicitly non-authoritative for Precision Surface
geometry. Browser integration will keep values in WASM i64/linear memory or use
an explicit loss-aware conversion at the HTML/CSS hybrid boundary.

## Rendering modes

Nothing in this contract replaces HTML or CSS. Arborcore keeps three peer
modes:

1. standard HTML/CSS/DOM;
2. Precision Surface;
3. Hybrid composition with explicit conversions between them.

## Freeze scope

`geometry/arborcore-geometry-1.contract` is the G4 numerical freeze candidate.
It freezes representation, arithmetic/rounding/overflow semantics, rectangle
semantics, affine composition, deterministic rotation semantics, device-scale
rules and the browser representation boundary. It does **not** freeze the
current C function names or the full higher-level framework API.

## Qualification implementation notes

The G4 WASM compiler-runtime helpers are marked as optimizer-opaque under
Clang. This is required because newer wasm32 optimizer pipelines can otherwise
recognize the manual 64-bit partial-product implementation of `__multi3`, fold
it back into a 128-bit multiply, and recursively call `__multi3` again. The
runtime WASM self-test remains the authoritative check.

Native x86-64 Q32.32 nearest-even division uses the hardware unsigned 128-by-64
`DIV` primitive after a fail-closed quotient-range precheck, then applies the
frozen sign/range/nearest-even rules. This avoids compiler-runtime 128-bit
division calls without changing numerical semantics. Other native targets retain
a portable signed-128 fallback, while wasm32 keeps the generic
unsigned-magnitude reduction path so its freestanding compiler runtime remains
small. Native and WASM are required to pass the same semantic vectors.

Q32.32 multiplication and fused affine reduction never use general division for
the fixed `2^32` scale. They take the unsigned 128-bit magnitude, shift right 32
bits, inspect the low 32 remainder bits, apply nearest-even at `0x80000000`, and
then restore the signed result after a transactional range check. The native
x86-64 contract verifier disassembles `arbor_coord_mul` and
`arbor_affine_transform_point` and rejects `DIV`/`IDIV` in those hot paths.

The G2-G4 performance gate compares equivalent operations individually: add,
multiply, divide and fused affine point transform. Candidate coefficients,
divisors and translations are loaded from volatile runtime inputs before each
measured block, preventing the compiler from constant-specializing the inline
G1 candidate while production receives runtime values through the library call
boundary. The affine reference forms the exact two-product sum plus translation
and rounds once per axis, matching production semantics. A metric passes under
the predeclared policy only when its median production cost is within 15 percent
relative overhead or within 8 ns absolute overhead; metrics may not compensate
for one another.
