#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <arborcore/linux_http_mvc_host.h>

_Static_assert(ARBOR_LINUX_HTTP_MVC_HOST_ABI_VERSION == 1u, "HOST1 ABI version");
_Static_assert(ARBOR_LINUX_HTTP_MVC_HOST_OPTIONS_KNOWN_FLAGS == UINT64_C(0),
    "HOST1 known flags");

_Static_assert(sizeof(arbor_linux_http_mvc_host_options) == 64u, "options size");
_Static_assert(_Alignof(arbor_linux_http_mvc_host_options) == 8u, "options align");
_Static_assert(offsetof(arbor_linux_http_mvc_host_options, abi_version) == 0u,
    "options abi_version");
_Static_assert(offsetof(arbor_linux_http_mvc_host_options, struct_size) == 4u,
    "options struct_size");
_Static_assert(offsetof(arbor_linux_http_mvc_host_options, flags) == 8u,
    "options flags");
_Static_assert(offsetof(arbor_linux_http_mvc_host_options, event_wait_ms) == 16u,
    "options event_wait_ms");
_Static_assert(offsetof(arbor_linux_http_mvc_host_options, drain_timeout_ms) == 24u,
    "options drain_timeout_ms");
_Static_assert(offsetof(arbor_linux_http_mvc_host_options, clock) == 32u,
    "options clock");
_Static_assert(offsetof(arbor_linux_http_mvc_host_options, clock_context) == 40u,
    "options clock_context");
_Static_assert(offsetof(arbor_linux_http_mvc_host_options, diagnostic) == 48u,
    "options diagnostic");
_Static_assert(offsetof(arbor_linux_http_mvc_host_options, diagnostic_context) == 56u,
    "options diagnostic_context");

_Static_assert(sizeof(arbor_linux_http_mvc_host_shutdown_result) == 56u,
    "shutdown result size");
_Static_assert(_Alignof(arbor_linux_http_mvc_host_shutdown_result) == 8u,
    "shutdown result align");
_Static_assert(offsetof(arbor_linux_http_mvc_host_shutdown_result,
    active_at_drain_start) == 0u, "result active");
_Static_assert(offsetof(arbor_linux_http_mvc_host_shutdown_result,
    inactive_before_deadline) == 8u, "result inactive");
_Static_assert(offsetof(arbor_linux_http_mvc_host_shutdown_result,
    forced_at_deadline) == 16u, "result forced");
_Static_assert(offsetof(arbor_linux_http_mvc_host_shutdown_result,
    drain_start_ms) == 24u, "result start");
_Static_assert(offsetof(arbor_linux_http_mvc_host_shutdown_result,
    drain_finish_ms) == 32u, "result finish");
_Static_assert(offsetof(arbor_linux_http_mvc_host_shutdown_result,
    first_failure) == 40u, "result failure");
_Static_assert(offsetof(arbor_linux_http_mvc_host_shutdown_result,
    deadline_expired) == 48u, "result expiry");

_Static_assert(sizeof(arbor_linux_http_mvc_host_slot) == 272u, "slot size");
_Static_assert(_Alignof(arbor_linux_http_mvc_host_slot) == 8u, "slot align");
_Static_assert(offsetof(arbor_linux_http_mvc_host_slot, storage) == 0u,
    "slot storage");
_Static_assert(offsetof(arbor_linux_http_mvc_host_slot, active) == 264u,
    "slot active");
_Static_assert(offsetof(arbor_linux_http_mvc_host_slot, more_work) == 265u,
    "slot more_work");

_Static_assert(sizeof(arbor_linux_http_mvc_host) == 184u, "host size");
_Static_assert(_Alignof(arbor_linux_http_mvc_host) == 8u, "host align");
_Static_assert(offsetof(arbor_linux_http_mvc_host, application) == 0u,
    "host application");
_Static_assert(offsetof(arbor_linux_http_mvc_host, slots) == 8u, "host slots");
_Static_assert(offsetof(arbor_linux_http_mvc_host, slot_count) == 16u,
    "host slot_count");
_Static_assert(offsetof(arbor_linux_http_mvc_host, events) == 24u, "host events");
_Static_assert(offsetof(arbor_linux_http_mvc_host, event_capacity) == 32u,
    "host event_capacity");
_Static_assert(offsetof(arbor_linux_http_mvc_host, event_wait_ms) == 40u,
    "host event_wait_ms");
