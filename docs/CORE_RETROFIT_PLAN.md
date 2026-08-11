# Arborcore Assembly Core Retrofit Plan

## Scope boundary

Construction begins only after the read-only audit documentation is committed. The first construction branch is `assembly-core`.

The initial mathematical retrofit is not an optimization sprint. Proven correctness/ABI defects and canonical mathematical contracts come first. Performance experiments remain separate until the hardened reference path is green.

## Baseline

Construction starts from the production tree represented by audit commit `523e7177aeb4699bfe7e1525218b08edc47574c5`, with:

- 25 production Assembly modules;
- production `.text` reference: 10,285 bytes;
- Polish Gates #1, #2, and #3 passing;
- portable server benchmark infrastructure;
- environment-specific performance profiles (e.g. `local-dev-i7-6700hq`) kept under ignored `generated/performance/`.

## Retrofit A — Integer geometry, spans, and checked narrowing

Primary modules:

- `u64_checked.asm`
- new internal/public mathematical helpers only where justified
- affected callers after contracts are established

Goals:

- canonical checked add/sub/mul semantics;
- floor/ceiling division where useful;
- align-up/align-down definitions;
- power-of-two predicates/next-power operations where justified;
- checked narrowing for Linux semantic domains;
- canonical span/range representability, containment, overlap, and remaining rules;
- avoid call overhead in hot paths where inline/macro equivalence is preferable.

Qualification:

- exhaustive small domains for alignment/range arithmetic;
- boundary lattice around zero, powers of two, `2^63`, and `UINT64_MAX`;
- independent reference computations;
- no regression in existing numeric tests.

## Retrofit B — Finite sequences, parsing, formatting, and codecs

Primary modules:

- `ascii.asm`
- `bytes.asm`
- `bytes_scan.asm`
- `parse_u64.asm`
- `u64_format.asm`
- `hex_codec.asm`
- `percent_codec.asm`
- `base64.asm`

Goals:

- exhaustive ASCII classification/case conversion;
- algebraic sequence properties;
- parse/format inverse qualification;
- encode/decode inverse/canonicalization qualification;
- canonical percent unreserved set without imposing per-byte function-call cost;
- preserve current correct algorithms as references.

Experiments explicitly deferred:

- faster `bytes_find`;
- reciprocal decimal formatting;
- SIMD classifiers/scanners.

## Retrofit C — Memory, buffers, arenas, and connection state

Primary modules:

- `memory.asm`
- `memory_threshold.asm`
- `buffer.asm`
- `arena.asm`
- `connection.asm`

Goals:

- exhaustive small-domain `memory_move` overlap geometry;
- scalar/qword/REP observational equivalence;
- define/repair `buffer_append` alias semantics;
- formal buffer capacity laws;
- formal arena alignment/containment/mark-rewind semantics;
- choose/document zero-size aligned-allocation behavior;
- exhaustively test all 64 valid-state connection transition pairs plus invalid states;
- remove/restrict `connection_reset_io` before ABI freeze;
- define pristine reusable connection/request storage.

## Retrofit D — HTTP, request targets, responses, and routing

Primary modules:

- `http_parser.asm`
- `request_target.asm`
- `http_response.asm`
- `router.asm`
- `route_pattern.asm`

Required production hardening:

- validate query bytes after `?` in request-target decomposition;
- formalize parser input/output disjointness/preconditions;
- formalize response body/output alias contract;
- define route-parameter validity and lifetime;
- formalize route precedence and structural ambiguity.

Qualification:

- span containment verification;
- framing equations;
- malformed/fail-closed properties;
- target decomposition reconstruction;
- segment/capture algebra;
- property tests for exact and patterned dispatch.

Experiments explicitly deferred:

- prepared static route indexes;
- hashing/perfect hashing/trie variants;
- scatter/gather response output.

## Retrofit E — I/O, time, events, and server composition

Primary modules:

- `write.asm`
- `io.asm`
- `net.asm`
- `event.asm`
- `server.asm`
- `start.asm` only as required

Required hardening/redesign:

- explicit Linux argument narrowing domains;
- separate duration/deadline semantics and monotonic absolute deadlines where needed;
- transactional `server_accept_connection` publication;
- pristine connection acquisition/reuse contract;
- remove server-level adversarial fragmented-request `O(n²)` reparsing;
- ensure buffered pipelined work is drained before returning to `epoll_wait`;
- preserve request-span lifetime boundary through handler completion;
- isolate infrastructure framing fields from mutable application behavior.

Experiments after correctness redesign:

- immediate response write before arming EPOLLOUT;
- work budgets/fairness;
- event batching;
- atomic listener flags;
- syscall reduction.

## Security retrofit before ABI freeze

Add separately named contracts/primitives for:

- secure clear;
- constant-time equality;
- checked resource-size arithmetic;
- deterministic/canonical malformed-input handling;
- bounded resource policies;
- retained-instruction/constant-time checks where appropriate.

Do not overload normal early-exit comparison or logical reset primitives with security guarantees they do not currently have.

## Core Qualification Gate

A permanent gate should be created after the coherent retrofit groups are implemented. It should verify:

- all existing tests;
- Polish Gates #1, #2, and #3;
- exhaustive finite-domain suites;
- algebraic/property suites;
- self-certifying/reference checks;
- reference/optimized equivalence where alternatives exist;
- ABI/public-symbol expectations;
- GNU-stack/non-executable stack;
- code-size accounting versus the 10,285-byte reference;
- structural complexity/work counters;
- selected machine-specific performance-profile verification.

The Core Qualification Gate must not encode one computer's nanosecond values as universal Arborcore behavior. The benchmark methodology is portable; accepted performance profiles are environment-specific.

## Construction commit sequence

Recommended sequence on `assembly-core`:

1. **Establish mathematical integer and span foundation**
2. **Add mathematical sequence and codec qualification**
3. **Harden memory, buffer, arena, and connection invariants**
4. **Harden HTTP target, response, and routing invariants**
5. **Harden event/time/server composition and eliminate fragmented-input quadratic work**
6. **Add Assembly security primitives**
7. **Add and pass Core Qualification Gate**
8. **Run code-size and performance-profile qualification**
9. **Review before remaining server-core features**

Each commit must remain recoverable and must run the relevant old and new gates before promotion.

## After the mathematical retrofit

Proceed with remaining Assembly server-core capabilities, such as:

- timers/deadlines;
- request/connection resource limits;
- backpressure;
- partial-output queues/streaming;
- graceful shutdown;
- event batching/work budgets;
- qualified zero-copy/scatter-gather experiments;
- prepared routing if benchmarks support it.

Then perform the final Assembly capability/security/performance/ABI qualification and freeze the Assembly ABI before introducing the C runtime/framework bridge.
