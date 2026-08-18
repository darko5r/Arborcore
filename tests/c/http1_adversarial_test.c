#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <arborcore/http_mvc.h>

static int fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    return 1;
}

typedef struct mode_context {
    uint32_t mode;
    uint64_t calls;
    uint64_t *mark_target;
} mode_context;

static int64_t alternate_dispatch(
    const arbor_request_scope *scope,
    void *context,
    arbor_response_plan *out)
{
    (void)scope; (void)context; (void)out;
    return 0;
}

static int64_t controller(
    const arbor_mvc_request *request,
    void *context,
    arbor_mvc_controller_result *out)
{
    (void)context;
    if (arbor_mvc_request_validate(request).native != 0 || out == NULL) return -EINVAL;
    *out = (arbor_mvc_controller_result){1u, 0u, NULL, 0u};
    return 0;
}

static int64_t presenter(
    const arbor_mvc_request *request,
    void *context,
    const arbor_mvc_controller_result *result,
    arbor_response_plan *out)
{
    (void)result;
    mode_context *mode = (mode_context *)context;
    if (request == NULL || mode == NULL || out == NULL) return -EINVAL;
    mode->calls += 1u;
    static const uint8_t good_name[] = "Content-Type";
    static const uint8_t good_value[] = "text/plain";
    static const uint8_t reserved_name[] = "Content-Length";
    static const uint8_t reserved_value[] = "1";
    arbor_status status;
    if (mode->mode == 1u) {
        status = arbor_http_mvc_response_field_append(
            request,
            (arbor_span){reserved_name, sizeof(reserved_name) - 1u},
            (arbor_span){reserved_value, sizeof(reserved_value) - 1u});
    } else if (mode->mode == 2u) {
        uint64_t *aliased_mark =
            (uint64_t *)(uintptr_t)request->scope->request->headers_ptr;
        status = arbor_http_mvc_response_fields_mark(request, aliased_mark);
    } else if (mode->mode == 3u) {
        if (mode->mark_target == NULL) return -EINVAL;
        status = arbor_http_mvc_response_fields_mark(request, mode->mark_target);
    } else {
        status = arbor_http_mvc_response_field_append(
            request,
            (arbor_span){good_name, sizeof(good_name) - 1u},
            (arbor_span){good_value, sizeof(good_value) - 1u});
    }
    if (status.native != 0) return status.native;
    *out = (arbor_response_plan){200u, NULL, 0u, ARBOR_RESPONSE_PLAN_FLAG_KEEP_ALIVE};
    return 0;
}

static int prepare_mvc(mode_context *mode, arbor_mvc_application *mvc)
{
    static const uint8_t method[] = "GET";
    static const uint8_t pattern[] = "/x";
    static arbor_mvc_route route;
    static arbor_mvc_catalog catalog;
    static arbor_route_param params[4];
    route = (arbor_mvc_route){
        method, sizeof(method) - 1u,
        pattern, sizeof(pattern) - 1u,
        controller, NULL, presenter, mode, NULL, 0u
    };
    catalog = (arbor_mvc_catalog){
        ARBOR_MVC_ABI_VERSION, (uint32_t)sizeof(arbor_mvc_catalog),
        ARBOR_MVC_CATALOG_FLAGS_NONE, &route, 1u, NULL, 0u
    };
    arbor_mvc_prepare_workspace workspace = {params, 4u};
    return arbor_mvc_application_prepare(&catalog, &workspace, mvc).native == 0 ? 0 : 1;
}

static int setup_connection(
    arbor_runtime_storage *storage,
    uint8_t *input, size_t input_size,
    uint8_t *output, size_t output_size,
    uint8_t *arena, size_t arena_size,
    int sv[2], int *epfd_out)
{
    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0, sv) != 0) return 1;
    int epfd = epoll_create1(EPOLL_CLOEXEC);
    if (epfd < 0) { close(sv[0]); close(sv[1]); return 1; }
    if (arbor_runtime_storage_prepare(
            storage,
            (arbor_mut_span){input, (uint64_t)input_size},
            (arbor_mut_span){output, (uint64_t)output_size},
            (arbor_mut_span){arena, (uint64_t)arena_size}).native != 0) {
        close(epfd); close(sv[0]); close(sv[1]); return 1;
    }
    if (connection_init(&storage->connection, sv[1], &storage->input, &storage->output,
                        &storage->arena).status != 0 ||
        connection_transition(&storage->connection, ARBOR_ASM_CONNECTION_READING).status != 0) {
        close(epfd); close(sv[0]); close(sv[1]); return 1;
    }
    struct epoll_event event = {0};
    event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP;
    event.data.ptr = &storage->connection;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, sv[1], &event) != 0) {
        close(epfd); close(sv[0]); close(sv[1]); return 1;
    }
    *epfd_out = epfd;
    return 0;
}

