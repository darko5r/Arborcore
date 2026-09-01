#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "hello0.h"

#define HELLO0_PREPARED_GUARD UINT64_C(0x48454c4c4f305731)
#define HELLO0_VALIDATION_PARAM_CAPACITY 8u

static const uint8_t hello0_method_get[] = "GET";
static const uint8_t hello0_pattern_page[] = "/hello";
static const uint8_t hello0_pattern_redirect[] = "/";
static const uint8_t hello0_template_field_message[] = "message";

static const uint8_t hello0_cache_control_name[] = "Cache-Control";
static const uint8_t hello0_cache_control_value[] = "no-store";
static const uint8_t hello0_content_type_name[] = "Content-Type";
static const uint8_t hello0_content_type_value[] =
    "text/html; charset=utf-8";
static const uint8_t hello0_location_name[] = "Location";
static const uint8_t hello0_location_value[] = "/hello";

static arbor_span hello0_span(
    const uint8_t *data,
    size_t size_with_terminator)
{
    return (arbor_span){data, (uint64_t)(size_with_terminator - 1u)};
}

static int64_t hello0_before(
    const arbor_mvc_request *request,
    void *context_pointer,
    arbor_mvc_middleware_before_result *result_out)
{
    hello0_web_application *application =
        (hello0_web_application *)context_pointer;
    if (arbor_mvc_request_validate(request).native != 0 ||
        application == NULL || result_out == NULL ||
        application->prepared_guard != HELLO0_PREPARED_GUARD) {
        return -EINVAL;
    }

    application->metrics.middleware_calls += UINT64_C(1);
    arbor_status status = arbor_http_mvc_response_field_append(
        request,
        hello0_span(hello0_cache_control_name, sizeof(hello0_cache_control_name)),
        hello0_span(
            hello0_cache_control_value,
            sizeof(hello0_cache_control_value)));
    if (status.native != 0) {
        return status.native;
    }

    *result_out = (arbor_mvc_middleware_before_result){
        ARBOR_MVC_MIDDLEWARE_CONTINUE,
        0u,
        {0u, NULL, 0u, ARBOR_RESPONSE_PLAN_FLAG_NONE}
    };
    return 0;
}

static int64_t hello0_controller(
    const arbor_mvc_request *request,
    void *context_pointer,
    arbor_mvc_controller_result *result_out)
{
    hello0_route_context *route_context =
        (hello0_route_context *)context_pointer;
    if (arbor_mvc_request_validate(request).native != 0 ||
        route_context == NULL || route_context->application == NULL ||
        result_out == NULL || request->scope == NULL ||
        request->scope->arena == NULL ||
        route_context->application->prepared_guard != HELLO0_PREPARED_GUARD) {
        return -EINVAL;
    }

    hello0_web_application *application = route_context->application;
    application->metrics.controller_calls += UINT64_C(1);

    hello0_service_result service_result = {0};
    application->metrics.service_calls += UINT64_C(1);
    arbor_status status = hello0_service_execute(
        &application->service,
        route_context->action,
        &service_result);
    if (status.native != 0) {
        return status.native;
    }

    arbor_mvc_controller_result candidate = {
        service_result.outcome_code,
        ARBOR_MVC_CONTROLLER_RESULT_FLAGS_NONE,
        NULL,
        0u
    };
    if (service_result.outcome_code == (uint32_t)HELLO0_OUTCOME_PAGE) {
        arbor_asm_result_ptr allocation = arena_alloc_aligned(
            request->scope->arena,
            (uint64_t)sizeof(hello0_page_model),
            (uint64_t)_Alignof(hello0_page_model));
        if (allocation.status != 0) {
            return allocation.status;
        }
        hello0_page_model *model = (hello0_page_model *)allocation.value;
        *model = (hello0_page_model){service_result.message};
        candidate.model_data = model;
        candidate.model_size = (uint64_t)sizeof(*model);
    } else if (
        service_result.outcome_code != (uint32_t)HELLO0_OUTCOME_REDIRECT) {
        return -EINVAL;
    }

    *result_out = candidate;
    return 0;
}

