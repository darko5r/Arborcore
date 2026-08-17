#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <arborcore/application_service.h>
#include <arborcore/application_transport.h>
#include <arborcore/ddd_support.h>
#include <arborcore/mvc.h>

static void close_if_valid(int fd)
{
    if (fd >= 0) {
        (void)close(fd);
    }
}

static bool contains_bytes(
    const uint8_t *haystack,
    size_t haystack_length,
    const uint8_t *needle,
    size_t needle_length)
{
    if (needle_length == 0u) {
        return true;
    }
    if (haystack == NULL || needle == NULL || needle_length > haystack_length) {
        return false;
    }
    for (size_t i = 0u; i + needle_length <= haystack_length; ++i) {
        if (memcmp(haystack + i, needle, needle_length) == 0) {
            return true;
        }
    }
    return false;
}

static int wait_readable(int fd)
{
    struct pollfd pfd = {fd, POLLIN | POLLHUP, 0};
    int result = poll(&pfd, 1u, 1000);
    return result > 0 ? 0 : 1;
}

static ssize_t drain_nonblocking(int fd, uint8_t *buffer, size_t capacity, bool *saw_eof)
{
    size_t used = 0u;
    if (saw_eof != NULL) {
        *saw_eof = false;
    }
    while (used < capacity) {
        ssize_t got = recv(fd, buffer + used, capacity - used, MSG_DONTWAIT);
        if (got > 0) {
            used += (size_t)got;
            continue;
        }
        if (got == 0) {
            if (saw_eof != NULL) {
                *saw_eof = true;
            }
            break;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            break;
        }
        return -1;
    }
    return (ssize_t)used;
}

typedef struct e2e_tx_state {
    uint64_t active;
} e2e_tx_state;

typedef struct e2e_tx_context {
    uint64_t begins;
    uint64_t commits;
    uint64_t rollbacks;
} e2e_tx_context;

static int64_t e2e_tx_begin(void *provider_context, void *state, uint64_t state_size)
{
    e2e_tx_context *context = (e2e_tx_context *)provider_context;
    e2e_tx_state *tx = (e2e_tx_state *)state;
    if (context == NULL || tx == NULL || state_size != sizeof(*tx)) {
        return -EINVAL;
    }
    context->begins += 1u;
    tx->active = 1u;
    return 0;
}

static int64_t e2e_tx_commit(void *provider_context, void *state)
{
    e2e_tx_context *context = (e2e_tx_context *)provider_context;
    e2e_tx_state *tx = (e2e_tx_state *)state;
    if (context == NULL || tx == NULL || tx->active != 1u) {
        return -EINVAL;
    }
    context->commits += 1u;
    tx->active = 0u;
    return 0;
}

static int64_t e2e_tx_rollback(void *provider_context, void *state)
{
    e2e_tx_context *context = (e2e_tx_context *)provider_context;
    e2e_tx_state *tx = (e2e_tx_state *)state;
    if (context == NULL || tx == NULL) {
        return -EINVAL;
    }
    context->rollbacks += 1u;
    tx->active = 0u;
    return 0;
}

typedef struct e2e_service_input {
    arbor_asm_arena *arena;
    const uint8_t *name;
    uint64_t name_length;
} e2e_service_input;

typedef struct e2e_service_output {
    const uint8_t *body;
    uint64_t body_length;
} e2e_service_output;

typedef int64_t (*e2e_service_execute_fn)(
    void *provider_context,
    const e2e_service_input *input,
    e2e_service_output *out);

typedef struct e2e_service_v1 {
    arbor_application_service_interface_header header;
    e2e_service_execute_fn execute;
} e2e_service_v1;

typedef struct e2e_service_context {
    arbor_capability_binding transaction;
    uint64_t calls;
} e2e_service_context;

