# Arborcore HOST1 — public Linux HTTP/MVC host

HOST1 promotes the qualified LIFE0-R0 host from example-private code to one
public, Linux-specific C source contract. It is a source-level promotion, not a
new transport or lifecycle design. The accepted connection path still advances
only through `arbor_http_mvc_server_step()` and therefore preserves the frozen
HTTP1, MVC0, HTTP0, VIEW0, runtime, and Assembly behavior.

The public surface is deliberately named for its platform:

~~~c
#include <arborcore/linux_http_mvc_host.h>
~~~

It currently targets x86-64 Linux with epoll. HOST1-R0 records exact GNU C
layout facts for qualification, but Arborcore pre-1.0 makes no stable C binary
ABI promise. There is no generic cross-platform host alias.

## Build and link

The sole production engine is `src/c/linux_http_mvc_host.c`. The Makefile
compiles it once and deterministically archives the one object as:

~~~text
build/libarborcore_host1.a
~~~

HELLO0, ECHO0, their host tests, both LIFE0 tests, the LIFE0 shutdown
diagnostic, and the public consumer test all link that archive. No consumer
compiles or copies the production source. The former example-local host
files and namespace are removed without wrappers or aliases. HOST1 symbols are
not added to the Assembly archive, shared object, or `ARBORCORE_1.0` version
map.

Run the complete gate from the repository root:

~~~sh
make host1-gate
~~~

The smaller public include/link check is:

~~~sh
make host1-public-consumer-test
~~~

## Explicit options

`arbor_linux_http_mvc_host_options_make()` publishes version 1, the exact
current structure size, zero known flags, and every caller-supplied value. It
accepts event waits from zero through `INT_MAX` and every `uint64_t` drain
timeout. Invalid input returns `-EINVAL` without changing the destination.

Preparation accepts only version 1, the exact current structure size, and no
unknown flag bits. It copies the values; it never retains the options address.
The options object therefore needs to live only through the prepare call.
Callback contexts are independent opaque borrows and may be null. A null clock
selects `event_monotonic_ms()`; a custom clock returns nonnegative monotonic
milliseconds or a negative mechanism status.

Minimal preparation has this shape:

~~~c
arbor_linux_http_mvc_host_options options;
arbor_status status = arbor_linux_http_mvc_host_options_make(
    &options,
    250,
    UINT64_C(2000),
    NULL,
    NULL,
    diagnostic,
    diagnostic_context);
if (status.native != 0) {
    return status;
}

status = arbor_linux_http_mvc_host_prepare(
    &host,
    &application,
    slots,
    slot_count,
    events,
    event_capacity,
    &options);
~~~

The caller separately prepares every slot with caller-owned input, output, and
arena spans before preparing the host.

## Lifecycle

One nonzero phase field is authoritative:

~~~text
PREPARED -> ACCEPTING -> DRAINING -> CLOSED
~~~

An all-zero host is invalid. Prepare is failure-atomic, open is
resource-transactional, and a closed host cannot reopen. Beginning drain first
changes phase and closes the listener, then fixes the participant count, start
time, and checked deadline exactly once. Repeated drain requests do not extend
the deadline.

During `DRAINING`, existing connections retain normal keep-alive, fragmented
request, pipelining, request-budget, response-backpressure, `MORE_WORK`,
`EAGAIN`, and rearm behavior. New accepts are prohibited. An active slot with
immediate work selects a zero event wait; otherwise the wait is the smaller of
the ordinary nonnegative wait and remaining deadline.

Natural completion and deadline enforcement both converge to `CLOSED`.
Deadline expiry is recorded as policy data, not turned into a negative
mechanism status. On a normally completed drain:

~~~text
inactive_before_deadline + forced_at_deadline = active_at_drain_start
~~~

Completion-oriented teardown attempts all reachable cleanup and retains the
first negative mechanism failure. `shutdown_result_get()` validates the host
and destination before publishing the complete result and is valid only after
`CLOSED`.

## Ownership and serialization

The host, slot array, epoll event array, connection buffers, arenas,
applications, callback contexts, and result destination are caller-owned.
Prepared pointers are stable borrows through `CLOSED`. The production engine
performs no heap allocation, implicit resize, reference counting, ownership
transfer, lock acquisition, or worker creation.

The LIFE0 disjointness rules remain mandatory across host metadata, slots,
events, every slot backing region, the HTTP/MVC application graph, catalog,
routes, middleware descriptors, names, patterns, and index arrays. Range and
counter arithmetic remains checked. State-changing operations are externally
serialized; HOST1 makes no thread-safety claim. Callbacks are synchronous and
observational.

## Diagnostics and application control

The seven diagnostic values remain listen, event-loop creation, event-loop
operation, accept, connection advancement, clock, and close. A diagnostic
cannot alter host authority.

HOST1 installs no signal handler and owns no wake descriptor. HELLO0 and ECHO0
continue to own SIGINT, SIGTERM, SIGPIPE, and their `sig_atomic_t` stop state.
The stop callback is observed at the host loop boundary. `open()` receives
explicit sockaddr bytes and backlog; HOST1 performs no name resolution and
reads no environment variable, command-line option, configuration file, or
deployment profile.

## Deliberate exclusions and roadmap

HOST1 changes no Assembly source or ABI, framework HTTP behavior, MVC or VIEW
logic, application logic, database boundary, TLS/protocol surface, or event
backend. It adds no persistent dependency.

CONFIG0 begins only after HOST1 is independently frozen, published, and
post-publication reviewed. IMAGE0 and Nonane remain independent optional work
after CONFIG0 and must compose in all four modes: both off, IMAGE0 only, Nonane
only, and both on. Nonane means resident prepared application/runtime reuse; it
never means response caching.
