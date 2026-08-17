#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <arborcore/http.h>

_Static_assert(sizeof(arbor_http_field) == 32u, "HTTP0 field layout drift");
_Static_assert(offsetof(arbor_http_field, name_data) == 0u, "HTTP0 field name pointer offset drift");
_Static_assert(offsetof(arbor_http_field, name_length) == 8u, "HTTP0 field name length offset drift");
_Static_assert(offsetof(arbor_http_field, value_data) == 16u, "HTTP0 field value pointer offset drift");
_Static_assert(offsetof(arbor_http_field, value_length) == 24u, "HTTP0 field value length offset drift");

_Static_assert(sizeof(arbor_http_response) == 56u, "HTTP0 response layout drift");
_Static_assert(offsetof(arbor_http_response, abi_version) == 0u, "HTTP0 response ABI version offset drift");
_Static_assert(offsetof(arbor_http_response, struct_size) == 4u, "HTTP0 response struct size offset drift");
_Static_assert(offsetof(arbor_http_response, flags) == 8u, "HTTP0 response flags offset drift");
_Static_assert(offsetof(arbor_http_response, status) == 16u, "HTTP0 response status offset drift");
_Static_assert(offsetof(arbor_http_response, fields) == 24u, "HTTP0 response fields offset drift");
_Static_assert(offsetof(arbor_http_response, field_count) == 32u, "HTTP0 response field count offset drift");
_Static_assert(offsetof(arbor_http_response, body_data) == 40u, "HTTP0 response body pointer offset drift");
_Static_assert(offsetof(arbor_http_response, body_length) == 48u, "HTTP0 response body length offset drift");

typedef struct http0_asm_response_args {
    uint64_t status;
    const uint8_t *reason_data;
    uint64_t reason_length;
    const arbor_http_field *fields;
    uint64_t field_count;
    const uint8_t *body_data;
    uint64_t body_length;
    uint64_t send_body;
    uint64_t emit_content_length;
    uint64_t close_after_response;
} http0_asm_response_args;

_Static_assert(sizeof(http0_asm_response_args) == 80u, "HTTP0 internal response args layout drift");
_Static_assert(offsetof(http0_asm_response_args, status) == 0u, "HTTP0 internal args status offset drift");
_Static_assert(offsetof(http0_asm_response_args, reason_data) == 8u, "HTTP0 internal args reason pointer offset drift");
_Static_assert(offsetof(http0_asm_response_args, reason_length) == 16u, "HTTP0 internal args reason length offset drift");
_Static_assert(offsetof(http0_asm_response_args, fields) == 24u, "HTTP0 internal args fields offset drift");
_Static_assert(offsetof(http0_asm_response_args, field_count) == 32u, "HTTP0 internal args field count offset drift");
_Static_assert(offsetof(http0_asm_response_args, body_data) == 40u, "HTTP0 internal args body pointer offset drift");
_Static_assert(offsetof(http0_asm_response_args, body_length) == 48u, "HTTP0 internal args body length offset drift");
_Static_assert(offsetof(http0_asm_response_args, send_body) == 56u, "HTTP0 internal args send-body offset drift");
_Static_assert(offsetof(http0_asm_response_args, emit_content_length) == 64u, "HTTP0 internal args content-length offset drift");
_Static_assert(offsetof(http0_asm_response_args, close_after_response) == 72u, "HTTP0 internal args close offset drift");

extern arbor_asm_result_u64 http0_header_next_asm(
    const uint8_t *headers,
    uint64_t headers_length,
    uint64_t cursor,
    arbor_http_field *out);

extern arbor_asm_result_u64 http0_response_serialize_asm(
    arbor_asm_buffer *buffer,
    const http0_asm_response_args *args);

static arbor_status invalid_argument_status(void)
{
    return arbor_status_from_native(-EINVAL);
}

static arbor_status overflow_status(void)
{
    return arbor_status_from_native(-EOVERFLOW);
}

static arbor_status ok_status(void)
{
    return (arbor_status){ARBOR_STATUS_OK, 0};
}

static bool object_span_valid(const void *data, uint64_t length)
{
    if (length == 0u) {
        return true;
    }
    if (data == NULL) {
        return false;
    }
    const uintptr_t start = (uintptr_t)data;
    return length <= (uint64_t)(UINTPTR_MAX - start);
}

