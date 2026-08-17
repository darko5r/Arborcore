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

static int parse_request(const uint8_t *bytes, uint64_t length, arbor_request_view *out)
{
    uint64_t required = 0u;
    const arbor_status status = arbor_request_parse((arbor_span){bytes, length}, out, &required);
    return status.native == 0 && required == length ? 0 : 1;
}

static int init_buffer(uint8_t *storage, uint64_t capacity, arbor_asm_buffer *buffer)
{
    const arbor_asm_result_u64 result = buffer_init(buffer, storage, capacity);
    return result.status == 0 ? 0 : 1;
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

static int expect_bytes(const arbor_asm_buffer *buffer, const char *expected)
{
    const size_t length = strlen(expected);
    return buffer->length == (uint64_t)length &&
           memcmp(buffer->data, expected, length) == 0 ? 0 : 1;
}

int main(void)
{
    static const uint8_t get_bytes[] =
        "GET /hello HTTP/1.1\r\nHost: local\r\n\r\n";
    static const uint8_t head_bytes[] =
        "HEAD /hello HTTP/1.1\r\nHost: local\r\n\r\n";
    static const uint8_t close_bytes[] =
        "GET /hello HTTP/1.1\r\nHost: local\r\nConnection: close\r\n\r\n";

    arbor_request_view get = {0};
    arbor_request_view head = {0};
    arbor_request_view close_request = {0};
    if (parse_request(get_bytes, sizeof(get_bytes) - 1u, &get) != 0 ||
        parse_request(head_bytes, sizeof(head_bytes) - 1u, &head) != 0 ||
        parse_request(close_bytes, sizeof(close_bytes) - 1u, &close_request) != 0) {
        return fail("HTTP0 response request fixtures");
    }

    static const uint8_t body[] = "Hello";
    static const uint8_t content_type_name[] = "Content-Type";
    static const uint8_t content_type_value[] = "text/html; charset=utf-8";
    static const uint8_t location_name[] = "Location";
    static const uint8_t location_value[] = "/next";
    static const uint8_t set_cookie_name[] = "Set-Cookie";
    static const uint8_t set_cookie_one[] = "a=1; Path=/";
    static const uint8_t set_cookie_two[] = "b=2; Path=/";

    const arbor_http_field fields[] = {
        {content_type_name, sizeof(content_type_name) - 1u,
         content_type_value, sizeof(content_type_value) - 1u},
        {location_name, sizeof(location_name) - 1u,
         location_value, sizeof(location_value) - 1u},
        {set_cookie_name, sizeof(set_cookie_name) - 1u,
         set_cookie_one, sizeof(set_cookie_one) - 1u},
        {set_cookie_name, sizeof(set_cookie_name) - 1u,
         set_cookie_two, sizeof(set_cookie_two) - 1u}
    };

    arbor_http_response response = {0};
    arbor_status status = arbor_http_response_make(
        302u,
        fields,
        (uint64_t)(sizeof(fields) / sizeof(fields[0])),
        (arbor_span){body, sizeof(body) - 1u},
        ARBOR_HTTP_RESPONSE_FLAG_NONE,
        &response);
    if (status.native != 0 || response.status != 302u || response.field_count != 4u) {
        return fail("HTTP0 response construction");
    }

    uint8_t storage[1024] = {0};
    arbor_asm_buffer buffer = {0};
    if (init_buffer(storage, sizeof(storage), &buffer) != 0) {
        return fail("HTTP0 output buffer init");
    }

    uint64_t written = UINT64_MAX;
    bool close_after = true;
    status = arbor_http_response_serialize(
        &buffer, &get.native, &response, &written, &close_after);
    static const char expected_get[] =
        "HTTP/1.1 302 Found\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Location: /next\r\n"
        "Set-Cookie: a=1; Path=/\r\n"
        "Set-Cookie: b=2; Path=/\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "Hello";
    if (status.native != 0 || close_after || written != buffer.length ||
        expect_bytes(&buffer, expected_get) != 0) {
        return fail("HTTP0 ordered final response serialization");
    }

    if (buffer_reset(&buffer).status != 0) {
        return fail("HTTP0 buffer reset before HEAD");
    }
    written = UINT64_MAX;
    close_after = true;
    status = arbor_http_response_serialize(
        &buffer, &head.native, &response, &written, &close_after);
    static const char expected_head[] =
        "HTTP/1.1 302 Found\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Location: /next\r\n"
        "Set-Cookie: a=1; Path=/\r\n"
        "Set-Cookie: b=2; Path=/\r\n"
        "Content-Length: 5\r\n"
        "\r\n";
    if (status.native != 0 || close_after || written != buffer.length ||
        expect_bytes(&buffer, expected_head) != 0) {
        return fail("HTTP0 HEAD omits content and retains hypothetical Content-Length");
    }

    if (buffer_reset(&buffer).status != 0) {
        return fail("HTTP0 buffer reset before request close");
    }
    close_after = false;
    status = arbor_http_response_serialize(
        &buffer, &close_request.native, &response, &written, &close_after);
    if (status.native != 0 || !close_after ||
        !buffer_contains(&buffer, "Connection: close\r\n")) {
        return fail("HTTP0 request Connection close forces closing response");
    }

    if (buffer_reset(&buffer).status != 0) {
        return fail("HTTP0 buffer reset before application close");
    }
    arbor_http_response close_response = {0};
    status = arbor_http_response_make(
        200u, NULL, 0u, (arbor_span){body, sizeof(body) - 1u},
        ARBOR_HTTP_RESPONSE_FLAG_CLOSE, &close_response);
    if (status.native != 0) {
        return fail("HTTP0 application close response construction");
    }
    close_after = false;
    status = arbor_http_response_serialize(
        &buffer, &get.native, &close_response, &written, &close_after);
    if (status.native != 0 || !close_after ||
        !buffer_contains(&buffer, "Connection: close\r\n")) {
        return fail("HTTP0 application can tighten persistence");
    }

    if (buffer_reset(&buffer).status != 0) {
        return fail("HTTP0 buffer reset before 204");
    }
    arbor_http_response no_content = {0};
    if (arbor_http_response_make(204u, NULL, 0u, (arbor_span){NULL, 0u}, 0u, &no_content).native != 0 ||
        arbor_http_response_serialize(&buffer, &get.native, &no_content, &written, &close_after).native != 0 ||
        expect_bytes(&buffer, "HTTP/1.1 204 No Content\r\n\r\n") != 0) {
        return fail("HTTP0 204 has no content and no Content-Length");
    }

    if (buffer_reset(&buffer).status != 0) {
        return fail("HTTP0 buffer reset before 205");
    }
    if (arbor_http_response_make(205u, NULL, 0u, (arbor_span){NULL, 0u}, 0u, &no_content).native != 0 ||
        arbor_http_response_serialize(&buffer, &get.native, &no_content, &written, &close_after).native != 0 ||
        expect_bytes(&buffer, "HTTP/1.1 205 Reset Content\r\nContent-Length: 0\r\n\r\n") != 0) {
        return fail("HTTP0 205 has zero content");
    }

    if (buffer_reset(&buffer).status != 0) {
        return fail("HTTP0 buffer reset before 304");
    }
    if (arbor_http_response_make(304u, NULL, 0u, (arbor_span){NULL, 0u}, 0u, &no_content).native != 0 ||
        arbor_http_response_serialize(&buffer, &get.native, &no_content, &written, &close_after).native != 0 ||
        expect_bytes(&buffer, "HTTP/1.1 304 Not Modified\r\n\r\n") != 0) {
        return fail("HTTP0 304 has no content and no automatic Content-Length");
    }

    if (buffer_reset(&buffer).status != 0) {
        return fail("HTTP0 buffer reset before unknown status");
    }
    arbor_http_response unknown = {0};
    if (arbor_http_response_make(299u, NULL, 0u, (arbor_span){NULL, 0u}, 0u, &unknown).native != 0 ||
        arbor_http_response_serialize(&buffer, &get.native, &unknown, &written, &close_after).native != 0 ||
        expect_bytes(&buffer, "HTTP/1.1 299 \r\nContent-Length: 0\r\n\r\n") != 0) {
        return fail("HTTP0 unknown final status uses valid empty reason phrase");
    }

    puts("PASS: HTTP0 ordered response fields, final statuses, HEAD and connection policy");
    return 0;
}
