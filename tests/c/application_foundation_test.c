#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <arborcore/application.h>

static int fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    return 1;
}

static bool plan_equal(arbor_response_plan left, arbor_response_plan right)
{
    return left.status == right.status &&
           left.body_data == right.body_data &&
           left.body_length == right.body_length &&
           left.flags == right.flags;
}

typedef struct test_context {
    uint64_t calls;
    uint32_t mode;
} test_context;

static const uint8_t body_ok[] = {'o', 'k'};

static int64_t test_dispatch(
    const arbor_request_scope *scope,
    void *application_context,
    arbor_response_plan *response_out)
{
    test_context *context = (test_context *)application_context;
    if (scope == NULL || context == NULL || response_out == NULL) {
        return -EINVAL;
    }

    context->calls += UINT64_C(1);
    if (context->mode == 1u) {
        *response_out = (arbor_response_plan){
            UINT64_C(500), body_ok, (uint64_t)sizeof(body_ok), ARBOR_RESPONSE_PLAN_FLAG_NONE
        };
        return -EIO;
    }
    if (context->mode == 2u) {
        *response_out = (arbor_response_plan){
            UINT64_C(202), body_ok, (uint64_t)sizeof(body_ok), ARBOR_RESPONSE_PLAN_FLAG_NONE
        };
        return 0;
    }
    if (context->mode == 3u) {
        *response_out = (arbor_response_plan){
            UINT64_C(200), body_ok, (uint64_t)sizeof(body_ok), ARBOR_RESPONSE_PLAN_FLAG_NONE
        };
        return INT64_C(1);
    }

    arbor_response_plan candidate = {0};
    arbor_status status = arbor_response_plan_make(
        UINT64_C(200),
        (arbor_span){body_ok, (uint64_t)sizeof(body_ok)},
        ARBOR_RESPONSE_PLAN_FLAG_KEEP_ALIVE,
        &candidate);
    if (status.native != 0) {
        return status.native;
    }
    *response_out = candidate;
    return 0;
}