static bool spans_overlap(
    const void *left,
    uint64_t left_length,
    const void *right,
    uint64_t right_length)
{
    if (left_length == 0u || right_length == 0u) {
        return false;
    }
    if (!object_span_valid(left, left_length) || !object_span_valid(right, right_length)) {
        return true;
    }
    const uintptr_t left_start = (uintptr_t)left;
    const uintptr_t right_start = (uintptr_t)right;
    const uintptr_t left_end = left_start + (uintptr_t)left_length;
    const uintptr_t right_end = right_start + (uintptr_t)right_length;
    return left_start < right_end && right_start < left_end;
}

static bool request_region_overlaps(
    const arbor_asm_http_request *request,
    const void *region,
    uint64_t region_length)
{
    if (request == NULL || !object_span_valid(region, region_length)) {
        return true;
    }
    return spans_overlap(region, region_length, request, sizeof(*request)) ||
           spans_overlap(region, region_length, request->method_ptr, request->method_len) ||
           spans_overlap(region, region_length, request->target_ptr, request->target_len) ||
           spans_overlap(region, region_length, request->version_ptr, request->version_len) ||
           spans_overlap(region, region_length, request->headers_ptr, request->headers_len) ||
           spans_overlap(region, region_length, request->body_ptr, request->body_available);
}

static bool regions_overlap(
    const void *left,
    uint64_t left_length,
    const void *right,
    uint64_t right_length)
{
    return spans_overlap(left, left_length, right, right_length);
}

static bool ascii_equal_ci(arbor_span left, arbor_span right)
{
    if (left.length != right.length) {
        return false;
    }
    if (!object_span_valid(left.data, left.length) || !object_span_valid(right.data, right.length)) {
        return false;
    }
    for (uint64_t i = 0u; i < left.length; ++i) {
        uint8_t a = left.data[i];
        uint8_t b = right.data[i];
        if (a >= (uint8_t)'A' && a <= (uint8_t)'Z') {
            a = (uint8_t)(a + ((uint8_t)'a' - (uint8_t)'A'));
        }
        if (b >= (uint8_t)'A' && b <= (uint8_t)'Z') {
            b = (uint8_t)(b + ((uint8_t)'a' - (uint8_t)'A'));
        }
        if (a != b) {
            return false;
        }
    }
    return true;
}

static bool span_equal_literal_ci(arbor_span value, const char *literal, uint64_t literal_length)
{
    return ascii_equal_ci(value, (arbor_span){(const uint8_t *)literal, literal_length});
}

static bool token_char(uint8_t value)
{
    if ((value >= (uint8_t)'0' && value <= (uint8_t)'9') ||
        (value >= (uint8_t)'A' && value <= (uint8_t)'Z') ||
        (value >= (uint8_t)'a' && value <= (uint8_t)'z')) {
        return true;
    }
    switch (value) {
        case (uint8_t)'!':
        case (uint8_t)'#':
        case (uint8_t)'$':
        case (uint8_t)'%':
        case (uint8_t)'&':
        case (uint8_t)'\'':
        case (uint8_t)'*':
        case (uint8_t)'+':
        case (uint8_t)'-':
        case (uint8_t)'.':
        case (uint8_t)'^':
        case (uint8_t)'_':
        case (uint8_t)'`':
        case (uint8_t)'|':
        case (uint8_t)'~':
            return true;
        default:
            return false;
    }
}

static bool token_span_valid(arbor_span value)
{
    if (value.length == 0u || !object_span_valid(value.data, value.length)) {
        return false;
    }
    for (uint64_t i = 0u; i < value.length; ++i) {
        if (!token_char(value.data[i])) {
            return false;
        }
    }
    return true;
}

static bool field_value_valid(arbor_span value)
{
    if (!object_span_valid(value.data, value.length)) {
        return false;
    }
    for (uint64_t i = 0u; i < value.length; ++i) {
        const uint8_t byte = value.data[i];
        if (byte == UINT8_C(0x09)) {
            continue;
        }
        if (byte < UINT8_C(0x20) || byte == UINT8_C(0x7f)) {
            return false;
        }
    }
    return true;
}

static arbor_status request_header_span_validate(const arbor_asm_http_request *request)
{
    static const uint8_t http11[] = "HTTP/1.1";
    if (request == NULL) {
        return invalid_argument_status();
    }
    if (!object_span_valid(request->headers_ptr, request->headers_len) ||
        !object_span_valid(request->method_ptr, request->method_len) ||
        !object_span_valid(request->target_ptr, request->target_len) ||
        !object_span_valid(request->version_ptr, request->version_len) ||
        !object_span_valid(request->body_ptr, request->body_available) ||
        request->content_length > request->body_available) {
        return invalid_argument_status();
    }
    if (request->method_len == 0u || request->target_len == 0u ||
        request->version_len != sizeof(http11) - 1u ||
        request->version_ptr == NULL ||
        memcmp(request->version_ptr, http11, sizeof(http11) - 1u) != 0) {
        return invalid_argument_status();
    }
    return ok_status();
}

