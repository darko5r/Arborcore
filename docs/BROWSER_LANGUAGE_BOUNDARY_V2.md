# Arborcore Browser Language Boundary v2

## Purpose

Browser Language Boundary v2 corrects language ownership at the browser edge without
rewriting the qualified Browser Precision Surface v1 or WebGPU Accelerator v1 history.
Those frozen implementations remain equivalence oracles. The preferred v2 path moves
all Arborcore numerical, validation, layout, state-policy and lookup-table authority into
C compiled to both native code and zero-import WebAssembly. GPU arithmetic is explicit
WGSL. JavaScript is restricted to browser-host API invocation.

## Language ownership

- x86-64 Assembly / C: native core and low-level semantics.
- C -> WASM: authoritative portable browser calculations and state.
- WGSL: GPU execution only.
- JavaScript: browser host syscall shim only.
- HTML/CSS: first-class standard presentation.
- R: optional offline analysis/statistics; never a browser host dependency.

## LB0-LB8

- LB0: inventory and preserve frozen Browser/WebGPU v1 identities.
- LB1: C/WASM numerical and host-policy authority.
- LB2: move Q32.32/CSS and DPR/size policy out of JavaScript.
- LB3: define WebGPU host boundary where WASM validates/decides and JS invokes host APIs.
- LB4: eliminate JavaScript lookup-table authority; upload GPU-ready tables from WASM memory.
- LB5: externalize GPU programs as WGSL source files.
- LB6: reduce preferred production JavaScript to one audited host shim.
- LB7: requalify OPT2 under v2; retain OPT3 as test-only experiment.
- LB8: full equivalence/regression gate and freeze decision.

## Compatibility

Frozen Browser Precision Surface v1, WebGPU Accelerator v1 and their hashes are not
modified. v2 must reproduce their established RGBA16, RGBA8, Canvas and WebGPU/fallback
semantics. A future direct-RGBA16 WebGPU contract revision remains separate from v1.

## Reference policy

The C references in `docs/SOURCES.md` are design/learning material only. Private EPUB
files are never part of the repository. Clean-code guidance is subordinate to ABI,
determinism, memory locality and measured low-level performance constraints.

## Frozen v2 decision

Browser Language Boundary v2 is frozen after LB0-LB8 qualification. The preferred
production browser implementation has one audited JavaScript host syscall shim and
zero authoritative JavaScript logic. Numerical, validation, layout and fallback-policy
authority remains C compiled to zero-import WebAssembly; GPU execution remains WGSL.
OPT2 is reimplemented under this v2 boundary. OPT3 remains test-only and requires a
future contract revision before production admission. Frozen Browser Precision Surface
v1 and WebGPU Accelerator v1 remain byte-exact historical equivalence oracles.

- `ARBORCORE_BROWSER_LANGUAGE_BOUNDARY_VERSION=2.0`
- `LB_CONTRACT_STATE=FROZEN`
- `LB_DELIVERY_STATE=FROZEN_V2`
- `LB_AUTHORITATIVE_JS_LOGIC=ZERO`
