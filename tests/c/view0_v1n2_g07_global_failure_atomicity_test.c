#include <arborcore/view0_conformance/native.h>
#include "g07.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool fail_collect = false;
static bool fail_final_parse = false;

arbor_status __real_arbor_view0_native_v1n2_g07_collect_anchors(
    arbor_span, arbor_view0_native_v1n2_g07_anchor *, uint64_t,
    arbor_view0_native_v1n2_g07_evaluation *);
arbor_status __wrap_arbor_view0_native_v1n2_g07_collect_anchors(
    arbor_span, arbor_view0_native_v1n2_g07_anchor *, uint64_t,
    arbor_view0_native_v1n2_g07_evaluation *);
arbor_status __wrap_arbor_view0_native_v1n2_g07_collect_anchors(
    arbor_span input, arbor_view0_native_v1n2_g07_anchor *anchors, uint64_t capacity,
    arbor_view0_native_v1n2_g07_evaluation *evaluation) {
    if (fail_collect) return arbor_status_from_native(-EIO);
    return __real_arbor_view0_native_v1n2_g07_collect_anchors(input, anchors, capacity, evaluation);
}

arbor_status __real_arbor_view0_native_lexbor_collect_exact(
    arbor_span, arbor_view0_native_diagnostic *, uint64_t,
    arbor_view0_native_parse_counts *, arbor_view0_native_document_facts *);
arbor_status __wrap_arbor_view0_native_lexbor_collect_exact(
    arbor_span, arbor_view0_native_diagnostic *, uint64_t,
    arbor_view0_native_parse_counts *, arbor_view0_native_document_facts *);
arbor_status __wrap_arbor_view0_native_lexbor_collect_exact(
    arbor_span input, arbor_view0_native_diagnostic *diagnostics, uint64_t capacity,
    arbor_view0_native_parse_counts *counts, arbor_view0_native_document_facts *facts) {
    if (fail_final_parse) return arbor_status_from_native(-EIO);
    return __real_arbor_view0_native_lexbor_collect_exact(input, diagnostics, capacity, counts, facts);
}

static void need(bool condition, const char *message) {
    if (!condition) { (void)fprintf(stderr, "FAIL: %s\n", message); exit(1); }
}

static void check_failure(bool collect_failure) {
    static const char fixture[] =
        "<!doctype html><html><head><title>x</title></head><body>"
        "<a href='http://'>x</a></body></html>";
    arbor_view0_native_diagnostic diagnostics[16];
    arbor_view0_native_result result;
    arbor_view0_native_diagnostic before_diagnostics[16];
    arbor_view0_native_result before_result;
    (void)memset(diagnostics, 0xa5, sizeof(diagnostics));
    (void)memset(&result, 0x5a, sizeof(result));
    (void)memcpy(before_diagnostics, diagnostics, sizeof(diagnostics));
    (void)memcpy(&before_result, &result, sizeof(result));
    fail_collect = collect_failure;
    fail_final_parse = !collect_failure;
    const arbor_status status = arbor_view0_native_check(
        (arbor_span){(const uint8_t *)fixture, sizeof(fixture) - 1u}, diagnostics, 16u, &result);
    need(status.native == -EIO, "injected failure propagated");
    need(memcmp(diagnostics, before_diagnostics, sizeof(diagnostics)) == 0,
         "diagnostic publication atomicity");
    need(memcmp(&result, &before_result, sizeof(result)) == 0, "result publication atomicity");
    fail_collect = false;
    fail_final_parse = false;
}

int main(void) {
    check_failure(true);
    check_failure(false);
    puts("VIEW0_V1N2_G07_ANCHOR_FAILURE_ATOMICITY=PASS");
    puts("VIEW0_V1N2_G07_FINAL_LEXBOR_FAILURE_ATOMICITY=PASS");
    puts("PASS: VIEW0 V1N2 G07 global mechanism failure atomicity");
    return 0;
}
