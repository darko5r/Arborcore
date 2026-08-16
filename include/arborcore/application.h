#ifndef ARBORCORE_APPLICATION_H
#define ARBORCORE_APPLICATION_H

#include <stdint.h>

#include <arborcore/arborcore.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ARBOR_APPLICATION_FOUNDATION_VERSION_MAJOR 0u
#define ARBOR_APPLICATION_FOUNDATION_VERSION_MINOR 1u
#define ARBOR_APPLICATION_FOUNDATION_VERSION_PATCH 0u

#define ARBOR_APPLICATION_CAPABILITIES_ABI_VERSION 1u
#define ARBOR_APPLICATION_CAPABILITIES_FLAG_NONE UINT64_C(0)

#define ARBOR_RESPONSE_PLAN_FLAG_NONE UINT64_C(0)
#define ARBOR_RESPONSE_PLAN_FLAG_KEEP_ALIVE (UINT64_C(1) << 0)
#define ARBOR_RESPONSE_PLAN_KNOWN_FLAGS ARBOR_RESPONSE_PLAN_FLAG_KEEP_ALIVE

/*
 * Stable AF1 request-scope callback layout. It points only at frozen Assembly
 * request/target/route-param/arena representations, not at the evolvable C
 * arbor_request_view aggregate. All pointers are borrowed.
 */
typedef struct arbor_request_scope {
    const arbor_asm_http_request *request;
    const arbor_asm_request_target *target;
    const arbor_route_param *params;
    uint64_t parameter_count;
    arbor_asm_arena *arena;
} arbor_request_scope;

/* Stable AF1 response-plan callback layout. Body storage remains borrowed. */
typedef struct arbor_response_plan {
    uint64_t status;
    const uint8_t *body_data;
    uint64_t body_length;
    uint64_t flags;
} arbor_response_plan;

/*
 * SysV-friendly framework-to-application upcall:
 *   RDI = request scope
 *   RSI = opaque application context
 *   RDX = response plan output
 *   RAX = native mechanism status (0 success, negative failure; positive reserved)
 *
 * Public C callers use arbor_application_invoke(). Negative callback status is
 * normalized into arbor_status; positive callback status is invalid in AF1.
 */
typedef int64_t (*arbor_application_request_dispatch_fn)(
    const arbor_request_scope *scope,
    void *application_context,
    arbor_response_plan *response_out);

typedef struct arbor_application_capabilities {
    uint32_t abi_version;
    uint32_t struct_size;
    uint64_t flags;
    void *application_context;
    arbor_application_request_dispatch_fn request_dispatch;
} arbor_application_capabilities;

arbor_status arbor_request_scope_make(
    const arbor_request_view *request,
    const arbor_route_param *params,
    uint64_t parameter_count,
    arbor_asm_arena *arena,
    arbor_request_scope *out);

arbor_status arbor_request_scope_validate(const arbor_request_scope *scope);

arbor_status arbor_response_plan_make(
    uint64_t status,
    arbor_span body,
    uint64_t flags,
    arbor_response_plan *out);

arbor_status arbor_response_plan_validate(const arbor_response_plan *plan);

arbor_status arbor_response_plan_serialize(
    arbor_asm_buffer *buffer,
    const arbor_response_plan *plan,
    uint64_t *bytes_written);

arbor_status arbor_application_capabilities_make(
    arbor_application_request_dispatch_fn request_dispatch,
    void *application_context,
    arbor_application_capabilities *out);

arbor_status arbor_application_capabilities_validate(
    const arbor_application_capabilities *capabilities);

arbor_status arbor_application_invoke(
    const arbor_application_capabilities *capabilities,
    const arbor_request_scope *scope,
    arbor_response_plan *response_out);

#ifdef __cplusplus
}
#endif

#endif
