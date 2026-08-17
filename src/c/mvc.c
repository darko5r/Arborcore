#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include <arborcore/mvc.h>

_Static_assert(sizeof(arbor_mvc_requirements) == 8u, "MVC requirements layout drift");
_Static_assert(sizeof(arbor_mvc_prepare_workspace) == 16u, "MVC workspace layout drift");
_Static_assert(sizeof(arbor_mvc_controller_result) == 24u, "MVC controller result layout drift");
_Static_assert(sizeof(arbor_mvc_middleware_before_result) == 40u, "MVC middleware-before layout drift");
_Static_assert(sizeof(arbor_mvc_middleware_descriptor) == 40u, "MVC middleware descriptor layout drift");
_Static_assert(sizeof(arbor_mvc_route) == 80u, "MVC route layout drift");
_Static_assert(sizeof(arbor_mvc_catalog) == 48u, "MVC catalog layout drift");
_Static_assert(sizeof(arbor_mvc_application) == 40u, "MVC application layout drift");
_Static_assert(sizeof(arbor_mvc_request) == 40u, "MVC request layout drift");

_Static_assert(offsetof(arbor_mvc_controller_result, outcome_code) == 0u, "MVC result outcome offset drift");
_Static_assert(offsetof(arbor_mvc_controller_result, flags) == 4u, "MVC result flags offset drift");
_Static_assert(offsetof(arbor_mvc_controller_result, model_data) == 8u, "MVC result model offset drift");
_Static_assert(offsetof(arbor_mvc_controller_result, model_size) == 16u, "MVC result size offset drift");
_Static_assert(offsetof(arbor_mvc_middleware_before_result, action) == 0u, "MVC before action offset drift");
_Static_assert(offsetof(arbor_mvc_middleware_before_result, response) == 8u, "MVC before response offset drift");
_Static_assert(offsetof(arbor_mvc_route, method_data) == 0u, "MVC route method offset drift");
_Static_assert(offsetof(arbor_mvc_route, pattern_data) == 16u, "MVC route pattern offset drift");
_Static_assert(offsetof(arbor_mvc_route, controller) == 32u, "MVC route controller offset drift");
_Static_assert(offsetof(arbor_mvc_route, presenter) == 48u, "MVC route presenter offset drift");
_Static_assert(offsetof(arbor_mvc_route, middleware_indices) == 64u, "MVC route middleware offset drift");
_Static_assert(offsetof(arbor_mvc_catalog, routes) == 16u, "MVC catalog routes offset drift");
_Static_assert(offsetof(arbor_mvc_catalog, middlewares) == 32u, "MVC catalog middleware offset drift");
_Static_assert(offsetof(arbor_mvc_application, catalog) == 16u, "MVC application catalog offset drift");
_Static_assert(offsetof(arbor_mvc_application, max_route_parameter_count) == 24u, "MVC application max-param offset drift");
_Static_assert(offsetof(arbor_mvc_application, max_route_parameter_count_guard) == 32u, "MVC application max-param guard offset drift");
_Static_assert(offsetof(arbor_mvc_request, scope) == 0u, "MVC request scope offset drift");
_Static_assert(offsetof(arbor_mvc_request, route) == 8u, "MVC request route offset drift");
_Static_assert(offsetof(arbor_mvc_request, params) == 16u, "MVC request params offset drift");
_Static_assert(offsetof(arbor_mvc_request, parameter_count) == 24u, "MVC request count offset drift");
_Static_assert(offsetof(arbor_mvc_request, route_index) == 32u, "MVC request index offset drift");

static arbor_status invalid_argument_status(void)
{
    return arbor_status_from_native(-EINVAL);
}

static arbor_status no_space_status(void)
{
    return arbor_status_from_native(-ENOSPC);
}

static arbor_status overflow_status(void)
{
    return arbor_status_from_native(-EOVERFLOW);
}

static arbor_status ok_status(void)
{
    return arbor_status_from_native(0);
}

static arbor_status callback_status(int64_t native)
{
    if (native < 0) {
        return arbor_status_from_native(native);
    }
    if (native > 0) {
        return invalid_argument_status();
    }
    return ok_status();
}

