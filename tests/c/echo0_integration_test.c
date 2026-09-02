#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "echo0.h"

#define ECHO0_TEST_BUFFER_CAPACITY 16384u

typedef struct echo0_exchange_result {
    int64_t native;
    uint64_t completed;
    uint64_t output_length;
    uint64_t connection_state;
    echo0_metrics metrics;
    uint8_t response[ECHO0_TEST_BUFFER_CAPACITY];
    size_t response_length;
} echo0_exchange_result;

static const uint8_t echo0_test_template[] =
    "<!doctype html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
    "  <meta charset=\"utf-8\">\n"
    "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
    "  <title>Arborcore ECHO0</title>\n"
    "</head>\n"
    "<body>\n"
    "  <main>\n"
    "    <h1>Arborcore ECHO0</h1>\n"
    "    <p>Echo: {{value}}</p>\n"
    "  </main>\n"
    "</body>\n"
    "</html>\n";

static int fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    return 1;
}

static size_t find_bytes(
    const uint8_t *haystack,
    size_t haystack_length,
    const uint8_t *needle,
    size_t needle_length)
{
    if (needle_length == 0u) {
        return 0u;
    }
    if (needle_length > haystack_length) {
        return SIZE_MAX;
    }
    for (size_t i = 0u; i + needle_length <= haystack_length; ++i) {
        if (memcmp(haystack + i, needle, needle_length) == 0) {
            return i;
        }
    }
    return SIZE_MAX;
}

static bool contains_bytes(
    const uint8_t *haystack,
    size_t haystack_length,
    const uint8_t *needle,
    size_t needle_length)
{
    return find_bytes(haystack, haystack_length, needle, needle_length) !=
        SIZE_MAX;
}

static size_t count_bytes(
    const uint8_t *haystack,
    size_t haystack_length,
    const uint8_t *needle,
    size_t needle_length)
{
    size_t count = 0u;
    if (needle_length == 0u || needle_length > haystack_length) {
        return 0u;
    }
    for (size_t i = 0u; i + needle_length <= haystack_length; ++i) {
        if (memcmp(haystack + i, needle, needle_length) == 0) {
            count += 1u;
        }
    }
    return count;
}

static bool metrics_equal(const echo0_metrics *metrics, uint64_t expected)
{
    return metrics->middleware_calls == expected &&
        metrics->controller_calls == expected &&
        metrics->service_calls == expected &&
        metrics->presenter_calls == expected;
}

static bool response_body(
    const uint8_t *response,
    size_t response_length,
    const uint8_t **body_out,
    size_t *body_length_out)
{
    static const uint8_t delimiter[] = "\r\n\r\n";
    if (response == NULL || body_out == NULL || body_length_out == NULL) {
        return false;
    }
    size_t position = find_bytes(
        response,
        response_length,
        delimiter,
        sizeof(delimiter) - 1u);
    if (position == SIZE_MAX) {
        return false;
    }
    position += sizeof(delimiter) - 1u;
    *body_out = response + position;
    *body_length_out = response_length - position;
    return true;
}

static int write_all(int fd, const uint8_t *bytes, size_t length)
{
    size_t offset = 0u;
    while (offset < length) {
        ssize_t written = write(fd, bytes + offset, length - offset);
        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }
            return 1;
        }
        if (written == 0) {
            return 1;
        }
        offset += (size_t)written;
    }
    return 0;
}

static int read_to_eof(int fd, uint8_t *bytes, size_t capacity, size_t *length_out)
{
    size_t used = 0u;
    for (;;) {
        if (used == capacity) {
            return 1;
        }
        ssize_t received = read(fd, bytes + used, capacity - used);
        if (received < 0) {
            if (errno == EINTR) {
                continue;
            }
            return 1;
        }
        if (received == 0) {
            break;
        }
        used += (size_t)received;
    }
    *length_out = used;
    return 0;
}

