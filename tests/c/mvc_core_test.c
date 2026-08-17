#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <arborcore/mvc.h>

static int fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    return 1;
}

typedef struct mvc0_asm_callback_context {
    uint64_t controller_calls;
    uint64_t presenter_calls;
    uint64_t before_calls;
    uint64_t after_calls;
    uint64_t stack_errors;
    const uint8_t *body;
    uint64_t body_length;
    uint64_t middleware_action;
    uint64_t response_status;
    uint64_t keep_alive;
} mvc0_asm_callback_context;

extern int64_t mvc0_asm_controller(
    const arbor_mvc_request *request, void *context, arbor_mvc_controller_result *out);
extern int64_t mvc0_asm_presenter(
    const arbor_mvc_request *request, void *context,
    const arbor_mvc_controller_result *result, arbor_response_plan *out);
extern int64_t mvc0_asm_before(
    const arbor_mvc_request *request, void *context,
    arbor_mvc_middleware_before_result *out);
extern int64_t mvc0_asm_after(
    const arbor_mvc_request *request, void *context,
    const arbor_response_plan *current, arbor_response_plan *out);
extern arbor_status mvc0_asm_call_request_validate(const arbor_mvc_request *request);

_Static_assert(sizeof(mvc0_asm_callback_context) == 80u, "MVC0 ASM callback context drift");

typedef struct test_trace {
    uint64_t count;
    uint64_t events[32];
    bool short_circuit;
} test_trace;

static void trace_add(test_trace *trace, uint64_t event)
{
    if (trace != NULL && trace->count < 32u) {
        trace->events[trace->count++] = event;
    }
}

static int64_t before_one(
    const arbor_mvc_request *request,
    void *context,
    arbor_mvc_middleware_before_result *out)
{
    if (arbor_mvc_request_validate(request).native != 0 || context == NULL || out == NULL) {
        return -EINVAL;
    }
    test_trace *trace = (test_trace *)context;
    trace_add(trace, 10u);
    *out = (arbor_mvc_middleware_before_result){
        ARBOR_MVC_MIDDLEWARE_CONTINUE,
        0u,
        {0u, NULL, 0u, 0u}
    };
    return 0;
}

static int64_t before_two(
    const arbor_mvc_request *request,
    void *context,
    arbor_mvc_middleware_before_result *out)
{
    if (arbor_mvc_request_validate(request).native != 0 || context == NULL || out == NULL) {
        return -EINVAL;
    }
    test_trace *trace = (test_trace *)context;
    trace_add(trace, 20u);
    if (trace->short_circuit) {
        static const uint8_t blocked[] = "blocked";
        *out = (arbor_mvc_middleware_before_result){
            ARBOR_MVC_MIDDLEWARE_RESPOND,
            0u,
            {400u, blocked, sizeof(blocked) - 1u, ARBOR_RESPONSE_PLAN_FLAG_KEEP_ALIVE}
        };
    } else {
        *out = (arbor_mvc_middleware_before_result){
            ARBOR_MVC_MIDDLEWARE_CONTINUE,
            0u,
            {0u, NULL, 0u, 0u}
        };
    }
    return 0;
}

static int64_t after_one(
    const arbor_mvc_request *request,
    void *context,
    const arbor_response_plan *current,
    arbor_response_plan *out)
{
    if (arbor_mvc_request_validate(request).native != 0 || context == NULL || current == NULL || out == NULL) {
        return -EINVAL;
    }
    test_trace *trace = (test_trace *)context;
    trace_add(trace, 60u);
    *out = *current;
    return 0;
}

static int64_t after_two(
    const arbor_mvc_request *request,
    void *context,
    const arbor_response_plan *current,
    arbor_response_plan *out)
{
    if (arbor_mvc_request_validate(request).native != 0 || context == NULL || current == NULL || out == NULL) {
        return -EINVAL;
    }
    test_trace *trace = (test_trace *)context;
    trace_add(trace, 50u);
    *out = *current;
    return 0;
}

static int64_t hello_controller(
    const arbor_mvc_request *request,
    void *context,
    arbor_mvc_controller_result *out)
{
    if (arbor_mvc_request_validate(request).native != 0 || context == NULL || out == NULL) {
        return -EINVAL;
    }
    test_trace *trace = (test_trace *)context;
    trace_add(trace, 30u);
    if (request->parameter_count != 1u || request->params == NULL ||
        request->params[0].name_len != 4u || request->params[0].value_len != 3u ||
        memcmp(request->params[0].name_ptr, "name", 4u) != 0 ||
        memcmp(request->params[0].value_ptr, "Ada", 3u) != 0) {
        return -EINVAL;
    }
    static const uint8_t message[] = "Hello Ada";
    *out = (arbor_mvc_controller_result){1u, 0u, message, sizeof(message) - 1u};
    return 0;
}