static bool span_valid(const uint8_t *data, uint64_t length)
{
    if (length == 0u) {
        return false;
    }
    if (data == NULL) {
        return false;
    }
    arbor_asm_result_u64 end = range_end_checked((uint64_t)(uintptr_t)data, length);
    return end.status == 0;
}

static bool object_span_valid(const void *data, uint64_t length)
{
    if (length == 0u) {
        return true;
    }
    if (data == NULL) {
        return false;
    }
    arbor_asm_result_u64 end = range_end_checked((uint64_t)(uintptr_t)data, length);
    return end.status == 0;
}

static bool spans_overlap(const void *a, uint64_t a_len, const void *b, uint64_t b_len)
{
    if (a_len == 0u || b_len == 0u || a == NULL || b == NULL) {
        return false;
    }
    arbor_asm_result_u64 overlap = range_overlaps(
        (uint64_t)(uintptr_t)a,
        a_len,
        (uint64_t)(uintptr_t)b,
        b_len);
    return overlap.status == 0 && overlap.value != 0u;
}

static arbor_status response_plan_validate_strict(const arbor_response_plan *response)
{
    arbor_status status = arbor_response_plan_validate(response);
    if (status.native != 0) {
        return status;
    }
    if (!object_span_valid(response->body_data, response->body_length)) {
        return invalid_argument_status();
    }
    return ok_status();
}

static bool response_body_overlaps(
    const arbor_response_plan *response,
    const void *object,
    uint64_t object_size)
{
    return response != NULL && spans_overlap(
        response->body_data,
        response->body_length,
        object,
        object_size);
}

static arbor_status route_param_array_validate(
    const arbor_route_param *params,
    uint64_t count)
{
    arbor_asm_result_u64 bytes = u64_mul_checked(
        count,
        (uint64_t)sizeof(arbor_route_param));
    if (bytes.status != 0) {
        return overflow_status();
    }
    if (!object_span_valid(params, bytes.value)) {
        return invalid_argument_status();
    }
    for (uint64_t i = 0u; i < count; ++i) {
        if (params[i].name_len == 0u || params[i].value_len == 0u ||
            !object_span_valid(params[i].name_ptr, params[i].name_len) ||
            !object_span_valid(params[i].value_ptr, params[i].value_len)) {
            return invalid_argument_status();
        }
    }
    return ok_status();
}

static arbor_status route_array_size(uint64_t count, uint64_t *bytes_out)
{
    if (bytes_out == NULL) {
        return invalid_argument_status();
    }
    arbor_asm_result_u64 bytes = u64_mul_checked(count, (uint64_t)sizeof(arbor_mvc_route));
    if (bytes.status != 0) {
        return overflow_status();
    }
    *bytes_out = bytes.value;
    return ok_status();
}

static arbor_status middleware_array_size(uint64_t count, uint64_t *bytes_out)
{
    if (bytes_out == NULL) {
        return invalid_argument_status();
    }
    arbor_asm_result_u64 bytes = u64_mul_checked(
        count,
        (uint64_t)sizeof(arbor_mvc_middleware_descriptor));
    if (bytes.status != 0) {
        return overflow_status();
    }
    *bytes_out = bytes.value;
    return ok_status();
}

static arbor_status index_array_size(uint64_t count, uint64_t *bytes_out)
{
    if (bytes_out == NULL) {
        return invalid_argument_status();
    }
    arbor_asm_result_u64 bytes = u64_mul_checked(count, (uint64_t)sizeof(uint64_t));
    if (bytes.status != 0) {
        return overflow_status();
    }
    *bytes_out = bytes.value;
    return ok_status();
}

static arbor_status middleware_descriptor_validate(
    const arbor_mvc_middleware_descriptor *middleware)
{
    if (middleware == NULL ||
        middleware->abi_version != ARBOR_MVC_ABI_VERSION ||
        middleware->struct_size != (uint32_t)sizeof(arbor_mvc_middleware_descriptor) ||
        middleware->flags != ARBOR_MVC_MIDDLEWARE_FLAGS_NONE ||
        (middleware->before == NULL && middleware->after == NULL)) {
        return invalid_argument_status();
    }
    return ok_status();
}

