#include <arpa/inet.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include "hello0.h"
#include <arborcore/linux_http_mvc_host.h>

#define LIFE0_SLOT_COUNT 4u
#define LIFE0_EVENT_CAPACITY 16u
#define LIFE0_BUFFER_CAPACITY 32768u

typedef struct life0_clock {
    int64_t now;
    uint64_t calls;
    uint64_t fail_on_call;
    int64_t failure;
} life0_clock;

typedef struct life0_fixture {
    hello0_web_application application;
    arbor_linux_http_mvc_host_slot slots[LIFE0_SLOT_COUNT];
    uint8_t inputs[LIFE0_SLOT_COUNT][LIFE0_BUFFER_CAPACITY];
    uint8_t outputs[LIFE0_SLOT_COUNT][LIFE0_BUFFER_CAPACITY];
    uint8_t arenas[LIFE0_SLOT_COUNT][LIFE0_BUFFER_CAPACITY];
    arbor_asm_epoll_event events[LIFE0_EVENT_CAPACITY];
    arbor_linux_http_mvc_host host;
    life0_clock clock;
    uint64_t diagnostic_calls;
    arbor_linux_http_mvc_host_diagnostic last_diagnostic;
    int64_t last_diagnostic_status;
    struct sockaddr_in address;
    uint16_t port;
} life0_fixture;

static life0_fixture fixture;

static int fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    return 1;
}

static int64_t controlled_clock(void *context)
{
    life0_clock *clock = (life0_clock *)context;
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
    arbor_linux_http_mvc_host_diagnostic diagnostic,
    int64_t native_status)
{
    life0_fixture *state = (life0_fixture *)context;
    if (state != NULL) {
        state->diagnostic_calls += UINT64_C(1);
        state->last_diagnostic = diagnostic;
        state->last_diagnostic_status = native_status;
    }
}

static bool stop_immediately(void *context)
{
    const bool *stop = (const bool *)context;
    return stop != NULL && *stop;
}

static uint64_t active_count(
    const arbor_linux_http_mvc_host *host)
{
    uint64_t count = 0u;
    for (uint64_t i = 0u; i < host->slot_count; ++i) {
        if (host->slots[i].active) {
            count += UINT64_C(1);
        }
    }
    return count;
}

static int prepare_fixture(uint64_t timeout_ms, int64_t now)
{
    static const uint8_t template_source[] =
        "<!doctype html>\n"
        "<html lang=\"en\"><head><meta charset=\"utf-8\">"
        "<title>Arborcore HELLO0</title></head>"
        "<body><main><h1>Hello World</h1><p>{{message}}</p>"
        "</main></body></html>\n";

    (void)memset(&fixture, 0, sizeof(fixture));
    if (hello0_web_application_prepare(
            (arbor_span){template_source, sizeof(template_source) - 1u},
            HELLO0_RESPONSE_FIELD_CAPACITY,
            &fixture.application).native != 0) {
        return 1;
    }
    for (uint64_t i = 0u; i < LIFE0_SLOT_COUNT; ++i) {
        if (arbor_linux_http_mvc_host_slot_prepare(
                &fixture.slots[i],
                (arbor_mut_span){fixture.inputs[i], sizeof(fixture.inputs[i])},
                (arbor_mut_span){fixture.outputs[i], sizeof(fixture.outputs[i])},
                (arbor_mut_span){fixture.arenas[i], sizeof(fixture.arenas[i])})
                .native != 0) {
            return 1;
        }
    }
    fixture.clock.now = now;
    arbor_linux_http_mvc_host_options host_options;
    if (arbor_linux_http_mvc_host_options_make(
            &host_options,
            5,
            timeout_ms,
            controlled_clock,
            &fixture.clock,
            record_diagnostic,
            &fixture).native != 0) {
        return 1;
    }
    arbor_linux_http_mvc_host_options invalid_options[5] = {
        host_options, host_options, host_options, host_options, host_options
    };
    invalid_options[0].abi_version = 0u;
    invalid_options[1].struct_size -= 1u;
    invalid_options[2].flags = UINT64_C(1);
    invalid_options[3].event_wait_ms = -1;
    invalid_options[4].event_wait_ms = (int64_t)INT_MAX + INT64_C(1);
    for (size_t i = 0u; i < 5u; ++i) {
        arbor_linux_http_mvc_host invalid_host;
        (void)memset(&invalid_host, 0xa5, sizeof(invalid_host));
        arbor_linux_http_mvc_host sentinel = invalid_host;
        if (arbor_linux_http_mvc_host_prepare(
                &invalid_host,
                &fixture.application.http_application,
                fixture.slots,
                LIFE0_SLOT_COUNT,
                fixture.events,
                LIFE0_EVENT_CAPACITY,
                &invalid_options[i]).native != -EINVAL ||
            memcmp(&invalid_host, &sentinel, sizeof(invalid_host)) != 0) {
            return 1;
        }
    }
    if (arbor_linux_http_mvc_host_prepare(
            &fixture.host,
            &fixture.application.http_application,
            fixture.slots,
            LIFE0_SLOT_COUNT,
            fixture.events,
            LIFE0_EVENT_CAPACITY,
            &host_options).native != 0) {
        return 1;
    }
    (void)memset(&host_options, 0xa5, sizeof(host_options));
    return 0;
}

