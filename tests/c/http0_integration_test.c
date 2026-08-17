#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <arborcore/http.h>

static int fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    return 1;
}

static bool buffer_contains(const arbor_asm_buffer *buffer, const char *needle)
{
    const size_t needle_length = strlen(needle);
    if (needle_length == 0u) {
        return true;
    }
    if (buffer->length < (uint64_t)needle_length) {
        return false;
    }
    const uint64_t limit = buffer->length - (uint64_t)needle_length;
    for (uint64_t i = 0u; i <= limit; ++i) {
        if (memcmp(buffer->data + i, needle, needle_length) == 0) {
            return true;
        }
    }
    return false;
}

int main(void)
{
    static const uint8_t request_bytes[] =
        "GET /hello HTTP/1.1\r\n"
        "Host: example.test\r\n"
        "Accept: text/html\r\n"
        "\r\n";

    arbor_request_view request = {0};
    uint64_t required = 0u;
    arbor_status status = arbor_request_parse(
        (arbor_span){request_bytes, sizeof(request_bytes) - 1u},
        &request,
        &required);
    if (status.native != 0 || required != sizeof(request_bytes) - 1u) {
        return fail("HTTP0 integration request parse");
    }

    arbor_span host = {0};
    if (arbor_http_request_host_validate(&request.native, &host).native != 0 ||
        host.length != 12u || memcmp(host.data, "example.test", 12u) != 0) {
        return fail("HTTP0 integration Host validation");
    }

    static const uint8_t content_type_name[] = "Content-Type";
    static const uint8_t content_type_value[] = "text/html; charset=utf-8";
    static const uint8_t cache_name[] = "Cache-Control";
    static const uint8_t cache_value[] = "no-store";
    static const uint8_t body[] =
        "<!doctype html><html><body><h1>Hello HTTP0</h1></body></html>";
    const arbor_http_field fields[] = {
        {content_type_name, sizeof(content_type_name) - 1u,
         content_type_value, sizeof(content_type_value) - 1u},
        {cache_name, sizeof(cache_name) - 1u,
         cache_value, sizeof(cache_value) - 1u}
    };

    arbor_http_response response = {0};
    status = arbor_http_response_make(
        200u,
        fields,
        (uint64_t)(sizeof(fields) / sizeof(fields[0])),
        (arbor_span){body, sizeof(body) - 1u},
        ARBOR_HTTP_RESPONSE_FLAG_NONE,
        &response);
    if (status.native != 0) {
        return fail("HTTP0 integration response make");
    }

    uint8_t output_bytes[1024] = {0};
    arbor_asm_buffer output = {0};
    if (buffer_init(&output, output_bytes, sizeof(output_bytes)).status != 0) {
        return fail("HTTP0 integration output init");
    }

    uint64_t written = 0u;
    bool close_after = true;
    status = arbor_http_response_serialize(
        &output, &request.native, &response, &written, &close_after);
    if (status.native != 0 || close_after || written != output.length ||
        !buffer_contains(&output, "HTTP/1.1 200 OK\r\n") ||
        !buffer_contains(&output, "Content-Type: text/html; charset=utf-8\r\n") ||
        !buffer_contains(&output, "Cache-Control: no-store\r\n") ||
        !buffer_contains(&output, "Content-Length: 61\r\n") ||
        !buffer_contains(&output, "<h1>Hello HTTP0</h1>")) {
        return fail("HTTP0 integration HTML metadata/body serialization");
    }

    if (buffer_reset(&output).status != 0) {
        return fail("HTTP0 integration reset");
    }
    arbor_http_response missing = {0};
    if (arbor_http_response_make(
            404u, fields, 1u,
            (arbor_span){NULL, 0u}, ARBOR_HTTP_RESPONSE_FLAG_NONE,
            &missing).native != 0 ||
        arbor_http_response_serialize(
            &output, &request.native, &missing, &written, &close_after).native != 0 ||
        !buffer_contains(&output, "HTTP/1.1 404 Not Found\r\n") ||
        !buffer_contains(&output, "Content-Length: 0\r\n")) {
        return fail("HTTP0 integration 404 metadata response");
    }

    puts("PASS: HTTP0 parser-to-header-to-final-response integration");
    return 0;
}