static arbor_status route_validate_shape(
    const arbor_mvc_route *route,
    uint64_t middleware_count)
{
    if (route == NULL ||
        !span_valid(route->method_data, route->method_length) ||
        !span_valid(route->pattern_data, route->pattern_length) ||
        route->controller == NULL || route->presenter == NULL) {
        return invalid_argument_status();
    }
    if (route->middleware_count != 0u && route->middleware_indices == NULL) {
        return invalid_argument_status();
    }

    uint64_t index_bytes = 0u;
    arbor_status status = index_array_size(route->middleware_count, &index_bytes);
    if (status.native != 0) {
        return status;
    }
    if (!object_span_valid(route->middleware_indices, index_bytes)) {
        return invalid_argument_status();
    }

    for (uint64_t i = 0u; i < route->middleware_count; ++i) {
        uint64_t index = route->middleware_indices[i];
        if (index >= middleware_count) {
            return invalid_argument_status();
        }
        for (uint64_t j = 0u; j < i; ++j) {
            if (route->middleware_indices[j] == index) {
                return invalid_argument_status();
            }
        }
    }
    return ok_status();
}

static arbor_status catalog_validate_shape(const arbor_mvc_catalog *catalog)
{
    if (catalog == NULL ||
        catalog->abi_version != ARBOR_MVC_ABI_VERSION ||
        catalog->struct_size != (uint32_t)sizeof(arbor_mvc_catalog) ||
        catalog->flags != ARBOR_MVC_CATALOG_FLAGS_NONE) {
        return invalid_argument_status();
    }
    if (catalog->route_count != 0u && catalog->routes == NULL) {
        return invalid_argument_status();
    }
    if (catalog->middleware_count != 0u && catalog->middlewares == NULL) {
        return invalid_argument_status();
    }

    uint64_t route_bytes = 0u;
    uint64_t middleware_bytes = 0u;
    arbor_status status = route_array_size(catalog->route_count, &route_bytes);
    if (status.native != 0) {
        return status;
    }
    status = middleware_array_size(catalog->middleware_count, &middleware_bytes);
    if (status.native != 0) {
        return status;
    }
    if (!object_span_valid(catalog->routes, route_bytes) ||
        !object_span_valid(catalog->middlewares, middleware_bytes)) {
        return invalid_argument_status();
    }
    if (spans_overlap(catalog->routes, route_bytes, catalog->middlewares, middleware_bytes)) {
        return invalid_argument_status();
    }

    for (uint64_t i = 0u; i < catalog->middleware_count; ++i) {
        status = middleware_descriptor_validate(&catalog->middlewares[i]);
        if (status.native != 0) {
            return status;
        }
    }
    for (uint64_t i = 0u; i < catalog->route_count; ++i) {
        status = route_validate_shape(&catalog->routes[i], catalog->middleware_count);
        if (status.native != 0) {
            return status;
        }
    }
    return ok_status();
}

static bool route_exact_duplicate(const arbor_mvc_route *left, const arbor_mvc_route *right)
{
    return left->method_length == right->method_length &&
           left->pattern_length == right->pattern_length &&
           bytes_equal(left->method_data, left->method_length,
                       right->method_data, right->method_length) != 0u &&
           bytes_equal(left->pattern_data, left->pattern_length,
                       right->pattern_data, right->pattern_length) != 0u;
}

static bool region_overlaps_route_inputs(
    const void *region,
    uint64_t region_length,
    const arbor_mvc_route *route)
{
    if (route == NULL) {
        return false;
    }
    uint64_t index_bytes = 0u;
    if (index_array_size(route->middleware_count, &index_bytes).native != 0) {
        return true;
    }
    return spans_overlap(region, region_length, route, sizeof(*route)) ||
           spans_overlap(region, region_length, route->method_data, route->method_length) ||
           spans_overlap(region, region_length, route->pattern_data, route->pattern_length) ||
           spans_overlap(region, region_length, route->middleware_indices, index_bytes);
}

