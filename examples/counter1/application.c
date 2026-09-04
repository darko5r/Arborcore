#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "counter1.h"

#define COUNTER1_APPLICATION_GUARD UINT64_C(0x434e545231415050)
#define COUNTER1_SERVICE_GUARD UINT64_C(0x434e545231535643)
#define COUNTER1_ID_HIGH UINT64_C(0x434f554e54455231)
#define COUNTER1_PROVIDER_MODULE_LOW UINT64_C(0x0000000000000001)
#define COUNTER1_SERVICE_MODULE_LOW UINT64_C(0x0000000000000002)
#define COUNTER1_REPOSITORY_CAPABILITY_LOW UINT64_C(0x0000000000001001)
#define COUNTER1_TRANSACTION_CAPABILITY_LOW UINT64_C(0x0000000000001002)
#define COUNTER1_SERVICE_CAPABILITY_LOW UINT64_C(0x0000000000001003)
#define COUNTER1_INCREMENT_EVENT_LOW UINT64_C(0x0000000000003001)
#define COUNTER1_EVENT_RECORD_CAPACITY 1u
#define COUNTER1_EVENT_PAYLOAD_CAPACITY 16u

static int64_t counter1_service_get(
    void *provider_context,
    const counter1_service_request *request,
    counter1_service_result *result_out);
static int64_t counter1_service_increment(
    void *provider_context,
    const counter1_service_request *request,
    counter1_service_result *result_out);
static int64_t counter1_service_prepare_callback(
    void *module_context,
    const arbor_application_service_prepare_context *prepare_context);
static void counter1_service_rollback_callback(void *module_context);
static int64_t counter1_service_stop_callback(void *module_context);

static const counter1_service_v1 counter1_service_interface = {
    {
        ARBOR_APPLICATION_SERVICE_INTERFACE_ABI_VERSION,
        (uint32_t)sizeof(counter1_service_v1),
        ARBOR_APPLICATION_SERVICE_INTERFACE_FLAGS_NONE
    },
    counter1_service_get,
    counter1_service_increment
};

static arbor_capability_id counter1_capability_id(uint64_t low)
{
    return (arbor_capability_id){COUNTER1_ID_HIGH, low};
}

static arbor_module_id counter1_module_id(uint64_t low)
{
    return (arbor_module_id){COUNTER1_ID_HIGH, low};
}

static int counter1_capability_id_equal(
    arbor_capability_id left,
    arbor_capability_id right)
{
    return left.high == right.high && left.low == right.low;
}

static arbor_status counter1_repository_binding_validate(
    const arbor_capability_binding *binding)
{
    if (binding == NULL ||
        !counter1_capability_id_equal(
            binding->id,
            counter1_capability_id(COUNTER1_REPOSITORY_CAPABILITY_LOW)) ||
        binding->version.major != 1u || binding->version.minor != 0u ||
        binding->interface_size != (uint32_t)sizeof(counter1_repository_v1) ||
        binding->flags != ARBOR_CAPABILITY_FLAGS_NONE ||
        binding->interface_table == NULL || binding->provider_context == NULL) {
        return arbor_status_from_native(-EINVAL);
    }
    const counter1_repository_v1 *repository =
        (const counter1_repository_v1 *)binding->interface_table;
    if (repository->abi_version != COUNTER1_REPOSITORY_ABI_VERSION ||
        repository->struct_size != (uint32_t)sizeof(counter1_repository_v1) ||
        repository->flags != COUNTER1_REPOSITORY_FLAGS_NONE ||
        repository->get == NULL || repository->increment == NULL) {
        return arbor_status_from_native(-EINVAL);
    }
    return arbor_status_from_native(0);
}

