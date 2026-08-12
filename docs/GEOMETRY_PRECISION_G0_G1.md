# Geometry Precision G0-G1 Qualification

G0-G1 is an experiment above the qualified C Runtime Bridge. It does not add a
public geometry API, modify the frozen Assembly ABI, or select a representation
by fiat.

## Rendering architecture

Arborcore keeps three peer rendering modes permanently available:

- HTML/CSS mode uses standard browser DOM/CSS semantics.
- Precision Surface mode uses Arborcore's deterministic numerical and raster
  contracts.
- Hybrid mode uses both in the same document, with explicit conversion at the
  DOM/Precision Surface boundary.

The Precision Surface does not replace HTML/CSS.

## G0 working requirements

The experiment requires a fixed-point candidate to provide at least:

- symmetric practical coordinate reach of at least about one million logical
  pixels in each direction;
- at least 16 fractional bits (1/65536 logical-pixel resolution);
- checked, transactional arithmetic with no silent wraparound;
- explicit floor, ceil, truncation and round-to-nearest/ties-to-even behavior;
- integer storage that maps naturally to WebAssembly i32 or i64.

These are qualification requirements, not yet a frozen public guarantee.
Raster coverage remains separate: coordinates retain fractions until the
rasterizer deliberately resolves coverage into device pixels.

## Candidates

G0-G1 compares:

| Candidate | Storage | Fraction bits | Approx. positive whole-pixel reach | Step |
| --- | ---: | ---: | ---: | ---: |
| Q16.16 | 32 | 16 | 32,767 | 1/2^16 px |
| Q26.6 | 32 | 6 | 33,554,431 | 1/2^6 px |
| Q32.32 | 64 | 32 | 2,147,483,647 | 1/2^32 px |
| Q24.40 | 64 | 40 | 8,388,607 | 1/2^40 px |

Under the G0 working requirements, Q16.16 is range-limited and Q26.6 is
precision-limited. Q32.32 and Q24.40 proceed to the same-host performance
comparison.

## Arithmetic semantics used by the experiment

Add/subtract are exact when representable. Multiply/divide use wider
intermediates and round to nearest with ties to even. Failure is transactional:
the destination is unchanged when the result is unrepresentable or division is
invalid.

This arithmetic policy is being qualified for the later geometry contract; G4
is the point at which public numerical semantics may be frozen.

## G1 selection rule

Q32.32 is the preferred architecture when it satisfies all G0 requirements and
its same-host median arithmetic score is no more than 15 percent slower than
Q24.40. The score is the sum of median checked add, multiply, divide and simple
affine-operation costs across 31 runs.

The preference is intentionally multi-dimensional rather than speed-only:
Q32.32 has 256 times the whole-pixel range of Q24.40 while both candidates
already exceed the working fractional-precision requirement by a very large
margin. If Q32.32 exceeds the performance review threshold, G1 stops for review
instead of weakening the threshold.

## WebAssembly boundary

The experiment can compile a small wasm32 representation probe when `clang` is
available. Lack of that optional compiler does not convert JavaScript `Number`
into an authoritative geometry representation. Browser Precision Surface
geometry remains intended to stay in WebAssembly integer/linear-memory form;
JavaScript inspection should use BigInt or explicit lossy conversion.

The wasm compile probe establishes representational/toolchain feasibility only.
Browser-side performance of wide intermediates must be qualified again in the
future browser/WASM phase before the Geometry Numerical Contract is frozen.

## Exit state

A passing G0-G1 gate produces a recommendation, not a production format freeze:

`G1_NUMERICAL_CONTRACT_STATE=UNFROZEN_EXPERIMENT`

G2 begins production checked geometry only after the recommendation is reviewed.