static int64_t hello_presenter(
    const arbor_mvc_request *request,
    void *context,
    const arbor_mvc_controller_result *result,
    arbor_response_plan *out)
{
    if (arbor_mvc_request_validate(request).native != 0 || context == NULL || result == NULL || out == NULL) {
        return -EINVAL;
    }
    test_trace *trace = (test_trace *)context;
    trace_add(trace, 40u);
    *out = (arbor_response_plan){
        200u,
        (const uint8_t *)result->model_data,
        result->model_size,
        ARBOR_RESPONSE_PLAN_FLAG_KEEP_ALIVE
    };
    return 0;
}

static int64_t marker_controller(
    const arbor_mvc_request *request,
    void *context,
    arbor_mvc_controller_result *out)
{
    if (arbor_mvc_request_validate(request).native != 0 || context == NULL || out == NULL) {
        return -EINVAL;
    }
    uint64_t *marker = (uint64_t *)context;
    *marker += 1u;
    static const uint8_t body[] = "first";
    *out = (arbor_mvc_controller_result){2u, 0u, body, sizeof(body) - 1u};
    return 0;
}

static int64_t marker_presenter(
    const arbor_mvc_request *request,
    void *context,
    const arbor_mvc_controller_result *result,
    arbor_response_plan *out)
{
    (void)context;
    if (arbor_mvc_request_validate(request).native != 0 || result == NULL || out == NULL) {
        return -EINVAL;
    }
    *out = (arbor_response_plan){200u, result->model_data, result->model_size,
                                ARBOR_RESPONSE_PLAN_FLAG_KEEP_ALIVE};
    return 0;
}

static int make_scope(
    const char *request_text,
    arbor_asm_arena *arena,
    arbor_request_view *view,
    arbor_request_scope *scope)
{
    uint64_t required = 0u;
    arbor_status status = arbor_request_parse(
        (arbor_span){(const uint8_t *)request_text, (uint64_t)strlen(request_text)},
        view,
        &required);
    if (status.native != 0 || required == 0u) {
        return 1;
    }
    status = arbor_request_scope_make(view, NULL, 0u, arena, scope);
    return status.native == 0 ? 0 : 1;
}