static int64_t hello0_publish_page(
    const arbor_mvc_request *request,
    hello0_web_application *application,
    const arbor_mvc_controller_result *result,
    arbor_response_plan *response_out)
{
    if (result->model_data == NULL ||
        result->model_size != (uint64_t)sizeof(hello0_page_model)) {
        return -EINVAL;
    }
    const hello0_page_model *model =
        (const hello0_page_model *)result->model_data;
    const arbor_span values[] = {model->message};
    const uint64_t mark = arena_mark(request->scope->arena);

    arbor_span body = {NULL, 0u};
    arbor_status status = arbor_view_html_template_render(
        &application->template_view,
        values,
        1u,
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
            hello0_span(
                hello0_content_type_name,
                sizeof(hello0_content_type_name)),
            hello0_span(
                hello0_content_type_value,
                sizeof(hello0_content_type_value)));
    }
    if (status.native != 0) {
        arbor_asm_result_u64 rewind_result =
            arena_rewind(request->scope->arena, mark);
        return rewind_result.status != 0 ? rewind_result.status : status.native;
    }
    *response_out = candidate;
    return 0;
}

static int64_t hello0_publish_redirect(
    const arbor_mvc_request *request,
    const arbor_mvc_controller_result *result,
    arbor_response_plan *response_out)
{
    if (result->model_data != NULL || result->model_size != 0u) {
        return -EINVAL;
    }

    arbor_response_plan candidate = {0};
    arbor_status status = arbor_response_plan_make(
        302u,
        (arbor_span){NULL, 0u},
        ARBOR_RESPONSE_PLAN_FLAG_KEEP_ALIVE,
        &candidate);
    if (status.native != 0) {
        return status.native;
    }
    status = arbor_http_mvc_response_field_append(
        request,
        hello0_span(hello0_location_name, sizeof(hello0_location_name)),
        hello0_span(hello0_location_value, sizeof(hello0_location_value)));
    if (status.native != 0) {
        return status.native;
    }
    *response_out = candidate;
    return 0;
}

static int64_t hello0_presenter(
    const arbor_mvc_request *request,
    void *context_pointer,
    const arbor_mvc_controller_result *result,
    arbor_response_plan *response_out)
{
    hello0_route_context *route_context =
        (hello0_route_context *)context_pointer;
    if (arbor_mvc_request_validate(request).native != 0 ||
        route_context == NULL || route_context->application == NULL ||
        result == NULL || response_out == NULL || request->scope == NULL ||
        request->scope->arena == NULL ||
        route_context->application->prepared_guard != HELLO0_PREPARED_GUARD) {
        return -EINVAL;
    }

    hello0_web_application *application = route_context->application;
    application->metrics.presenter_calls += UINT64_C(1);
    switch (result->outcome_code) {
    case HELLO0_OUTCOME_PAGE:
        return hello0_publish_page(request, application, result, response_out);
    case HELLO0_OUTCOME_REDIRECT:
        return hello0_publish_redirect(request, result, response_out);
    default:
        return -EINVAL;
    }
}

