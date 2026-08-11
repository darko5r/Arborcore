# Arborcore Assembly Core Quality Policy

## Purpose

Arborcore does not optimize for a single scalar such as raw speed. Engineering decisions are made against a qualified quality vector that includes correctness, determinism, reliability, security, performance, resource efficiency, code size, portability, and architectural simplicity.

Performance remains a first-class requirement, but it does not override a stronger correctness, security, or bounded-behavior guarantee merely because one benchmark is faster.

This policy applies to Assembly Core construction, qualification, optimization experiments, ABI freeze, and later runtime/framework layers.

## Decision hierarchy

### 1. Non-negotiable invariants

A candidate must not knowingly violate:

- functional correctness;
- memory safety and valid-span rules;
- checked arithmetic/range requirements;
- protocol correctness;
- required deterministic behavior;
- security guarantees;
- ownership/lifetime invariants;
- fail-closed malformed-input behavior;
- defined ABI contracts;
- mandatory resource bounds.

A candidate that violates a non-negotiable invariant is rejected regardless of speed.

### 2. Measured engineering qualities

For candidates that satisfy the required invariants, compare measured qualities such as:

- median latency;
- tail latency;
- throughput;
- syscall count;
- work/probe/scan counts;
- memory consumption;
- allocation/storage footprint;
- production `.text` size;
- branch/cache behavior when materially relevant;
- resource ceilings;
- failure/recovery behavior;
- malformed/adversarial-input cost.

No single metric automatically decides the result.

### 3. Structural qualities

Also evaluate:

- implementation simplicity;
- verifiability;
- maintainability;
- portability;
- composability;
- ownership/lifetime clarity;
- rollback/transactional clarity;
- reference-versus-optimized equivalence;
- future ABI stability.

Structural quality may justify a modest measured cost when it materially reduces ambiguity, failure modes, or verification burden.

## Quality-vector decisions

Use three primary decisions.

### ADMIT

Admit a candidate when:

- all non-negotiable invariants pass; and
- measured regressions remain within accepted envelopes; or
- a material quality improvement is obtained with no meaningful engineering downside.

Examples include:

- eliminating an overflow class with negligible latency change;
- replacing ambiguous behavior with deterministic semantics;
- reducing code size without harming required performance;
- improving performance while preserving exact semantics.

### REVIEW

Require explicit trade-off review when:

- a material quality improvement causes a performance, size, or resource regression outside the normal accepted envelope; or
- different candidates dominate different dimensions and no candidate is clearly superior.

The review must record both the gain and the cost.

Example:

```text
QUALITY GAIN
worst-case work: O(n²) -> O(n)
fragmentation robustness: improved
resource bound: explicit

COST
median lifecycle latency: +2.2%
production .text: +180 bytes
per-connection state: +24 bytes

DECISION
ACCEPT / REDESIGN / REJECT
RATIONALE
<evidence-backed explanation>
```

### REJECT

Reject a candidate when:

- a non-negotiable invariant fails;
- performance/resource cost is material and no material quality gain is demonstrated;
- complexity increases without sufficient evidence-backed benefit;
- an optimization changes semantics without an explicit approved contract change;
- benchmark improvement is not repeatable or cannot be attributed reliably.

## Pareto discipline

Prefer candidates on the practical Pareto frontier.

A candidate is dominated when another candidate is at least as good in every material dimension and strictly better in at least one.

Do not keep a dominated candidate merely because it is newer, more complicated, or uses a theoretically interesting technique.

## Performance-gate semantics

A performance gate reports measurement evidence. It does not silently override architecture.

Therefore:

- `PASS` means measured regressions are within the accepted profile envelopes.
- `FAIL` means a performance trade-off exists and must be investigated.
- A performance `FAIL` is not automatically an architectural rejection when the candidate provides a material correctness, determinism, security, bounded-complexity, or reliability improvement.
- Such a candidate enters REVIEW and requires stronger measurement before acceptance.

