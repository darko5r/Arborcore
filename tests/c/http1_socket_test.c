#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
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

static size_t count_bytes(
    const uint8_t *haystack, size_t haystack_length,
    const uint8_t *needle, size_t needle_length)
{
    size_t count = 0u;
    if (needle_length == 0u || needle_length > haystack_length) return 0u;
    for (size_t i = 0u; i + needle_length <= haystack_length; ++i) {
        if (memcmp(haystack + i, needle, needle_length) == 0) count += 1u;
    }
    return count;
}

static bool contains_bytes(
    const uint8_t *haystack, size_t haystack_length,
    const uint8_t *needle, size_t needle_length)
{
    return count_bytes(haystack, haystack_length, needle, needle_length) != 0u;
}

typedef struct response_context {
    uint64_t calls;
    uint64_t status;
    const uint8_t *body;
    uint64_t body_length;
    uint64_t flags;
    bool add_content_type;
} response_context;

static int64_t controller(
    const arbor_mvc_request *request,
    void *context,
    arbor_mvc_controller_result *out)
{
    response_context *response = (response_context *)context;
    if (arbor_mvc_request_validate(request).native != 0 || response == NULL || out == NULL) {
        return -EINVAL;
    }
    *out = (arbor_mvc_controller_result){1u, 0u, response->body, response->body_length};
    return 0;
}

static int64_t presenter(
    const arbor_mvc_request *request,
    void *context,
    const arbor_mvc_controller_result *result,
    arbor_response_plan *out)
{
    response_context *response = (response_context *)context;
    if (request == NULL || response == NULL || result == NULL || out == NULL) return -EINVAL;
    response->calls += 1u;
    if (response->add_content_type) {
        static const uint8_t name[] = "Content-Type";
        static const uint8_t value[] = "text/plain; charset=utf-8";
        arbor_status status = arbor_http_mvc_response_field_append(
            request,
            (arbor_span){name, sizeof(name) - 1u},
            (arbor_span){value, sizeof(value) - 1u});
        if (status.native != 0) return status.native;
    }
    *out = (arbor_response_plan){
        response->status, result->model_data, result->model_size, response->flags
    };
    return 0;
}

typedef struct fixture {
    response_context response;
    arbor_mvc_route route;
    arbor_mvc_catalog catalog;
    arbor_route_param workspace_params[8];
    arbor_mvc_prepare_workspace workspace;
    arbor_mvc_application mvc;
    arbor_http_mvc_application app;
} fixture;

static int fixture_prepare_method(
    fixture *f,
    uint64_t field_capacity,
    const uint8_t *method,
    uint64_t method_length)
{
    static const uint8_t pattern[] = "/x";
    f->route = (arbor_mvc_route){
        method, method_length,
        pattern, sizeof(pattern) - 1u,
        controller, &f->response, presenter, &f->response, NULL, 0u
    };
    f->catalog = (arbor_mvc_catalog){
        ARBOR_MVC_ABI_VERSION, (uint32_t)sizeof(arbor_mvc_catalog),
        ARBOR_MVC_CATALOG_FLAGS_NONE, &f->route, 1u, NULL, 0u
    };
    f->workspace = (arbor_mvc_prepare_workspace){f->workspace_params, 8u};
    if (arbor_mvc_application_prepare(&f->catalog, &f->workspace, &f->mvc).native != 0) return 1;
    return arbor_http_mvc_application_prepare(&f->mvc, field_capacity, &f->app).native == 0 ? 0 : 1;
}

static int fixture_prepare(fixture *f, uint64_t field_capacity)
{
    static const uint8_t method[] = "GET";
    return fixture_prepare_method(f, field_capacity, method, sizeof(method) - 1u);
}

typedef struct connection_fixture {
    arbor_runtime_storage storage;
    int sv[2];
    int epfd;
} connection_fixture;

