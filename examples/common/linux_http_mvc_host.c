#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "linux_http_mvc_host.h"

#define ARBOR_EXAMPLE_LINUX_HTTP_MVC_HOST_GUARD \
    UINT64_C(0x484f535430523031)

static arbor_status host_invalid_argument(void)
{
    return arbor_status_from_native(-EINVAL);
}

static arbor_status host_ok(void)
{
    return arbor_status_from_native(0);
}

static bool host_spans_overlap(
    const void *left,
    uint64_t left_length,
    const void *right,
    uint64_t right_length)
{
    if (left_length == 0u || right_length == 0u) {
        return false;
    }
    if (left == NULL || right == NULL) {
        return true;
    }
    arbor_asm_result_u64 overlap = range_overlaps(
        (uint64_t)(uintptr_t)left,
        left_length,
        (uint64_t)(uintptr_t)right,
        right_length);
    return overlap.status != 0 || overlap.value != 0u;
}

static bool host_slot_backing_overlaps_region(
    const arbor_example_linux_http_mvc_host_slot *slot,
    const void *region,
    uint64_t region_length)
{
    return host_spans_overlap(
               slot->storage.input.data,
               slot->storage.input.capacity,
               region,
               region_length) ||
        host_spans_overlap(
               slot->storage.output.data,
               slot->storage.output.capacity,
               region,
               region_length) ||
        host_spans_overlap(
               slot->storage.arena.base,
               slot->storage.arena.capacity,
               region,
               region_length);
}

static bool host_slot_backings_overlap(
    const arbor_example_linux_http_mvc_host_slot *left,
    const arbor_example_linux_http_mvc_host_slot *right)
{
    return host_slot_backing_overlaps_region(
               left,
               right->storage.input.data,
               right->storage.input.capacity) ||
        host_slot_backing_overlaps_region(
               left,
               right->storage.output.data,
               right->storage.output.capacity) ||
        host_slot_backing_overlaps_region(
               left,
               right->storage.arena.base,
               right->storage.arena.capacity);
}

static bool host_region_overlaps_application(
    const void *region,
    uint64_t region_length,
    const arbor_http_mvc_application *application)
{
    if (application == NULL || application->mvc_application == NULL ||
        application->mvc_application->catalog == NULL) {
        return true;
    }
    const arbor_mvc_application *mvc_application = application->mvc_application;
    const arbor_mvc_catalog *catalog = mvc_application->catalog;
    arbor_asm_result_u64 route_bytes = u64_mul_checked(
        catalog->route_count,
        (uint64_t)sizeof(arbor_mvc_route));
    arbor_asm_result_u64 middleware_bytes = u64_mul_checked(
        catalog->middleware_count,
        (uint64_t)sizeof(arbor_mvc_middleware_descriptor));
    if (route_bytes.status != 0 || middleware_bytes.status != 0 ||
        host_spans_overlap(region, region_length, application, sizeof(*application)) ||
        host_spans_overlap(
            region,
            region_length,
            mvc_application,
            sizeof(*mvc_application)) ||
        host_spans_overlap(region, region_length, catalog, sizeof(*catalog)) ||
        host_spans_overlap(
            region,
            region_length,
            catalog->routes,
            route_bytes.value) ||
        host_spans_overlap(
            region,
            region_length,
            catalog->middlewares,
            middleware_bytes.value)) {
        return true;
    }
    for (uint64_t i = 0u; i < catalog->route_count; ++i) {
        const arbor_mvc_route *route = &catalog->routes[i];
        arbor_asm_result_u64 index_bytes = u64_mul_checked(
            route->middleware_count,
            (uint64_t)sizeof(uint64_t));
        if (index_bytes.status != 0 ||
            host_spans_overlap(
                region,
                region_length,
                route->method_data,
                route->method_length) ||
            host_spans_overlap(
                region,
                region_length,
                route->pattern_data,
                route->pattern_length) ||
            host_spans_overlap(
                region,
                region_length,
                route->middleware_indices,
                index_bytes.value)) {
            return true;
        }
    }
    return false;
}

static void host_diagnostic(
    const arbor_example_linux_http_mvc_host *host,
    arbor_example_linux_http_mvc_host_diagnostic diagnostic,
    int64_t native_status)
{
    if (host->diagnostic != NULL) {
        host->diagnostic(host->diagnostic_context, diagnostic, native_status);
    }
}

