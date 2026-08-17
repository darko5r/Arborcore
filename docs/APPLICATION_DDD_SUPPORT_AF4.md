# Arborcore AF4 — DDD Support

AF4 is the final planned Application Foundation phase. It completes the
framework-level DDD interaction rules above AF2 capability composition and AF3
typed Application-service lifecycle without introducing persistence or MVC.

## Typed ports

A port is a semantic role over an AF2 typed capability. AF4 deliberately does
not create a second registry, dependency graph, provider-selection algorithm,
or hot-path lookup mechanism. Providers publish typed versioned AF2 interface
tables; Application services resolve required bindings during composition and
cache them.

Concrete port methods remain bounded-context-specific. The generic framework
does not expose a universal `execute(void *, void *, void *)` data plane.

## Repositories

A repository is an outbound bounded-context-specific port. AF4 does not define
generic CRUD functions. A bounded context may define, for example, a typed
`OrderRepository` capability whose methods accept domain-specific identifiers,
aggregates, commands, results, and—when required—an AF4 transaction view.

The interface belongs to the bounded context. A future MariaDB adapter may
implement it, but neither the domain-facing interface nor AF4 itself depends on
MariaDB.

## Single-authority transaction model

AF4 defines one transaction interface per authority. The interface is an AF2
typed capability table with a 128-bit authority ID, caller-owned transaction
state size/alignment, and begin/commit/rollback callbacks.

A Unit of Work uses exactly one authority in AF4 v1. Distributed transactions,
nested transactions, and savepoints are intentionally out of scope.

Callbacks use the native mechanism convention: zero is success, negative is a
mechanism failure, positive is invalid/reserved. Business outcomes remain
bounded-context typed outputs and are not encoded as errno-like callback
results.

Transaction state memory is supplied by the composition/application caller.
AF4 has no hidden allocation. A provider begin callback that returns failure is
required to be failure-atomic/self-cleaning with respect to provider-owned
state; AF4 guarantees only that the Unit-of-Work object is not published.
Opaque provider-context region size is not knowable to AF4. AF4 therefore
rejects the machine-detectable case where the provider-context anchor pointer
falls inside a known AF4 input/output/state/journal/interface region; overlap
that begins outside those known regions remains a provider/caller contract. A
standalone transaction view is also rejected when the view object itself
overlaps its referenced interface or transaction-state storage, because later
state mutation could otherwise corrupt the validated view object.

## Unit-of-Work state machine

The state machine is:

`ZERO -> ACTIVE -> COMMITTED`

or

`ZERO -> ACTIVE -> ROLLED_BACK`

or on commit/rollback callback failure:

`ZERO -> ACTIVE -> FAULTED`

Begin is valid only from a completely zero Unit-of-Work object. Terminal
objects require explicit reset before reuse. Reset is deliberately local to the
Unit-of-Work object and does not dereference the borrowed transaction interface,
provider context, transaction state, or event journal, so those external
dependencies need not remain valid merely for terminal reset. Lifecycle
mutation is externally serialized.

While a Unit of Work is ACTIVE, callers must not externally clear, rewind, or
otherwise mutate its journal except through normal event append operations.
A successful begin requires the supplied event journal to satisfy the full
canonical packed-prefix invariant before it captures an event-journal
checkpoint. This prevents publishing an ACTIVE Unit of Work over caller-corrupt
journal metadata that could later make both commit and rollback unusable.
Commit success leaves events appended after that checkpoint in the journal and makes them eligible
for application-level publication. Rollback always rewinds to the checkpoint.
Commit failure also rewinds and transitions to `FAULTED`, preventing
uncommitted or uncertain events from being treated as publishable.

AF4 does not automatically retry a failed transaction.

## Domain-event journal

The DDD event journal is distinct from `src/asm/event.asm`, which is Arborcore's
Linux epoll event-loop mechanism.

The AF4 journal uses caller-owned event-record storage and caller-owned payload
bytes. Appending copies payload bytes into the journal. No pointer into
transient domain state is retained.

Ordering is exact append order and the valid journal representation is a
canonical packed payload prefix: each retained record begins at the byte offset
immediately following the retained payload of its predecessor, and the final
retained record ends at `byte_count`. A checkpoint records record count, payload
byte count, and generation. Rewind is accepted only for the same generation,
for counts that are not ahead of the current journal, and when its byte count is
the exact event boundary for its record count. Checkpoint creation and rewind
also require the current journal itself to satisfy the canonical packed-prefix
invariant. This prevents a fabricated same-generation checkpoint—or existing
caller corruption—from being accepted as a valid transactional boundary. Clear
increments generation, invalidating stale checkpoints.

Append is failure-atomic with respect to journal counts and sequence output.
Capacity, range, pointer-overflow, and detectable alias hazards are rejected
before mutation.

The journal is in-process staging. AF4 does not publish externally and does not
promise durable or exactly-once delivery. A durable outbox, broker, or message
bus belongs to a later infrastructure adapter.

## Ownership

All journal records, payload bytes, transaction state, Unit-of-Work objects,
AF2 bindings, provider contexts, and bounded-context typed inputs/outputs remain
explicitly owned by their callers/providers according to the relevant
contracts. AF4 introduces no hidden heap, mutable registry, internal lock, or
reference count.

AF4 pointer-bearing structures are native in-process objects. They are not HTTP,
database, WASM, wire, or process-boundary representations.

## ABI

Current qualification is x86-64 System V. The frozen target sizes are:

- `arbor_ddd_event_record`: 32 bytes
- `arbor_ddd_event_checkpoint`: 16 bytes
- `arbor_ddd_event_journal`: 40 bytes
- `arbor_ddd_event_view`: 40 bytes
- `arbor_ddd_transaction_interface`: 72 bytes
- `arbor_ddd_transaction_view`: 32 bytes
- `arbor_ddd_unit_of_work`: 64 bytes

The public AF4 function inventory is exactly 14 functions, covering journal
operations, transaction validation, and Unit-of-Work lifecycle.

## Lower-layer policy

AF0 through AF3 remain frozen during AF4 construction. If AF4 demonstrates a
concrete need to change a frozen production path, construction stops and that
need is reviewed as a controlled retrofit rather than silently altering lower
authority.

After AF4, Application Foundation work is complete. MVC and infrastructure
continue as separate architectural tracks.
