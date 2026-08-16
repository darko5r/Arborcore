# Arborcore Application / DDD / MVC Foundation — AF0–AF1

## Scope

AF0–AF1 establishes the contract immediately above the qualified Arborcore
Assembly/C runtime and alongside the existing Geometry, Renderer and Browser
capabilities. It does not implement controllers, middleware, repositories,
MariaDB, R or deployment integration.

The base is commit `a8d23f77bea211e31bd404230f06c2bc6e8a33d1`, tree
`3da07e77f2af6103e8f0bd8e3781c6000f8082a5`.

## Architectural rule: controlled dependencies, bidirectional runtime calls

Arborcore does not impose a one-way runtime call graph. Application/MVC code may
consume appropriate lower framework services through the public Arborcore C API
and other qualified capability interfaces. Lower framework/runtime code may
invoke higher application behavior only through explicit registered capability
ABIs or callbacks.

Source and policy dependencies remain controlled even when runtime calls travel
both directions. In particular, reusable low-level code may know a generic
application-dispatch capability but must not hard-code a blog controller, a
commerce use case, a repository implementation, SQL or other application policy.

The Domain layer remains inward-facing. Domain rules do not depend directly on
HTTP request objects, the Assembly ABI, browser state, MariaDB, R or deployment
facilities.

## Frozen does not mean permanently immutable

Existing Assembly, C-runtime, Geometry, Renderer and Browser freezes are
qualified baselines. AF preserves them by default. If a later AF phase proves a
lower-layer limitation, the affected contract may be reopened only through a
controlled retrofit:

1. record the concrete limitation and necessity;
2. identify the smallest affected boundary and all dependents;
3. analyze ABI/layout/semantic compatibility;
4. implement the smallest repair or extension;
5. run focused/adversarial tests;
6. rerun every affected and dependent qualification gate;
7. compare performance/resource behavior when relevant;
8. regenerate reproducibility and identity evidence;
9. review the new baseline before continuing AF construction.

AF0–AF1 itself requires no lower-layer retrofit.

## Three deterministic result channels

AF keeps three concepts separate.

1. **Mechanism/framework failure** uses `arbor_status`. This includes invalid
   arguments, capacity failures and lower infrastructure failures.
2. **Application/domain outcome** is a bounded-context-owned typed result. AF
   deliberately does not define a universal business-error enum or CRUD result.
3. **Presentation result** is an `arbor_response_plan`, which maps the selected
   application outcome into the currently qualified HTTP serializer subset.

A domain result is therefore never represented by a Linux errno merely because
an HTTP controller may later map that result to a status code.

## Request scope and ownership

`arbor_request_scope` is a request-lifetime view. It borrows:

- the parsed `arbor_request_view`;
- route-parameter records;
- the connection/request arena.

Existing lower contracts remain authoritative: route-parameter names borrow
immutable catalog/pattern storage and route-parameter values borrow active
request-target/input storage. Request-derived bytes that must survive input
consume/reset/reuse must be copied to appropriate owned storage, normally the
request arena for request-local work.

The request arena is caller/runtime-owned. AF does not free or resize it. Its
lifetime ends when the lower request lifecycle resets/reuses it. Sensitive values
must follow the existing secure-clear policy when required by the threat model.

## Response plan

AF1 intentionally matches the currently qualified bounded response serializer.
`arbor_response_plan` carries:

- status;
- borrowed body span;
- response flags (`KEEP_ALIVE` currently).

Supported statuses are 200, 201, 204, 400, 404 and 500. Status 204 requires an
empty body. Arbitrary headers, cookies, redirects, streaming and broader status
coverage are not silently invented in AF1; they require a later explicit
response-contract phase.

The body remains caller-owned and must stay valid through serialization. The
existing lower serializer retains its whole-operation snapshot/transactional
alias semantics.

## Framework-to-application capability ABI

AF1 defines `arbor_application_capabilities`, ABI version 1. It is an explicit
prefix-stable table containing:

- ABI version and structure size;
- capability flags;
- opaque application/composition context;
- request-dispatch callback.

The composition root will eventually own and publish a validated capability
instance. AF1 introduces no mutable global registry.

The callback ABI is deliberately friendly to both C and hand-written x86-64
Assembly. `arbor_request_scope` points only at frozen Assembly request, target,
route-parameter and arena representations; it does not expose the evolvable C
`arbor_request_view` aggregate across the callback ABI. `arbor_response_plan`
uses fixed scalar/pointer fields. The callback returns a native signed 64-bit
mechanism status in `RAX` and writes a candidate response plan. Public C callers
use `arbor_application_invoke()`, which normalizes that native value into
`arbor_status`. AF1 defines callback return `0` as success and a negative value
as a mechanism failure. Positive callback values are reserved and rejected as an
invalid callback result; this prevents Arborcore's generic positive native-status
classification from appearing successful when no response plan is published.
Caller-visible response output is published only when the callback returns zero
and the response plan validates.

This function is a generic framework/application upcall boundary, not a
controller implementation. MVC route catalogs and controller selection remain
AF2/AF3 work.

## Application-to-framework direction

Application/MVC code is not isolated from lower Arborcore capabilities. It may
call appropriate public framework services directly. AF1 demonstrates this by
serializing a validated response plan through `arbor_response_serialize()`.
Future application code may similarly use qualified memory, security,
serialization, geometry, rendering or other APIs where doing so respects layer
responsibilities.

This permission does not make Domain code dependent on transport or
infrastructure APIs.

## Current frozen-server integration boundary

The current Assembly server calls the frozen route handler and interprets its
nonnegative `int64_t` result as the HTTP status. It then resets the output buffer
and serializes an empty-body response. AF0–AF1 does not widen or work around that
contract.

A later integration phase must decide, with evidence, whether rich MVC network
responses belong in a higher transport composition or justify a controlled
runtime retrofit. The existing freeze is a baseline, not a prohibition on an
otherwise justified future improvement.

## AF0–AF1 qualification

The gate requires:

- exact contract lines;
- exact lower-layer source/contract aggregates;
- native layout/validation/upcall/downcall tests;
- ASan/UBSan qualification;
- deterministic source archive reproduction;
- exact construction-scope verification against the authorized base;
- `git diff --check`.

The AF0–AF1 candidate stops before AF2 construction and before any commit or
remote write.

## AF0–AF1 source review repair R1

R1 closes the callback-status publication ambiguity found during source review.
Framework-to-application dispatch now accepts exactly zero as callback success,
normalizes negative mechanism failures, and rejects positive callback values
transactionally. This does not change the capability-table layout or any protected
lower-layer source.
