#include <arborcore/view0_conformance/native.h>
#include "g04_r2a.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef enum injection_mode {
    INJECTION_NONE = 0,
    INJECTION_R2_ANCHOR_FAILURE = 1,
    INJECTION_FINAL_FRAGMENT_PUBLICATION_FAILURE = 2
} injection_mode;

static injection_mode current_mode = INJECTION_NONE;

arbor_status __real_arbor_view0_native_g04_r2a_collect_fragment_anchors(
    arbor_span input,
    arbor_view0_native_source_anchor *anchors,
    uint64_t anchor_capacity,
    arbor_view0_native_g04_r2a_evaluation *evaluation_out);

arbor_status __wrap_arbor_view0_native_g04_r2a_collect_fragment_anchors(
    arbor_span input,
    arbor_view0_native_source_anchor *anchors,
    uint64_t anchor_capacity,
    arbor_view0_native_g04_r2a_evaluation *evaluation_out);

arbor_status __wrap_arbor_view0_native_g04_r2a_collect_fragment_anchors(
    arbor_span input,
    arbor_view0_native_source_anchor *anchors,
    uint64_t anchor_capacity,
    arbor_view0_native_g04_r2a_evaluation *evaluation_out)
{
    if (current_mode == INJECTION_R2_ANCHOR_FAILURE)
        return arbor_status_from_native(-(int64_t)ENOMEM);
    return __real_arbor_view0_native_g04_r2a_collect_fragment_anchors(
        input, anchors, anchor_capacity, evaluation_out);
}

arbor_status __real_arbor_view0_native_lexbor_fragment_collect_exact(
    arbor_span input,
    arbor_view0_native_diagnostic *diagnostics,
    uint64_t diagnostic_capacity,
    const arbor_view0_native_parse_counts *expected_counts);

arbor_status __wrap_arbor_view0_native_lexbor_fragment_collect_exact(
    arbor_span input,
    arbor_view0_native_diagnostic *diagnostics,
    uint64_t diagnostic_capacity,
    const arbor_view0_native_parse_counts *expected_counts);

arbor_status __wrap_arbor_view0_native_lexbor_fragment_collect_exact(
    arbor_span input,
    arbor_view0_native_diagnostic *diagnostics,
    uint64_t diagnostic_capacity,
    const arbor_view0_native_parse_counts *expected_counts)
{
    if (current_mode == INJECTION_FINAL_FRAGMENT_PUBLICATION_FAILURE)
        return arbor_status_from_native(-(int64_t)ENOMEM);
    return __real_arbor_view0_native_lexbor_fragment_collect_exact(
        input, diagnostics, diagnostic_capacity, expected_counts);
}

static int outputs_unchanged(injection_mode mode)
{
    static const char html[] = "<a><html></html></a>";
    arbor_view0_native_diagnostic diagnostics[64], before_diagnostics[64];
    arbor_view0_native_result result, before_result;
    (void)memset(diagnostics, 0x6a, sizeof(diagnostics));
    (void)memcpy(before_diagnostics, diagnostics, sizeof(diagnostics));
    (void)memset(&result, 0xa6, sizeof(result));
    before_result = result;
    current_mode = mode;
    const arbor_status status = arbor_view0_native_check_fragment_model(
        (arbor_span){(const uint8_t *)html, (uint64_t)(sizeof(html) - 1u)},
        diagnostics, 64u, &result);
    current_mode = INJECTION_NONE;
    if (status.native != -(int64_t)ENOMEM) return 1;
    if (memcmp(diagnostics, before_diagnostics, sizeof(diagnostics)) != 0) return 2;
    if (memcmp(&result, &before_result, sizeof(result)) != 0) return 3;
    return 0;
}

static int anchor_equivalence(void)
{
    static const char html[] = "<a><html></html></a>";
    const arbor_span input = {(const uint8_t *)html, (uint64_t)(sizeof(html) - 1u)};
    arbor_view0_native_g04_r2a_evaluation measured = {0};
    arbor_status status = arbor_view0_native_g04_r2a_measure_fragment_model(input, &measured);
    if (status.native != 0 || measured.diagnostic_count != 1u) return 1;
    arbor_view0_native_source_anchor anchors[2] = {{0}};
    arbor_view0_native_g04_r2a_evaluation collected = {0};
    status = arbor_view0_native_g04_r2a_collect_fragment_anchors(
        input, anchors, 1u, &collected);
    if (status.native != 0 || memcmp(&measured, &collected, sizeof(measured)) != 0)
        return 2;
    arbor_view0_native_diagnostic diagnostic = {0};
    arbor_view0_native_g04_r2a_materialize_anchor(&anchors[0], 17u, &diagnostic);
    if (diagnostic.rule_id != ARBOR_VIEW_V1_G04_TRANSPARENT_PARENTLESS_FLOW ||
        diagnostic.byte_offset != 4u || diagnostic.source_length != 4u ||
        diagnostic.discovery_sequence != 17u)
        return 3;
    return 0;
}

int main(void)
{
    int rc = outputs_unchanged(INJECTION_R2_ANCHOR_FAILURE);
    if (rc != 0) return 10 + rc;
    rc = outputs_unchanged(INJECTION_FINAL_FRAGMENT_PUBLICATION_FAILURE);
    if (rc != 0) return 20 + rc;
    rc = anchor_equivalence();
    if (rc != 0) return 30 + rc;
    (void)puts("VIEW0_V1N1_G04_R2_ANCHOR_FAILURE_ATOMICITY=PASS");
    (void)puts("VIEW0_V1N1_G04_R2_FINAL_FRAGMENT_PUBLICATION_ATOMICITY=PASS");
    (void)puts("VIEW0_V1N1_G04_R2_ANCHOR_EQUIVALENCE=PASS");
    (void)puts("PASS: VIEW0 V1N1 G04 R2 global mechanism failure atomicity");
    return 0;
}
