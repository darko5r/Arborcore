#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <arborcore/http_mvc.h>

extern arbor_status http1_asm_call_measure(uint64_t capacity, arbor_http_mvc_requirements *out);

_Static_assert(sizeof(arbor_status) == 16u, "HTTP1 ABI test requires 16-byte arbor_status");
_Static_assert(offsetof(arbor_status, code) == 0u, "HTTP1 arbor_status code offset drift");
_Static_assert(offsetof(arbor_status, native) == 8u, "HTTP1 arbor_status native offset drift");

static int fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    return 1;
}

static int64_t dummy_controller(
    const arbor_mvc_request *request,
    void *context,
    arbor_mvc_controller_result *out)
{
    (void)request;
    (void)context;
    if (out == NULL) {
        return -EINVAL;
    }
    *out = (arbor_mvc_controller_result){0u, 0u, NULL, 0u};
    return 0;
}

static int64_t dummy_presenter(
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
    *out = (arbor_response_plan){204u, NULL, 0u, ARBOR_RESPONSE_PLAN_FLAG_KEEP_ALIVE};
    return 0;
}

int main(void)
{
    arbor_http_mvc_requirements abi_requirements = {0u, 0u};
    arbor_status abi_status = http1_asm_call_measure(4u, &abi_requirements);
    if (abi_status.native != 0 || abi_requirements.arena_prefix_bytes != 231u ||
        abi_requirements.response_field_capacity != 4u) {
        return fail("HTTP1 real NASM -> C measure ABI");
    }

    arbor_http_mvc_requirements requirements = {0u, 0u};
    arbor_status status = arbor_http_mvc_application_measure(4u, &requirements);
    if (status.native != 0 || requirements.response_field_capacity != 4u ||
        requirements.arena_prefix_bytes != 231u) {
        return fail("HTTP1 deterministic arena-prefix measurement");
    }

    arbor_mvc_catalog catalog = {
        ARBOR_MVC_ABI_VERSION,
        (uint32_t)sizeof(arbor_mvc_catalog),
        ARBOR_MVC_CATALOG_FLAGS_NONE,
        NULL,
        0u,
        NULL,
        0u
    };
    arbor_mvc_prepare_workspace workspace = {NULL, 0u};
    arbor_mvc_application mvc = {0};
    status = arbor_mvc_application_prepare(&catalog, &workspace, &mvc);
    if (status.native != 0) {
        return fail("empty MVC application preparation");
    }

    arbor_http_mvc_application app = {0};
    status = arbor_http_mvc_application_prepare(&mvc, 4u, &app);
    if (status.native != 0 || app.abi_version != ARBOR_HTTP_MVC_ABI_VERSION ||
        app.struct_size != sizeof(app) || app.mvc_application != &mvc ||
        app.response_field_capacity != 4u || app.arena_prefix_bytes != 231u ||
        app.mvc_capabilities.application_context != &mvc) {
        return fail("HTTP1 application preparation");
    }
    if (arbor_http_mvc_application_validate(&app).native != 0) {
        return fail("HTTP1 prepared application validation");
    }

    arbor_http_mvc_application corrupt = app;
    corrupt.prepared_guard ^= UINT64_C(1);
    if (arbor_http_mvc_application_validate(&corrupt).native != -EINVAL) {
        return fail("HTTP1 prepared-integrity guard");
    }

    arbor_http_mvc_requirements unchanged = {77u, 88u};
    status = arbor_http_mvc_application_measure(UINT64_MAX, &unchanged);
    if (status.native != -EOVERFLOW || unchanged.arena_prefix_bytes != 77u ||
        unchanged.response_field_capacity != 88u) {
        return fail("HTTP1 measure overflow is transactional");
    }

    /* A valid generic MVC request is not automatically an HTTP1 request. The
     * HTTP1 field API must reject a missing locator rather than accepting it. */
    uint8_t padded[256] = {0};
    arbor_asm_arena generic_arena = {padded + 64u, 128u, 0u};
    arbor_asm_http_request native = {0};
    arbor_asm_request_target target = {0};
    arbor_request_scope scope = {&native, &target, NULL, 0u, &generic_arena};
    static const uint8_t method[] = "GET";
    static const uint8_t pattern[] = "/";
    arbor_mvc_route route = {
        method, sizeof(method) - 1u,
        pattern, sizeof(pattern) - 1u,
        dummy_controller, NULL,
        dummy_presenter, NULL,
        NULL, 0u
    };
    arbor_mvc_request request = {&scope, &route, NULL, 0u, 0u};
    static const uint8_t name[] = "Content-Type";
    static const uint8_t value[] = "text/plain";
    status = arbor_http_mvc_response_field_append(
        &request,
        (arbor_span){name, sizeof(name) - 1u},
        (arbor_span){value, sizeof(value) - 1u});
    if (status.native != -EINVAL) {
        return fail("HTTP1 field API rejects non-HTTP1 MVC request");
    }

    puts("PASS: HTTP1 application preparation, AF1 retrofit boundary and sidecar API preconditions");
    return 0;
}
