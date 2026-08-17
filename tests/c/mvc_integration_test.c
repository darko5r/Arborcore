#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <arborcore/application_service.h>
#include <arborcore/ddd_support.h>
#include <arborcore/mvc.h>

static int fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    return 1;
}

typedef struct integration_tx_state {
    uint64_t begun;
    uint64_t marker;
} integration_tx_state;

typedef struct integration_tx_context {
    uint64_t begin_calls;
    uint64_t commit_calls;
    uint64_t rollback_calls;
    bool fail_commit;
} integration_tx_context;

static int64_t tx_begin(void *provider_context, void *state, uint64_t state_size)
{
    integration_tx_context *context = (integration_tx_context *)provider_context;
    integration_tx_state *tx_state = (integration_tx_state *)state;
    if (context == NULL || tx_state == NULL || state_size != sizeof(*tx_state)) {
        return -EINVAL;
    }
    context->begin_calls += 1u;
    tx_state->begun = 1u;
    tx_state->marker = UINT64_C(0xabc);
    return 0;
}

static int64_t tx_commit(void *provider_context, void *state)
{
    integration_tx_context *context = (integration_tx_context *)provider_context;
    integration_tx_state *tx_state = (integration_tx_state *)state;
    if (context == NULL || tx_state == NULL || tx_state->begun != 1u) {
        return -EINVAL;
    }
    context->commit_calls += 1u;
    return context->fail_commit ? -EIO : 0;
}

static int64_t tx_rollback(void *provider_context, void *state)
{
    integration_tx_context *context = (integration_tx_context *)provider_context;
    integration_tx_state *tx_state = (integration_tx_state *)state;
    if (context == NULL || tx_state == NULL || tx_state->begun != 1u) {
        return -EINVAL;
    }
    context->rollback_calls += 1u;
    return 0;
}

typedef struct integration_service_input {
    arbor_asm_arena *arena;
} integration_service_input;

typedef struct integration_service_output {
    uint32_t kind;
    uint32_t reserved0;
    const uint8_t *body;
    uint64_t body_length;
} integration_service_output;

typedef int64_t (*integration_service_execute_fn)(
    void *provider_context,
    const integration_service_input *input,
    integration_service_output *out);

typedef struct integration_service_v1 {
    arbor_application_service_interface_header header;
    integration_service_execute_fn execute;
} integration_service_v1;

typedef struct integration_service_module_context {
    arbor_capability_binding transaction;
    uint64_t prepare_calls;
    uint64_t rollback_calls;
    uint64_t stop_calls;
    uint64_t execute_calls;
    bool prepared;
} integration_service_module_context;

typedef struct integration_service_provider_context {
    integration_service_module_context *module;
} integration_service_provider_context;

static int64_t service_prepare(
    void *module_context,
    const arbor_application_service_prepare_context *prepare_context)
{
    integration_service_module_context *context =
        (integration_service_module_context *)module_context;
    if (context == NULL || prepare_context == NULL) {
        return -EINVAL;
    }
    context->prepare_calls += 1u;
    context->prepared = false;
    (void)memset(&context->transaction, 0, sizeof(context->transaction));

    arbor_capability_binding binding = {0};
    arbor_status status = arbor_application_service_prepare_resolve(
        prepare_context,
        0u,
        &binding);
    if (status.native != 0) {
        return status.native;
    }
    if (arbor_ddd_transaction_interface_validate(&binding).native != 0) {
        return -EINVAL;
    }

    context->transaction = binding;
    context->prepared = true;
    return 0;
}

static void service_rollback(void *module_context)
{
    integration_service_module_context *context =
        (integration_service_module_context *)module_context;
    if (context != NULL) {
        context->rollback_calls += 1u;
        context->prepared = false;
        (void)memset(&context->transaction, 0, sizeof(context->transaction));
    }
}