static bool host_slot_storage_valid(
    const arbor_example_linux_http_mvc_host_slot *slot)
{
    if (slot == NULL || slot->active || slot->more_work ||
        slot->storage.input.length != 0u ||
        slot->storage.output.length != 0u ||
        slot->storage.arena.offset != 0u ||
        slot->storage.input.capacity == 0u ||
        slot->storage.output.capacity == 0u ||
        slot->storage.arena.capacity == 0u ||
        slot->storage.input.data == NULL ||
        slot->storage.output.data == NULL ||
        slot->storage.arena.base == NULL) {
        return false;
    }
    if (host_slot_backing_overlaps_region(slot, slot, sizeof(*slot))) {
        return false;
    }
    arbor_asm_result_u64 input_end = range_end_checked(
        (uint64_t)(uintptr_t)slot->storage.input.data,
        slot->storage.input.capacity);
    arbor_asm_result_u64 output_end = range_end_checked(
        (uint64_t)(uintptr_t)slot->storage.output.data,
        slot->storage.output.capacity);
    arbor_asm_result_u64 arena_end = range_end_checked(
        (uint64_t)(uintptr_t)slot->storage.arena.base,
        slot->storage.arena.capacity);
    if (input_end.status != 0 || output_end.status != 0 || arena_end.status != 0) {
        return false;
    }
    arbor_asm_result_u64 input_output = range_overlaps(
        (uint64_t)(uintptr_t)slot->storage.input.data,
        slot->storage.input.capacity,
        (uint64_t)(uintptr_t)slot->storage.output.data,
        slot->storage.output.capacity);
    arbor_asm_result_u64 input_arena = range_overlaps(
        (uint64_t)(uintptr_t)slot->storage.input.data,
        slot->storage.input.capacity,
        (uint64_t)(uintptr_t)slot->storage.arena.base,
        slot->storage.arena.capacity);
    arbor_asm_result_u64 output_arena = range_overlaps(
        (uint64_t)(uintptr_t)slot->storage.output.data,
        slot->storage.output.capacity,
        (uint64_t)(uintptr_t)slot->storage.arena.base,
        slot->storage.arena.capacity);
    return input_output.status == 0 && input_output.value == 0u &&
        input_arena.status == 0 && input_arena.value == 0u &&
        output_arena.status == 0 && output_arena.value == 0u;
}

static arbor_status host_runtime_validate(
    const arbor_example_linux_http_mvc_host *host)
{
    if (host == NULL ||
        host->prepared_guard != ARBOR_EXAMPLE_LINUX_HTTP_MVC_HOST_GUARD ||
        host->application == NULL || host->slots == NULL ||
        host->slot_count == 0u || host->events == NULL ||
        host->event_capacity == 0u ||
        host->event_capacity > (uint64_t)INT_MAX ||
        host->event_wait_ms < 0 || host->event_wait_ms > (int64_t)INT_MAX) {
        return host_invalid_argument();
    }
    if (host->opened) {
        if (host->closed || host->listener_fd < 0 || host->epoll_fd < 0) {
            return host_invalid_argument();
        }
    } else if (host->listener_fd != -1 || host->epoll_fd != -1 ||
               host->listener_readable) {
        return host_invalid_argument();
    }
    return host_ok();
}

arbor_status arbor_example_linux_http_mvc_host_slot_prepare(
    arbor_example_linux_http_mvc_host_slot *slot,
    arbor_mut_span input_storage,
    arbor_mut_span output_storage,
    arbor_mut_span arena_storage)
{
    if (slot == NULL) {
        return host_invalid_argument();
    }
    (void)memset(slot, 0, sizeof(*slot));
    arbor_status status = arbor_runtime_storage_prepare(
        &slot->storage,
        input_storage,
        output_storage,
        arena_storage);
    if (status.native != 0) {
        return status;
    }
    if (!host_slot_storage_valid(slot)) {
        (void)memset(slot, 0, sizeof(*slot));
        return host_invalid_argument();
    }
    return host_ok();
}