arbor_status hello0_web_application_prepare(
    arbor_span template_source,
    uint64_t response_field_capacity,
    hello0_web_application *out)
{
    if (out == NULL || template_source.data == NULL ||
        template_source.length == 0u) {
        return arbor_status_from_native(-EINVAL);
    }
    memset(out, 0, sizeof(*out));

    arbor_status status = hello0_service_prepare(&out->service);
    if (status.native != 0) {
        return status;
    }

    const arbor_span field_names[] = {
        hello0_span(
            hello0_template_field_message,
            sizeof(hello0_template_field_message))
    };
    arbor_view_html_template_requirements requirements = {0};
    status = arbor_view_html_template_measure(
        template_source,
        field_names,
        1u,
        &requirements);
    if (status.native != 0) {
        return status;
    }
    if (requirements.part_count > HELLO0_TEMPLATE_PART_CAPACITY ||
        requirements.literal_bytes > HELLO0_TEMPLATE_LITERAL_CAPACITY) {
        return arbor_status_from_native(-ENOSPC);
    }

    out->template_storage = (arbor_view_html_template_storage){
        out->template_parts,
        HELLO0_TEMPLATE_PART_CAPACITY,
        out->template_literals,
        HELLO0_TEMPLATE_LITERAL_CAPACITY
    };
    status = arbor_view_html_template_prepare(
        template_source,
        field_names,
        1u,
        &out->template_storage,
        &out->template_view);
    if (status.native != 0) {
        return status;
    }

    out->route_contexts[0] = (hello0_route_context){
        out,
        HELLO0_ACTION_SHOW_PAGE
    };
    out->route_contexts[1] = (hello0_route_context){
        out,
        HELLO0_ACTION_REDIRECT
    };
    out->middleware_indices[0] = 0u;
    out->middlewares[0] = (arbor_mvc_middleware_descriptor){
        ARBOR_MVC_ABI_VERSION,
        (uint32_t)sizeof(arbor_mvc_middleware_descriptor),
        ARBOR_MVC_MIDDLEWARE_FLAGS_NONE,
        out,
        hello0_before,
        NULL
    };

    out->routes[0] = (arbor_mvc_route){
        hello0_method_get,
        (uint64_t)(sizeof(hello0_method_get) - 1u),
        hello0_pattern_page,
        (uint64_t)(sizeof(hello0_pattern_page) - 1u),
        hello0_controller,
        &out->route_contexts[0],
        hello0_presenter,
        &out->route_contexts[0],
        out->middleware_indices,
        HELLO0_MIDDLEWARE_COUNT
    };
    out->routes[1] = (arbor_mvc_route){
        hello0_method_get,
        (uint64_t)(sizeof(hello0_method_get) - 1u),
        hello0_pattern_redirect,
        (uint64_t)(sizeof(hello0_pattern_redirect) - 1u),
        hello0_controller,
        &out->route_contexts[1],
        hello0_presenter,
        &out->route_contexts[1],
        out->middleware_indices,
        HELLO0_MIDDLEWARE_COUNT
    };
    out->catalog = (arbor_mvc_catalog){
        ARBOR_MVC_ABI_VERSION,
        (uint32_t)sizeof(arbor_mvc_catalog),
        ARBOR_MVC_CATALOG_FLAGS_NONE,
        out->routes,
        HELLO0_ROUTE_COUNT,
        out->middlewares,
        HELLO0_MIDDLEWARE_COUNT
    };

    arbor_route_param validation_params[HELLO0_VALIDATION_PARAM_CAPACITY] = {0};
    arbor_mvc_prepare_workspace workspace = {
        validation_params,
        HELLO0_VALIDATION_PARAM_CAPACITY
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

    out->prepared_guard = HELLO0_PREPARED_GUARD;
    status = hello0_web_application_validate(out);
    if (status.native != 0) {
        out->prepared_guard = 0u;
    }
    return status;
}

arbor_status hello0_web_application_validate(
    const hello0_web_application *application)
{
    if (application == NULL ||
        application->prepared_guard != HELLO0_PREPARED_GUARD ||
        application->service.page_message.data == NULL ||
        application->service.page_message.length == 0u ||
        application->template_storage.parts != application->template_parts ||
        application->template_storage.part_capacity !=
            HELLO0_TEMPLATE_PART_CAPACITY ||
        application->template_storage.literal_bytes !=
            application->template_literals ||
        application->template_storage.literal_capacity !=
            HELLO0_TEMPLATE_LITERAL_CAPACITY ||
        application->template_view.parts != application->template_parts ||
        application->template_view.literal_bytes !=
            application->template_literals ||
        application->template_view.value_count != 1u ||
        application->catalog.routes != application->routes ||
        application->catalog.route_count != HELLO0_ROUTE_COUNT ||
        application->catalog.middlewares != application->middlewares ||
        application->catalog.middleware_count != HELLO0_MIDDLEWARE_COUNT ||
        application->route_contexts[0].application != application ||
        application->route_contexts[1].application != application ||
        application->mvc_application.catalog != &application->catalog ||
        application->http_application.mvc_application !=
            &application->mvc_application) {
        return arbor_status_from_native(-EINVAL);
    }
    return arbor_http_mvc_application_validate(&application->http_application);
}