Do not weaken or widen accepted performance envelopes merely to force a desired candidate to pass.

## Candidate versus strict verification

Pre-commit candidate qualification may measure a deliberately dirty production tree when exact source identity is recorded.

Committed/release qualification requires a clean production tree.

The measured source identity must include the exact production Assembly source hash so a pre-commit candidate result can be matched to the committed result.

## Measurement escalation

When a decision depends on a small performance difference, increase evidence quality instead of guessing.

Escalation may include:

1. repeat independent benchmark rounds;
2. compare median-of-medians;
3. inspect MAD/noise and tail behavior;
4. inspect code size;
5. measure syscall counts;
6. measure structural work counters;
7. use `perf` or equivalent hardware-counter evidence where useful;
8. inspect memory/resource footprint;
9. test adversarial/worst-case inputs;
10. verify the exact candidate source hash.

A small change inside normal run-to-run noise must not be presented as a demonstrated optimization or regression.

## Machine-specific performance policy

Benchmark methodology is portable.

Accepted absolute timings and regression profiles are environment-specific.

For example, a local workstation, Hetzner production host, and CI reference machine may each have independent accepted profiles.

Never encode one machine's absolute nanosecond values as a universal Arborcore requirement.

## Code-size policy

Production `.text` growth is a real cost and must be measured.

Growth may be accepted when it buys a material quality improvement, such as:

- canonical checked arithmetic;
- a reusable invariant-enforcing primitive;
- elimination of an algorithmic worst case;
- stronger security behavior;
- removal of duplicated unsafe semantics.

A code-size increase without a demonstrated quality or performance benefit should be challenged.

## Reference and optimized implementations

Where practical, preserve a mathematically obvious reference implementation when introducing an optimized implementation.

The optimized candidate must demonstrate:

- equivalent successful results;
- equivalent error behavior;
- equivalent boundary behavior;
- preserved ownership/lifetime semantics;
- preserved security guarantees;
- measured benefit sufficient to justify added complexity.

Correctness equivalence is required before performance comparison can decide production admission.

## Security rule

Do not trade away an explicit security guarantee for ordinary performance.

If a security-sensitive operation requires constant-time comparison, secure clear, canonical decoding, checked resource limits, or fail-closed behavior, benchmark candidate implementations only after they satisfy the required security contract.

Ordinary early-exit or logical-reset primitives must not be relabeled as secure merely because they are faster.

## Determinism and reliability

When relevant, qualify:

- deterministic outputs;
- deterministic precedence;
- stable failure codes;
- bounded retry behavior;
- bounded work;
- transactional publication;
- rollback semantics;
- resource ownership and release.

A modest performance cost may be acceptable when it converts undefined, ambiguous, or adversarially unbounded behavior into deterministic bounded behavior.

## Decision record

For a material trade-off, record:

```text
CHANGE
<what changed>

INVARIANTS
<pass/fail evidence>

QUALITY GAINS
<correctness, security, determinism, reliability, complexity, etc.>

QUALITY COSTS
<latency, throughput, size, memory, complexity, portability, etc.>

MEASUREMENTS
<profiles, rounds, hashes, counters, code size>

ALTERNATIVES
<reference and competing candidates>

DECISION
ADMIT / REVIEW / REJECT

RATIONALE
<why this is the best qualified system-level choice>
```

## Core Qualification Gate

The future Core Qualification Gate must aggregate evidence rather than reduce quality to speed alone.

It should include, where applicable:

- existing correctness tests;
- Polish Gates;
- exhaustive finite-domain tests;
- algebraic/property tests;
- self-certifying/reference checks;
- ABI/symbol checks;
- GNU-stack checks;
- security qualification;
- code-size accounting;
- structural work/complexity counters;
- memory/resource bounds;
- machine-specific performance verification.

The objective is the best qualified Arborcore implementation, not merely the fastest isolated benchmark.
