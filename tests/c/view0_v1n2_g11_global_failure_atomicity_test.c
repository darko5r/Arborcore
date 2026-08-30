#include <arborcore/view0_conformance/native.h>
#include "g11.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint64_t calls;
static uint64_t fail_at;
static bool fail_collect;

void *__real_arbor_view0_native_v1n2_g11_support_calloc(void *, size_t);
void *__wrap_arbor_view0_native_v1n2_g11_support_calloc(void *, size_t);
void *__wrap_arbor_view0_native_v1n2_g11_support_calloc(void *arena, size_t size) {
    calls += 1u;
    if (fail_at != 0u && calls == fail_at) return NULL;
    return __real_arbor_view0_native_v1n2_g11_support_calloc(arena, size);
}

arbor_status __real_arbor_view0_native_v1n2_g11_collect_anchors(
    arbor_span, arbor_view0_native_v1n2_g11_anchor *, uint64_t,
    arbor_view0_native_v1n2_g11_evaluation *);
arbor_status __wrap_arbor_view0_native_v1n2_g11_collect_anchors(
    arbor_span, arbor_view0_native_v1n2_g11_anchor *, uint64_t,
    arbor_view0_native_v1n2_g11_evaluation *);
arbor_status __wrap_arbor_view0_native_v1n2_g11_collect_anchors(
    arbor_span input, arbor_view0_native_v1n2_g11_anchor *anchors, uint64_t capacity,
    arbor_view0_native_v1n2_g11_evaluation *evaluation) {
    if (fail_collect) return arbor_status_from_native(-EIO);
    return __real_arbor_view0_native_v1n2_g11_collect_anchors(
        input, anchors, capacity, evaluation);
}

static void need(bool condition, const char *message) {
    if (!condition) { (void)fprintf(stderr, "FAIL: %s\n", message); exit(1); }
}

static arbor_span input(void) {
    static const char value[] = "<!doctype html><title>x</title><details name=x open></details>"
        "<details name=x open></details><dialog open popover=manual></dialog>";
    return (arbor_span){(const uint8_t *)value, sizeof(value) - 1u};
}

static void invoke_failure(int expected) {
    arbor_view0_native_diagnostic diagnostics[32], before[32];
    arbor_view0_native_result result, result_before;
    (void)memset(diagnostics, 0xa5, sizeof(diagnostics));
    (void)memset(&result, 0x5a, sizeof(result));
    (void)memcpy(before, diagnostics, sizeof(before));
    (void)memcpy(&result_before, &result, sizeof(result));
    need(arbor_view0_native_check(input(), diagnostics, 32u, &result).native == expected,
         "failure propagated");
    need(memcmp(diagnostics, before, sizeof(before)) == 0, "diagnostic atomicity");
    need(memcmp(&result, &result_before, sizeof(result)) == 0, "result atomicity");
}

int main(void) {
    arbor_view0_native_diagnostic diagnostics[32];
    arbor_view0_native_result result;
    calls = 0u; fail_at = 0u;
    need(arbor_view0_native_check(input(), diagnostics, 32u, &result).native == 0,
         "allocation census");
    const uint64_t total = calls;
    need(total != 0u, "allocation seam exercised");
    for (uint64_t i = 1u; i <= total; ++i) {
        calls = 0u; fail_at = i; invoke_failure(-ENOMEM);
    }
    fail_at = 0u; calls = 0u;
    fail_collect = true; invoke_failure(-EIO); fail_collect = false;
    puts("VIEW0_V1N2_G11_TRANSIENT_ARENA_FAILURE_ATOMICITY=PASS");
    puts("VIEW0_V1N2_G11_ANCHOR_FAILURE_ATOMICITY=PASS");
    puts("PASS: VIEW0 V1N2 G11 global mechanism failure atomicity");
    return 0;
}
