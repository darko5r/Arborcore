#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "counter1.h"

#define COUNTER1_WEB_GUARD UINT64_C(0x434e545231574542)
#define COUNTER1_ACTION_GET UINT32_C(1)
#define COUNTER1_ACTION_INCREMENT UINT32_C(2)
#define COUNTER1_VALIDATION_PARAM_CAPACITY 22u
#define COUNTER1_DECIMAL_BUFFER_CAPACITY 20u

static const uint8_t counter1_method_get[] = "GET";
static const uint8_t counter1_method_post[] = "POST";
static const uint8_t counter1_pattern_get[] = "/counter/:id";
static const uint8_t counter1_pattern_increment[] = "/counter/:id/increment";
static const uint8_t counter1_parameter_id[] = "id";
static const uint8_t counter1_template_field_id[] = "id";
static const uint8_t counter1_template_field_value[] = "value";
static const uint8_t counter1_content_type_name[] = "Content-Type";
static const uint8_t counter1_content_type_value[] = "text/html; charset=utf-8";

static arbor_span counter1_span(
    const uint8_t *data,
    size_t size_with_terminator)
{
    return (arbor_span){data, (uint64_t)(size_with_terminator - 1u)};
}

static int counter1_parse_u64(arbor_span text, uint64_t *value_out)
{
    if (value_out == NULL || text.data == NULL || text.length == 0u) {
        return 0;
    }
    uint64_t value = 0u;
    for (uint64_t index = 0u; index < text.length; ++index) {
        const uint8_t byte = text.data[index];
        if (byte < (uint8_t)'0' || byte > (uint8_t)'9') {
            return 0;
        }
        const uint64_t digit = (uint64_t)(byte - (uint8_t)'0');
        if (value > (UINT64_MAX - digit) / UINT64_C(10)) {
            return 0;
        }
        value = value * UINT64_C(10) + digit;
    }
    *value_out = value;
    return 1;
}

static int64_t counter1_page_model_make(
    arbor_asm_arena *arena,
    uint64_t id,
    uint64_t value,
    counter1_page_model **model_out)
{
    if (arena == NULL || model_out == NULL) {
        return -EINVAL;
    }
    arbor_asm_result_ptr model_allocation = arena_alloc_aligned(
        arena,
        (uint64_t)sizeof(counter1_page_model),
        (uint64_t)_Alignof(counter1_page_model));
    arbor_asm_result_ptr id_allocation = arena_alloc_aligned(
        arena,
        COUNTER1_DECIMAL_BUFFER_CAPACITY,
        1u);
    arbor_asm_result_ptr value_allocation = arena_alloc_aligned(
        arena,
        COUNTER1_DECIMAL_BUFFER_CAPACITY,
        1u);
    if (model_allocation.status != 0 || id_allocation.status != 0 ||
        value_allocation.status != 0) {
        return -ENOSPC;
    }

    arbor_asm_result_u64 id_result = u64_format_decimal(
        id,
        id_allocation.value,
        COUNTER1_DECIMAL_BUFFER_CAPACITY);
    arbor_asm_result_u64 value_result = u64_format_decimal(
        value,
        value_allocation.value,
        COUNTER1_DECIMAL_BUFFER_CAPACITY);
    if (id_result.status != 0 || value_result.status != 0 ||
        id_result.value == 0u || value_result.value == 0u) {
        return id_result.status != 0 ? id_result.status :
            (value_result.status != 0 ? value_result.status : -EINVAL);
    }

    counter1_page_model *model =
        (counter1_page_model *)model_allocation.value;
    *model = (counter1_page_model){
        {(const uint8_t *)id_allocation.value, id_result.value},
        {(const uint8_t *)value_allocation.value, value_result.value}
    };
    *model_out = model;
    return 0;
}