static int run_exchange(
    const uint8_t *request,
    size_t request_length,
    uint64_t field_capacity,
    echo0_exchange_result *result_out)
{
    if (request == NULL || request_length == 0u || result_out == NULL) {
        return 1;
    }
    *result_out = (echo0_exchange_result){0};

    echo0_web_application application = {0};
    if (echo0_web_application_prepare(
            (arbor_span){
                echo0_test_template,
                (uint64_t)(sizeof(echo0_test_template) - 1u)},
            field_capacity,
            &application).native != 0) {
        return 1;
    }

    int sockets[2] = {-1, -1};
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) != 0) {
        return 1;
    }

    uint8_t input[ECHO0_TEST_BUFFER_CAPACITY] = {0};
    uint8_t output[ECHO0_TEST_BUFFER_CAPACITY] = {0};
    uint8_t arena[ECHO0_TEST_BUFFER_CAPACITY] = {0};
    arbor_runtime_storage storage = {0};
    if (arbor_runtime_storage_prepare(
            &storage,
            (arbor_mut_span){input, sizeof(input)},
            (arbor_mut_span){output, sizeof(output)},
            (arbor_mut_span){arena, sizeof(arena)}).native != 0) {
        (void)close(sockets[0]);
        (void)close(sockets[1]);
        return 1;
    }
    arbor_asm_result_u64 initialized = connection_init(
        &storage.connection,
        sockets[1],
        &storage.input,
        &storage.output,
        &storage.arena);
    if (initialized.status != 0 ||
        connection_transition(
            &storage.connection,
            ARBOR_ASM_CONNECTION_READING).status != 0 ||
        write_all(sockets[0], request, request_length) != 0) {
        (void)close(sockets[0]);
        (void)close(sockets[1]);
        return 1;
    }

    arbor_status status = arbor_http_mvc_server_step(
        &storage,
        &application.http_application,
        -1,
        &result_out->completed);
    result_out->native = status.native;
    result_out->output_length = storage.output.length;
    result_out->connection_state = storage.connection.state;
    result_out->metrics = application.metrics;

    if (storage.connection.state == ARBOR_ASM_CONNECTION_CLOSED) {
        if (read_to_eof(
                sockets[0],
                result_out->response,
                sizeof(result_out->response),
                &result_out->response_length) != 0) {
            (void)close(sockets[0]);
            return 1;
        }
    } else {
        (void)close(sockets[1]);
    }
    (void)close(sockets[0]);
    return 0;
}

static int page_case(void)
{
    static const uint8_t request[] =
        "GET /echo/A&B-Ol%C3%A1 HTTP/1.1\r\n"
        "Host: local\r\n"
        "Connection: close\r\n"
        "\r\n";
    echo0_exchange_result result = {0};
    if (run_exchange(
            request,
            sizeof(request) - 1u,
            ECHO0_RESPONSE_FIELD_CAPACITY,
            &result) != 0 ||
        result.native != 0 || result.completed != 1u ||
        result.connection_state != ARBOR_ASM_CONNECTION_CLOSED ||
        !metrics_equal(&result.metrics, 1u)) {
        fprintf(
            stderr,
            "ECHO0_PAGE_DEBUG native=%" PRId64 " completed=%" PRIu64
            " state=%" PRIu64 " middleware=%" PRIu64
            " controller=%" PRIu64 " service=%" PRIu64
            " presenter=%" PRIu64 " output=%" PRIu64 " response=%zu\n",
            result.native,
            result.completed,
            result.connection_state,
            result.metrics.middleware_calls,
            result.metrics.controller_calls,
            result.metrics.service_calls,
            result.metrics.presenter_calls,
            result.output_length,
            result.response_length);
        return fail("parameterized GET completes through the ECHO0 pipeline");
    }

    static const uint8_t status[] = "HTTP/1.1 200 OK\r\n";
    static const uint8_t cache[] = "Cache-Control: no-store\r\n";
    static const uint8_t content_type[] =
        "Content-Type: text/html; charset=utf-8\r\n";
    static const uint8_t close_field[] = "Connection: close\r\n";
    static const uint8_t expected_body[] =
        "<!doctype html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "  <meta charset=\"utf-8\">\n"
        "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
        "  <title>Arborcore ECHO0</title>\n"
        "</head>\n"
        "<body>\n"
        "  <main>\n"
        "    <h1>Arborcore ECHO0</h1>\n"
        "    <p>Echo: A&amp;B-Ol%C3%A1</p>\n"
        "  </main>\n"
        "</body>\n"
        "</html>\n";
    const uint8_t *body = NULL;
    size_t body_length = 0u;
    if (!contains_bytes(result.response, result.response_length, status, sizeof(status) - 1u) ||
        count_bytes(result.response, result.response_length, cache, sizeof(cache) - 1u) != 1u ||
        count_bytes(
            result.response,
            result.response_length,
            content_type,
            sizeof(content_type) - 1u) != 1u ||
        !contains_bytes(
            result.response,
            result.response_length,
            close_field,
            sizeof(close_field) - 1u) ||
        !response_body(
            result.response,
            result.response_length,
            &body,
            &body_length) ||
        body_length != sizeof(expected_body) - 1u ||
        memcmp(body, expected_body, sizeof(expected_body) - 1u) != 0) {
        return fail("page wire has exact fields and escaped UTF-8 body");
    }
    return 0;
}

