#include <arborcore/view0_conformance/native.h>
#include "g10.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint64_t support_calls = 0u;
static uint64_t fail_support_at = 0u;
static bool fail_collect = false;
static bool fail_measure_count = false;

void *__real_arbor_view0_native_v1n2_g10_support_calloc(void *, size_t);
void *__wrap_arbor_view0_native_v1n2_g10_support_calloc(void *, size_t);
void *__wrap_arbor_view0_native_v1n2_g10_support_calloc(void *arena, size_t size) {
    support_calls += 1u;
    if (fail_support_at != 0u && support_calls == fail_support_at) return NULL;
    return __real_arbor_view0_native_v1n2_g10_support_calloc(arena, size);
}

arbor_status __real_arbor_view0_native_v1n2_g10_collect_anchors(
    arbor_span, arbor_view0_native_v1n2_g10_anchor *, uint64_t,
    arbor_view0_native_v1n2_g10_evaluation *);
arbor_status __wrap_arbor_view0_native_v1n2_g10_collect_anchors(
    arbor_span, arbor_view0_native_v1n2_g10_anchor *, uint64_t,
    arbor_view0_native_v1n2_g10_evaluation *);
arbor_status __wrap_arbor_view0_native_v1n2_g10_collect_anchors(
    arbor_span input, arbor_view0_native_v1n2_g10_anchor *anchors, uint64_t capacity,
    arbor_view0_native_v1n2_g10_evaluation *evaluation) {
    if (fail_collect) return arbor_status_from_native(-EIO);
    return __real_arbor_view0_native_v1n2_g10_collect_anchors(
        input, anchors, capacity, evaluation);
}

arbor_status __real_arbor_view0_native_v1n2_g10_measure(
    arbor_span, arbor_view0_native_v1n2_g10_evaluation *);
arbor_status __wrap_arbor_view0_native_v1n2_g10_measure(
    arbor_span, arbor_view0_native_v1n2_g10_evaluation *);
arbor_status __wrap_arbor_view0_native_v1n2_g10_measure(
    arbor_span input, arbor_view0_native_v1n2_g10_evaluation *evaluation) {
    if (fail_measure_count) {
        if (evaluation == NULL) return arbor_status_from_native(-EINVAL);
        (void)memset(evaluation, 0, sizeof(*evaluation));
        evaluation->diagnostic_count = UINT64_MAX;
        return arbor_status_from_native(0);
    }
    return __real_arbor_view0_native_v1n2_g10_measure(input, evaluation);
}

static void need(bool condition, const char *message) {
    if (!condition) { (void)fprintf(stderr, "FAIL: %s\n", message); exit(1); }
}

static arbor_span fixture(void) {
    static const char html[] =
        "<!doctype html><html><head><title>x</title></head><body>"
        "<form id=f accept-charset=latin1><label for=missing>x</label>"
        "<input type=image alt=x></form></body></html>";
    return (arbor_span){(const uint8_t *)html, sizeof(html) - 1u};
}

static arbor_span overflow_fixture(const char *control, char *storage, size_t capacity) {
    static const char prefix[] =
        "<!doctype html><html><head><title>x</title></head><body>";
    static const char suffix[] = "</body></html>";
    const size_t control_length = strlen(control);
    const size_t length = sizeof(prefix) - 1u + control_length + sizeof(suffix) - 1u;
    need(length < capacity, "overflow fixture capacity");
    (void)memcpy(storage, prefix, sizeof(prefix) - 1u);
    (void)memcpy(storage + sizeof(prefix) - 1u, control, control_length);
    (void)memcpy(storage + sizeof(prefix) - 1u + control_length,
                 suffix, sizeof(suffix) - 1u);
    return (arbor_span){(const uint8_t *)storage, (uint64_t)length};
}

