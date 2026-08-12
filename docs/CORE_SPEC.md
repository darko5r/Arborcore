# Arborcore Assembly Core Specification

## Purpose

This document defines the canonical mathematical model that all current and future Arborcore Assembly modules should implement. It describes semantics, not necessarily function-call boundaries: a hot path may inline an equivalent checked formula rather than call a helper.

## 1. Unsigned integer domain

For current size/address arithmetic:

```text
U64 = {0, 1, ..., 2^64 - 1}
MAX = 2^64 - 1
```

Checked operations succeed exactly when the mathematical result belongs to `U64`; otherwise they return the established overflow error and must not publish a partial result.

### Addition

```text
add(a,b) succeeds iff a+b <= MAX
```

### Subtraction

```text
sub(a,b) succeeds iff a >= b
```

### Multiplication

```text
mul(a,b) succeeds iff a*b <= MAX
```

### Checked narrowing

Where a public Arborcore service maps a 64-bit register value into a narrower Linux semantic domain, the conversion must have an explicit range contract. Examples:

```text
u64_to_u32(x) succeeds iff x <= UINT32_MAX
u64_to_i32_nonnegative(x) succeeds iff x <= INT32_MAX
```

Signed domains must be modeled explicitly rather than inferred from register width.

## 2. Integer geometry

### Floor/ceiling division

For nonnegative integer `x` and positive divisor `a`:

```text
floor_div(x,a) = floor(x/a)
ceil_div(x,a)  = ceil(x/a)
```

An implementation may use algebraically equivalent integer formulas only if overflow behavior is accounted for.

### Power-of-two predicate

For unsigned `a`:

```text
is_pow2(a) <=> a != 0 and (a & (a-1)) == 0
```

### Alignment

For `a > 0`, define:

```text
align_up(x,a) = smallest y >= x such that y mod a = 0
align_down(x,a) = greatest y <= x such that y mod a = 0
```

Required `align_up` properties:

```text
y >= x
y mod a = 0
0 <= y-x < a
```

For a power-of-two `a`, the optimized formula may be:

```text
(x + a - 1) & ~(a - 1)
```

but the addition must be checked for overflow first.

## 3. Span/range algebra

A byte span is modeled as a half-open interval:

```text
Span(base,n) = [base, base+n)
```

### Representability

A span is representable iff:

```text
base+n <= MAX
```

For `n=0`, no memory dereference is implied.

### Containment

For spans `A=[a,a+n)` and `B=[b,b+m)`:

```text
B subset-of A
<=>
a <= b and b+m <= a+n
```

with endpoint arithmetic proven representable.

### Remaining capacity

For `0 <= offset <= length`:

```text
remaining = length-offset
```

Using subtraction after proving `offset <= length` avoids unnecessary endpoint overflow.

### Overlap

Two nonempty spans overlap iff their intersection is nonempty. Implementations may use difference-based overlap tests when they avoid potentially overflowing endpoint calculations.

### Logical vs physical state

For buffers/arenas, bytes outside the current logical range are unspecified/stale storage. Reset/rewind changes logical state but does not imply secure physical erasure.

## 4. Finite-sequence algebra

Byte strings are finite sequences:

```text
A = (a0, a1, ..., an-1)
```

Required reusable laws include:

```text
equal(A,A) = true
equal(A,B) = equal(B,A)
compare(A,B) = -compare(B,A)
trim(trim(A)) = trim(A)
```

Search returns the minimum valid matching index, not an arbitrary matching index.

Codecs should expose inverse/canonicalization laws where their syntax permits them:

```text
hex_decode(hex_encode(B)) = B
base64_decode(base64_encode(B)) = B
percent_decode(percent_encode(B)) = B
parse_decimal(format_decimal(x)) = x
parse_hex(format_hex(x)) = x
```

For accepted canonical Base64 text `S`:

```text
base64_encode(base64_decode(S)) = S
```

Percent encoding may canonicalize accepted noncanonical escape spelling; therefore `encode(decode(S))` is canonicalization rather than universal identity.

