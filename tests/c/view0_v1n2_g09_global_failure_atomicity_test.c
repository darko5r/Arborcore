#include <arborcore/view0_conformance/native.h>
#include "g09.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint64_t support_call_count = 0u;
static uint64_t fail_support_at = 0u;
static bool fail_collect = false;

arbor_status arbor_view0_native_v1n2_g09_test_checked_add(
    uint64_t, uint64_t, uint64_t *);
arbor_status arbor_view0_native_v1n2_g09_test_checked_multiply(
    uint64_t, size_t, size_t *);

void *__real_arbor_view0_native_v1n2_g09_support_calloc(void *, size_t);
void *__wrap_arbor_view0_native_v1n2_g09_support_calloc(void *, size_t);
void *__wrap_arbor_view0_native_v1n2_g09_support_calloc(void *arena, size_t size) {
    support_call_count += 1u;
    if (fail_support_at != 0u && support_call_count == fail_support_at) return NULL;
    return __real_arbor_view0_native_v1n2_g09_support_calloc(arena, size);
}

arbor_status __real_arbor_view0_native_v1n2_g09_collect_anchors(
    arbor_span, arbor_view0_native_v1n2_g09_anchor *, uint64_t,
    arbor_view0_native_v1n2_g09_evaluation *);
arbor_status __wrap_arbor_view0_native_v1n2_g09_collect_anchors(
    arbor_span, arbor_view0_native_v1n2_g09_anchor *, uint64_t,
    arbor_view0_native_v1n2_g09_evaluation *);
arbor_status __wrap_arbor_view0_native_v1n2_g09_collect_anchors(
    arbor_span input, arbor_view0_native_v1n2_g09_anchor *anchors, uint64_t capacity,
    arbor_view0_native_v1n2_g09_evaluation *evaluation) {
    if (fail_collect) return arbor_status_from_native(-EIO);
    return __real_arbor_view0_native_v1n2_g09_collect_anchors(
        input, anchors, capacity, evaluation);
}

static void need(bool condition, const char *message) {
    if (!condition) {
        (void)fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

static arbor_span fixture_span(void) {
    static const char fixture[] =
        "<!doctype html><html><head><title>x</title></head><body>"
        "<div id='h'>first</div><table><colgroup><col span='2'></colgroup>"
        "<thead><tr><th id='h'>A</th><th id='b' scope='col'>B</th></tr></thead>"
        "<tbody><tr><th id='r' scope='row'>R</th><td headers='b r'>D</td></tr>"
        "<tr><td colspan='2'></td></tr></tbody></table></body></html>";
    return (arbor_span){(const uint8_t *)fixture, sizeof(fixture) - 1u};
}

static void invoke_failure(int expected_status) {
    arbor_view0_native_diagnostic diagnostics[128], before[128];
    arbor_view0_native_result result, result_before;
    (void)memset(diagnostics, 0xa5, sizeof(diagnostics));
    (void)memset(&result, 0x5a, sizeof(result));
    (void)memcpy(before, diagnostics, sizeof(before));
    (void)memcpy(&result_before, &result, sizeof(result));
    const arbor_status status = arbor_view0_native_check(
        fixture_span(), diagnostics, 128u, &result);
    need(status.native == expected_status, "injected failure propagated");
    need(memcmp(diagnostics, before, sizeof(before)) == 0, "diagnostic atomicity");
    need(memcmp(&result, &result_before, sizeof(result)) == 0, "result atomicity");
}

static void check_every_support_allocation(void) {
    arbor_view0_native_diagnostic diagnostics[128];
    arbor_view0_native_result result;
    support_call_count = 0u;
    fail_support_at = 0u;
    need(arbor_view0_native_check(fixture_span(), diagnostics, 128u, &result).native == 0,
         "support-allocation census succeeds");
    const uint64_t total = support_call_count;
    need(total != 0u, "support-allocation census is nonempty");
    for (uint64_t ordinal = 1u; ordinal <= total; ordinal += 1u) {
        support_call_count = 0u;
        fail_support_at = ordinal;
        invoke_failure(-ENOMEM);
    }
    fail_support_at = 0u;
    support_call_count = 0u;
}

static void check_collect_failure(void) {
    fail_collect = true;
    invoke_failure(-EIO);
    fail_collect = false;
}

static void check_parser_boundary(void) {
    static const uint8_t byte = 0u;
    arbor_view0_native_diagnostic diagnostics[2], before[2];
    arbor_view0_native_result result, result_before;
    (void)memset(diagnostics, 0xa5, sizeof(diagnostics));
    (void)memset(&result, 0x5a, sizeof(result));
    (void)memcpy(before, diagnostics, sizeof(before));
    (void)memcpy(&result_before, &result, sizeof(result));
    const arbor_status status = arbor_view0_native_check(
        (arbor_span){&byte, ARBOR_VIEW0_NATIVE_MAX_INPUT_BYTES + 1u},
        diagnostics, 2u, &result);
    need(status.native == -EFBIG, "oversized parser input rejected");
    need(memcmp(diagnostics, before, sizeof(before)) == 0,
         "parser-boundary diagnostic atomicity");
    need(memcmp(&result, &result_before, sizeof(result)) == 0,
         "parser-boundary result atomicity");
}

static void check_coordinate_overflow(void) {
    uint64_t value = UINT64_C(0x1122334455667788);
    need(arbor_view0_native_v1n2_g09_test_checked_add(
             UINT64_MAX, 1u, &value).native == -EOVERFLOW,
         "coordinate overflow rejected");
    need(value == UINT64_C(0x1122334455667788),
         "coordinate overflow output atomicity");
}

static void check_allocation_multiplication_overflow(void) {
    size_t value = (size_t)UINT64_C(0x55667788);
    need(arbor_view0_native_v1n2_g09_test_checked_multiply(
             UINT64_MAX, 2u, &value).native == -EOVERFLOW,
         "allocation multiplication overflow rejected");
    need(value == (size_t)UINT64_C(0x55667788),
         "allocation multiplication overflow output atomicity");
}

int main(void) {
    check_parser_boundary();
    check_every_support_allocation();
    check_collect_failure();
    check_coordinate_overflow();
    check_allocation_multiplication_overflow();
    puts("VIEW0_V1N2_G09_PARSER_BOUNDARY_FAILURE_ATOMICITY=PASS");
    puts("VIEW0_V1N2_G09_TRANSIENT_ARENA_FAILURE_ATOMICITY=PASS");
    puts("VIEW0_V1N2_G09_ANCHOR_FAILURE_ATOMICITY=PASS");
    puts("VIEW0_V1N2_G09_COORDINATE_OVERFLOW_ATOMICITY=PASS");
    puts("VIEW0_V1N2_G09_ALLOCATION_MULTIPLICATION_OVERFLOW_ATOMICITY=PASS");
    puts("PASS: VIEW0 V1N2 G09 global mechanism failure atomicity");
    return 0;
}