static bool region_overlaps_catalog_inputs(
    const void *region,
    uint64_t region_length,
    const arbor_mvc_catalog *catalog)
{
    if (catalog == NULL) {
        return false;
    }
    uint64_t route_bytes = 0u;
    uint64_t middleware_bytes = 0u;
    if (route_array_size(catalog->route_count, &route_bytes).native != 0 ||
        middleware_array_size(catalog->middleware_count, &middleware_bytes).native != 0) {
        return true;
    }
    if (spans_overlap(region, region_length, catalog, sizeof(*catalog)) ||
        spans_overlap(region, region_length, catalog->routes, route_bytes) ||
        spans_overlap(region, region_length, catalog->middlewares, middleware_bytes)) {
        return true;
    }
    for (uint64_t i = 0u; i < catalog->route_count; ++i) {
        if (region_overlaps_route_inputs(region, region_length, &catalog->routes[i])) {
            return true;
        }
    }
    return false;
}

arbor_status arbor_mvc_catalog_measure(
    const arbor_mvc_catalog *catalog,
    arbor_mvc_requirements *out)
{
    if (out == NULL) {
        return invalid_argument_status();
    }
    arbor_status status = catalog_validate_shape(catalog);
    if (status.native != 0) {
        return status;
    }
    if (region_overlaps_catalog_inputs(out, sizeof(*out), catalog)) {
        return invalid_argument_status();
    }

    uint64_t capacity = 0u;
    for (uint64_t i = 0u; i < catalog->route_count; ++i) {
        if (catalog->routes[i].pattern_length > capacity) {
            capacity = catalog->routes[i].pattern_length;
        }
    }

    arbor_mvc_requirements candidate = {capacity};
    *out = candidate;
    return ok_status();
}

arbor_status arbor_mvc_application_prepare(
    const arbor_mvc_catalog *catalog,
    arbor_mvc_prepare_workspace *workspace,
    arbor_mvc_application *out)
{
    if (workspace == NULL || out == NULL) {
        return invalid_argument_status();
    }

    arbor_mvc_requirements requirements = {0u};
    arbor_status status = arbor_mvc_catalog_measure(catalog, &requirements);
    if (status.native != 0) {
        return status;
    }
    if (workspace->route_param_capacity < requirements.route_validation_param_capacity) {
        return no_space_status();
    }
    if (workspace->route_param_capacity != 0u && workspace->route_params == NULL) {
        return invalid_argument_status();
    }

    uint64_t workspace_bytes = 0u;
    arbor_asm_result_u64 workspace_mul = u64_mul_checked(
        workspace->route_param_capacity,
        (uint64_t)sizeof(arbor_route_param));
    if (workspace_mul.status != 0) {
        return overflow_status();
    }
    workspace_bytes = workspace_mul.value;
    if (!object_span_valid(workspace->route_params, workspace_bytes)) {
        return invalid_argument_status();
    }

    uint64_t route_bytes = 0u;
    uint64_t middleware_bytes = 0u;
    status = route_array_size(catalog->route_count, &route_bytes);
    if (status.native != 0) {
        return status;
    }
    status = middleware_array_size(catalog->middleware_count, &middleware_bytes);
    if (status.native != 0) {
        return status;
    }

    if (region_overlaps_catalog_inputs(workspace->route_params, workspace_bytes, catalog) ||
        region_overlaps_catalog_inputs(out, sizeof(*out), catalog) ||
        spans_overlap(out, sizeof(*out), workspace, sizeof(*workspace)) ||
        spans_overlap(out, sizeof(*out), workspace->route_params, workspace_bytes) ||
        spans_overlap(workspace, sizeof(*workspace), workspace->route_params, workspace_bytes) ||
        spans_overlap(workspace, sizeof(*workspace), catalog, sizeof(*catalog)) ||
        spans_overlap(workspace, sizeof(*workspace), catalog->routes, route_bytes) ||
        spans_overlap(workspace, sizeof(*workspace), catalog->middlewares, middleware_bytes)) {
        return invalid_argument_status();
    }

    for (uint64_t i = 0u; i < catalog->route_count; ++i) {
        for (uint64_t j = 0u; j < i; ++j) {
            if (route_exact_duplicate(&catalog->routes[i], &catalog->routes[j])) {
                return arbor_status_from_native(-EEXIST);
            }
        }
    }

    uint64_t max_params = 0u;
    for (uint64_t i = 0u; i < catalog->route_count; ++i) {
        const arbor_mvc_route *route = &catalog->routes[i];
        bool matched = false;
        uint64_t parameter_count = 0u;
        status = arbor_route_match(
            (arbor_span){route->pattern_data, route->pattern_length},
            (arbor_span){route->pattern_data, route->pattern_length},
            workspace->route_params,
            workspace->route_param_capacity,
            &matched,
            &parameter_count);
        if (status.native != 0) {
            return status;
        }
        if (!matched) {
            return invalid_argument_status();
        }
        if (parameter_count > max_params) {
            max_params = parameter_count;
        }
    }

    arbor_mvc_application candidate = {
        ARBOR_MVC_ABI_VERSION,
        (uint32_t)sizeof(arbor_mvc_application),
        ARBOR_MVC_APPLICATION_FLAGS_NONE,
        catalog,
        max_params,
        ~max_params
    };
    status = arbor_mvc_application_validate(&candidate);
    if (status.native != 0) {
        return status;
    }

    *out = candidate;
    return ok_status();
}