static int64_t counter1_controller(
    const arbor_mvc_request *request,
    void *context_pointer,
    arbor_mvc_controller_result *result_out)
{
    counter1_route_context *route_context =
        (counter1_route_context *)context_pointer;
    if (arbor_mvc_request_validate(request).native != 0 ||
        route_context == NULL || route_context->application == NULL ||
        result_out == NULL || request->scope == NULL ||
        request->scope->arena == NULL || request->params == NULL ||
        request->parameter_count != 1u ||
        counter1_application_validate(route_context->application).native != 0) {
        return -EINVAL;
    }

    const arbor_route_param *parameter = &request->params[0];
    if (parameter->name_ptr == NULL || parameter->value_ptr == NULL ||
        parameter->name_len != sizeof(counter1_parameter_id) - 1u ||
        memcmp(parameter->name_ptr, counter1_parameter_id,
               sizeof(counter1_parameter_id) - 1u) != 0) {
        return -EINVAL;
    }

    uint64_t id = 0u;
    arbor_span id_text = {parameter->value_ptr, parameter->value_len};
    if (!counter1_parse_u64(id_text, &id)) {
        *result_out = (arbor_mvc_controller_result){
            (uint32_t)COUNTER1_CONTROLLER_INVALID_REQUEST,
            ARBOR_MVC_CONTROLLER_RESULT_FLAGS_NONE,
            NULL,
            0u
        };
        return 0;
    }

    arbor_capability_binding *binding = &route_context->application->service_binding;
    arbor_status status = arbor_application_service_interface_validate(binding);
    if (status.native != 0 || binding->interface_table == NULL ||
        binding->provider_context == NULL ||
        binding->interface_size != (uint32_t)sizeof(counter1_service_v1)) {
        return -EINVAL;
    }
    const counter1_service_v1 *service =
        (const counter1_service_v1 *)binding->interface_table;
    if (service->get_counter == NULL || service->increment_counter == NULL) {
        return -EINVAL;
    }

    counter1_service_request service_request = {request->scope->arena, id};
    counter1_service_result service_result = {0};
    int64_t native = 0;
    if (route_context->action == COUNTER1_ACTION_GET) {
        native = service->get_counter(
            binding->provider_context,
            &service_request,
            &service_result);
    } else if (route_context->action == COUNTER1_ACTION_INCREMENT) {
        native = service->increment_counter(
            binding->provider_context,
            &service_request,
            &service_result);
    } else {
        return -EINVAL;
    }
    if (native != 0) {
        return native;
    }

    arbor_mvc_controller_result candidate = {
        0u,
        ARBOR_MVC_CONTROLLER_RESULT_FLAGS_NONE,
        NULL,
        0u
    };
    switch (service_result.outcome_code) {
    case COUNTER1_SERVICE_FOUND:
        candidate.outcome_code = (uint32_t)COUNTER1_CONTROLLER_FOUND;
        break;
    case COUNTER1_SERVICE_INCREMENTED:
        candidate.outcome_code = (uint32_t)COUNTER1_CONTROLLER_INCREMENTED;
        break;
    case COUNTER1_SERVICE_NOT_FOUND:
        candidate.outcome_code = (uint32_t)COUNTER1_CONTROLLER_NOT_FOUND;
        break;
    case COUNTER1_SERVICE_LIMIT_REACHED:
        candidate.outcome_code = (uint32_t)COUNTER1_CONTROLLER_LIMIT_REACHED;
        break;
    default:
        return -EINVAL;
    }

    if (candidate.outcome_code == (uint32_t)COUNTER1_CONTROLLER_FOUND ||
        candidate.outcome_code == (uint32_t)COUNTER1_CONTROLLER_INCREMENTED) {
        counter1_page_model *model = NULL;
        native = counter1_page_model_make(
            request->scope->arena,
            service_result.id,
            service_result.value,
            &model);
        if (native != 0) {
            return native;
        }
        candidate.model_data = model;
        candidate.model_size = (uint64_t)sizeof(*model);
    }

    *result_out = candidate;
    return 0;
}