arbor_status arbor_example_linux_http_mvc_host_prepare(
    arbor_example_linux_http_mvc_host *host,
    const arbor_http_mvc_application *application,
    arbor_example_linux_http_mvc_host_slot *slots,
    uint64_t slot_count,
    arbor_asm_epoll_event *events,
    uint64_t event_capacity,
    int64_t event_wait_ms,
    arbor_example_linux_http_mvc_host_diagnostic_fn diagnostic,
    void *diagnostic_context)
{
    if (host == NULL || application == NULL || slots == NULL ||
        slot_count == 0u || events == NULL || event_capacity == 0u ||
        event_capacity > (uint64_t)INT_MAX || event_wait_ms < 0 ||
        event_wait_ms > (int64_t)INT_MAX ||
        arbor_http_mvc_application_validate(application).native != 0) {
        return host_invalid_argument();
    }
    arbor_asm_result_u64 slot_bytes = u64_mul_checked(
        slot_count,
        (uint64_t)sizeof(*slots));
    arbor_asm_result_u64 event_bytes = u64_mul_checked(
        event_capacity,
        (uint64_t)sizeof(*events));
    if (slot_bytes.status != 0) {
        return arbor_status_from_native(slot_bytes.status);
    }
    if (event_bytes.status != 0) {
        return arbor_status_from_native(event_bytes.status);
    }
    if (host_spans_overlap(host, sizeof(*host), slots, slot_bytes.value) ||
        host_spans_overlap(host, sizeof(*host), events, event_bytes.value) ||
        host_spans_overlap(slots, slot_bytes.value, events, event_bytes.value) ||
        host_region_overlaps_application(host, sizeof(*host), application) ||
        host_region_overlaps_application(slots, slot_bytes.value, application) ||
        host_region_overlaps_application(events, event_bytes.value, application)) {
        return host_invalid_argument();
    }
    for (uint64_t i = 0u; i < slot_count; ++i) {
        if (!host_slot_storage_valid(&slots[i]) ||
            host_slot_backing_overlaps_region(
                &slots[i], host, sizeof(*host)) ||
            host_slot_backing_overlaps_region(
                &slots[i], slots, slot_bytes.value) ||
            host_slot_backing_overlaps_region(
                &slots[i], events, event_bytes.value) ||
            host_region_overlaps_application(
                slots[i].storage.input.data,
                slots[i].storage.input.capacity,
                application) ||
            host_region_overlaps_application(
                slots[i].storage.output.data,
                slots[i].storage.output.capacity,
                application) ||
            host_region_overlaps_application(
                slots[i].storage.arena.base,
                slots[i].storage.arena.capacity,
                application)) {
            return host_invalid_argument();
        }
        for (uint64_t j = 0u; j < i; ++j) {
            if (host_slot_backings_overlap(&slots[i], &slots[j])) {
                return host_invalid_argument();
            }
        }
    }

    arbor_example_linux_http_mvc_host candidate = {
        .application = application,
        .slots = slots,
        .slot_count = slot_count,
        .events = events,
        .event_capacity = event_capacity,
        .event_wait_ms = event_wait_ms,
        .listener_fd = -1,
        .epoll_fd = -1,
        .diagnostic = diagnostic,
        .diagnostic_context = diagnostic_context,
        .prepared_guard = ARBOR_EXAMPLE_LINUX_HTTP_MVC_HOST_GUARD,
        .listener_readable = false,
        .opened = false,
        .closed = false
    };
    *host = candidate;
    return host_ok();
}

arbor_status arbor_example_linux_http_mvc_host_validate(
    const arbor_example_linux_http_mvc_host *host)
{
    arbor_status status = host_runtime_validate(host);
    if (status.native != 0) {
        return status;
    }
    status = arbor_http_mvc_application_validate(host->application);
    if (status.native != 0) {
        return status;
    }
    for (uint64_t i = 0u; i < host->slot_count; ++i) {
        const arbor_example_linux_http_mvc_host_slot *slot = &host->slots[i];
        if (host->closed && slot->active) {
            return host_invalid_argument();
        }
        if (!slot->active && !host->closed && !host_slot_storage_valid(slot)) {
            return host_invalid_argument();
        }
        if (slot->more_work && !slot->active) {
            return host_invalid_argument();
        }
    }
    return host_ok();
}