arbor_status arbor_http_field_validate(const arbor_http_field *field)
{
    if (field == NULL) {
        return invalid_argument_status();
    }
    if (!token_span_valid((arbor_span){field->name_data, field->name_length}) ||
        !field_value_valid((arbor_span){field->value_data, field->value_length})) {
        return invalid_argument_status();
    }
    return ok_status();
}

arbor_status arbor_http_request_header_next(
    const arbor_asm_http_request *request,
    uint64_t *cursor,
    arbor_http_field *out,
    bool *has_field)
{
    if (cursor == NULL || out == NULL || has_field == NULL) {
        return invalid_argument_status();
    }
    arbor_status status = request_header_span_validate(request);
    if (status.native != 0) {
        return status;
    }
    if (!object_span_valid(cursor, sizeof(*cursor)) ||
        !object_span_valid(out, sizeof(*out)) ||
        !object_span_valid(has_field, sizeof(*has_field)) ||
        request_region_overlaps(request, cursor, sizeof(*cursor)) ||
        request_region_overlaps(request, out, sizeof(*out)) ||
        request_region_overlaps(request, has_field, sizeof(*has_field)) ||
        regions_overlap(cursor, sizeof(*cursor), out, sizeof(*out)) ||
        regions_overlap(cursor, sizeof(*cursor), has_field, sizeof(*has_field)) ||
        regions_overlap(out, sizeof(*out), has_field, sizeof(*has_field))) {
        return invalid_argument_status();
    }
    if (*cursor > request->headers_len) {
        return invalid_argument_status();
    }
    if (*cursor == request->headers_len) {
        *has_field = false;
        return ok_status();
    }

    arbor_http_field candidate = {0};
    const arbor_asm_result_u64 result = http0_header_next_asm(
        request->headers_ptr,
        request->headers_len,
        *cursor,
        &candidate);
    if (result.status != 0) {
        return arbor_status_from_native(result.status);
    }
    status = arbor_http_field_validate(&candidate);
    if (status.native != 0 || result.value <= *cursor || result.value > request->headers_len) {
        return invalid_argument_status();
    }

    *out = candidate;
    *cursor = result.value;
    *has_field = true;
    return ok_status();
}

arbor_status arbor_http_request_header_find_first(
    const arbor_asm_http_request *request,
    arbor_span name,
    arbor_http_field *out,
    bool *found)
{
    if (out == NULL || found == NULL || !token_span_valid(name)) {
        return invalid_argument_status();
    }
    arbor_status status = request_header_span_validate(request);
    if (status.native != 0) {
        return status;
    }
    if (!object_span_valid(out, sizeof(*out)) ||
        !object_span_valid(found, sizeof(*found)) ||
        request_region_overlaps(request, out, sizeof(*out)) ||
        request_region_overlaps(request, found, sizeof(*found)) ||
        regions_overlap(out, sizeof(*out), found, sizeof(*found)) ||
        regions_overlap(out, sizeof(*out), name.data, name.length) ||
        regions_overlap(found, sizeof(*found), name.data, name.length)) {
        return invalid_argument_status();
    }

    uint64_t cursor = 0u;
    while (cursor < request->headers_len) {
        arbor_http_field field = {0};
        bool has_field = false;
        status = arbor_http_request_header_next(request, &cursor, &field, &has_field);
        if (status.native != 0) {
            return status;
        }
        if (!has_field) {
            break;
        }
        if (ascii_equal_ci(
                (arbor_span){field.name_data, field.name_length}, name)) {
            *out = field;
            *found = true;
            return ok_status();
        }
    }

    *found = false;
    return ok_status();
}

arbor_status arbor_http_request_header_count(
    const arbor_asm_http_request *request,
    arbor_span name,
    uint64_t *count)
{
    if (count == NULL || !token_span_valid(name)) {
        return invalid_argument_status();
    }
    arbor_status status = request_header_span_validate(request);
    if (status.native != 0) {
        return status;
    }
    if (!object_span_valid(count, sizeof(*count)) ||
        request_region_overlaps(request, count, sizeof(*count)) ||
        regions_overlap(count, sizeof(*count), name.data, name.length)) {
        return invalid_argument_status();
    }

    uint64_t cursor = 0u;
    uint64_t candidate = 0u;
    while (cursor < request->headers_len) {
        arbor_http_field field = {0};
        bool has_field = false;
        status = arbor_http_request_header_next(request, &cursor, &field, &has_field);
        if (status.native != 0) {
            return status;
        }
        if (!has_field) {
            break;
        }
        if (ascii_equal_ci((arbor_span){field.name_data, field.name_length}, name)) {
            if (candidate == UINT64_MAX) {
                return overflow_status();
            }
            candidate += 1u;
        }
    }

    *count = candidate;
    return ok_status();
}