static int64_t service_stop(void *module_context)
{
    integration_service_module_context *context =
        (integration_service_module_context *)module_context;
    if (context == NULL) {
        return -EINVAL;
    }
    context->stop_calls += 1u;
    context->prepared = false;
    (void)memset(&context->transaction, 0, sizeof(context->transaction));
    return 0;
}

static int64_t service_execute(
    void *provider_context,
    const integration_service_input *input,
    integration_service_output *out)
{
    integration_service_provider_context *provider =
        (integration_service_provider_context *)provider_context;
    if (provider == NULL || provider->module == NULL || !provider->module->prepared ||
        input == NULL || input->arena == NULL || out == NULL) {
        return -EINVAL;
    }
    provider->module->execute_calls += 1u;

    arbor_asm_result_ptr state_alloc = arena_alloc_aligned(
        input->arena, sizeof(integration_tx_state), 8u);
    arbor_asm_result_ptr records_alloc = arena_alloc_aligned(
        input->arena, 2u * sizeof(arbor_ddd_event_record), 8u);
    arbor_asm_result_ptr bytes_alloc = arena_alloc_aligned(input->arena, 64u, 8u);
    if (state_alloc.status != 0 || records_alloc.status != 0 || bytes_alloc.status != 0) {
        return -ENOSPC;
    }

    arbor_ddd_event_journal journal = {0};
    arbor_status status = arbor_ddd_event_journal_init(
        (arbor_ddd_event_record *)records_alloc.value,
        2u,
        (uint8_t *)bytes_alloc.value,
        64u,
        &journal);
    if (status.native != 0) {
        return status.native;
    }

    arbor_ddd_unit_of_work uow = {0};
    status = arbor_ddd_unit_of_work_begin(
        &provider->module->transaction,
        state_alloc.value,
        sizeof(integration_tx_state),
        &journal,
        &uow);
    if (status.native != 0) {
        return status.native;
    }

    static const uint8_t event_payload[] = "mvc-integrated";
    uint32_t sequence = UINT32_MAX;
    status = arbor_ddd_event_journal_append(
        &journal,
        (arbor_ddd_event_type_id){UINT64_C(0x901), UINT64_C(0x1)},
        1u,
        ARBOR_DDD_EVENT_FLAGS_NONE,
        event_payload,
        (uint32_t)(sizeof(event_payload) - 1u),
        &sequence);
    if (status.native != 0 || sequence != 0u) {
        (void)arbor_ddd_unit_of_work_rollback(&uow);
        return status.native != 0 ? status.native : -EINVAL;
    }

    status = arbor_ddd_unit_of_work_commit(&uow);
    if (status.native != 0) {
        return status.native;
    }
    if (uow.state != ARBOR_DDD_UNIT_OF_WORK_COMMITTED || journal.record_count != 1u) {
        return -EINVAL;
    }

    static const uint8_t body[] = "AF3+AF4 integrated";
    integration_service_output candidate = {
        1u,
        0u,
        body,
        sizeof(body) - 1u
    };
    *out = candidate;
    return 0;
}

typedef struct integration_controller_context {
    arbor_capability_binding cached_service;
    uint64_t controller_calls;
} integration_controller_context;

static int64_t integration_controller(
    const arbor_mvc_request *request,
    void *controller_context,
    arbor_mvc_controller_result *out)
{
    integration_controller_context *context =
        (integration_controller_context *)controller_context;
    if (arbor_mvc_request_validate(request).native != 0 || context == NULL || out == NULL) {
        return -EINVAL;
    }
    context->controller_calls += 1u;

    arbor_status status = arbor_application_service_interface_validate(&context->cached_service);
    if (status.native != 0) {
        return status.native;
    }
    const integration_service_v1 *service =
        (const integration_service_v1 *)context->cached_service.interface_table;
    if (service == NULL || service->execute == NULL) {
        return -EINVAL;
    }

    integration_service_input input = {request->scope->arena};
    integration_service_output service_result = {0u, 0u, NULL, 0u};
    int64_t native = service->execute(
        context->cached_service.provider_context,
        &input,
        &service_result);
    if (native != 0) {
        return native;
    }

    *out = (arbor_mvc_controller_result){
        service_result.kind,
        0u,
        service_result.body,
        service_result.body_length
    };
    return 0;
}