static arbor_status counter1_service_binding_validate(
    const arbor_capability_binding *binding)
{
    if (binding == NULL ||
        !counter1_capability_id_equal(
            binding->id,
            counter1_capability_id(COUNTER1_SERVICE_CAPABILITY_LOW)) ||
        binding->version.major != 1u || binding->version.minor != 0u ||
        binding->interface_size != (uint32_t)sizeof(counter1_service_v1) ||
        binding->flags != ARBOR_CAPABILITY_FLAGS_NONE ||
        binding->provider_context == NULL) {
        return arbor_status_from_native(-EINVAL);
    }
    arbor_status status = arbor_application_service_interface_validate(binding);
    if (status.native != 0) {
        return status;
    }
    const counter1_service_v1 *service =
        (const counter1_service_v1 *)binding->interface_table;
    if (service == NULL || service->get_counter == NULL ||
        service->increment_counter == NULL) {
        return arbor_status_from_native(-EINVAL);
    }
    return arbor_status_from_native(0);
}

static arbor_capability_export counter1_make_export(
    arbor_capability_id id,
    const void *interface_table,
    uint32_t interface_size,
    void *provider_context)
{
    return (arbor_capability_export){
        id,
        {1u, 0u},
        interface_size,
        0u,
        ARBOR_CAPABILITY_FLAGS_NONE,
        interface_table,
        provider_context
    };
}

static arbor_module_descriptor counter1_make_module(
    arbor_module_id id,
    const arbor_capability_export *provides,
    uint64_t provides_count,
    const arbor_capability_requirement *consumes,
    uint64_t consumes_count)
{
    return (arbor_module_descriptor){
        id,
        ARBOR_MODULE_DESCRIPTOR_ABI_VERSION,
        (uint32_t)sizeof(arbor_module_descriptor),
        ARBOR_MODULE_FLAGS_NONE,
        provides,
        provides_count,
        consumes,
        consumes_count
    };
}

static int64_t counter1_service_prepare_callback(
    void *module_context,
    const arbor_application_service_prepare_context *prepare_context)
{
    counter1_service_module_state *state =
        (counter1_service_module_state *)module_context;
    if (state == NULL || prepare_context == NULL) {
        return -EINVAL;
    }

    arbor_capability_binding repository = {0};
    arbor_capability_binding transaction = {0};
    arbor_status status = arbor_application_service_prepare_resolve(
        prepare_context, 0u, &repository);
    if (status.native != 0) {
        return status.native;
    }
    status = counter1_repository_binding_validate(&repository);
    if (status.native != 0) {
        return status.native;
    }
    status = arbor_application_service_prepare_resolve(
        prepare_context, 1u, &transaction);
    if (status.native != 0) {
        return status.native;
    }
    status = arbor_ddd_transaction_interface_validate(&transaction);
    if (status.native != 0) {
        return status.native;
    }

    state->repository_binding = repository;
    state->transaction_binding = transaction;
    state->prepared_guard = COUNTER1_SERVICE_GUARD;
    return 0;
}

static void counter1_service_rollback_callback(void *module_context)
{
    counter1_service_module_state *state =
        (counter1_service_module_state *)module_context;
    if (state != NULL) {
        (void)memset(&state->repository_binding, 0, sizeof(state->repository_binding));
        (void)memset(&state->transaction_binding, 0, sizeof(state->transaction_binding));
        state->prepared_guard = 0u;
    }
}

static int64_t counter1_service_stop_callback(void *module_context)
{
    counter1_service_module_state *state =
        (counter1_service_module_state *)module_context;
    if (state == NULL || state->prepared_guard != COUNTER1_SERVICE_GUARD) {
        return -EINVAL;
    }
    (void)memset(&state->repository_binding, 0, sizeof(state->repository_binding));
    (void)memset(&state->transaction_binding, 0, sizeof(state->transaction_binding));
    state->prepared_guard = 0u;
    return 0;
}