static int64_t e2e_service_execute(
    void *provider_context,
    const e2e_service_input *input,
    e2e_service_output *out)
{
    e2e_service_context *context = (e2e_service_context *)provider_context;
    if (context == NULL || input == NULL || input->arena == NULL || out == NULL ||
        input->name == NULL || input->name_length == 0u) {
        return -EINVAL;
    }
    context->calls += 1u;

    arbor_asm_result_ptr state_mem = arena_alloc_aligned(input->arena, sizeof(e2e_tx_state), 8u);
    arbor_asm_result_ptr records_mem = arena_alloc_aligned(
        input->arena, sizeof(arbor_ddd_event_record), 8u);
    arbor_asm_result_ptr payload_mem = arena_alloc_aligned(input->arena, 64u, 8u);
    if (state_mem.status != 0 || records_mem.status != 0 || payload_mem.status != 0) {
        return -ENOSPC;
    }

    arbor_ddd_event_journal journal = {0};
    arbor_status status = arbor_ddd_event_journal_init(
        (arbor_ddd_event_record *)records_mem.value, 1u,
        (uint8_t *)payload_mem.value, 64u, &journal);
    if (status.native != 0) {
        return status.native;
    }

    arbor_ddd_unit_of_work uow = {0};
    status = arbor_ddd_unit_of_work_begin(
        &context->transaction, state_mem.value, sizeof(e2e_tx_state), &journal, &uow);
    if (status.native != 0) {
        return status.native;
    }

    uint32_t event_sequence = UINT32_MAX;
    status = arbor_ddd_event_journal_append(
        &journal,
        (arbor_ddd_event_type_id){UINT64_C(0x990), UINT64_C(1)},
        1u, 0u, input->name, (uint32_t)input->name_length, &event_sequence);
    if (status.native != 0 || event_sequence != 0u) {
        (void)arbor_ddd_unit_of_work_rollback(&uow);
        return status.native != 0 ? status.native : -EIO;
    }

    status = arbor_ddd_unit_of_work_commit(&uow);
    if (status.native != 0) {
        return status.native;
    }

    static const uint8_t prefix[] = "hello ";
    arbor_asm_result_u64 total = u64_add_checked(sizeof(prefix) - 1u, input->name_length);
    if (total.status != 0) {
        return total.status;
    }
    arbor_asm_result_ptr body_mem = arena_alloc(input->arena, total.value);
    if (body_mem.status != 0) {
        return body_mem.status;
    }
    uint8_t *body = (uint8_t *)body_mem.value;
    (void)memory_copy(body, prefix, sizeof(prefix) - 1u);
    (void)memory_copy(body + sizeof(prefix) - 1u, input->name, input->name_length);

    *out = (e2e_service_output){body, total.value};
    return 0;
}

typedef struct e2e_controller_context {
    arbor_capability_binding service;
} e2e_controller_context;

static int64_t e2e_controller(
    const arbor_mvc_request *request,
    void *controller_context,
    arbor_mvc_controller_result *out)
{
    e2e_controller_context *context = (e2e_controller_context *)controller_context;
    if (arbor_mvc_request_validate(request).native != 0 || context == NULL || out == NULL ||
        request->parameter_count != 1u) {
        return -EINVAL;
    }
    const e2e_service_v1 *service = (const e2e_service_v1 *)context->service.interface_table;
    if (arbor_application_service_interface_validate(&context->service).native != 0 ||
        service == NULL || service->execute == NULL) {
        return -EINVAL;
    }
    e2e_service_input input = {
        request->scope->arena,
        request->params[0].value_ptr,
        request->params[0].value_len
    };
    e2e_service_output service_out = {NULL, 0u};
    int64_t native = service->execute(context->service.provider_context, &input, &service_out);
    if (native != 0) {
        return native;
    }
    *out = (arbor_mvc_controller_result){1u, 0u, service_out.body, service_out.body_length};
    return 0;
}

static int64_t e2e_presenter(
    const arbor_mvc_request *request,
    void *presenter_context,
    const arbor_mvc_controller_result *result,
    arbor_response_plan *out)
{
    if (arbor_mvc_request_validate(request).native != 0 || presenter_context == NULL ||
        result == NULL || out == NULL) {
        return -EINVAL;
    }
    const uint64_t flags = *(const uint64_t *)presenter_context;
    *out = (arbor_response_plan){200u, result->model_data, result->model_size, flags};
    return 0;
}

typedef struct simple_application_context {
    const uint8_t *body;
    uint64_t body_length;
    uint64_t status;
    uint64_t flags;
    int64_t native_result;
    uint64_t calls;
} simple_application_context;