_Static_assert(offsetof(arbor_linux_http_mvc_host, drain_timeout_ms) == 48u,
    "host drain_timeout_ms");
_Static_assert(offsetof(arbor_linux_http_mvc_host, listener_fd) == 56u,
    "host listener_fd");
_Static_assert(offsetof(arbor_linux_http_mvc_host, epoll_fd) == 64u,
    "host epoll_fd");
_Static_assert(offsetof(arbor_linux_http_mvc_host, drain_deadline_ms) == 72u,
    "host drain_deadline_ms");
_Static_assert(offsetof(arbor_linux_http_mvc_host, clock) == 80u, "host clock");
_Static_assert(offsetof(arbor_linux_http_mvc_host, clock_context) == 88u,
    "host clock_context");
_Static_assert(offsetof(arbor_linux_http_mvc_host, diagnostic) == 96u,
    "host diagnostic");
_Static_assert(offsetof(arbor_linux_http_mvc_host, diagnostic_context) == 104u,
    "host diagnostic_context");
_Static_assert(offsetof(arbor_linux_http_mvc_host, shutdown_result) == 112u,
    "host shutdown_result");
_Static_assert(offsetof(arbor_linux_http_mvc_host, prepared_guard) == 168u,
    "host prepared_guard");
_Static_assert(offsetof(arbor_linux_http_mvc_host, phase) == 176u, "host phase");
_Static_assert(offsetof(arbor_linux_http_mvc_host, listener_readable) == 180u,
    "host listener_readable");

_Static_assert(ARBOR_LINUX_HTTP_MVC_HOST_DIAGNOSTIC_LISTEN == 1,
    "listen diagnostic");
_Static_assert(ARBOR_LINUX_HTTP_MVC_HOST_DIAGNOSTIC_EVENT_LOOP_CREATE == 2,
    "event-loop-create diagnostic");
_Static_assert(ARBOR_LINUX_HTTP_MVC_HOST_DIAGNOSTIC_EVENT_LOOP == 3,
    "event-loop diagnostic");
_Static_assert(ARBOR_LINUX_HTTP_MVC_HOST_DIAGNOSTIC_ACCEPT == 4,
    "accept diagnostic");
_Static_assert(ARBOR_LINUX_HTTP_MVC_HOST_DIAGNOSTIC_CONNECTION == 5,
    "connection diagnostic");
_Static_assert(ARBOR_LINUX_HTTP_MVC_HOST_DIAGNOSTIC_CLOCK == 6,
    "clock diagnostic");
_Static_assert(ARBOR_LINUX_HTTP_MVC_HOST_DIAGNOSTIC_CLOSE == 7,
    "close diagnostic");
_Static_assert(ARBOR_LINUX_HTTP_MVC_HOST_PHASE_PREPARED == 1, "prepared phase");
_Static_assert(ARBOR_LINUX_HTTP_MVC_HOST_PHASE_ACCEPTING == 2, "accepting phase");
_Static_assert(ARBOR_LINUX_HTTP_MVC_HOST_PHASE_DRAINING == 3, "draining phase");
_Static_assert(ARBOR_LINUX_HTTP_MVC_HOST_PHASE_CLOSED == 4, "closed phase");

typedef arbor_status (*host1_options_make_fn)(
    arbor_linux_http_mvc_host_options *,
    int64_t,
    uint64_t,
    arbor_linux_http_mvc_host_clock_fn,
    void *,
    arbor_linux_http_mvc_host_diagnostic_fn,
    void *);
typedef arbor_status (*host1_slot_prepare_fn)(
    arbor_linux_http_mvc_host_slot *,
    arbor_mut_span,
    arbor_mut_span,
    arbor_mut_span);
typedef arbor_status (*host1_prepare_fn)(
    arbor_linux_http_mvc_host *,
    const arbor_http_mvc_application *,
    arbor_linux_http_mvc_host_slot *,
    uint64_t,
    arbor_asm_epoll_event *,
    uint64_t,
    const arbor_linux_http_mvc_host_options *);
typedef arbor_status (*host1_validate_fn)(
    const arbor_linux_http_mvc_host *);
typedef arbor_status (*host1_open_fn)(
    arbor_linux_http_mvc_host *,
    const void *,
    uint64_t,
    int64_t);
