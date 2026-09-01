# Arborcore HELLO0

HELLO0 is the first complete standalone Arborcore web application. It composes
the existing Linux listener and epoll runtime, HTTP1 adapter, MVC0,
application-foundation response plans, HTTP0 serialization, and VIEW0 T1
template rendering. It does not implement a parallel HTTP stack.

## Behavior

| Request | Owner | Result |
|---|---|---|
| GET /hello | HELLO0 page route | 200, UTF-8 HTML |
| GET / | HELLO0 redirect route | 302, Location: /hello, empty body |
| Any other route | MVC0 fallback | 404, empty body |

The middleware appends Cache-Control: no-store to matched routes. The page
presenter appends Content-Type: text/html; charset=utf-8; the redirect
presenter appends Location: /hello. HTTP0 owns Content-Length and Connection.

The page model contains:

~~~text
Arborcore safely renders <dynamic data> & UTF-8: Olá 😀
~~~

VIEW0 publishes it in ordinary HTML text context as:

~~~html
Arborcore safely renders &lt;dynamic data&gt; &amp; UTF-8: Olá 😀
~~~

## Composition

1. main.c reads at most 4096 template bytes, prepares the application, opens
   a loopback-only listener, accepts into eight fixed connection slots, and
   advances every ready connection with arbor_http_mvc_server_step().
2. web.c prepares the persistent template backing, middleware, two routes,
   MVC application, and HTTP/MVC adapter in their final address.
3. The controller asks the typed service in application.c for a domain
   outcome. A page model is allocated in the request arena; a redirect has no
   model.
4. The presenter maps the typed outcome to 200 or 302. For a page it renders
   the prepared template transactionally, validates UTF-8, and publishes the
   borrowed body in an arbor_response_plan.
5. HTTP1 combines the response plan with the response-field sidecar. HTTP0
   performs final protocol validation, framing, and keep-alive/close handling.

The template source and field-name bytes are preparation-only borrows. VIEW0
copies trusted literals and resolves the field slot into the arrays owned by
hello0_web_application, so main.c clears the original source buffer
immediately after successful preparation.

There is no database, heap-owned application state, new Arborcore public API,
or new Assembly symbol in HELLO0.

## Build and run

From the repository root:

~~~sh
make hello0-application
./build/hello0/hello0 examples/hello0/page.html 8080
~~~

Open:

~~~text
http://127.0.0.1:8080/hello
~~~

The process binds only 127.0.0.1. Ctrl-C and SIGTERM perform a graceful
shutdown. Port 0 asks Linux for an unused ephemeral port and is used by the
automated live check.

## Evidence

Run all HELLO0 evidence:

~~~sh
make hello0-gate
~~~

Or run each level separately:

~~~sh
make hello0-core-test
make hello0-integration-test
make hello0-sanitize
make hello0-live-verify
make hello0-route-scale-benchmark
~~~

The route-scale program reports the median of nine rounds for first match,
last match, miss, and quiescent server-step cases at 2, 16, 64, and 256
routes. It is a diagnostic baseline. It deliberately has no numeric
performance threshold until repeated measurements establish a defensible
budget.
