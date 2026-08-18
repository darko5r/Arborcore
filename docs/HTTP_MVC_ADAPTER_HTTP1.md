# Arborcore HTTP1 — HTTP / MVC presentation adapter

HTTP1 connects the frozen MVC0 application pipeline to the frozen HTTP0 message
semantics without creating a second router, middleware chain, controller graph or
socket state machine.

## Controlled AF1 semantic retrofit

AF1 deliberately deferred broader final statuses and redirects to a later explicit
response-contract phase. HTTP1 exercises that controlled-reopen policy. The stable
32-byte `arbor_response_plan` and capability-table layouts do not change. AF1 now
accepts final statuses 200-599, with 204, 205 and 304 requiring an empty body.
`arbor_response_plan_serialize()` remains the legacy six-status serializer and
rejects broader HTTP1-only statuses; HTTP1 sends those responses through HTTP0.

## Request-local HTTP field sidecar

MVC0 callbacks do not gain an HTTP-specific parameter. Instead HTTP1 reserves a
bounded prefix in the transport request arena. The prefix contains the exchange,
an ordered `arbor_http_field` array, and a versioned guarded locator. MVC0 receives
a separate application sub-arena whose backing storage begins after the prefix.
Application `arena_reset()` or rewind operations therefore cannot overwrite the
HTTP1 sidecar. The measured prefix includes seven bytes of worst-case alignment
slack; HTTP1 aligns the internal exchange and application boundary to 8 bytes at
dispatch time, preserving the lower arena contract for unaligned caller backing.

`arbor_http_mvc_response_field_append()`, mark and rewind are valid only during
the synchronous MVC callback invocation reached from `arbor_http_mvc_server_step()`.
HTTP1 binds the sidecar to the exact AF1 `scope.request` object after
`arbor_request_scope_make()` succeeds; the request/scope/sidecar pointers are
callback-lifetime borrows and must not escape that nested invocation. Field names
and values are borrowed and must outlive synchronous HTTP0 serialization.
Duplicate field lines and append order are preserved, including repeated
`Set-Cookie`. HTTP0 remains authoritative for syntax and reserved framing-field
rejection. Output parameters are not allowed to overwrite the sidecar, application
sub-arena, prepared HTTP1 application, prepared MVC application/catalog, or
borrowed request bytes.

## Transport composition

HTTP1 reuses MVC0's private `application_transport_handle_once` Assembly state
machine. The HTTP1 C callback validates Host and Connection before invoking MVC.
Invalid Host or malformed Connection produces a 400 response and forces close
without entering MVC. For a valid request, HTTP1 creates the sidecar/sub-arena,
invokes the prepared MVC0 AF1 capability, builds `arbor_http_response` from the
MVC0 status/body plus sidecar fields, and serializes through HTTP0. Request-side
`Connection: close` always wins; AF1 keep-alive can allow persistence but cannot
force it against request policy. MVC0 method matching remains exact: HTTP1 does not
silently map `HEAD` to a `GET` route. An application that wants a HEAD route in
HTTP1 declares it explicitly; HTTP0 still supplies the qualified no-body framing.

## Qualification/performance note

`arbor_http_mvc_server_step()` retains deep catalog-overlap checks for transport
storage and completion-output alias safety. Those checks are linear in the route
catalog and can also run on resumable transport steps. The current HTTP1 benchmark
is a diagnostic prepared-application validation baseline, not a route-scaling or
socket-step benchmark. HTTP1 keeps the safety checks for this freeze; route-count
scaling is a nonblocking measurement item for HELLO0/framework-ergonomics review
before any optimization is considered.

## Deferred protocol obligations

HTTP1 intentionally does not claim production-origin completeness. A clocked
origin-server Date policy remains DATE1 work, and absolute-form request-target
support remains TARGET1 work before a complete production HTTP/1.1 claim.
Interim 1xx, CONNECT tunneling, 101 upgrade, chunked response encoding and trailers
remain explicitly deferred.

## Next sequence

After HTTP1 is frozen, Arborcore builds VIEW0/templates with standard HTML/CSS as
a first-class path. Then HELLO0 exercises a real `/hello` application, redirects,
escaping, 404, keep-alive and close behavior. MariaDB, R and other complex
infrastructure remain deferred until that real application is working and reviewed.

## HTTP1 source-review hardening R4-R5

R4 repaired the request-identity anchor used by the sidecar field APIs. AF1 scope
construction borrows the `request_view.native` copy, so HTTP1 binds
`exchange->request` to `scope.request` after scope construction and before MVC0
invocation while preserving the pointer-identity guard.

R5 closes output-alias coverage for `arbor_http_mvc_response_fields_mark()`.
The sidecar now retains a prepared HTTP1-application anchor, and mark output is
rejected when it overlaps that application, the prepared MVC application, or
immutable MVC catalog storage. The private exchange therefore grows by one
pointer (eight bytes on the qualified x86-64 target); callers continue to obtain
the required request-arena prefix from `arbor_http_mvc_application_measure()`
rather than hard-coding it. No public C ABI layout changes in R5.