static int connection_prepare(
    connection_fixture *c,
    uint8_t *input, uint64_t input_size,
    uint8_t *output, uint64_t output_size,
    uint8_t *arena, uint64_t arena_size,
    int send_buffer)
{
    c->sv[0] = -1; c->sv[1] = -1; c->epfd = -1;
    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0, c->sv) != 0) return 1;
    if (send_buffer > 0 && setsockopt(c->sv[1], SOL_SOCKET, SO_SNDBUF,
                                      &send_buffer, sizeof(send_buffer)) != 0) return 1;
    c->epfd = epoll_create1(EPOLL_CLOEXEC);
    if (c->epfd < 0) return 1;
    if (arbor_runtime_storage_prepare(
            &c->storage,
            (arbor_mut_span){input, input_size},
            (arbor_mut_span){output, output_size},
            (arbor_mut_span){arena, arena_size}).native != 0) return 1;
    if (connection_init(&c->storage.connection, c->sv[1], &c->storage.input,
                        &c->storage.output, &c->storage.arena).status != 0 ||
        connection_transition(&c->storage.connection, ARBOR_ASM_CONNECTION_READING).status != 0) return 1;
    struct epoll_event event = {0};
    event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP;
    event.data.ptr = &c->storage.connection;
    if (epoll_ctl(c->epfd, EPOLL_CTL_ADD, c->sv[1], &event) != 0) return 1;
    return 0;
}

static void connection_cleanup(connection_fixture *c)
{
    if (c->sv[1] >= 0 && c->storage.connection.state != ARBOR_ASM_CONNECTION_CLOSED && c->epfd >= 0) {
        (void)arbor_server_close(c->epfd, &c->storage);
    }
    if (c->sv[0] >= 0) close(c->sv[0]);
    if (c->epfd >= 0) close(c->epfd);
    c->sv[0] = -1; c->sv[1] = -1; c->epfd = -1;
}

static ssize_t drain(int fd, uint8_t *buffer, size_t capacity)
{
    size_t used = 0u;
    while (used < capacity) {
        ssize_t got = read(fd, buffer + used, capacity - used);
        if (got > 0) { used += (size_t)got; continue; }
        if (got == 0) break;
        if (errno == EAGAIN || errno == EWOULDBLOCK) break;
        return -1;
    }
    return (ssize_t)used;
}