static int open_fixture(void)
{
    (void)memset(&fixture.address, 0, sizeof(fixture.address));
    fixture.address.sin_family = AF_INET;
    fixture.address.sin_port = htons(0u);
    fixture.address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (arbor_linux_http_mvc_host_open(
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

static int connect_loopback(void)
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
    address.sin_port = htons(fixture.port);
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

static int read_to_eof(
    int fd,
    uint8_t *bytes,
    size_t capacity,
    size_t *length_out)
{
    size_t length = 0u;
    for (;;) {
        if (length == capacity) {
            return 1;
        }
        ssize_t received = read(fd, bytes + length, capacity - length);
        if (received < 0) {
            if (errno == EINTR) {
                continue;
            }
            return 1;
        }
        if (received == 0) {
            *length_out = length;
            return 0;
        }
        length += (size_t)received;
    }
}

static size_t count_bytes(
    const uint8_t *bytes,
    size_t length,
    const uint8_t *needle,
    size_t needle_length)
{
    size_t count = 0u;
    if (needle_length == 0u || needle_length > length) {
        return 0u;
    }
    for (size_t i = 0u; i + needle_length <= length; ++i) {
        if (memcmp(bytes + i, needle, needle_length) == 0) {
            count += 1u;
        }
    }
    return count;
}

static int accept_one(int client)
{
    (void)client;
    for (uint64_t attempt = 0u; attempt < 16u; ++attempt) {
        if (arbor_linux_http_mvc_host_step(&fixture.host).native != 0) {
            return 1;
        }
        if (active_count(&fixture.host) == 1u) {
            return 0;
        }
    }
    return 1;
}

static int drive_drain(uint64_t limit)
{
    for (uint64_t attempt = 0u; attempt < limit; ++attempt) {
        if (fixture.host.phase ==
            ARBOR_LINUX_HTTP_MVC_HOST_PHASE_CLOSED) {
            return 0;
        }
        if (arbor_linux_http_mvc_host_step(&fixture.host).native != 0) {
            return 1;
        }
    }
    return fixture.host.phase ==
        ARBOR_LINUX_HTTP_MVC_HOST_PHASE_CLOSED ? 0 : 1;
}

static int test_phase_contract_and_empty_run(void)
{
    arbor_linux_http_mvc_host unprepared = {0};
    arbor_linux_http_mvc_host before = unprepared;
    if (arbor_linux_http_mvc_host_validate(&unprepared).native != -EINVAL ||
        arbor_linux_http_mvc_host_open(
            &unprepared,
            &fixture.address,
            (uint64_t)sizeof(fixture.address),
            1).native != -EINVAL ||
        arbor_linux_http_mvc_host_step(&unprepared).native != -EINVAL ||
        arbor_linux_http_mvc_host_begin_drain(&unprepared).native != -EINVAL ||
        arbor_linux_http_mvc_host_close(&unprepared).native != -EINVAL ||
        memcmp(&unprepared, &before, sizeof(unprepared)) != 0) {
        return fail("all-zero host is invalid and remains unpublished");
    }

    if (prepare_fixture(UINT64_C(1000), 100) != 0 ||
        fixture.host.phase != ARBOR_LINUX_HTTP_MVC_HOST_PHASE_PREPARED) {
        return fail("prepare publishes the sole PREPARED phase");
    }
    arbor_linux_http_mvc_host prepared = fixture.host;
    arbor_linux_http_mvc_host_shutdown_result sentinel;
    (void)memset(&sentinel, 0xa5, sizeof(sentinel));
    arbor_linux_http_mvc_host_shutdown_result unchanged = sentinel;
    if (arbor_linux_http_mvc_host_step(&fixture.host).native != -EINVAL ||
        arbor_linux_http_mvc_host_begin_drain(&fixture.host).native != -EINVAL ||
        arbor_linux_http_mvc_host_shutdown_result_get(
            &fixture.host,
            &unchanged).native != -EINVAL ||
        memcmp(&fixture.host, &prepared, sizeof(prepared)) != 0 ||
        memcmp(&unchanged, &sentinel, sizeof(unchanged)) != 0) {
        return fail("PREPARED rejects serve, drain and result publication atomically");
    }
    struct sockaddr_in truncated_address;
    (void)memset(&truncated_address, 0, sizeof(truncated_address));
    truncated_address.sin_family = AF_INET;
    truncated_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (arbor_linux_http_mvc_host_open(
            &fixture.host,
            &truncated_address,
            UINT64_C(1),
            1).native >= 0 ||
        memcmp(&fixture.host, &prepared, sizeof(prepared)) != 0) {
        return fail("failed native open leaves PREPARED without a descriptor");
    }
    if (open_fixture() != 0 ||
        fixture.host.phase != ARBOR_LINUX_HTTP_MVC_HOST_PHASE_ACCEPTING) {
        return fail("open publishes ACCEPTING");
    }
    arbor_linux_http_mvc_host accepting = fixture.host;
    if (arbor_linux_http_mvc_host_open(
            &fixture.host,
            &fixture.address,
            (uint64_t)sizeof(fixture.address),
            1).native != -EINVAL ||
        memcmp(&fixture.host, &accepting, sizeof(accepting)) != 0) {
        return fail("second open is rejected without lifecycle mutation");
    }

    bool stop = true;
    if (arbor_linux_http_mvc_host_run(
            &fixture.host,
            stop_immediately,
            &stop).native != 0 ||
        fixture.host.phase != ARBOR_LINUX_HTTP_MVC_HOST_PHASE_CLOSED) {
        return fail("empty run observes stop and reaches CLOSED");
    }
    arbor_linux_http_mvc_host_shutdown_result result = {0};
    if (arbor_linux_http_mvc_host_shutdown_result_get(
            &fixture.host,
            &result).native != 0 ||
        result.active_at_drain_start != 0u ||
        result.inactive_before_deadline != 0u ||
        result.forced_at_deadline != 0u || result.deadline_expired ||
        result.drain_start_ms != UINT64_C(100) ||
        result.drain_finish_ms != UINT64_C(100) ||
        result.first_failure != 0) {
        return fail("empty drain publishes exact zero accounting and times");
    }
    arbor_linux_http_mvc_host closed = fixture.host;
    if (arbor_linux_http_mvc_host_close(&fixture.host).native != 0 ||
        memcmp(&fixture.host, &closed, sizeof(closed)) != 0 ||
        arbor_linux_http_mvc_host_open(
            &fixture.host,
            &fixture.address,
            (uint64_t)sizeof(fixture.address),
            1).native != -EINVAL ||
        arbor_linux_http_mvc_host_begin_drain(&fixture.host).native != -EINVAL ||
        arbor_linux_http_mvc_host_step(&fixture.host).native != -EINVAL) {
        return fail("CLOSED is idempotent and cannot reopen or serve");
    }
    return 0;
}

static int test_explicit_close_transitions_and_draining_run(void)
{
    if (prepare_fixture(UINT64_C(100), 500) != 0 || open_fixture() != 0) {
        return fail("prepare ACCEPTING explicit-close fixture");
    }
    int client = connect_loopback();
    if (client < 0 || accept_one(client) != 0 ||
        arbor_linux_http_mvc_host_close(&fixture.host).native != 0 ||
        fixture.host.phase != ARBOR_LINUX_HTTP_MVC_HOST_PHASE_CLOSED) {
        if (client >= 0) {
            (void)close(client);
        }
        return fail("explicit close converges ACCEPTING directly to CLOSED");
    }
    arbor_linux_http_mvc_host_shutdown_result result = {0};
    if (arbor_linux_http_mvc_host_shutdown_result_get(
            &fixture.host,
            &result).native != 0 ||
        result.active_at_drain_start != 1u ||
        result.inactive_before_deadline != 0u ||
        result.forced_at_deadline != 1u || result.deadline_expired ||
        result.first_failure != 0) {
        (void)close(client);
        return fail("ACCEPTING close publishes exact forced teardown accounting");
    }
    (void)close(client);

    if (prepare_fixture(UINT64_C(100), 600) != 0 || open_fixture() != 0) {
        return fail("prepare DRAINING explicit-close fixture");
    }
    client = connect_loopback();
    if (client < 0 || accept_one(client) != 0 ||
        arbor_linux_http_mvc_host_begin_drain(&fixture.host).native != 0 ||
        fixture.host.phase != ARBOR_LINUX_HTTP_MVC_HOST_PHASE_DRAINING) {
        if (client >= 0) {
            (void)close(client);
        }
        return fail("enter DRAINING before explicit close");
    }
    arbor_linux_http_mvc_host draining = fixture.host;
    if (arbor_linux_http_mvc_host_open(
            &fixture.host,
            &fixture.address,
            (uint64_t)sizeof(fixture.address),
            1).native != -EINVAL ||
        memcmp(&fixture.host, &draining, sizeof(draining)) != 0 ||
        arbor_linux_http_mvc_host_close(&fixture.host).native != 0 ||
        fixture.host.phase != ARBOR_LINUX_HTTP_MVC_HOST_PHASE_CLOSED ||
        arbor_linux_http_mvc_host_shutdown_result_get(
            &fixture.host,
            &result).native != 0 ||
        result.active_at_drain_start != 1u ||
        result.inactive_before_deadline != 0u ||
        result.forced_at_deadline != 1u || result.deadline_expired ||
        result.drain_start_ms != UINT64_C(600) ||
        result.drain_finish_ms != UINT64_C(600) || result.first_failure != 0) {
        (void)close(client);
        return fail("DRAINING rejects reopen and explicit close converges exactly");
    }
    (void)close(client);

    if (prepare_fixture(UINT64_C(25), 700) != 0 || open_fixture() != 0) {
        return fail("prepare run-from-DRAINING fixture");
    }
    client = connect_loopback();
    if (client < 0 || accept_one(client) != 0 ||
        arbor_linux_http_mvc_host_begin_drain(&fixture.host).native != 0) {
        if (client >= 0) {
            (void)close(client);
        }
        return fail("enter DRAINING before run continuation");
    }
    fixture.clock.now = 725;
    bool stop = false;
    if (arbor_linux_http_mvc_host_run(
            &fixture.host,
            stop_immediately,
            &stop).native != 0 ||
        fixture.host.phase != ARBOR_LINUX_HTTP_MVC_HOST_PHASE_CLOSED ||
        arbor_linux_http_mvc_host_shutdown_result_get(
            &fixture.host,
            &result).native != 0 ||
        result.active_at_drain_start != 1u ||
        result.inactive_before_deadline != 0u ||
        result.forced_at_deadline != 1u || !result.deadline_expired ||
        result.drain_start_ms != UINT64_C(700) ||
        result.drain_finish_ms != UINT64_C(725) || result.first_failure != 0) {
        (void)close(client);
        return fail("run continues an existing drain without replacing its deadline");
    }
    (void)close(client);
    return 0;
}

static int test_fragmented_natural_drain(void)
{
    static const uint8_t first[] =
        "GET /hello HTTP/1.1\r\nHost: local\r\nConnection:";
    static const uint8_t second[] = " close\r\n\r\n";
    if (prepare_fixture(UINT64_C(500), 200) != 0 || open_fixture() != 0) {
        return fail("prepare fragmented natural-drain fixture");
    }
    int client = connect_loopback();
    if (client < 0 || write_all(client, first, sizeof(first) - 1u) != 0 ||
        accept_one(client) != 0) {
        if (client >= 0) {
            (void)close(client);
        }
        (void)arbor_linux_http_mvc_host_close(&fixture.host);
        return fail("accept fragmented request before drain");
    }
    if (arbor_linux_http_mvc_host_begin_drain(&fixture.host).native != 0 ||
        fixture.host.phase != ARBOR_LINUX_HTTP_MVC_HOST_PHASE_DRAINING ||
        fixture.host.listener_fd != -1 || fixture.host.listener_readable ||
        write_all(client, second, sizeof(second) - 1u) != 0 ||
        drive_drain(64u) != 0) {
        (void)close(client);
        (void)arbor_linux_http_mvc_host_close(&fixture.host);
        return fail("fragmented accepted request completes during drain");
    }
    uint8_t response[32768] = {0};
    size_t response_length = 0u;
    arbor_linux_http_mvc_host_shutdown_result result = {0};
    static const uint8_t ok[] = "HTTP/1.1 200 OK\r\n";
    if (read_to_eof(client, response, sizeof(response), &response_length) != 0 ||
        count_bytes(response, response_length, ok, sizeof(ok) - 1u) != 1u ||
        arbor_linux_http_mvc_host_shutdown_result_get(
            &fixture.host,
            &result).native != 0 ||
        result.active_at_drain_start != 1u ||
        result.inactive_before_deadline != 1u ||
        result.forced_at_deadline != 0u || result.deadline_expired ||
        result.first_failure != 0 ||
        result.inactive_before_deadline + result.forced_at_deadline !=
            result.active_at_drain_start) {
        (void)close(client);
        return fail("natural drain preserves response and exact accounting");
    }
    (void)close(client);
    return 0;
}

static int test_buffered_pipeline_drain(void)
{
    static const uint8_t pipeline[] =
        "GET /hello HTTP/1.1\r\nHost: local\r\n\r\n"
        "GET / HTTP/1.1\r\nHost: local\r\nConnection: close\r\n\r\n";
    if (prepare_fixture(UINT64_C(500), 300) != 0 || open_fixture() != 0) {
        return fail("prepare buffered-pipeline drain fixture");
    }
    int client = connect_loopback();
    if (client < 0 || write_all(client, pipeline, sizeof(pipeline) - 1u) != 0 ||
        accept_one(client) != 0 ||
        arbor_linux_http_mvc_host_begin_drain(&fixture.host).native != 0 ||
        drive_drain(128u) != 0) {
        if (client >= 0) {
            (void)close(client);
        }
        (void)arbor_linux_http_mvc_host_close(&fixture.host);
        return fail("buffered pipeline follows frozen request-budget drain");
    }
    uint8_t response[32768] = {0};
    size_t response_length = 0u;
    static const uint8_t prefix[] = "HTTP/1.1 ";
    arbor_linux_http_mvc_host_shutdown_result result = {0};
    if (read_to_eof(client, response, sizeof(response), &response_length) != 0 ||
        count_bytes(response, response_length, prefix, sizeof(prefix) - 1u) != 2u ||
        fixture.application.metrics.controller_calls != 2u ||
        arbor_linux_http_mvc_host_shutdown_result_get(
            &fixture.host,
            &result).native != 0 ||
        result.active_at_drain_start != 1u ||
        result.inactive_before_deadline != 1u ||
        result.forced_at_deadline != 0u || result.first_failure != 0) {
        (void)close(client);
        return fail("pipeline drain returns two ordered representations naturally");
    }
    (void)close(client);
    return 0;
}

static int test_deadline_idempotence_and_zero_timeout(void)
{
    if (prepare_fixture(UINT64_C(25), 400) != 0 || open_fixture() != 0) {
        return fail("prepare deadline-idempotence fixture");
    }
    int client = connect_loopback();
    if (client < 0 || accept_one(client) != 0 ||
        arbor_linux_http_mvc_host_begin_drain(&fixture.host).native != 0) {
        if (client >= 0) {
            (void)close(client);
        }
        (void)arbor_linux_http_mvc_host_close(&fixture.host);
        return fail("begin idle keep-alive drain");
    }
    uint64_t start = fixture.host.shutdown_result.drain_start_ms;
    uint64_t deadline = fixture.host.drain_deadline_ms;
    fixture.clock.now = 410;
    if (arbor_linux_http_mvc_host_begin_drain(&fixture.host).native != 0 ||
        fixture.host.shutdown_result.drain_start_ms != start ||
        fixture.host.drain_deadline_ms != deadline ||
        fixture.host.phase != ARBOR_LINUX_HTTP_MVC_HOST_PHASE_DRAINING) {
        (void)close(client);
        (void)arbor_linux_http_mvc_host_close(&fixture.host);
        return fail("repeated drain preserves original start and deadline");
    }
    fixture.clock.now = 425;
    if (arbor_linux_http_mvc_host_step(&fixture.host).native != 0 ||
        fixture.host.phase != ARBOR_LINUX_HTTP_MVC_HOST_PHASE_CLOSED) {
        (void)close(client);
        return fail("idle keep-alive is bounded by immutable deadline");
    }
    arbor_linux_http_mvc_host_shutdown_result result = {0};
    if (arbor_linux_http_mvc_host_shutdown_result_get(
            &fixture.host,
            &result).native != 0 ||
        result.active_at_drain_start != 1u ||
        result.inactive_before_deadline != 0u ||
        result.forced_at_deadline != 1u || !result.deadline_expired ||
        result.drain_start_ms != UINT64_C(400) ||
        result.drain_finish_ms != UINT64_C(425) || result.first_failure != 0) {
        (void)close(client);
        return fail("deadline enforcement publishes exact forced result");
    }
    (void)close(client);

    if (prepare_fixture(0u, 500) != 0 || open_fixture() != 0) {
        return fail("prepare zero-timeout fixture");
    }
    client = connect_loopback();
    if (client < 0 || accept_one(client) != 0 ||
        arbor_linux_http_mvc_host_begin_drain(&fixture.host).native != 0 ||
        fixture.host.phase != ARBOR_LINUX_HTTP_MVC_HOST_PHASE_CLOSED ||
        arbor_linux_http_mvc_host_shutdown_result_get(
            &fixture.host,
            &result).native != 0 ||
        result.active_at_drain_start != 1u || result.forced_at_deadline != 1u ||
        !result.deadline_expired || result.first_failure != 0) {
        if (client >= 0) {
            (void)close(client);
        }
        return fail("zero timeout closes listener and enforces immediately");
    }
    (void)close(client);
    return 0;
}

int main(void)
{
    if (test_phase_contract_and_empty_run() != 0 ||
        test_explicit_close_transitions_and_draining_run() != 0 ||
        test_fragmented_natural_drain() != 0 ||
        test_buffered_pipeline_drain() != 0 ||
        test_deadline_idempotence_and_zero_timeout() != 0) {
        return 1;
    }
    puts("PASS: LIFE0-R0 phases, explicit close, natural work, pipeline and deadline drains");
    return 0;
}
