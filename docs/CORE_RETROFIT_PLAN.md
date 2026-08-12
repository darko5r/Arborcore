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

Retrofit D experiment:

- qualify a prepared static exact-route index against the ordered linear reference; retain the linear router unless the prepared candidate preserves first-match semantics and wins the quality vector.

Experiments still deferred to later phases:

- broader hashing/perfect-hashing/trie route compilers beyond the D4 prepared-index candidate;
- scatter/gather response output (Retrofit E).

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


## Retrofit E — runtime/event/server composition

See `docs/CORE_RETROFIT_E.md` for the qualified runtime transaction, deadline, incremental framing, pipeline-drain, work-budget and experiment contracts.

## Final Assembly Security + ABI Freeze (S0–S10)

After Retrofit E, normal A–E runtime retrofit work is frozen.  The final
Assembly phase is security/compatibility hardening only:

1. S0 symbol/dependency inventory;
2. S1 secure clear;
3. S2 content-independent equality;
4. S3 sensitive-memory lifetime policy;
5. S4 hostile-boundary qualification;
6. S5 stable Capability ABI selection;
7. S6 internal/public ELF visibility separation;
8. S7 PIC/shared-object readiness;
9. S8 deterministic static-library readiness;
10. S9 final Assembly qualification;
11. S10 ABI v1 freeze evidence.

Library readiness is qualified here, but publication/installation of
`libarborcore.a` and `libarborcore.so.1` remains the immediately following
library-packaging phase.

## Assembly Library Packaging (L0–L4)

After the ABI-v1 freeze commit, packaging must not alter production Assembly,
public symbols, frozen layouts, ownership/lifetime contracts or error
semantics.  The library phase formalizes the already-qualified artifacts:

1. L0 canonical deterministic `libarborcore.a` and fully-versioned
   `libarborcore.so.1.0.0` builds;
2. L1 static consumer, independent reproducibility, staged installation and
   uninstall qualification;
3. L2 SONAME `libarborcore.so.1`, `ARBORCORE_1.0` version-script and exact
   dynamic-export qualification;
4. L3 shared consumer/runtime and staged symlink/install ownership
   qualification;
5. L4 release gate against the frozen ABI/source/library identities.

The default installed library layout is `/usr/local/lib`; frozen ABI metadata
is installed under `/usr/local/share/arborcore/abi`. `DESTDIR`, `PREFIX`,
`LIBDIR`, and `DATADIR` remain overridable for package managers. The phase
creates no C header; the subsequent C bridge defines the C-facing interface.

## C Runtime Bridge (CR0–CR8)

The C runtime bridge begins from Assembly library release/tag v1.0.0 and treats
Assembly ABI v1 as immutable lower-layer infrastructure:

1. CR0 exact C/System-V representation of the 94-symbol Assembly ABI;
2. CR1 canonical static consumer and shared-Assembly equivalence consumer;
3. CR2 compile-time frozen layout/state assertions;
4. CR3 typed C status classification preserving native negative errno;
5. CR4 borrowed-span and ownership/lifetime wrappers;
6. CR5 request/route/response composition without reimplementation;
7. CR6 connection/event/server composition through the frozen runtime;
8. CR7 strict compiler, adversarial, dependency and sanitizer qualification;
9. CR8 raw-vs-wrapper bridge-overhead and reproducibility gate.

The default lower-layer link remains `libarborcore.a`. Shared equivalence against
`libarborcore.so.1` remains mandatory. The C runtime API itself is not frozen by
CR0–CR8; it remains construction-stage until the higher-level C framework is
qualified.

## Geometry Precision Model after CR foundation

After the CR bridge is sealed, geometry becomes a first-class higher-level
capability above the frozen Assembly ABI. HTML/CSS is never replaced. Arborcore
supports HTML/CSS mode, Precision Surface mode, and Hybrid mode in parallel.
See `docs/GEOMETRY_PRECISION_PLAN.md`.
