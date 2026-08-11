# Arborcore Assembly Core Audit

## Audit identity

- Audit branch: `assembly-core-audit`
- Audited commit: `523e7177aeb4699bfe7e1525218b08edc47574c5`
- Audited archive SHA-256: `55e87667aef8e47d62acfd1439ac9cd71261e71bb981c6e54762f75145ac5628`
- Production Assembly modules: 25
- Audited public/global production symbols: 101
- Production `.text` reference: 10,285 bytes
- Existing permanent correctness gates: Polish Gates #1, #2, and #3
- Local performance reference profile used during construction: `local-dev-i7-6700hq`

This audit is read-only with respect to `src/asm`. Its purpose is to define the mathematical contracts, identify genuine hardening requirements, distinguish them from optional performance experiments, and establish an ordered retrofit plan before the Assembly ABI is frozen.

## Decision vocabulary

- **KEEP** — current production algorithm/representation should remain; strengthen specification or qualification only.
- **HARDEN** — retain the architecture but repair or make explicit a correctness, ownership, lifetime, range, or state contract.
- **REFACTOR** — consolidate duplicated semantics or move a repeated rule into a canonical mathematical model without changing externally observable behavior unnecessarily.
- **REDESIGN** — a cross-layer property cannot be guaranteed cleanly by a local patch and needs an architectural change.
- **EXPERIMENT** — an alternative may improve performance or code size but is not justified for production until benchmarked against the reference implementation.

## Executive findings

The existing Assembly core is substantially stronger than a typical early low-level prototype. Checked arithmetic, fail-closed HTTP output, absolute-address arena alignment, bounded routing arithmetic, transactional response length rollback, explicit connection states, and nonblocking I/O semantics are already present.

The retrofit should therefore be selective. The main goals are:

1. define one canonical integer/range/state/time/resource model;
2. repair the small set of proven contract defects;
3. add exhaustive/property/self-certifying qualification where domains permit it;
4. preserve simple existing algorithms as reference implementations;
5. benchmark every optimization experiment against the pre-retrofit performance profile;
6. freeze the Assembly ABI only after the Assembly core and remaining server-core features are qualified.

## Module classification

| Module | Decision | Audit conclusion |
|---|---|---|
| `start.asm` | KEEP | Correct smoke entry point; later runtime startup lifecycle will supersede it. |
| `write.asm` | KEEP | Clean monotonic-progress loop; successful iterations strictly reduce remaining bytes. |
| `memory_threshold.asm` | KEEP | Correct piecewise copy-policy structure; threshold qualification tooling should later use stronger statistics/profile identity. |
| `memory.asm` | KEEP + qualification hardening | Strong overlap geometry, especially `memory_move`; add exhaustive small-domain overlap and copy-engine equivalence checks. |
| `ascii.asm` | KEEP | Entire byte domain is finite (256 values), enabling exhaustive qualification. |
| `bytes.asm` | KEEP + EXPERIMENT | Sequence semantics are sound; `bytes_find` remains the simple reference while faster search is experimental. |
| `bytes_scan.asm` | KEEP | Clean bounded scanning; add minimality/idempotence properties. |
| `parse_u64.asm` | KEEP | Correct recurrence-based decimal/hex parsing and overflow behavior. |
| `u64_checked.asm` | REFACTOR / EXTEND | Becomes the nucleus for canonical integer geometry and checked narrowing. |
| `u64_format.asm` | KEEP + EXPERIMENT | Correct bounded formatting; reciprocal-multiply decimal formatting may be benchmarked later. |
| `hex_codec.asm` | KEEP | Strong two-pass decode and exact inverse properties. |
| `percent_codec.asm` | REFACTOR | Unreserved-byte classification is semantically duplicated; canonicalize without forcing per-byte call overhead. |
| `base64.asm` | KEEP | Strict/canonical decoder and exact length algebra are strong. |
| `buffer.asm` | HARDEN / REFACTOR | `buffer_append` delegates to non-overlap `memory_copy` without a complete public alias contract. |
| `arena.asm` | KEEP + contract hardening | Absolute-address alignment is correct; formalize zero-size allocation and logical rewind semantics. |
| `io.asm` | KEEP + contract hardening | EINTR/EAGAIN behavior is sound; document idempotent nonblocking and consuming close ownership. |
| `net.asm` | HARDEN | Public 64-bit register ABI needs explicit semantics for narrower Linux syscall domains. |
| `event.asm` | HARDEN | Positive timeout retried after EINTR is not a strict total elapsed-time bound; define timeout/deadline semantics and narrow argument domains. |
| `connection.asm` | HARDEN + exhaustive qualification | Transition relation is good; unrestricted public `connection_reset_io` can corrupt write progress. |
| `http_parser.asm` | KEEP + qualification hardening | Strong bounded fail-closed parser; establish span containment and server-level complexity counters. |
| `request_target.asm` | HARDEN | Proven defect: bytes after the first `?` are returned as query without validating raw `#` or non-visible ASCII. |
| `router.asm` | KEEP + EXPERIMENT | Correct ordered exact reference router; static/prepared indexing must preserve precedence or reject ambiguity. |
| `route_pattern.asm` | HARDEN + EXPERIMENT | Segment matcher is sound; formalize capture validity/lifetime and route-shape ambiguity. |
| `http_response.asm` | HARDEN | Strong logical transaction/preflight; define body/destination alias semantics and later evaluate scatter/gather output. |
| `server.asm` | HARDEN + REDESIGN selected paths + EXPERIMENT | Composition exposes transactional accept defect, adversarial fragmented-request O(n²) reparsing, pipelined-work obligations, and epoll/syscall optimization opportunities. |