arbor_status arbor_example_linux_http_mvc_host_open(
    arbor_example_linux_http_mvc_host *host,
    const void *sockaddr,
    uint64_t sockaddr_length,
    int64_t backlog)
{
    arbor_status status = arbor_example_linux_http_mvc_host_validate(host);
    if (status.native != 0 || host->opened || host->closed || sockaddr == NULL ||
        sockaddr_length == 0u) {
        return host_invalid_argument();
    }

    int64_t listener_fd = -1;
    status = arbor_server_open(sockaddr, sockaddr_length, backlog, &listener_fd);
    if (status.native != 0) {
        host_diagnostic(
            host,
            ARBOR_EXAMPLE_LINUX_HTTP_MVC_HOST_DIAGNOSTIC_LISTEN,
            status.native);
        return status;
    }

    int64_t epoll_fd = -1;
    status = arbor_event_loop_create(listener_fd, &epoll_fd);
    if (status.native != 0) {
        host_diagnostic(
            host,
            ARBOR_EXAMPLE_LINUX_HTTP_MVC_HOST_DIAGNOSTIC_EVENT_LOOP_CREATE,
            status.native);
        (void)close((int)listener_fd);
        return status;
    }

    host->listener_fd = listener_fd;
    host->epoll_fd = epoll_fd;
    host->listener_readable = true;
    host->opened = true;
    return host_ok();
}

static arbor_example_linux_http_mvc_host_slot *host_find_free_slot(
    arbor_example_linux_http_mvc_host *host)
{
    for (uint64_t i = 0u; i < host->slot_count; ++i) {
        if (!host->slots[i].active) {
            return &host->slots[i];
        }
    }
    return NULL;
}

static arbor_example_linux_http_mvc_host_slot *host_find_connection_slot(
    arbor_example_linux_http_mvc_host *host,
    uint64_t event_data)
{
    for (uint64_t i = 0u; i < host->slot_count; ++i) {
        arbor_example_linux_http_mvc_host_slot *slot = &host->slots[i];
        if (slot->active &&
            (uint64_t)(uintptr_t)&slot->storage.connection == event_data) {
            return slot;
        }
    }
    return NULL;
}

static arbor_status host_close_slot(
    arbor_example_linux_http_mvc_host *host,
    arbor_example_linux_http_mvc_host_slot *slot)
{
    if (slot == NULL || !slot->active) {
        return host_ok();
    }
    arbor_status status = host_ok();
    if (slot->storage.connection.state != ARBOR_ASM_CONNECTION_CLOSED) {
        status = arbor_server_close(host->epoll_fd, &slot->storage);
    }
    slot->active = false;
    slot->more_work = false;
    return status;
}

static void host_advance_slot(
    arbor_example_linux_http_mvc_host *host,
    arbor_example_linux_http_mvc_host_slot *slot)
{
    if (slot == NULL || !slot->active) {
        return;
    }
    uint64_t completed = 0u;
    arbor_status status = arbor_http_mvc_server_step(
        &slot->storage,
        host->application,
        host->epoll_fd,
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
        host_diagnostic(
            host,
            ARBOR_EXAMPLE_LINUX_HTTP_MVC_HOST_DIAGNOSTIC_CONNECTION,
            status.native);
        (void)host_close_slot(host, slot);
        return;
    }
    if (slot->storage.connection.state == ARBOR_ASM_CONNECTION_CLOSED) {
        slot->active = false;
    }
}

static bool host_any_more_work(
    const arbor_example_linux_http_mvc_host *host)
{
    for (uint64_t i = 0u; i < host->slot_count; ++i) {
        if (host->slots[i].active && host->slots[i].more_work) {
            return true;
        }
    }
    return false;
}

static arbor_status host_set_listener_readable(
    arbor_example_linux_http_mvc_host *host,
    bool readable)
{
    uint64_t events = (uint64_t)(EPOLLERR | EPOLLHUP);
    if (readable) {
        events |= (uint64_t)EPOLLIN;
    }
    void *data = (void *)(uintptr_t)(uint64_t)host->listener_fd;
    int64_t native = event_epoll_modify(
        host->epoll_fd,
        host->listener_fd,
        events,
        data);
    if (native != 0) {
        host_diagnostic(
            host,
            ARBOR_EXAMPLE_LINUX_HTTP_MVC_HOST_DIAGNOSTIC_EVENT_LOOP,
            native);
        return arbor_status_from_native(native);
    }
    host->listener_readable = readable;
    return host_ok();
}