static int64_t counter1_publish_page(
    const arbor_mvc_request *request,
    counter1_web_application *application,
    const arbor_mvc_controller_result *result,
    arbor_response_plan *response_out)
{
    if (result->model_data == NULL ||
        result->model_size != (uint64_t)sizeof(counter1_page_model)) {
        return -EINVAL;
    }
    const counter1_page_model *model =
        (const counter1_page_model *)result->model_data;
    const arbor_span values[COUNTER1_TEMPLATE_FIELD_COUNT] = {
        model->id_text,
        model->value_text
    };
    const uint64_t mark = arena_mark(request->scope->arena);

    arbor_span body = {NULL, 0u};
    arbor_status status = arbor_view_html_template_render(
        &application->template_view,
        values,
        COUNTER1_TEMPLATE_FIELD_COUNT,
        request->scope->arena,
        &body);
    if (status.native != 0) {
        return status.native;
    }

    arbor_response_plan candidate = {0};
    status = arbor_view_utf8_validate(body);
    if (status.native == 0) {
        status = arbor_response_plan_make(
            200u,
            body,
            ARBOR_RESPONSE_PLAN_FLAG_KEEP_ALIVE,
            &candidate);
    }
    if (status.native == 0) {
        status = arbor_http_mvc_response_field_append(
            request,
            counter1_span(
                counter1_content_type_name,
                sizeof(counter1_content_type_name)),
            counter1_span(
                counter1_content_type_value,
                sizeof(counter1_content_type_value)));
    }
    if (status.native != 0) {
        arbor_asm_result_u64 rewind_result =
            arena_rewind(request->scope->arena, mark);
        return rewind_result.status != 0 ? rewind_result.status : status.native;
    }
    *response_out = candidate;
    return 0;
}

static int64_t counter1_publish_empty(
    uint64_t status_code,
    const arbor_mvc_controller_result *result,
    arbor_response_plan *response_out)
{
    if (result->model_data != NULL || result->model_size != 0u) {
        return -EINVAL;
    }
    arbor_response_plan candidate = {0};
    arbor_status status = arbor_response_plan_make(
        status_code,
        (arbor_span){NULL, 0u},
        ARBOR_RESPONSE_PLAN_FLAG_KEEP_ALIVE,
        &candidate);
    if (status.native != 0) {
        return status.native;
    }
    *response_out = candidate;
    return 0;
}

static int64_t counter1_presenter(
    const arbor_mvc_request *request,
    void *context_pointer,
    const arbor_mvc_controller_result *result,
    arbor_response_plan *response_out)
{
    counter1_web_application *web =
        (counter1_web_application *)context_pointer;
    if (arbor_mvc_request_validate(request).native != 0 ||
        web == NULL || web->application == NULL ||
        web->prepared_guard != COUNTER1_WEB_GUARD ||
        result == NULL || response_out == NULL || request->scope == NULL ||
        request->scope->arena == NULL) {
        return -EINVAL;
    }

    switch (result->outcome_code) {
    case COUNTER1_CONTROLLER_FOUND:
    case COUNTER1_CONTROLLER_INCREMENTED:
        return counter1_publish_page(request, web, result, response_out);
    case COUNTER1_CONTROLLER_NOT_FOUND:
        return counter1_publish_empty(404u, result, response_out);
    case COUNTER1_CONTROLLER_LIMIT_REACHED:
        return counter1_publish_empty(409u, result, response_out);
    case COUNTER1_CONTROLLER_INVALID_REQUEST:
        return counter1_publish_empty(400u, result, response_out);
    default:
        return -EINVAL;
    }
}