static int64_t integration_presenter(
    const arbor_mvc_request *request,
    void *presenter_context,
    const arbor_mvc_controller_result *result,
    arbor_response_plan *out)
{
    (void)presenter_context;
    if (arbor_mvc_request_validate(request).native != 0 || result == NULL || out == NULL) {
        return -EINVAL;
    }
    *out = (arbor_response_plan){
        200u,
        (const uint8_t *)result->model_data,
        result->model_size,
        ARBOR_RESPONSE_PLAN_FLAG_KEEP_ALIVE
    };
    return 0;
}

static arbor_module_descriptor make_module(
    arbor_module_id id,
    const arbor_capability_export *provides,
    uint64_t provides_count,
    const arbor_capability_requirement *consumes,
    uint64_t consumes_count)
{
    return (arbor_module_descriptor){
        id, ARBOR_MODULE_DESCRIPTOR_ABI_VERSION,
        (uint32_t)sizeof(arbor_module_descriptor), ARBOR_MODULE_FLAGS_NONE,
        provides, provides_count, consumes, consumes_count
    };
}

static arbor_capability_export make_export(
    arbor_capability_id id,
    const void *interface_table,
    uint32_t interface_size,
    void *provider_context)
{
    return (arbor_capability_export){
        id, {1u, 0u}, interface_size, 0u, ARBOR_CAPABILITY_FLAGS_NONE,
        interface_table, provider_context
    };
}

