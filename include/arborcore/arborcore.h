#ifndef ARBORCORE_ARBORCORE_H
#define ARBORCORE_ARBORCORE_H

#include <stdbool.h>
#include <stdint.h>

#include <arborcore/assembly_abi.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ARBORCORE_C_RUNTIME_VERSION_MAJOR 0u
#define ARBORCORE_C_RUNTIME_VERSION_MINOR 1u
#define ARBORCORE_C_RUNTIME_VERSION_PATCH 0u

/*
 * The C runtime bridge is intentionally not ABI-frozen yet. Assembly ABI v1
 * beneath it is frozen. These types/functions may evolve while the C framework
 * is constructed, but they may not mutate the underlying Assembly contracts.
 */
typedef enum arbor_status_code {
    ARBOR_STATUS_OK = 0,
    ARBOR_STATUS_MORE_WORK,
    ARBOR_STATUS_NOT_FOUND,
    ARBOR_STATUS_INTERRUPTED,
    ARBOR_STATUS_IO,
    ARBOR_STATUS_WOULD_BLOCK,
    ARBOR_STATUS_INVALID_ARGUMENT,
    ARBOR_STATUS_NO_SPACE,
    ARBOR_STATUS_OVERFLOW,
    ARBOR_STATUS_CONNECTION_RESET,
    ARBOR_STATUS_NATIVE_ERROR,
    ARBOR_STATUS_NATIVE_SUCCESS
} arbor_status_code;

typedef struct arbor_status {
    arbor_status_code code;
    int64_t native;
} arbor_status;

typedef struct arbor_span {
    const uint8_t *data;
    uint64_t length;
} arbor_span;

typedef struct arbor_mut_span {
    uint8_t *data;
    uint64_t length;
} arbor_mut_span;

typedef struct arbor_request_view {
    arbor_asm_http_request native;
    arbor_asm_request_target target;
} arbor_request_view;

typedef arbor_asm_route arbor_route;
typedef arbor_asm_route_param arbor_route_param;
typedef arbor_asm_route_handler arbor_route_handler;

/*
 * Metadata container only. Caller-provided input/output/arena backing memory is
 * borrowed and remains caller-owned. The request view aliases input storage.
 */
typedef struct arbor_runtime_storage {
    arbor_asm_connection connection;
    arbor_asm_buffer input;
    arbor_asm_buffer output;
    arbor_asm_arena arena;
    arbor_asm_http_request request;
} arbor_runtime_storage;

arbor_status arbor_status_from_native(int64_t native);
bool arbor_status_is_success(arbor_status status);
bool arbor_status_is_error(arbor_status status);
const char *arbor_status_name(arbor_status status);

void *arbor_secure_clear(void *destination, uint64_t length);
bool arbor_secure_equal(const void *left, const void *right, uint64_t length);

arbor_status arbor_request_parse(arbor_span bytes, arbor_request_view *out, uint64_t *required_length);
arbor_status arbor_response_serialize(arbor_asm_buffer *buffer, uint64_t status, arbor_span body, bool keep_alive, uint64_t *bytes_written);

arbor_status arbor_route_init(arbor_route *route, arbor_span method, arbor_span pattern, arbor_route_handler handler);
arbor_status arbor_route_match(arbor_span pattern, arbor_span path, arbor_route_param *params, uint64_t params_capacity, bool *matched, uint64_t *parameter_count);
arbor_status arbor_route_dispatch(const arbor_route *routes, uint64_t route_count, const arbor_asm_http_request *request, void *context, arbor_route_param *params, uint64_t params_capacity, int64_t *handler_result);

arbor_status arbor_runtime_storage_prepare(
    arbor_runtime_storage *storage,
    arbor_mut_span input_storage,
    arbor_mut_span output_storage,
    arbor_mut_span arena_storage);

arbor_status arbor_server_open(const void *sockaddr, uint64_t sockaddr_length, int64_t backlog, int64_t *listener_fd);
arbor_status arbor_event_loop_create(int64_t listener_fd, int64_t *epoll_fd);
arbor_status arbor_server_accept(int64_t listener_fd, int64_t epoll_fd, arbor_runtime_storage *storage, int64_t *accepted_fd);
arbor_status arbor_server_step(arbor_runtime_storage *storage, const arbor_route *routes, uint64_t route_count, void *context, int64_t epoll_fd, uint64_t *completed_request_count);
arbor_status arbor_server_close(int64_t epoll_fd, arbor_runtime_storage *storage);

#ifdef __cplusplus
}
#endif

#endif