## Proven or required production hardening

### 1. Request-target query validation

`request_target_split` validates path bytes only until the first `?`, then publishes the remainder as query. The documented rule says raw `#` and non-visible ASCII are rejected, so the query must be validated as well.

Required contract for origin-form targets:

```text
target = path
```

or

```text
target = path || "?" || query
```

with `path` beginning `/` and every byte in both path and query in visible ASCII `0x21..0x7e`, excluding raw `#`. A second `?` inside query remains ordinary query data.

### 2. `buffer_append` alias semantics

The desired operation is concatenation:

```text
new = old || snapshot(source)
```

subject to capacity/representability. The current implementation uses `memory_copy`, whose contract excludes overlap, but `buffer_append` does not prove all legal source spans are non-overlapping with the append destination.

The construction phase must either:

- define a strict disjoint-source precondition and enforce/document it; or
- provide snapshot/overlap-safe semantics, preserving a fast non-overlap path.

The preferred API direction is overlap-safe snapshot semantics if benchmark cost is acceptable.

### 3. Public `connection_reset_io`

`connection_reset_io` can reset `write_bytes` while state is `WRITING`. The server uses that field as response-send progress. Resetting it mid-write could resend bytes.

Before ABI freeze, remove/make-internal this operation or restrict it to mathematically safe states.

### 4. Transactional connection acceptance

`server_accept_connection` initializes and transitions the caller-visible object to `READING` before epoll registration. If registration fails, the fd is closed while the object remains logically `READING` and stores the closed descriptor.

Required property:

```text
failure => no published valid READING connection
```

Use prepare/register/commit or explicit rollback/poisoning.

### 5. Linux argument narrowing

The Assembly ABI transports values in 64-bit registers, while several Linux syscall semantic domains are narrower (`int`, `unsigned int`, enumerations, packed flags). Before ABI freeze, explicitly define whether high bits are rejected, normalized, or deliberately truncated.

The mathematical kernel should support checked narrowing where appropriate.

### 6. Event timeout/deadline semantics

`event_epoll_wait(timeout_ms)` currently reuses the full positive timeout after `EINTR`; therefore `timeout_ms` is not a strict total elapsed-time bound.

Separate concepts:

- **duration** — relative amount requested for one attempt;
- **deadline** — absolute monotonic time bound;
- **remaining** — `max(0, deadline - now)`.

Future deadline-sensitive runtime paths should recompute remaining time after interruption.

### 7. Reusable connection/request storage

Successful requests reset the request arena and output state, but failure/close paths do not establish a complete pristine-state contract for pooled connection reuse.

Before pooling/reuse, define one acquisition/reset operation that establishes empty input/output logical buffers, arena offset zero, counters zero, and valid initial state before publication.

## Cross-layer algorithmic redesign

### Fragmented-request reparsing

`http_parse_request` itself is linear, but `server_handle_http_once` reparses the entire accumulated input after every successful read when the parser returns `-EAGAIN`.

For an `n`-byte request arriving one byte at a time, total parse work is approximately:

```text
1 + 2 + ... + n = n(n+1)/2 = O(n²)
```

This is a server-composition complexity defect.

The retrofit should preserve the current parser as a correctness reference and introduce either:

- incremental parser state;
- incremental framing/frontier state plus one final full parse; or
- another persistent parse-progress representation that guarantees total request work is `O(n)`.

### Buffered pipelined work

After consuming request 1, a complete request 2 may already remain in the userspace input buffer while the kernel socket has no unread bytes. The outer event loop must therefore drain logical work until `EAGAIN`/close before returning to `epoll_wait`.

### Request span lifetime

Parsed request and route-parameter value spans alias the input buffer. After `buffer_consume` compacts the buffer, those spans are no longer stable. Their lifetime is request-scoped and ends before/at request completion.

The future C/request-context ABI must preserve this rule.

## Reference implementations and experiments

Reference implementations remain authoritative for semantics. Experiments are admitted only after equivalence/correctness qualification and measured benefit.

Candidates:

- `bytes_find`: current bounded naive search vs Two-Way/Boyer-Moore-Horspool/SIMD candidates;
- decimal formatting: repeated `DIV` vs reciprocal multiplication;
- forward `memory_move`: size-dependent scalar/qword/REP policy;
- connection transitions: branch relation vs table/bitmask representation;
- route lookup: ordered linear reference vs prepared hash/trie/static index;
- response output: contiguous buffer copy vs `writev`/scatter-gather body spans;
- server response readiness: unconditional EPOLLOUT arm/disarm vs immediate write then EPOLLOUT only on `EAGAIN`;
- listener creation: atomic nonblocking/CLOEXEC creation;
- future event batching and work budgets.

## Security work required before Assembly ABI freeze

Add distinct primitives/contracts for:

- secure memory clear (not ordinary `memory_zero`);
- constant-time equality (not early-exit compare/equality);
- canonical checked size/range arithmetic;
- bounded decoding/resource limits;
- deterministic fail-closed malformed-input handling;
- retained-instruction/constant-time qualification where applicable.

## Go decision

**GO to construction after this audit is sealed as documentation.**

Construction must begin with canonical mathematical primitives and proven contract defects, not optimization experiments. No optimized route index, SIMD search, scatter/gather response path, or epoll fast path should enter production until the correctness retrofit is green under existing tests, all Polish Gates, the Core Qualification Gate, code-size review, and machine-specific performance verification.