## 5. Parser/span model

A successful HTTP request parse returns views that are contained within the input byte span:

```text
method  subset-of request
 target subset-of request
version subset-of request
headers subset-of request
body    subset-of request
```

On success:

```text
message_length <= available_input_length
```

For Content-Length framing:

```text
message_length = (body_start - request_start) + content_length
```

The parser output object and input bytes must obey the ABI alias/precondition chosen by construction (currently intended to be disjoint).

All failure outputs documented as fail-closed must remain zero/invalid and must not publish partial logical state.

## 6. Request-target algebra

Origin form is:

```text
target = path
```

or:

```text
target = path || "?" || query
```

The first `?` separates path and query without copying. Path begins `/`. The complete raw target is validated before either span is published: raw `#` and non-visible ASCII are rejected in both components, while later `?` bytes are ordinary query data. Percent decoding is not part of decomposition.

The asterisk form `*` remains a distinct accepted target form.

### Route parameter span lifetimes

A successful parameterized route match returns borrowed spans:

```text
parameter name  -> immutable route-catalog/pattern storage
parameter value -> current request-target/input storage
```

Name spans remain valid only while the immutable route catalog remains alive. Value spans remain valid only for the active request lifetime and are invalidated when the underlying input buffer is consumed, compacted, reset, or reused. Parameter records are valid only on a successful match; callers must ignore their contents when the match result is zero or negative.

Catalog order is authoritative. The first valid method+pattern match wins; exact/static patterns do not implicitly outrank parameter patterns.

## 7. Buffer algebra

For initialized buffer `(data,length,capacity)`:

```text
0 <= length <= capacity
```

### Append

Desired abstract operation:

```text
new_logical_bytes = old_logical_bytes || snapshot(source)
new_length = old_length + source_length
```

subject to checked representability and `new_length <= capacity`.

### Consume

For `n <= length`:

```text
new_length = length-n
new_logical_bytes = old_logical_bytes[n..length)
```

Useful laws:

```text
reset(reset(B)) = reset(B)
append(B, empty) = B
consume(consume(B,a),b) = consume(B,a+b)
```

when domains/capacity make each expression valid.

## 8. Arena algebra

Arena state `(base,capacity,offset)` obeys:

```text
0 <= offset <= capacity
```

For aligned allocation:

```text
current = base+offset
p = align_up(current, alignment)
new_offset = (p-base)+size
```

Success requires the allocation span to remain within the arena and all arithmetic to be representable.

Successful allocation is monotonic:

```text
new_offset >= old_offset
```

Mark/rewind provides logical frontier reversibility:

```text
m = mark(A)
... allocations ...
rewind(A,m)
=> offset = m
```

It does not restore/erase memory contents.

Zero-size aligned allocation semantics are frozen for Assembly ABI v1: a
zero-size allocation still honors the requested power-of-two alignment and may
advance the logical frontier to the next aligned absolute address.  On success
the returned pointer is that aligned address and the new offset is the aligned
offset; no payload bytes are reserved beyond the alignment gap.

## 9. State algebra

Connection states:

```text
S = {
  ACCEPTED,
  READING,
  REQUEST_READY,
  DISPATCHING,
  WRITING,
  KEEP_ALIVE,
  CLOSING,
  CLOSED
}
```

The allowed transition relation is:

```text
ACCEPTED      -> READING | CLOSING
READING       -> REQUEST_READY | CLOSING
REQUEST_READY -> DISPATCHING | CLOSING
DISPATCHING   -> WRITING | CLOSING
WRITING       -> KEEP_ALIVE | CLOSING
KEEP_ALIVE    -> READING | CLOSING
CLOSING       -> CLOSED
CLOSED        -> {}
```

Invalid transitions must leave state unchanged.

State-coupled counters obey state-specific invariants, e.g. write progress is meaningful only while writing. Public APIs must not permit mutation that invalidates these invariants.

## 10. Monotonic progress and termination

Loops that operate on bounded data should expose a monotonic quantity.

Examples:

```text
write_all: remaining bytes strictly decrease after each successful positive write
buffered read: remaining capacity decreases after each successful positive read
response write: write offset strictly increases after each successful positive write
bounded scanners: cursor strictly advances until end/match
```

`EINTR`/`EAGAIN` do not count as logical progress unless explicitly documented.

## 11. Time algebra

Do not conflate duration with deadline.

```text
duration = relative interval
deadline = absolute CLOCK_MONOTONIC timestamp
remaining(now,deadline) = max(0, deadline-now)
```

Deadline-sensitive wait/retry code recomputes `remaining` after interruption.

## 12. Response serialization alias contract

HTTP response serialization preflights the complete destination requirement before publication. Expected validation and capacity failures leave the logical buffer state and backing bytes unchanged.

Caller-supplied response bodies may alias the output buffer. The serialized body has whole-operation snapshot semantics:

```text
serialized_body = body_bytes_visible_at_serializer_entry
```

If response metadata would overwrite an aliased body source before the body append occurs, the body is staged with overlap-safe movement into its final destination before metadata publication. Static/header fragments use the explicitly prechecked disjoint append path only after complete destination preflight.

## 13. Resource algebra

Kernel descriptors are owned resources, not ordinary reusable integers.

Useful abstract states:

```text
UNOWNED -> ACQUIRED -> PUBLISHED -> RELEASING -> RELEASED
```

A consuming close attempt ends Arborcore ownership; callers must not blindly retry the same numeric descriptor after ambiguous close results.

Compound acquisition should be transactional:

```text
prepare -> acquire -> validate/register -> publish
```

Failure before publish cleans up resources and leaves no logically valid published object.

## 14. Transactional logical mutation

For operations advertised as transactional:

```text
success => commit complete logical result
failure => caller-visible logical state remains prior state or documented invalid/poison state
```

Physical bytes outside the logical state need not be restored unless the contract is explicitly security-sensitive.

## 15. Complexity/resource contracts

Every important subsystem should eventually state a complexity contract plus structural counters where useful.

Initial targets:

```text
arena allocation             O(1)
HTTP parsing per complete input O(n)
request-target split         O(n)
route-pattern match/candidate O(path+pattern)
event-result processing      O(events returned)
```

The complete server request lifecycle must avoid adversarial fragmented-input `O(n²)` reparsing.

Machine-independent counters should be preferred alongside wall-clock profiles where practical:

- bytes scanned;
- bytes copied;
- comparisons/probes;
- allocations;
- syscalls;
- event-control operations;
- instructions/branches where tooling permits.

## 16. Qualification principles

1. finite domains should be exhaustive when inexpensive (ASCII byte domain; connection-state pairs);
2. algebraic identities/inverses supplement example vectors;
3. reference and optimized implementations must be observationally equivalent;
4. self-certifying qualification may produce/check witnesses outside production hot paths;
5. production performance profiles remain environment-specific;
6. optimization acceptance requires correctness gates plus measured benefit.


## Retrofit E runtime contracts

See `docs/CORE_RETROFIT_E.md` for the qualified runtime transaction, deadline, incremental framing, pipeline-drain, work-budget and experiment contracts.


## Assembly security primitives

Security-sensitive byte operations are deliberately distinct from ordinary
performance-oriented primitives:

```text
memory_zero                    ordinary logical zero fill
memory_secure_clear            architectural overwrite of every byte
memory_compare                 lexicographic, early exit
memory_equal_constant_time     content-independent equality over a public length
```

`memory_secure_clear` does not claim to erase copies, CPU-cache history, swap,
snapshots or device-level remnants. `memory_equal_constant_time` may scale with
length but must not terminate early based on byte contents.

## Assembly ABI v1 freeze

The canonical public symbol list, internal-symbol classification and frozen
data layouts are maintained under `abi/`.  Assembly ABI v1 targets Linux
x86-64 using System V AMD64.  Symbols omitted from `abi/arborcore-1.symbols` are
implementation details even when they remain ELF globals for static cross-object
resolution.  Shared-library readiness builds enforce the public surface with an
ELF version script.