static arbor_status counter1_service_request_validate(
    const counter1_service_provider_context *provider,
    const counter1_service_request *request,
    const counter1_service_result *result_out)
{
    if (provider == NULL || provider->module == NULL ||
        provider->module->prepared_guard != COUNTER1_SERVICE_GUARD ||
        request == NULL || request->arena == NULL || result_out == NULL) {
        return arbor_status_from_native(-EINVAL);
    }
    arbor_status status = counter1_repository_binding_validate(
        &provider->module->repository_binding);
    if (status.native != 0) {
        return status;
    }
    return arbor_ddd_transaction_interface_validate(
        &provider->module->transaction_binding);
}

static arbor_status counter1_uow_begin(
    counter1_service_module_state *module,
    arbor_asm_arena *arena,
    arbor_ddd_event_journal *journal_out,
    arbor_ddd_unit_of_work *uow_out)
{
    arbor_asm_result_ptr state = arena_alloc_aligned(
        arena,
        (uint64_t)sizeof(counter1_transaction_state),
        (uint64_t)_Alignof(counter1_transaction_state));
    arbor_asm_result_ptr records = arena_alloc_aligned(
        arena,
        (uint64_t)sizeof(arbor_ddd_event_record),
        (uint64_t)_Alignof(arbor_ddd_event_record));
    arbor_asm_result_ptr payload = arena_alloc_aligned(
        arena,
        COUNTER1_EVENT_PAYLOAD_CAPACITY,
        8u);
    if (state.status != 0 || records.status != 0 || payload.status != 0) {
        return arbor_status_from_native(-ENOSPC);
    }

    arbor_status status = arbor_ddd_event_journal_init(
        (arbor_ddd_event_record *)records.value,
        COUNTER1_EVENT_RECORD_CAPACITY,
        (uint8_t *)payload.value,
        COUNTER1_EVENT_PAYLOAD_CAPACITY,
        journal_out);
    if (status.native != 0) {
        return status;
    }
    return arbor_ddd_unit_of_work_begin(
        &module->transaction_binding,
        state.value,
        (uint64_t)sizeof(counter1_transaction_state),
        journal_out,
        uow_out);
}

static int64_t counter1_service_get(
    void *provider_context,
    const counter1_service_request *request,
    counter1_service_result *result_out)
{
    counter1_service_provider_context *provider =
        (counter1_service_provider_context *)provider_context;
    arbor_status status = counter1_service_request_validate(
        provider, request, result_out);
    if (status.native != 0) {
        return status.native;
    }

    arbor_ddd_event_journal journal = {0};
    arbor_ddd_unit_of_work uow = {0};
    status = counter1_uow_begin(
        provider->module, request->arena, &journal, &uow);
    if (status.native != 0) {
        return status.native;
    }

    arbor_ddd_transaction_view transaction = {0};
    status = arbor_ddd_unit_of_work_active_transaction(&uow, &transaction);
    if (status.native != 0) {
        (void)arbor_ddd_unit_of_work_rollback(&uow);
        return status.native;
    }

    const counter1_repository_v1 *repository =
        (const counter1_repository_v1 *)
            provider->module->repository_binding.interface_table;
    counter1_repository_get_result repository_result = {0};
    const int64_t native = repository->get(
        provider->module->repository_binding.provider_context,
        &transaction,
        request->id,
        &repository_result);
    if (native != 0) {
        arbor_status rollback_status = arbor_ddd_unit_of_work_rollback(&uow);
        return rollback_status.native != 0 ? rollback_status.native : native;
    }

    counter1_service_result candidate = {0};
    switch (repository_result.outcome_code) {
    case COUNTER1_REPOSITORY_GET_FOUND:
        candidate = (counter1_service_result){
            (uint32_t)COUNTER1_SERVICE_FOUND,
            0u,
            repository_result.id,
            repository_result.value
        };
        break;
    case COUNTER1_REPOSITORY_GET_NOT_FOUND:
        candidate = (counter1_service_result){
            (uint32_t)COUNTER1_SERVICE_NOT_FOUND,
            0u,
            repository_result.id,
            0u
        };
        break;
    default:
        (void)arbor_ddd_unit_of_work_rollback(&uow);
        return -EINVAL;
    }

    status = arbor_ddd_unit_of_work_commit(&uow);
    if (status.native != 0) {
        return status.native;
    }
    *result_out = candidate;
    return 0;
}