static bool uri_unreserved_char(uint8_t value)
{
    return (value >= (uint8_t)'0' && value <= (uint8_t)'9') ||
           (value >= (uint8_t)'A' && value <= (uint8_t)'Z') ||
           (value >= (uint8_t)'a' && value <= (uint8_t)'z') ||
           value == (uint8_t)'-' || value == (uint8_t)'.' ||
           value == (uint8_t)'_' || value == (uint8_t)'~';
}

static bool uri_sub_delim_char(uint8_t value)
{
    switch (value) {
        case (uint8_t)'!':
        case (uint8_t)'$':
        case (uint8_t)'&':
        case (uint8_t)'\'':
        case (uint8_t)'(':
        case (uint8_t)')':
        case (uint8_t)'*':
        case (uint8_t)'+':
        case (uint8_t)',':
        case (uint8_t)';':
        case (uint8_t)'=':
            return true;
        default:
            return false;
    }
}

static bool hex_digit(uint8_t value)
{
    return (value >= (uint8_t)'0' && value <= (uint8_t)'9') ||
           (value >= (uint8_t)'A' && value <= (uint8_t)'F') ||
           (value >= (uint8_t)'a' && value <= (uint8_t)'f');
}

static bool uri_pct_encoded_at(arbor_span value, uint64_t index)
{
    return index + 2u < value.length && value.data[index] == (uint8_t)'%' &&
           hex_digit(value.data[index + 1u]) && hex_digit(value.data[index + 2u]);
}

static bool reg_name_valid(arbor_span value)
{
    if (value.length == 0u || !object_span_valid(value.data, value.length)) {
        return false;
    }
    uint64_t index = 0u;
    while (index < value.length) {
        const uint8_t byte = value.data[index];
        if (uri_unreserved_char(byte) || uri_sub_delim_char(byte)) {
            index += 1u;
            continue;
        }
        if (byte == (uint8_t)'%' && uri_pct_encoded_at(value, index)) {
            index += 3u;
            continue;
        }
        return false;
    }
    return true;
}

static bool ipv4_octet_valid(arbor_span value)
{
    if (value.length == 0u || value.length > 3u ||
        !object_span_valid(value.data, value.length)) {
        return false;
    }
    if (value.length > 1u && value.data[0] == (uint8_t)'0') {
        return false;
    }
    uint32_t number = 0u;
    for (uint64_t i = 0u; i < value.length; ++i) {
        const uint8_t byte = value.data[i];
        if (byte < (uint8_t)'0' || byte > (uint8_t)'9') {
            return false;
        }
        number = number * UINT32_C(10) + (uint32_t)(byte - (uint8_t)'0');
    }
    return number <= UINT32_C(255);
}

static bool ipv4_address_valid(arbor_span value)
{
    if (value.length == 0u || !object_span_valid(value.data, value.length)) {
        return false;
    }
    uint64_t start = 0u;
    uint32_t parts = 0u;
    for (uint64_t i = 0u; i <= value.length; ++i) {
        if (i != value.length && value.data[i] != (uint8_t)'.') {
            continue;
        }
        if (parts >= 4u || !ipv4_octet_valid((arbor_span){value.data + start, i - start})) {
            return false;
        }
        parts += 1u;
        start = i + 1u;
    }
    return parts == 4u;
}

static bool h16_valid(arbor_span value)
{
    if (value.length == 0u || value.length > 4u ||
        !object_span_valid(value.data, value.length)) {
        return false;
    }
    for (uint64_t i = 0u; i < value.length; ++i) {
        if (!hex_digit(value.data[i])) {
            return false;
        }
    }
    return true;
}

