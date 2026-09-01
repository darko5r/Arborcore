#include <arpa/inet.h>
#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "hello0.h"

#define HELLO0_EVENT_CAPACITY 16u
#define HELLO0_EVENT_WAIT_MS 250

typedef struct hello0_connection_slot {
    arbor_runtime_storage storage;
    uint8_t input[HELLO0_CONNECTION_BUFFER_CAPACITY];
    uint8_t output[HELLO0_CONNECTION_BUFFER_CAPACITY];
    uint8_t arena[HELLO0_CONNECTION_BUFFER_CAPACITY];
    bool active;
    bool more_work;
} hello0_connection_slot;

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

static int hello0_prepare_slots(hello0_connection_slot *slots, size_t count)
{
    if (slots == NULL || count == 0u) {
        return 1;
    }
    for (size_t i = 0u; i < count; ++i) {
        arbor_status status = arbor_runtime_storage_prepare(
            &slots[i].storage,
            (arbor_mut_span){slots[i].input, sizeof(slots[i].input)},
            (arbor_mut_span){slots[i].output, sizeof(slots[i].output)},
            (arbor_mut_span){slots[i].arena, sizeof(slots[i].arena)});
        if (status.native != 0) {
            return 1;
        }
        slots[i].active = false;
        slots[i].more_work = false;
    }
    return 0;
}

static hello0_connection_slot *hello0_find_free_slot(
    hello0_connection_slot *slots,
    size_t count)
{
    for (size_t i = 0u; i < count; ++i) {
        if (!slots[i].active) {
            return &slots[i];
        }
    }
    return NULL;
}

static hello0_connection_slot *hello0_find_connection_slot(
    hello0_connection_slot *slots,
    size_t count,
    uint64_t event_data)
{
    for (size_t i = 0u; i < count; ++i) {
        if (slots[i].active &&
            (uint64_t)(uintptr_t)&slots[i].storage.connection == event_data) {
            return &slots[i];
        }
    }
    return NULL;
}

static void hello0_close_slot(int64_t epoll_fd, hello0_connection_slot *slot)
{
    if (slot == NULL || !slot->active) {
        return;
    }
    if (slot->storage.connection.state != ARBOR_ASM_CONNECTION_CLOSED) {
        (void)arbor_server_close(epoll_fd, &slot->storage);
    }
    slot->active = false;
    slot->more_work = false;
}

static void hello0_advance_slot(
    int64_t epoll_fd,
    const hello0_web_application *application,
    hello0_connection_slot *slot)
{
    if (slot == NULL || !slot->active) {
        return;
    }
    uint64_t completed = 0u;
    arbor_status status = arbor_http_mvc_server_step(
        &slot->storage,
        &application->http_application,
        epoll_fd,
        &completed);
    (void)completed;

    if (status.native == (int64_t)ARBORCORE_SERVER_MORE_WORK) {
        slot->more_work = true;
        return;
    }
    slot->more_work = false;
    if (status.native == -EAGAIN) {
        return;
    }
    if (status.native != 0) {
        fprintf(stderr, "HELLO0_CONNECTION_ERROR=%" PRId64 "\n", status.native);
        hello0_close_slot(epoll_fd, slot);
        return;
    }
    if (slot->storage.connection.state == ARBOR_ASM_CONNECTION_CLOSED) {
        slot->active = false;
    }
}

static bool hello0_any_more_work(
    const hello0_connection_slot *slots,
    size_t count)
{
    for (size_t i = 0u; i < count; ++i) {
        if (slots[i].active && slots[i].more_work) {
            return true;
        }
    }
    return false;
}

static int hello0_set_listener_readable(
    int64_t epoll_fd,
    int64_t listener_fd,
    bool readable)
{
    uint64_t events = (uint64_t)(EPOLLERR | EPOLLHUP);
    if (readable) {
        events |= (uint64_t)EPOLLIN;
    }
    void *data = (void *)(uintptr_t)(uint64_t)listener_fd;
    return event_epoll_modify(epoll_fd, listener_fd, events, data) == 0 ? 0 : 1;
}

static int hello0_accept_ready(
    int64_t listener_fd,
    int64_t epoll_fd,
    hello0_connection_slot *slots,
    size_t count,
    bool *listener_readable)
{
    for (;;) {
        hello0_connection_slot *slot = hello0_find_free_slot(slots, count);
        if (slot == NULL) {
            if (*listener_readable &&
                hello0_set_listener_readable(epoll_fd, listener_fd, false) != 0) {
                return 1;
            }
            *listener_readable = false;
            return 0;
        }

        int64_t accepted_fd = -1;
        arbor_status status = arbor_server_accept(
            listener_fd,
            epoll_fd,
            &slot->storage,
            &accepted_fd);
        if (status.native == -EAGAIN) {
            return 0;
        }
        if (status.native != 0 || accepted_fd < 0) {
            fprintf(stderr, "HELLO0_ACCEPT_ERROR=%" PRId64 "\n", status.native);
            return 1;
        }
        slot->active = true;
        slot->more_work = false;
    }
}

