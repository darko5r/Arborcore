#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <arborcore/http.h>

extern int64_t http0_asm_call_field_validate(const arbor_http_field *field);
extern int64_t http0_asm_call_response_validate(const arbor_http_response *response);
extern int64_t http0_asm_call_connection_close(const arbor_asm_http_request *request, bool *close_requested);

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

static bool span_eq(arbor_span span, const char *text)
{
    const size_t length = strlen(text);
    return span.length == (uint64_t)length &&
           (length == 0u || memcmp(span.data, text, length) == 0);
}

int main(void)
{
    static const uint8_t request_bytes[] =
        "GET /hello HTTP/1.1\r\n"
        "Host: local.example:8080\r\n"
        "X-One: alpha\r\n"
        "x-one:\tbeta \t\r\n"
        "Connection: keep-alive, Upgrade\r\n"
        "Connection: CLOSE\r\n"
        "Cookie: a=1\r\n"
        "Cookie: b=2\r\n"
        "\r\n";

    arbor_request_view request = {0};
    if (parse_request(request_bytes, sizeof(request_bytes) - 1u, &request) != 0) {
        return fail("HTTP0 request parse fixture");
    }

    static const char *const names[] = {
        "Host", "X-One", "x-one", "Connection", "Connection", "Cookie", "Cookie"
    };
    static const char *const values[] = {
        "local.example:8080", "alpha", "beta", "keep-alive, Upgrade", "CLOSE", "a=1", "b=2"
    };

    uint64_t cursor = 0u;
    for (size_t i = 0u; i < sizeof(names) / sizeof(names[0]); ++i) {
        arbor_http_field field = {0};
        bool has_field = false;
        const arbor_status status = arbor_http_request_header_next(
            &request.native, &cursor, &field, &has_field);
        if (status.native != 0 || !has_field ||
            !span_eq((arbor_span){field.name_data, field.name_length}, names[i]) ||
            !span_eq((arbor_span){field.value_data, field.value_length}, values[i])) {
            return fail("HTTP0 ordered zero-copy header iteration");
        }
        if (field.name_data < request.native.headers_ptr ||
            field.name_data >= request.native.headers_ptr + request.native.headers_len ||
            field.value_data < request.native.headers_ptr ||
            field.value_data > request.native.headers_ptr + request.native.headers_len) {
            return fail("HTTP0 header views borrow request header storage");
        }
    }
    arbor_http_field end_sentinel = {
        (const uint8_t *)(uintptr_t)UINT64_C(1), UINT64_C(2),
        (const uint8_t *)(uintptr_t)UINT64_C(3), UINT64_C(4)
    };
    arbor_http_field end_before = end_sentinel;
    bool has_field = true;
    const uint64_t end_cursor = cursor;
    arbor_status status = arbor_http_request_header_next(
        &request.native, &cursor, &end_sentinel, &has_field);
    if (status.native != 0 || has_field || cursor != end_cursor ||
        memcmp(&end_sentinel, &end_before, sizeof(end_before)) != 0) {
        return fail("HTTP0 header iteration end is stable and non-publishing");
    }

    static const uint8_t x_one[] = "X-ONE";
    arbor_http_field first = {0};
    bool found = false;
    status = arbor_http_request_header_find_first(
        &request.native,
        (arbor_span){x_one, sizeof(x_one) - 1u},
        &first,
        &found);
    if (status.native != 0 || !found ||
        !span_eq((arbor_span){first.value_data, first.value_length}, "alpha")) {
        return fail("HTTP0 case-insensitive first header lookup");
    }

    uint64_t count = UINT64_MAX;
    status = arbor_http_request_header_count(
        &request.native,
        (arbor_span){x_one, sizeof(x_one) - 1u},
        &count);
    if (status.native != 0 || count != 2u) {
        return fail("HTTP0 duplicate header count");
    }

    if (http0_asm_call_field_validate(&first) != 0) {
        return fail("HTTP0 Assembly-to-C field validation ABI");
    }
    arbor_http_response abi_response = {
        ARBOR_HTTP_RESPONSE_ABI_VERSION,
        (uint32_t)sizeof(arbor_http_response),
        ARBOR_HTTP_RESPONSE_FLAG_NONE,
        200u,
        NULL,
        0u,
        NULL,
        0u
    };
    if (http0_asm_call_response_validate(&abi_response) != 0) {
        return fail("HTTP0 Assembly-to-C response validation ABI");
    }

    arbor_span host = {0};
    status = arbor_http_request_host_validate(&request.native, &host);
    if (status.native != 0 || !span_eq(host, "local.example:8080")) {
        return fail("HTTP0 exactly-one valid Host");
    }

    bool close_requested = false;
    status = arbor_http_request_connection_close(&request.native, &close_requested);
    if (status.native != 0 || !close_requested) {
        return fail("HTTP0 Connection close across multiple field lines");
    }

    static const uint8_t persistent_bytes[] =
        "GET / HTTP/1.1\r\nHost: local\r\nConnection: keep-alive\r\n\r\n";
    arbor_request_view persistent = {0};
    if (parse_request(persistent_bytes, sizeof(persistent_bytes) - 1u, &persistent) != 0) {
        return fail("HTTP0 persistent fixture parse");
    }
    close_requested = true;
    status = arbor_http_request_connection_close(&persistent.native, &close_requested);
    if (status.native != 0 || close_requested) {
        return fail("HTTP0 HTTP/1.1 persistence without close option");
    }
    close_requested = true;
    if (http0_asm_call_connection_close(&persistent.native, &close_requested) != 0 || close_requested) {
        return fail("HTTP0 Assembly-to-C Connection policy ABI");
    }

    static const uint8_t ipv6_bytes[] =
        "GET / HTTP/1.1\r\nHost: [2001:db8::1]:443\r\n\r\n";
    arbor_request_view ipv6 = {0};
    if (parse_request(ipv6_bytes, sizeof(ipv6_bytes) - 1u, &ipv6) != 0 ||
        arbor_http_request_host_validate(&ipv6.native, &host).native != 0 ||
        !span_eq(host, "[2001:db8::1]:443")) {
        return fail("HTTP0 bracketed Host syntax");
    }

    static const uint8_t ipvfuture_bytes[] =
        "GET / HTTP/1.1\r\nHost: [v1.alpha:beta]:443\r\n\r\n";
    arbor_request_view ipvfuture = {0};
    if (parse_request(ipvfuture_bytes, sizeof(ipvfuture_bytes) - 1u, &ipvfuture) != 0 ||
        arbor_http_request_host_validate(&ipvfuture.native, &host).native != 0 ||
        !span_eq(host, "[v1.alpha:beta]:443")) {
        return fail("HTTP0 IPvFuture Host syntax");
    }

    static const uint8_t zone_bytes[] =
        "GET / HTTP/1.1\r\nHost: [fe80::1%25eth0]:8080\r\n\r\n";
    arbor_request_view zone = {0};
    if (parse_request(zone_bytes, sizeof(zone_bytes) - 1u, &zone) != 0 ||
        arbor_http_request_host_validate(&zone.native, &host).native != -EINVAL) {
        return fail("HTTP0 obsolete RFC6874 zone-id Host syntax rejection");
    }

    static const uint8_t missing_host_bytes[] =
        "GET / HTTP/1.1\r\nX: y\r\n\r\n";
    arbor_request_view missing_host = {0};
    if (parse_request(missing_host_bytes, sizeof(missing_host_bytes) - 1u, &missing_host) != 0 ||
        arbor_http_request_host_validate(&missing_host.native, &host).native != -EINVAL) {
        return fail("HTTP0 missing Host rejection");
    }

    static const uint8_t duplicate_host_bytes[] =
        "GET / HTTP/1.1\r\nHost: a\r\nHost: b\r\n\r\n";
    arbor_request_view duplicate_host = {0};
    if (parse_request(duplicate_host_bytes, sizeof(duplicate_host_bytes) - 1u, &duplicate_host) != 0 ||
        arbor_http_request_host_validate(&duplicate_host.native, &host).native != -EINVAL) {
        return fail("HTTP0 duplicate Host rejection");
    }

    static const uint8_t invalid_ipv6_bytes[] =
        "GET / HTTP/1.1\r\nHost: [2001:::1]\r\n\r\n";
    arbor_request_view invalid_ipv6 = {0};
    if (parse_request(invalid_ipv6_bytes, sizeof(invalid_ipv6_bytes) - 1u, &invalid_ipv6) != 0 ||
        arbor_http_request_host_validate(&invalid_ipv6.native, &host).native != -EINVAL) {
        return fail("HTTP0 invalid IPv6 Host rejection");
    }

    static const uint8_t empty_host_bytes[] =
        "GET / HTTP/1.1\r\nHost:\r\n\r\n";
    arbor_request_view empty_host = {0};
    if (parse_request(empty_host_bytes, sizeof(empty_host_bytes) - 1u, &empty_host) != 0 ||
        arbor_http_request_host_validate(&empty_host.native, &host).native != -EINVAL) {
        return fail("HTTP0 empty Host rejection for supported origin-form");
    }

    puts("PASS: HTTP0 zero-copy request headers, Host and Connection semantics");
    return 0;
}
