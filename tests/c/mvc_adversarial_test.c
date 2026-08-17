#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <arborcore/mvc.h>

static int fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    return 1;
}

static int64_t good_controller(
    const arbor_mvc_request *request,
    void *context,
    arbor_mvc_controller_result *out)
{
    (void)context;
    if (request == NULL || out == NULL) {
        return -EINVAL;
    }
    static const uint8_t body[] = "ok";
    *out = (arbor_mvc_controller_result){1u, 0u, body, 2u};
    return 0;
}

static int64_t positive_controller(
    const arbor_mvc_request *request,
    void *context,
    arbor_mvc_controller_result *out)
{
    (void)request;
    (void)context;
    (void)out;
    return 7;
}

static int64_t good_presenter(
    const arbor_mvc_request *request,
    void *context,
    const arbor_mvc_controller_result *result,
    arbor_response_plan *out)
{
    (void)request;
    (void)context;
    if (result == NULL || out == NULL) {
        return -EINVAL;
    }
    *out = (arbor_response_plan){200u, result->model_data, result->model_size,
                                ARBOR_RESPONSE_PLAN_FLAG_KEEP_ALIVE};
    return 0;
}

static int64_t self_referential_presenter(
    const arbor_mvc_request *request,
    void *context,
    const arbor_mvc_controller_result *result,
    arbor_response_plan *out)
{
    (void)request;
    (void)context;
    (void)result;
    if (out == NULL) {
        return -EINVAL;
    }
    *out = (arbor_response_plan){200u, (const uint8_t *)out, sizeof(*out),
                                ARBOR_RESPONSE_PLAN_FLAG_KEEP_ALIVE};
    return 0;
}

static int64_t self_referential_after(
    const arbor_mvc_request *request,
    void *context,
    const arbor_response_plan *current,
    arbor_response_plan *out)
{
    (void)request;
    (void)context;
    (void)current;
    if (out == NULL) {
        return -EINVAL;
    }
    *out = (arbor_response_plan){200u, (const uint8_t *)out, sizeof(*out),
                                ARBOR_RESPONSE_PLAN_FLAG_KEEP_ALIVE};
    return 0;
}

static int64_t bad_before(
    const arbor_mvc_request *request,
    void *context,
    arbor_mvc_middleware_before_result *out)
{
    (void)request;
    (void)context;
    if (out == NULL) {
        return -EINVAL;
    }
    *out = (arbor_mvc_middleware_before_result){
        ARBOR_MVC_MIDDLEWARE_CONTINUE,
        0u,
        {200u, NULL, 0u, 0u}
    };
    return 0;
}

static int setup_scope(arbor_asm_arena *arena, arbor_request_view *view, arbor_request_scope *scope)
{
    static const uint8_t request[] = "GET /x/1 HTTP/1.1\r\nHost: local\r\n\r\n";
    uint64_t required = 0u;
    arbor_status status = arbor_request_parse(
        (arbor_span){request, sizeof(request) - 1u}, view, &required);
    if (status.native != 0) {
        return 1;
    }
    return arbor_request_scope_make(view, NULL, 0u, arena, scope).native == 0 ? 0 : 1;
}

