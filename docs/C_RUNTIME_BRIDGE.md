# Arborcore C Runtime Bridge — CR0–CR8

## Boundary

The C runtime bridge begins at library-packaging commit
`cad044071fbb877239ec9c1621a65071cacca9e9`.  Assembly ABI v1.0 and its
94-symbol capability surface are frozen underneath this layer.

CR0–CR8 may add C headers, C implementation, tests, benchmarks and build
integration. It must not modify `src/asm`, the v1 symbol/layout/version-script
manifests, or the canonical Assembly library hashes.

The default C-to-Assembly composition is static through `libarborcore.a`.
Every bridge gate also links and executes an equivalent consumer through
`libarborcore.so.1` so dynamic compatibility cannot silently diverge.

## CR0 — native ABI contract

`include/arborcore/assembly_abi.h` is the exact Linux x86-64 System V C view of
Assembly ABI v1. It contains all 94 public Assembly declarations and frozen
layout assertions.

Many native functions return a status/result pair in `RAX:RDX`. The System V
AMD64 ABI returns a two-INTEGER-eightbyte C aggregate in those same registers,
so the bridge models the native result as 16-byte result structures instead of
adding an adapter ABI.

## CR1 — static/shared consumers

`libarborcore_runtime.a` is a deterministic archive containing only the C
bridge. Static qualification links it to `libarborcore.a`. Shared equivalence
links the same runtime archive to the versioned Assembly shared object.

The C runtime archive is not a frozen public release yet; its API remains under
construction while the higher-level framework is built.

## CR2 — frozen layouts

C representations are compile-time checked for:

- BUFFER 24 bytes
- ARENA 24 bytes
- CONNECTION 96 bytes
- HTTP_REQUEST 96 bytes
- REQUEST_TARGET 32 bytes
- ROUTE 40 bytes
- ROUTE_PARAM 32 bytes
- EPOLL_EVENT 12 bytes

Connection state values and server work-budget constants are also mirrored from
the frozen layout manifest.

## CR3 — status translation

Assembly negative Linux errno values remain preserved in `arbor_status.native`.
The C layer adds a typed classification without destroying the native code.
Unknown negative values remain `ARBOR_STATUS_NATIVE_ERROR`.

## CR4 — ownership and lifetime

The bridge is intentionally span/view oriented:

- `arbor_span` is borrowed and never implicitly copied.
- `arbor_request_view` aliases the supplied request bytes.
- route parameter names alias immutable route-catalog storage.
- route parameter values alias request-target/input storage.
- `arbor_runtime_storage` owns metadata only; input/output/arena backing spans
  remain caller-owned.
- logical buffer/arena reset is not secure erasure. Sensitive bytes require
  `arbor_secure_clear` when the threat model requires zeroization.

## CR5 — request, route and response bridge

The C layer composes the frozen parser, request-target splitter, pattern router
and response serializer. It does not reimplement them.

C route handlers are called directly from Assembly using the System V handler
ABI:

```
handler(request, context, params, parameter_count) -> int64_t
```

## CR6 — connection/event/server bridge

The C runtime wraps the frozen transactional accept, event loop, server work
budget and connection lifecycle. `server_create_epoll` is a listener-bound
capability: the listener fd is passed in System V argument register `RDI` so
the Assembly routine can create epoll and register that listener atomically as
part of its server composition contract. The Assembly runtime remains
authoritative for socket/event/parser/router/response behavior.

## CR7 — hardening

CR qualification includes strict compiler diagnostics, ABI-header inventory,
C-runtime external dependency classification, adversarial bridge tests, an
AddressSanitizer/UndefinedBehaviorSanitizer pass, and the complete frozen
Assembly regression suite.

## CR8 — bridge overhead

Bridge performance is measured against the exact equivalent raw Assembly calls
in the same process. This is an initial C-bridge policy, not a widening of an
older threshold.

A metric passes when either its median relative wrapper overhead stays inside
its CR envelope or its absolute wrapper overhead is at most 8 ns. This avoids
penalizing tiny primitives solely because a few nanoseconds represent a large
percentage while still rejecting material wrapper cost.

## Next layer

After CR0–CR8 is sealed, Arborcore can begin its C framework capabilities and
Geometry Precision Model. HTML/CSS remains a first-class rendering path. The
Precision Surface is parallel and optional; hybrid applications may use both in
the same page.
