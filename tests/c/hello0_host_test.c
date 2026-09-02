#include <arpa/inet.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include "hello0.h"
#include "linux_http_mvc_host.h"

#define HELLO0_HOST_TEST_SLOT_COUNT 2u
#define HELLO0_HOST_TEST_EVENT_CAPACITY 8u
#define HELLO0_HOST_TEST_BUFFER_CAPACITY 16384u
#define HELLO0_HOST_TEST_RESPONSE_CAPACITY 32768u

typedef struct hello0_host_test_diagnostics {
    uint64_t calls;
    arbor_example_linux_http_mvc_host_diagnostic last;
    int64_t last_native;
} hello0_host_test_diagnostics;

static int fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    return 1;
}

static void record_diagnostic(
    void *context,
    arbor_example_linux_http_mvc_host_diagnostic diagnostic,
    int64_t native_status)
{
    hello0_host_test_diagnostics *diagnostics =
        (hello0_host_test_diagnostics *)context;
    if (diagnostics != NULL) {
        diagnostics->calls += UINT64_C(1);
        diagnostics->last = diagnostic;
        diagnostics->last_native = native_status;
    }
}

static bool stop_immediately(void *context)
{
    const bool *stop = (const bool *)context;
    return stop != NULL && *stop;
}

static uint64_t active_slot_count(
    const arbor_example_linux_http_mvc_host *host)
{
    uint64_t count = 0u;
    for (uint64_t i = 0u; i < host->slot_count; ++i) {
        if (host->slots[i].active) {
            count += UINT64_C(1);
        }
    }
    return count;
}

static int connect_loopback(uint16_t port)
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

static int read_to_eof(
    int fd,
    uint8_t *bytes,
    size_t capacity,
    size_t *length_out)
{
    size_t used = 0u;
    for (;;) {
        if (used == capacity) {
            return 1;
        }
        ssize_t received = read(fd, bytes + used, capacity - used);
        if (received < 0) {
            if (errno == EINTR) {
                continue;
            }
            return 1;
        }
        if (received == 0) {
            break;
        }
        used += (size_t)received;
    }
    *length_out = used;
    return 0;
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
    if (needle_length > haystack_length) {
        return false;
    }
    for (size_t i = 0u; i + needle_length <= haystack_length; ++i) {
        if (memcmp(haystack + i, needle, needle_length) == 0) {
            return true;
        }
    }
    return false;
}

