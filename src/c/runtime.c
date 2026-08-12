#include <errno.h>

#include <arborcore/arborcore.h>

static arbor_status invalid_argument_status(void)
{
    return arbor_status_from_native(-EINVAL);
}

static arbor_status status_from_pair(arbor_asm_result_u64 result)
{
    return arbor_status_from_native(result.status);
}

arbor_status arbor_runtime_storage_prepare(
    arbor_runtime_storage *storage,
    arbor_mut_span input_storage,
    arbor_mut_span output_storage,
    arbor_mut_span arena_storage)
{
    if (storage == NULL) {
        return invalid_argument_status();
    }

    (void)memory_zero(storage, (uint64_t)sizeof(*storage));

    arbor_asm_result_u64 input_result = buffer_init(
        &storage->input,
        input_storage.data,
        input_storage.length);
    if (input_result.status < 0) {
        return status_from_pair(input_result);
    }

    arbor_asm_result_u64 output_result = buffer_init(
        &storage->output,
        output_storage.data,
        output_storage.length);
    if (output_result.status < 0) {
        return status_from_pair(output_result);
    }

    arbor_asm_result_u64 arena_result = arena_init(
        &storage->arena,
        arena_storage.data,
        arena_storage.length);
    if (arena_result.status < 0) {
        return status_from_pair(arena_result);
    }

    return arbor_status_from_native(0);
}

arbor_status arbor_server_open(const void *sockaddr, uint64_t sockaddr_length, int64_t backlog, int64_t *listener_fd)
{
    if (listener_fd == NULL) {
        return invalid_argument_status();
    }
    *listener_fd = -1;

    int64_t result = server_open_listener(sockaddr, sockaddr_length, backlog);
    if (result < 0) {
        return arbor_status_from_native(result);
    }

    *listener_fd = result;
    return arbor_status_from_native(0);
}

arbor_status arbor_event_loop_create(int64_t listener_fd, int64_t *epoll_fd)
{
    if (epoll_fd == NULL) {
        return invalid_argument_status();
    }
    *epoll_fd = -1;

    int64_t result = server_create_epoll(listener_fd);
    if (result < 0) {
        return arbor_status_from_native(result);
    }

    *epoll_fd = result;
    return arbor_status_from_native(0);
}

arbor_status arbor_server_accept(int64_t listener_fd, int64_t epoll_fd, arbor_runtime_storage *storage, int64_t *accepted_fd)
{
    if (storage == NULL || accepted_fd == NULL) {
        return invalid_argument_status();
    }
    *accepted_fd = -1;

    arbor_asm_result_i64 result = server_accept_connection(
        listener_fd,
        epoll_fd,
        &storage->connection,
        &storage->input,
        &storage->output,
        &storage->arena);

    if (result.status < 0) {
        return arbor_status_from_native(result.status);
    }

    *accepted_fd = result.value;
    return arbor_status_from_native(0);
}

arbor_status arbor_server_step(arbor_runtime_storage *storage, const arbor_route *routes, uint64_t route_count, void *context, int64_t epoll_fd, uint64_t *completed_request_count)
{
    if (storage == NULL) {
        return invalid_argument_status();
    }
    if (completed_request_count != NULL) {
        *completed_request_count = 0u;
    }

    arbor_asm_result_u64 result = server_handle_http_once(
        &storage->connection,
        &storage->request,
        routes,
        route_count,
        context,
        epoll_fd);

    if (result.status >= 0 && completed_request_count != NULL) {
        *completed_request_count = result.value;
    }
    return arbor_status_from_native(result.status);
}

arbor_status arbor_server_close(int64_t epoll_fd, arbor_runtime_storage *storage)
{
    if (storage == NULL) {
        return invalid_argument_status();
    }

    arbor_asm_result_u64 result = server_close_connection(epoll_fd, &storage->connection);
    return arbor_status_from_native(result.status);
}
