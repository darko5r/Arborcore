#include <arpa/inet.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "hello0.h"
#include <arborcore/linux_http_mvc_host.h>

#define LIFE0_BENCH_ROUNDS 9u
#define LIFE0_BENCH_SLOTS 2u
#define LIFE0_BENCH_EVENTS 8u
#define LIFE0_BENCH_BUFFER_CAPACITY 16384u

typedef enum life0_bench_case {
    LIFE0_BENCH_EMPTY = 1,
    LIFE0_BENCH_ZERO_TIMEOUT = 2,
    LIFE0_BENCH_DEADLINE = 3
} life0_bench_case;

typedef struct life0_bench_fixture {
    arbor_linux_http_mvc_host_slot slots[LIFE0_BENCH_SLOTS];
    uint8_t inputs[LIFE0_BENCH_SLOTS][LIFE0_BENCH_BUFFER_CAPACITY];
    uint8_t outputs[LIFE0_BENCH_SLOTS][LIFE0_BENCH_BUFFER_CAPACITY];
    uint8_t arenas[LIFE0_BENCH_SLOTS][LIFE0_BENCH_BUFFER_CAPACITY];
    arbor_asm_epoll_event events[LIFE0_BENCH_EVENTS];
    arbor_linux_http_mvc_host host;
    int64_t now;
    struct sockaddr_in address;
    uint16_t port;
} life0_bench_fixture;

static hello0_web_application application;
static life0_bench_fixture fixture;