static int redirect_case(void)
{
    static const uint8_t request[] =
        "GET / HTTP/1.1\r\n"
        "Host: local\r\n"
        "Connection: close\r\n"
        "\r\n";
    echo0_exchange_result result = {0};
    if (run_exchange(
            request,
            sizeof(request) - 1u,
            ECHO0_RESPONSE_FIELD_CAPACITY,
            &result) != 0 ||
        result.native != 0 || result.completed != 1u ||
        !metrics_equal(&result.metrics, 1u)) {
        return fail("GET / typed redirect completes");
    }
    static const uint8_t status[] = "HTTP/1.1 302 Found\r\n";
    static const uint8_t location[] = "Location: /echo/Arborcore\r\n";
    static const uint8_t cache[] = "Cache-Control: no-store\r\n";
    static const uint8_t empty[] = "Content-Length: 0\r\n";
    const uint8_t *body = NULL;
    size_t body_length = 1u;
    if (!contains_bytes(result.response, result.response_length, status, sizeof(status) - 1u) ||
        count_bytes(result.response, result.response_length, location, sizeof(location) - 1u) != 1u ||
        count_bytes(result.response, result.response_length, cache, sizeof(cache) - 1u) != 1u ||
        !contains_bytes(result.response, result.response_length, empty, sizeof(empty) - 1u) ||
        !response_body(result.response, result.response_length, &body, &body_length) ||
        body_length != 0u) {
        return fail("redirect maps typed outcome to exact empty 302 representation");
    }
    return 0;
}

static int not_found_case(void)
{
    static const uint8_t request[] =
        "GET /missing HTTP/1.1\r\n"
        "Host: local\r\n"
        "Connection: close\r\n"
        "\r\n";
    echo0_exchange_result result = {0};
    if (run_exchange(
            request,
            sizeof(request) - 1u,
            ECHO0_RESPONSE_FIELD_CAPACITY,
            &result) != 0 ||
        result.native != 0 || result.completed != 1u ||
        !metrics_equal(&result.metrics, 0u)) {
        return fail("unmatched route bypasses ECHO0 callbacks");
    }
    static const uint8_t status[] = "HTTP/1.1 404 Not Found\r\n";
    static const uint8_t cache[] = "Cache-Control:";
    const uint8_t *body = NULL;
    size_t body_length = 1u;
    if (!contains_bytes(result.response, result.response_length, status, sizeof(status) - 1u) ||
        contains_bytes(result.response, result.response_length, cache, sizeof(cache) - 1u) ||
        !response_body(result.response, result.response_length, &body, &body_length) ||
        body_length != 0u) {
        return fail("MVC0 supplies protocol-correct empty 404");
    }
    return 0;
}

