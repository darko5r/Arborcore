#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "hello0.h"

#define HELLO0_TEST_BUFFER_CAPACITY 16384u

typedef struct hello0_exchange_result {
    int64_t native;
    uint64_t completed;
    uint64_t output_length;
    uint64_t connection_state;
    hello0_metrics metrics;
    uint8_t response[HELLO0_TEST_BUFFER_CAPACITY];
    size_t response_length;
} hello0_exchange_result;

static int fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    return 1;
}

static bool contains_bytes(
    const uint8_t *haystack,
    size_t haystack_length,
    const uint8_t *needle,
    size_t needle_length)
{
    if (needle_length == 0u) {
        return true;
    }
    if (needle_length > haystack_length) {
        return false;
    }
    for (size_t i = 0u; i + needle_length <= haystack_length; ++i) {
        if (memcmp(haystack + i, needle, needle_length) == 0) {
            return true;
        }
    }
    return false;
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

static bool metrics_equal(const hello0_metrics *metrics, uint64_t expected)
{
    return metrics->middleware_calls == expected &&
        metrics->controller_calls == expected &&
        metrics->service_calls == expected &&
        metrics->presenter_calls == expected;
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
    hello0_exchange_result *result_out)
{
    static const uint8_t source[] =
        "<!doctype html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "  <meta charset=\"utf-8\">\n"
        "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
        "  <title>Arborcore HELLO0</title>\n"
        "</head>\n"
        "<body>\n"
        "  <main>\n"
        "    <h1>Hello World</h1>\n"
        "    <p>{{message}}</p>\n"
        "  </main>\n"
        "</body>\n"
        "</html>\n";
    if (request == NULL || request_length == 0u || result_out == NULL) {
        return 1;
    }
    *result_out = (hello0_exchange_result){0};

    hello0_web_application application = {0};
    if (hello0_web_application_prepare(
            (arbor_span){source, (uint64_t)(sizeof(source) - 1u)},
            field_capacity,
            &application).native != 0) {
        return 1;
    }

    int sockets[2] = {-1, -1};
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) != 0) {
        return 1;
    }

    uint8_t input[HELLO0_TEST_BUFFER_CAPACITY] = {0};
    uint8_t output[HELLO0_TEST_BUFFER_CAPACITY] = {0};
    uint8_t arena[HELLO0_TEST_BUFFER_CAPACITY] = {0};
    arbor_runtime_storage storage = {0};
    if (arbor_runtime_storage_prepare(
            &storage,
            (arbor_mut_span){input, sizeof(input)},
            (arbor_mut_span){output, sizeof(output)},
            (arbor_mut_span){arena, sizeof(arena)}).native != 0) {
        close(sockets[0]);
        close(sockets[1]);
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
        close(sockets[0]);
        close(sockets[1]);
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
            close(sockets[0]);
            return 1;
        }
    } else {
        close(sockets[1]);
    }
    close(sockets[0]);
    return 0;
}

