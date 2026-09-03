#ifndef ARBORCORE_LINUX_HTTP_MVC_HOST_H
#define ARBORCORE_LINUX_HTTP_MVC_HOST_H

#include <stdbool.h>
#include <stdint.h>

#include <arborcore/http_mvc.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ARBOR_LINUX_HTTP_MVC_HOST_ABI_VERSION 1u
#define ARBOR_LINUX_HTTP_MVC_HOST_OPTIONS_KNOWN_FLAGS UINT64_C(0)

typedef enum arbor_linux_http_mvc_host_diagnostic {
    ARBOR_LINUX_HTTP_MVC_HOST_DIAGNOSTIC_LISTEN = 1,
    ARBOR_LINUX_HTTP_MVC_HOST_DIAGNOSTIC_EVENT_LOOP_CREATE = 2,
    ARBOR_LINUX_HTTP_MVC_HOST_DIAGNOSTIC_EVENT_LOOP = 3,
    ARBOR_LINUX_HTTP_MVC_HOST_DIAGNOSTIC_ACCEPT = 4,
    ARBOR_LINUX_HTTP_MVC_HOST_DIAGNOSTIC_CONNECTION = 5,
    ARBOR_LINUX_HTTP_MVC_HOST_DIAGNOSTIC_CLOCK = 6,
    ARBOR_LINUX_HTTP_MVC_HOST_DIAGNOSTIC_CLOSE = 7
} arbor_linux_http_mvc_host_diagnostic;

typedef void (*arbor_linux_http_mvc_host_diagnostic_fn)(
    void *context,
    arbor_linux_http_mvc_host_diagnostic diagnostic,
    int64_t native_status);

typedef bool (*arbor_linux_http_mvc_host_stop_fn)(void *context);

typedef int64_t (*arbor_linux_http_mvc_host_clock_fn)(void *context);

typedef struct arbor_linux_http_mvc_host_options {
    uint32_t abi_version;
    uint32_t struct_size;
    uint64_t flags;
    int64_t event_wait_ms;
    uint64_t drain_timeout_ms;
    arbor_linux_http_mvc_host_clock_fn clock;
    void *clock_context;
    arbor_linux_http_mvc_host_diagnostic_fn diagnostic;
    void *diagnostic_context;
} arbor_linux_http_mvc_host_options;

typedef enum arbor_linux_http_mvc_host_phase {
    ARBOR_LINUX_HTTP_MVC_HOST_PHASE_PREPARED = 1,
    ARBOR_LINUX_HTTP_MVC_HOST_PHASE_ACCEPTING = 2,
    ARBOR_LINUX_HTTP_MVC_HOST_PHASE_DRAINING = 3,
    ARBOR_LINUX_HTTP_MVC_HOST_PHASE_CLOSED = 4
} arbor_linux_http_mvc_host_phase;

typedef struct arbor_linux_http_mvc_host_shutdown_result {
    uint64_t active_at_drain_start;
    uint64_t inactive_before_deadline;
    uint64_t forced_at_deadline;
    uint64_t drain_start_ms;
    uint64_t drain_finish_ms;
    int64_t first_failure;
    bool deadline_expired;
} arbor_linux_http_mvc_host_shutdown_result;

/* Input, output and arena bytes remain caller-owned. */
typedef struct arbor_linux_http_mvc_host_slot {
    arbor_runtime_storage storage;
    bool active;
    bool more_work;
} arbor_linux_http_mvc_host_slot;

/* All pointer targets remain borrowed and stable through close. */
typedef struct arbor_linux_http_mvc_host {
    const arbor_http_mvc_application *application;
    arbor_linux_http_mvc_host_slot *slots;
    uint64_t slot_count;
    arbor_asm_epoll_event *events;
    uint64_t event_capacity;
    int64_t event_wait_ms;
    uint64_t drain_timeout_ms;
    int64_t listener_fd;
    int64_t epoll_fd;
    uint64_t drain_deadline_ms;
    arbor_linux_http_mvc_host_clock_fn clock;
    void *clock_context;
    arbor_linux_http_mvc_host_diagnostic_fn diagnostic;
    void *diagnostic_context;
    arbor_linux_http_mvc_host_shutdown_result shutdown_result;
    uint64_t prepared_guard;
    arbor_linux_http_mvc_host_phase phase;
    bool listener_readable;
} arbor_linux_http_mvc_host;

arbor_status arbor_linux_http_mvc_host_options_make(
    arbor_linux_http_mvc_host_options *options_out,
    int64_t event_wait_ms,
    uint64_t drain_timeout_ms,
    arbor_linux_http_mvc_host_clock_fn clock,
    void *clock_context,
    arbor_linux_http_mvc_host_diagnostic_fn diagnostic,
    void *diagnostic_context);

arbor_status arbor_linux_http_mvc_host_slot_prepare(
    arbor_linux_http_mvc_host_slot *slot,
    arbor_mut_span input_storage,
    arbor_mut_span output_storage,
    arbor_mut_span arena_storage);

arbor_status arbor_linux_http_mvc_host_prepare(
    arbor_linux_http_mvc_host *host,
    const arbor_http_mvc_application *application,
    arbor_linux_http_mvc_host_slot *slots,
    uint64_t slot_count,
    arbor_asm_epoll_event *events,
    uint64_t event_capacity,
    const arbor_linux_http_mvc_host_options *options);

arbor_status arbor_linux_http_mvc_host_validate(
    const arbor_linux_http_mvc_host *host);

arbor_status arbor_linux_http_mvc_host_open(
    arbor_linux_http_mvc_host *host,
    const void *sockaddr,
    uint64_t sockaddr_length,
    int64_t backlog);

arbor_status arbor_linux_http_mvc_host_step(
    arbor_linux_http_mvc_host *host);

arbor_status arbor_linux_http_mvc_host_begin_drain(
    arbor_linux_http_mvc_host *host);

arbor_status arbor_linux_http_mvc_host_run(
    arbor_linux_http_mvc_host *host,
    arbor_linux_http_mvc_host_stop_fn stop_requested,
    void *stop_context);

arbor_status arbor_linux_http_mvc_host_close(
    arbor_linux_http_mvc_host *host);

arbor_status arbor_linux_http_mvc_host_shutdown_result_get(
    const arbor_linux_http_mvc_host *host,
    arbor_linux_http_mvc_host_shutdown_result *result_out);

#ifdef __cplusplus
}
#endif

#endif