static int64_t simple_application_dispatch(
    const arbor_request_scope *scope,
    void *application_context,
    arbor_response_plan *out)
{
    simple_application_context *context =
        (simple_application_context *)application_context;
    if (arbor_request_scope_validate(scope).native != 0 || context == NULL || out == NULL) {
        return -EINVAL;
    }
    context->calls += 1u;
    if (context->native_result != 0) {
        return context->native_result;
    }
    *out = (arbor_response_plan){
        context->status, context->body, context->body_length, context->flags
    };
    return 0;
}

static int make_simple_capabilities(
    simple_application_context *context,
    arbor_application_capabilities *out)
{
    return arbor_application_capabilities_make(
        simple_application_dispatch, context, out).native == 0 ? 0 : 1;
}

typedef int64_t (*mvc0_transport_dispatch_fn)(
    const arbor_asm_http_request *, arbor_asm_buffer *, arbor_asm_arena *, void *, uint64_t *);

extern arbor_asm_result_u64 application_transport_handle_once(
    arbor_asm_connection *, arbor_asm_http_request *, mvc0_transport_dispatch_fn, void *, int64_t);
extern int64_t mvc0_asm_transport_dispatch(
    const arbor_asm_http_request *, arbor_asm_buffer *, arbor_asm_arena *, void *, uint64_t *);

typedef struct mvc0_transport_asm_context {
    uint64_t calls;
    uint64_t stack_errors;
    const uint8_t *body;
    uint64_t body_length;
    uint64_t status;
    uint64_t keep_alive;
} mvc0_transport_asm_context;

static int accept_client(
    int64_t listener_fd,
    int64_t epoll_fd,
    arbor_runtime_storage *storage,
    int *client_fd_out,
    int64_t *accepted_fd_out,
    const struct sockaddr_in *address,
    socklen_t address_length)
{
    int client = socket(AF_INET, SOCK_STREAM, 0);
    if (client < 0) {
        return 1;
    }
    if (connect(client, (const struct sockaddr *)address, address_length) != 0) {
        close_if_valid(client);
        return 1;
    }
    int64_t accepted = -1;
    arbor_status status = arbor_server_accept(listener_fd, epoll_fd, storage, &accepted);
    if (status.native != 0 || accepted < 0 || accepted > INT32_MAX) {
        close_if_valid(client);
        return 1;
    }
    *client_fd_out = client;
    *accepted_fd_out = accepted;
    return 0;
}

