#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <arborcore/http_mvc.h>

static int fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    return 1;
}

static bool contains_bytes(
    const uint8_t *haystack, size_t haystack_length,
    const uint8_t *needle, size_t needle_length)
{
    if (needle_length == 0u) return true;
    if (needle_length > haystack_length) return false;
    for (size_t i = 0u; i + needle_length <= haystack_length; ++i) {
        if (memcmp(haystack + i, needle, needle_length) == 0) return true;
    }
    return false;
}

static const uint8_t *find_bytes(
    const uint8_t *haystack, size_t haystack_length,
    const uint8_t *needle, size_t needle_length)
{
    if (needle_length == 0u) return haystack;
    if (needle_length > haystack_length) return NULL;
    for (size_t i = 0u; i + needle_length <= haystack_length; ++i) {
        if (memcmp(haystack + i, needle, needle_length) == 0) return haystack + i;
    }
    return NULL;
}

typedef struct presenter_context {
    uint64_t calls;
} presenter_context;

static int64_t controller(
    const arbor_mvc_request *request,
    void *context,
    arbor_mvc_controller_result *out)
{
    (void)context;
    if (arbor_mvc_request_validate(request).native != 0 || out == NULL) return -EINVAL;
    *out = (arbor_mvc_controller_result){1u, 0u, NULL, 0u};
    return 0;
}

static int64_t middleware_before(
    const arbor_mvc_request *request,
    void *context,
    arbor_mvc_middleware_before_result *out)
{
    (void)context;
    if (request == NULL || out == NULL) return -EINVAL;
    static const uint8_t cookie[] = "Set-Cookie";
    static const uint8_t one[] = "one=1; Path=/";
    static const uint8_t transient_name[] = "X-Transient";
    static const uint8_t transient_value[] = "discard";
    uint64_t mark = UINT64_MAX;
    arbor_status status = arbor_http_mvc_response_fields_mark(request, &mark);
    if (status.native != 0) return status.native;
    status = arbor_http_mvc_response_field_append(
        request,
        (arbor_span){transient_name, sizeof(transient_name) - 1u},
        (arbor_span){transient_value, sizeof(transient_value) - 1u});
    if (status.native != 0) return status.native;
    status = arbor_http_mvc_response_fields_rewind(request, mark);
    if (status.native != 0) return status.native;
    status = arbor_http_mvc_response_field_append(
        request,
        (arbor_span){cookie, sizeof(cookie) - 1u},
        (arbor_span){one, sizeof(one) - 1u});
    if (status.native != 0) return status.native;
    *out = (arbor_mvc_middleware_before_result){ARBOR_MVC_MIDDLEWARE_CONTINUE, 0u, {0}};
    return 0;
}

static int64_t presenter(
    const arbor_mvc_request *request,
    void *context,
    const arbor_mvc_controller_result *result,
    arbor_response_plan *out)
{
    presenter_context *pc = (presenter_context *)context;
    if (request == NULL || pc == NULL || result == NULL || out == NULL) return -EINVAL;
    pc->calls += 1u;
    static const uint8_t content_type[] = "Content-Type";
    static const uint8_t content_value[] = "text/plain; charset=utf-8";
    static const uint8_t location[] = "Location";
    static const uint8_t destination[] = "/hello";
    static const uint8_t cookie[] = "Set-Cookie";
    static const uint8_t two[] = "two=2; Path=/";
    arbor_status status = arbor_http_mvc_response_field_append(
        request,
        (arbor_span){content_type, sizeof(content_type) - 1u},
        (arbor_span){content_value, sizeof(content_value) - 1u});
    if (status.native != 0) return status.native;
    status = arbor_http_mvc_response_field_append(
        request,
        (arbor_span){location, sizeof(location) - 1u},
        (arbor_span){destination, sizeof(destination) - 1u});
    if (status.native != 0) return status.native;
    status = arbor_http_mvc_response_field_append(
        request,
        (arbor_span){cookie, sizeof(cookie) - 1u},
        (arbor_span){two, sizeof(two) - 1u});
    if (status.native != 0) return status.native;
    *out = (arbor_response_plan){302u, NULL, 0u, ARBOR_RESPONSE_PLAN_FLAG_KEEP_ALIVE};
    return 0;
}

