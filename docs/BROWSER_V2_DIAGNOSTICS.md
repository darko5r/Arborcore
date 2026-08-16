# Browser v2 diagnostics surface

The BV2H diagnostics page is a development/qualification surface, not an application UI and not a source of rendering truth. It exists so low-level behavior can be inspected without returning numerical, geometry or fallback authority to JavaScript.

The page shows:

- browser user agent and secure-context state;
- zero-import WASM state and memory size;
- production JavaScript role and `AUTHORITATIVE_JS_LOGIC=ZERO`;
- frozen RGBA16/RGBA8/Canvas hashes;
- WebGPU mode, failure class, adapter/device information when exposed by the browser, device features/selected limits and canvas format;
- source-view, texture, bind-group and pipeline creation/reuse counters;
- C/WASM lifecycle state and recovery evidence;
- C/WASM timing aggregates for validation, view acquisition, upload call, encode/submit, host enqueue, queue-completion callback latency and synchronized diagnostic presentation;
- DPR/device-size vector results;
- OPT3 exactness with explicit non-admission state;
- broad browser release state, retaining WebKit/Safari as unqualified.

Browser timestamps are necessarily observed by test-only JavaScript using `performance.now()`. Raw elapsed samples are passed into C/WASM; percentile and aggregate values displayed by the page are read back from C/WASM. These measurements are diagnostic, not deterministic semantic output.

## Review repair R1 — measurement semantics

The live page reports the observed `window.devicePixelRatio` and an estimated `performance.now()` resolution. `queueCompletionLatency` is the latency of `GPUQueue.onSubmittedWorkDone()` as observed by the browser host and **must not be interpreted as GPU execution time**. `hostEnqueue` stops immediately after `queue.submit()` and is the production-relevant CPU/host enqueue diagnostic. `synchronizedPresent` includes the explicit diagnostic completion wait.

If WebGPU initializes and later encounters a qualified platform/runtime failure, the report retains pre-failure WebGPU counters but labels the final mode as fallback, records the failure stage, increments the fallback metric, and requires the C/WASM lifecycle state to become `FALLBACK_READY`. Adapter identity may be privacy-redacted by the browser.

A live WASM-memory-growth probe verifies that ArrayBuffer identity replacement invalidates the cached source view and that presentation continues correctly.


## Review repair R2 — final report semantics

A successful WebGPU qualification with `failureClass=0` reports `failureStage=NONE`. A nonzero failure class reports the exact stage at which qualification fell back. The visual surface captions are result-sensitive: WebGPU transport, OPT3 and recovery are marked PASS only when that browser actually qualified those surfaces; otherwise they are marked NOT QUALIFIED.

This avoids presenting a fallback browser as if it had qualified direct WebGPU/OPT3 surfaces while preserving the useful pre-failure resource and timing counters.

## Review repair R2.1 — evidence binding

Live browser evidence is bound to the exact 23-path BV2H candidate aggregate and to the exact live WASM module SHA-256. Starting a live qualification removes prior retained browser reports before rebuilding the module. Evidence mode recomputes the current candidate aggregate and requires both it and the current live WASM module to match the identities recorded by the successful Firefox/Chrome qualification run. This prevents stale browser reports from qualifying a changed candidate.