static arbor_status application_runtime_validate(const arbor_mvc_application *application)
{
    if (application == NULL ||
        application->abi_version != ARBOR_MVC_ABI_VERSION ||
        application->struct_size != (uint32_t)sizeof(arbor_mvc_application) ||
        application->flags != ARBOR_MVC_APPLICATION_FLAGS_NONE ||
        application->catalog == NULL ||
        application->max_route_parameter_count_guard !=
            ~application->max_route_parameter_count) {
        return invalid_argument_status();
    }
    return ok_status();
}

arbor_status arbor_mvc_application_validate(const arbor_mvc_application *application)
{
    arbor_status status = application_runtime_validate(application);
    if (status.native != 0) {
        return status;
    }

    status = catalog_validate_shape(application->catalog);
    if (status.native != 0) {
        return status;
    }
    if (region_overlaps_catalog_inputs(application, sizeof(*application), application->catalog)) {
        return invalid_argument_status();
    }

    arbor_mvc_requirements requirements = {0u};
    status = arbor_mvc_catalog_measure(application->catalog, &requirements);
    if (status.native != 0) {
        return status;
    }
    if (application->max_route_parameter_count > requirements.route_validation_param_capacity) {
        return invalid_argument_status();
    }
    return ok_status();
}

arbor_status arbor_mvc_request_validate(const arbor_mvc_request *request)
{
    if (request == NULL || request->scope == NULL || request->route == NULL) {
        return invalid_argument_status();
    }
    arbor_status status = arbor_request_scope_validate(request->scope);
    if (status.native != 0) {
        return status;
    }
    if (request->parameter_count != 0u && request->params == NULL) {
        return invalid_argument_status();
    }
    if (request->scope->params != request->params ||
        request->scope->parameter_count != request->parameter_count) {
        return invalid_argument_status();
    }
    status = route_param_array_validate(request->params, request->parameter_count);
    if (status.native != 0) {
        return status;
    }
    return route_validate_shape(request->route, UINT64_MAX);
}

arbor_status arbor_mvc_controller_result_validate(
    const arbor_mvc_controller_result *result)
{
    if (result == NULL || result->flags != ARBOR_MVC_CONTROLLER_RESULT_FLAGS_NONE) {
        return invalid_argument_status();
    }
    if (result->model_size != 0u && result->model_data == NULL) {
        return invalid_argument_status();
    }
    if (!object_span_valid(result->model_data, result->model_size)) {
        return invalid_argument_status();
    }
    return ok_status();
}

arbor_status arbor_mvc_middleware_before_result_validate(
    const arbor_mvc_middleware_before_result *result)
{
    if (result == NULL || result->reserved0 != 0u) {
        return invalid_argument_status();
    }
    if (result->action == ARBOR_MVC_MIDDLEWARE_CONTINUE) {
        if (result->response.status != 0u || result->response.body_data != NULL ||
            result->response.body_length != 0u || result->response.flags != 0u) {
            return invalid_argument_status();
        }
        return ok_status();
    }
    if (result->action == ARBOR_MVC_MIDDLEWARE_RESPOND) {
        arbor_status status = response_plan_validate_strict(&result->response);
        if (status.native != 0) {
            return status;
        }
        if (response_body_overlaps(&result->response, result, sizeof(*result))) {
            return invalid_argument_status();
        }
        return ok_status();
    }
    return invalid_argument_status();
}