int main(void)
{
    int result_code = 1;
    int64_t listener_fd = -1;
    int64_t epoll_fd = -1;
    int client_fd = -1;
    int64_t accepted_fd = -1;

    struct sockaddr_in address;
    (void)memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(0u);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    arbor_status status = arbor_server_open(&address, sizeof(address), 16, &listener_fd);
    if (status.native != 0 || listener_fd < 0 || listener_fd > INT32_MAX) {
        goto done;
    }
    socklen_t address_length = (socklen_t)sizeof(address);
    if (getsockname((int)listener_fd, (struct sockaddr *)&address, &address_length) != 0) {
        goto done;
    }
    status = arbor_event_loop_create(listener_fd, &epoll_fd);
    if (status.native != 0 || epoll_fd < 0 || epoll_fd > INT32_MAX) {
        goto done;
    }

    uint8_t input_bytes[131072];
    uint8_t output_bytes[131072];
    uint8_t arena_bytes[131072];
    arbor_runtime_storage storage;
    status = arbor_runtime_storage_prepare(
        &storage,
        (arbor_mut_span){input_bytes, sizeof(input_bytes)},
        (arbor_mut_span){output_bytes, sizeof(output_bytes)},
        (arbor_mut_span){arena_bytes, sizeof(arena_bytes)});
    if (status.native != 0) {
        goto done;
    }

    e2e_tx_context tx_context = {0u, 0u, 0u};
    arbor_ddd_transaction_interface tx_interface = {
        ARBOR_DDD_TRANSACTION_INTERFACE_ABI_VERSION,
        (uint32_t)sizeof(arbor_ddd_transaction_interface),
        ARBOR_DDD_TRANSACTION_FLAGS_NONE,
        {UINT64_C(0x991), UINT64_C(1)},
        sizeof(e2e_tx_state), 8u,
        e2e_tx_begin, e2e_tx_commit, e2e_tx_rollback
    };
    arbor_capability_binding tx_binding = {
        {UINT64_C(0x992), UINT64_C(1)}, {1u, 0u},
        (uint32_t)sizeof(tx_interface), 0u, ARBOR_CAPABILITY_FLAGS_NONE,
        &tx_interface, &tx_context, 0u
    };
    if (arbor_ddd_transaction_interface_validate(&tx_binding).native != 0) {
        goto done;
    }

    e2e_service_context service_context = {tx_binding, 0u};
    e2e_service_v1 service_interface = {
        {ARBOR_APPLICATION_SERVICE_INTERFACE_ABI_VERSION,
         (uint32_t)sizeof(e2e_service_v1), 0u},
        e2e_service_execute
    };
    arbor_capability_binding service_binding = {
        {UINT64_C(0x993), UINT64_C(1)}, {1u, 0u},
        (uint32_t)sizeof(service_interface), 0u, ARBOR_CAPABILITY_FLAGS_NONE,
        &service_interface, &service_context, 1u
    };
    if (arbor_application_service_interface_validate(&service_binding).native != 0) {
        goto done;
    }
    e2e_controller_context controller_context = {service_binding};

    uint64_t keep_alive_flag = ARBOR_RESPONSE_PLAN_FLAG_KEEP_ALIVE;
    uint64_t close_flag = ARBOR_RESPONSE_PLAN_FLAG_NONE;
    arbor_mvc_route routes[2] = {
        {(const uint8_t *)"GET", 3u, (const uint8_t *)"/hello/:name", 12u,
         e2e_controller, &controller_context, e2e_presenter, &keep_alive_flag, NULL, 0u},
        {(const uint8_t *)"GET", 3u, (const uint8_t *)"/close/:name", 12u,
         e2e_controller, &controller_context, e2e_presenter, &close_flag, NULL, 0u}
    };
    arbor_mvc_catalog catalog = {
        ARBOR_MVC_ABI_VERSION, (uint32_t)sizeof(arbor_mvc_catalog), 0u,
        routes, 2u, NULL, 0u
    };
    arbor_route_param validation_params[32] = {0};
    arbor_mvc_prepare_workspace workspace = {validation_params, 32u};
    arbor_mvc_application application = {0};
    status = arbor_mvc_application_prepare(&catalog, &workspace, &application);
    if (status.native != 0) {
        goto done;
    }
    arbor_application_capabilities capabilities = {0};
    status = arbor_mvc_application_capabilities_make(&application, &capabilities);
    if (status.native != 0) {
        goto done;
    }

    if (accept_client(listener_fd, epoll_fd, &storage, &client_fd, &accepted_fd,
                      &address, address_length) != 0) {
        goto done;
    }

    static const uint8_t pipelined[] =
        "GET /hello/Ada HTTP/1.1\r\nHost: local\r\n\r\n"
        "GET /hello/Bob HTTP/1.1\r\nHost: local\r\n\r\n";
    if (write(client_fd, pipelined, sizeof(pipelined) - 1u) != (ssize_t)(sizeof(pipelined) - 1u)) {
        goto done;
    }
    if (wait_readable((int)accepted_fd) != 0) {
        goto done;
    }

    uint64_t completed = 0u;
    status = arbor_application_server_step(&storage, &capabilities, epoll_fd, &completed);
    if (status.native != 0 || completed != 2u) {
        goto done;
    }

    uint8_t responses[8192];
    if (wait_readable(client_fd) != 0) {
        goto done;
    }
    bool eof = false;
    ssize_t response_bytes = drain_nonblocking(client_fd, responses, sizeof(responses), &eof);
    if (response_bytes <= 0 || eof ||
        !contains_bytes(responses, (size_t)response_bytes, (const uint8_t *)"hello Ada", 9u) ||
        !contains_bytes(responses, (size_t)response_bytes, (const uint8_t *)"hello Bob", 9u)) {
        goto done;
    }

    static const uint8_t close_request[] =
        "GET /close/Cyd HTTP/1.1\r\nHost: local\r\n\r\n";
    if (write(client_fd, close_request, sizeof(close_request) - 1u) !=
        (ssize_t)(sizeof(close_request) - 1u)) {
        goto done;
    }
    if (wait_readable((int)accepted_fd) != 0) {
        goto done;
    }
    completed = 0u;
    status = arbor_application_server_step(&storage, &capabilities, epoll_fd, &completed);
    if (status.native != 0 || completed != 3u ||
        storage.connection.state != ARBOR_ASM_CONNECTION_CLOSED) {
        goto done;
    }

    uint8_t close_response[4096];
    size_t close_used = 0u;
    bool saw_eof = false;
    for (unsigned attempt = 0u; attempt < 10u && !saw_eof; ++attempt) {
        if (wait_readable(client_fd) != 0) {
            goto done;
        }
        bool local_eof = false;
        ssize_t got = drain_nonblocking(
            client_fd, close_response + close_used,
            sizeof(close_response) - close_used, &local_eof);
        if (got < 0) {
            goto done;
        }
        close_used += (size_t)got;
        saw_eof = local_eof;
    }
    if (!saw_eof ||
        !contains_bytes(close_response, close_used, (const uint8_t *)"hello Cyd", 9u)) {
        goto done;
    }

    if (service_context.calls != 3u || tx_context.begins != 3u || tx_context.commits != 3u ||
        tx_context.rollbacks != 0u) {
        goto done;
    }

    close_if_valid(client_fd);
    client_fd = -1;
    accepted_fd = -1;

    /* Fragmented read: no dispatch until the complete HTTP frame arrives. */
    status = arbor_runtime_storage_prepare(
        &storage,
        (arbor_mut_span){input_bytes, sizeof(input_bytes)},
        (arbor_mut_span){output_bytes, sizeof(output_bytes)},
        (arbor_mut_span){arena_bytes, sizeof(arena_bytes)});
    if (status.native != 0 ||
        accept_client(listener_fd, epoll_fd, &storage, &client_fd, &accepted_fd,
                      &address, address_length) != 0) {
        goto done;
    }
    static const uint8_t fragment_body[] = "fragment-ok";
    simple_application_context fragment_context = {
        fragment_body, sizeof(fragment_body) - 1u, 200u,
        ARBOR_RESPONSE_PLAN_FLAG_NONE, 0, 0u
    };
    arbor_application_capabilities fragment_caps = {0};
    if (make_simple_capabilities(&fragment_context, &fragment_caps) != 0) {
        goto done;
    }
    static const uint8_t request_part_one[] = "GET /fragment HTTP/1.1\r\nHost:";
    static const uint8_t request_part_two[] = " local\r\n\r\n";
    if (write(client_fd, request_part_one, sizeof(request_part_one) - 1u) !=
            (ssize_t)(sizeof(request_part_one) - 1u) ||
        wait_readable((int)accepted_fd) != 0) {
        goto done;
    }
    completed = UINT64_MAX;
    status = arbor_application_server_step(&storage, &fragment_caps, epoll_fd, &completed);
    if (status.native != -EAGAIN || completed != 0u || fragment_context.calls != 0u ||
        storage.connection.state != ARBOR_ASM_CONNECTION_READING) {
        goto done;
    }
    if (write(client_fd, request_part_two, sizeof(request_part_two) - 1u) !=
            (ssize_t)(sizeof(request_part_two) - 1u) ||
        wait_readable((int)accepted_fd) != 0) {
        goto done;
    }
    completed = 0u;
    status = arbor_application_server_step(&storage, &fragment_caps, epoll_fd, &completed);
    if (status.native != 0 || completed != 1u || fragment_context.calls != 1u ||
        storage.connection.state != ARBOR_ASM_CONNECTION_CLOSED) {
        goto done;
    }
    uint8_t fragment_response[4096];
    size_t fragment_used = 0u;
    saw_eof = false;
    for (unsigned attempt = 0u; attempt < 10u && !saw_eof; ++attempt) {
        if (wait_readable(client_fd) != 0) {
            goto done;
        }
        bool local_eof = false;
        ssize_t got = drain_nonblocking(
            client_fd, fragment_response + fragment_used,
            sizeof(fragment_response) - fragment_used, &local_eof);
        if (got < 0) {
            goto done;
        }
        fragment_used += (size_t)got;
        saw_eof = local_eof;
    }
    if (!saw_eof ||
        !contains_bytes(fragment_response, fragment_used, fragment_body,
                        sizeof(fragment_body) - 1u)) {
        goto done;
    }
    close_if_valid(client_fd);
    client_fd = -1;
    accepted_fd = -1;

    /* Force a nonblocking partial write/EAGAIN and verify WRITING-state resume
     * preserves the rich transport keep-alive decision. */
    status = arbor_runtime_storage_prepare(
        &storage,
        (arbor_mut_span){input_bytes, sizeof(input_bytes)},
        (arbor_mut_span){output_bytes, sizeof(output_bytes)},
        (arbor_mut_span){arena_bytes, sizeof(arena_bytes)});
    if (status.native != 0 ||
        accept_client(listener_fd, epoll_fd, &storage, &client_fd, &accepted_fd,
                      &address, address_length) != 0) {
        goto done;
    }
    int small_buffer = 4096;
    if (setsockopt((int)accepted_fd, SOL_SOCKET, SO_SNDBUF,
                   &small_buffer, (socklen_t)sizeof(small_buffer)) != 0 ||
        setsockopt(client_fd, SOL_SOCKET, SO_RCVBUF,
                   &small_buffer, (socklen_t)sizeof(small_buffer)) != 0) {
        goto done;
    }
    uint8_t large_body[120000];
    (void)memset(large_body, 'L', sizeof(large_body));
    simple_application_context large_context = {
        large_body, sizeof(large_body), 200u,
        ARBOR_RESPONSE_PLAN_FLAG_KEEP_ALIVE, 0, 0u
    };
    arbor_application_capabilities large_caps = {0};
    if (make_simple_capabilities(&large_context, &large_caps) != 0) {
        goto done;
    }
    static const uint8_t large_request[] =
        "GET /large HTTP/1.1\r\nHost: local\r\n\r\n";
    if (write(client_fd, large_request, sizeof(large_request) - 1u) !=
            (ssize_t)(sizeof(large_request) - 1u) ||
        wait_readable((int)accepted_fd) != 0) {
        goto done;
    }
    completed = 0u;
    status = arbor_application_server_step(&storage, &large_caps, epoll_fd, &completed);
    if (status.native != -EAGAIN || completed != 0u || large_context.calls != 1u ||
        storage.connection.state != ARBOR_ASM_CONNECTION_WRITING) {
        goto done;
    }
    uint8_t large_response[131072];
    size_t large_used = 0u;
    bool large_completed = false;
    for (unsigned attempt = 0u; attempt < 256u && !large_completed; ++attempt) {
        if (wait_readable(client_fd) == 0 && large_used < sizeof(large_response)) {
            bool local_eof = false;
            ssize_t got = drain_nonblocking(
                client_fd, large_response + large_used,
                sizeof(large_response) - large_used, &local_eof);
            if (got < 0 || local_eof) {
                goto done;
            }
            large_used += (size_t)got;
        }
        completed = 0u;
        status = arbor_application_server_step(&storage, &large_caps, epoll_fd, &completed);
        if (status.native == -EAGAIN) {
            if (storage.connection.state != ARBOR_ASM_CONNECTION_WRITING || completed != 0u) {
                goto done;
            }
            continue;
        }
        if (status.native == 0 && completed == 1u &&
            storage.connection.state == ARBOR_ASM_CONNECTION_READING) {
            large_completed = true;
            break;
        }
        goto done;
    }
    if (!large_completed || large_context.calls != 1u) {
        goto done;
    }
    for (unsigned attempt = 0u; attempt < 16u && large_used < sizeof(large_response); ++attempt) {
        if (wait_readable(client_fd) != 0) {
            break;
        }
        bool local_eof = false;
        ssize_t got = drain_nonblocking(
            client_fd, large_response + large_used,
            sizeof(large_response) - large_used, &local_eof);
        if (got < 0 || local_eof) {
            goto done;
        }
        if (got == 0) {
            break;
        }
        large_used += (size_t)got;
    }
    static const uint8_t large_length_header[] = "Content-Length: 120000";
    if (!contains_bytes(large_response, large_used, large_length_header,
                        sizeof(large_length_header) - 1u)) {
        goto done;
    }
    status = arbor_server_close(epoll_fd, &storage);
    if (status.native != 0 || storage.connection.state != ARBOR_ASM_CONNECTION_CLOSED) {
        goto done;
    }
    close_if_valid(client_fd);
    client_fd = -1;
    accepted_fd = -1;

    /* Rich transport rejects response bodies that alias output metadata. */
    status = arbor_runtime_storage_prepare(
        &storage,
        (arbor_mut_span){input_bytes, sizeof(input_bytes)},
        (arbor_mut_span){output_bytes, sizeof(output_bytes)},
        (arbor_mut_span){arena_bytes, sizeof(arena_bytes)});
    if (status.native != 0 ||
        accept_client(listener_fd, epoll_fd, &storage, &client_fd, &accepted_fd,
                      &address, address_length) != 0) {
        goto done;
    }
    simple_application_context overlap_context = {
        (const uint8_t *)&storage.output, sizeof(storage.output), 200u,
        ARBOR_RESPONSE_PLAN_FLAG_NONE, 0, 0u
    };
    arbor_application_capabilities overlap_caps = {0};
    if (make_simple_capabilities(&overlap_context, &overlap_caps) != 0) {
        goto done;
    }
    static const uint8_t overlap_request[] =
        "GET /overlap HTTP/1.1\r\nHost: local\r\n\r\n";
    if (write(client_fd, overlap_request, sizeof(overlap_request) - 1u) !=
            (ssize_t)(sizeof(overlap_request) - 1u) ||
        wait_readable((int)accepted_fd) != 0) {
        goto done;
    }
    completed = 0u;
    status = arbor_application_server_step(&storage, &overlap_caps, epoll_fd, &completed);
    if (status.native != -EINVAL || completed != 0u || overlap_context.calls != 1u ||
        storage.connection.state != ARBOR_ASM_CONNECTION_CLOSING) {
        goto done;
    }
    if (arbor_server_close(epoll_fd, &storage).native != 0) {
        goto done;
    }
    close_if_valid(client_fd);
    client_fd = -1;
    accepted_fd = -1;

    /* Machine-detectable Application-context overlap with request storage is
     * rejected before dispatch. */
    status = arbor_runtime_storage_prepare(
        &storage,
        (arbor_mut_span){input_bytes, sizeof(input_bytes)},
        (arbor_mut_span){output_bytes, sizeof(output_bytes)},
        (arbor_mut_span){arena_bytes, sizeof(arena_bytes)});
    if (status.native != 0 ||
        accept_client(listener_fd, epoll_fd, &storage, &client_fd, &accepted_fd,
                      &address, address_length) != 0) {
        goto done;
    }
    arbor_application_capabilities bad_context_caps = {0};
    if (arbor_application_capabilities_make(
            simple_application_dispatch, arena_bytes, &bad_context_caps).native != 0) {
        goto done;
    }
    completed = 77u;
    status = arbor_application_server_step(
        &storage, &bad_context_caps, epoll_fd, &completed);
    if (status.native != -EINVAL || completed != 77u ||
        storage.connection.state != ARBOR_ASM_CONNECTION_READING) {
        goto done;
    }
    if (arbor_server_close(epoll_fd, &storage).native != 0) {
        goto done;
    }
    close_if_valid(client_fd);
    client_fd = -1;
    accepted_fd = -1;

    /* Completed-count output may not alias transport storage. */
    status = arbor_runtime_storage_prepare(
        &storage,
        (arbor_mut_span){input_bytes, sizeof(input_bytes)},
        (arbor_mut_span){output_bytes, sizeof(output_bytes)},
        (arbor_mut_span){arena_bytes, sizeof(arena_bytes)});
    if (status.native != 0 ||
        accept_client(listener_fd, epoll_fd, &storage, &client_fd, &accepted_fd,
                      &address, address_length) != 0) {
        goto done;
    }
    simple_application_context idle_context = {NULL, 0u, 204u, 0u, 0, 0u};
    arbor_application_capabilities idle_caps = {0};
    if (make_simple_capabilities(&idle_context, &idle_caps) != 0) {
        goto done;
    }
    uint64_t count_before = storage.connection.request_count;
    status = arbor_application_server_step(
        &storage, &idle_caps, epoll_fd, &storage.connection.request_count);
    if (status.native != -EINVAL || storage.connection.request_count != count_before ||
        idle_context.calls != 0u) {
        goto done;
    }
    if (arbor_server_close(epoll_fd, &storage).native != 0) {
        goto done;
    }
    close_if_valid(client_fd);
    client_fd = -1;
    accepted_fd = -1;

    /* Application callback mechanism failure follows the existing connection
     * error/closing semantics and publishes no response. */
    status = arbor_runtime_storage_prepare(
        &storage,
        (arbor_mut_span){input_bytes, sizeof(input_bytes)},
        (arbor_mut_span){output_bytes, sizeof(output_bytes)},
        (arbor_mut_span){arena_bytes, sizeof(arena_bytes)});
    if (status.native != 0 ||
        accept_client(listener_fd, epoll_fd, &storage, &client_fd, &accepted_fd,
                      &address, address_length) != 0) {
        goto done;
    }
    simple_application_context failure_context = {NULL, 0u, 500u, 0u, -EIO, 0u};
    arbor_application_capabilities failure_caps = {0};
    if (make_simple_capabilities(&failure_context, &failure_caps) != 0) {
        goto done;
    }
    static const uint8_t failure_request[] =
        "GET /failure HTTP/1.1\r\nHost: local\r\n\r\n";
    if (write(client_fd, failure_request, sizeof(failure_request) - 1u) !=
            (ssize_t)(sizeof(failure_request) - 1u) ||
        wait_readable((int)accepted_fd) != 0) {
        goto done;
    }
    completed = 0u;
    status = arbor_application_server_step(&storage, &failure_caps, epoll_fd, &completed);
    if (status.native != -EIO || completed != 0u || failure_context.calls != 1u ||
        storage.connection.state != ARBOR_ASM_CONNECTION_CLOSING ||
        storage.connection.last_error != -EIO) {
        goto done;
    }
    if (arbor_server_close(epoll_fd, &storage).native != 0) {
        goto done;
    }
    close_if_valid(client_fd);
    client_fd = -1;
    accepted_fd = -1;

    /* Direct rich-transport call with a handwritten Assembly dispatch callback. */
    status = arbor_runtime_storage_prepare(
        &storage,
        (arbor_mut_span){input_bytes, sizeof(input_bytes)},
        (arbor_mut_span){output_bytes, sizeof(output_bytes)},
        (arbor_mut_span){arena_bytes, sizeof(arena_bytes)});
    if (status.native != 0) {
        goto done;
    }
    if (accept_client(listener_fd, epoll_fd, &storage, &client_fd, &accepted_fd,
                      &address, address_length) != 0) {
        goto done;
    }
    static const uint8_t abi_request[] =
        "GET /abi HTTP/1.1\r\nHost: local\r\n\r\n";
    if (write(client_fd, abi_request, sizeof(abi_request) - 1u) !=
        (ssize_t)(sizeof(abi_request) - 1u) || wait_readable((int)accepted_fd) != 0) {
        goto done;
    }
    static const uint8_t abi_body[] = "asm-transport";
    mvc0_transport_asm_context asm_context = {
        0u, 0u, abi_body, sizeof(abi_body) - 1u, 200u, 0u
    };
    arbor_asm_result_u64 asm_result = application_transport_handle_once(
        &storage.connection, &storage.request, mvc0_asm_transport_dispatch,
        &asm_context, epoll_fd);
    if (asm_result.status != 0 || asm_result.value != 1u ||
        asm_context.calls != 1u || asm_context.stack_errors != 0u ||
        storage.connection.state != ARBOR_ASM_CONNECTION_CLOSED) {
        goto done;
    }
    uint8_t abi_response[4096];
    size_t abi_used = 0u;
    saw_eof = false;
    for (unsigned attempt = 0u; attempt < 10u && !saw_eof; ++attempt) {
        if (wait_readable(client_fd) != 0) {
            goto done;
        }
        bool local_eof = false;
        ssize_t got = drain_nonblocking(
            client_fd, abi_response + abi_used, sizeof(abi_response) - abi_used, &local_eof);
        if (got < 0) {
            goto done;
        }
        abi_used += (size_t)got;
        saw_eof = local_eof;
    }
    if (!saw_eof ||
        !contains_bytes(abi_response, abi_used, abi_body, sizeof(abi_body) - 1u)) {
        goto done;
    }

    result_code = 0;

done:
    close_if_valid(client_fd);
    if (accepted_fd >= 0 && storage.connection.state != ARBOR_ASM_CONNECTION_CLOSED && epoll_fd >= 0) {
        (void)arbor_server_close(epoll_fd, &storage);
    }
    if (epoll_fd >= 0) {
        close_if_valid((int)epoll_fd);
    }
    if (listener_fd >= 0) {
        close_if_valid((int)listener_fd);
    }

    if (result_code == 0) {
        puts("PASS: MVC0 rich transport sockets, fragmentation, EAGAIN resume, alias/failure hardening and Assembly callback ABI");
    }
    return result_code;
}
