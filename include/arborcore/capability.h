#ifndef ARBORCORE_CAPABILITY_H
#define ARBORCORE_CAPABILITY_H

#include <stdint.h>

#include <arborcore/arborcore.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ARBOR_CAPABILITY_KERNEL_ABI_VERSION 1u
#define ARBOR_MODULE_DESCRIPTOR_ABI_VERSION 1u
#define ARBOR_CAPABILITY_FLAGS_NONE UINT64_C(0)
#define ARBOR_MODULE_FLAGS_NONE UINT64_C(0)
#define ARBOR_CAPABILITY_CATALOG_FLAGS_NONE UINT64_C(0)

/* Stable 128-bit identities. Zero is reserved as an invalid/uninitialized ID. */
typedef struct arbor_module_id {
    uint64_t high;
    uint64_t low;
} arbor_module_id;

typedef struct arbor_capability_id {
    uint64_t high;
    uint64_t low;
} arbor_capability_id;

/* Major must match exactly; provider minor must be >= required minor. */
typedef struct arbor_capability_version {
    uint32_t major;
    uint32_t minor;
} arbor_capability_version;

/*
 * A provided capability publishes a versioned typed interface table plus an
 * opaque provider context. The interface table is public ABI, not private
 * implementation memory. Provider context may be NULL when the interface is
 * stateless. A non-NULL context is borrowed provider-owned state and must remain
 * valid for every cached binding use that can reach the provider interface.
 */
typedef struct arbor_capability_export {
    arbor_capability_id id;
    arbor_capability_version version;
    uint32_t interface_size;
    uint32_t reserved0;
    uint64_t flags;
    const void *interface_table;
    void *provider_context;
} arbor_capability_export;

/* A consumed capability declares the minimum interface it can accept. */
typedef struct arbor_capability_requirement {
    arbor_capability_id id;
    arbor_capability_version minimum_version;
    uint32_t minimum_interface_size;
    uint32_t reserved0;
    uint64_t flags;
} arbor_capability_requirement;

/*
 * Immutable-after-publication module/bounded-context descriptor. ABI v1 uses
 * an exact fixed-size descriptor because descriptors are a contiguous typed
 * array; struct_size must therefore equal sizeof(arbor_module_descriptor). The
 * arrays and every interface table referenced by them must outlive the catalog.
 */
typedef struct arbor_module_descriptor {
    arbor_module_id id;
    uint32_t abi_version;
    uint32_t struct_size;
    uint64_t flags;
    const arbor_capability_export *provides;
    uint64_t provides_count;
    const arbor_capability_requirement *consumes;
    uint64_t consumes_count;
} arbor_module_descriptor;

/* Published provider record. Consumers cache this rather than hot-path lookup. */
typedef struct arbor_capability_binding {
    arbor_capability_id id;
    arbor_capability_version version;
    uint32_t interface_size;
    uint32_t reserved0;
    uint64_t flags;
    const void *interface_table;
    void *provider_context;
    uint64_t provider_module_index;
} arbor_capability_binding;

/* Deterministic mapping of one module requirement to one binding. */
typedef struct arbor_capability_resolution {
    uint64_t consumer_module_index;
    uint64_t requirement_index;
    uint64_t binding_index;
} arbor_capability_resolution;

typedef struct arbor_capability_catalog_requirements {
    uint64_t module_count;
    uint64_t binding_count;
    uint64_t resolution_count;
} arbor_capability_catalog_requirements;

/* Caller-owned persistent publication storage. */
typedef struct arbor_capability_catalog_storage {
    arbor_capability_binding *bindings;
    uint64_t binding_capacity;
    arbor_capability_resolution *resolutions;
    uint64_t resolution_capacity;
    uint64_t *module_order;
    uint64_t module_order_capacity;
} arbor_capability_catalog_storage;

/*
 * Caller-owned scratch. It may be modified on failed prepare; persistent
 * storage and the catalog output remain transactional.
 */
typedef struct arbor_capability_workspace {
    uint64_t *resolved_provider_module_indices;
    uint64_t *resolved_binding_indices;
    uint64_t resolution_capacity;
    uint64_t *indegree;
    uint8_t *selected;
    uint64_t *module_order;
    uint64_t module_capacity;
} arbor_capability_workspace;

/*
 * Published catalog. All pointers are borrowed from composition-root-owned
 * immutable storage. Capability lookup is a composition/startup operation;
 * hot consumers should cache the returned typed binding.
 */
typedef struct arbor_capability_catalog {
    uint32_t abi_version;
    uint32_t struct_size;
    uint64_t flags;
    const arbor_module_descriptor *modules;
    uint64_t module_count;
    const arbor_capability_binding *bindings;
    uint64_t binding_count;
    const arbor_capability_resolution *resolutions;
    uint64_t resolution_count;
    const uint64_t *module_order;
} arbor_capability_catalog;

arbor_status arbor_capability_catalog_measure(
    const arbor_module_descriptor *modules,
    uint64_t module_count,
    arbor_capability_catalog_requirements *out);

arbor_status arbor_capability_catalog_prepare(
    const arbor_module_descriptor *modules,
    uint64_t module_count,
    arbor_capability_catalog_storage *storage,
    arbor_capability_workspace *workspace,
    arbor_capability_catalog *out);

arbor_status arbor_capability_catalog_validate(
    const arbor_capability_catalog *catalog);

arbor_status arbor_capability_catalog_find_module(
    const arbor_capability_catalog *catalog,
    arbor_module_id id,
    uint64_t *module_index_out);

arbor_status arbor_capability_catalog_find(
    const arbor_capability_catalog *catalog,
    arbor_capability_id id,
    arbor_capability_version minimum_version,
    uint32_t minimum_interface_size,
    arbor_capability_binding *binding_out);

arbor_status arbor_capability_catalog_resolve(
    const arbor_capability_catalog *catalog,
    uint64_t consumer_module_index,
    uint64_t requirement_index,
    arbor_capability_binding *binding_out);

#ifdef __cplusplus
}
#endif

#endif