static bool ipv6_address_valid(arbor_span value)
{
    if (value.length == 0u || !object_span_valid(value.data, value.length)) {
        return false;
    }

    uint64_t index = 0u;
    uint32_t groups = 0u;
    bool compressed = false;

    if (value.length >= 2u && value.data[0] == (uint8_t)':' &&
        value.data[1] == (uint8_t)':') {
        compressed = true;
        index = 2u;
        if (index == value.length) {
            return true;
        }
    } else if (value.data[0] == (uint8_t)':') {
        return false;
    }

    while (index < value.length) {
        const uint64_t segment_start = index;
        while (index < value.length && value.data[index] != (uint8_t)':') {
            index += 1u;
        }
        if (index == segment_start) {
            return false;
        }
        const arbor_span segment = {value.data + segment_start, index - segment_start};

        bool has_dot = false;
        for (uint64_t i = 0u; i < segment.length; ++i) {
            if (segment.data[i] == (uint8_t)'.') {
                has_dot = true;
                break;
            }
        }
        if (has_dot) {
            if (index != value.length || !ipv4_address_valid(segment) || groups > 6u) {
                return false;
            }
            groups += 2u;
            break;
        }
        if (!h16_valid(segment) || groups >= 8u) {
            return false;
        }
        groups += 1u;

        if (index == value.length) {
            break;
        }

        index += 1u;
        if (index == value.length) {
            return false;
        }
        if (value.data[index] == (uint8_t)':') {
            if (compressed) {
                return false;
            }
            compressed = true;
            index += 1u;
            if (index == value.length) {
                break;
            }
        }
    }

    return compressed ? groups < 8u : groups == 8u;
}

static bool ipvfuture_valid(arbor_span value)
{
    if (value.length < 4u || !object_span_valid(value.data, value.length) ||
        (value.data[0] != (uint8_t)'v' && value.data[0] != (uint8_t)'V')) {
        return false;
    }
    uint64_t index = 1u;
    const uint64_t version_start = index;
    while (index < value.length && hex_digit(value.data[index])) {
        index += 1u;
    }
    if (index == version_start || index >= value.length || value.data[index] != (uint8_t)'.') {
        return false;
    }
    index += 1u;
    if (index == value.length) {
        return false;
    }
    while (index < value.length) {
        const uint8_t byte = value.data[index];
        if (!uri_unreserved_char(byte) && !uri_sub_delim_char(byte) && byte != (uint8_t)':') {
            return false;
        }
        index += 1u;
    }
    return true;
}

static bool ip_literal_valid(arbor_span value)
{
    if (value.length == 0u || !object_span_valid(value.data, value.length)) {
        return false;
    }
    return ipvfuture_valid(value) || ipv6_address_valid(value);
}

static bool host_value_valid(arbor_span value)
{
    if (value.length == 0u || !object_span_valid(value.data, value.length)) {
        return false;
    }

    uint64_t host_end = value.length;
    if (value.data[0] == (uint8_t)'[') {
        uint64_t close_index = 1u;
        while (close_index < value.length && value.data[close_index] != (uint8_t)']') {
            close_index += 1u;
        }
        if (close_index >= value.length || close_index == 1u ||
            !ip_literal_valid((arbor_span){value.data + 1u, close_index - 1u})) {
            return false;
        }
        host_end = close_index + 1u;
    } else {
        uint64_t colon = 0u;
        while (colon < value.length && value.data[colon] != (uint8_t)':') {
            colon += 1u;
        }
        if (!reg_name_valid((arbor_span){value.data, colon})) {
            return false;
        }
        host_end = colon;
    }

    if (host_end == value.length) {
        return true;
    }
    if (value.data[host_end] != (uint8_t)':') {
        return false;
    }
    for (uint64_t i = host_end + 1u; i < value.length; ++i) {
        if (value.data[i] < (uint8_t)'0' || value.data[i] > (uint8_t)'9') {
            return false;
        }
    }
    return true;
}

arbor_status arbor_http_request_host_validate(
    const arbor_asm_http_request *request,
    arbor_span *host)
{
    if (host == NULL) {
        return invalid_argument_status();
    }
    arbor_status status = request_header_span_validate(request);
    if (status.native != 0) {
        return status;
    }
    if (!object_span_valid(host, sizeof(*host)) ||
        request_region_overlaps(request, host, sizeof(*host))) {
        return invalid_argument_status();
    }
    if (request->target_len == 0u || request->target_ptr == NULL ||
        !((request->target_ptr[0] == (uint8_t)'/') ||
          (request->target_len == 1u && request->target_ptr[0] == (uint8_t)'*'))) {
        return invalid_argument_status();
    }

    static const uint8_t host_name[] = "Host";
    uint64_t count = 0u;
    status = arbor_http_request_header_count(
        request,
        (arbor_span){host_name, sizeof(host_name) - 1u},
        &count);
    if (status.native != 0) {
        return status;
    }
    if (count != 1u) {
        return invalid_argument_status();
    }

    arbor_http_field field = {0};
    bool found = false;
    status = arbor_http_request_header_find_first(
        request,
        (arbor_span){host_name, sizeof(host_name) - 1u},
        &field,
        &found);
    if (status.native != 0 || !found ||
        !host_value_valid((arbor_span){field.value_data, field.value_length})) {
        return invalid_argument_status();
    }

    *host = (arbor_span){field.value_data, field.value_length};
    return ok_status();
}