int main(void)
{
    arbor_mvc_requirements requirements = {99u};
    if (arbor_mvc_catalog_measure(NULL, &requirements).native != -EINVAL ||
        requirements.route_validation_param_capacity != 99u) {
        return fail("MVC0 NULL catalog / output atomicity");
    }

    arbor_mvc_catalog invalid_version = {9u, (uint32_t)sizeof(arbor_mvc_catalog), 0u, NULL, 0u, NULL, 0u};
    if (arbor_mvc_catalog_measure(&invalid_version, &requirements).native != -EINVAL) {
        return fail("MVC0 catalog ABI rejection");
    }

    arbor_mvc_middleware_descriptor no_callbacks = {
        ARBOR_MVC_ABI_VERSION, (uint32_t)sizeof(arbor_mvc_middleware_descriptor), 0u,
        NULL, NULL, NULL
    };
    arbor_mvc_catalog bad_middleware_catalog = {
        ARBOR_MVC_ABI_VERSION, (uint32_t)sizeof(arbor_mvc_catalog), 0u,
        NULL, 0u, &no_callbacks, 1u
    };
    if (arbor_mvc_catalog_measure(&bad_middleware_catalog, &requirements).native != -EINVAL) {
        return fail("MVC0 middleware callback-shape rejection");
    }

    uint64_t duplicate_indices[2] = {0u, 0u};
    arbor_mvc_middleware_descriptor middleware = {
        ARBOR_MVC_ABI_VERSION, (uint32_t)sizeof(arbor_mvc_middleware_descriptor), 0u,
        NULL, bad_before, NULL
    };
    arbor_mvc_route duplicate_index_route = {
        (const uint8_t *)"GET", 3u, (const uint8_t *)"/x/:id", 6u,
        good_controller, NULL, good_presenter, NULL,
        duplicate_indices, 2u
    };
    arbor_mvc_catalog duplicate_index_catalog = {
        ARBOR_MVC_ABI_VERSION, (uint32_t)sizeof(arbor_mvc_catalog), 0u,
        &duplicate_index_route, 1u, &middleware, 1u
    };
    if (arbor_mvc_catalog_measure(&duplicate_index_catalog, &requirements).native != -EINVAL) {
        return fail("MVC0 duplicate middleware index rejection");
    }

    arbor_mvc_route out_of_range_route = duplicate_index_route;
    uint64_t out_of_range_indices[1] = {1u};
    out_of_range_route.middleware_indices = out_of_range_indices;
    out_of_range_route.middleware_count = 1u;
    arbor_mvc_catalog out_of_range_catalog = duplicate_index_catalog;
    out_of_range_catalog.routes = &out_of_range_route;
    if (arbor_mvc_catalog_measure(&out_of_range_catalog, &requirements).native != -EINVAL) {
        return fail("MVC0 middleware index range rejection");
    }

    arbor_mvc_route duplicate_routes[2] = {
        {(const uint8_t *)"GET", 3u, (const uint8_t *)"/x/:id", 6u,
         good_controller, NULL, good_presenter, NULL, NULL, 0u},
        {(const uint8_t *)"GET", 3u, (const uint8_t *)"/x/:id", 6u,
         good_controller, NULL, good_presenter, NULL, NULL, 0u}
    };
    arbor_mvc_catalog duplicate_catalog = {
        ARBOR_MVC_ABI_VERSION, (uint32_t)sizeof(arbor_mvc_catalog), 0u,
        duplicate_routes, 2u, NULL, 0u
    };
    arbor_route_param params[16] = {0};
    arbor_mvc_prepare_workspace workspace = {params, 16u};
    arbor_mvc_application unchanged = {9u, 9u, 9u, NULL, 9u, 9u};
    arbor_mvc_application app = unchanged;
    if (arbor_mvc_application_prepare(&duplicate_catalog, &workspace, &app).native != -EEXIST ||
        memcmp(&app, &unchanged, sizeof(app)) != 0) {
        return fail("MVC0 exact duplicate route rejection / output atomicity");
    }

    arbor_mvc_route invalid_pattern = {
        (const uint8_t *)"GET", 3u, (const uint8_t *)"/x/:", 4u,
        good_controller, NULL, good_presenter, NULL, NULL, 0u
    };
    arbor_mvc_catalog invalid_pattern_catalog = {
        ARBOR_MVC_ABI_VERSION, (uint32_t)sizeof(arbor_mvc_catalog), 0u,
        &invalid_pattern, 1u, NULL, 0u
    };
    app = unchanged;
    if (arbor_mvc_application_prepare(&invalid_pattern_catalog, &workspace, &app).native != -EINVAL ||
        memcmp(&app, &unchanged, sizeof(app)) != 0) {
        return fail("MVC0 invalid route pattern rejection");
    }

    arbor_mvc_route good_route = {
        (const uint8_t *)"GET", 3u, (const uint8_t *)"/x/:id", 6u,
        good_controller, NULL, good_presenter, NULL, NULL, 0u
    };
    arbor_mvc_catalog good_catalog = {
        ARBOR_MVC_ABI_VERSION, (uint32_t)sizeof(arbor_mvc_catalog), 0u,
        &good_route, 1u, NULL, 0u
    };
    arbor_mvc_prepare_workspace tiny_workspace = {params, 1u};
    if (arbor_mvc_application_prepare(&good_catalog, &tiny_workspace, &app).native != -ENOSPC) {
        return fail("MVC0 prepare workspace capacity rejection");
    }

    arbor_mvc_application guarded_app = {0};
    if (arbor_mvc_application_prepare(&good_catalog, &workspace, &guarded_app).native != 0) {
        return fail("MVC0 guarded application prepare");
    }
    arbor_mvc_application corrupt_max = guarded_app;
    corrupt_max.max_route_parameter_count = 0u;
    if (arbor_mvc_application_validate(&corrupt_max).native != -EINVAL) {
        return fail("MVC0 max-route-parameter guard corruption rejection");
    }
    arbor_mvc_application corrupt_guard = guarded_app;
    corrupt_guard.max_route_parameter_count_guard ^= UINT64_C(1);
    if (arbor_mvc_application_validate(&corrupt_guard).native != -EINVAL) {
        return fail("MVC0 max-route-parameter guard-field corruption rejection");
    }

    /* Workspace metadata must not overlap route-param scratch that the matcher writes. */
    arbor_mvc_prepare_workspace self_workspace = {0};
    self_workspace.route_params = (arbor_route_param *)(void *)&self_workspace;
    self_workspace.route_param_capacity = 6u;
    app = unchanged;
    if (arbor_mvc_application_prepare(&good_catalog, &self_workspace, &app).native != -EINVAL ||
        memcmp(&app, &unchanged, sizeof(app)) != 0) {
        return fail("MVC0 prepare workspace/scratch alias rejection");
    }

    if (arbor_mvc_controller_result_validate(
            &(arbor_mvc_controller_result){1u, 1u, NULL, 0u}).native != -EINVAL ||
        arbor_mvc_controller_result_validate(
            &(arbor_mvc_controller_result){1u, 0u, NULL, 4u}).native != -EINVAL) {
        return fail("MVC0 controller result validation");
    }

    if (arbor_mvc_middleware_before_result_validate(
            &(arbor_mvc_middleware_before_result){ARBOR_MVC_MIDDLEWARE_CONTINUE, 0u,
                                                  {200u, NULL, 0u, 0u}}).native != -EINVAL ||
        arbor_mvc_middleware_before_result_validate(
            &(arbor_mvc_middleware_before_result){8u, 0u, {0u, NULL, 0u, 0u}}).native != -EINVAL) {
        return fail("MVC0 middleware result validation");
    }

    arbor_mvc_middleware_before_result self_response = {
        ARBOR_MVC_MIDDLEWARE_RESPOND, 0u, {200u, NULL, 0u, 0u}
    };
    self_response.response.body_data = (const uint8_t *)&self_response;
    self_response.response.body_length = sizeof(self_response);
    if (arbor_mvc_middleware_before_result_validate(&self_response).native != -EINVAL) {
        return fail("MVC0 middleware self-referential response rejection");
    }

    arbor_mvc_middleware_before_result overflow_response = {
        ARBOR_MVC_MIDDLEWARE_RESPOND, 0u,
        {200u, (const uint8_t *)(uintptr_t)(UINTPTR_MAX - 1u), 4u, 0u}
    };
    if (arbor_mvc_middleware_before_result_validate(&overflow_response).native != -EINVAL) {
        return fail("MVC0 middleware response span-overflow rejection");
    }

    /* Positive controller native result is invalid and response output remains unchanged. */
    arbor_mvc_route positive_route = good_route;
    positive_route.controller = positive_controller;
    arbor_mvc_catalog positive_catalog = good_catalog;
    positive_catalog.routes = &positive_route;
    arbor_mvc_application positive_app = {0};
    if (arbor_mvc_application_prepare(&positive_catalog, &workspace, &positive_app).native != 0) {
        return fail("MVC0 positive-controller app prepare");
    }
    arbor_application_capabilities positive_caps = {0};
    if (arbor_mvc_application_capabilities_make(&positive_app, &positive_caps).native != 0) {
        return fail("MVC0 positive-controller caps");
    }
    uint8_t arena_bytes[512] = {0};
    arbor_asm_arena arena = {0};
    if (arena_init(&arena, arena_bytes, sizeof(arena_bytes)).status != 0) {
        return fail("MVC0 adversarial arena init");
    }
    arbor_request_view view = {0};
    arbor_request_scope scope = {0};
    if (setup_scope(&arena, &view, &scope) != 0) {
        return fail("MVC0 adversarial scope setup");
    }

    arbor_route_param bad_param = {(const uint8_t *)"id", 2u, (const uint8_t *)"", 0u};
    arbor_request_scope bad_param_scope = scope;
    bad_param_scope.params = &bad_param;
    bad_param_scope.parameter_count = 1u;
    arbor_mvc_request bad_request = {&bad_param_scope, &good_route, &bad_param, 1u, 0u};
    if (arbor_mvc_request_validate(&bad_request).native != -EINVAL) {
        return fail("MVC0 route-parameter span validation");
    }

    arbor_response_plan response_sentinel = {201u, (const uint8_t *)"z", 1u, 0u};
    arbor_response_plan response = response_sentinel;
    if (arbor_application_invoke(&positive_caps, &scope, &response).native != -EINVAL ||
        memcmp(&response, &response_sentinel, sizeof(response)) != 0) {
        return fail("MVC0 positive callback rejection / response atomicity");
    }

    /* Before callback returned an invalid CONTINUE response shape. */
    uint64_t mw_index[1] = {0u};
    arbor_mvc_route bad_before_route = good_route;
    bad_before_route.middleware_indices = mw_index;
    bad_before_route.middleware_count = 1u;
    arbor_mvc_catalog bad_before_catalog = {
        ARBOR_MVC_ABI_VERSION, (uint32_t)sizeof(arbor_mvc_catalog), 0u,
        &bad_before_route, 1u, &middleware, 1u
    };
    arbor_mvc_application bad_before_app = {0};
    if (arbor_mvc_application_prepare(&bad_before_catalog, &workspace, &bad_before_app).native != 0) {
        return fail("MVC0 bad-before app prepare");
    }
    arbor_application_capabilities bad_before_caps = {0};
    if (arbor_mvc_application_capabilities_make(&bad_before_app, &bad_before_caps).native != 0) {
        return fail("MVC0 bad-before caps");
    }
    (void)arena_reset(&arena);
    if (setup_scope(&arena, &view, &scope) != 0) {
        return fail("MVC0 bad-before scope");
    }
    response = response_sentinel;
    if (arbor_application_invoke(&bad_before_caps, &scope, &response).native != -EINVAL ||
        memcmp(&response, &response_sentinel, sizeof(response)) != 0) {
        return fail("MVC0 invalid middleware result publication atomicity");
    }

    /* Presenter output cannot publish a body pointer into its transient output object. */
    arbor_mvc_route self_presenter_route = good_route;
    self_presenter_route.presenter = self_referential_presenter;
    arbor_mvc_catalog self_presenter_catalog = good_catalog;
    self_presenter_catalog.routes = &self_presenter_route;
    arbor_mvc_application self_presenter_app = {0};
    if (arbor_mvc_application_prepare(
            &self_presenter_catalog, &workspace, &self_presenter_app).native != 0) {
        return fail("MVC0 self-presenter app prepare");
    }
    arbor_application_capabilities self_presenter_caps = {0};
    if (arbor_mvc_application_capabilities_make(
            &self_presenter_app, &self_presenter_caps).native != 0) {
        return fail("MVC0 self-presenter capabilities");
    }
    (void)arena_reset(&arena);
    if (setup_scope(&arena, &view, &scope) != 0) {
        return fail("MVC0 self-presenter scope");
    }
    response = response_sentinel;
    if (arbor_application_invoke(&self_presenter_caps, &scope, &response).native != -EINVAL ||
        memcmp(&response, &response_sentinel, sizeof(response)) != 0) {
        return fail("MVC0 transient presenter-body rejection / output atomicity");
    }

    /* After middleware likewise cannot publish its transient output object as body storage. */
    arbor_mvc_middleware_descriptor self_after_middleware = {
        ARBOR_MVC_ABI_VERSION, (uint32_t)sizeof(arbor_mvc_middleware_descriptor), 0u,
        NULL, NULL, self_referential_after
    };
    uint64_t self_after_index[1] = {0u};
    arbor_mvc_route self_after_route = good_route;
    self_after_route.middleware_indices = self_after_index;
    self_after_route.middleware_count = 1u;
    arbor_mvc_catalog self_after_catalog = {
        ARBOR_MVC_ABI_VERSION, (uint32_t)sizeof(arbor_mvc_catalog), 0u,
        &self_after_route, 1u, &self_after_middleware, 1u
    };
    arbor_mvc_application self_after_app = {0};
    if (arbor_mvc_application_prepare(&self_after_catalog, &workspace, &self_after_app).native != 0) {
        return fail("MVC0 self-after app prepare");
    }
    arbor_application_capabilities self_after_caps = {0};
    if (arbor_mvc_application_capabilities_make(&self_after_app, &self_after_caps).native != 0) {
        return fail("MVC0 self-after capabilities");
    }
    (void)arena_reset(&arena);
    if (setup_scope(&arena, &view, &scope) != 0) {
        return fail("MVC0 self-after scope");
    }
    response = response_sentinel;
    if (arbor_application_invoke(&self_after_caps, &scope, &response).native != -EINVAL ||
        memcmp(&response, &response_sentinel, sizeof(response)) != 0) {
        return fail("MVC0 transient after-body rejection / output atomicity");
    }

    puts("PASS: MVC0 adversarial catalog, alias, lifetime, callback and publication qualification");
    return 0;
}