static int64_t controlled_clock(void *context)
{
    const int64_t *now = (const int64_t *)context;
    return now == NULL ? -EINVAL : *now;
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

static int prepare_fixture(uint64_t timeout_ms)
{
    (void)memset(&fixture, 0, sizeof(fixture));
    for (uint64_t i = 0u; i < LIFE0_BENCH_SLOTS; ++i) {
        if (arbor_linux_http_mvc_host_slot_prepare(
                &fixture.slots[i],
                (arbor_mut_span){fixture.inputs[i], sizeof(fixture.inputs[i])},
                (arbor_mut_span){fixture.outputs[i], sizeof(fixture.outputs[i])},
                (arbor_mut_span){fixture.arenas[i], sizeof(fixture.arenas[i])})
                .native != 0) {
            return 1;
        }
    }
    fixture.now = 1000;
    arbor_linux_http_mvc_host_options host_options;
    if (arbor_linux_http_mvc_host_options_make(
            &host_options,
            1,
            timeout_ms,
            controlled_clock,
            &fixture.now,
            NULL,
            NULL).native != 0) {
        return 1;
    }
    if (arbor_linux_http_mvc_host_prepare(
            &fixture.host,
            &application.http_application,
            fixture.slots,
            LIFE0_BENCH_SLOTS,
            fixture.events,
            LIFE0_BENCH_EVENTS,
            &host_options).native != 0) {
        return 1;
    }
    (void)memset(&fixture.address, 0, sizeof(fixture.address));
    fixture.address.sin_family = AF_INET;
    fixture.address.sin_port = htons(0u);
    fixture.address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (arbor_linux_http_mvc_host_open(
            &fixture.host,
            &fixture.address,
            (uint64_t)sizeof(fixture.address),
            8).native != 0) {
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

static int connect_one(void)
{
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0) {
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

static int accept_expected(uint64_t expected)
{
    for (uint64_t attempt = 0u; attempt < 32u; ++attempt) {
        if (arbor_linux_http_mvc_host_step(&fixture.host).native != 0) {
            return 1;
        }
        if (active_count() == expected) {
            return 0;
        }
    }
    return 1;
}

static uint64_t elapsed_ns(
    const struct timespec *start,
    const struct timespec *finish)
{
    uint64_t start_ns = (uint64_t)start->tv_sec * UINT64_C(1000000000) +
        (uint64_t)start->tv_nsec;
    uint64_t finish_ns = (uint64_t)finish->tv_sec * UINT64_C(1000000000) +
        (uint64_t)finish->tv_nsec;
    return finish_ns >= start_ns ? finish_ns - start_ns : 0u;
}

static int run_round(
    life0_bench_case selected,
    uint64_t participants,
    uint64_t *elapsed_out,
    uint64_t *forced_out)
{
    uint64_t timeout = selected == LIFE0_BENCH_ZERO_TIMEOUT ? 0u : 10u;
    if (prepare_fixture(timeout) != 0) {
        return 1;
    }
    int clients[LIFE0_BENCH_SLOTS] = {-1, -1};
    for (uint64_t i = 0u; i < participants; ++i) {
        clients[i] = connect_one();
        if (clients[i] < 0) {
            return 1;
        }
    }
    if (participants != 0u && accept_expected(participants) != 0) {
        return 1;
    }

    struct timespec start;
    struct timespec finish;
    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0 ||
        arbor_linux_http_mvc_host_begin_drain(&fixture.host).native != 0) {
        return 1;
    }
    if (selected == LIFE0_BENCH_DEADLINE) {
        fixture.now += 10;
        if (arbor_linux_http_mvc_host_step(&fixture.host).native != 0) {
            return 1;
        }
    }
    if (clock_gettime(CLOCK_MONOTONIC, &finish) != 0) {
        return 1;
    }

    arbor_linux_http_mvc_host_shutdown_result result = {0};
    if (arbor_linux_http_mvc_host_shutdown_result_get(
            &fixture.host,
            &result).native != 0 ||
        result.active_at_drain_start != participants ||
        result.inactive_before_deadline != 0u ||
        result.forced_at_deadline != participants ||
        result.deadline_expired !=
            (selected != LIFE0_BENCH_EMPTY) ||
        result.first_failure != 0) {
        return 1;
    }
    for (uint64_t i = 0u; i < participants; ++i) {
        (void)close(clients[i]);
    }
    *elapsed_out = elapsed_ns(&start, &finish);
    *forced_out = result.forced_at_deadline;
    return 0;
}

static void sort_u64(uint64_t *values, size_t count)
{
    for (size_t i = 1u; i < count; ++i) {
        uint64_t value = values[i];
        size_t position = i;
        while (position > 0u && values[position - 1u] > value) {
            values[position] = values[position - 1u];
            position -= 1u;
        }
        values[position] = value;
    }
}

static int report_case(
    const char *name,
    life0_bench_case selected,
    uint64_t participants)
{
    uint64_t samples[LIFE0_BENCH_ROUNDS] = {0};
    uint64_t forced = 0u;
    for (size_t round = 0u; round < LIFE0_BENCH_ROUNDS; ++round) {
        if (run_round(selected, participants, &samples[round], &forced) != 0) {
            return 1;
        }
    }
    sort_u64(samples, LIFE0_BENCH_ROUNDS);
    printf(
        "%s,%" PRIu64 ",%" PRIu64 ",%" PRIu64 "\n",
        name,
        participants,
        samples[LIFE0_BENCH_ROUNDS / 2u],
        forced);
    return 0;
}

int main(void)
{
    static const uint8_t template_source[] =
        "<!doctype html><html><body><p>{{message}}</p></body></html>";
    if (hello0_web_application_prepare(
            (arbor_span){template_source, sizeof(template_source) - 1u},
            HELLO0_RESPONSE_FIELD_CAPACITY,
            &application).native != 0) {
        return 1;
    }

    puts("LIFE0_SHUTDOWN_DIAGNOSTIC");
    puts("case,participants,median_ns,forced_count");
    if (report_case("empty", LIFE0_BENCH_EMPTY, 0u) != 0 ||
        report_case("zero_timeout", LIFE0_BENCH_ZERO_TIMEOUT, 1u) != 0 ||
        report_case("deadline", LIFE0_BENCH_DEADLINE, 2u) != 0) {
        return 1;
    }
    puts("THRESHOLD_GATE=NONE_DIAGNOSTIC_ONLY");
    puts("PASS: LIFE0-R0 shutdown latency and forced-count diagnostic completed");
    return 0;
}