static int pipeline_case(void)
{
    static const uint8_t request[] =
        "GET /echo/First&Ol%C3%A1 HTTP/1.1\r\n"
        "Host: local\r\n"
        "\r\n"
        "GET / HTTP/1.1\r\n"
        "Host: local\r\n"
        "Connection: close\r\n"
        "\r\n";
    echo0_exchange_result result = {0};
    if (run_exchange(
            request,
            sizeof(request) - 1u,
            ECHO0_RESPONSE_FIELD_CAPACITY,
            &result) != 0 ||
        result.native != 0 || result.completed != 2u ||
        !metrics_equal(&result.metrics, 2u)) {
        return fail("keep-alive pipeline completes page then redirect");
    }
    static const uint8_t http_prefix[] = "HTTP/1.1 ";
    static const uint8_t ok[] = "HTTP/1.1 200 OK\r\n";
    static const uint8_t found[] = "HTTP/1.1 302 Found\r\n";
    size_t ok_position = find_bytes(
        result.response,
        result.response_length,
        ok,
        sizeof(ok) - 1u);
    size_t found_position = find_bytes(
        result.response,
        result.response_length,
        found,
        sizeof(found) - 1u);
    if (count_bytes(
            result.response,
            result.response_length,
            http_prefix,
            sizeof(http_prefix) - 1u) != 2u ||
        ok_position == SIZE_MAX || found_position == SIZE_MAX ||
        ok_position >= found_position) {
        return fail("pipeline preserves response order on one connection");
    }
    return 0;
}

static int capacity_failure_case(void)
{
    static const uint8_t request[] =
        "GET /echo/value HTTP/1.1\r\n"
        "Host: local\r\n"
        "Connection: close\r\n"
        "\r\n";
    echo0_exchange_result result = {0};
    if (run_exchange(request, sizeof(request) - 1u, 1u, &result) != 0 ||
        result.native != -ENOSPC || result.completed != 0u ||
        result.output_length != 0u || !metrics_equal(&result.metrics, 1u)) {
        return fail("field-capacity exhaustion publishes no partial response");
    }
    return 0;
}

static int malformed_host_case(void)
{
    static const uint8_t request[] =
        "GET /echo/value HTTP/1.1\r\n"
        "Connection: close\r\n"
        "\r\n";
    echo0_exchange_result result = {0};
    if (run_exchange(
            request,
            sizeof(request) - 1u,
            ECHO0_RESPONSE_FIELD_CAPACITY,
            &result) != 0 ||
        result.native != 0 || result.completed != 1u ||
        !metrics_equal(&result.metrics, 0u)) {
        return fail("invalid HTTP/1.1 Host is handled before MVC");
    }
    static const uint8_t status[] = "HTTP/1.1 400 Bad Request\r\n";
    if (!contains_bytes(result.response, result.response_length, status, sizeof(status) - 1u)) {
        return fail("invalid Host publishes protocol-owned 400");
    }
    return 0;
}