typedef arbor_status (*host1_step_fn)(arbor_linux_http_mvc_host *);
typedef arbor_status (*host1_begin_drain_fn)(arbor_linux_http_mvc_host *);
typedef arbor_status (*host1_run_fn)(
    arbor_linux_http_mvc_host *,
    arbor_linux_http_mvc_host_stop_fn,
    void *);
typedef arbor_status (*host1_close_fn)(arbor_linux_http_mvc_host *);
typedef arbor_status (*host1_shutdown_result_get_fn)(
    const arbor_linux_http_mvc_host *,
    arbor_linux_http_mvc_host_shutdown_result *);

#define HOST1_ASSERT_SIGNATURE(function_name, function_type) \
    _Static_assert( \
        _Generic(&(function_name), function_type: 1, default: 0), \
        #function_name " signature")

HOST1_ASSERT_SIGNATURE(arbor_linux_http_mvc_host_options_make,
    host1_options_make_fn);
HOST1_ASSERT_SIGNATURE(arbor_linux_http_mvc_host_slot_prepare,
    host1_slot_prepare_fn);
HOST1_ASSERT_SIGNATURE(arbor_linux_http_mvc_host_prepare, host1_prepare_fn);
HOST1_ASSERT_SIGNATURE(arbor_linux_http_mvc_host_validate, host1_validate_fn);
HOST1_ASSERT_SIGNATURE(arbor_linux_http_mvc_host_open, host1_open_fn);
HOST1_ASSERT_SIGNATURE(arbor_linux_http_mvc_host_step, host1_step_fn);
HOST1_ASSERT_SIGNATURE(arbor_linux_http_mvc_host_begin_drain,
    host1_begin_drain_fn);
HOST1_ASSERT_SIGNATURE(arbor_linux_http_mvc_host_run, host1_run_fn);
HOST1_ASSERT_SIGNATURE(arbor_linux_http_mvc_host_close, host1_close_fn);
HOST1_ASSERT_SIGNATURE(arbor_linux_http_mvc_host_shutdown_result_get,
    host1_shutdown_result_get_fn);

#undef HOST1_ASSERT_SIGNATURE

static int fail(const char *message)
{
    (void)fprintf(stderr, "FAIL: %s\n", message);
    return 1;
}

static int64_t test_clock(void *context)
{
    return context == NULL ? -EINVAL : *(const int64_t *)context;
}

static void test_diagnostic(
    void *context,
    arbor_linux_http_mvc_host_diagnostic diagnostic,
    int64_t native_status)
{
    (void)context;
    (void)diagnostic;
    (void)native_status;
}

int main(void)
{
    int64_t clock_value = 17;
    int diagnostic_context = 23;
    arbor_linux_http_mvc_host_options options;
    (void)memset(&options, 0xa5, sizeof(options));
    arbor_linux_http_mvc_host_options sentinel = options;

    if (arbor_linux_http_mvc_host_options_make(
            NULL, 0, 0u, NULL, NULL, NULL, NULL).native != -EINVAL ||
        arbor_linux_http_mvc_host_options_make(
            &options, -1, 0u, NULL, NULL, NULL, NULL).native != -EINVAL ||
        memcmp(&options, &sentinel, sizeof(options)) != 0 ||
        arbor_linux_http_mvc_host_options_make(
            &options,
            (int64_t)INT_MAX + INT64_C(1),
            0u,
            NULL,
            NULL,
            NULL,
            NULL).native != -EINVAL ||
        memcmp(&options, &sentinel, sizeof(options)) != 0) {
        return fail("options constructor validation and failure atomicity");
    }

    if (arbor_linux_http_mvc_host_options_make(
            &options,
            INT_MAX,
            UINT64_MAX,
            test_clock,
            &clock_value,
            test_diagnostic,
            &diagnostic_context).native != 0 ||
        options.abi_version != ARBOR_LINUX_HTTP_MVC_HOST_ABI_VERSION ||
        options.struct_size != (uint32_t)sizeof(options) ||
        options.flags != ARBOR_LINUX_HTTP_MVC_HOST_OPTIONS_KNOWN_FLAGS ||
        options.event_wait_ms != INT_MAX ||
        options.drain_timeout_ms != UINT64_MAX ||
        options.clock != test_clock || options.clock_context != &clock_value ||
        options.diagnostic != test_diagnostic ||
        options.diagnostic_context != &diagnostic_context) {
        return fail("options constructor exact explicit publication");
    }

    puts("PASS: HOST1 public C include, layout, options and archive linkage");
    return 0;
}
