#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "echo0.h"

#define ECHO0_PREPARED_GUARD UINT64_C(0x4543484f30573131)
#define ECHO0_VALIDATION_PARAM_CAPACITY 16u

static const uint8_t echo0_method_get[] = "GET";
static const uint8_t echo0_pattern_page[] = "/echo/:value";
static const uint8_t echo0_pattern_redirect[] = "/";
static const uint8_t echo0_template_field_value[] = "value";
static const uint8_t echo0_route_parameter_value[] = "value";

static const uint8_t echo0_cache_control_name[] = "Cache-Control";
static const uint8_t echo0_cache_control_value[] = "no-store";
static const uint8_t echo0_content_type_name[] = "Content-Type";
static const uint8_t echo0_content_type_value[] =
    "text/html; charset=utf-8";
static const uint8_t echo0_location_name[] = "Location";
static const uint8_t echo0_location_value[] = "/echo/Arborcore";

static arbor_span echo0_span(
    const uint8_t *data,
    size_t size_with_terminator)
{
    return (arbor_span){data, (uint64_t)(size_with_terminator - 1u)};
}

static int64_t echo0_before(
    const arbor_mvc_request *request,
    void *context_pointer,
    arbor_mvc_middleware_before_result *result_out)
{
    echo0_web_application *application =
        (echo0_web_application *)context_pointer;
    if (arbor_mvc_request_validate(request).native != 0 ||
        application == NULL || result_out == NULL ||
        application->prepared_guard != ECHO0_PREPARED_GUARD) {
        return -EINVAL;
    }

    application->metrics.middleware_calls += UINT64_C(1);
    arbor_status status = arbor_http_mvc_response_field_append(
        request,
        echo0_span(echo0_cache_control_name, sizeof(echo0_cache_control_name)),
        echo0_span(
            echo0_cache_control_value,
            sizeof(echo0_cache_control_value)));
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

static int64_t echo0_controller(
    const arbor_mvc_request *request,
    void *context_pointer,
    arbor_mvc_controller_result *result_out)
{
    echo0_route_context *route_context =
        (echo0_route_context *)context_pointer;
    if (arbor_mvc_request_validate(request).native != 0 ||
        route_context == NULL || route_context->application == NULL ||
        result_out == NULL || request->scope == NULL ||
        request->scope->arena == NULL ||
        route_context->application->prepared_guard != ECHO0_PREPARED_GUARD) {
        return -EINVAL;
    }

    echo0_web_application *application = route_context->application;
    application->metrics.controller_calls += UINT64_C(1);

    arbor_span value = {NULL, 0u};
    switch (route_context->action) {
    case ECHO0_ACTION_SHOW_PAGE:
        if (request->parameter_count != 1u || request->params == NULL ||
            request->params[0].name_len !=
                (uint64_t)(sizeof(echo0_route_parameter_value) - 1u) ||
            request->params[0].value_ptr == NULL ||
            request->params[0].value_len == 0u ||
            memcmp(
                request->params[0].name_ptr,
                echo0_route_parameter_value,
                sizeof(echo0_route_parameter_value) - 1u) != 0) {
            return -EINVAL;
        }
        value = (arbor_span){
            request->params[0].value_ptr,
            request->params[0].value_len
        };
        break;
    case ECHO0_ACTION_REDIRECT:
        if (request->parameter_count != 0u) {
            return -EINVAL;
        }
        break;
    default:
        return -EINVAL;
    }

    echo0_service_result service_result = {0};
    application->metrics.service_calls += UINT64_C(1);
    arbor_status status = echo0_service_execute(
        &application->service,
        route_context->action,
        value,
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
    if (service_result.outcome_code == (uint32_t)ECHO0_OUTCOME_PAGE) {
        arbor_asm_result_ptr allocation = arena_alloc_aligned(
            request->scope->arena,
            (uint64_t)sizeof(echo0_page_model),
            (uint64_t)_Alignof(echo0_page_model));
        if (allocation.status != 0) {
            return allocation.status;
        }
        echo0_page_model *model = (echo0_page_model *)allocation.value;
        *model = (echo0_page_model){service_result.value};
        candidate.model_data = model;
        candidate.model_size = (uint64_t)sizeof(*model);
    } else if (
        service_result.outcome_code != (uint32_t)ECHO0_OUTCOME_REDIRECT) {
        return -EINVAL;
    }

    *result_out = candidate;
    return 0;
}

static int64_t echo0_publish_page(
    const arbor_mvc_request *request,
    echo0_web_application *application,
    const arbor_mvc_controller_result *result,
    arbor_response_plan *response_out)
{
    if (result->model_data == NULL ||
        result->model_size != (uint64_t)sizeof(echo0_page_model)) {
        return -EINVAL;
    }
    const echo0_page_model *model =
        (const echo0_page_model *)result->model_data;
    if (model->value.data == NULL || model->value.length == 0u ||
        range_end_checked(
            (uint64_t)(uintptr_t)model->value.data,
            model->value.length).status != 0) {
        return -EINVAL;
    }
    const arbor_span values[] = {model->value};
    const uint64_t arena_mark_value = arena_mark(request->scope->arena);
    uint64_t field_mark = 0u;
    arbor_status status = arbor_http_mvc_response_fields_mark(
        request,
        &field_mark);
    if (status.native != 0) {
        return status.native;
    }

    arbor_span body = {NULL, 0u};
    status = arbor_view_html_template_render(
        &application->template_view,
        values,
        1u,
        request->scope->arena,
        &body);
    arbor_response_plan candidate = {0};
    if (status.native == 0) {
        status = arbor_view_utf8_validate(body);
    }
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
            echo0_span(
                echo0_content_type_name,
                sizeof(echo0_content_type_name)),
            echo0_span(
                echo0_content_type_value,
                sizeof(echo0_content_type_value)));
    }
    if (status.native != 0) {
        arbor_status field_rewind = arbor_http_mvc_response_fields_rewind(
            request,
            field_mark);
        arbor_asm_result_u64 arena_rewind_result =
            arena_rewind(request->scope->arena, arena_mark_value);
        if (field_rewind.native != 0) {
            return field_rewind.native;
        }
        return arena_rewind_result.status != 0 ?
            arena_rewind_result.status : status.native;
    }
    *response_out = candidate;
    return 0;
}

