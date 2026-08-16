# Arborcore AF3 — Application Service Runtime, Lifecycle, and Typed Use-Case Model

AF3 connects the frozen AF2 capability/module composition kernel to concrete,
strongly typed Application services. It does not replace AF2 and it does not
introduce a second dependency graph.

## Why AF3 exists

AF2 can answer structural questions such as:

- which module provides a capability;
- whether a provider version satisfies a consumer requirement;
- which binding corresponds to a requirement;
- and which deterministic provider-before-consumer module order is canonical.

Those facts are necessary, but they do not by themselves make an Application
module *ready to execute*. A real service may need to validate and cache its
dependencies, prepare caller-owned or module-owned state, publish itself only
after successful preparation, and shut down in the reverse lifecycle order.
AF3 is that missing runtime connection.

The relationship is therefore:

```text
AF2 module/capability catalog
        |
        | canonical dependency order + bindings
        v
AF3 Application-service lifecycle runtime
        |
        | ready managed modules + cached typed bindings
        v
bounded-context typed use cases
```

## AF2 remains authoritative

AF3 deliberately reuses AF2 for module IDs, capability IDs, capability
major/minor matching, provider selection, requirement resolution, the dependency
graph, canonical topological ordering, and immutable catalog publication.

AF3 never computes a second graph. It filters AF2's already-qualified module
order to the modules that have AF3 lifecycle descriptors.

One AF3 lifecycle descriptor corresponds to one AF2 module. That module may
publish several typed Application-service capabilities.

## Generic control plane, typed data plane

AF3 is generic only where the framework must be generic: lifecycle, readiness,
validation, ownership, dependency preparation, storage, and failure handling.
Business calls remain bounded-context-specific and strongly typed.

A typical typed service method follows this convention:

```c
int64_t create_order(
    void *provider_context,
    const create_order_command *command,
    create_order_result *result_out);
```

On System V AMD64 this naturally maps to:

```text
RDI = provider context
RSI = typed input
RDX = typed output
RAX = native mechanism status
```

AF3 does not define or admit a universal `execute(void *, void *, void *)`
business dispatcher. Such an API would discard compile-time type safety.

The native return remains the mechanism channel:

- `0`: the mechanism completed successfully;
- negative: native mechanism failure;
- positive: reserved/invalid and normalized to `-EINVAL` when processed by the
  AF3 status helper.

A business result such as `ORDER_REJECTED` or `LIMIT_EXCEEDED` belongs in the
typed output. It is not an errno. HTTP/presentation mapping remains outside AF3.

## Typed service interface prefix

Every service capability managed as an AF3 typed service begins with:

```c
typedef struct arbor_application_service_interface_header {
    uint32_t abi_version;
    uint32_t struct_size;
    uint64_t flags;
} arbor_application_service_interface_header;
```

AF3 v1 requires:

- interface ABI version `1`;
- flags `0`;
- `struct_size` to be the complete typed table size;
- and `struct_size` to equal the AF2 binding's `interface_size` exactly.

AF2 capability major/minor negotiation remains separate. The AF3 interface ABI
version describes the common service-table convention, not the semantic version
of the capability itself.

## Module lifecycle descriptor

`arbor_application_service_module_descriptor` is exactly 80 bytes on the
qualified Linux x86-64 ABI. It identifies one AF2 module, declares which
`provides[]` entries are typed AF3 services, and optionally supplies lifecycle
callbacks.

The lifecycle `module_context` and the AF2 `provider_context` are deliberately
different concepts:

- `module_context` belongs to the module lifecycle;
- every service export may have its own AF2 `provider_context` for typed calls.

A module can therefore expose several services with distinct provider contexts
while sharing one prepare/rollback/stop lifecycle.

A descriptor is either:

- **passive**: `prepare`, `rollback`, and `stop` are all `NULL`; or
- **active**: all three callbacks are non-`NULL`.

Partial callback shapes are invalid.

## Preparation

`arbor_application_runtime_prepare()` is transactional with respect to AF3
persistent publication.

The runtime:

1. validates the AF2 catalog;
2. validates all AF3 module descriptors and service-interface prefixes;
3. measures required caller-owned persistent and scratch capacity;
4. validates region disjointness and checked byte ranges;
5. initializes a scratch module-index map with `UINT64_MAX` for unmanaged
   modules;
6. filters AF2's canonical module order to managed AF3 modules;
7. prepares each managed module in that order;
8. lets active prepare callbacks resolve AF2 requirements through
   `arbor_application_service_prepare_resolve()`;
9. requires already-managed providers to be READY before returning their
   dependency binding;
10. rolls back the previously prepared prefix in reverse order if one prepare
    callback fails;
11. copies complete READY records to caller-owned persistent storage only after
    every module succeeds;
12. publishes the runtime in READY state last.

On prepare failure:

- `runtime_out` is unchanged;
- persistent runtime records are unchanged;
- scratch workspace may have changed;
- the currently failing `prepare()` is responsible for being self-cleaning;
- every previously successful active module is rolled back in reverse order;
- and the caller may correct the condition and retry with the same storage.

Rollback is intentionally a no-fail lifecycle *unprepare* operation. It is not a
future business/data transaction rollback API.

