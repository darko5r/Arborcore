# Browser v2 Hardening / Optimization — BV2H0 through BV2H9

This candidate continues from the frozen Browser Language Boundary v2 and the completed Browser v1 JavaScript retirement. It does not reopen the language-authority decision: production JavaScript remains the browser host/syscall/presentation shim, portable authoritative CPU behavior remains C→WASM, and GPU arithmetic remains WGSL.

R2 freezes the Browser-v2 hardening/observability layer while keeping OPT3 outside production authority. Production OPT3 admission remains deferred to a future explicit browser/WebGPU contract revision.

## Non-negotiable authority boundary

- Native authority: Assembly + C.
- Portable browser authority: C compiled to zero-import WASM.
- GPU arithmetic: WGSL.
- Production JavaScript: `browser/arborcore_host.js`, unchanged by this candidate.
- Authoritative JavaScript logic: zero.
- Diagnostic JavaScript is test-only. It observes browser APIs, drives host calls and timestamps externally observable operations; lifecycle transition policy, resource counters and timing aggregation/statistics are C/WASM-owned.
- HTML/CSS stays first-class. The Precision Surface remains optional and parallel.

## BV2H0 — inventory and baseline preservation

Inventory verifies the exact clean base commit/tree, the frozen production host SHA, the LBv2 contract and historical Browser/WebGPU v1 recovery paths. The protected OPT0–OPT5 archive is inspected as historical experimental input, not copied back into production.

## BV2H1 — rich observability

The live qualification page now exposes browser/runtime state, frozen hashes, C/WASM state, WebGPU adapter/device diagnostics when available, DPR results, resource-lifetime counters, timing distributions, lifecycle/recovery evidence, OPT2/OPT3 evidence and raw JSON. Rich UI does not imply JavaScript authority.

## BV2H2 — performance instrumentation

`browser_hardening_v2.c` stores fixed-capacity timing samples without allocation. C/WASM computes count, min, max, mean, p50, p95 and p99. The browser diagnostic driver records externally measurable phases:

1. C/WASM validation;
2. source-view/resource acquisition;
3. queue texture upload call;
4. command encoding + submission;
5. queue-completion callback latency (diagnostic synchronization, not GPU execution time);
6. synchronized diagnostic presentation latency;
7. host enqueue latency ending immediately after `queue.submit()`.

The initial candidate records evidence only. No hardware-specific threshold is frozen before a reproducible baseline exists.

## BV2H3 — OPT2 resource lifetime extension

The existing LBv2 presenter already caches the WASM typed-array view and reuses source textures/bind groups when dimensions are stable. BV2H adds independent counters for source-view creation/reuse, texture creation/reuse, bind-group creation/reuse, pipeline creation/reuse and WASM memory-buffer changes. Qualification demonstrates what is actually reused before making further production changes.

## BV2H4 — OPT3 qualification, not automatic admission

The direct RGBA16 WGSL conversion remains test-only. Firefox 154 produced the frozen RGBA8 result byte-for-byte at both the golden scene and the retained 640×360 / 1280×720 performance workloads. Its performance signal is strong positive, but only one successful WebGPU implementation established exact OPT3 equivalence. Chrome 151 produced a qualified external-instance/device-lifetime platform failure before comparable OPT3 evidence. Production admission is therefore deferred to a future explicit contract revision.

## BV2H5 — lifecycle hardening

C/WASM owns a deterministic lifecycle machine:

`COLD → PROBING → WEBGPU_READY`

Platform failure moves probing to `FALLBACK_READY`. Device loss moves `WEBGPU_READY → DEVICE_LOST → RECOVERING`, followed by either `WEBGPU_READY` or `FALLBACK_READY`. Destruction terminates in `DESTROYED`. Native/WASM tests reject invalid transitions. Live qualification performs a post-qualification device-loss/reacquisition probe on a WebGPU-capable browser without making recovery a hidden production policy.

## BV2H6 — DPR / device-size qualification

The vector set covers DPR 1, 1.25, 1.5, 2, 2.625 and 3, fractional CSS sizes and the device-pixel-content-box precedence case. Expected sizes are checked against the existing C/WASM LBv2 resolver.

## BV2H7 — visual transport suite

The candidate uses already-established deterministic scenes instead of inventing unqualified geometry semantics: frozen RGBA16, frozen RGBA8, opaque Canvas2D transport, WebGPU upload/canvas transport and test-only direct RGBA16 conversion when WebGPU is available. Additional geometry scenes can be added after this integrated baseline is reviewed.

## BV2H8 — browser matrix

Normal Firefox and normal Chrome are required. At least one must exercise WebGPU. A browser without a usable WebGPU path may qualify deterministic Canvas fallback according to the frozen LBv2 failure classifier. Safari/WebKit remains not admitted until formally executed and qualified.

## BV2H9 — integrated gate and freeze readiness

The final R2 gate runs contract/audit/native/WASM/reproducibility checks, verifies retained LBv2/Browser-v1 evidence, consumes fresh Firefox/Chrome live evidence and the retained OPT3 performance evidence, and seals the Browser-v2 hardening contract. It does **not** create a commit, push, or mutate `main`.

Final freeze decision:

`BV2H9_DECISION=FREEZE_BROWSER_V2_HARDENING_OPTIMIZATION`

Staging, commit and promotion remain explicit later checkpoints.

### BV2H review repair R1

Freeze review distinguishes production-relevant host enqueue latency from diagnostic queue-completion latency. Browser timer resolution is recorded because privacy/timer-precision policy can quantize short host intervals. Late WebGPU platform failures after successful initialization transition `WEBGPU_READY → FALLBACK_READY` through an explicit runtime-fallback event rather than leaving stale ready state. The live suite also exercises WASM memory growth and source-view cache invalidation.


## BV2H review repair R2 / final OPT3 disposition

R2 cleans the remaining diagnostics semantics before freeze:

- `failureClass=0` is reported with `failureStage=NONE`;
- qualified-surface captions distinguish PASS from NOT QUALIFIED after fallback;
- browser callback/queue-completion latency remains diagnostic-only and is never labeled GPU execution time;
- live WASM memory growth must recreate the cached source view;
- late class-2 Chrome failure ends in `FALLBACK_READY`.

Retained OPT3 performance evidence is diagnostic, environment-specific evidence rather than a universal threshold. On Firefox 154 the direct path remained byte-exact and showed a strong host-enqueue reduction signal at 640×360 and 1280×720. WebGPU timestamp-query pass durations exclude `queue.writeTexture` transfer, so no end-to-end GPU speedup claim is made.

The frozen disposition is:

`BV2H_OPT3_STATE=QUALIFIED_TEST_ONLY_PROMISING_FUTURE_CONTRACT_REVISION`

and:

`BV2H_OPT3_PRODUCTION_ADMISSION=DEFERRED`.


## BV2H review repair R2.1 — qualification evidence integrity

The source review found that the R2 live-evidence consumer accepted retained build reports without binding them to the exact candidate that produced them. R2.1 repairs the gate without changing production rendering or authority: live qualification records the SHA-256 aggregate of the exact 23-path candidate and the live WASM SHA-256; evidence verification recomputes and requires both identities. A new live run deletes stale retained reports before serving browsers.

The deterministic BV2H source/evidence archive now includes the modified `Makefile`, bringing the archive inventory to 31 files. Its SHA-256 remains an externally recorded freeze identity because embedding an archive's own expected hash inside that same archive would be self-referential.