arbor_status counter1_web_application_prepare(
    counter1_application *application,
    arbor_span template_source,
    uint64_t response_field_capacity,
    counter1_web_application *out)
{
    if (application == NULL || out == NULL || template_source.data == NULL ||
        template_source.length == 0u ||
        response_field_capacity != COUNTER1_RESPONSE_FIELD_CAPACITY) {
        return arbor_status_from_native(-EINVAL);
    }
    arbor_status status = counter1_application_validate(application);
    if (status.native != 0) {
        return status;
    }

    (void)memset(out, 0, sizeof(*out));
    out->application = application;

    const arbor_span field_names[COUNTER1_TEMPLATE_FIELD_COUNT] = {
        counter1_span(counter1_template_field_id,
                      sizeof(counter1_template_field_id)),
        counter1_span(counter1_template_field_value,
                      sizeof(counter1_template_field_value))
    };
    arbor_view_html_template_requirements requirements = {0};
    status = arbor_view_html_template_measure(
        template_source,
        field_names,
        COUNTER1_TEMPLATE_FIELD_COUNT,
        &requirements);
    if (status.native != 0) {
        return status;
    }
    if (requirements.part_count > 64u || requirements.literal_bytes > 4096u) {
        return arbor_status_from_native(-ENOSPC);
    }

    out->template_storage = (arbor_view_html_template_storage){
        out->template_parts,
        64u,
        out->template_literals,
        4096u
    };
    status = arbor_view_html_template_prepare(
        template_source,
        field_names,
        COUNTER1_TEMPLATE_FIELD_COUNT,
        &out->template_storage,
        &out->template_view);
    if (status.native != 0) {
        return status;
    }

    out->route_contexts[0] = (counter1_route_context){
        application,
        COUNTER1_ACTION_GET,
        0u
    };
    out->route_contexts[1] = (counter1_route_context){
        application,
        COUNTER1_ACTION_INCREMENT,
        0u
    };

    out->routes[0] = (arbor_mvc_route){
        counter1_method_get,
        (uint64_t)(sizeof(counter1_method_get) - 1u),
        counter1_pattern_get,
        (uint64_t)(sizeof(counter1_pattern_get) - 1u),
        counter1_controller,
        &out->route_contexts[0],
        counter1_presenter,
        out,
        NULL,
        0u
    };
    out->routes[1] = (arbor_mvc_route){
        counter1_method_post,
        (uint64_t)(sizeof(counter1_method_post) - 1u),
        counter1_pattern_increment,
        (uint64_t)(sizeof(counter1_pattern_increment) - 1u),
        counter1_controller,
        &out->route_contexts[1],
        counter1_presenter,
        out,
        NULL,
        0u
    };
    out->catalog = (arbor_mvc_catalog){
        ARBOR_MVC_ABI_VERSION,
        (uint32_t)sizeof(arbor_mvc_catalog),
        ARBOR_MVC_CATALOG_FLAGS_NONE,
        out->routes,
        COUNTER1_ROUTE_COUNT,
        NULL,
        0u
    };

    arbor_route_param validation_params[COUNTER1_VALIDATION_PARAM_CAPACITY] = {{0}};
    arbor_mvc_prepare_workspace workspace = {
        validation_params,
        COUNTER1_VALIDATION_PARAM_CAPACITY
    };
    status = arbor_mvc_application_prepare(
        &out->catalog,
        &workspace,
        &out->mvc_application);
    if (status.native != 0) {
        return status;
    }
    status = arbor_http_mvc_application_prepare(
        &out->mvc_application,
        response_field_capacity,
        &out->http_application);
    if (status.native != 0) {
        return status;
    }

    out->prepared_guard = COUNTER1_WEB_GUARD;
    status = counter1_web_application_validate(out);
    if (status.native != 0) {
        out->prepared_guard = 0u;
    }
    return status;
}

arbor_status counter1_web_application_validate(
    const counter1_web_application *application)
{
    if (application == NULL || application->application == NULL ||
        application->prepared_guard != COUNTER1_WEB_GUARD ||
        application->template_storage.parts != application->template_parts ||
        application->template_storage.part_capacity != 64u ||
        application->template_storage.literal_bytes != application->template_literals ||
        application->template_storage.literal_capacity != 4096u ||
        application->template_view.parts != application->template_parts ||
        application->template_view.literal_bytes != application->template_literals ||
        application->template_view.value_count != COUNTER1_TEMPLATE_FIELD_COUNT ||
        application->catalog.routes != application->routes ||
        application->catalog.route_count != COUNTER1_ROUTE_COUNT ||
        application->catalog.middlewares != NULL ||
        application->catalog.middleware_count != 0u ||
        application->route_contexts[0].application != application->application ||
        application->route_contexts[1].application != application->application ||
        application->mvc_application.catalog != &application->catalog ||
        application->http_application.mvc_application !=
            &application->mvc_application) {
        return arbor_status_from_native(-EINVAL);
    }
    arbor_status status = counter1_application_validate(application->application);
    if (status.native != 0) {
        return status;
    }
    return arbor_http_mvc_application_validate(&application->http_application);
}
