#ifndef ARBORCORE_HTTP_MVC_H
#define ARBORCORE_HTTP_MVC_H

#include <stdint.h>

#include <arborcore/http.h>
#include <arborcore/mvc.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ARBOR_HTTP_MVC_ABI_VERSION 1u
#define ARBOR_HTTP_MVC_APPLICATION_FLAGS_NONE UINT64_C(0)

typedef struct arbor_http_mvc_requirements {
    uint64_t arena_prefix_bytes;
    uint64_t response_field_capacity;
} arbor_http_mvc_requirements;

typedef struct arbor_http_mvc_application {
    uint32_t abi_version;
    uint32_t struct_size;
    uint64_t flags;
    arbor_mvc_application *mvc_application;
    arbor_application_capabilities mvc_capabilities;
    uint64_t response_field_capacity;
    uint64_t arena_prefix_bytes;
    uint64_t prepared_guard;
} arbor_http_mvc_application;

arbor_status arbor_http_mvc_application_measure(
    uint64_t response_field_capacity,
    arbor_http_mvc_requirements *out);

arbor_status arbor_http_mvc_application_prepare(
    arbor_mvc_application *mvc_application,
    uint64_t response_field_capacity,
    arbor_http_mvc_application *out);

arbor_status arbor_http_mvc_application_validate(
    const arbor_http_mvc_application *application);

/*
 * The response-field APIs require an arbor_mvc_request produced inside the
 * synchronous MVC invocation of arbor_http_mvc_server_step(). The request,
 * request scope and HTTP1 sidecar are callback-lifetime borrows and must not
 * escape that invocation. Field name/value bytes are borrowed and must remain
 * live until HTTP0 serialization completes for the current request.
 */
arbor_status arbor_http_mvc_response_field_append(
    const arbor_mvc_request *request,
    arbor_span name,
    arbor_span value);

arbor_status arbor_http_mvc_response_fields_mark(
    const arbor_mvc_request *request,
    uint64_t *mark_out);

arbor_status arbor_http_mvc_response_fields_rewind(
    const arbor_mvc_request *request,
    uint64_t mark);

arbor_status arbor_http_mvc_server_step(
    arbor_runtime_storage *storage,
    const arbor_http_mvc_application *application,
    int64_t epoll_fd,
    uint64_t *completed_request_count);

#ifdef __cplusplus
}
#endif

#endif
