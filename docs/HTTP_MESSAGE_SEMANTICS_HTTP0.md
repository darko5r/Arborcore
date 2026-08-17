# Arborcore HTTP0 — HTTP Message Metadata and Final-Response Semantics

HTTP0 is the first HTTP-specific presentation/transport semantics layer above the frozen low-level request parser and the completed MVC0 core. Its purpose is deliberately narrow: expose safe zero-copy request fields, define HTTP/1.1 Host and connection persistence semantics, and serialize final HTTP responses with ordered application field lines and correct method/status body rules.

## Boundary

HTTP0 is **not** a replacement HTTP server and does not reopen the frozen Assembly ABI v1. The existing `arbor_asm_http_request` already carries a borrowed `headers_ptr` / `headers_len` span. HTTP0 consumes that span without copying. The frozen `http_parse_request` and legacy `http_response_serialize` remain byte-exact. HTTP0 adds separate internal Assembly routines and a public C API in `include/arborcore/http.h`.

AF0–AF4 and MVC0 also remain byte-exact. HTTP0 does not add headers to generic AF1 `arbor_response_plan`, and it does not create a second router/controller/middleware pipeline. HTTP-specific integration with the existing MVC0 presenter boundary is deferred to HTTP1.

## Request field-line model

`arbor_http_request_header_next()` walks the parser-owned header section in wire order. Returned `arbor_http_field` names and values are borrowed spans into request input storage. Field-name matching helpers are ASCII case-insensitive, while original spelling and duplicate field lines are preserved. No unordered string map is introduced. Public header-query outputs are not allowed to overlap the borrowed request object or request-byte spans, so querying metadata cannot corrupt the immutable request view.

The scanner validates token field names, CRLF field-line framing and safe field values. OWS around field values is removed from the borrowed value view. The frozen parser remains framing authority: HTTP0 does not reinterpret Content-Length or Transfer-Encoding.

`Host` validation requires exactly one Host field for the request-target forms Arborcore currently accepts (origin-form and asterisk-form). The value is checked as RFC 3986 `uri-host` plus optional port, including IPv6 and IPvFuture literals. HTTP0 does not accept the former RFC 6874 `%25ZoneID` URI extension: RFC 9844 obsoleted RFC 6874 and reverted that extension to RFC 3986 URI syntax. Absolute-form and authority-form remain outside the frozen request-target layer.

`Connection` is interpreted as a comma-separated token list across every Connection field line. A case-insensitive `close` token forces close after the current response. Application code can also request close, but cannot force persistence against request-side close. If Connection is semantically malformed, `arbor_http_request_connection_close()` reports `-EINVAL`; response serialization nevertheless fails closed to `Connection: close` so HTTP1 can still serialize a 400/error response.

## Response model

`arbor_http_response` ABI v1 has a 56-byte x86-64 prefix: ABI version, structure size, flags, final status, ordered field array/count, and borrowed body pointer/length. Validation accepts a structure size at least as large as the v1 prefix for prefix-compatible extension; the constructor publishes the current prefix size. Alias validation treats the complete caller-declared `struct_size` as response-owned storage, so an extended tail cannot be overwritten by the output buffer while older HTTP0 code is consuming only the known prefix.

Application field lines are caller-owned and borrowed through synchronous serialization. Duplicate lines remain distinct, including repeated `Set-Cookie`. HTTP0 rejects application control of `Content-Length`, `Connection`, `Transfer-Encoding`, `Trailer` and `Upgrade`. Field names use token grammar; field values reject CR, LF, NUL and other disallowed controls.

## Final status and body semantics

HTTP0 handles final status codes 200–599. Known common codes get conventional reason phrases; an unknown final code uses a syntactically valid empty reason phrase. Interim 1xx responses, 101 switching protocols and CONNECT tunneling are deferred. Until tunneling exists, a 2xx response to CONNECT is rejected rather than serialized with ordinary message framing.

- HEAD: serialize no body bytes; Content-Length equals the supplied hypothetical representation body length.
- 204: body length must be zero; no Content-Length.
- 205: body length must be zero; `Content-Length: 0`.
- 304: body length must be zero; HTTP0 does not automatically emit Content-Length.
- other final responses: automatic Content-Length from body length.

Chunked response encoding and trailers are deferred. Persistent HTTP/1.1 responses omit `Connection`; closing responses emit `Connection: close`.

## Ownership, failure and security

HTTP0 allocates no hidden heap storage and owns no mutable process-global registry. Response arrays, names, values and bodies are borrowed. The response constructor rejects publishing its output object over any of those borrowed source regions. Serializer result/control outputs are likewise forbidden from aliasing the request, response sources, output buffer object or serialized backing storage. The public serializer rejects response/body/field source regions that overlap output backing storage. This conservative HTTP0 rule keeps capacity preflight and transactional serialization simple; view/template output will use request arena or other separate caller-owned storage.

Validation and capacity failures occur before serialization starts: output logical length and backing bytes remain unchanged and `bytes_written` remains zero. The internal serializer preflights the complete output length. If a lower append unexpectedly fails after that successful preflight, HTTP0 restores the original logical buffer length; bytes written beyond that original logical end are outside the buffer's published contents and are not promised byte-for-byte restored.

## Performance policy

HTTP0 adds a representative response-serialization microbenchmark. This phase records a diagnostic median only; no threshold is invented from a single first measurement.

## Next phases

HTTP0 stops before changing MVC0. HTTP1 must reuse the **existing** MVC0 route → middleware → controller → presenter and AF3/AF4 machinery without creating a second router or service pipeline. The final HTTP1 design must, however, establish an explicit per-request presentation metadata channel for dynamic HTTP fields such as `Content-Type`, `Location`, cookies and cache metadata; generic AF1 `arbor_response_plan` does not carry those fields. A minimal controlled presentation-boundary extension is therefore allowed if the HTTP1 review proves it necessary, but the MVC0 routing/controller/service core is not to be duplicated.

HTTP1 must also account for origin-server obligations that are deliberately outside standalone HTTP0: a clocked origin server needs framework-level `Date` generation where the HTTP standard requires it, and full HTTP/1.1 request-target compliance eventually requires absolute-form acceptance even though the currently frozen target layer is scoped to the forms used by Arborcore's direct-origin path. These are explicit future obligations, not silent HTTP0 claims.

After HTTP1, VIEW0 adds standard HTML/CSS views/templates with safe escaping. HELLO0 then builds the first tiny real Arborcore application (`GET /hello` → HTML "Hello World") and exercises 404, redirect/Location, escaping, keep-alive and Connection-close behavior.

MariaDB, R and other complex infrastructure remain deferred until HELLO0 works and the framework is reviewed as an actual application-development experience.