int main(void)
{
    const arbor_module_id tx_module_id = {UINT64_C(0x800), UINT64_C(1)};
    const arbor_module_id service_module_id = {UINT64_C(0x800), UINT64_C(2)};
    const arbor_capability_id tx_cap_id = {UINT64_C(0x810), UINT64_C(1)};
    const arbor_capability_id service_cap_id = {UINT64_C(0x810), UINT64_C(2)};

    integration_tx_context tx_context = {0u, 0u, 0u, false};
    arbor_ddd_transaction_interface tx_interface = {
        ARBOR_DDD_TRANSACTION_INTERFACE_ABI_VERSION,
        (uint32_t)sizeof(arbor_ddd_transaction_interface),
        ARBOR_DDD_TRANSACTION_FLAGS_NONE,
        {UINT64_C(0x820), UINT64_C(1)},
        sizeof(integration_tx_state),
        8u,
        tx_begin,
        tx_commit,
        tx_rollback
    };

    integration_service_module_context service_module_context = {0};
    integration_service_provider_context service_provider_context = {&service_module_context};
    integration_service_v1 service_interface = {
        {ARBOR_APPLICATION_SERVICE_INTERFACE_ABI_VERSION,
         (uint32_t)sizeof(integration_service_v1),
         ARBOR_APPLICATION_SERVICE_INTERFACE_FLAGS_NONE},
        service_execute
    };

    arbor_capability_export tx_exports[1] = {
        make_export(tx_cap_id, &tx_interface, (uint32_t)sizeof(tx_interface), &tx_context)
    };
    arbor_capability_export service_exports[1] = {
        make_export(service_cap_id, &service_interface,
                    (uint32_t)sizeof(service_interface), &service_provider_context)
    };
    arbor_capability_requirement service_consumes[1] = {
        {tx_cap_id, {1u, 0u}, (uint32_t)sizeof(arbor_ddd_transaction_interface),
         0u, ARBOR_CAPABILITY_FLAGS_NONE}
    };
    arbor_module_descriptor modules[2] = {
        make_module(tx_module_id, tx_exports, 1u, NULL, 0u),
        make_module(service_module_id, service_exports, 1u, service_consumes, 1u)
    };

    arbor_capability_catalog_requirements catalog_req = {0};
    arbor_status status = arbor_capability_catalog_measure(modules, 2u, &catalog_req);
    if (status.native != 0 || catalog_req.binding_count != 2u ||
        catalog_req.resolution_count != 1u || catalog_req.module_count != 2u) {
        return fail("MVC0 integration AF2 catalog measurement");
    }

    arbor_capability_binding bindings[2] = {0};
    arbor_capability_resolution resolutions[1] = {0};
    uint64_t module_order[2] = {0u, 0u};
    arbor_capability_catalog_storage catalog_storage = {
        bindings, 2u, resolutions, 1u, module_order, 2u
    };
    uint64_t ws_provider[1] = {0u};
    uint64_t ws_binding[1] = {0u};
    uint64_t ws_indegree[2] = {0u, 0u};
    uint8_t ws_selected[2] = {0u, 0u};
    uint64_t ws_order[2] = {0u, 0u};
    arbor_capability_workspace catalog_workspace = {
        ws_provider, ws_binding, 1u,
        ws_indegree, ws_selected, ws_order, 2u
    };
    arbor_capability_catalog catalog = {0};
    status = arbor_capability_catalog_prepare(
        modules, 2u, &catalog_storage, &catalog_workspace, &catalog);
    if (status.native != 0 || arbor_capability_catalog_validate(&catalog).native != 0) {
        return fail("MVC0 integration AF2 catalog prepare");
    }

    uint64_t service_indices[1] = {0u};
    arbor_application_service_module_descriptor service_descriptor = {
        service_module_id,
        ARBOR_APPLICATION_SERVICE_MODULE_DESCRIPTOR_ABI_VERSION,
        (uint32_t)sizeof(arbor_application_service_module_descriptor),
        ARBOR_APPLICATION_SERVICE_MODULE_FLAGS_NONE,
        &service_module_context,
        service_indices,
        1u,
        service_prepare,
        service_rollback,
        service_stop
    };

    arbor_application_runtime_requirements runtime_req = {0};
    status = arbor_application_runtime_measure(&catalog, &service_descriptor, 1u, &runtime_req);
    if (status.native != 0 || runtime_req.managed_module_record_count != 1u ||
        runtime_req.module_map_count != 2u) {
        return fail("MVC0 integration AF3 runtime measurement");
    }

    arbor_application_service_runtime_record persistent[1] = {0};
    arbor_application_service_runtime_record scratch[1] = {0};
    uint64_t module_map[2] = {0u, 0u};
    arbor_application_runtime_storage runtime_storage = {persistent, 1u};
    arbor_application_runtime_workspace runtime_workspace = {scratch, 1u, module_map, 2u};
    arbor_application_runtime runtime = {0};
    status = arbor_application_runtime_prepare(
        &catalog, &service_descriptor, 1u,
        &runtime_storage, &runtime_workspace, &runtime);
    if (status.native != 0 || !service_module_context.prepared ||
        service_module_context.prepare_calls != 1u) {
        return fail("MVC0 integration AF3 runtime prepare / AF4 binding cache");
    }

    integration_controller_context controller_context = {0};
    status = arbor_application_runtime_find_ready(
        &runtime,
        service_cap_id,
        (arbor_capability_version){1u, 0u},
        (uint32_t)sizeof(integration_service_v1),
        &controller_context.cached_service);
    if (status.native != 0) {
        return fail("MVC0 integration composition-time AF3 binding lookup");
    }

    arbor_mvc_route route = {
        (const uint8_t *)"GET", 3u,
        (const uint8_t *)"/integrated", 11u,
        integration_controller, &controller_context,
        integration_presenter, NULL,
        NULL, 0u
    };
    arbor_mvc_catalog mvc_catalog = {
        ARBOR_MVC_ABI_VERSION, (uint32_t)sizeof(arbor_mvc_catalog), 0u,
        &route, 1u, NULL, 0u
    };
    arbor_route_param route_workspace[16] = {0};
    arbor_mvc_prepare_workspace mvc_workspace = {route_workspace, 16u};
    arbor_mvc_application mvc_application = {0};
    status = arbor_mvc_application_prepare(&mvc_catalog, &mvc_workspace, &mvc_application);
    if (status.native != 0) {
        return fail("MVC0 integration MVC application prepare");
    }
    arbor_application_capabilities mvc_caps = {0};
    status = arbor_mvc_application_capabilities_make(&mvc_application, &mvc_caps);
    if (status.native != 0) {
        return fail("MVC0 integration AF1 capabilities");
    }

    uint8_t arena_bytes[4096] = {0};
    arbor_asm_arena arena = {0};
    if (arena_init(&arena, arena_bytes, sizeof(arena_bytes)).status != 0) {
        return fail("MVC0 integration request arena");
    }
    static const uint8_t request_bytes[] =
        "GET /integrated HTTP/1.1\r\nHost: local\r\n\r\n";
    arbor_request_view view = {0};
    uint64_t required = 0u;
    status = arbor_request_parse(
        (arbor_span){request_bytes, sizeof(request_bytes) - 1u}, &view, &required);
    if (status.native != 0) {
        return fail("MVC0 integration request parse");
    }
    arbor_request_scope scope = {0};
    status = arbor_request_scope_make(&view, NULL, 0u, &arena, &scope);
    if (status.native != 0) {
        return fail("MVC0 integration request scope");
    }

    arbor_capability_binding cached_before = controller_context.cached_service;
    arbor_response_plan response = {0};
    status = arbor_application_invoke(&mvc_caps, &scope, &response);
    if (status.native != 0 || response.status != 200u || response.body_length != 18u ||
        memcmp(response.body_data, "AF3+AF4 integrated", 18u) != 0 ||
        controller_context.controller_calls != 1u || service_module_context.execute_calls != 1u ||
        tx_context.begin_calls != 1u || tx_context.commit_calls != 1u ||
        tx_context.rollback_calls != 0u ||
        memcmp(&cached_before, &controller_context.cached_service, sizeof(cached_before)) != 0) {
        return fail("MVC0 AF2/AF3/AF4/MVC successful integrated request");
    }

    /* Repeated request uses the same cached AF3 binding; no catalog lookup is needed. */
    (void)arena_reset(&arena);
    response = (arbor_response_plan){0};
    status = arbor_application_invoke(&mvc_caps, &scope, &response);
    if (status.native != 0 || controller_context.controller_calls != 2u ||
        service_module_context.execute_calls != 2u || tx_context.commit_calls != 2u ||
        memcmp(&cached_before, &controller_context.cached_service, sizeof(cached_before)) != 0) {
        return fail("MVC0 repeated request cached AF3 binding hot path");
    }

    /* Failed AF4 commit produces mechanism failure and no successful response publication. */
    tx_context.fail_commit = true;
    (void)arena_reset(&arena);
    arbor_response_plan sentinel = {201u, (const uint8_t *)"x", 1u, 0u};
    response = sentinel;
    status = arbor_application_invoke(&mvc_caps, &scope, &response);
    if (status.native != -EIO || memcmp(&response, &sentinel, sizeof(response)) != 0 ||
        tx_context.commit_calls != 3u) {
        return fail("MVC0 AF4 commit failure does not publish HTTP success");
    }
    tx_context.fail_commit = false;

    status = arbor_application_runtime_stop(&runtime);
    if (status.native != 0 || service_module_context.stop_calls != 1u ||
        service_module_context.prepared) {
        return fail("MVC0 integration AF3 runtime shutdown");
    }

    puts("PASS: MVC0 AF2 composition, cached AF3 service, AF4 UOW/events, controller/presenter integration");
    return 0;
}