static bool ows_byte(uint8_t value)
{
    return value == UINT8_C(0x20) || value == UINT8_C(0x09);
}

static arbor_status connection_value_has_close(arbor_span value, bool *close_found)
{
    if (close_found == NULL || !field_value_valid(value)) {
        return invalid_argument_status();
    }

    uint64_t index = 0u;
    bool close_candidate = false;
    while (index < value.length) {
        while (index < value.length &&
               (ows_byte(value.data[index]) || value.data[index] == (uint8_t)',')) {
            index += 1u;
        }
        if (index == value.length) {
            break;
        }

        const uint64_t start = index;
        while (index < value.length && token_char(value.data[index])) {
            index += 1u;
        }
        if (index == start) {
            return invalid_argument_status();
        }
        const arbor_span token = {value.data + start, index - start};
        if (span_equal_literal_ci(token, "close", UINT64_C(5))) {
            close_candidate = true;
        }

        while (index < value.length && ows_byte(value.data[index])) {
            index += 1u;
        }
        if (index < value.length) {
            if (value.data[index] != (uint8_t)',') {
                return invalid_argument_status();
            }
            index += 1u;
        }
    }

    *close_found = close_candidate;
    return ok_status();
}

arbor_status arbor_http_request_connection_close(
    const arbor_asm_http_request *request,
    bool *close_requested)
{
    if (close_requested == NULL) {
        return invalid_argument_status();
    }
    arbor_status status = request_header_span_validate(request);
    if (status.native != 0) {
        return status;
    }
    if (!object_span_valid(close_requested, sizeof(*close_requested)) ||
        request_region_overlaps(request, close_requested, sizeof(*close_requested))) {
        return invalid_argument_status();
    }

    static const uint8_t connection_name[] = "Connection";
    uint64_t cursor = 0u;
    bool candidate = false;
    while (cursor < request->headers_len) {
        arbor_http_field field = {0};
        bool has_field = false;
        status = arbor_http_request_header_next(request, &cursor, &field, &has_field);
        if (status.native != 0) {
            return status;
        }
        if (!has_field) {
            break;
        }
        if (!ascii_equal_ci(
                (arbor_span){field.name_data, field.name_length},
                (arbor_span){connection_name, sizeof(connection_name) - 1u})) {
            continue;
        }
        bool field_close = false;
        status = connection_value_has_close(
            (arbor_span){field.value_data, field.value_length},
            &field_close);
        if (status.native != 0) {
            return status;
        }
        candidate = candidate || field_close;
    }

    *close_requested = candidate;
    return ok_status();
}

static bool response_reserved_field_name(arbor_span name)
{
    static const char *const reserved[] = {
        "Content-Length",
        "Connection",
        "Transfer-Encoding",
        "Trailer",
        "Upgrade"
    };
    static const uint64_t lengths[] = {14u, 10u, 17u, 7u, 7u};
    for (size_t i = 0u; i < sizeof(reserved) / sizeof(reserved[0]); ++i) {
        if (span_equal_literal_ci(name, reserved[i], lengths[i])) {
            return true;
        }
    }
    return false;
}

arbor_status arbor_http_response_validate(const arbor_http_response *response)
{
    if (response == NULL ||
        response->abi_version != ARBOR_HTTP_RESPONSE_ABI_VERSION ||
        response->struct_size < (uint32_t)sizeof(arbor_http_response) ||
        (response->flags & ~ARBOR_HTTP_RESPONSE_KNOWN_FLAGS) != 0u ||
        response->status < UINT64_C(200) || response->status > UINT64_C(599)) {
        return invalid_argument_status();
    }
    if (!object_span_valid(response, (uint64_t)response->struct_size)) {
        return invalid_argument_status();
    }

    if (!object_span_valid(response->body_data, response->body_length)) {
        return invalid_argument_status();
    }
    if ((response->status == UINT64_C(204) ||
         response->status == UINT64_C(205) ||
         response->status == UINT64_C(304)) &&
        response->body_length != 0u) {
        return invalid_argument_status();
    }

    if (response->field_count > UINT64_MAX / (uint64_t)sizeof(arbor_http_field)) {
        return overflow_status();
    }
    const uint64_t field_bytes = response->field_count * (uint64_t)sizeof(arbor_http_field);
    if (!object_span_valid(response->fields, field_bytes)) {
        return invalid_argument_status();
    }

    for (uint64_t i = 0u; i < response->field_count; ++i) {
        arbor_status status = arbor_http_field_validate(&response->fields[i]);
        if (status.native != 0) {
            return status;
        }
        if (response_reserved_field_name(
                (arbor_span){response->fields[i].name_data, response->fields[i].name_length})) {
            return invalid_argument_status();
        }
    }
    return ok_status();
}

