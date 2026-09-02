#include <arpa/inet.h>
#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "hello0.h"
#include "linux_http_mvc_host.h"

#define HELLO0_EVENT_CAPACITY 16u
#define HELLO0_EVENT_WAIT_MS 250
#define HELLO0_DRAIN_TIMEOUT_MS UINT64_C(2000)

static volatile sig_atomic_t hello0_stop_requested = 0;

static void hello0_request_stop(int signal_number)
{
    (void)signal_number;
    hello0_stop_requested = 1;
}

static int hello0_install_signal_handlers(void)
{
    struct sigaction action;
    (void)memset(&action, 0, sizeof(action));
    action.sa_handler = hello0_request_stop;
    if (sigemptyset(&action.sa_mask) != 0 ||
        sigaction(SIGINT, &action, NULL) != 0 ||
        sigaction(SIGTERM, &action, NULL) != 0) {
        return 1;
    }
    struct sigaction ignored;
    (void)memset(&ignored, 0, sizeof(ignored));
    ignored.sa_handler = SIG_IGN;
    if (sigemptyset(&ignored.sa_mask) != 0 ||
        sigaction(SIGPIPE, &ignored, NULL) != 0) {
        return 1;
    }
    return 0;
}

static int hello0_parse_port(const char *text, uint16_t *port_out)
{
    if (text == NULL || port_out == NULL || text[0] == '\0') {
        return 1;
    }
    errno = 0;
    char *end = NULL;
    unsigned long parsed = strtoul(text, &end, 10);
    if (errno != 0 || end == text || end == NULL || *end != '\0' ||
        parsed > (unsigned long)UINT16_MAX) {
        return 1;
    }
    *port_out = (uint16_t)parsed;
    return 0;
}

static int hello0_load_template(
    const char *path,
    uint8_t *storage,
    size_t capacity,
    arbor_span *source_out)
{
    if (path == NULL || storage == NULL || capacity == 0u ||
        source_out == NULL) {
        return 1;
    }
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "HELLO0_TEMPLATE_OPEN_ERROR=%s\n", path);
        return 1;
    }

    size_t used = fread(storage, 1u, capacity, file);
    bool failed = ferror(file) != 0;
    if (!failed && used == capacity) {
        int extra = fgetc(file);
        if (extra != EOF || ferror(file) != 0) {
            failed = true;
        }
    }
    if (fclose(file) != 0) {
        failed = true;
    }
    if (failed || used == 0u) {
        fprintf(stderr, "HELLO0_TEMPLATE_READ_ERROR=%s\n", path);
        return 1;
    }
    *source_out = (arbor_span){storage, (uint64_t)used};
    return 0;
}

static void hello0_host_diagnostic(
    void *context,
    arbor_example_linux_http_mvc_host_diagnostic diagnostic,
    int64_t native_status)
{
    (void)context;
    switch (diagnostic) {
    case ARBOR_EXAMPLE_LINUX_HTTP_MVC_HOST_DIAGNOSTIC_LISTEN:
        fprintf(stderr, "HELLO0_LISTEN_ERROR=%" PRId64 "\n", native_status);
        break;
    case ARBOR_EXAMPLE_LINUX_HTTP_MVC_HOST_DIAGNOSTIC_EVENT_LOOP_CREATE:
        fprintf(
            stderr,
            "HELLO0_EVENT_LOOP_ERROR=%" PRId64 "\n",
            native_status);
        break;
    case ARBOR_EXAMPLE_LINUX_HTTP_MVC_HOST_DIAGNOSTIC_EVENT_LOOP:
        fprintf(stderr, "HELLO0_EPOLL_ERROR=%" PRId64 "\n", native_status);
        break;
    case ARBOR_EXAMPLE_LINUX_HTTP_MVC_HOST_DIAGNOSTIC_ACCEPT:
        fprintf(stderr, "HELLO0_ACCEPT_ERROR=%" PRId64 "\n", native_status);
        break;
    case ARBOR_EXAMPLE_LINUX_HTTP_MVC_HOST_DIAGNOSTIC_CONNECTION:
        fprintf(stderr, "HELLO0_CONNECTION_ERROR=%" PRId64 "\n", native_status);
        break;
    case ARBOR_EXAMPLE_LINUX_HTTP_MVC_HOST_DIAGNOSTIC_CLOCK:
        fprintf(stderr, "HELLO0_CLOCK_ERROR=%" PRId64 "\n", native_status);
        break;
    case ARBOR_EXAMPLE_LINUX_HTTP_MVC_HOST_DIAGNOSTIC_CLOSE:
        fprintf(stderr, "HELLO0_CLOSE_ERROR=%" PRId64 "\n", native_status);
        break;
    default:
        break;
    }
}

