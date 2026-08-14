# Browser v1 JavaScript retirement

Browser Language Boundary v2 makes Assembly/C the native authority,
C/WASM the portable browser authority, WGSL the GPU authority, and
JavaScript a browser-host syscall shim only.

The Browser Precision Surface v1 and WebGPU Accelerator v1 JavaScript
implementations are therefore retired from the current source tree.

Their source is not destroyed. The exact historical implementations remain
addressable through their frozen Git commits, trees, blob object IDs, and
SHA-256 identities.

Current production JavaScript is:

- `browser/arborcore_host.js`

Retired production JavaScript is:

- `browser/precision_surface.js`
- `browser/webgpu_accelerator.js`

The frozen Browser v1 and WebGPU v1 contracts remain in the repository.

The retained Browser v1 C layer continues to qualify:

- native browser-surface semantics;
- ASan/UBSan behavior;
- zero-import C/WASM export behavior;
- renderer-equivalent RGBA8 golden output;
- deterministic library reproducibility;
- browser-export performance.

Legacy JavaScript unit tests, browser pages, and runners whose sole purpose
was executing the retired implementations are removed.

`tools/browser_v1_history_verify.sh` verifies the retired implementations
against their frozen Git objects.

`tests/data/browser_v1_precision_vectors.json` preserves the Browser v1
Q32.32/CSS and device-size behavior used as an equivalence oracle by
Browser Language Boundary v2.

The retirement does not rewrite Git history and does not alter the frozen
Browser v1 or WebGPU v1 commits.

## Frozen retirement status

Browser v1 JavaScript retirement is frozen at contract version 1.0.

The current browser production JavaScript surface contains exactly one file:

- `browser/arborcore_host.js`

The retired Browser Precision Surface v1 and WebGPU Accelerator v1
JavaScript implementations are absent from the current tree and remain
recoverable through their frozen Git objects and recorded SHA-256 identities.

The frozen retirement preserves:

- `AUTHORITATIVE_JS_LOGIC=ZERO`;
- the Browser v1 C/WASM lower-layer qualification;
- the Browser v1 and WebGPU v1 historical contracts;
- the Browser Language Boundary v2 frozen delivery identity;
- current post-freeze LBv2 qualification evidence.
