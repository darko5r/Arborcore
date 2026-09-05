# PROFILE0 — explicit low-overhead profiling substrate

PROFILE0 is Arborcore's first explicit profiling boundary. It is deliberately
small: callers prepare a fixed session containing immutable nonzero region IDs
and caller-owned aggregate storage, then bracket selected work with caller-owned
span tokens.

PROFILE0 is not a sampling profiler, tracing backend, logging system, telemetry
exporter, or automatic MVC/HTTP/HOST instrumentation layer. The unprofiled path
contains zero PROFILE0 calls.

## Clock contract

The core never acquires time from the operating system. A caller supplies:

```c
int64_t (*arbor_profile_clock_fn)(void *context);
```

A nonnegative return is a monotonic timestamp in nanoseconds. A negative return
is a native mechanism failure and is propagated unchanged. Timestamp zero is
valid. A finish timestamp smaller than the span's start fails with `-ERANGE`.

A successful span performs exactly two clock calls: one in begin and one in end.

## Storage and ownership

Descriptors, aggregates, sessions, span tokens, the clock callback, and clock
context remain caller-owned. PROFILE0 allocates no heap memory, creates no
threads, uses no TLS, locks, registries, reference counts, file/network export,
or mutable global state.

The session, descriptor array, and aggregate array are pairwise disjoint known
ranges. Descriptor IDs are fixed, nonzero, and unique. The session's final
address is part of its prepared identity and the session may not move after
prepare.

An active span token also has final-address identity. Its first use requires all
zero bytes. Successful end returns it to the same all-zero canonical form.
Copying or moving an active token is prohibited and a copied token fails
validation at the different address. Distinct tokens may nest, including for
the same region.

## Aggregates

Each region stores only:

- `sample_count`
- `total_ns`
- `min_ns`
- `max_ns`

The first successful sample sets both minimum and maximum to its duration.
Subsequent samples update minimum and maximum across all successful durations.
Count and total use checked unsigned arithmetic. A clock failure, clock
regression, or arithmetic overflow leaves both the aggregate and active token
byte-exact unchanged so the caller may retry after correcting the mechanism or
test fixture.

`arbor_profile_region_get()` copies one region ID plus its current aggregate to
caller output. The output must not overlap the session, descriptor array, or
aggregate array.

## COUNTER1 diagnostic

The R0 diagnostic wraps the already-frozen COUNTER1 service GET call without
editing COUNTER1 source. It preserves the existing 1,000-operation warm-up and
200,000-operation measured-run precedent. A benchmark-owned
`clock_gettime(CLOCK_MONOTONIC)` adapter supplies PROFILE0 timestamps.

The benchmark reports baseline and profiled totals/ns-per-operation plus the
PROFILE0 aggregate. This measurement is advisory; there is no universal
performance threshold.

## Roadmap boundary

PROFILE0 does not reopen AF2, AF3, AF4, MVC0, HTTP0/HTTP1, VIEW0, HOST1,
CONFIG0, COUNTER1, or Assembly ABI v1. It adds no configuration keys and no
database, IMAGE0, CACHE0, NONANE0, or ASYNC0 implementation. After PROFILE0 is
independently frozen, the next milestone is IMAGE0.
