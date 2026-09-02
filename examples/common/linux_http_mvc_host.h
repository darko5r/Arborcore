#ifndef ARBORCORE_EXAMPLES_COMMON_LINUX_HTTP_MVC_HOST_H
#define ARBORCORE_EXAMPLES_COMMON_LINUX_HTTP_MVC_HOST_H

#include <stdbool.h>
#include <stdint.h>

#include <arborcore/http_mvc.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum arbor_example_linux_http_mvc_host_diagnostic {
    ARBOR_EXAMPLE_LINUX_HTTP_MVC_HOST_DIAGNOSTIC_LISTEN = 1,
    ARBOR_EXAMPLE_LINUX_HTTP_MVC_HOST_DIAGNOSTIC_EVENT_LOOP_CREATE = 2,
    ARBOR_EXAMPLE_LINUX_HTTP_MVC_HOST_DIAGNOSTIC_EVENT_LOOP = 3,
    ARBOR_EXAMPLE_LINUX_HTTP_MVC_HOST_DIAGNOSTIC_ACCEPT = 4,
    ARBOR_EXAMPLE_LINUX_HTTP_MVC_HOST_DIAGNOSTIC_CONNECTION = 5
} arbor_example_linux_http_mvc_host_diagnostic;

typedef void (*arbor_example_linux_http_mvc_host_diagnostic_fn)(
    void *context,
    arbor_example_linux_http_mvc_host_diagnostic diagnostic,
    int64_t native_status);

typedef bool (*arbor_example_linux_http_mvc_host_stop_fn)(void *context);

/*
 * Private example-host connection metadata. Input, output and arena bytes remain
 * caller-owned; this object owns no heap storage.
 */
typedef struct arbor_example_linux_http_mvc_host_slot {
    arbor_runtime_storage storage;
    bool active;
    bool more_work;
} arbor_example_linux_http_mvc_host_slot;

/*
 * Private Linux host candidate. All pointer targets are borrowed and must remain
 * at stable addresses from preparation through close.
 */
typedef struct arbor_example_linux_http_mvc_host {
    const arbor_http_mvc_application *application;
    arbor_example_linux_http_mvc_host_slot *slots;
    uint64_t slot_count;
    arbor_asm_epoll_event *events;
    uint64_t event_capacity;
    int64_t event_wait_ms;
    int64_t listener_fd;
    int64_t epoll_fd;
    arbor_example_linux_http_mvc_host_diagnostic_fn diagnostic;
    void *diagnostic_context;
    uint64_t prepared_guard;
    bool listener_readable;
    bool opened;
    bool closed;
} arbor_example_linux_http_mvc_host;

arbor_status arbor_example_linux_http_mvc_host_slot_prepare(
    arbor_example_linux_http_mvc_host_slot *slot,
    arbor_mut_span input_storage,
    arbor_mut_span output_storage,
    arbor_mut_span arena_storage);

arbor_status arbor_example_linux_http_mvc_host_prepare(
    arbor_example_linux_http_mvc_host *host,
    const arbor_http_mvc_application *application,
    arbor_example_linux_http_mvc_host_slot *slots,
    uint64_t slot_count,
    arbor_asm_epoll_event *events,
    uint64_t event_capacity,
    int64_t event_wait_ms,
    arbor_example_linux_http_mvc_host_diagnostic_fn diagnostic,
    void *diagnostic_context);

arbor_status arbor_example_linux_http_mvc_host_validate(
    const arbor_example_linux_http_mvc_host *host);

arbor_status arbor_example_linux_http_mvc_host_open(
    arbor_example_linux_http_mvc_host *host,
    const void *sockaddr,
    uint64_t sockaddr_length,
    int64_t backlog);

arbor_status arbor_example_linux_http_mvc_host_step(
    arbor_example_linux_http_mvc_host *host);

arbor_status arbor_example_linux_http_mvc_host_run(
    arbor_example_linux_http_mvc_host *host,
    arbor_example_linux_http_mvc_host_stop_fn stop_requested,
    void *stop_context);

arbor_status arbor_example_linux_http_mvc_host_close(
    arbor_example_linux_http_mvc_host *host);

#ifdef __cplusplus
}
#endif

#endif