int main(void)
{
    if (sizeof(arbor_mvc_requirements) != 8u ||
        sizeof(arbor_mvc_prepare_workspace) != 16u ||
        sizeof(arbor_mvc_controller_result) != 24u ||
        sizeof(arbor_mvc_middleware_before_result) != 40u ||
        sizeof(arbor_mvc_middleware_descriptor) != 40u ||
        sizeof(arbor_mvc_route) != 80u ||
        sizeof(arbor_mvc_catalog) != 48u ||
        sizeof(arbor_mvc_application) != 40u ||
        sizeof(arbor_mvc_request) != 40u) {
        return fail("MVC0 public layout sizes");
    }

    test_trace trace = {0u, {0u}, false};
    arbor_mvc_middleware_descriptor middleware[2] = {
        {ARBOR_MVC_ABI_VERSION, (uint32_t)sizeof(arbor_mvc_middleware_descriptor), 0u,
         &trace, before_one, after_one},
        {ARBOR_MVC_ABI_VERSION, (uint32_t)sizeof(arbor_mvc_middleware_descriptor), 0u,
         &trace, before_two, after_two}
    };
    uint64_t route_middleware[2] = {0u, 1u};
    arbor_mvc_route route = {
        (const uint8_t *)"GET", 3u,
        (const uint8_t *)"/hello/:name", 12u,
        hello_controller, &trace,
        hello_presenter, &trace,
        route_middleware, 2u
    };
    arbor_mvc_catalog catalog = {
        ARBOR_MVC_ABI_VERSION,
        (uint32_t)sizeof(arbor_mvc_catalog),
        0u,
        &route,
        1u,
        middleware,
        2u
    };

    arbor_mvc_requirements req = {0u};
    arbor_status status = arbor_mvc_catalog_measure(&catalog, &req);
    if (status.native != 0 || req.route_validation_param_capacity != 12u) {
        return fail("MVC0 catalog measurement");
    }

    arbor_route_param validation_params[16] = {0};
    arbor_mvc_prepare_workspace workspace = {validation_params, 16u};
    arbor_mvc_application application = {0};
    status = arbor_mvc_application_prepare(&catalog, &workspace, &application);
    if (status.native != 0 || arbor_mvc_application_validate(&application).native != 0 ||
        application.max_route_parameter_count != 1u ||
        application.max_route_parameter_count_guard != ~UINT64_C(1)) {
        return fail("MVC0 application prepare");
    }

    arbor_application_capabilities capabilities = {0};
    status = arbor_mvc_application_capabilities_make(&application, &capabilities);
    if (status.native != 0) {
        return fail("MVC0 AF1 capabilities make");
    }

    uint8_t arena_bytes[1024] = {0};
    arbor_asm_arena arena = {0};
    if (arena_init(&arena, arena_bytes, sizeof(arena_bytes)).status != 0) {
        return fail("MVC0 arena init");
    }
    arbor_request_view view = {0};
    arbor_request_scope scope = {0};
    if (make_scope("GET /hello/Ada HTTP/1.1\r\nHost: local\r\n\r\n", &arena, &view, &scope) != 0) {
        return fail("MVC0 request scope setup");
    }

    arbor_response_plan response = {0};
    status = arbor_application_invoke(&capabilities, &scope, &response);
    const uint64_t expected_trace[] = {10u, 20u, 30u, 40u, 50u, 60u};
    if (status.native != 0 || response.status != 200u || response.body_length != 9u ||
        memcmp(response.body_data, "Hello Ada", 9u) != 0 || trace.count != 6u ||
        memcmp(trace.events, expected_trace, sizeof(expected_trace)) != 0) {
        return fail("MVC0 middleware/controller/presenter pipeline");
    }

    (void)arena_reset(&arena);
    trace.count = 0u;
    trace.short_circuit = true;
    if (make_scope("GET /hello/Ada HTTP/1.1\r\nHost: local\r\n\r\n", &arena, &view, &scope) != 0) {
        return fail("MVC0 short-circuit scope setup");
    }
    response = (arbor_response_plan){0};
    status = arbor_application_invoke(&capabilities, &scope, &response);
    const uint64_t expected_short[] = {10u, 20u, 50u, 60u};
    if (status.native != 0 || response.status != 400u || response.body_length != 7u ||
        memcmp(response.body_data, "blocked", 7u) != 0 || trace.count != 4u ||
        memcmp(trace.events, expected_short, sizeof(expected_short)) != 0) {
        return fail("MVC0 middleware short-circuit and reverse after order");
    }

    (void)arena_reset(&arena);
    trace.short_circuit = false;
    if (make_scope("POST /hello/Ada HTTP/1.1\r\nHost: local\r\n\r\n", &arena, &view, &scope) != 0) {
        return fail("MVC0 404 scope setup");
    }
    response = (arbor_response_plan){0};
    status = arbor_application_invoke(&capabilities, &scope, &response);
    if (status.native != 0 || response.status != 404u || response.body_length != 0u) {
        return fail("MVC0 no-route 404 response");
    }

    /* First-match catalog order across semantically overlapping patterns. */
    uint64_t first_marker = 0u;
    uint64_t second_marker = 0u;
    arbor_mvc_route overlapping[2] = {
        {(const uint8_t *)"GET", 3u, (const uint8_t *)"/item/:id", 9u,
         marker_controller, &first_marker, marker_presenter, NULL, NULL, 0u},
        {(const uint8_t *)"GET", 3u, (const uint8_t *)"/item/:name", 11u,
         marker_controller, &second_marker, marker_presenter, NULL, NULL, 0u}
    };
    arbor_mvc_catalog overlap_catalog = {
        ARBOR_MVC_ABI_VERSION, (uint32_t)sizeof(arbor_mvc_catalog), 0u,
        overlapping, 2u, NULL, 0u
    };
    arbor_route_param overlap_workspace_params[16] = {0};
    arbor_mvc_prepare_workspace overlap_workspace = {overlap_workspace_params, 16u};
    arbor_mvc_application overlap_app = {0};
    status = arbor_mvc_application_prepare(&overlap_catalog, &overlap_workspace, &overlap_app);
    if (status.native != 0) {
        return fail("MVC0 overlap application prepare");
    }
    arbor_application_capabilities overlap_caps = {0};
    status = arbor_mvc_application_capabilities_make(&overlap_app, &overlap_caps);
    if (status.native != 0) {
        return fail("MVC0 overlap capabilities");
    }
    (void)arena_reset(&arena);
    if (make_scope("GET /item/42 HTTP/1.1\r\nHost: local\r\n\r\n", &arena, &view, &scope) != 0) {
        return fail("MVC0 overlap scope");
    }
    response = (arbor_response_plan){0};
    status = arbor_application_invoke(&overlap_caps, &scope, &response);
    if (status.native != 0 || first_marker != 1u || second_marker != 0u ||
        response.body_length != 5u) {
        return fail("MVC0 deterministic first-match catalog ordering");
    }

    /* Real Assembly controller/presenter/middleware callback ABI. */
    static const uint8_t asm_body[] = "asm-mvc";
    mvc0_asm_callback_context asm_context = {
        0u, 0u, 0u, 0u, 0u, asm_body, sizeof(asm_body) - 1u,
        ARBOR_MVC_MIDDLEWARE_CONTINUE, 200u, 1u
    };
    arbor_mvc_middleware_descriptor asm_middleware = {
        ARBOR_MVC_ABI_VERSION, (uint32_t)sizeof(arbor_mvc_middleware_descriptor), 0u,
        &asm_context, mvc0_asm_before, mvc0_asm_after
    };
    uint64_t asm_middleware_index[1] = {0u};
    arbor_mvc_route asm_route = {
        (const uint8_t *)"GET", 3u, (const uint8_t *)"/asm", 4u,
        mvc0_asm_controller, &asm_context, mvc0_asm_presenter, &asm_context,
        asm_middleware_index, 1u
    };
    arbor_mvc_catalog asm_catalog = {
        ARBOR_MVC_ABI_VERSION, (uint32_t)sizeof(arbor_mvc_catalog), 0u,
        &asm_route, 1u, &asm_middleware, 1u
    };
    arbor_route_param asm_workspace_params[8] = {0};
    arbor_mvc_prepare_workspace asm_workspace = {asm_workspace_params, 8u};
    arbor_mvc_application asm_app = {0};
    status = arbor_mvc_application_prepare(&asm_catalog, &asm_workspace, &asm_app);
    if (status.native != 0) {
        return fail("MVC0 ASM callback application prepare");
    }
    arbor_application_capabilities asm_caps = {0};
    status = arbor_mvc_application_capabilities_make(&asm_app, &asm_caps);
    if (status.native != 0) {
        return fail("MVC0 ASM callback capabilities");
    }
    (void)arena_reset(&arena);
    if (make_scope("GET /asm HTTP/1.1\r\nHost: local\r\n\r\n", &arena, &view, &scope) != 0) {
        return fail("MVC0 ASM callback scope");
    }
    response = (arbor_response_plan){0};
    status = arbor_application_invoke(&asm_caps, &scope, &response);
    if (status.native != 0 || response.status != 200u || response.body_length != 7u ||
        memcmp(response.body_data, asm_body, 7u) != 0 ||
        asm_context.controller_calls != 1u || asm_context.presenter_calls != 1u ||
        asm_context.before_calls != 1u || asm_context.after_calls != 1u ||
        asm_context.stack_errors != 0u) {
        return fail("MVC0 real Assembly callback register/stack ABI");
    }

    arbor_request_scope asm_routed_scope = scope;
    arbor_mvc_request asm_request = {&asm_routed_scope, &asm_route, NULL, 0u, 0u};
    arbor_status asm_validate = mvc0_asm_call_request_validate(&asm_request);
    if (asm_validate.native != 0) {
        return fail("MVC0 Assembly-to-C request validation ABI");
    }

    /* Rich response-plan serialization must preserve the qualified legacy
     * status-only serializer bytes when no body is present. */
    uint8_t rich_bytes[512] = {0};
    uint8_t legacy_bytes[512] = {0};
    arbor_asm_buffer rich_buffer = {0};
    arbor_asm_buffer legacy_buffer = {0};
    if (buffer_init(&rich_buffer, rich_bytes, sizeof(rich_bytes)).status != 0 ||
        buffer_init(&legacy_buffer, legacy_bytes, sizeof(legacy_bytes)).status != 0) {
        return fail("MVC0 serializer parity buffer init");
    }
    arbor_response_plan status_only = {200u, NULL, 0u, ARBOR_RESPONSE_PLAN_FLAG_KEEP_ALIVE};
    uint64_t rich_written = 0u;
    uint64_t legacy_written = 0u;
    status = arbor_response_plan_serialize(&rich_buffer, &status_only, &rich_written);
    if (status.native != 0) {
        return fail("MVC0 rich status-only serialization");
    }
    status = arbor_response_serialize(
        &legacy_buffer, 200u, (arbor_span){NULL, 0u}, true, &legacy_written);
    if (status.native != 0 || rich_written != legacy_written ||
        rich_buffer.length != legacy_buffer.length ||
        memcmp(rich_bytes, legacy_bytes, (size_t)rich_written) != 0) {
        return fail("MVC0 rich/legacy status-only serialized-byte parity");
    }

    arbor_mvc_catalog empty_catalog = {
        ARBOR_MVC_ABI_VERSION, (uint32_t)sizeof(arbor_mvc_catalog), 0u,
        NULL, 0u, NULL, 0u
    };
    arbor_mvc_prepare_workspace empty_workspace = {NULL, 0u};
    arbor_mvc_application empty_app = {0};
    status = arbor_mvc_application_prepare(&empty_catalog, &empty_workspace, &empty_app);
    if (status.native != 0) {
        return fail("MVC0 zero-route application prepare");
    }

    puts("PASS: MVC0 core routing, middleware, controller and presenter semantics");
    return 0;
}