int main(void)
{
    fixture f = {0};
    static const uint8_t hello[] = "hello";
    f.response = (response_context){0u, 200u, hello, sizeof(hello) - 1u,
                                    ARBOR_RESPONSE_PLAN_FLAG_KEEP_ALIVE, true};
    if (fixture_prepare(&f, 4u) != 0) return fail("HTTP1 socket fixture preparation");

    uint8_t input[262144] = {0};
    uint8_t output[262144] = {0};
    uint8_t arena[65536] = {0};
    connection_fixture c = {0};

    /* Missing Host is a protocol 400, closes, and never enters MVC. */
    if (connection_prepare(&c, input, sizeof(input), output, sizeof(output),
                           arena, sizeof(arena), 0) != 0) return fail("missing-Host setup");
    static const char missing_host[] = "GET /x HTTP/1.1\r\n\r\n";
    if (write(c.sv[0], missing_host, sizeof(missing_host) - 1u) != (ssize_t)(sizeof(missing_host) - 1u)) {
        connection_cleanup(&c); return fail("missing-Host write");
    }
    uint64_t completed = 0u;
    arbor_status status = arbor_http_mvc_server_step(&c.storage, &f.app, c.epfd, &completed);
    uint8_t response[4096] = {0};
    ssize_t response_used = drain(c.sv[0], response, sizeof(response));
    static const uint8_t bad_status[] = "HTTP/1.1 400 Bad Request\r\n";
    if (status.native != 0 || completed != 1u || f.response.calls != 0u ||
        c.storage.connection.state != ARBOR_ASM_CONNECTION_CLOSED || response_used <= 0 ||
        !contains_bytes(response, (size_t)response_used, bad_status, sizeof(bad_status) - 1u)) {
        connection_cleanup(&c); return fail("missing Host -> 400 close before MVC");
    }
    connection_cleanup(&c);

    /* Malformed Connection is also a 400 + close before MVC. */
    if (connection_prepare(&c, input, sizeof(input), output, sizeof(output),
                           arena, sizeof(arena), 0) != 0) return fail("bad-Connection setup");
    static const char bad_connection[] =
        "GET /x HTTP/1.1\r\nHost: local\r\nConnection: close bad\r\n\r\n";
    if (write(c.sv[0], bad_connection, sizeof(bad_connection) - 1u) !=
        (ssize_t)(sizeof(bad_connection) - 1u)) {
        connection_cleanup(&c); return fail("bad-Connection write");
    }
    completed = 0u;
    status = arbor_http_mvc_server_step(&c.storage, &f.app, c.epfd, &completed);
    memset(response, 0, sizeof(response)); response_used = drain(c.sv[0], response, sizeof(response));
    if (status.native != 0 || completed != 1u || f.response.calls != 0u || response_used <= 0 ||
        !contains_bytes(response, (size_t)response_used, bad_status, sizeof(bad_status) - 1u)) {
        connection_cleanup(&c); return fail("malformed Connection -> 400 close before MVC");
    }
    connection_cleanup(&c);

    /* Fragmentation: incomplete request returns EAGAIN, then completes after rest arrives. */
    f.response.calls = 0u;
    if (connection_prepare(&c, input, sizeof(input), output, sizeof(output),
                           arena, sizeof(arena), 0) != 0) return fail("fragment setup");
    static const char frag1[] = "GET /x HTTP/1.1\r\nHost: lo";
    static const char frag2[] = "cal\r\nConnection: close\r\n\r\n";
    if (write(c.sv[0], frag1, sizeof(frag1) - 1u) != (ssize_t)(sizeof(frag1) - 1u)) {
        connection_cleanup(&c); return fail("fragment first write");
    }
    completed = 0u;
    status = arbor_http_mvc_server_step(&c.storage, &f.app, c.epfd, &completed);
    if (status.native != -EAGAIN || completed != 0u || f.response.calls != 0u ||
        c.storage.connection.state != ARBOR_ASM_CONNECTION_READING) {
        connection_cleanup(&c); return fail("fragmented request quiesces before MVC");
    }
    if (write(c.sv[0], frag2, sizeof(frag2) - 1u) != (ssize_t)(sizeof(frag2) - 1u)) {
        connection_cleanup(&c); return fail("fragment second write");
    }
    status = arbor_http_mvc_server_step(&c.storage, &f.app, c.epfd, &completed);
    if (status.native != 0 || completed != 1u || f.response.calls != 1u ||
        c.storage.connection.state != ARBOR_ASM_CONNECTION_CLOSED) {
        connection_cleanup(&c); return fail("fragmented request completion");
    }
    connection_cleanup(&c);

    /* HEAD keeps hypothetical Content-Length but suppresses body bytes. MVC0
     * method matching remains exact, so this qualification uses an explicit
     * HEAD route rather than silently introducing HEAD->GET fallback. */
    fixture head_fixture = {0};
    head_fixture.response = (response_context){
        0u, 200u, hello, sizeof(hello) - 1u, ARBOR_RESPONSE_PLAN_FLAG_KEEP_ALIVE, true
    };
    static const uint8_t head_method[] = "HEAD";
    if (fixture_prepare_method(
            &head_fixture, 4u, head_method, sizeof(head_method) - 1u) != 0) {
        return fail("HEAD fixture preparation");
    }
    if (connection_prepare(&c, input, sizeof(input), output, sizeof(output),
                           arena, sizeof(arena), 0) != 0) return fail("HEAD setup");
    static const char head_req[] =
        "HEAD /x HTTP/1.1\r\nHost: local\r\nConnection: close\r\n\r\n";
    if (write(c.sv[0], head_req, sizeof(head_req) - 1u) != (ssize_t)(sizeof(head_req) - 1u)) {
        connection_cleanup(&c); return fail("HEAD write");
    }
    completed = 0u;
    status = arbor_http_mvc_server_step(&c.storage, &head_fixture.app, c.epfd, &completed);
    memset(response, 0, sizeof(response)); response_used = drain(c.sv[0], response, sizeof(response));
    static const uint8_t len5[] = "Content-Length: 5\r\n";
    static const uint8_t body_marker[] = "\r\n\r\nhello";
    if (status.native != 0 || completed != 1u || head_fixture.response.calls != 1u ||
        response_used <= 0 ||
        !contains_bytes(response, (size_t)response_used, len5, sizeof(len5) - 1u) ||
        contains_bytes(response, (size_t)response_used, body_marker, sizeof(body_marker) - 1u)) {
        connection_cleanup(&c); return fail("HTTP1 HEAD delegates no-body semantics to HTTP0");
    }
    connection_cleanup(&c);

    /* Final 204/205/304 statuses pass through AF1/MVC0 and retain HTTP0's
     * status-specific no-content framing policy. */
    static const uint64_t no_body_statuses[] = {204u, 205u, 304u};
    static const uint8_t content_length_name[] = "Content-Length:";
    for (size_t i = 0u; i < sizeof(no_body_statuses) / sizeof(no_body_statuses[0]); ++i) {
        f.response.status = no_body_statuses[i];
        f.response.body = NULL;
        f.response.body_length = 0u;
        f.response.calls = 0u;
        if (fixture_prepare(&f, 4u) != 0) return fail("no-content fixture preparation");
        if (connection_prepare(&c, input, sizeof(input), output, sizeof(output),
                               arena, sizeof(arena), 0) != 0) {
            return fail("no-content socket setup");
        }
        static const char no_content_req[] =
            "GET /x HTTP/1.1\r\nHost: local\r\nConnection: close\r\n\r\n";
        if (write(c.sv[0], no_content_req, sizeof(no_content_req) - 1u) !=
            (ssize_t)(sizeof(no_content_req) - 1u)) {
            connection_cleanup(&c); return fail("no-content request write");
        }
        completed = 0u;
        status = arbor_http_mvc_server_step(&c.storage, &f.app, c.epfd, &completed);
        memset(response, 0, sizeof(response));
        response_used = drain(c.sv[0], response, sizeof(response));
        if (status.native != 0 || completed != 1u || f.response.calls != 1u ||
            response_used <= 0 ||
            (no_body_statuses[i] != 205u &&
             contains_bytes(response, (size_t)response_used,
                            content_length_name, sizeof(content_length_name) - 1u))) {
            connection_cleanup(&c);
            return fail("HTTP1 204/205/304 delegates no-content semantics to HTTP0");
        }
        connection_cleanup(&c);
    }

    /* Restore ordinary GET response for subsequent pipeline tests. */
    f.response = (response_context){0u, 200u, hello, sizeof(hello) - 1u,
                                    ARBOR_RESPONSE_PLAN_FLAG_KEEP_ALIVE, true};
    if (fixture_prepare(&f, 4u) != 0) return fail("post-no-content fixture restoration");

    /* Keep-alive pipelining reuses MVC0 transport and produces two responses. */
    f.response.calls = 0u;
    if (connection_prepare(&c, input, sizeof(input), output, sizeof(output),
                           arena, sizeof(arena), 0) != 0) return fail("pipeline setup");
    static const char pipeline[] =
        "GET /x HTTP/1.1\r\nHost: local\r\n\r\n"
        "GET /x HTTP/1.1\r\nHost: local\r\n\r\n";
    if (write(c.sv[0], pipeline, sizeof(pipeline) - 1u) != (ssize_t)(sizeof(pipeline) - 1u)) {
        connection_cleanup(&c); return fail("pipeline write");
    }
    completed = 0u;
    for (unsigned attempt = 0u; attempt < 4u && completed < 2u; ++attempt) {
        status = arbor_http_mvc_server_step(&c.storage, &f.app, c.epfd, &completed);
        if (status.native != 0 && status.native != -EAGAIN && status.native != 1) {
            connection_cleanup(&c); return fail("pipeline server step");
        }
    }
    memset(response, 0, sizeof(response)); response_used = drain(c.sv[0], response, sizeof(response));
    static const uint8_t ok_status[] = "HTTP/1.1 200 OK\r\n";
    if (completed != 2u || f.response.calls != 2u || response_used <= 0 ||
        count_bytes(response, (size_t)response_used, ok_status, sizeof(ok_status) - 1u) != 2u ||
        c.storage.connection.state != ARBOR_ASM_CONNECTION_READING) {
        connection_cleanup(&c); return fail("HTTP1 keep-alive pipelining via MVC0 transport");
    }
    connection_cleanup(&c);

    /* Unaligned caller arena backing remains supported through HTTP1 alignment slack. */
    f.response = (response_context){0u, 200u, hello, sizeof(hello) - 1u,
                                    ARBOR_RESPONSE_PLAN_FLAG_KEEP_ALIVE, true};
    if (fixture_prepare(&f, 4u) != 0) return fail("unaligned-arena fixture preparation");
    f.response.calls = 0u;
    if (connection_prepare(&c, input, sizeof(input), output, sizeof(output),
                           arena + 1u, sizeof(arena) - 1u, 0) != 0) return fail("unaligned arena setup");
    static const char unaligned_req[] =
        "GET /x HTTP/1.1\r\nHost: local\r\nConnection: close\r\n\r\n";
    if (write(c.sv[0], unaligned_req, sizeof(unaligned_req) - 1u) !=
        (ssize_t)(sizeof(unaligned_req) - 1u)) {
        connection_cleanup(&c); return fail("unaligned arena request write");
    }
    completed = 0u;
    status = arbor_http_mvc_server_step(&c.storage, &f.app, c.epfd, &completed);
    if (status.native != 0 || completed != 1u || f.response.calls != 1u ||
        c.storage.connection.state != ARBOR_ASM_CONNECTION_CLOSED) {
        connection_cleanup(&c); return fail("HTTP1 unaligned arena backing support");
    }
    connection_cleanup(&c);

    /* EAGAIN write resume: large response fills nonblocking server send buffer. */
    static uint8_t large_body[120000];
    memset(large_body, 'Z', sizeof(large_body));
    f.response = (response_context){0u, 200u, large_body, sizeof(large_body),
                                    ARBOR_RESPONSE_PLAN_FLAG_KEEP_ALIVE, false};
    if (fixture_prepare(&f, 2u) != 0) return fail("large-response fixture preparation");
    static uint8_t large_output[140000];
    if (connection_prepare(&c, input, sizeof(input), large_output, sizeof(large_output),
                           arena, sizeof(arena), 4096) != 0) return fail("EAGAIN setup");
    static const char large_req[] = "GET /x HTTP/1.1\r\nHost: local\r\n\r\n";
    if (write(c.sv[0], large_req, sizeof(large_req) - 1u) != (ssize_t)(sizeof(large_req) - 1u)) {
        connection_cleanup(&c); return fail("EAGAIN request write");
    }
    completed = 0u;
    status = arbor_http_mvc_server_step(&c.storage, &f.app, c.epfd, &completed);
    if (status.native != -EAGAIN || completed != 0u || f.response.calls != 1u ||
        c.storage.connection.state != ARBOR_ASM_CONNECTION_WRITING) {
        connection_cleanup(&c); return fail("HTTP1 large response reaches EAGAIN writing state");
    }
    size_t drained_total = 0u;
    uint8_t drain_buffer[16384];
    for (unsigned attempt = 0u; attempt < 64u && completed == 0u; ++attempt) {
        ssize_t got = drain(c.sv[0], drain_buffer, sizeof(drain_buffer));
        if (got < 0) { connection_cleanup(&c); return fail("EAGAIN client drain"); }
        drained_total += (size_t)got;
        status = arbor_http_mvc_server_step(&c.storage, &f.app, c.epfd, &completed);
        if (status.native != 0 && status.native != -EAGAIN) {
            connection_cleanup(&c); return fail("EAGAIN resume server step");
        }
    }
    if (completed != 1u || f.response.calls != 1u || drained_total == 0u ||
        c.storage.connection.state != ARBOR_ASM_CONNECTION_READING) {
        connection_cleanup(&c); return fail("HTTP1 EAGAIN write resume completion");
    }
    connection_cleanup(&c);

    puts("PASS: HTTP1 protocol 400s, fragmentation, HEAD, pipelining and EAGAIN write resume");
    return 0;
}
