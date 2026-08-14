# Browser WebGPU post-freeze optimization — OPT0–OPT5

This phase is intentionally above the frozen Browser WebGPU Accelerator Contract v1. It does not reopen the Reference Raster, Browser Precision Surface v1, or WebGPU Accelerator Contract v1.

## Frozen boundary

- Stable base commit: `66574f02102c0b5bdcc97590ddfb3b30e298cf97`
- Stable base tree: `7b93cfa50f3a0d70b9e8934c187099ab29185a4b`
- WebGPU v1 contract SHA-256: `a0d95467817504dbdc4db38c22b10c210b208e0604857ab752aa2730e97a144b`
- WebGPU v1 JavaScript SHA-256: `b42d48d8f30a9c3cd11c63c68219effe4ad7126d511f0233583fd1a1a59e38bb`
- WebGPU v1 still consumes frozen B1 `RGBA8_UNPREMULTIPLIED_SRGB`.
- Canvas2D remains the frozen reference fallback.
- HTML/CSS remains a parallel independent rendering path.

## OPT0 — inspection

Read-only inspection identifies the existing per-frame path as WASM view acquisition, `queue.writeTexture`, command encoding, current-canvas texture acquisition, render pass, and queue submission. B1 conversion remains a separate CPU/WASM cost. No source changes are admitted by OPT0.

## OPT1 — measurement model

The new measurement model separates:

1. frozen B1 RGBA16→RGBA8 WASM export;
2. WASM typed-view acquisition;
3. `GPUQueue.writeTexture()` call cost on the content timeline;
4. command encoding;
5. `GPUQueue.submit()` call cost;
6. 120-frame burst submission and drain;
7. completion-callback latency.

Completion-callback latency stays diagnostic. It is never labeled GPU execution time.

## OPT2 — v1-compatible optimizer

`OptimizedRgba8WebGpuPresenter` remains a consumer of the exact frozen B1 RGBA8 bytes. It does not alter pixel conversion, shader semantics, canvas format policy, device-loss policy, or fallback semantics.

Its candidate changes are deliberately small:

- cache a stable WASM `Uint8Array` while the memory buffer/offset/length epoch is unchanged;
- invalidate that cache immediately after WASM memory-buffer replacement;
- reuse mutable JavaScript render-pass descriptor objects and the one-element submit list;
- retain the existing size-keyed WebGPU source texture/bind group reuse.

Admission requires byte-exact upload/readback, demonstrable view reuse, and no material (>5%) median submission regression on a live WebGPU browser.

## OPT3 — direct RGBA16 GPU experiment

This is **not** WebGPU Contract v1 and cannot silently replace B1.

A zero-import WASM oracle uses the frozen C `arbor_browser_export_rgba8()` implementation to generate:

- all 65,536 opaque linear16 values;
- 16,384 deterministic transparent/mixed-alpha boundary vectors;
- a 640×360 mixed-alpha performance surface.

The GPU experiment uploads `rgba16uint`, performs the exact integer unpremultiply / nearest-even transfer / alpha reduction in WGSL, writes packed RGBA8 from a compute pass, maps it back, and requires byte-for-byte equality with the frozen C B1 oracle. Direct presentation is measured separately.

Even exact and faster OPT3 results remain experimental. A future contract revision would require an explicit new qualification/freeze phase.

## OPT4 — browser matrix

The current host gate requires normal-profile Firefox and Chrome, preserving the same honest fallback semantics as WebGPU v1. At least one live browser must execute the WebGPU optimization path. Chrome platform allocation/device loss may qualify only through the already-frozen fallback semantics.

Safari/WebKit and Edge are accepted by the portable report format as future external-host evidence, but are not required on this Linux host. No broad-browser release claim is admitted until Safari/WebKit is formally qualified on an appropriate host.

## OPT5 — frozen decision

OPT0–OPT5 qualified successfully on the reference host. The frozen decision is intentionally asymmetric:

- OPT2 is admitted as an **optional WebGPU-v1-compatible implementation**. It remains a parallel implementation; the frozen WebGPU v1 reference source and RGBA8 input contract are unchanged.
- OPT3 is retained as an **experimental exact-equivalence result** only. Its direct RGBA16 input boundary is not admitted into WebGPU Contract v1 and requires an explicit future contract revision before production integration.
- OPT1 completion-callback latency remains diagnostic and is not labeled GPU execution time.
- OPT4 does not admit a broad-browser release claim until Safari/WebKit is formally qualified.

The frozen post-W6 optimization decision is recorded by Post-freeze Optimization Contract v1.0.

## Workflow

1. apply the integrated candidate;
2. run static/zero-import WASM qualification;
3. run one normal-profile Firefox+Chrome OPT live test;
4. run the full OPT0–OPT5 gate;
5. review evidence;
6. freeze Post-freeze Optimization Contract v1.0 with OPT2 admitted and OPT3 explicitly experimental;
7. stage exact tree, commit, push feature, fast-forward main.


## Qualification-page canvas lifetime

The live qualification page intentionally destroys/unconfigures the baseline, OPT2, and OPT3 WebGPU presenters after the JSON evidence has been collected and reported. The canvas frames can therefore remain visible while their GPU images disappear. This is qualification cleanup, not a rendering failure. A future human-facing demo should use a separate page that deliberately retains presentation resources.