static void invoke_domain_overflow(const char *control) {
    char html[512];
    const arbor_span input = overflow_fixture(control, html, sizeof(html));
    arbor_view0_native_v1n2_g10_evaluation evaluation, evaluation_before;
    (void)memset(&evaluation, 0x5a, sizeof(evaluation));
    (void)memcpy(&evaluation_before, &evaluation, sizeof(evaluation));
    need(arbor_view0_native_v1n2_g10_measure(input, &evaluation).native == -EOVERFLOW,
         "direct G10 domain overflow propagated");
    need(memcmp(&evaluation, &evaluation_before, sizeof(evaluation)) == 0,
         "direct G10 domain overflow output atomicity");

    arbor_view0_native_diagnostic diagnostics[64], diagnostics_before[64];
    arbor_view0_native_result result, result_before;
    (void)memset(diagnostics, 0xa5, sizeof(diagnostics));
    (void)memset(&result, 0x5a, sizeof(result));
    (void)memcpy(diagnostics_before, diagnostics, sizeof(diagnostics));
    (void)memcpy(&result_before, &result, sizeof(result));
    need(arbor_view0_native_check(input, diagnostics, 64u, &result).native == -EOVERFLOW,
         "global domain overflow propagated");
    need(memcmp(diagnostics, diagnostics_before, sizeof(diagnostics)) == 0,
         "global domain overflow diagnostic atomicity");
    need(memcmp(&result, &result_before, sizeof(result)) == 0,
         "global domain overflow result atomicity");
}

static void invoke_sr2_domain_overflows(void) {
    invoke_domain_overflow(
        "<input type=number value=18446744073709551616 min=0 step=1 name=x>");
    invoke_domain_overflow(
        "<input type=number value=1e999999 min=0 step=1 name=x>");
    invoke_domain_overflow(
        "<input type=date value=18446744073709551616-01-01 min=0001-01-01 name=x>");
    invoke_domain_overflow(
        "<input type=date value=60000000000000000-01-01 min=0001-01-01 name=x>");
    invoke_domain_overflow(
        "<input type=month value=2000000000000000000-01 min=0001-01 name=x>");
    invoke_domain_overflow(
        "<input type=week value=60000000000000000-W01 min=0001-W01 name=x>");
    invoke_domain_overflow(
        "<input type=datetime-local value=1000000000000-01-01T00:00:00 "
        "min=0001-01-01T00:00:00 name=x>");
    invoke_domain_overflow(
        "<input type=week value=2026-W03 min=2026-W01 "
        "step=18446744073709551615 name=x>");
}

static void invoke_checked_total_overflow(void) {
    char html[512];
    const arbor_span input = overflow_fixture("<img alt=x>", html, sizeof(html));
    arbor_view0_native_diagnostic diagnostics[64], diagnostics_before[64];
    arbor_view0_native_result result, result_before;
    (void)memset(diagnostics, 0xa5, sizeof(diagnostics));
    (void)memset(&result, 0x5a, sizeof(result));
    (void)memcpy(diagnostics_before, diagnostics, sizeof(diagnostics));
    (void)memcpy(&result_before, &result, sizeof(result));
    fail_measure_count = true;
    const int64_t observed = arbor_view0_native_check(
        input, diagnostics, UINT64_C(64), &result).native;
    fail_measure_count = false;
    need(observed == -EOVERFLOW, "checked total overflow propagated");
    need(memcmp(diagnostics, diagnostics_before, sizeof(diagnostics)) == 0,
         "checked total overflow diagnostic atomicity");
    need(memcmp(&result, &result_before, sizeof(result)) == 0,
         "checked total overflow result atomicity");
}

