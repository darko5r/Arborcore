# Arborcore Assembly Security and ABI Freeze

## Scope

This phase closes the Assembly core after Retrofits A–E.  It adds no framework
feature layer.  Its purpose is to make the existing core safe and stable enough
for formal library packaging and the future C bridge.

## S0 — symbol inventory

Every production ELF global is classified exactly once as either stable ABI or
implementation-only.  The canonical files are:

- `abi/arborcore-1.symbols`
- `abi/arborcore-1.internal-symbols`

Implementation-only ELF globals may remain globally bindable inside the static
archive because separate object files require cross-object resolution.  They
are not compatibility promises.  `libarborcore.so.1` readiness builds enforce
external visibility using `abi/arborcore-1.map`.

## S1 — secure clear

`memory_secure_clear(destination,length)` overwrites the complete requested span
with zero when `length > 0`, returns the original destination, and performs no
memory access for length zero.  Because it is implemented directly in Assembly,
the operation itself cannot be removed by a C compiler dead-store pass.

The guarantee is architectural overwrite of the caller-supplied span only.  It
does not claim erasure of unrelated copies, cache history, swap, snapshots or
physical-media remanence.

## S2 — content-independent equality

`memory_equal_constant_time(left,right,length)` returns 1 for equality and 0
otherwise.  It reads/folds every byte in the supplied range.  Control flow may
depend on the public length but not on the first mismatch position or byte
values.  It is intended for fixed-length security-sensitive comparisons.

## S3 — sensitive-memory lifetime policy

`buffer_reset`, `arena_reset`, `arena_rewind` and connection request reuse are
logical frontier/state operations.  They do not securely erase backing bytes.
Sensitive data must be explicitly cleared with `memory_secure_clear` before its
storage lifetime ends or before that storage is released/reassigned when the
threat model requires zeroization.

A secure clear cannot retroactively erase copies already made elsewhere.
Callers therefore remain responsible for minimizing sensitive copies.

## S4 — hostile-boundary qualification

The final gate runs all A–E property/adversarial suites plus the dedicated
security primitive qualification.  Finite domains remain exhaustive where
practical.  Existing rollback, malformed-input, aliasing, fragmentation,
integer-boundary, state-transition and deadline tests remain mandatory.

## S5/S6 — Capability ABI and visibility

Assembly ABI v1 exposes the qualified capability surface in
`abi/arborcore-1.symbols`.  Raw Linux I/O/network shims, retained implementation
variants, benchmark-policy data, `http_frame_scan`, exact-router internals,
`write_all`, `_start`, and the prechecked-disjoint buffer helper remain internal.

The static archive may contain those implementation symbols; consumers must not
depend on them.  The shared-object readiness build exports only the versioned v1
surface.

## S7 — shared-object readiness

A candidate `libarborcore.so.1` is linked directly from the same qualified
Assembly objects using:

- `-z defs`
- `-z text`
- `-z relro`
- `-z now`
- `-z noexecstack`
- `-Bsymbolic-functions`
- SONAME `libarborcore.so.1`
- `abi/arborcore-1.map`

The readiness gate requires no unresolved dynamic dependencies, no TEXTREL,
non-executable GNU_STACK, the exact dynamic symbol manifest, and a runnable
minimal dynamic consumer.

This is readiness evidence, not a published library release.

## S8 — static-library readiness

A deterministic `libarborcore.a` candidate is created from all production core
objects except the process entry object `start.o`.  A standalone no-libc ABI
consumer must link against and execute using the archive.

## Frozen layout policy

`abi/arborcore-1.layout` freezes the public structure sizes, offsets, connection
state values, event-record layout and server work-budget constants needed by
native ABI consumers.

Arena zero-size aligned allocation is explicitly frozen: it honors alignment
and may advance the logical frontier to the next aligned address even though no
payload bytes are requested.

## Compatibility rule

After the v1 freeze, adding a new versioned symbol can be backward-compatible.
Removing a v1 symbol, changing a v1 calling convention, changing a frozen
layout/state value, or changing documented ownership/lifetime/error semantics
requires an explicit ABI-version decision.