static void cleanup(arbor_runtime_storage *storage, int sv[2], int epfd)
{
    if (sv[1] >= 0 && storage->connection.state != ARBOR_ASM_CONNECTION_CLOSED) {
        (void)arbor_server_close(epfd, storage);
    }
    if (sv[0] >= 0) close(sv[0]);
    if (epfd >= 0) close(epfd);
}

int main(void)
{
    mode_context mode = {0u, 0u, NULL};
    arbor_mvc_application mvc = {0};
    if (prepare_mvc(&mode, &mvc) != 0) return fail("MVC preparation");

    arbor_http_mvc_requirements sentinel = {11u, 22u};
    arbor_status status = arbor_http_mvc_application_measure(UINT64_MAX, &sentinel);
    if (status.native != -EOVERFLOW || sentinel.arena_prefix_bytes != 11u ||
        sentinel.response_field_capacity != 22u) {
        return fail("HTTP1 measure overflow transactional");
    }

    arbor_http_mvc_application app = {0};
    if (arbor_http_mvc_application_prepare(&mvc, 1u, &app).native != 0) {
        return fail("HTTP1 app preparation");
    }
    arbor_http_mvc_application bad = app;
    bad.response_field_capacity += 1u;
    if (arbor_http_mvc_application_validate(&bad).native != -EINVAL) {
        return fail("HTTP1 prepared metadata corruption rejection");
    }
    bad = app;
    bad.mvc_capabilities.request_dispatch = alternate_dispatch;
    if (arbor_http_mvc_application_validate(&bad).native != -EINVAL) {
        return fail("HTTP1 prepared capability-table guard rejection");
    }

    /* Too-small request arena is rejected on an otherwise valid live transport,
     * before HTTP1 invokes the transport engine or mutates connection/request state. */
    uint8_t input_small[256] = {0};
    uint8_t output_small[256] = {0};
    uint8_t arena_small[32] = {0};
    arbor_runtime_storage storage = {0};
    int small_sv[2] = {-1, -1};
    int small_epfd = -1;
    if (setup_connection(&storage, input_small, sizeof(input_small),
                         output_small, sizeof(output_small),
                         arena_small, sizeof(arena_small),
                         small_sv, &small_epfd) != 0) {
        return fail("small live transport setup");
    }
    const uint64_t small_request_count_before = storage.connection.request_count;
    const uint64_t small_input_length_before = storage.input.length;
    const uint64_t small_output_length_before = storage.output.length;
    const uint64_t small_arena_offset_before = storage.arena.offset;
    uint64_t completed = 91u;
    status = arbor_http_mvc_server_step(&storage, &app, small_epfd, &completed);
    if (status.native != -ENOSPC || completed != 91u ||
        storage.connection.state != ARBOR_ASM_CONNECTION_READING ||
        storage.connection.request_count != small_request_count_before ||
        storage.input.length != small_input_length_before ||
        storage.output.length != small_output_length_before ||
        storage.arena.offset != small_arena_offset_before) {
        cleanup(&storage, small_sv, small_epfd);
        return fail("HTTP1 arena prefix capacity rejection");
    }
    cleanup(&storage, small_sv, small_epfd);

    /* Application object may not alias transport storage. Establish a valid
     * transport first so generic connection validation cannot mask the alias rule. */
    uint8_t enough_arena[512] = {0};
    int alias_sv[2] = {-1, -1};
    int alias_epfd = -1;
    if (setup_connection(&storage, input_small, sizeof(input_small),
                         output_small, sizeof(output_small),
                         enough_arena, sizeof(enough_arena),
                         alias_sv, &alias_epfd) != 0) {
        return fail("alias live transport setup");
    }
    arbor_http_mvc_application *aliased = (arbor_http_mvc_application *)(void *)enough_arena;
    *aliased = app;
    const uint64_t alias_request_count_before = storage.connection.request_count;
    completed = 92u;
    status = arbor_http_mvc_server_step(&storage, aliased, alias_epfd, &completed);
    if (status.native != -EINVAL || completed != 92u ||
        storage.connection.state != ARBOR_ASM_CONNECTION_READING ||
        storage.connection.request_count != alias_request_count_before) {
        cleanup(&storage, alias_sv, alias_epfd);
        return fail("HTTP1 application/transport alias rejection");
    }
    cleanup(&storage, alias_sv, alias_epfd);

    /* Completed-request output may not overwrite the separately stored MVC app.
     * Again start from a valid transport so this tests the intended alias predicate. */
    int count_sv[2] = {-1, -1};
    int count_epfd = -1;
    if (setup_connection(&storage, input_small, sizeof(input_small),
                         output_small, sizeof(output_small),
                         enough_arena, sizeof(enough_arena),
                         count_sv, &count_epfd) != 0) {
        return fail("completion-alias live transport setup");
    }
    uint64_t *mvc_alias_count = &mvc.max_route_parameter_count;
    const uint64_t mvc_alias_before = *mvc_alias_count;
    const uint64_t count_request_count_before = storage.connection.request_count;
    status = arbor_http_mvc_server_step(&storage, &app, count_epfd, mvc_alias_count);
    if (status.native != -EINVAL || *mvc_alias_count != mvc_alias_before ||
        storage.connection.state != ARBOR_ASM_CONNECTION_READING ||
        storage.connection.request_count != count_request_count_before) {
        cleanup(&storage, count_sv, count_epfd);
        return fail("HTTP1 completion-output/MVC application alias rejection");
    }
    cleanup(&storage, count_sv, count_epfd);

    /* Zero response-field capacity makes an otherwise valid presenter append fail ENOSPC. */
    arbor_http_mvc_application zero_fields = {0};
    if (arbor_http_mvc_application_prepare(&mvc, 0u, &zero_fields).native != 0) {
        return fail("zero-field HTTP1 app preparation");
    }
    uint8_t input[1024] = {0}, output[1024] = {0}, arena[1024] = {0};
    int sv[2] = {-1, -1}; int epfd = -1;
    if (setup_connection(&storage, input, sizeof(input), output, sizeof(output),
                         arena, sizeof(arena), sv, &epfd) != 0) {
        return fail("zero-field socket setup");
    }
    static const char req[] = "GET /x HTTP/1.1\r\nHost: local\r\n\r\n";
    if (write(sv[0], req, sizeof(req) - 1u) != (ssize_t)(sizeof(req) - 1u)) {
        cleanup(&storage, sv, epfd); return fail("zero-field request write");
    }
    completed = 0u;
    status = arbor_http_mvc_server_step(&storage, &zero_fields, epfd, &completed);
    if (status.native != -ENOSPC || completed != 0u || mode.calls != 1u ||
        storage.connection.state != ARBOR_ASM_CONNECTION_CLOSING) {
        cleanup(&storage, sv, epfd); return fail("HTTP1 field-capacity failure propagation");
    }
    cleanup(&storage, sv, epfd);

    /* Reserved framing field is rejected by the sidecar before final serialization. */
    mode.mode = 1u;
    mode.calls = 0u;
    if (setup_connection(&storage, input, sizeof(input), output, sizeof(output),
                         arena, sizeof(arena), sv, &epfd) != 0) {
        return fail("reserved-field socket setup");
    }
    if (write(sv[0], req, sizeof(req) - 1u) != (ssize_t)(sizeof(req) - 1u)) {
        cleanup(&storage, sv, epfd); return fail("reserved-field request write");
    }
    completed = 0u;
    status = arbor_http_mvc_server_step(&storage, &app, epfd, &completed);
    if (status.native != -EINVAL || completed != 0u || mode.calls != 1u ||
        storage.connection.state != ARBOR_ASM_CONNECTION_CLOSING) {
        cleanup(&storage, sv, epfd); return fail("HTTP1 reserved framing field rejection");
    }
    cleanup(&storage, sv, epfd);

    /* Mark output cannot overwrite borrowed request bytes. */
    mode.mode = 2u;
    mode.calls = 0u;
    if (setup_connection(&storage, input, sizeof(input), output, sizeof(output),
                         arena, sizeof(arena), sv, &epfd) != 0) {
        return fail("mark-alias socket setup");
    }
    if (write(sv[0], req, sizeof(req) - 1u) != (ssize_t)(sizeof(req) - 1u)) {
        cleanup(&storage, sv, epfd); return fail("mark-alias request write");
    }
    completed = 0u;
    status = arbor_http_mvc_server_step(&storage, &app, epfd, &completed);
    if (status.native != -EINVAL || completed != 0u || mode.calls != 1u ||
        storage.connection.state != ARBOR_ASM_CONNECTION_CLOSING) {
        cleanup(&storage, sv, epfd);
        return fail("HTTP1 mark-output/request-bytes alias rejection");
    }
    cleanup(&storage, sv, epfd);

    /* Mark output cannot overwrite the prepared HTTP1 application. */
    mode.mode = 3u;
    mode.calls = 0u;
    mode.mark_target = &app.prepared_guard;
    const uint64_t http1_guard_before = app.prepared_guard;
    if (setup_connection(&storage, input, sizeof(input), output, sizeof(output),
                         arena, sizeof(arena), sv, &epfd) != 0) {
        return fail("mark/HTTP1-application alias socket setup");
    }
    if (write(sv[0], req, sizeof(req) - 1u) != (ssize_t)(sizeof(req) - 1u)) {
        cleanup(&storage, sv, epfd); return fail("mark/HTTP1-application alias request write");
    }
    completed = 0u;
    status = arbor_http_mvc_server_step(&storage, &app, epfd, &completed);
    if (status.native != -EINVAL || completed != 0u || mode.calls != 1u ||
        app.prepared_guard != http1_guard_before ||
        storage.connection.state != ARBOR_ASM_CONNECTION_CLOSING) {
        cleanup(&storage, sv, epfd);
        return fail("HTTP1 mark-output/prepared HTTP1 application alias rejection");
    }
    cleanup(&storage, sv, epfd);

    /* Mark output cannot overwrite the prepared MVC application. */
    mode.calls = 0u;
    mode.mark_target = &mvc.max_route_parameter_count_guard;
    const uint64_t mvc_guard_before = mvc.max_route_parameter_count_guard;
    if (setup_connection(&storage, input, sizeof(input), output, sizeof(output),
                         arena, sizeof(arena), sv, &epfd) != 0) {
        return fail("mark/MVC-application alias socket setup");
    }
    if (write(sv[0], req, sizeof(req) - 1u) != (ssize_t)(sizeof(req) - 1u)) {
        cleanup(&storage, sv, epfd); return fail("mark/MVC-application alias request write");
    }
    completed = 0u;
    status = arbor_http_mvc_server_step(&storage, &app, epfd, &completed);
    if (status.native != -EINVAL || completed != 0u || mode.calls != 1u ||
        mvc.max_route_parameter_count_guard != mvc_guard_before ||
        storage.connection.state != ARBOR_ASM_CONNECTION_CLOSING) {
        cleanup(&storage, sv, epfd);
        return fail("HTTP1 mark-output/prepared MVC application alias rejection");
    }
    cleanup(&storage, sv, epfd);

    /* Mark output cannot overwrite immutable MVC catalog structure. */
    mode.calls = 0u;
    mode.mark_target = (uint64_t *)(uintptr_t)&mvc.catalog->route_count;
    const uint64_t route_count_before = mvc.catalog->route_count;
    if (setup_connection(&storage, input, sizeof(input), output, sizeof(output),
                         arena, sizeof(arena), sv, &epfd) != 0) {
        return fail("mark/catalog alias socket setup");
    }
    if (write(sv[0], req, sizeof(req) - 1u) != (ssize_t)(sizeof(req) - 1u)) {
        cleanup(&storage, sv, epfd); return fail("mark/catalog alias request write");
    }
    completed = 0u;
    status = arbor_http_mvc_server_step(&storage, &app, epfd, &completed);
    if (status.native != -EINVAL || completed != 0u || mode.calls != 1u ||
        mvc.catalog->route_count != route_count_before ||
        storage.connection.state != ARBOR_ASM_CONNECTION_CLOSING) {
        cleanup(&storage, sv, epfd);
        return fail("HTTP1 mark-output/MVC catalog alias rejection");
    }
    cleanup(&storage, sv, epfd);
    mode.mark_target = NULL;

    puts("PASS: HTTP1 adversarial preparation, capacity, alias and reserved-field policy");
    return 0;
}