static void invoke_failure(int64_t expected) {
    arbor_view0_native_diagnostic diagnostics[64], before[64];
    arbor_view0_native_result result, result_before;
    (void)memset(diagnostics, 0xa5, sizeof(diagnostics));
    (void)memset(&result, 0x5a, sizeof(result));
    (void)memcpy(before, diagnostics, sizeof(before));
    (void)memcpy(&result_before, &result, sizeof(result));
    const int64_t observed = arbor_view0_native_check(
        fixture(), diagnostics, 64u, &result).native;
    if (observed != expected) {
        (void)fprintf(stderr, "FAIL: injected failure expected=%lld observed=%lld\n",
                      (long long)expected, (long long)observed);
        exit(1);
    }
    need(memcmp(diagnostics, before, sizeof(before)) == 0, "diagnostic atomicity");
    need(memcmp(&result, &result_before, sizeof(result)) == 0, "result atomicity");
}

static void invoke_parser_failure(void) {
    arbor_view0_native_diagnostic diagnostics[4], before[4];
    arbor_view0_native_result result, result_before;
    (void)memset(diagnostics, 0xa5, sizeof(diagnostics));
    (void)memset(&result, 0x5a, sizeof(result));
    (void)memcpy(before, diagnostics, sizeof(before));
    (void)memcpy(&result_before, &result, sizeof(result));
    need(arbor_view0_native_check((arbor_span){NULL, UINT64_C(1)},
        diagnostics, UINT64_C(4), &result).native != 0,
        "parser boundary failure propagated");
    need(memcmp(diagnostics, before, sizeof(before)) == 0,
         "parser diagnostic atomicity");
    need(memcmp(&result, &result_before, sizeof(result)) == 0,
         "parser result atomicity");
}

static void invoke_checked_count_failure(void) {
    arbor_view0_native_v1n2_g10_evaluation measured = {0};
    need(arbor_view0_native_v1n2_g10_measure(fixture(), &measured).native == 0,
         "count-failure census");
    arbor_view0_native_v1n2_g10_anchor anchors[64] = {0};
    arbor_view0_native_v1n2_g10_evaluation output, before;
    (void)memset(&output, 0x5a, sizeof(output));
    (void)memcpy(&before, &output, sizeof(before));
    need(measured.diagnostic_count < UINT64_C(64), "count-failure fixture bound");
    need(arbor_view0_native_v1n2_g10_collect_anchors(
        fixture(), anchors, measured.diagnostic_count + UINT64_C(1), &output).native != 0,
        "checked count mismatch propagated");
    need(memcmp(&output, &before, sizeof(output)) == 0,
         "checked count output atomicity");
}

int main(void) {
    arbor_view0_native_diagnostic diagnostics[64];
    arbor_view0_native_result result;
    support_calls = 0u;
    need(arbor_view0_native_check(fixture(), diagnostics, 64u, &result).native == 0,
         "support census");
    const uint64_t count = support_calls;
    need(count != 0u, "support census nonempty");
    for (uint64_t i = 1u; i <= count; ++i) {
        support_calls = 0u; fail_support_at = i; invoke_failure(-ENOMEM);
    }
    fail_support_at = 0u;
    fail_collect = true; invoke_failure(-EIO); fail_collect = false;
    invoke_parser_failure();
    invoke_checked_count_failure();
    invoke_sr2_domain_overflows();
    invoke_checked_total_overflow();
    puts("VIEW0_V1N2_G10_PARSER_BOUNDARY_FAILURE_ATOMICITY=PASS");
    puts("VIEW0_V1N2_G10_TRANSIENT_ARENA_FAILURE_ATOMICITY=PASS");
    puts("VIEW0_V1N2_G10_ANCHOR_FAILURE_ATOMICITY=PASS");
    puts("VIEW0_V1N2_G10_CHECKED_COUNT_FAILURE_ATOMICITY=PASS");
    puts("VIEW0_V1N2_G10_SR2_DOMAIN_OVERFLOW_CASES=S214_S223_EXECUTED");
    puts("VIEW0_V1N2_G10_SR2_CHECKED_TOTAL_OVERFLOW_CASE=S224_EXECUTED");
    puts("PASS: VIEW0 V1N2 G10 global mechanism failure atomicity");
    return 0;
}
