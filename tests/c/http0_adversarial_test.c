#include <errno.h>
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
    return buffer_init(buffer, storage, capacity).status == 0 ? 0 : 1;
}

int main(void)
{
    static const uint8_t valid_name[] = "X-Test";
    static const uint8_t valid_value[] = "value";
    arbor_http_field valid_field = {
        valid_name, sizeof(valid_name) - 1u,
        valid_value, sizeof(valid_value) - 1u
    };
    if (arbor_http_field_validate(&valid_field).native != 0) {
        return fail("HTTP0 valid field baseline");
    }

    static const uint8_t bad_name[] = "Bad Name";
    arbor_http_field bad_field = valid_field;
    bad_field.name_data = bad_name;
    bad_field.name_length = sizeof(bad_name) - 1u;
    if (arbor_http_field_validate(&bad_field).native != -EINVAL) {
        return fail("HTTP0 invalid field-name token rejection");
    }

    static const uint8_t injected_value[] = "ok\r\nInjected: yes";
    bad_field = valid_field;
    bad_field.value_data = injected_value;
    bad_field.value_length = sizeof(injected_value) - 1u;
    if (arbor_http_field_validate(&bad_field).native != -EINVAL) {
        return fail("HTTP0 response splitting value rejection");
    }

    bad_field = valid_field;
    bad_field.name_data = (const uint8_t *)(uintptr_t)(UINTPTR_MAX - (uintptr_t)1u);
    bad_field.name_length = UINT64_C(8);
    if (arbor_http_field_validate(&bad_field).native != -EINVAL) {
        return fail("HTTP0 field pointer range overflow rejection");
    }

    static const char *const reserved_names[] = {
        "Content-Length", "Connection", "Transfer-Encoding", "Trailer", "Upgrade"
    };
    for (size_t i = 0u; i < sizeof(reserved_names) / sizeof(reserved_names[0]); ++i) {
        const arbor_http_field reserved = {
            (const uint8_t *)reserved_names[i], (uint64_t)strlen(reserved_names[i]),
            valid_value, sizeof(valid_value) - 1u
        };
        arbor_http_response out = {0};
        if (arbor_http_response_make(
                200u, &reserved, 1u, (arbor_span){NULL, 0u}, 0u, &out).native != -EINVAL) {
            return fail("HTTP0 framework-owned response field rejection");
        }
    }

    static const uint8_t set_cookie_name[] = "Set-Cookie";
    static const uint8_t cookie_a[] = "a=1";
    static const uint8_t cookie_b[] = "b=2";
    const arbor_http_field cookies[] = {
        {set_cookie_name, sizeof(set_cookie_name) - 1u, cookie_a, sizeof(cookie_a) - 1u},
        {set_cookie_name, sizeof(set_cookie_name) - 1u, cookie_b, sizeof(cookie_b) - 1u}
    };
    arbor_http_response response = {0};
    if (arbor_http_response_make(200u, cookies, 2u, (arbor_span){NULL, 0u}, 0u, &response).native != 0) {
        return fail("HTTP0 repeated Set-Cookie field lines remain representable");
    }

    if (arbor_http_response_make(199u, NULL, 0u, (arbor_span){NULL, 0u}, 0u, &response).native != -EINVAL ||
        arbor_http_response_make(600u, NULL, 0u, (arbor_span){NULL, 0u}, 0u, &response).native != -EINVAL) {
        return fail("HTTP0 final status range rejection");
    }

    arbor_http_response size_probe = {
        ARBOR_HTTP_RESPONSE_ABI_VERSION,
        (uint32_t)(sizeof(arbor_http_response) - 1u),
        0u, 200u, NULL, 0u, NULL, 0u
    };
    if (arbor_http_response_validate(&size_probe).native != -EINVAL) {
        return fail("HTTP0 undersized response prefix rejection");
    }
    size_probe.struct_size = (uint32_t)(sizeof(arbor_http_response) + 8u);
    if (arbor_http_response_validate(&size_probe).native != 0) {
        return fail("HTTP0 response prefix-compatible extension acceptance");
    }

    static const uint8_t body[] = "x";
    for (uint64_t status_code = 204u; status_code <= 304u; status_code += status_code == 205u ? 99u : 1u) {
        if (arbor_http_response_make(
                status_code, NULL, 0u,
                (arbor_span){body, sizeof(body) - 1u},
                0u, &response).native != -EINVAL) {
            return fail("HTTP0 no-content status body rejection");
        }
    }

    if (arbor_http_response_make(
            200u, NULL, 0u, (arbor_span){body, sizeof(body) - 1u},
            UINT64_C(2), &response).native != -EINVAL) {
        return fail("HTTP0 unknown response flag rejection");
    }

    static const uint8_t request_bytes[] =
        "GET / HTTP/1.1\r\nHost: local\r\n\r\n";
    arbor_request_view request = {0};
    if (parse_request(request_bytes, sizeof(request_bytes) - 1u, &request) != 0) {
        return fail("HTTP0 adversarial request fixture");
    }

    typedef struct extended_http_response {
        arbor_http_response base;
        uint64_t extension_sentinel;
    } extended_http_response;
    extended_http_response extended = {
        {
            ARBOR_HTTP_RESPONSE_ABI_VERSION,
            (uint32_t)sizeof(extended_http_response),
            ARBOR_HTTP_RESPONSE_FLAG_NONE,
            200u,
            NULL,
            0u,
            NULL,
            0u
        },
        UINT64_C(0x1122334455667788)
    };
    if (arbor_http_response_validate(&extended.base).native != 0) {
        return fail("HTTP0 extended response prefix validation fixture");
    }
    arbor_asm_buffer extension_alias_buffer = {0};
    if (init_buffer(
            (uint8_t *)(void *)&extended.extension_sentinel,
            (uint64_t)sizeof(extended.extension_sentinel),
            &extension_alias_buffer) != 0) {
        return fail("HTTP0 extended response alias buffer fixture");
    }
    uint64_t extension_written = UINT64_C(77);
    bool extension_close = true;
    if (arbor_http_response_serialize(
            &extension_alias_buffer, &request.native, &extended.base,
            &extension_written, &extension_close).native != -EINVAL ||
        extension_alias_buffer.length != 0u ||
        extended.extension_sentinel != UINT64_C(0x1122334455667788) ||
        extension_written != UINT64_C(77) || !extension_close) {
        return fail("HTTP0 prefix-extension tail/output alias rejection");
    }

    uint64_t alias_cursor = 0u;
    bool alias_has_field = true;
    if (arbor_http_request_header_next(
            &request.native,
            &alias_cursor,
            (arbor_http_field *)(uintptr_t)request.native.headers_ptr,
            &alias_has_field).native != -EINVAL ||
        alias_cursor != 0u || !alias_has_field) {
        return fail("HTTP0 header output alias with borrowed request storage rejection");
    }

    arbor_span alias_host = {(const uint8_t *)(uintptr_t)UINT64_C(1), UINT64_C(9)};
    if (arbor_http_request_host_validate(
            &request.native,
            (arbor_span *)(uintptr_t)request.native.headers_ptr).native != -EINVAL ||
        alias_host.data == NULL) {
        return fail("HTTP0 Host output alias with borrowed request storage rejection");
    }

    arbor_asm_http_request bad_version = request.native;
    static const uint8_t http10[] = "HTTP/1.0";
    bad_version.version_ptr = http10;
    bad_version.version_len = sizeof(http10) - 1u;
    uint64_t bad_version_count = UINT64_C(99);
    static const uint8_t host_name[] = "Host";
    if (arbor_http_request_header_count(
            &bad_version, (arbor_span){host_name, sizeof(host_name) - 1u},
            &bad_version_count).native != -EINVAL ||
        bad_version_count != UINT64_C(99)) {
        return fail("HTTP0 manual non-HTTP/1.1 request rejection is transactional");
    }

    uint64_t cursor = request.native.headers_len + 1u;
    arbor_http_field sentinel = valid_field;
    const arbor_http_field sentinel_before = sentinel;
    bool has_field = true;
    bool has_before = has_field;
    if (arbor_http_request_header_next(
            &request.native, &cursor, &sentinel, &has_field).native != -EINVAL ||
        memcmp(&sentinel, &sentinel_before, sizeof(sentinel)) != 0 ||
        has_field != has_before || cursor != request.native.headers_len + 1u) {
        return fail("HTTP0 header iteration failure is transactional");
    }

    static const uint8_t malformed_headers[] = "Bad Name: x\r\n";
    arbor_asm_http_request malformed = request.native;
    malformed.headers_ptr = malformed_headers;
    malformed.headers_len = sizeof(malformed_headers) - 1u;
    cursor = 0u;
    sentinel = valid_field;
    has_field = true;
    if (arbor_http_request_header_next(
            &malformed, &cursor, &sentinel, &has_field).native != -EINVAL ||
        cursor != 0u || memcmp(&sentinel, &sentinel_before, sizeof(sentinel)) != 0 || !has_field) {
        return fail("HTTP0 malformed manual header span rejection");
    }

    static const uint8_t bad_host_bytes[] =
        "GET / HTTP/1.1\r\nHost: [bad host]\r\n\r\n";
    arbor_request_view bad_host = {0};
    arbor_span host = {(const uint8_t *)(uintptr_t)UINT64_C(1), UINT64_C(9)};
    const arbor_span host_before = host;
    if (parse_request(bad_host_bytes, sizeof(bad_host_bytes) - 1u, &bad_host) != 0 ||
        arbor_http_request_host_validate(&bad_host.native, &host).native != -EINVAL ||
        host.data != host_before.data || host.length != host_before.length) {
        return fail("HTTP0 invalid Host syntax transactional rejection");
    }

    static const uint8_t malformed_connection_bytes[] =
        "GET / HTTP/1.1\r\nHost: local\r\nConnection: close;bad\r\n\r\n";
    arbor_request_view malformed_connection = {0};
    bool close_requested = true;
    if (parse_request(
            malformed_connection_bytes,
            sizeof(malformed_connection_bytes) - 1u,
            &malformed_connection) != 0 ||
        arbor_http_request_connection_close(
            &malformed_connection.native, &close_requested).native != -EINVAL ||
        !close_requested) {
        return fail("HTTP0 malformed Connection token list transactional rejection");
    }

    arbor_http_response normal = {0};
    if (arbor_http_response_make(
            200u, NULL, 0u, (arbor_span){body, sizeof(body) - 1u}, 0u, &normal).native != 0) {
        return fail("HTTP0 normal response fixture");
    }

    arbor_http_response constructor_alias = {
        ARBOR_HTTP_RESPONSE_ABI_VERSION,
        (uint32_t)sizeof(arbor_http_response),
        0u, 500u, NULL, 0u, NULL, 0u
    };
    const arbor_http_response constructor_alias_before = constructor_alias;
    if (arbor_http_response_make(
            200u, NULL, 0u,
            (arbor_span){(const uint8_t *)&constructor_alias, 1u},
            0u, &constructor_alias).native != -EINVAL ||
        memcmp(&constructor_alias, &constructor_alias_before, sizeof(constructor_alias)) != 0) {
        return fail("HTTP0 response constructor source/output alias rejection");
    }

    uint8_t error_output[256] = {0};
    arbor_asm_buffer error_buffer = {0};
    if (init_buffer(error_output, sizeof(error_output), &error_buffer) != 0) {
        return fail("HTTP0 malformed-Connection error-response output fixture");
    }
    arbor_http_response bad_request_response = {0};
    if (arbor_http_response_make(
            400u, NULL, 0u, (arbor_span){NULL, 0u}, 0u,
            &bad_request_response).native != 0) {
        return fail("HTTP0 malformed-Connection error-response fixture");
    }
    uint64_t error_written = 0u;
    bool error_close = false;
    if (arbor_http_response_serialize(
            &error_buffer, &malformed_connection.native, &bad_request_response,
            &error_written, &error_close).native != 0 ||
        !error_close || error_written != error_buffer.length) {
        return fail("HTTP0 malformed Connection remains error-response serializable and forces close");
    }
    static const uint8_t close_line[] = "Connection: close\r\n";
    bool saw_close_line = false;
    if (error_buffer.length >= sizeof(close_line) - 1u) {
        for (uint64_t i = 0u; i + sizeof(close_line) - 1u <= error_buffer.length; ++i) {
            if (memcmp(error_buffer.data + i, close_line, sizeof(close_line) - 1u) == 0) {
                saw_close_line = true;
                break;
            }
        }
    }
    if (!saw_close_line) {
        return fail("HTTP0 malformed Connection error response emits close");
    }

    uint8_t output[128] = {0};
    arbor_asm_buffer buffer = {0};
    if (init_buffer(output, sizeof(output), &buffer) != 0 ||
        buffer_append(&buffer, "PRE", 3u).status != 0) {
        return fail("HTTP0 transactional output fixture");
    }
    const uint8_t before[3] = {'P', 'R', 'E'};
    buffer.capacity = 12u;
    uint64_t written = UINT64_MAX;
    bool close_after = true;
    if (arbor_http_response_serialize(
            &buffer, &request.native, &normal, &written, &close_after).native != -ENOSPC ||
        buffer.length != 3u || memcmp(buffer.data, before, sizeof(before)) != 0 ||
        written != 0u || close_after) {
        return fail("HTTP0 capacity failure preserves output transactionally");
    }

    buffer.capacity = sizeof(output);
    written = UINT64_C(77);
    close_after = true;
    if (arbor_http_response_serialize(
            &buffer, &request.native, &normal,
            (uint64_t *)(void *)(output + 40u), &close_after).native != -EINVAL ||
        buffer.length != 3u || close_after != true) {
        return fail("HTTP0 bytes-written output alias with serialized buffer rejection");
    }

    arbor_asm_buffer request_alias_buffer = {
        (uint8_t *)(uintptr_t)request.native.headers_ptr,
        0u,
        request.native.headers_len
    };
    written = UINT64_C(88);
    close_after = true;
    if (arbor_http_response_serialize(
            &request_alias_buffer, &request.native, &normal,
            &written, &close_after).native != -EINVAL ||
        request_alias_buffer.length != 0u || written != UINT64_C(88) || !close_after) {
        return fail("HTTP0 serializer output backing alias with request storage rejection");
    }
    normal.body_data = output + 20u;
    normal.body_length = 1u;
    output[20] = (uint8_t)'x';
    if (arbor_http_response_serialize(
            &buffer, &request.native, &normal, &written, &close_after).native != -EINVAL ||
        buffer.length != 3u) {
        return fail("HTTP0 body/output alias rejection");
    }

    normal.body_data = body;
    normal.body_length = sizeof(body) - 1u;
    arbor_http_field alias_field = {
        output + 30u, 1u, valid_value, sizeof(valid_value) - 1u
    };
    output[30] = (uint8_t)'X';
    normal.fields = &alias_field;
    normal.field_count = 1u;
    if (arbor_http_response_serialize(
            &buffer, &request.native, &normal, &written, &close_after).native != -EINVAL ||
        buffer.length != 3u) {
        return fail("HTTP0 field/output alias rejection");
    }

    static const uint8_t connect_bytes[] =
        "CONNECT server.example:443 HTTP/1.1\r\n"
        "Host: server.example:443\r\n\r\n";
    arbor_asm_http_request connect_request = {0};
    const arbor_asm_result_u64 connect_parsed = http_parse_request(
        connect_bytes, sizeof(connect_bytes) - 1u, &connect_request);
    if (connect_parsed.status != 0 ||
        connect_request.message_length != sizeof(connect_bytes) - 1u) {
        return fail("HTTP0 CONNECT native fixture parse");
    }
    arbor_http_response connect_ok = {0};
    if (arbor_http_response_make(
            200u, NULL, 0u, (arbor_span){NULL, 0u}, 0u, &connect_ok).native != 0) {
        return fail("HTTP0 CONNECT response fixture");
    }
    buffer.length = 0u;
    written = UINT64_C(66);
    close_after = true;
    if (arbor_http_response_serialize(
            &buffer, &connect_request, &connect_ok,
            &written, &close_after).native != -EINVAL ||
        buffer.length != 0u || written != UINT64_C(66) || !close_after) {
        return fail("HTTP0 CONNECT 2xx rejected until tunnel support exists");
    }

    puts("PASS: HTTP0 adversarial validation, framing ownership, alias and transactional policy");
    return 0;
}