static bool response_sources_overlap_region(
    const arbor_http_response *response,
    const void *region,
    uint64_t region_length)
{
    if (response == NULL || !object_span_valid(region, region_length)) {
        return true;
    }
    const uint64_t field_bytes = response->field_count * (uint64_t)sizeof(arbor_http_field);
    if (spans_overlap(region, region_length, response, (uint64_t)response->struct_size) ||
        spans_overlap(region, region_length, response->fields, field_bytes) ||
        spans_overlap(region, region_length, response->body_data, response->body_length)) {
        return true;
    }
    for (uint64_t i = 0u; i < response->field_count; ++i) {
        const arbor_http_field *field = &response->fields[i];
        if (spans_overlap(region, region_length, field->name_data, field->name_length) ||
            spans_overlap(region, region_length, field->value_data, field->value_length)) {
            return true;
        }
    }
    return false;
}

arbor_status arbor_http_response_make(
    uint64_t status,
    const arbor_http_field *fields,
    uint64_t field_count,
    arbor_span body,
    uint64_t flags,
    arbor_http_response *out)
{
    if (out == NULL) {
        return invalid_argument_status();
    }

    const arbor_http_response candidate = {
        ARBOR_HTTP_RESPONSE_ABI_VERSION,
        (uint32_t)sizeof(arbor_http_response),
        flags,
        status,
        fields,
        field_count,
        body.data,
        body.length
    };
    arbor_status result = arbor_http_response_validate(&candidate);
    if (result.native != 0) {
        return result;
    }
    if (!object_span_valid(out, sizeof(*out)) ||
        response_sources_overlap_region(&candidate, out, sizeof(*out))) {
        return invalid_argument_status();
    }

    *out = candidate;
    return ok_status();
}

static bool method_is(const arbor_asm_http_request *request, const char *method, uint64_t length)
{
    if (request == NULL || request->method_len != length ||
        !object_span_valid(request->method_ptr, request->method_len)) {
        return false;
    }
    for (uint64_t i = 0u; i < length; ++i) {
        if (request->method_ptr[i] != (uint8_t)method[i]) {
            return false;
        }
    }
    return true;
}

static arbor_span reason_phrase(uint64_t status)
{
#define REASON(code, text) case UINT64_C(code): { \
    static const uint8_t value[] = text; \
    return (arbor_span){value, sizeof(value) - 1u}; \
}
    switch (status) {
        REASON(200, "OK")
        REASON(201, "Created")
        REASON(202, "Accepted")
        REASON(204, "No Content")
        REASON(205, "Reset Content")
        REASON(206, "Partial Content")
        REASON(301, "Moved Permanently")
        REASON(302, "Found")
        REASON(303, "See Other")
        REASON(304, "Not Modified")
        REASON(307, "Temporary Redirect")
        REASON(308, "Permanent Redirect")
        REASON(400, "Bad Request")
        REASON(401, "Unauthorized")
        REASON(403, "Forbidden")
        REASON(404, "Not Found")
        REASON(405, "Method Not Allowed")
        REASON(409, "Conflict")
        REASON(410, "Gone")
        REASON(412, "Precondition Failed")
        REASON(415, "Unsupported Media Type")
        REASON(422, "Unprocessable Content")
        REASON(429, "Too Many Requests")
        REASON(500, "Internal Server Error")
        REASON(501, "Not Implemented")
        REASON(503, "Service Unavailable")
        default:
            return (arbor_span){NULL, 0u};
    }
#undef REASON
}