int main(void)
{
    static uint8_t template_source[] =
        "<!doctype html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "  <meta charset=\"utf-8\">\n"
        "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
        "  <title>Arborcore HELLO0</title>\n"
        "</head>\n"
        "<body>\n"
        "  <main>\n"
        "    <h1>Hello World</h1>\n"
        "    <p>{{message}}</p>\n"
        "  </main>\n"
        "</body>\n"
        "</html>\n";
    hello0_web_application application = {0};
    if (hello0_web_application_prepare(
            (arbor_span){
                template_source,
                (uint64_t)(sizeof(template_source) - 1u)},
            HELLO0_RESPONSE_FIELD_CAPACITY,
            &application).native != 0) {
        return fail("prepare HELLO0 application for private Linux host");
    }

    static arbor_example_linux_http_mvc_host_slot
        slots[HELLO0_HOST_TEST_SLOT_COUNT];
    static uint8_t inputs
        [HELLO0_HOST_TEST_SLOT_COUNT][HELLO0_HOST_TEST_BUFFER_CAPACITY];
    static uint8_t outputs
        [HELLO0_HOST_TEST_SLOT_COUNT][HELLO0_HOST_TEST_BUFFER_CAPACITY];
    static uint8_t arenas
        [HELLO0_HOST_TEST_SLOT_COUNT][HELLO0_HOST_TEST_BUFFER_CAPACITY];
    for (uint64_t i = 0u; i < HELLO0_HOST_TEST_SLOT_COUNT; ++i) {
        if (arbor_example_linux_http_mvc_host_slot_prepare(
                &slots[i],
                (arbor_mut_span){inputs[i], sizeof(inputs[i])},
                (arbor_mut_span){outputs[i], sizeof(outputs[i])},
                (arbor_mut_span){arenas[i], sizeof(arenas[i])}).native != 0) {
            return fail("prepare caller-owned private-host connection slot");
        }
    }

    arbor_example_linux_http_mvc_host_slot overlapping_slot = {0};
    uint8_t overlapping_bytes[64] = {0};
    uint8_t separate_arena[64] = {0};
    if (arbor_example_linux_http_mvc_host_slot_prepare(
            &overlapping_slot,
            (arbor_mut_span){overlapping_bytes, sizeof(overlapping_bytes)},
            (arbor_mut_span){overlapping_bytes, sizeof(overlapping_bytes)},
            (arbor_mut_span){separate_arena, sizeof(separate_arena)}).native !=
        -EINVAL) {
        return fail("private-host slot rejects overlapping backing regions");
    }

    arbor_asm_epoll_event events[HELLO0_HOST_TEST_EVENT_CAPACITY] = {0};
    hello0_host_test_diagnostics diagnostics = {0};

    static arbor_example_linux_http_mvc_host_slot alias_slots[2];
    static uint8_t shared_input[128];
    static uint8_t alias_outputs[2][128];
    static uint8_t alias_arenas[2][128];
    for (uint64_t i = 0u; i < 2u; ++i) {
        if (arbor_example_linux_http_mvc_host_slot_prepare(
                &alias_slots[i],
                (arbor_mut_span){shared_input, sizeof(shared_input)},
                (arbor_mut_span){alias_outputs[i], sizeof(alias_outputs[i])},
                (arbor_mut_span){alias_arenas[i], sizeof(alias_arenas[i])}).native != 0) {
            return fail("prepare individually valid alias-adversarial slots");
        }
    }
    arbor_example_linux_http_mvc_host alias_host;
    (void)memset(&alias_host, 0xa5, sizeof(alias_host));
    arbor_example_linux_http_mvc_host alias_sentinel = alias_host;
    if (arbor_example_linux_http_mvc_host_prepare(
            &alias_host,
            &application.http_application,
            alias_slots,
            2u,
            events,
            HELLO0_HOST_TEST_EVENT_CAPACITY,
            20,
            record_diagnostic,
            &diagnostics).native != -EINVAL ||
        memcmp(&alias_host, &alias_sentinel, sizeof(alias_host)) != 0) {
        return fail("private host rejects cross-slot backing aliases atomically");
    }

    arbor_example_linux_http_mvc_host host = {0};
    if (arbor_example_linux_http_mvc_host_prepare(
            &host,
            &application.http_application,
            slots,
            HELLO0_HOST_TEST_SLOT_COUNT,
            events,
            HELLO0_HOST_TEST_EVENT_CAPACITY,
            20,
            record_diagnostic,
            &diagnostics).native != 0 ||
        arbor_example_linux_http_mvc_host_validate(&host).native != 0) {
        return fail("prepare and validate private Linux HTTP/MVC host");
    }

    arbor_example_linux_http_mvc_host sentinel;
    (void)memset(&sentinel, 0xa5, sizeof(sentinel));
    arbor_example_linux_http_mvc_host unchanged = sentinel;
    if (arbor_example_linux_http_mvc_host_prepare(
            &unchanged,
            &application.http_application,
            slots,
            0u,
            events,
            HELLO0_HOST_TEST_EVENT_CAPACITY,
            20,
            record_diagnostic,
            &diagnostics).native != -EINVAL ||
        memcmp(&unchanged, &sentinel, sizeof(unchanged)) != 0) {
        return fail("invalid private-host preparation is failure-atomic");
    }

    struct sockaddr_in address;
    (void)memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(0u);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    arbor_example_linux_http_mvc_host before_invalid_open = host;
    if (arbor_example_linux_http_mvc_host_open(
            &host,
            &address,
            0u,
            8).native != -EINVAL ||
        memcmp(&host, &before_invalid_open, sizeof(host)) != 0) {
        return fail("invalid private-host open is failure-atomic");
    }
    if (arbor_example_linux_http_mvc_host_open(
            &host,
            &address,
            (uint64_t)sizeof(address),
            8).native != 0 ||
        arbor_example_linux_http_mvc_host_validate(&host).native != 0) {
        return fail("open loopback listener through private host");
    }

    socklen_t address_length = (socklen_t)sizeof(address);
    if (getsockname(
            (int)host.listener_fd,
            (struct sockaddr *)(void *)&address,
            &address_length) != 0 ||
        address_length != (socklen_t)sizeof(address) ||
        ntohs(address.sin_port) == 0u) {
        (void)arbor_example_linux_http_mvc_host_close(&host);
        return fail("read private-host ephemeral listener address");
    }
    const uint16_t port = ntohs(address.sin_port);

    bool stop = true;
    if (arbor_example_linux_http_mvc_host_run(
            &host,
            stop_immediately,
            &stop).native != 0) {
        (void)arbor_example_linux_http_mvc_host_close(&host);
        return fail("private host accepts externally owned stop predicate");
    }

    int clients[3] = {-1, -1, -1};
    static const uint8_t requests[3][80] = {
        "GET /hello HTTP/1.1\r\nHost: local\r\nConnection: close\r\n\r\n",
        "GET / HTTP/1.1\r\nHost: local\r\nConnection: close\r\n\r\n",
        "GET /missing HTTP/1.1\r\nHost: local\r\nConnection: close\r\n\r\n"
    };
    static const size_t request_lengths[3] = {
        sizeof("GET /hello HTTP/1.1\r\nHost: local\r\nConnection: close\r\n\r\n") - 1u,
        sizeof("GET / HTTP/1.1\r\nHost: local\r\nConnection: close\r\n\r\n") - 1u,
        sizeof("GET /missing HTTP/1.1\r\nHost: local\r\nConnection: close\r\n\r\n") - 1u
    };
    for (size_t i = 0u; i < 3u; ++i) {
        clients[i] = connect_loopback(port);
        if (clients[i] < 0 ||
            write_all(clients[i], requests[i], request_lengths[i]) != 0) {
            (void)arbor_example_linux_http_mvc_host_close(&host);
            return fail("connect and write three loopback requests");
        }
    }

    if (arbor_example_linux_http_mvc_host_step(&host).native != 0 ||
        active_slot_count(&host) != HELLO0_HOST_TEST_SLOT_COUNT ||
        host.listener_readable) {
        (void)arbor_example_linux_http_mvc_host_close(&host);
        return fail("slot saturation disables listener readability");
    }

    bool saw_reenabled_listener = false;
    bool completed = false;
    for (uint64_t attempt = 0u; attempt < 128u; ++attempt) {
        if (arbor_example_linux_http_mvc_host_step(&host).native != 0) {
            (void)arbor_example_linux_http_mvc_host_close(&host);
            return fail("advance private-host saturation and rearm sequence");
        }
        if (host.listener_readable) {
            saw_reenabled_listener = true;
        }
        if (saw_reenabled_listener && active_slot_count(&host) == 0u &&
            application.metrics.controller_calls == UINT64_C(2)) {
            completed = true;
            break;
        }
    }
    if (!completed || !saw_reenabled_listener || diagnostics.calls != 0u) {
        (void)arbor_example_linux_http_mvc_host_close(&host);
        return fail("private host rearms and completes queued connection without diagnostics");
    }

    uint8_t responses[3][HELLO0_HOST_TEST_RESPONSE_CAPACITY] = {{0}};
    size_t response_lengths[3] = {0u, 0u, 0u};
    for (size_t i = 0u; i < 3u; ++i) {
        if (read_to_eof(
                clients[i],
                responses[i],
                sizeof(responses[i]),
                &response_lengths[i]) != 0) {
            (void)arbor_example_linux_http_mvc_host_close(&host);
            return fail("read private-host responses to EOF");
        }
        (void)close(clients[i]);
        clients[i] = -1;
    }

    static const uint8_t ok[] = "HTTP/1.1 200 OK\r\n";
    static const uint8_t found[] = "HTTP/1.1 302 Found\r\n";
    static const uint8_t missing[] = "HTTP/1.1 404 Not Found\r\n";
    if (!contains_bytes(responses[0], response_lengths[0], ok, sizeof(ok) - 1u) ||
        !contains_bytes(
            responses[1], response_lengths[1], found, sizeof(found) - 1u) ||
        !contains_bytes(
            responses[2], response_lengths[2], missing, sizeof(missing) - 1u)) {
        (void)arbor_example_linux_http_mvc_host_close(&host);
        return fail("private host preserves HELLO0 page, redirect and fallback responses");
    }

    if (arbor_example_linux_http_mvc_host_close(&host).native != 0 ||
        host.opened || !host.closed || host.listener_fd != -1 ||
        host.epoll_fd != -1 ||
        arbor_example_linux_http_mvc_host_validate(&host).native != 0 ||
        arbor_example_linux_http_mvc_host_close(&host).native != 0 ||
        arbor_example_linux_http_mvc_host_open(
            &host,
            &address,
            (uint64_t)sizeof(address),
            8).native != -EINVAL) {
        return fail("private host cleanup is complete and idempotent");
    }

    puts("PASS: HOST0-R0 private Linux host slots, backpressure, rearm and HELLO0 responses");
    return 0;
}
