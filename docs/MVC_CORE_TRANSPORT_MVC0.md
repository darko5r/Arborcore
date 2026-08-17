# Arborcore MVC0 — Core MVC and Rich Application Transport

MVC0 is the first complete request-to-presentation layer above the frozen AF0–AF4 Application Foundation. It adds MVC routing, middleware, controllers, presenters, and a parallel rich HTTP transport that can deliver a full `arbor_response_plan` body over a real socket. The existing status-only Assembly server remains available and byte-exact.

## Why the transport is parallel

`server_handle_http_once` is part of the frozen Assembly ABI v1 path. Its route-handler contract returns an integer HTTP status and the server serializes an empty body. AF1 already supports a body-bearing `arbor_response_plan`, so changing the legacy handler meaning would be an ABI retrofit and would destabilize a qualified path.

MVC0 instead adds `src/asm/application_transport.asm` as a higher-layer Assembly object. It reuses the qualified connection state machine, epoll interest changes, HTTP framing/parser, buffers, arena, read/write retry primitives, and close path. These dependencies are Arborcore-private static-link dependencies and are not new Assembly ABI v1 promises.

The public C entry is `arbor_application_server_step()`. Its internal adapter creates an AF1 request scope, invokes `arbor_application_invoke()`, validates the response, serializes it into the output buffer, and gives the Assembly transport the keep-alive decision. Request input and request-arena bytes remain live until the output has been fully written, which allows response bodies to borrow request-lifetime storage safely.

The rich transport validates that input/output/arena backing regions are representable and mutually disjoint, that the `arbor_runtime_storage` metadata does not overlap those backings, and that machine-detectable application-context/completed-count aliases cannot be corrupted by transport mutation. A response body must not overlap either the output-buffer metadata object or its backing region.

## MVC route catalog

An `arbor_mvc_catalog` is immutable and borrowed for the lifetime of an `arbor_mvc_application`. HTTP methods match exactly and case-sensitively. Path pattern semantics are delegated to the existing qualified `arbor_route_match()` bridge rather than being reimplemented in MVC.

Routes are scanned in catalog order. The first method+pattern match wins. Exact duplicate method/pattern entries are rejected during prepare, while distinct overlapping patterns are allowed and resolved deterministically by catalog order.

Route parameter arrays are request-lifetime storage allocated from the AF1 request arena. MVC validates the parameter spans before publishing an `arbor_mvc_request`.

## Prepare and validation

`arbor_mvc_catalog_measure()` reports a conservative caller-owned scratch requirement. `arbor_mvc_application_prepare()` uses caller-provided `arbor_mvc_prepare_workspace` only during preparation and does not retain it. It validates middleware references, rejects duplicate middleware indices within a route, rejects exact duplicate routes, invokes the qualified pattern matcher to establish each pattern's maximum parameter count, and publishes the application transactionally only after all validation succeeds. Workspace metadata is not allowed to alias its own parameter scratch. The prepared application stores the bitwise complement of its exact maximum parameter count as a lightweight integrity guard; explicit validation and capability construction remain deep checks, while the per-request hot path checks only the prepared application header/guard and relies on the documented immutability of the catalog.

## Middleware

A route names middleware by immutable catalog indices. `before` callbacks run in listed order. A successful `CONTINUE` enters the middleware and proceeds. A successful `RESPOND` publishes a validated response candidate, enters that middleware, skips later `before` callbacks and the controller/presenter, then runs `after` callbacks for entered middleware in reverse order.

`after` callbacks receive the current response and may replace it with another validated response. A mechanism failure aborts immediately; `after` is not a resource-cleanup guarantee. Every middleware callback is therefore required to be failure-atomic.

## Controller and presenter

The controller is the HTTP/MVC boundary. It reads the MVC request and route parameters and normally calls typed AF3 services through bindings cached in its caller-owned controller context. The controller returns an `arbor_mvc_controller_result`, which is a route-specific presentation model—not a generic business-service ABI.

The presenter maps that route-specific result to `arbor_response_plan`. The framework rejects machine-detectable attempts for presenter/middleware response bodies to escape transient framework stack objects. Callback-owned automatic storage cannot be identified portably after a callback returns, so controllers, presenters, and middleware must never publish model/body pointers whose lifetime ends with the callback. Controller model storage must remain valid through the paired presenter call; response-body storage must remain valid through synchronous transport serialization. Body bytes may instead borrow request input, request arena, immutable/static storage, or another explicitly longer-lived region.

MVC0 never introduces a generic `execute(void *, void *, void *)` data plane. AF3 remains the typed Application-service boundary.

## AF3 and AF4 composition

MVC0 does not wrap AF2, AF3, or AF4. A composition root resolves an AF3 service binding once and stores it in the controller context. Repeated requests use that cached typed binding. AF4 transaction, port, and domain-event APIs are used by the typed service/use-case (or explicit controller code where appropriate), not hidden behind MVC abstractions.

The integration qualification constructs an AF2 catalog, prepares an AF3 service that resolves an AF4 transaction capability, caches the ready service binding in the MVC controller context, executes an AF4 Unit of Work and event append/commit, then presents the typed service result. It also covers commit failure without publishing a successful response.

## Public MVC0 surface

MVC0 deliberately exposes only eight functions:

- `arbor_application_server_step`
- `arbor_mvc_catalog_measure`
- `arbor_mvc_application_prepare`
- `arbor_mvc_application_validate`
- `arbor_mvc_application_capabilities_make`
- `arbor_mvc_request_validate`
- `arbor_mvc_controller_result_validate`
- `arbor_mvc_middleware_before_result_validate`

The rich Assembly entry and its callback ABI are MVC-layer internals, not Assembly ABI v1 additions.

## Qualification focus

MVC0 is accepted only if it proves the whole application path, not merely isolated functions. Required evidence includes deterministic route/middleware/controller/presenter behavior, real NASM callback/transport ABI checks, fragmented request reads, partial/nonblocking writes with `EAGAIN` resume, keep-alive pipelining, close-after-body, rich-vs-legacy status-only serialization parity, legacy server regression, AF2→AF3 cached service→AF4 Unit-of-Work integration, a non-empty real socket response, sanitizer/static-analysis coverage, lower-layer byte identity, and deterministic source reproducibility.

## Deferred presentation work

MVC0 intentionally stops at `arbor_response_plan`. Templates, standard HTML/CSS view composition, and the optional precision-rendering surface belong to the next presentation-layer phase. MariaDB, R, deployment, and observability remain outside MVC0.