static int hello0_run(
    int64_t listener_fd,
    int64_t epoll_fd,
    const hello0_web_application *application,
    hello0_connection_slot *slots,
    size_t slot_count)
{
    arbor_asm_epoll_event events[HELLO0_EVENT_CAPACITY] = {0};
    bool listener_readable = true;

    while (hello0_stop_requested == 0) {
        for (size_t i = 0u; i < slot_count; ++i) {
            if (slots[i].active && slots[i].more_work) {
                hello0_advance_slot(epoll_fd, application, &slots[i]);
            }
        }

        if (!listener_readable &&
            hello0_find_free_slot(slots, slot_count) != NULL) {
            if (hello0_set_listener_readable(epoll_fd, listener_fd, true) != 0) {
                return 1;
            }
            listener_readable = true;
        }

        int64_t timeout = hello0_any_more_work(slots, slot_count) ?
            0 : HELLO0_EVENT_WAIT_MS;
        int64_t event_count = event_epoll_wait(
            epoll_fd,
            events,
            HELLO0_EVENT_CAPACITY,
            timeout);
        if (event_count < 0) {
            fprintf(stderr, "HELLO0_EPOLL_ERROR=%" PRId64 "\n", event_count);
            return 1;
        }

        bool listener_event = false;
        for (int64_t i = 0; i < event_count; ++i) {
            uint64_t event_data = events[i].data;
            if (event_data == (uint64_t)listener_fd) {
                listener_event = true;
                continue;
            }
            hello0_connection_slot *slot = hello0_find_connection_slot(
                slots,
                slot_count,
                event_data);
            if (slot != NULL) {
                hello0_advance_slot(epoll_fd, application, slot);
            }
        }
        if (listener_event && listener_readable &&
            hello0_accept_ready(
                listener_fd,
                epoll_fd,
                slots,
                slot_count,
                &listener_readable) != 0) {
            return 1;
        }
    }
    return 0;
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

    int64_t listener_fd = -1;
    int64_t epoll_fd = -1;
    status = arbor_server_open(
        &address,
        (uint64_t)sizeof(address),
        HELLO0_LISTEN_BACKLOG,
        &listener_fd);
    if (status.native != 0) {
        fprintf(stderr, "HELLO0_LISTEN_ERROR=%" PRId64 "\n", status.native);
        return 1;
    }
    status = arbor_event_loop_create(listener_fd, &epoll_fd);
    if (status.native != 0) {
        fprintf(stderr, "HELLO0_EVENT_LOOP_ERROR=%" PRId64 "\n", status.native);
        (void)close((int)listener_fd);
        return 1;
    }

    socklen_t address_length = (socklen_t)sizeof(address);
    if (getsockname(
            (int)listener_fd,
            (struct sockaddr *)(void *)&address,
            &address_length) != 0 ||
        address_length != (socklen_t)sizeof(address)) {
        fputs("HELLO0_LISTENER_ADDRESS_ERROR\n", stderr);
        (void)close((int)epoll_fd);
        (void)close((int)listener_fd);
        return 1;
    }
    port = ntohs(address.sin_port);

    static hello0_connection_slot slots[HELLO0_CONNECTION_SLOT_COUNT];
    if (hello0_prepare_slots(slots, HELLO0_CONNECTION_SLOT_COUNT) != 0) {
        fputs("HELLO0_STORAGE_PREPARE_ERROR\n", stderr);
        (void)close((int)epoll_fd);
        (void)close((int)listener_fd);
        return 1;
    }

    printf(
        "HELLO0_READY=http://127.0.0.1:%" PRIu16 "/hello\n",
        port);
    (void)fflush(stdout);
    int result = hello0_run(
        listener_fd,
        epoll_fd,
        &application,
        slots,
        HELLO0_CONNECTION_SLOT_COUNT);

    for (size_t i = 0u; i < HELLO0_CONNECTION_SLOT_COUNT; ++i) {
        hello0_close_slot(epoll_fd, &slots[i]);
    }
    (void)close((int)epoll_fd);
    (void)close((int)listener_fd);
    printf(
        "HELLO0_STOPPED middleware=%" PRIu64 " controller=%" PRIu64
        " service=%" PRIu64 " presenter=%" PRIu64 "\n",
        application.metrics.middleware_calls,
        application.metrics.controller_calls,
        application.metrics.service_calls,
        application.metrics.presenter_calls);
    return result;
}
