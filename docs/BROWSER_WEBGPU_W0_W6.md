# Browser WebGPU Accelerator W0-W6 — Frozen Contract v1

This phase introduces WebGPU only above the frozen Browser Precision Surface
Contract v1. It does not alter the authoritative Q32.32 Geometry, Reference
Raster, RGBA16 pixel format, B1 RGBA8 export, Canvas/ImageData reference path,
or standard HTML/CSS rendering path.

## Architectural invariant

WebGPU is not a second rendering truth. The accelerator consumes the already
qualified B1 `RGBA8_UNPREMULTIPLIED_SRGB` export. The source texture may be read
back for byte-exact qualification, but translucent WebGPU canvas readback is not
an authority because compositor/premultiplication/color transport can be lossy.
The frozen opaque 2x2 transport probe remains the browser-presentation oracle.

## Browser qualification model

W0-W6 has two deliberately separate browser modes.

### Live user-browser qualification

The primary real-user qualification is performed in browser environments the user actually opens and uses for Arborcore, including distinct Firefox/Chrome editions or launch wrappers. Arborcore does not launch those browsers, inject a temporary
profile, or force a WebGPU enable flag. A loopback HTTP server prints one
`127.0.0.1` URL and waits for the user to open that exact URL in normal Firefox
and normal Chrome.

The page records `window.isSecureContext`, `navigator.gpu`, actual adapter/device
creation, preferred canvas format, adapter information, selected capabilities,
transport hashes, device-loss fallback/recovery, and performance medians. The
browser's actual runtime configuration decides whether the WebGPU path exists.
At least one live reference browser must exercise WebGPU; a live browser without
WebGPU must still pass the frozen Canvas2D fallback.

### Isolated clean-profile qualification

A second automated qualification launches disposable clean-profile browser
instances only as additional reproducibility/fallback evidence. Regular Firefox
runs as a visible, isolated instance on the qualification host display by default
because Firefox's Linux headless software-compositor path can fail before the test
page reports; Chrome remains isolated and headless. Neither profile receives
WebGPU-enabling arguments. WebGPU availability is not required in isolated mode;
each browser must pass whichever path its clean default profile actually exposes.
Isolated qualification never substitutes for the live normal-profile requirement.

## W0 — host and browser admissibility

The host inspection records CPU/GPU/driver/Vulkan/browser/toolchain state without
changing repository bytes. Browser qualification records the actual in-browser
capability rather than inferring it from Vulkan or a preference value.

## W1 — accelerator contract

`browser/arborcore-browser-webgpu-1.contract` pins the frozen B0-B6 contract and
source identities and defines transport, alpha, fallback, resize, loss,
readback, resource, secure-context and browser-admission policies. The WebGPU
JavaScript API remains an evolvable implementation surface while observable accelerator
behavior is frozen by Contract v1.

## W2 — exact RGBA8 GPU transport and presentation

`WebGpuPrecisionSurfacePresenter` uploads tight frozen B1 RGBA8 rows to an
`rgba8unorm` source texture. A full-screen triangle uses `textureLoad`, not
filtering, so every source texel is selected directly. The fragment stage
premultiplies only for WebGPU canvas compositing; it does not alter or replace
the frozen B1 byte buffer.

The preferred WebGPU canvas format is always queried from `navigator.gpu`, and
the context is configured for the sRGB color space. No BGRA/RGBA presentation
format is hard-coded.

## W3 — lifecycle, resize, failure and fallback

The WebGPU canvas and frozen Canvas2D fallback use distinct canvas elements.
`AdaptivePrecisionSurfacePresenter` activates WebGPU when initialization succeeds
and otherwise keeps the frozen B2 path active.

Device loss immediately demotes presentation to Canvas2D. Recovery requests a
fresh adapter/device and reconfigures the context rather than assuming the old
adapter/device remains reusable. Owned upload textures and readback buffers are
explicitly destroyed.

## W4 — equivalence qualification

The browser test proves:

- the frozen RGBA16 and RGBA8 golden hashes before GPU involvement;
- byte-exact RGBA8 source-texture upload/readback when WebGPU is available;
- opaque 2x2 WebGPU presentation transport through `createImageBitmap` and a
  Canvas2D readback sink;
- Canvas2D fallback after explicit device destruction;
- recovery through a fresh adapter/device when the browser permits it;
- frozen Canvas2D behavior in browsers with no usable WebGPU path;
- live user-browser evidence from Firefox and Chrome;
- additional isolated clean-profile reproducibility/fallback evidence.

## W5 — performance and reproducibility

Performance evidence is taken from the live user-browser WebGPU-capable
browser, not from the isolated clean profiles. The browser test records
640x360 WebGPU submit and completed-work medians. These remain diagnostic in Contract v1: the qualification host records a baseline,
but no browser-specific completed-work latency is promoted into a frozen numeric
regression threshold.

The W1-W6 source bundle is independently archived twice with normalized metadata
and must be byte-for-byte reproducible.

## W6 — final admission gate

W6 requires:

1. frozen B0-B6 source/contract identities unchanged;
2. WebGPU contract/source invariants and JavaScript unit checks;
3. live user-browser Firefox and Chrome qualification evidence;
4. at least one live browser exercising a real WebGPU adapter/device path;
5. exact source-texture upload/readback and opaque transport equivalence;
6. device-loss fallback and recovery on the live WebGPU-capable browser;
7. isolated clean-profile Firefox/Chrome reproducibility/fallback qualification;
8. reproducible WebGPU source archive;
9. the complete frozen B0-B6 and lower-layer regression suites.

If neither live user browser environment exposes WebGPU, W6 reports a
platform-capability review state rather than silently treating Canvas fallback as
a WebGPU pass. A browser that initially acquires WebGPU but then suffers genuine
device loss or a platform allocation failure may qualify through the byte-exact
frozen Canvas2D fallback; programming/validation failures do not qualify as an
admissible WebGPU fallback event.

## Qualification-host note

The qualified host uses an NVIDIA GeForce GTX 970M on the proprietary NVIDIA
driver, Vulkan 1.4 and Wayland. Live qualification proved Firefox Developer
Edition can exercise the real WebGPU accelerator on this host, while Chrome 151
can acquire an NVIDIA Maxwell adapter/device but encounters a Dawn/Vulkan device
allocation failure and therefore qualifies only through the exact frozen Canvas2D
fallback. Browser availability remains separate from GPU/driver capability. The
gate records actual in-browser behavior rather than inferring it from Vulkan alone.
