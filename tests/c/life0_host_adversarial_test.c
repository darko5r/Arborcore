#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include "echo0.h"
#include "linux_http_mvc_host.h"

#define LIFE0_ADV_SLOT_COUNT 2u
#define LIFE0_ADV_EVENT_CAPACITY 16u
#define LIFE0_ADV_BUFFER_CAPACITY 32768u
#define LIFE0_ADV_PIPELINE_REQUESTS 256u
#define LIFE0_ADV_RESPONSE_CAPACITY 262144u

typedef struct adversarial_clock {
    int64_t now;
    uint64_t calls;
    uint64_t fail_on_call;
    int64_t failure;
} adversarial_clock;

typedef struct adversarial_fixture {
    echo0_web_application application;
    arbor_example_linux_http_mvc_host_slot slots[LIFE0_ADV_SLOT_COUNT];
    uint8_t inputs[LIFE0_ADV_SLOT_COUNT][LIFE0_ADV_BUFFER_CAPACITY];
    uint8_t outputs[LIFE0_ADV_SLOT_COUNT][LIFE0_ADV_BUFFER_CAPACITY];
    uint8_t arenas[LIFE0_ADV_SLOT_COUNT][LIFE0_ADV_BUFFER_CAPACITY];
    arbor_asm_epoll_event events[LIFE0_ADV_EVENT_CAPACITY];
    arbor_example_linux_http_mvc_host host;
    adversarial_clock clock;
    uint64_t diagnostics;
    arbor_example_linux_http_mvc_host_diagnostic first_diagnostic;
    int64_t first_diagnostic_status;
    struct sockaddr_in address;
    uint16_t port;
} adversarial_fixture;

static adversarial_fixture fixture;
static uint8_t pipeline_request[LIFE0_ADV_BUFFER_CAPACITY];
static uint8_t pipeline_response[LIFE0_ADV_RESPONSE_CAPACITY];

static int fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    return 1;
}

static int64_t adversarial_clock_read(void *context)
{
    adversarial_clock *clock = (adversarial_clock *)context;
    if (clock == NULL) {
        return -EINVAL;
    }
    clock->calls += UINT64_C(1);
    if (clock->fail_on_call != 0u &&
        clock->calls == clock->fail_on_call) {
        return clock->failure;
    }
    return clock->now;
}

static void record_diagnostic(
    void *context,
    arbor_example_linux_http_mvc_host_diagnostic diagnostic,
    int64_t native_status)
{
    adversarial_fixture *state = (adversarial_fixture *)context;
    if (state != NULL) {
        if (state->diagnostics == 0u) {
            state->first_diagnostic = diagnostic;
            state->first_diagnostic_status = native_status;
        }
        state->diagnostics += UINT64_C(1);
    }
}

static uint64_t active_count(void)
{
    uint64_t count = 0u;
    for (uint64_t i = 0u; i < fixture.host.slot_count; ++i) {
        if (fixture.host.slots[i].active) {
            count += UINT64_C(1);
        }
    }
    return count;
}

static arbor_example_linux_http_mvc_host_slot *first_active_slot(void)
{
    for (uint64_t i = 0u; i < fixture.host.slot_count; ++i) {
        if (fixture.host.slots[i].active) {
            return &fixture.host.slots[i];
        }
    }
    return NULL;
}

static int prepare_fixture(uint64_t timeout_ms, int64_t now)
{
    static const uint8_t template_source[] =
        "<!doctype html>\n"
        "<html lang=\"en\"><head><meta charset=\"utf-8\">"
        "<title>Arborcore ECHO0</title></head>"
        "<body><main><h1>Arborcore ECHO0</h1>"
        "<p>Echo: {{value}}</p></main></body></html>\n";

    (void)memset(&fixture, 0, sizeof(fixture));
    if (echo0_web_application_prepare(
            (arbor_span){template_source, sizeof(template_source) - 1u},
            ECHO0_RESPONSE_FIELD_CAPACITY,
            &fixture.application).native != 0) {
        return 1;
    }
    for (uint64_t i = 0u; i < LIFE0_ADV_SLOT_COUNT; ++i) {
        if (arbor_example_linux_http_mvc_host_slot_prepare(
                &fixture.slots[i],
                (arbor_mut_span){fixture.inputs[i], sizeof(fixture.inputs[i])},
                (arbor_mut_span){fixture.outputs[i], sizeof(fixture.outputs[i])},
                (arbor_mut_span){fixture.arenas[i], sizeof(fixture.arenas[i])})
                .native != 0) {
            return 1;
        }
    }
    fixture.clock.now = now;
    if (arbor_example_linux_http_mvc_host_prepare(
            &fixture.host,
            &fixture.application.http_application,
            fixture.slots,
            LIFE0_ADV_SLOT_COUNT,
            fixture.events,
            LIFE0_ADV_EVENT_CAPACITY,
            2,
            timeout_ms,
            adversarial_clock_read,
            &fixture.clock,
            record_diagnostic,
            &fixture).native != 0) {
        return 1;
    }
    return 0;
}