static int64_t echo0_publish_redirect(
    const arbor_mvc_request *request,
    const arbor_mvc_controller_result *result,
    arbor_response_plan *response_out)
{
    if (result->model_data != NULL || result->model_size != 0u) {
        return -EINVAL;
    }

    uint64_t field_mark = 0u;
    arbor_status status = arbor_http_mvc_response_fields_mark(
        request,
        &field_mark);
    if (status.native != 0) {
        return status.native;
    }
    arbor_response_plan candidate = {0};
    status = arbor_response_plan_make(
        302u,
        (arbor_span){NULL, 0u},
        ARBOR_RESPONSE_PLAN_FLAG_KEEP_ALIVE,
        &candidate);
    if (status.native != 0) {
        return status.native;
    }
    status = arbor_http_mvc_response_field_append(
        request,
        echo0_span(echo0_location_name, sizeof(echo0_location_name)),
        echo0_span(echo0_location_value, sizeof(echo0_location_value)));
    if (status.native != 0) {
        arbor_status rewind = arbor_http_mvc_response_fields_rewind(
            request,
            field_mark);
        return rewind.native != 0 ? rewind.native : status.native;
    }
    *response_out = candidate;
    return 0;
}

static int64_t echo0_presenter(
    const arbor_mvc_request *request,
    void *context_pointer,
    const arbor_mvc_controller_result *result,
    arbor_response_plan *response_out)
{
    echo0_route_context *route_context =
        (echo0_route_context *)context_pointer;
    if (arbor_mvc_request_validate(request).native != 0 ||
        route_context == NULL || route_context->application == NULL ||
        result == NULL || response_out == NULL || request->scope == NULL ||
        request->scope->arena == NULL ||
        route_context->application->prepared_guard != ECHO0_PREPARED_GUARD) {
        return -EINVAL;
    }

    echo0_web_application *application = route_context->application;
    application->metrics.presenter_calls += UINT64_C(1);
    switch (result->outcome_code) {
    case ECHO0_OUTCOME_PAGE:
        return echo0_publish_page(request, application, result, response_out);
    case ECHO0_OUTCOME_REDIRECT:
        return echo0_publish_redirect(request, result, response_out);
    default:
        return -EINVAL;
    }
}

