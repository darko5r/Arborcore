# COUNTER1 — APP1-R0

COUNTER1 is Arborcore's second real application milestone after HELLO0/ECHO0 and the first application in this lineage whose primary purpose is to exercise the frozen AF2 capability kernel, AF3 service runtime, and AF4 bounded-context repository/Unit-of-Work model end to end over MVC0, HTTP1, VIEW0, HOST1, and CONFIG0.

COUNTER1 is an example-local bounded context. It adds no public Arborcore framework API and changes no frozen Assembly, AF2, AF3, AF4, MVC0, HTTP0, HTTP1, VIEW0, HOST1, or CONFIG0 production implementation.

## Bounded-context model

The application starts with exactly three in-memory records:

- counter `1` = `0`
- counter `2` = `41`
- counter `3` = `UINT64_MAX`

The bounded-context repository port has two operations: typed get and typed increment. It is not a generic CRUD repository. The first provider is fixed-capacity, composition-root-owned in-memory storage. It contains no hidden heap, thread, lock, database vendor, SQL, cache, IMAGE0, Nonane, or ASYNC0 implementation.

The provider exports two AF2 capabilities: `COUNTER1_REPOSITORY_V1` and the standard AF4 transaction interface. The service module exports one AF3 typed service capability and consumes those two provider capabilities. AF3 preparation resolves and caches exactly two bindings. Normal service calls perform zero AF2 catalog lookup.

## Transaction semantics

Every service call owns an AF4 Unit of Work in request-arena storage.

A get call begins a UoW, obtains its active transaction view, invokes the repository, then commits before publishing either `FOUND` or `NOT_FOUND`. It emits no events.

A successful increment stages at most one mutation in transaction state. Provider state is unchanged before commit. The service appends exactly one `COUNTER_INCREMENTED_V1` event carrying the counter id and new value, commits, and only then publishes `INCREMENTED`.

An increment `NOT_FOUND` or `LIMIT_REACHED` outcome rolls back before publication and retains zero events. A required commit or rollback failure suppresses the typed business outcome and returns the negative mechanism status. Commit failure leaves provider state unchanged.

## HTTP surface

The application has exactly two routes and no middleware:

- `GET /counter/:id`
- `POST /counter/:id/increment`

Route ids use full-consumption ASCII unsigned-decimal syntax. A malformed id is a controller-level invalid request: the service is not called, the response is HTTP 400, and the body is empty.

Typed not-found maps to HTTP 404 with an empty body. Increment at `UINT64_MAX` maps to HTTP 409 with an empty body. Successful reads/increments map to HTTP 200 with `Content-Type: text/html; charset=utf-8` and the prepared VIEW0 T1 template in `examples/counter1/page.html`.

The canonical successful page is:

```html
<!doctype html>
<html><head><meta charset="utf-8"><title>COUNTER1</title></head><body><p>Counter {{id}} = {{value}}</p></body></html>
```

The file includes one final LF byte.

## Lifetime and host composition

CONFIG0 retains the exact six deployment keys already used by HELLO0/ECHO0: `template`, `bind_ipv4`, `port`, `backlog`, `event_wait_ms`, and `drain_timeout_ms`. Configuration resolves before template/host resource publication.

The COUNTER1 provider, AF2 metadata, AF3 module state/runtime, MVC graph, and prepared template are application-lifetime composition-root state and are not moved after preparation. HOST1 owns no COUNTER1 repository state.

On normal shutdown, HOST1 drains and closes first; AF3 stops only after host quiescence. If bootstrap fails after AF3 becomes ready, AF3 is stopped before the process returns.

## Qualification

`counter1-focused-gate` covers strict C17/GCC analyzer checks, repository/transaction core tests, adversarial commit/rollback failure injection, AF2→AF3→AF4→MVC→VIEW integration, HOST1 lifecycle, ASan/UBSan, the tracked contract, boundary scans, the real loopback HTTP oracle, deterministic source-archive reproduction, and an advisory repeated-GET measurement.

`app1-gate` adds the cumulative frozen CONFIG0, VIEW0 D1, and C-runtime gates. The repeated-GET benchmark is diagnostic only and defines no universal performance threshold.

After APP1 is independently frozen, the next milestone is PROFILE0. IMAGE0, CACHE0, NONANE0, four-mode qualification, and the later database/async track remain separate later milestones.
