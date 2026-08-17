#ifndef ARBORCORE_HTTP_H
#define ARBORCORE_HTTP_H

#include <stdbool.h>
#include <stdint.h>

#include <arborcore/arborcore.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ARBOR_HTTP_VERSION_MAJOR 0u
#define ARBOR_HTTP_VERSION_MINOR 1u
#define ARBOR_HTTP_VERSION_PATCH 0u

#define ARBOR_HTTP_RESPONSE_ABI_VERSION 1u
#define ARBOR_HTTP_RESPONSE_FLAG_NONE UINT64_C(0)
#define ARBOR_HTTP_RESPONSE_FLAG_CLOSE (UINT64_C(1) << 0)
#define ARBOR_HTTP_RESPONSE_KNOWN_FLAGS ARBOR_HTTP_RESPONSE_FLAG_CLOSE

typedef struct arbor_http_field {
    const uint8_t *name_data;
    uint64_t name_length;
    const uint8_t *value_data;
    uint64_t value_length;
} arbor_http_field;

typedef struct arbor_http_response {
    uint32_t abi_version;
    uint32_t struct_size;
    uint64_t flags;
    uint64_t status;
    const arbor_http_field *fields;
    uint64_t field_count;
    const uint8_t *body_data;
    uint64_t body_length;
} arbor_http_response;

arbor_status arbor_http_field_validate(const arbor_http_field *field);

arbor_status arbor_http_request_header_next(
    const arbor_asm_http_request *request,
    uint64_t *cursor,
    arbor_http_field *out,
    bool *has_field);

arbor_status arbor_http_request_header_find_first(
    const arbor_asm_http_request *request,
    arbor_span name,
    arbor_http_field *out,
    bool *found);

arbor_status arbor_http_request_header_count(
    const arbor_asm_http_request *request,
    arbor_span name,
    uint64_t *count);

arbor_status arbor_http_request_host_validate(
    const arbor_asm_http_request *request,
    arbor_span *host);

arbor_status arbor_http_request_connection_close(
    const arbor_asm_http_request *request,
    bool *close_requested);

arbor_status arbor_http_response_make(
    uint64_t status,
    const arbor_http_field *fields,
    uint64_t field_count,
    arbor_span body,
    uint64_t flags,
    arbor_http_response *out);

arbor_status arbor_http_response_validate(const arbor_http_response *response);

arbor_status arbor_http_response_serialize(
    arbor_asm_buffer *buffer,
    const arbor_asm_http_request *request,
    const arbor_http_response *response,
    uint64_t *bytes_written,
    bool *close_after_response);

#ifdef __cplusplus
}
#endif

#endif