static arbor_status host_accept_ready(
    arbor_example_linux_http_mvc_host *host)
{
    for (;;) {
        arbor_example_linux_http_mvc_host_slot *slot = host_find_free_slot(host);
        if (slot == NULL) {
            if (host->listener_readable) {
                return host_set_listener_readable(host, false);
            }
            return host_ok();
        }

        int64_t accepted_fd = -1;
        arbor_status status = arbor_server_accept(
            host->listener_fd,
            host->epoll_fd,
            &slot->storage,
            &accepted_fd);
        if (status.native == -EAGAIN) {
            return host_ok();
        }
        if (status.native != 0 || accepted_fd < 0) {
            int64_t diagnostic_status = status.native;
            host_diagnostic(
                host,
                ARBOR_EXAMPLE_LINUX_HTTP_MVC_HOST_DIAGNOSTIC_ACCEPT,
                diagnostic_status);
            return status.native != 0 ? status : arbor_status_from_native(-EIO);
        }
        slot->active = true;
        slot->more_work = false;
    }
}

arbor_status arbor_example_linux_http_mvc_host_step(
    arbor_example_linux_http_mvc_host *host)
{
    arbor_status status = host_runtime_validate(host);
    if (status.native != 0 || !host->opened) {
        return host_invalid_argument();
    }

    for (uint64_t i = 0u; i < host->slot_count; ++i) {
        if (host->slots[i].active && host->slots[i].more_work) {
            host_advance_slot(host, &host->slots[i]);
        }
    }

    if (!host->listener_readable && host_find_free_slot(host) != NULL) {
        status = host_set_listener_readable(host, true);
        if (status.native != 0) {
            return status;
        }
    }

    int64_t timeout = host_any_more_work(host) ? 0 : host->event_wait_ms;
    int64_t event_count = event_epoll_wait(
        host->epoll_fd,
        host->events,
        host->event_capacity,
        timeout);
    if (event_count < 0) {
        host_diagnostic(
            host,
            ARBOR_EXAMPLE_LINUX_HTTP_MVC_HOST_DIAGNOSTIC_EVENT_LOOP,
            event_count);
        return arbor_status_from_native(event_count);
    }

    bool listener_event = false;
    for (int64_t i = 0; i < event_count; ++i) {
        uint64_t event_data = host->events[i].data;
        if (event_data == (uint64_t)host->listener_fd) {
            listener_event = true;
            continue;
        }
        arbor_example_linux_http_mvc_host_slot *slot =
            host_find_connection_slot(host, event_data);
        if (slot != NULL) {
            host_advance_slot(host, slot);
        }
    }
    if (listener_event && host->listener_readable) {
        return host_accept_ready(host);
    }
    return host_ok();
}

arbor_status arbor_example_linux_http_mvc_host_run(
    arbor_example_linux_http_mvc_host *host,
    arbor_example_linux_http_mvc_host_stop_fn stop_requested,
    void *stop_context)
{
    arbor_status status = host_runtime_validate(host);
    if (status.native != 0 || !host->opened || stop_requested == NULL) {
        return host_invalid_argument();
    }
    while (!stop_requested(stop_context)) {
        status = arbor_example_linux_http_mvc_host_step(host);
        if (status.native != 0) {
            return status;
        }
    }
    return host_ok();
}

arbor_status arbor_example_linux_http_mvc_host_close(
    arbor_example_linux_http_mvc_host *host)
{
    arbor_status status = host_runtime_validate(host);
    if (status.native != 0) {
        return status;
    }
    if (!host->opened) {
        return host_ok();
    }

    int64_t first_failure = 0;
    for (uint64_t i = 0u; i < host->slot_count; ++i) {
        status = host_close_slot(host, &host->slots[i]);
        if (status.native != 0 && first_failure == 0) {
            first_failure = status.native;
        }
    }
    if (close((int)host->epoll_fd) != 0 && first_failure == 0) {
        first_failure = -(int64_t)errno;
    }
    if (close((int)host->listener_fd) != 0 && first_failure == 0) {
        first_failure = -(int64_t)errno;
    }
    host->epoll_fd = -1;
    host->listener_fd = -1;
    host->listener_readable = false;
    host->opened = false;
    host->closed = true;
    return arbor_status_from_native(first_failure);
}