static arbor_status response_output_alias_validate(
    const arbor_asm_buffer *buffer,
    const arbor_asm_http_request *request,
    const arbor_http_response *response,
    const uint64_t *bytes_written,
    const bool *close_after_response)
{
    if (buffer == NULL || request == NULL || response == NULL ||
        buffer->length > buffer->capacity ||
        !object_span_valid(buffer->data, buffer->capacity) ||
        !object_span_valid(buffer, sizeof(*buffer))) {
        return invalid_argument_status();
    }

    if (spans_overlap(buffer, sizeof(*buffer), buffer->data, buffer->capacity) ||
        request_region_overlaps(request, buffer, sizeof(*buffer)) ||
        response_sources_overlap_region(response, buffer, sizeof(*buffer)) ||
        request_region_overlaps(request, buffer->data, buffer->capacity) ||
        response_sources_overlap_region(response, buffer->data, buffer->capacity)) {
        return invalid_argument_status();
    }

    if (bytes_written != NULL) {
        if (!object_span_valid(bytes_written, sizeof(*bytes_written)) ||
            spans_overlap(bytes_written, sizeof(*bytes_written), buffer, sizeof(*buffer)) ||
            spans_overlap(bytes_written, sizeof(*bytes_written), buffer->data, buffer->capacity) ||
            request_region_overlaps(request, bytes_written, sizeof(*bytes_written)) ||
            response_sources_overlap_region(response, bytes_written, sizeof(*bytes_written))) {
            return invalid_argument_status();
        }
    }
    if (close_after_response != NULL) {
        if (!object_span_valid(close_after_response, sizeof(*close_after_response)) ||
            spans_overlap(close_after_response, sizeof(*close_after_response), buffer, sizeof(*buffer)) ||
            spans_overlap(close_after_response, sizeof(*close_after_response), buffer->data, buffer->capacity) ||
            request_region_overlaps(request, close_after_response, sizeof(*close_after_response)) ||
            response_sources_overlap_region(response, close_after_response, sizeof(*close_after_response))) {
            return invalid_argument_status();
        }
    }
    if (bytes_written != NULL && close_after_response != NULL &&
        spans_overlap(bytes_written, sizeof(*bytes_written),
                      close_after_response, sizeof(*close_after_response))) {
        return invalid_argument_status();
    }
    return ok_status();
}

arbor_status arbor_http_response_serialize(
    arbor_asm_buffer *buffer,
    const arbor_asm_http_request *request,
    const arbor_http_response *response,
    uint64_t *bytes_written,
    bool *close_after_response)
{
    arbor_status status = request_header_span_validate(request);
    if (status.native != 0) {
        return status;
    }
    status = arbor_http_response_validate(response);
    if (status.native != 0) {
        return status;
    }
    status = response_output_alias_validate(
        buffer, request, response, bytes_written, close_after_response);
    if (status.native != 0) {
        return status;
    }
    if (method_is(request, "CONNECT", UINT64_C(7)) &&
        response->status >= UINT64_C(200) && response->status <= UINT64_C(299)) {
        return invalid_argument_status();
    }

    if (bytes_written != NULL) {
        *bytes_written = 0u;
    }
    if (close_after_response != NULL) {
        *close_after_response = false;
    }

    bool request_close = false;
    status = arbor_http_request_connection_close(request, &request_close);
    if (status.native != 0) {
        /* A malformed Connection field is request-policy failure, but it must
         * not make an HTTP error response unserializable. Fail closed for the
         * transport decision; HTTP1 remains responsible for selecting 400. */
        request_close = true;
    }

    const bool application_close =
        (response->flags & ARBOR_HTTP_RESPONSE_FLAG_CLOSE) != 0u;
    const bool close = request_close || application_close;
    const bool head = method_is(request, "HEAD", UINT64_C(4));
    const bool no_content = response->status == UINT64_C(204) ||
                            response->status == UINT64_C(205) ||
                            response->status == UINT64_C(304);
    const bool send_body = !head && !no_content && response->body_length != 0u;
    const bool emit_content_length = response->status != UINT64_C(204) &&
                                     response->status != UINT64_C(304);
    const arbor_span reason = reason_phrase(response->status);

    const http0_asm_response_args args = {
        response->status,
        reason.data,
        reason.length,
        response->fields,
        response->field_count,
        response->body_data,
        response->body_length,
        send_body ? UINT64_C(1) : UINT64_C(0),
        emit_content_length ? UINT64_C(1) : UINT64_C(0),
        close ? UINT64_C(1) : UINT64_C(0)
    };

    const arbor_asm_result_u64 serialized = http0_response_serialize_asm(buffer, &args);
    if (serialized.status != 0) {
        return arbor_status_from_native(serialized.status);
    }
    if (bytes_written != NULL) {
        *bytes_written = serialized.value;
    }
    if (close_after_response != NULL) {
        *close_after_response = close;
    }
    return ok_status();
}