int main(void)
{
    static const uint8_t method[] = "GET";
    static const uint8_t pattern[] = "/go";
    static const uint64_t middleware_indices[] = {0u};
    presenter_context pc = {0u};
    arbor_mvc_middleware_descriptor middleware = {
        ARBOR_MVC_ABI_VERSION, (uint32_t)sizeof(arbor_mvc_middleware_descriptor),
        ARBOR_MVC_MIDDLEWARE_FLAGS_NONE, NULL, middleware_before, NULL
    };
    arbor_mvc_route route = {
        method, sizeof(method) - 1u,
        pattern, sizeof(pattern) - 1u,
        controller, NULL, presenter, &pc,
        middleware_indices, 1u
    };
    arbor_mvc_catalog catalog = {
        ARBOR_MVC_ABI_VERSION, (uint32_t)sizeof(arbor_mvc_catalog),
        ARBOR_MVC_CATALOG_FLAGS_NONE, &route, 1u, &middleware, 1u
    };
    arbor_route_param workspace_params[8] = {0};
    arbor_mvc_prepare_workspace workspace = {workspace_params, 8u};
    arbor_mvc_application mvc = {0};
    if (arbor_mvc_application_prepare(&catalog, &workspace, &mvc).native != 0) {
        return fail("MVC preparation for HTTP1 integration");
    }
    arbor_http_mvc_application app = {0};
    if (arbor_http_mvc_application_prepare(&mvc, 8u, &app).native != 0) {
        return fail("HTTP1 application preparation");
    }

    int sv[2] = {-1, -1};
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0) return fail("socketpair");

    uint8_t input[4096] = {0};
    uint8_t output[4096] = {0};
    uint8_t arena[4096] = {0};
    arbor_runtime_storage storage = {0};
    if (arbor_runtime_storage_prepare(
            &storage,
            (arbor_mut_span){input, sizeof(input)},
            (arbor_mut_span){output, sizeof(output)},
            (arbor_mut_span){arena, sizeof(arena)}).native != 0) {
        close(sv[0]); close(sv[1]);
        return fail("runtime storage preparation");
    }
    arbor_asm_result_u64 init = connection_init(
        &storage.connection, sv[1], &storage.input, &storage.output, &storage.arena);
    if (init.status != 0 ||
        connection_transition(&storage.connection, ARBOR_ASM_CONNECTION_READING).status != 0) {
        close(sv[0]); close(sv[1]);
        return fail("connection initialization");
    }

    static const uint8_t request[] =
        "GET /go HTTP/1.1\r\n"
        "Host: local\r\n"
        "Connection: close\r\n"
        "\r\n";
    if (write(sv[0], request, sizeof(request) - 1u) != (ssize_t)(sizeof(request) - 1u)) {
        close(sv[0]); close(sv[1]);
        return fail("write HTTP1 request");
    }

    uint64_t completed = 0u;
    arbor_status status = arbor_http_mvc_server_step(&storage, &app, -1, &completed);
    if (status.native != 0 || completed != 1u || pc.calls != 1u ||
        storage.connection.state != ARBOR_ASM_CONNECTION_CLOSED) {
        close(sv[0]);
        return fail("HTTP1 transport dispatch completion");
    }

    uint8_t response[4096] = {0};
    ssize_t used = read(sv[0], response, sizeof(response));
    if (used <= 0) {
        close(sv[0]);
        return fail("read HTTP1 response");
    }
    const size_t length = (size_t)used;
    static const uint8_t status_line[] = "HTTP/1.1 302 Found\r\n";
    static const uint8_t ct[] = "Content-Type: text/plain; charset=utf-8\r\n";
    static const uint8_t loc[] = "Location: /hello\r\n";
    static const uint8_t c1[] = "Set-Cookie: one=1; Path=/\r\n";
    static const uint8_t c2[] = "Set-Cookie: two=2; Path=/\r\n";
    static const uint8_t close_header[] = "Connection: close\r\n";
    static const uint8_t transient[] = "X-Transient:";
    if (!contains_bytes(response, length, status_line, sizeof(status_line) - 1u) ||
        !contains_bytes(response, length, ct, sizeof(ct) - 1u) ||
        !contains_bytes(response, length, loc, sizeof(loc) - 1u) ||
        !contains_bytes(response, length, c1, sizeof(c1) - 1u) ||
        !contains_bytes(response, length, c2, sizeof(c2) - 1u) ||
        !contains_bytes(response, length, close_header, sizeof(close_header) - 1u) ||
        contains_bytes(response, length, transient, sizeof(transient) - 1u)) {
        close(sv[0]);
        return fail("HTTP1 dynamic ordered response fields and redirect serialization");
    }
    const uint8_t *p1 = find_bytes(response, length, c1, sizeof(c1) - 1u);
    const uint8_t *p2 = find_bytes(response, length, c2, sizeof(c2) - 1u);
    if (p1 == NULL || p2 == NULL || p1 >= p2) {
        close(sv[0]);
        return fail("HTTP1 repeated Set-Cookie order");
    }

    close(sv[0]);
    puts("PASS: HTTP1 MVC0 reuse, request-local field sidecar, redirect and HTTP0 final serialization");
    return 0;
}
