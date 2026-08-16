#ifndef ARBORCORE_APPLICATION_SERVICE_H
#define ARBORCORE_APPLICATION_SERVICE_H

#include <stdint.h>

#include <arborcore/capability.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ARBOR_APPLICATION_SERVICE_INTERFACE_ABI_VERSION 1u
#define ARBOR_APPLICATION_SERVICE_MODULE_DESCRIPTOR_ABI_VERSION 1u
#define ARBOR_APPLICATION_RUNTIME_ABI_VERSION 1u

#define ARBOR_APPLICATION_SERVICE_INTERFACE_FLAGS_NONE UINT64_C(0)
#define ARBOR_APPLICATION_SERVICE_MODULE_FLAGS_NONE UINT64_C(0)
#define ARBOR_APPLICATION_RUNTIME_FLAGS_NONE UINT64_C(0)

/*
 * Common prefix for every AF3-managed typed Application-service interface.
 * The complete typed interface table is provider-owned and immutable while a
 * binding can be used. struct_size is the complete provider table size and
 * must exactly equal the AF2 capability export/binding interface_size.
 */
typedef struct arbor_application_service_interface_header {
    uint32_t abi_version;
    uint32_t struct_size;
    uint64_t flags;
} arbor_application_service_interface_header;

typedef struct arbor_application_service_prepare_context
    arbor_application_service_prepare_context;

typedef int64_t (*arbor_application_service_prepare_fn)(
    void *module_context,
    const arbor_application_service_prepare_context *prepare_context);

typedef void (*arbor_application_service_rollback_fn)(void *module_context);
typedef int64_t (*arbor_application_service_stop_fn)(void *module_context);

/*
 * One AF3 lifecycle descriptor manages one AF2 module. A managed module may
 * declare several of its AF2 provides[] entries as typed Application-service
 * exports. The lifecycle module_context is distinct from each AF2 capability
 * export's provider_context.
 */
typedef struct arbor_application_service_module_descriptor {
    arbor_module_id module_id;
    uint32_t abi_version;
    uint32_t struct_size;
    uint64_t flags;
    void *module_context;
    const uint64_t *service_export_indices;
    uint64_t service_export_count;
    arbor_application_service_prepare_fn prepare;
    arbor_application_service_rollback_fn rollback;
    arbor_application_service_stop_fn stop;
} arbor_application_service_module_descriptor;

typedef enum arbor_application_runtime_state {
    ARBOR_APPLICATION_RUNTIME_READY = 1,
    ARBOR_APPLICATION_RUNTIME_STOPPING = 2,
    ARBOR_APPLICATION_RUNTIME_STOPPED = 3,
    ARBOR_APPLICATION_RUNTIME_STOP_FAILED = 4
} arbor_application_runtime_state;

typedef enum arbor_application_service_record_state {
    ARBOR_APPLICATION_SERVICE_RECORD_EMPTY = 0,
    ARBOR_APPLICATION_SERVICE_RECORD_PREPARING = 1,
    ARBOR_APPLICATION_SERVICE_RECORD_READY = 2,
    ARBOR_APPLICATION_SERVICE_RECORD_ROLLING_BACK = 3,
    ARBOR_APPLICATION_SERVICE_RECORD_ROLLED_BACK = 4,
    ARBOR_APPLICATION_SERVICE_RECORD_STOPPING = 5,
    ARBOR_APPLICATION_SERVICE_RECORD_STOPPED = 6,
    ARBOR_APPLICATION_SERVICE_RECORD_STOP_FAILED = 7,
    ARBOR_APPLICATION_SERVICE_RECORD_PREPARE_FAILED = 8
} arbor_application_service_record_state;

typedef struct arbor_application_service_runtime_record {
    uint64_t descriptor_index;
    uint64_t module_index;
    uint32_t state;
    uint32_t reserved0;
    int64_t stop_native;
} arbor_application_service_runtime_record;