static int page_case(void)
{
    static const uint8_t request[] =
        "GET /hello?source=integration HTTP/1.1\r\n"
        "Host: local\r\n"
        "Connection: close\r\n"
        "\r\n";
    hello0_exchange_result result = {0};
    if (run_exchange(
            request,
            sizeof(request) - 1u,
            HELLO0_RESPONSE_FIELD_CAPACITY,
            &result) != 0 ||
        result.native != 0 || result.completed != 1u ||
        result.connection_state != ARBOR_ASM_CONNECTION_CLOSED ||
        !metrics_equal(&result.metrics, 1u)) {
        return fail("GET /hello completes through middleware, service and presenter");
    }

    static const uint8_t status[] = "HTTP/1.1 200 OK\r\n";
    static const uint8_t cache[] = "Cache-Control: no-store\r\n";
    static const uint8_t content_type[] =
        "Content-Type: text/html; charset=utf-8\r\n";
    static const uint8_t close_field[] = "Connection: close\r\n";
    static const uint8_t heading[] = "<h1>Hello World</h1>";
    static const uint8_t escaped[] =
        "Arborcore safely renders &lt;dynamic data&gt; &amp; UTF-8: "
        "Ol\xc3\xa1 \xf0\x9f\x98\x80";
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
        !contains_bytes(result.response, result.response_length, heading, sizeof(heading) - 1u) ||
        !contains_bytes(result.response, result.response_length, escaped, sizeof(escaped) - 1u)) {
        return fail("GET /hello wire status, fields, escaped HTML and close authority");
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
    hello0_exchange_result result = {0};
    if (run_exchange(
            request,
            sizeof(request) - 1u,
            HELLO0_RESPONSE_FIELD_CAPACITY,
            &result) != 0 ||
        result.native != 0 || result.completed != 1u ||
        !metrics_equal(&result.metrics, 1u)) {
        return fail("GET / typed redirect completes");
    }
    static const uint8_t status[] = "HTTP/1.1 302 Found\r\n";
    static const uint8_t location[] = "Location: /hello\r\n";
    static const uint8_t cache[] = "Cache-Control: no-store\r\n";
    static const uint8_t empty[] = "Content-Length: 0\r\n";
    if (!contains_bytes(result.response, result.response_length, status, sizeof(status) - 1u) ||
        count_bytes(result.response, result.response_length, location, sizeof(location) - 1u) != 1u ||
        count_bytes(result.response, result.response_length, cache, sizeof(cache) - 1u) != 1u ||
        !contains_bytes(result.response, result.response_length, empty, sizeof(empty) - 1u)) {
        return fail("redirect presenter maps outcome to 302, Location and empty body");
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
    hello0_exchange_result result = {0};
    if (run_exchange(
            request,
            sizeof(request) - 1u,
            HELLO0_RESPONSE_FIELD_CAPACITY,
            &result) != 0 ||
        result.native != 0 || result.completed != 1u ||
        !metrics_equal(&result.metrics, 0u)) {
        return fail("unmatched route bypasses HELLO0 callbacks");
    }
    static const uint8_t status[] = "HTTP/1.1 404 Not Found\r\n";
    static const uint8_t cache[] = "Cache-Control:";
    if (!contains_bytes(result.response, result.response_length, status, sizeof(status) - 1u) ||
        contains_bytes(result.response, result.response_length, cache, sizeof(cache) - 1u)) {
        return fail("MVC0 supplies unmatched-route 404");
    }
    return 0;
}

static int pipeline_case(void)
{
    static const uint8_t request[] =
        "GET /hello HTTP/1.1\r\n"
        "Host: local\r\n"
        "\r\n"
        "GET / HTTP/1.1\r\n"
        "Host: local\r\n"
        "Connection: close\r\n"
        "\r\n";
    hello0_exchange_result result = {0};
    if (run_exchange(
            request,
            sizeof(request) - 1u,
            HELLO0_RESPONSE_FIELD_CAPACITY,
            &result) != 0 ||
        result.native != 0 || result.completed != 2u ||
        !metrics_equal(&result.metrics, 2u)) {
        return fail("keep-alive pipeline completes two requests");
    }
    static const uint8_t http_prefix[] = "HTTP/1.1 ";
    static const uint8_t ok[] = "HTTP/1.1 200 OK\r\n";
    static const uint8_t found[] = "HTTP/1.1 302 Found\r\n";
    if (count_bytes(
            result.response,
            result.response_length,
            http_prefix,
            sizeof(http_prefix) - 1u) != 2u ||
        !contains_bytes(result.response, result.response_length, ok, sizeof(ok) - 1u) ||
        !contains_bytes(result.response, result.response_length, found, sizeof(found) - 1u)) {
        return fail("pipeline publishes page then redirect on one connection");
    }
    return 0;
}

static int capacity_failure_case(void)
{
    static const uint8_t request[] =
        "GET /hello HTTP/1.1\r\n"
        "Host: local\r\n"
        "Connection: close\r\n"
        "\r\n";
    hello0_exchange_result result = {0};
    if (run_exchange(request, sizeof(request) - 1u, 1u, &result) != 0 ||
        result.native != -ENOSPC || result.completed != 0u ||
        result.output_length != 0u || !metrics_equal(&result.metrics, 1u)) {
        return fail("field-capacity exhaustion blocks partial response publication");
    }
    return 0;
}

static int malformed_host_case(void)
{
    static const uint8_t request[] =
        "GET /hello HTTP/1.1\r\n"
        "Connection: close\r\n"
        "\r\n";
    hello0_exchange_result result = {0};
    if (run_exchange(
            request,
            sizeof(request) - 1u,
            HELLO0_RESPONSE_FIELD_CAPACITY,
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

int main(void)
{
    if (page_case() != 0 || redirect_case() != 0 || not_found_case() != 0 ||
        pipeline_case() != 0 || capacity_failure_case() != 0 ||
        malformed_host_case() != 0) {
        return 1;
    }
    puts("PASS: HELLO0 MVC/VIEW/HTTP integration, pipeline and adversarial cases");
    return 0;
}