static int open_fixture(void)
{
    (void)memset(&fixture.address, 0, sizeof(fixture.address));
    fixture.address.sin_family = AF_INET;
    fixture.address.sin_port = htons(0u);
    fixture.address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (arbor_example_linux_http_mvc_host_open(
            &fixture.host,
            &fixture.address,
            (uint64_t)sizeof(fixture.address),
            16).native != 0) {
        return 1;
    }
    socklen_t length = (socklen_t)sizeof(fixture.address);
    if (getsockname(
            (int)fixture.host.listener_fd,
            (struct sockaddr *)(void *)&fixture.address,
            &length) != 0 ||
        length != (socklen_t)sizeof(fixture.address)) {
        return 1;
    }
    fixture.port = ntohs(fixture.address.sin_port);
    return fixture.port == 0u ? 1 : 0;
}

static int connect_to(uint16_t port)
{
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0) {
        return -1;
    }
    struct timeval timeout = {1, 0};
    if (setsockopt(
            fd,
            SOL_SOCKET,
            SO_RCVTIMEO,
            &timeout,
            (socklen_t)sizeof(timeout)) != 0) {
        (void)close(fd);
        return -1;
    }
    struct sockaddr_in address;
    (void)memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (connect(
            fd,
            (const struct sockaddr *)(const void *)&address,
            (socklen_t)sizeof(address)) != 0) {
        (void)close(fd);
        return -1;
    }
    return fd;
}

static int write_all(int fd, const uint8_t *bytes, size_t length)
{
    size_t offset = 0u;
    while (offset < length) {
        ssize_t written = write(fd, bytes + offset, length - offset);
        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }
            return 1;
        }
        if (written == 0) {
            return 1;
        }
        offset += (size_t)written;
    }
    return 0;
}

static int accept_until(uint64_t expected)
{
    for (uint64_t attempt = 0u; attempt < 32u; ++attempt) {
        if (arbor_example_linux_http_mvc_host_step(&fixture.host).native != 0) {
            return 1;
        }
        if (active_count() == expected) {
            return 0;
        }
    }
    return 1;
}

static int verify_closed_result(
    uint64_t active,
    uint64_t natural,
    uint64_t forced,
    bool expired,
    int64_t first_failure)
{
    arbor_example_linux_http_mvc_host_shutdown_result result = {0};
    if (fixture.host.phase != ARBOR_EXAMPLE_LINUX_HTTP_MVC_HOST_PHASE_CLOSED ||
        arbor_example_linux_http_mvc_host_shutdown_result_get(
            &fixture.host,
            &result).native != 0 ||
        result.active_at_drain_start != active ||
        result.inactive_before_deadline != natural ||
        result.forced_at_deadline != forced ||
        result.deadline_expired != expired ||
        result.first_failure != first_failure ||
        natural + forced != active) {
        return 1;
    }
    return 0;
}