arbor_status echo0_web_application_prepare(
    arbor_span template_source,
    uint64_t response_field_capacity,
    echo0_web_application *out)
{
    if (out == NULL || template_source.data == NULL ||
        template_source.length == 0u) {
        return arbor_status_from_native(-EINVAL);
    }
    memset(out, 0, sizeof(*out));

    arbor_status status = echo0_service_prepare(&out->service);
    if (status.native != 0) {
        return status;
    }

    const arbor_span field_names[] = {
        echo0_span(
            echo0_template_field_value,
            sizeof(echo0_template_field_value))
    };
    arbor_view_html_template_requirements requirements = {0};
    status = arbor_view_html_template_measure(
        template_source,
        field_names,
        1u,
        &requirements);
    if (status.native != 0) {
        goto fail;
    }
    if (requirements.part_count > ECHO0_TEMPLATE_PART_CAPACITY ||
        requirements.literal_bytes > ECHO0_TEMPLATE_LITERAL_CAPACITY) {
        status = arbor_status_from_native(-ENOSPC);
        goto fail;
    }

    out->template_storage = (arbor_view_html_template_storage){
        out->template_parts,
        ECHO0_TEMPLATE_PART_CAPACITY,
        out->template_literals,
        ECHO0_TEMPLATE_LITERAL_CAPACITY
    };
    status = arbor_view_html_template_prepare(
        template_source,
        field_names,
        1u,
        &out->template_storage,
        &out->template_view);
    if (status.native != 0) {
        goto fail;
    }

    out->route_contexts[0] = (echo0_route_context){
        out,
        ECHO0_ACTION_SHOW_PAGE
    };
    out->route_contexts[1] = (echo0_route_context){
        out,
        ECHO0_ACTION_REDIRECT
    };
    out->middleware_indices[0] = 0u;
    out->middlewares[0] = (arbor_mvc_middleware_descriptor){
        ARBOR_MVC_ABI_VERSION,
        (uint32_t)sizeof(arbor_mvc_middleware_descriptor),
        ARBOR_MVC_MIDDLEWARE_FLAGS_NONE,
        out,
        echo0_before,
        NULL
    };

    out->routes[0] = (arbor_mvc_route){
        echo0_method_get,
        (uint64_t)(sizeof(echo0_method_get) - 1u),
        echo0_pattern_page,
        (uint64_t)(sizeof(echo0_pattern_page) - 1u),
        echo0_controller,
        &out->route_contexts[0],
        echo0_presenter,
        &out->route_contexts[0],
        out->middleware_indices,
        ECHO0_MIDDLEWARE_COUNT
    };
    out->routes[1] = (arbor_mvc_route){
        echo0_method_get,
        (uint64_t)(sizeof(echo0_method_get) - 1u),
        echo0_pattern_redirect,
        (uint64_t)(sizeof(echo0_pattern_redirect) - 1u),
        echo0_controller,
        &out->route_contexts[1],
        echo0_presenter,
        &out->route_contexts[1],
        out->middleware_indices,
        ECHO0_MIDDLEWARE_COUNT
    };
    out->catalog = (arbor_mvc_catalog){
        ARBOR_MVC_ABI_VERSION,
        (uint32_t)sizeof(arbor_mvc_catalog),
        ARBOR_MVC_CATALOG_FLAGS_NONE,
        out->routes,
        ECHO0_ROUTE_COUNT,
        out->middlewares,
        ECHO0_MIDDLEWARE_COUNT
    };

    arbor_route_param validation_params[ECHO0_VALIDATION_PARAM_CAPACITY] = {0};
    arbor_mvc_prepare_workspace workspace = {
        validation_params,
        ECHO0_VALIDATION_PARAM_CAPACITY
    };
    status = arbor_mvc_application_prepare(
        &out->catalog,
        &workspace,
        &out->mvc_application);
    if (status.native != 0) {
        goto fail;
    }
    status = arbor_http_mvc_application_prepare(
        &out->mvc_application,
        response_field_capacity,
        &out->http_application);
    if (status.native != 0) {
        goto fail;
    }

    out->prepared_guard = ECHO0_PREPARED_GUARD;
    status = echo0_web_application_validate(out);
    if (status.native != 0) {
        goto fail;
    }
    return status;

fail:
    (void)memset(out, 0, sizeof(*out));
    return status;
}

arbor_status echo0_web_application_validate(
    const echo0_web_application *application)
{
    if (application == NULL ||
        application->prepared_guard != ECHO0_PREPARED_GUARD ||
        application->service.prepared_guard !=
            ECHO0_SERVICE_PREPARED_GUARD ||
        application->template_storage.parts != application->template_parts ||
        application->template_storage.part_capacity !=
            ECHO0_TEMPLATE_PART_CAPACITY ||
        application->template_storage.literal_bytes !=
            application->template_literals ||
        application->template_storage.literal_capacity !=
            ECHO0_TEMPLATE_LITERAL_CAPACITY ||
        application->template_view.parts != application->template_parts ||
        application->template_view.literal_bytes !=
            application->template_literals ||
        application->template_view.value_count != 1u ||
        application->catalog.routes != application->routes ||
        application->catalog.route_count != ECHO0_ROUTE_COUNT ||
        application->catalog.middlewares != application->middlewares ||
        application->catalog.middleware_count != ECHO0_MIDDLEWARE_COUNT ||
        application->route_contexts[0].application != application ||
        application->route_contexts[1].application != application ||
        application->route_contexts[0].action != ECHO0_ACTION_SHOW_PAGE ||
        application->route_contexts[1].action != ECHO0_ACTION_REDIRECT ||
        application->routes[0].pattern_data != echo0_pattern_page ||
        application->routes[0].pattern_length !=
            (uint64_t)(sizeof(echo0_pattern_page) - 1u) ||
        application->routes[1].pattern_data != echo0_pattern_redirect ||
        application->routes[1].pattern_length !=
            (uint64_t)(sizeof(echo0_pattern_redirect) - 1u) ||
        application->mvc_application.max_route_parameter_count != 1u ||
        application->mvc_application.catalog != &application->catalog ||
        application->http_application.mvc_application !=
            &application->mvc_application) {
        return arbor_status_from_native(-EINVAL);
    }
    return arbor_http_mvc_application_validate(&application->http_application);
}
