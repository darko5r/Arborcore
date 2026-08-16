# Arborcore AF2 shared bidirectional capability/module kernel

AF2 extends the frozen AF0–AF1 R1 Application/DDD/MVC foundation with the
shared composition mechanism that later Application services, DDD-support, MVC
and other adapters reuse. It does **not** implement those higher policies yet.

## Purpose

Arborcore runtime calls may travel in both directions while source ownership
remains explicit:

```text
higher Application / DDD-support / MVC
              |
              | direct qualified API or resolved typed capability
              v
Assembly / C / Geometry / Renderer / Browser-facing services
              |
              | registered typed capability
              v
higher Application / DDD-support / MVC
```

The capability kernel is not a replacement for direct calls. Stable hot-path
services such as checked arithmetic, memory and other already-qualified public
APIs remain directly callable where lookup would add pointless indirection.
Capability composition is for discovery, inversion of control, replacement,
version negotiation and module wiring. Consumers should normally resolve a
binding during composition/startup and cache the typed table/context for hot
execution.

AF2 is generic enough to compose current and future qualified numerical,
advanced-mathematical, quantum-related, security, HTTP/routing, event/network,
Geometry, Renderer and browser-facing services. It does not manufacture a
public service ABI that is not present and qualified in the source tree.

## Stable identities

Module/bounded-context and capability identities are independent 128-bit
nonzero constants. They are not pointers and are not derived from mutable
storage addresses. Applications assign stable IDs under their own namespace.

Capability versions contain `major` and `minor`:

- provider major must equal required major;
- provider minor must be greater than or equal to the required minor;
- the provider interface table must be at least the required byte size.

The byte-size rule combines with the normal `abi_version`/`struct_size` prefix
pattern inside typed function tables such as the AF1
`arbor_application_capabilities` table.

## Provider and consumer declarations

`arbor_capability_export` publishes:

- capability ID;
- capability version;
- interface-table byte size;
- pointer to the public typed interface table;
- opaque provider context.

The interface pointer is an explicit public ABI table, not permission to expose
private implementation structures. The opaque context is interpreted only by
the provider's typed functions.

`arbor_capability_requirement` declares the capability ID, minimum compatible
version and minimum table size a consumer accepts.

`arbor_module_descriptor` owns immutable arrays of provides/consumes
declarations. Descriptor ABI v1 has an exact fixed size: because modules are a
contiguous typed array, `struct_size` must equal `sizeof(arbor_module_descriptor)`
rather than merely advertising a larger future prefix. A future descriptor size
requires an explicit ABI revision/iteration contract. These arrays and their
referenced interface tables must remain alive and unchanged for the lifetime of
the published catalog. A non-NULL provider context is borrowed provider-owned
state and must outlive every cached binding use that can invoke its interface.

## Composition and publication

AF2 uses no hidden heap allocation. The composition root supplies:

- persistent binding storage;
- persistent requirement-resolution storage;
- persistent provider-before-consumer module-order storage;
- scratch workspace for dependency resolution and cycle detection.

`arbor_capability_catalog_measure()` computes the required persistent counts.
Count accumulation delegates overflow detection to the qualified Assembly
`u64_add_checked` primitive rather than duplicating unchecked arithmetic.

`arbor_capability_catalog_prepare()` validates all descriptors, dependencies,
versions and capacities before writing any persistent publication storage. Only
a fully valid, acyclic graph is published. The catalog output and persistent
storage therefore remain unchanged on expected prepare failure. Scratch
workspace may change and has no publication semantics. Publication arrays, scratch
arrays and the catalog output must be disjoint from each other and from immutable
module/declaration/interface-table regions; AF2 validates these ranges using the
qualified lower range primitives before scratch or persistent writes begin.

After successful publication, the module descriptors, provided/consumed arrays,
interface tables and persistent catalog arrays are immutable for the catalog
lifetime. `arbor_capability_catalog_validate()` can detect structural drift or
corruption, including binding/resolution mismatch and any module order that is
not the canonical provider-before-consumer order with the catalog-index
tie-break. A merely topologically valid but non-canonical permutation is invalid.

## Deterministic dependency semantics

AF2 rejects:

- duplicate module IDs with `-EEXIST`;
- duplicate capability providers with `-EEXIST`;
- duplicate requirements for one capability within a module with `-EEXIST`;
- missing required capabilities with `-ENOENT`;
- incompatible major/minor/table-size requirements with `-EPROTONOSUPPORT`;
- cyclic/self dependency with `-ELOOP`;
- malformed descriptors/publication objects with `-EINVAL`;
- insufficient caller storage/workspace with `-ENOSPC`;
- count overflow with `-EOVERFLOW`.

The prepared module order is provider-before-consumer. When multiple modules are
ready simultaneously, the lower original catalog index wins. This makes startup
ordering deterministic without inventing application priority policy.

## Bidirectional qualification

The AF2 native test intentionally composes modules in reverse dependency order.
It proves both directions using existing qualified interfaces:

1. a higher application module resolves a lower checked-arithmetic capability
   and calls the existing Assembly `u64_add_checked` implementation through a
   typed table;
2. a lower/runtime-like module resolves the higher AF1
   `arbor_application_capabilities` table and invokes the registered application
   callback through `arbor_application_invoke()`.

The kernel itself stays generic; the test demonstrates how typed capability
ABIs sit on top of it.

## Ownership and boundaries

All descriptors, bindings and function-table pointers are same-process borrowed
references. AF2 does not authorize raw pointers to cross HTTP, database, R,
WASM or other process/runtime boundaries. Those boundaries continue to use
serialized bytes, stable handles or their own qualified contracts.

Application-specific domain rules, controller policy, SQL, schemas and
repository implementations never become capability-kernel primitives.

## Deferred work

AF2 deliberately does not implement:

- concrete application use cases/services (AF3);
- domain-event, port, repository or transaction semantics (AF4 and later);
- MVC route catalogs, controllers, middleware or presenters;
- MariaDB, R or deployment;
- wrappers around every existing direct Arborcore API.

If a later phase proves that an existing qualified lower service requires a
better public capability surface or upward callback hook, that layer is reopened
only through the controlled-retrofit policy and complete affected-layer
requalification.