## Prepare-time dependency resolution

`arbor_application_service_prepare_resolve()` wraps the real frozen AF2
`arbor_capability_catalog_resolve()` operation for the currently preparing
module.

If the provider is unmanaged by AF3, the valid AF2 structural binding is
accepted. If the provider is AF3-managed, its runtime record must already be
READY. If the resolved export is one of that module's declared typed service
exports, the AF3 common interface prefix is also validated.

The consumer then caches the typed table and provider context in its own
service/module state. Normal business calls do not repeat AF2 catalog lookup.

## Ready service discovery

`arbor_application_runtime_find_ready()` is a composition/adapter-initialization
helper. It only returns capabilities that:

- come from an AF3-managed module;
- are explicitly declared as AF3 service exports;
- have a READY provider record;
- and contain a valid AF3 interface prefix.

Consumers should call it once during composition and cache the returned typed
binding. AF3 intentionally does not interpose on every typed hot-path call.

## Ownership and lifetime

AF3 performs no hidden heap allocation and owns no global mutable registry.

- AF2 catalog: composition-root-owned, borrowed, must outlive AF3 runtime.
- AF3 descriptors/export-index arrays: owner-owned immutable input, must outlive
  runtime.
- Interface tables/provider contexts: AF2 provider-owned, borrowed, must outlive
  every cached binding use.
- Lifecycle module contexts: module/composition-root-owned, must outlive runtime.
- Persistent runtime records: caller-owned.
- Preparation workspace: caller-owned scratch, needed only during preparation.
- Typed inputs: caller-owned and borrowed for the call unless a bounded-context
  API explicitly copies them.
- Typed outputs: caller-owned.

Mutable AF3 metadata/backing regions are checked for overlap with each other and
with known immutable catalog, descriptor, export-index, requirement, and
interface-table regions before preparation writes begin. The smaller public
output APIs follow the same rule: `arbor_application_runtime_measure()`,
`arbor_application_service_prepare_resolve()`, and
`arbor_application_runtime_find_ready()` reject outputs that would overwrite
known immutable catalog/service metadata or AF3 internal runtime/workspace
regions.

Prefix-extensible public objects are range-checked using their claimed
`struct_size`, not merely the v1 C prefix size. This preserves disjointness when
a later compatible runtime or AF2 catalog adds fields beyond the v1 prefix.

Opaque provider/module contexts cannot be range-checked because AF3 does not
know their object sizes; their non-overlap/lifetime remains an explicit caller
contract.

## Zero managed modules

A catalog with no AF3-managed modules is valid. Measurement returns zero
persistent records and zero module-map entries. Preparation publishes a READY
runtime with no service modules or records, and stop transitions it to STOPPED.

## Shutdown

Normal stop requires external quiescence: there must be no in-flight typed
service calls. AF3 v1 does not hide locks or reference counts in the hot path.

`arbor_application_runtime_stop()` validates the complete runtime before
returning state-specific `-EBUSY`/`-EALREADY`. A malformed object therefore
returns its validation failure rather than being accepted merely because its
state word resembles a terminal or in-progress runtime.

For a valid runtime:

1. requires READY for a new stop operation;
2. moves it to STOPPING;
3. walks persistent records in reverse effective preparation order;
4. invokes every active stop callback;
5. marks successful records STOPPED;
6. marks failures STOP_FAILED and retains the raw callback return;
7. continues cleanup even after failures;
8. returns the first normalized mechanism failure after every module has been
   attempted;
9. publishes final runtime state STOPPED or STOP_FAILED.

A STOP_FAILED runtime is terminal for AF3 v1. Stop retry is not admitted.

## Runtime states

Published runtime states:

- READY = 1
- STOPPING = 2
- STOPPED = 3
- STOP_FAILED = 4

Per-module runtime record states also include preparation/rollback transients.
Those transient states are workspace-only and are invalid in a published READY
runtime.

## Assembly/C ABI qualification

AF3 includes a handwritten x86-64 Assembly test object to exercise:

- C -> Assembly prepare callbacks;
- C -> Assembly stop callbacks;
- C -> Assembly typed service calls;
- Assembly -> C typed service calls;
- System V stack alignment;
- and callee-saved register preservation.

The public C source also contains `_Static_assert` checks for every frozen AF3
v1 x86-64 structure size and important field offset.

## Reproducibility mode normalization

AF3 reproducibility archives are deterministic across normal host umask/file-mode
variation. The source archive uses ustar, fixed timestamp/owner/group metadata,
and normalizes member permissions with `u=rwX,go=rX`: non-executable source
files are archived as `0644`, while executable verification tools are archived
as `0755`. This mirrors the Git executable-bit model instead of allowing
incidental host write-permission bits such as `0664`/`0666` or `0775`/`0777`
to change the archive identity. The verifier extracts the archive and checks
every member mode before publishing the reproducibility result.

## Deferred boundaries

AF3 deliberately does **not** implement:

- MVC routes, controllers, middleware, or presenters;
- domain-event transport;
- repository or transaction semantics;
- MariaDB;
- R integration;
- browser redesign;
- deployment.

Those are not skipped. AF3 creates the precise Application runtime connection
that those later layers can depend on without invading Domain or duplicating
AF2.