static int test_clock_failure_and_overflow(void)
{
    if (prepare_fixture(UINT64_C(100), 1000) != 0 || open_fixture() != 0) {
        return fail("prepare clock-failure fixture");
    }
    int client = connect_to(fixture.port);
    if (client < 0 || accept_until(1u) != 0) {
        if (client >= 0) {
            (void)close(client);
        }
        return fail("accept participant before injected clock failure");
    }
    fixture.clock.fail_on_call = 1u;
    fixture.clock.failure = -EIO;
    if (arbor_example_linux_http_mvc_host_begin_drain(&fixture.host).native != -EIO ||
        verify_closed_result(1u, 0u, 1u, false, -EIO) != 0 ||
        fixture.diagnostics == 0u ||
        fixture.first_diagnostic !=
            ARBOR_EXAMPLE_LINUX_HTTP_MVC_HOST_DIAGNOSTIC_CLOCK ||
        fixture.first_diagnostic_status != -EIO) {
        (void)close(client);
        return fail("clock failure stops accepts, forces participants and closes");
    }
    (void)close(client);

    if (prepare_fixture(UINT64_MAX, INT64_MAX) != 0 || open_fixture() != 0 ||
        arbor_example_linux_http_mvc_host_begin_drain(&fixture.host).native !=
            -EOVERFLOW ||
        verify_closed_result(0u, 0u, 0u, false, -EOVERFLOW) != 0 ||
        fixture.host.shutdown_result.drain_start_ms != (uint64_t)INT64_MAX ||
        fixture.host.shutdown_result.drain_finish_ms != (uint64_t)INT64_MAX) {
        return fail("checked deadline overflow converges closed with first failure");
    }
    if (prepare_fixture(
            (uint64_t)INT64_MAX + UINT64_C(1),
            0) != 0) {
        return fail("prepare representational-overflow fixture");
    }
    if (open_fixture() != 0 ||
        arbor_example_linux_http_mvc_host_begin_drain(&fixture.host).native !=
            -EOVERFLOW ||
        verify_closed_result(0u, 0u, 0u, false, -EOVERFLOW) != 0) {
        return fail("deadline outside monotonic result range converges closed");
    }
    return 0;
}

static int test_close_failures_preserve_first(void)
{
    if (prepare_fixture(UINT64_C(100), 1100) != 0 || open_fixture() != 0) {
        return fail("prepare close-failure fixture");
    }
    int listener_fd = (int)fixture.host.listener_fd;
    int epoll_fd = (int)fixture.host.epoll_fd;
    if (close(listener_fd) != 0 || close(epoll_fd) != 0) {
        return fail("pre-close descriptors for deterministic failure injection");
    }
    if (arbor_example_linux_http_mvc_host_begin_drain(&fixture.host).native !=
            -EBADF ||
        verify_closed_result(0u, 0u, 0u, false, -EBADF) != 0 ||
        fixture.diagnostics < 2u ||
        fixture.first_diagnostic !=
            ARBOR_EXAMPLE_LINUX_HTTP_MVC_HOST_DIAGNOSTIC_CLOSE ||
        fixture.first_diagnostic_status != -EBADF) {
        return fail("completion teardown retains first close failure and attempts all");
    }
    return 0;
}

static int test_saturated_deadline_and_refusal(void)
{
    if (prepare_fixture(UINT64_C(10), 1200) != 0 || open_fixture() != 0) {
        return fail("prepare saturated deadline fixture");
    }
    int clients[LIFE0_ADV_SLOT_COUNT] = {-1, -1};
    for (uint64_t i = 0u; i < LIFE0_ADV_SLOT_COUNT; ++i) {
        clients[i] = connect_to(fixture.port);
        if (clients[i] < 0) {
            return fail("connect saturated participants");
        }
    }
    if (accept_until(LIFE0_ADV_SLOT_COUNT) != 0 ||
        fixture.host.listener_readable ||
        arbor_example_linux_http_mvc_host_begin_drain(&fixture.host).native != 0 ||
        fixture.host.phase != ARBOR_EXAMPLE_LINUX_HTTP_MVC_HOST_PHASE_DRAINING ||
        fixture.host.listener_fd != -1 || fixture.host.listener_readable) {
        return fail("saturated drain closes listener and disables accept rearm");
    }
    int refused = connect_to(fixture.port);
    if (refused >= 0) {
        (void)close(refused);
        return fail("connection opened after drain entry");
    }
    fixture.clock.now = 1210;
    if (arbor_example_linux_http_mvc_host_step(&fixture.host).native != 0 ||
        verify_closed_result(LIFE0_ADV_SLOT_COUNT, 0u, LIFE0_ADV_SLOT_COUNT, true, 0) !=
            0) {
        return fail("saturated deadline force-closes exact participant count");
    }
    for (uint64_t i = 0u; i < LIFE0_ADV_SLOT_COUNT; ++i) {
        (void)close(clients[i]);
    }
    return 0;
}