typedef struct arbor_application_runtime_requirements {
    uint64_t managed_module_record_count;
    uint64_t module_map_count;
} arbor_application_runtime_requirements;

/* Caller-owned persistent publication storage. */
typedef struct arbor_application_runtime_storage {
    arbor_application_service_runtime_record *records;
    uint64_t record_capacity;
} arbor_application_runtime_storage;

/* Caller-owned scratch. May change when runtime preparation fails. */
typedef struct arbor_application_runtime_workspace {
    arbor_application_service_runtime_record *records;
    uint64_t record_capacity;
    uint64_t *module_to_descriptor_index;
    uint64_t module_capacity;
} arbor_application_runtime_workspace;

/*
 * Published AF3 runtime. All pointer targets are borrowed and must outlive the
 * runtime and every cached typed binding use. A READY runtime is immutable
 * except through the externally serialized stop lifecycle operation.
 */
typedef struct arbor_application_runtime {
    uint32_t abi_version;
    uint32_t struct_size;
    uint64_t flags;
    const arbor_capability_catalog *catalog;
    const arbor_application_service_module_descriptor *service_modules;
    uint64_t managed_module_count;
    arbor_application_service_runtime_record *records;
    uint32_t state;
    uint32_t reserved0;
} arbor_application_runtime;

/* Normalize native callback/use-case status: 0, negative errno, positive invalid. */
arbor_status arbor_application_service_status_from_native(int64_t native);

/* Validate the AF3 common typed-service interface prefix of an AF2 binding. */
arbor_status arbor_application_service_interface_validate(
    const arbor_capability_binding *binding);

/*
 * Measure caller-owned AF3 persistent/scratch capacities without publication.
 * out must not overlap known immutable catalog/service metadata.
 */
arbor_status arbor_application_runtime_measure(
    const arbor_capability_catalog *catalog,
    const arbor_application_service_module_descriptor *service_modules,
    uint64_t managed_module_count,
    arbor_application_runtime_requirements *out);

/*
 * Prepare managed modules in filtered AF2 canonical order. Publication is
 * transactional: on failure persistent storage and runtime_out are unchanged.
 */
arbor_status arbor_application_runtime_prepare(
    const arbor_capability_catalog *catalog,
    const arbor_application_service_module_descriptor *service_modules,
    uint64_t managed_module_count,
    arbor_application_runtime_storage *storage,
    arbor_application_runtime_workspace *workspace,
    arbor_application_runtime *runtime_out);

/* Validate a published or terminal AF3 runtime and its state-specific records. */
arbor_status arbor_application_runtime_validate(
    const arbor_application_runtime *runtime);

/*
 * Resolve one AF2 requirement during a managed module's prepare callback.
 * Managed providers must already be READY. binding_out is unchanged on error
 * and must not overlap the prepare context/workspace or known immutable inputs.
 */
arbor_status arbor_application_service_prepare_resolve(
    const arbor_application_service_prepare_context *prepare_context,
    uint64_t requirement_index,
    arbor_capability_binding *binding_out);

/*
 * Find one declared managed typed Application service while runtime is READY.
 * Intended for composition/adapter initialization; cache the returned binding.
 * binding_out must not overlap the runtime, records, or known immutable inputs.
 */
arbor_status arbor_application_runtime_find_ready(
    const arbor_application_runtime *runtime,
    arbor_capability_id id,
    arbor_capability_version minimum_version,
    uint32_t minimum_interface_size,
    arbor_capability_binding *binding_out);

/*
 * Stop all managed modules in reverse effective preparation order. The full
 * runtime is validated before state-specific EBUSY/EALREADY is returned.
 * Cleanup continues after individual stop failures; the first normalized
 * failure is returned after every module has been attempted.
 */
arbor_status arbor_application_runtime_stop(arbor_application_runtime *runtime);

#ifdef __cplusplus
}
#endif

#endif
