# COUNTER1

COUNTER1 is the APP1 bounded-context repository example. It composes the frozen AF2 capability catalog, AF3 service lifecycle, AF4 Unit of Work, MVC0, HTTP1, VIEW0 T1, HOST1, and CONFIG0 without changing those framework layers.

Build it from the Arborcore repository root:

```bash
make counter1-application
```

Run with the legacy positional interface:

```bash
./build/counter1/counter1 examples/counter1/page.html 8080
```

or with the CONFIG0 named interface:

```bash
./build/counter1/counter1 \
  --template=examples/counter1/page.html \
  --bind-ipv4=127.0.0.1 \
  --port=8080
```

`--port=0` is valid and lets the kernel choose an available port. The process prints a line of the form:

```text
COUNTER1_READY=http://127.0.0.1:PORT/counter/1
```

The application starts with counters `1=0`, `2=41`, and `3=UINT64_MAX`.

HTTP routes:

```text
GET  /counter/:id
POST /counter/:id/increment
```

Examples:

```bash
curl -i http://127.0.0.1:8080/counter/1
curl -i -X POST http://127.0.0.1:8080/counter/1/increment
curl -i http://127.0.0.1:8080/counter/1
```

Expected semantics include HTTP 400 for a malformed numeric id without a service call, HTTP 404 for an absent counter, HTTP 409 when incrementing counter `3`, and HTTP 200 HTML for successful reads/increments.

Run the focused APP1 qualification with:

```bash
make counter1-focused-gate
```

Run the real loopback oracle alone with:

```bash
make counter1-live-verify
```

The in-memory provider is intentionally application-local and bounded. It is a first provider for the typed COUNTER1 repository port, not a database abstraction, generic CRUD API, cache, or persistent-storage claim. PROFILE0 is the next milestone after APP1 freeze.