static size_t append_bytes(
    uint8_t *destination,
    size_t capacity,
    size_t offset,
    const uint8_t *source,
    size_t length)
{
    if (offset > capacity || length > capacity - offset) {
        return capacity + 1u;
    }
    (void)memcpy(destination + offset, source, length);
    return offset + length;
}

static int build_pipeline(size_t *length_out)
{
    static const uint8_t keep_alive[] =
        "GET /echo/Backpressure HTTP/1.1\r\nHost: local\r\n\r\n";
    static const uint8_t close_request[] =
        "GET /echo/Backpressure HTTP/1.1\r\nHost: local\r\n"
        "Connection: close\r\n\r\n";
    size_t length = 0u;
    for (uint64_t i = 0u; i + 1u < LIFE0_ADV_PIPELINE_REQUESTS; ++i) {
        length = append_bytes(
            pipeline_request,
            sizeof(pipeline_request),
            length,
            keep_alive,
            sizeof(keep_alive) - 1u);
        if (length > sizeof(pipeline_request)) {
            return 1;
        }
    }
    length = append_bytes(
        pipeline_request,
        sizeof(pipeline_request),
        length,
        close_request,
        sizeof(close_request) - 1u);
    if (length > sizeof(pipeline_request)) {
        return 1;
    }
    *length_out = length;
    return 0;
}

static int drain_client_nonblocking(
    int client,
    size_t *response_length)
{
    for (;;) {
        if (*response_length == sizeof(pipeline_response)) {
            return 1;
        }
        ssize_t received = recv(
            client,
            pipeline_response + *response_length,
            sizeof(pipeline_response) - *response_length,
            MSG_DONTWAIT);
        if (received > 0) {
            *response_length += (size_t)received;
            continue;
        }
        if (received == 0) {
            return 0;
        }
        if (errno == EINTR) {
            continue;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }
        return 1;
    }
}

static int drain_client_until_eof(
    int client,
    size_t *response_length)
{
    for (;;) {
        if (*response_length == sizeof(pipeline_response)) {
            return 1;
        }
        ssize_t received = recv(
            client,
            pipeline_response + *response_length,
            sizeof(pipeline_response) - *response_length,
            0);
        if (received > 0) {
            *response_length += (size_t)received;
            continue;
        }
        if (received == 0) {
            return 0;
        }
        if (errno == EINTR) {
            continue;
        }
        return 1;
    }
}

static size_t count_http_responses(size_t length)
{
    static const uint8_t prefix[] = "HTTP/1.1 ";
    size_t count = 0u;
    for (size_t i = 0u; i + sizeof(prefix) - 1u <= length; ++i) {
        if (memcmp(
                pipeline_response + i,
                prefix,
                sizeof(prefix) - 1u) == 0) {
            count += 1u;
        }
    }
    return count;
}