static bool hello0_should_stop(void *context)
{
    (void)context;
    return hello0_stop_requested != 0;
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s TEMPLATE PORT\n", argv[0]);
        return 2;
    }
    uint16_t port = 0u;
    if (hello0_parse_port(argv[2], &port) != 0) {
        fprintf(stderr, "HELLO0_INVALID_PORT=%s\n", argv[2]);
        return 2;
    }
    if (hello0_install_signal_handlers() != 0) {
        fputs("HELLO0_SIGNAL_SETUP_ERROR\n", stderr);
        return 1;
    }

    static uint8_t template_source[HELLO0_TEMPLATE_SOURCE_CAPACITY];
    arbor_span source = {NULL, 0u};
    if (hello0_load_template(
            argv[1],
            template_source,
            sizeof(template_source),
            &source) != 0) {
        return 1;
    }

    static hello0_web_application application;
    arbor_status status = hello0_web_application_prepare(
        source,
        HELLO0_RESPONSE_FIELD_CAPACITY,
        &application);
    if (status.native != 0) {
        fprintf(stderr, "HELLO0_PREPARE_ERROR=%" PRId64 "\n", status.native);
        return 1;
    }
    (void)arbor_secure_clear(template_source, sizeof(template_source));

    struct sockaddr_in address;
    (void)memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    static arbor_example_linux_http_mvc_host_slot
        slots[HELLO0_CONNECTION_SLOT_COUNT];
    static uint8_t slot_inputs
        [HELLO0_CONNECTION_SLOT_COUNT][HELLO0_CONNECTION_BUFFER_CAPACITY];
    static uint8_t slot_outputs
        [HELLO0_CONNECTION_SLOT_COUNT][HELLO0_CONNECTION_BUFFER_CAPACITY];
    static uint8_t slot_arenas
        [HELLO0_CONNECTION_SLOT_COUNT][HELLO0_CONNECTION_BUFFER_CAPACITY];
    for (size_t i = 0u; i < HELLO0_CONNECTION_SLOT_COUNT; ++i) {
        status = arbor_example_linux_http_mvc_host_slot_prepare(
            &slots[i],
            (arbor_mut_span){slot_inputs[i], sizeof(slot_inputs[i])},
            (arbor_mut_span){slot_outputs[i], sizeof(slot_outputs[i])},
            (arbor_mut_span){slot_arenas[i], sizeof(slot_arenas[i])});
        if (status.native != 0) {
            fputs("HELLO0_STORAGE_PREPARE_ERROR\n", stderr);
            return 1;
        }
    }

    static arbor_asm_epoll_event events[HELLO0_EVENT_CAPACITY];
    arbor_example_linux_http_mvc_host host = {0};
    status = arbor_example_linux_http_mvc_host_prepare(
        &host,
        &application.http_application,
        slots,
        HELLO0_CONNECTION_SLOT_COUNT,
        events,
        HELLO0_EVENT_CAPACITY,
        HELLO0_EVENT_WAIT_MS,
        HELLO0_DRAIN_TIMEOUT_MS,
        NULL,
        NULL,
        hello0_host_diagnostic,
        NULL);
    if (status.native != 0) {
        fprintf(stderr, "HELLO0_HOST_PREPARE_ERROR=%" PRId64 "\n", status.native);
        return 1;
    }
    status = arbor_example_linux_http_mvc_host_open(
        &host,
        &address,
        (uint64_t)sizeof(address),
        HELLO0_LISTEN_BACKLOG);
    if (status.native != 0) {
        return 1;
    }

    socklen_t address_length = (socklen_t)sizeof(address);
    if (getsockname(
            (int)host.listener_fd,
            (struct sockaddr *)(void *)&address,
            &address_length) != 0 ||
        address_length != (socklen_t)sizeof(address)) {
        fputs("HELLO0_LISTENER_ADDRESS_ERROR\n", stderr);
        (void)arbor_example_linux_http_mvc_host_close(&host);
        return 1;
    }
    port = ntohs(address.sin_port);

    printf(
        "HELLO0_READY=http://127.0.0.1:%" PRIu16 "/hello\n",
        port);
    (void)fflush(stdout);
    arbor_status run_status = arbor_example_linux_http_mvc_host_run(
        &host,
        hello0_should_stop,
        NULL);
    arbor_status close_status = arbor_example_linux_http_mvc_host_close(&host);
    arbor_example_linux_http_mvc_host_shutdown_result shutdown_result = {0};
    arbor_status result_status =
        arbor_example_linux_http_mvc_host_shutdown_result_get(
            &host,
            &shutdown_result);
    printf(
        "HELLO0_STOPPED middleware=%" PRIu64 " controller=%" PRIu64
        " service=%" PRIu64 " presenter=%" PRIu64 "\n",
        application.metrics.middleware_calls,
        application.metrics.controller_calls,
        application.metrics.service_calls,
        application.metrics.presenter_calls);
    if (result_status.native == 0) {
        printf(
            "HELLO0_LIFE0 phase=CLOSED active_at_drain_start=%" PRIu64
            " inactive_before_deadline=%" PRIu64
            " forced_at_deadline=%" PRIu64
            " deadline_expired=%u drain_start_ms=%" PRIu64
            " drain_finish_ms=%" PRIu64 " first_failure=%" PRId64 "\n",
            shutdown_result.active_at_drain_start,
            shutdown_result.inactive_before_deadline,
            shutdown_result.forced_at_deadline,
            shutdown_result.deadline_expired ? 1u : 0u,
            shutdown_result.drain_start_ms,
            shutdown_result.drain_finish_ms,
            shutdown_result.first_failure);
    }
    return run_status.native == 0 && close_status.native == 0 &&
        result_status.native == 0 && shutdown_result.first_failure == 0 ? 0 : 1;
}