static int64_t counter1_service_increment(
    void *provider_context,
    const counter1_service_request *request,
    counter1_service_result *result_out)
{
    counter1_service_provider_context *provider =
        (counter1_service_provider_context *)provider_context;
    arbor_status status = counter1_service_request_validate(
        provider, request, result_out);
    if (status.native != 0) {
        return status.native;
    }

    arbor_ddd_event_journal journal = {0};
    arbor_ddd_unit_of_work uow = {0};
    status = counter1_uow_begin(
        provider->module, request->arena, &journal, &uow);
    if (status.native != 0) {
        return status.native;
    }

    arbor_ddd_transaction_view transaction = {0};
    status = arbor_ddd_unit_of_work_active_transaction(&uow, &transaction);
    if (status.native != 0) {
        (void)arbor_ddd_unit_of_work_rollback(&uow);
        return status.native;
    }

    const counter1_repository_v1 *repository =
        (const counter1_repository_v1 *)
            provider->module->repository_binding.interface_table;
    counter1_repository_increment_result repository_result = {0};
    const int64_t native = repository->increment(
        provider->module->repository_binding.provider_context,
        &transaction,
        request->id,
        &repository_result);
    if (native != 0) {
        arbor_status rollback_status = arbor_ddd_unit_of_work_rollback(&uow);
        return rollback_status.native != 0 ? rollback_status.native : native;
    }

    counter1_service_result candidate = {0};
    if (repository_result.outcome_code ==
        (uint32_t)COUNTER1_REPOSITORY_INCREMENTED) {
        counter1_incremented_event_v1 event = {
            repository_result.id,
            repository_result.value
        };
        uint32_t sequence = UINT32_MAX;
        status = arbor_ddd_event_journal_append(
            &journal,
            (arbor_ddd_event_type_id){
                COUNTER1_ID_HIGH,
                COUNTER1_INCREMENT_EVENT_LOW
            },
            1u,
            ARBOR_DDD_EVENT_FLAGS_NONE,
            &event,
            (uint32_t)sizeof(event),
            &sequence);
        if (status.native != 0 || sequence != 0u) {
            arbor_status rollback_status = arbor_ddd_unit_of_work_rollback(&uow);
            if (rollback_status.native != 0) {
                return rollback_status.native;
            }
            return status.native != 0 ? status.native : -EINVAL;
        }
        candidate = (counter1_service_result){
            (uint32_t)COUNTER1_SERVICE_INCREMENTED,
            0u,
            repository_result.id,
            repository_result.value
        };
        status = arbor_ddd_unit_of_work_commit(&uow);
        if (status.native != 0) {
            return status.native;
        }
        *result_out = candidate;
        return 0;
    }

    if (repository_result.outcome_code ==
        (uint32_t)COUNTER1_REPOSITORY_INCREMENT_NOT_FOUND) {
        candidate = (counter1_service_result){
            (uint32_t)COUNTER1_SERVICE_NOT_FOUND,
            0u,
            repository_result.id,
            0u
        };
    } else if (repository_result.outcome_code ==
               (uint32_t)COUNTER1_REPOSITORY_INCREMENT_LIMIT_REACHED) {
        candidate = (counter1_service_result){
            (uint32_t)COUNTER1_SERVICE_LIMIT_REACHED,
            0u,
            repository_result.id,
            repository_result.value
        };
    } else {
        (void)arbor_ddd_unit_of_work_rollback(&uow);
        return -EINVAL;
    }

    status = arbor_ddd_unit_of_work_rollback(&uow);
    if (status.native != 0) {
        return status.native;
    }
    *result_out = candidate;
    return 0;
}

