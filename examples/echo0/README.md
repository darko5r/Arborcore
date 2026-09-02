# Arborcore ECHO0

ECHO0 is Arborcore's second complete standalone web application and the first
to prove parameterized routing while reusing the unchanged private Linux host.
It composes the existing listener and epoll runtime, HTTP1 adapter, MVC0,
application-foundation response plans, HTTP0 serialization, and VIEW0 T1
template rendering. It does not implement a parallel HTTP stack.

## Behavior

| Request | Owner | Result |
|---|---|---|
| GET /echo/:value | ECHO0 page route | 200, escaped UTF-8 HTML |
| GET / | ECHO0 redirect route | 302, Location: /echo/Arborcore, empty body |
| Any other route | MVC0 fallback | 404, empty body |

The middleware appends Cache-Control: no-store to matched routes. The page
presenter appends Content-Type: text/html; charset=utf-8; the redirect
presenter appends Location: /echo/Arborcore. HTTP0 owns Content-Length,
Connection, reason phrases, HEAD semantics, and final wire serialization.

At the direct MVC/VIEW boundary, a routed span containing:

~~~text
A&B-Olá😀
~~~

VIEW0 publishes the borrowed request value in ordinary HTML text context as:

~~~html
A&amp;B-Olá😀
~~~

On the live HTTP path, clients percent-encode non-ASCII request-target bytes.
ECHO0 intentionally does not decode them, so the live value
`A&B-Ol%C3%A1` renders as `A&amp;B-Ol%C3%A1`. The direct core evidence separately
proves that admitted multibyte UTF-8 remains intact while HTML-sensitive bytes
are escaped.

## Composition

1. main.c reads at most 4096 template bytes, prepares the application and
   supplies caller-owned fixed connection, event and buffer storage.
2. examples/common/linux_http_mvc_host.c is the frozen private HOST0-R0 source. It
   opens the loopback listener and epoll instance, applies eight-slot accept
   backpressure, and advances every ready connection with
   arbor_http_mvc_server_step().
3. web.c prepares the persistent template backing, middleware, two routes,
   MVC application, and HTTP/MVC adapter in their final address.
4. The page controller requires exactly one route parameter named `value`.
   It asks the typed service in application.c for an outcome, then allocates a
   page model in the request arena. The model span borrows the route-value
   bytes from the stable request input through synchronous presentation. The
   redirect controller requires zero route parameters and publishes no model.
5. The presenter maps the typed outcome to 200 or 302. For a page it renders
   the prepared template transactionally, validates UTF-8, and publishes the
   borrowed body in an arbor_response_plan.
6. HTTP1 combines the response plan with the response-field sidecar. HTTP0
   performs final protocol validation, framing, and keep-alive/close handling.

The template source and field-name bytes are preparation-only borrows. VIEW0
copies trusted literals and resolves the field slot into the arrays owned by
echo0_web_application, so main.c clears the original source buffer
immediately after successful preparation.

ECHO0 performs no URL decoding or normalization: `:value` is pointer-length
metadata, not a C string. HOST0-R0 remains private to the examples while LIFE0
establishes the reusable lifecycle boundary. There is no database, heap-owned
application or host state, new Arborcore public API, or new Assembly symbol.

## Build and run

From the repository root:

~~~sh
make echo0-application
./build/echo0/echo0 examples/echo0/page.html 8080
~~~

Open:

~~~text
http://127.0.0.1:8080/echo/Arborcore
~~~

The process binds only 127.0.0.1. Ctrl-C and SIGTERM request controlled
shutdown; R0 closes remaining active slots after the event loop stops. A
deadline-governed connection drain belongs to the later LIFE0 milestone. Port
0 asks Linux for an unused ephemeral port and is used by the automated live
check.

## Evidence

Run all ECHO0 evidence:

~~~sh
make echo0-gate
~~~

Or run each level separately:

~~~sh
make echo0-core-test
make echo0-integration-test
make echo0-host-test
make echo0-sanitize
make echo0-live-verify
make echo0-route-scale-benchmark
~~~

The route-scale program reports the median of nine rounds for first match,
last match, miss, and quiescent server-step cases at 2, 16, 64, and 256
routes. It is a diagnostic baseline. It deliberately has no numeric
performance threshold until repeated measurements establish a defensible
budget.