int main(void)
{
    if (sizeof(arbor_request_scope) != 40u ||
        offsetof(arbor_request_scope, request) != 0u ||
        offsetof(arbor_request_scope, target) != 8u ||
        offsetof(arbor_request_scope, params) != 16u ||
        offsetof(arbor_request_scope, parameter_count) != 24u ||
        offsetof(arbor_request_scope, arena) != 32u) {
        return fail("request scope stable callback layout");
    }
    if (sizeof(arbor_response_plan) != 32u ||
        offsetof(arbor_response_plan, body_data) != 8u ||
        offsetof(arbor_response_plan, body_length) != 16u ||
        offsetof(arbor_response_plan, flags) != 24u) {
        return fail("response plan stable callback layout");
    }
    if (sizeof(arbor_application_capabilities) != 32u ||
        offsetof(arbor_application_capabilities, request_dispatch) != 24u) {
        return fail("capabilities stable callback layout");
    }

    uint8_t arena_bytes[128] = {0};
    arbor_asm_arena arena = {arena_bytes, (uint64_t)sizeof(arena_bytes), 0u};
    arbor_request_view request = {0};
    arbor_route_param params[1] = {0};
    arbor_request_scope scope = {0};

    arbor_status status = arbor_request_scope_make(&request, NULL, 0u, &arena, &scope);
    if (status.native != 0 || scope.request != &request.native || scope.target != &request.target ||
        scope.params != NULL || scope.parameter_count != 0u || scope.arena != &arena) {
        return fail("valid request scope construction");
    }

    arbor_request_scope sentinel_scope = {&request.native, &request.target, params, UINT64_C(9), &arena};
    arbor_request_scope unchanged_scope = sentinel_scope;
    status = arbor_request_scope_make(&request, NULL, 1u, &arena, &sentinel_scope);
    if (status.native != -EINVAL ||
        sentinel_scope.request != unchanged_scope.request ||
        sentinel_scope.target != unchanged_scope.target ||
        sentinel_scope.params != unchanged_scope.params ||
        sentinel_scope.parameter_count != unchanged_scope.parameter_count ||
        sentinel_scope.arena != unchanged_scope.arena) {
        return fail("request scope failure is transactional");
    }

    arena.offset = arena.capacity + UINT64_C(1);
    status = arbor_request_scope_validate(&scope);
    if (status.native != -EINVAL) {
        return fail("invalid arena invariant rejected");
    }
    arena.offset = 0u;

    static const uint64_t supported[] = {
        UINT64_C(200), UINT64_C(201), UINT64_C(204),
        UINT64_C(400), UINT64_C(404), UINT64_C(500)
    };
    for (size_t i = 0u; i < sizeof(supported) / sizeof(supported[0]); ++i) {
        arbor_span body = {body_ok, (uint64_t)sizeof(body_ok)};
        if (supported[i] == UINT64_C(204)) {
            body = (arbor_span){NULL, 0u};
        }
        arbor_response_plan plan = {0};
        status = arbor_response_plan_make(
            supported[i], body, ARBOR_RESPONSE_PLAN_FLAG_NONE, &plan);
        if (status.native != 0 || plan.status != supported[i]) {
            return fail("supported response status");
        }
    }

    arbor_response_plan sentinel_plan = {
        UINT64_C(500), body_ok, (uint64_t)sizeof(body_ok), ARBOR_RESPONSE_PLAN_FLAG_KEEP_ALIVE
    };
    arbor_response_plan unchanged_plan = sentinel_plan;
    status = arbor_response_plan_make(
        UINT64_C(202),
        (arbor_span){body_ok, (uint64_t)sizeof(body_ok)},
        ARBOR_RESPONSE_PLAN_FLAG_NONE,
        &sentinel_plan);
    if (status.native != -EINVAL || !plan_equal(sentinel_plan, unchanged_plan)) {
        return fail("unsupported response status transactional failure");
    }

    status = arbor_response_plan_make(
        UINT64_C(204),
        (arbor_span){body_ok, (uint64_t)sizeof(body_ok)},
        ARBOR_RESPONSE_PLAN_FLAG_NONE,
        &sentinel_plan);
    if (status.native != -EINVAL) {
        return fail("204 nonempty body rejected");
    }

    status = arbor_response_plan_make(
        UINT64_C(200),
        (arbor_span){NULL, UINT64_C(1)},
        ARBOR_RESPONSE_PLAN_FLAG_NONE,
        &sentinel_plan);
    if (status.native != -EINVAL) {
        return fail("nonempty NULL response body rejected");
    }

    status = arbor_response_plan_make(
        UINT64_C(200),
        (arbor_span){body_ok, (uint64_t)sizeof(body_ok)},
        UINT64_C(2),
        &sentinel_plan);
    if (status.native != -EINVAL) {
        return fail("unknown response flags rejected");
    }

    test_context context = {0u, 0u};
    arbor_application_capabilities capabilities = {0};
    status = arbor_application_capabilities_make(test_dispatch, &context, &capabilities);
    if (status.native != 0 ||
        capabilities.abi_version != ARBOR_APPLICATION_CAPABILITIES_ABI_VERSION ||
        capabilities.struct_size != (uint32_t)sizeof(capabilities) ||
        capabilities.request_dispatch != test_dispatch ||
        capabilities.application_context != &context) {
        return fail("valid capability table construction");
    }

    arbor_application_capabilities bad_capabilities = capabilities;
    bad_capabilities.abi_version += 1u;
    if (arbor_application_capabilities_validate(&bad_capabilities).native != -EINVAL) {
        return fail("capability version rejection");
    }
    bad_capabilities = capabilities;
    bad_capabilities.struct_size -= 1u;
    if (arbor_application_capabilities_validate(&bad_capabilities).native != -EINVAL) {
        return fail("capability size rejection");
    }
    bad_capabilities = capabilities;
    bad_capabilities.flags = UINT64_C(1);
    if (arbor_application_capabilities_validate(&bad_capabilities).native != -EINVAL) {
        return fail("capability flags rejection");
    }
    bad_capabilities = capabilities;
    bad_capabilities.request_dispatch = NULL;
    if (arbor_application_capabilities_validate(&bad_capabilities).native != -EINVAL) {
        return fail("capability callback rejection");
    }

    arbor_response_plan invoked = {UINT64_C(500), NULL, 0u, 0u};
    status = arbor_application_invoke(&capabilities, &scope, &invoked);
    if (status.native != 0 || context.calls != UINT64_C(1) ||
        invoked.status != UINT64_C(200) ||
        invoked.body_data != body_ok ||
        invoked.body_length != (uint64_t)sizeof(body_ok) ||
        invoked.flags != ARBOR_RESPONSE_PLAN_FLAG_KEEP_ALIVE) {
        return fail("framework-to-application capability invocation");
    }

    unchanged_plan = invoked;
    context.mode = 1u;
    status = arbor_application_invoke(&capabilities, &scope, &invoked);
    if (status.native != -EIO || !plan_equal(invoked, unchanged_plan)) {
        return fail("callback mechanism failure is canonical and transactional");
    }

    context.mode = 2u;
    status = arbor_application_invoke(&capabilities, &scope, &invoked);
    if (status.native != -EINVAL || !plan_equal(invoked, unchanged_plan)) {
        return fail("invalid callback response is rejected transactionally");
    }

    context.mode = 3u;
    status = arbor_application_invoke(&capabilities, &scope, &invoked);
    if (status.native != -EINVAL || !plan_equal(invoked, unchanged_plan)) {
        return fail("positive callback status is reserved and rejected transactionally");
    }

    context.mode = 0u;
    uint8_t output_bytes[256] = {0};
    arbor_asm_buffer output = {0};
    arbor_asm_result_u64 init_result = buffer_init(
        &output,
        output_bytes,
        (uint64_t)sizeof(output_bytes));
    if (init_result.status != 0) {
        return fail("lower buffer initialization");
    }

    arbor_response_plan serial_plan = {0};
    status = arbor_response_plan_make(
        UINT64_C(200),
        (arbor_span){body_ok, (uint64_t)sizeof(body_ok)},
        ARBOR_RESPONSE_PLAN_FLAG_KEEP_ALIVE,
        &serial_plan);
    if (status.native != 0) {
        return fail("serialization plan construction");
    }

    uint64_t written = 0u;
    status = arbor_response_plan_serialize(&output, &serial_plan, &written);
    static const char expected[] =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 2\r\n"
        "Connection: keep-alive\r\n"
        "\r\n"
        "ok";
    if (status.native != 0 ||
        written != (uint64_t)(sizeof(expected) - 1u) ||
        output.length != written ||
        memcmp(output.data, expected, sizeof(expected) - 1u) != 0) {
        return fail("application-to-framework response serialization");
    }

    puts("PASS: AF0-AF1 request scope, response plan, stable capability ABI and bidirectional foundation");
    return 0;
}