arbor_status counter1_application_prepare(
    const counter1_repository_provider *provider,
    counter1_application *out)
{
    if (provider == NULL || out == NULL) {
        return arbor_status_from_native(-EINVAL);
    }
    arbor_status status = counter1_repository_provider_validate(provider);
    if (status.native != 0) {
        return status;
    }

    (void)memset(out, 0, sizeof(*out));
    out->provider = *provider;
    out->service_provider_context.module = &out->service_module_state;

    out->provider_exports[0] = counter1_make_export(
        counter1_capability_id(COUNTER1_REPOSITORY_CAPABILITY_LOW),
        provider->repository,
        (uint32_t)sizeof(counter1_repository_v1),
        provider->provider_context);
    out->provider_exports[1] = counter1_make_export(
        counter1_capability_id(COUNTER1_TRANSACTION_CAPABILITY_LOW),
        provider->transaction,
        (uint32_t)sizeof(arbor_ddd_transaction_interface),
        provider->provider_context);
    out->service_exports[0] = counter1_make_export(
        counter1_capability_id(COUNTER1_SERVICE_CAPABILITY_LOW),
        &counter1_service_interface,
        (uint32_t)sizeof(counter1_service_v1),
        &out->service_provider_context);

    out->service_requirements[0] = (arbor_capability_requirement){
        counter1_capability_id(COUNTER1_REPOSITORY_CAPABILITY_LOW),
        {1u, 0u},
        (uint32_t)sizeof(counter1_repository_v1),
        0u,
        ARBOR_CAPABILITY_FLAGS_NONE
    };
    out->service_requirements[1] = (arbor_capability_requirement){
        counter1_capability_id(COUNTER1_TRANSACTION_CAPABILITY_LOW),
        {1u, 0u},
        (uint32_t)sizeof(arbor_ddd_transaction_interface),
        0u,
        ARBOR_CAPABILITY_FLAGS_NONE
    };

    out->modules[0] = counter1_make_module(
        counter1_module_id(COUNTER1_PROVIDER_MODULE_LOW),
        out->provider_exports,
        2u,
        NULL,
        0u);
    out->modules[1] = counter1_make_module(
        counter1_module_id(COUNTER1_SERVICE_MODULE_LOW),
        out->service_exports,
        1u,
        out->service_requirements,
        2u);

    arbor_capability_catalog_requirements catalog_requirements = {0};
    status = arbor_capability_catalog_measure(
        out->modules, 2u, &catalog_requirements);
    if (status.native != 0 || catalog_requirements.module_count != 2u ||
        catalog_requirements.binding_count != 3u ||
        catalog_requirements.resolution_count != 2u) {
        return status.native != 0 ? status : arbor_status_from_native(-EINVAL);
    }

    arbor_capability_catalog_storage catalog_storage = {
        out->catalog_bindings,
        3u,
        out->catalog_resolutions,
        2u,
        out->catalog_module_order,
        2u
    };
    uint64_t resolved_provider_indices[2] = {0u, 0u};
    uint64_t resolved_binding_indices[2] = {0u, 0u};
    uint64_t indegree[2] = {0u, 0u};
    uint8_t selected[2] = {0u, 0u};
    uint64_t workspace_order[2] = {0u, 0u};
    arbor_capability_workspace catalog_workspace = {
        resolved_provider_indices,
        resolved_binding_indices,
        2u,
        indegree,
        selected,
        workspace_order,
        2u
    };
    status = arbor_capability_catalog_prepare(
        out->modules,
        2u,
        &catalog_storage,
        &catalog_workspace,
        &out->catalog);
    if (status.native != 0) {
        return status;
    }

    out->service_export_indices[0] = 0u;
    out->service_module_descriptor =
        (arbor_application_service_module_descriptor){
            counter1_module_id(COUNTER1_SERVICE_MODULE_LOW),
            ARBOR_APPLICATION_SERVICE_MODULE_DESCRIPTOR_ABI_VERSION,
            (uint32_t)sizeof(arbor_application_service_module_descriptor),
            ARBOR_APPLICATION_SERVICE_MODULE_FLAGS_NONE,
            &out->service_module_state,
            out->service_export_indices,
            1u,
            counter1_service_prepare_callback,
            counter1_service_rollback_callback,
            counter1_service_stop_callback
        };

    arbor_application_runtime_requirements runtime_requirements = {0};
    status = arbor_application_runtime_measure(
        &out->catalog,
        &out->service_module_descriptor,
        1u,
        &runtime_requirements);
    if (status.native != 0 ||
        runtime_requirements.managed_module_record_count != 1u ||
        runtime_requirements.module_map_count != 2u) {
        return status.native != 0 ? status : arbor_status_from_native(-EINVAL);
    }

    arbor_application_service_runtime_record scratch_records[1] = {{0}};
    uint64_t module_map[2] = {0u, 0u};
    arbor_application_runtime_storage runtime_storage = {
        out->runtime_records,
        1u
    };
    arbor_application_runtime_workspace runtime_workspace = {
        scratch_records,
        1u,
        module_map,
        2u
    };
    status = arbor_application_runtime_prepare(
        &out->catalog,
        &out->service_module_descriptor,
        1u,
        &runtime_storage,
        &runtime_workspace,
        &out->runtime);
    if (status.native != 0) {
        return status;
    }

    status = arbor_application_runtime_find_ready(
        &out->runtime,
        counter1_capability_id(COUNTER1_SERVICE_CAPABILITY_LOW),
        (arbor_capability_version){1u, 0u},
        (uint32_t)sizeof(counter1_service_v1),
        &out->service_binding);
    if (status.native != 0 ||
        counter1_service_binding_validate(&out->service_binding).native != 0) {
        (void)arbor_application_runtime_stop(&out->runtime);
        return status.native != 0 ? status : arbor_status_from_native(-EINVAL);
    }

    out->prepared_guard = COUNTER1_APPLICATION_GUARD;
    status = counter1_application_validate(out);
    if (status.native != 0) {
        (void)arbor_application_runtime_stop(&out->runtime);
        out->prepared_guard = 0u;
    }
    return status;
}