static int test_write_backpressure_resume(void)
{
    size_t request_length = 0u;
    (void)memset(pipeline_response, 0, sizeof(pipeline_response));
    if (build_pipeline(&request_length) != 0 ||
        prepare_fixture(UINT64_C(5000), 1300) != 0 || open_fixture() != 0) {
        return fail("prepare write-backpressure pipeline fixture");
    }
    int client = connect_to(fixture.port);
    if (client < 0 || write_all(client, pipeline_request, request_length) != 0 ||
        accept_until(1u) != 0) {
        if (client >= 0) {
            (void)close(client);
        }
        return fail("accept large keep-alive pipeline");
    }
    arbor_example_linux_http_mvc_host_slot *slot = first_active_slot();
    int send_buffer = 1024;
    int receive_buffer = 1024;
    if (slot == NULL ||
        setsockopt(
            (int)slot->storage.connection.fd,
            SOL_SOCKET,
            SO_SNDBUF,
            &send_buffer,
            (socklen_t)sizeof(send_buffer)) != 0 ||
        setsockopt(
            client,
            SOL_SOCKET,
            SO_RCVBUF,
            &receive_buffer,
            (socklen_t)sizeof(receive_buffer)) != 0 ||
        arbor_example_linux_http_mvc_host_begin_drain(&fixture.host).native != 0) {
        (void)close(client);
        (void)arbor_example_linux_http_mvc_host_close(&fixture.host);
        return fail("enter backpressured drain with constrained socket buffers");
    }

    bool stalled = false;
    uint64_t prior_write = 0u;
    bool have_prior_write = false;
    for (uint64_t attempt = 0u; attempt < 2048u; ++attempt) {
        slot = first_active_slot();
        if (slot == NULL) {
            break;
        }
        prior_write = slot->storage.connection.write_bytes;
        have_prior_write =
            slot->storage.connection.state == ARBOR_ASM_CONNECTION_WRITING;
        if (arbor_example_linux_http_mvc_host_step(&fixture.host).native != 0) {
            (void)close(client);
            return fail("advance pipeline toward deterministic backpressure");
        }
        slot = first_active_slot();
        if (have_prior_write && slot != NULL &&
            slot->storage.connection.state == ARBOR_ASM_CONNECTION_WRITING &&
            slot->storage.connection.write_bytes == prior_write) {
            stalled = true;
            break;
        }
    }
    if (!stalled) {
        (void)close(client);
        (void)arbor_example_linux_http_mvc_host_close(&fixture.host);
        return fail("large drain did not expose a no-progress WRITING/EAGAIN state");
    }

    size_t response_length = 0u;
    for (uint64_t attempt = 0u; attempt < 10000u &&
         fixture.host.phase != ARBOR_EXAMPLE_LINUX_HTTP_MVC_HOST_PHASE_CLOSED;
         ++attempt) {
        if (drain_client_nonblocking(client, &response_length) != 0 ||
            arbor_example_linux_http_mvc_host_step(&fixture.host).native != 0) {
            (void)close(client);
            return fail("resume backpressured response while client drains");
        }
    }
    if (fixture.host.phase != ARBOR_EXAMPLE_LINUX_HTTP_MVC_HOST_PHASE_CLOSED) {
        (void)close(client);
        (void)arbor_example_linux_http_mvc_host_close(&fixture.host);
        return fail("backpressured drain did not reach CLOSED");
    }
    if (drain_client_until_eof(client, &response_length) != 0) {
        (void)close(client);
        return fail("closed backpressured peer did not deliver buffered bytes and EOF");
    }
    if (count_http_responses(response_length) != LIFE0_ADV_PIPELINE_REQUESTS ||
        fixture.application.metrics.controller_calls !=
            LIFE0_ADV_PIPELINE_REQUESTS ||
        verify_closed_result(1u, 1u, 0u, false, 0) != 0) {
        (void)close(client);
        return fail("backpressured drain resumes and completes all buffered responses");
    }
    (void)close(client);
    return 0;
}

static int test_explicit_close_and_result_atomicity(void)
{
    if (prepare_fixture(UINT64_C(100), 1400) != 0) {
        return fail("prepare explicit-close fixture");
    }
    arbor_example_linux_http_mvc_host_shutdown_result result;
    (void)memset(&result, 0xa5, sizeof(result));
    arbor_example_linux_http_mvc_host_shutdown_result sentinel = result;
    if (arbor_example_linux_http_mvc_host_shutdown_result_get(
            &fixture.host,
            &result).native != -EINVAL ||
        memcmp(&result, &sentinel, sizeof(result)) != 0 ||
        arbor_example_linux_http_mvc_host_close(&fixture.host).native != 0 ||
        verify_closed_result(0u, 0u, 0u, false, 0) != 0) {
        return fail("result publication is failure-atomic and PREPARED closes cleanly");
    }
    return 0;
}

int main(void)
{
    if (test_clock_failure_and_overflow() != 0 ||
        test_close_failures_preserve_first() != 0 ||
        test_saturated_deadline_and_refusal() != 0 ||
        test_write_backpressure_resume() != 0 ||
        test_explicit_close_and_result_atomicity() != 0) {
        return 1;
    }
    puts("PASS: LIFE0-R0 failure convergence, saturation, refusal and backpressure resume");
    return 0;
}