static int64_t mvc_request_dispatch(
    const arbor_request_scope *scope,
    void *application_context,
    arbor_response_plan *response_out)
{
    if (scope == NULL || application_context == NULL || response_out == NULL) {
        return -EINVAL;
    }

    arbor_mvc_application *application = (arbor_mvc_application *)application_context;
    arbor_status status = application_runtime_validate(application);
    if (status.native != 0) {
        return status.native;
    }
    status = arbor_request_scope_validate(scope);
    if (status.native != 0) {
        return status.native;
    }
    if (spans_overlap(response_out, sizeof(*response_out), scope, sizeof(*scope)) ||
        spans_overlap(response_out, sizeof(*response_out), application, sizeof(*application)) ||
        region_overlaps_catalog_inputs(
            response_out, sizeof(*response_out), application->catalog) ||
        spans_overlap(
            response_out, sizeof(*response_out),
            scope->arena->base, scope->arena->capacity)) {
        return -EINVAL;
    }

    arbor_route_param *params = NULL;
    if (application->max_route_parameter_count != 0u) {
        arbor_asm_result_u64 bytes = u64_mul_checked(
            application->max_route_parameter_count,
            (uint64_t)sizeof(arbor_route_param));
        if (bytes.status != 0) {
            return bytes.status;
        }
        arbor_asm_result_ptr allocated = arena_alloc_aligned(
            scope->arena,
            bytes.value,
            8u);
        if (allocated.status != 0) {
            return allocated.status;
        }
        params = (arbor_route_param *)allocated.value;
        if (region_overlaps_catalog_inputs(params, bytes.value, application->catalog) ||
            spans_overlap(params, bytes.value, application, sizeof(*application)) ||
            spans_overlap(params, bytes.value, response_out, sizeof(*response_out)) ||
            spans_overlap(params, bytes.value, scope, sizeof(*scope)) ||
            spans_overlap(params, bytes.value, scope->request, sizeof(*scope->request)) ||
            spans_overlap(params, bytes.value, scope->target, sizeof(*scope->target))) {
            return -EINVAL;
        }
    }

    const arbor_mvc_catalog *catalog = application->catalog;
    const arbor_mvc_route *matched_route = NULL;
    uint64_t matched_index = 0u;
    uint64_t parameter_count = 0u;

    for (uint64_t i = 0u; i < catalog->route_count; ++i) {
        const arbor_mvc_route *route = &catalog->routes[i];
        if (route->method_length != scope->request->method_len ||
            bytes_equal(route->method_data, route->method_length,
                        scope->request->method_ptr, scope->request->method_len) == 0u) {
            continue;
        }

        bool matched = false;
        uint64_t count = 0u;
        status = arbor_route_match(
            (arbor_span){route->pattern_data, route->pattern_length},
            (arbor_span){scope->target->path_ptr, scope->target->path_len},
            params,
            application->max_route_parameter_count,
            &matched,
            &count);
        if (status.native != 0) {
            return status.native;
        }
        if (matched) {
            matched_route = route;
            matched_index = i;
            parameter_count = count;
            break;
        }
    }

    if (matched_route == NULL) {
        arbor_response_plan not_found = {0u, NULL, 0u, 0u};
        status = arbor_response_plan_make(
            404u,
            (arbor_span){NULL, 0u},
            ARBOR_RESPONSE_PLAN_FLAG_KEEP_ALIVE,
            &not_found);
        if (status.native != 0) {
            return status.native;
        }
        *response_out = not_found;
        return 0;
    }

    arbor_request_scope routed_scope = {
        scope->request,
        scope->target,
        params,
        parameter_count,
        scope->arena
    };
    status = arbor_request_scope_validate(&routed_scope);
    if (status.native != 0) {
        return status.native;
    }

    arbor_mvc_request request = {
        &routed_scope,
        matched_route,
        params,
        parameter_count,
        matched_index
    };
    status = arbor_mvc_request_validate(&request);
    if (status.native != 0) {
        return status.native;
    }

    uint64_t entered = 0u;
    bool have_response = false;
    arbor_response_plan current = {0u, NULL, 0u, 0u};

    for (uint64_t i = 0u; i < matched_route->middleware_count; ++i) {
        const arbor_mvc_middleware_descriptor *middleware =
            &catalog->middlewares[matched_route->middleware_indices[i]];

        arbor_mvc_middleware_before_result before_result = {0};
        if (middleware->before != NULL) {
            int64_t native = middleware->before(
                &request,
                middleware->middleware_context,
                &before_result);
            status = callback_status(native);
            if (status.native != 0) {
                return status.native;
            }
            status = arbor_mvc_middleware_before_result_validate(&before_result);
            if (status.native != 0) {
                return status.native;
            }
            if (before_result.action == ARBOR_MVC_MIDDLEWARE_RESPOND &&
                (response_body_overlaps(&before_result.response, &request, sizeof(request)) ||
                 response_body_overlaps(
                     &before_result.response, &routed_scope, sizeof(routed_scope)))) {
                return -EINVAL;
            }
        } else {
            before_result.action = ARBOR_MVC_MIDDLEWARE_CONTINUE;
        }

        entered = i + 1u;
        if (before_result.action == ARBOR_MVC_MIDDLEWARE_RESPOND) {
            current = before_result.response;
            have_response = true;
            break;
        }
    }

    if (!have_response) {
        arbor_mvc_controller_result controller_result = {0u, 0u, NULL, 0u};
        int64_t native = matched_route->controller(
            &request,
            matched_route->controller_context,
            &controller_result);
        status = callback_status(native);
        if (status.native != 0) {
            return status.native;
        }
        status = arbor_mvc_controller_result_validate(&controller_result);
        if (status.native != 0) {
            return status.native;
        }

        arbor_response_plan presented = {0u, NULL, 0u, 0u};
        native = matched_route->presenter(
            &request,
            matched_route->presenter_context,
            &controller_result,
            &presented);
        status = callback_status(native);
        if (status.native != 0) {
            return status.native;
        }
        status = response_plan_validate_strict(&presented);
        if (status.native != 0) {
            return status.native;
        }
        if (response_body_overlaps(&presented, &presented, sizeof(presented)) ||
            response_body_overlaps(
                &presented, &controller_result, sizeof(controller_result)) ||
            response_body_overlaps(&presented, &request, sizeof(request)) ||
            response_body_overlaps(&presented, &routed_scope, sizeof(routed_scope))) {
            return -EINVAL;
        }
        current = presented;
    }

    for (uint64_t reverse = entered; reverse != 0u; --reverse) {
        uint64_t middleware_list_index = reverse - 1u;
        const arbor_mvc_middleware_descriptor *middleware =
            &catalog->middlewares[matched_route->middleware_indices[middleware_list_index]];
        if (middleware->after == NULL) {
            continue;
        }

        arbor_response_plan candidate = {0u, NULL, 0u, 0u};
        int64_t native = middleware->after(
            &request,
            middleware->middleware_context,
            &current,
            &candidate);
        status = callback_status(native);
        if (status.native != 0) {
            return status.native;
        }
        status = response_plan_validate_strict(&candidate);
        if (status.native != 0) {
            return status.native;
        }
        if (response_body_overlaps(&candidate, &candidate, sizeof(candidate)) ||
            response_body_overlaps(&candidate, &current, sizeof(current)) ||
            response_body_overlaps(&candidate, &request, sizeof(request)) ||
            response_body_overlaps(&candidate, &routed_scope, sizeof(routed_scope))) {
            return -EINVAL;
        }
        current = candidate;
    }

    status = response_plan_validate_strict(&current);
    if (status.native != 0 ||
        response_body_overlaps(&current, &current, sizeof(current)) ||
        response_body_overlaps(&current, &request, sizeof(request)) ||
        response_body_overlaps(&current, &routed_scope, sizeof(routed_scope))) {
        return status.native != 0 ? status.native : -EINVAL;
    }

    *response_out = current;
    return 0;
}

arbor_status arbor_mvc_application_capabilities_make(
    arbor_mvc_application *application,
    arbor_application_capabilities *out)
{
    if (out == NULL) {
        return invalid_argument_status();
    }
    arbor_status status = arbor_mvc_application_validate(application);
    if (status.native != 0) {
        return status;
    }
    if (spans_overlap(out, sizeof(*out), application, sizeof(*application)) ||
        region_overlaps_catalog_inputs(out, sizeof(*out), application->catalog)) {
        return invalid_argument_status();
    }
    return arbor_application_capabilities_make(
        mvc_request_dispatch,
        application,
        out);
}