static int direct_callback_adversarial_case(void)
{
    echo0_web_application application = {0};
    if (echo0_web_application_prepare(
            (arbor_span){
                echo0_test_template,
                (uint64_t)(sizeof(echo0_test_template) - 1u)},
            ECHO0_RESPONSE_FIELD_CAPACITY,
            &application).native != 0) {
        return fail("prepare direct-callback ECHO0 application");
    }

    static const uint8_t name[] = "value";
    static const uint8_t routed[] = {
        'D', 'i', 'r', 'e', 'c', 't', '&', 'O', 'l',
        UINT8_C(0xc3), UINT8_C(0xa1)
    };
    arbor_route_param parameter = {
        name,
        sizeof(name) - 1u,
        routed,
        sizeof(routed)
    };
    arbor_asm_http_request native_request = {0};
    arbor_asm_request_target target = {NULL, 0u, NULL, 0u};
    uint8_t arena_bytes[256] = {0};
    arbor_asm_arena arena = {0};
    if (arena_init(&arena, arena_bytes, sizeof(arena_bytes)).status != 0) {
        return fail("initialize direct-callback request arena");
    }
    arbor_request_scope scope = {
        &native_request,
        &target,
        &parameter,
        1u,
        &arena
    };
    arbor_mvc_request request = {
        &scope,
        &application.routes[0],
        &parameter,
        1u,
        0u
    };
    arbor_mvc_controller_result controller_result = {0};
    if (application.routes[0].controller(
            &request,
            application.routes[0].controller_context,
            &controller_result) != 0 ||
        controller_result.outcome_code != (uint32_t)ECHO0_OUTCOME_PAGE ||
        controller_result.model_data == NULL ||
        controller_result.model_size != (uint64_t)sizeof(echo0_page_model)) {
        return fail("route metadata reaches page controller and typed model");
    }
    const echo0_page_model *model =
        (const echo0_page_model *)controller_result.model_data;
    uintptr_t model_address = (uintptr_t)model;
    uintptr_t arena_begin = (uintptr_t)arena_bytes;
    uintptr_t arena_end = arena_begin + sizeof(arena_bytes);
    if (parameter.name_ptr != name || parameter.name_len != sizeof(name) - 1u ||
        parameter.value_ptr != routed || parameter.value_len != sizeof(routed) ||
        model_address < arena_begin || model_address >= arena_end ||
        model->value.data != routed || model->value.length != sizeof(routed)) {
        return fail("model is arena-owned and borrows exact request-input value bytes");
    }

    arbor_asm_arena empty_arena = {NULL, 0u, 0u};
    scope.arena = &empty_arena;
    const arbor_mvc_controller_result controller_sentinel = {
        UINT32_MAX,
        UINT32_MAX,
        routed,
        sizeof(routed)
    };
    arbor_mvc_controller_result unchanged_controller = controller_sentinel;
    if (application.routes[0].controller(
            &request,
            application.routes[0].controller_context,
            &unchanged_controller) != -ENOSPC ||
        memcmp(
            &unchanged_controller,
            &controller_sentinel,
            sizeof(unchanged_controller)) != 0) {
        return fail("arena exhaustion leaves controller publication untouched");
    }

    scope.arena = &arena;
    const arbor_response_plan response_sentinel = {
        UINT64_MAX,
        routed,
        sizeof(routed),
        UINT64_MAX
    };
    arbor_response_plan unchanged_response = response_sentinel;
    arbor_mvc_controller_result invalid_model = {
        (uint32_t)ECHO0_OUTCOME_PAGE,
        ARBOR_MVC_CONTROLLER_RESULT_FLAGS_NONE,
        NULL,
        0u
    };
    if (application.routes[0].presenter(
            &request,
            application.routes[0].presenter_context,
            &invalid_model,
            &unchanged_response) != -EINVAL ||
        memcmp(
            &unchanged_response,
            &response_sentinel,
            sizeof(unchanged_response)) != 0) {
        return fail("invalid page model publishes no response plan");
    }

    unchanged_response = response_sentinel;
    arbor_mvc_controller_result invalid_outcome = {
        UINT32_MAX,
        ARBOR_MVC_CONTROLLER_RESULT_FLAGS_NONE,
        NULL,
        0u
    };
    if (application.routes[0].presenter(
            &request,
            application.routes[0].presenter_context,
            &invalid_outcome,
            &unchanged_response) != -EINVAL ||
        memcmp(
            &unchanged_response,
            &response_sentinel,
            sizeof(unchanged_response)) != 0) {
        return fail("invalid typed outcome publishes no response plan");
    }
    return 0;
}

int main(void)
{
    if (page_case() != 0 || redirect_case() != 0 || not_found_case() != 0 ||
        pipeline_case() != 0 || capacity_failure_case() != 0 ||
        malformed_host_case() != 0 || direct_callback_adversarial_case() != 0) {
        return 1;
    }
    puts("PASS: ECHO0 route-value MVC/VIEW/HTTP integration and adversarial publication");
    return 0;
}