arbor_status counter1_application_validate(
    const counter1_application *application)
{
    if (application == NULL ||
        application->prepared_guard != COUNTER1_APPLICATION_GUARD ||
        application->service_provider_context.module !=
            &application->service_module_state ||
        application->service_module_state.prepared_guard != COUNTER1_SERVICE_GUARD ||
        application->catalog.modules != application->modules ||
        application->catalog.module_count != 2u ||
        application->catalog.bindings != application->catalog_bindings ||
        application->catalog.binding_count != 3u ||
        application->catalog.resolutions != application->catalog_resolutions ||
        application->catalog.resolution_count != 2u ||
        application->runtime.state != (uint32_t)ARBOR_APPLICATION_RUNTIME_READY) {
        return arbor_status_from_native(-EINVAL);
    }

    arbor_status status = counter1_repository_provider_validate(&application->provider);
    if (status.native != 0) {
        return status;
    }
    status = arbor_capability_catalog_validate(&application->catalog);
    if (status.native != 0) {
        return status;
    }
    status = arbor_application_runtime_validate(&application->runtime);
    if (status.native != 0) {
        return status;
    }
    status = counter1_repository_binding_validate(
        &application->service_module_state.repository_binding);
    if (status.native != 0) {
        return status;
    }
    status = arbor_ddd_transaction_interface_validate(
        &application->service_module_state.transaction_binding);
    if (status.native != 0) {
        return status;
    }
    return counter1_service_binding_validate(&application->service_binding);
}

arbor_status counter1_application_stop(counter1_application *application)
{
    if (application == NULL ||
        application->prepared_guard != COUNTER1_APPLICATION_GUARD) {
        return arbor_status_from_native(-EINVAL);
    }
    arbor_status status = arbor_application_runtime_stop(&application->runtime);
    if (status.native == 0) {
        (void)memset(&application->service_binding, 0,
                     sizeof(application->service_binding));
        application->prepared_guard = 0u;
    }
    return status;
}
