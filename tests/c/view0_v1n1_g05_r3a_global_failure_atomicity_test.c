#include <arborcore/view0_conformance/native.h>
#include "g05_r3a.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <lexbor/html/parser.h>

typedef enum injection_mode {
    INJECTION_NONE = 0,
    INJECTION_R3_ANCHOR_FAILURE = 1,
    INJECTION_FINAL_LEXBOR_PARSER_FAILURE = 2
} injection_mode;

static injection_mode current_mode = INJECTION_NONE;
static bool inside_exact_lexbor_publication = false;

arbor_status __real_arbor_view0_native_g05_r3a_collect_anchors(
    arbor_span input,
    arbor_view0_native_source_anchor *anchors,
    uint64_t anchor_capacity,
    arbor_view0_native_g05_r3a_evaluation *evaluation_out);
arbor_status __wrap_arbor_view0_native_g05_r3a_collect_anchors(
    arbor_span input,
    arbor_view0_native_source_anchor *anchors,
    uint64_t anchor_capacity,
    arbor_view0_native_g05_r3a_evaluation *evaluation_out);

arbor_status __wrap_arbor_view0_native_g05_r3a_collect_anchors(
    arbor_span input,
    arbor_view0_native_source_anchor *anchors,
    uint64_t anchor_capacity,
    arbor_view0_native_g05_r3a_evaluation *evaluation_out)
{
    if (current_mode == INJECTION_R3_ANCHOR_FAILURE)
        return arbor_status_from_native(-(int64_t)ENOMEM);
    return __real_arbor_view0_native_g05_r3a_collect_anchors(
        input, anchors, anchor_capacity, evaluation_out);
}

arbor_status __real_arbor_view0_native_lexbor_collect_exact(
    arbor_span input,
    arbor_view0_native_diagnostic *diagnostics,
    uint64_t diagnostic_capacity,
    const arbor_view0_native_parse_counts *expected_counts,
    const arbor_view0_native_document_facts *expected_facts);
arbor_status __wrap_arbor_view0_native_lexbor_collect_exact(
    arbor_span input,
    arbor_view0_native_diagnostic *diagnostics,
    uint64_t diagnostic_capacity,
    const arbor_view0_native_parse_counts *expected_counts,
    const arbor_view0_native_document_facts *expected_facts);

arbor_status __wrap_arbor_view0_native_lexbor_collect_exact(
    arbor_span input,
    arbor_view0_native_diagnostic *diagnostics,
    uint64_t diagnostic_capacity,
    const arbor_view0_native_parse_counts *expected_counts,
    const arbor_view0_native_document_facts *expected_facts)
{
    const bool previous = inside_exact_lexbor_publication;
    inside_exact_lexbor_publication = true;
    const arbor_status status = __real_arbor_view0_native_lexbor_collect_exact(
        input, diagnostics, diagnostic_capacity, expected_counts, expected_facts);
    inside_exact_lexbor_publication = previous;
    return status;
}

lxb_html_parser_t *__real_lxb_html_parser_create(void);
lxb_html_parser_t *__wrap_lxb_html_parser_create(void);

lxb_html_parser_t *__wrap_lxb_html_parser_create(void)
{
    if (current_mode == INJECTION_FINAL_LEXBOR_PARSER_FAILURE &&
        inside_exact_lexbor_publication)
        return NULL;
    return __real_lxb_html_parser_create();
}

static int unchanged_on_failure(injection_mode mode)
{
    static const char html[] =
        "<!doctype html><title>x</title><body><a target=\"_blank\">x</a></body>";
    arbor_view0_native_diagnostic diagnostics[64], before_diagnostics[64];
    arbor_view0_native_result result, before_result;
    (void)memset(diagnostics, 0x6a, sizeof(diagnostics));
    (void)memcpy(before_diagnostics, diagnostics, sizeof(diagnostics));
    (void)memset(&result, 0xa6, sizeof(result));
    before_result = result;
    current_mode = mode;
    const arbor_status status = arbor_view0_native_check(
        (arbor_span){(const uint8_t *)html, (uint64_t)(sizeof(html) - 1u)},
        diagnostics, UINT64_C(64), &result);
    current_mode = INJECTION_NONE;
    if (status.native != -(int64_t)ENOMEM) return 1;
    if (memcmp(diagnostics, before_diagnostics, sizeof(diagnostics)) != 0) return 2;
    if (memcmp(&result, &before_result, sizeof(result)) != 0) return 3;
    return 0;
}

static int anchor_equivalence(void)
{
    static const char html[] =
        "<!doctype html><title>x</title><body><a target=\"_blank\">x</a></body>";
    const arbor_span input = {(const uint8_t *)html, (uint64_t)(sizeof(html) - 1u)};
    arbor_view0_native_g05_r3a_evaluation measured = {0};
    arbor_status status = arbor_view0_native_g05_r3a_measure(input, &measured);
    if (status.native != 0 || measured.diagnostic_count != UINT64_C(1)) return 1;
    arbor_view0_native_source_anchor anchor = {0};
    arbor_view0_native_g05_r3a_evaluation collected = {0};
    status = arbor_view0_native_g05_r3a_collect_anchors(input, &anchor, UINT64_C(1), &collected);
    if (status.native != 0 || memcmp(&measured, &collected, sizeof(measured)) != 0) return 2;
    const char *target = strstr(html, "target");
    if (target == NULL || anchor.byte_offset != (uint32_t)(target - html) ||
        anchor.source_length != UINT32_C(6))
        return 3;
    arbor_view0_native_diagnostic diagnostic = {0};
    arbor_view0_native_g05_r3a_materialize_anchor(&anchor, UINT64_C(73), &diagnostic);
    if (diagnostic.rule_id != ARBOR_VIEW_V1_G05_CONDITIONAL_ATTRIBUTE_APPLICABILITY ||
        diagnostic.byte_offset != (uint64_t)(target - html) ||
        diagnostic.source_length != UINT64_C(6) ||
        diagnostic.discovery_sequence != UINT64_C(73))
        return 4;
    return 0;
}

int main(void)
{
    int rc = unchanged_on_failure(INJECTION_R3_ANCHOR_FAILURE);
    if (rc != 0) return 10 + rc;
    rc = unchanged_on_failure(INJECTION_FINAL_LEXBOR_PARSER_FAILURE);
    if (rc != 0) return 20 + rc;
    rc = anchor_equivalence();
    if (rc != 0) return 30 + rc;
    (void)puts("VIEW0_V1N1_G05_R3A_ANCHOR_FAILURE_ATOMICITY=PASS");
    (void)puts("VIEW0_V1N1_G05_R3A_FINAL_LEXBOR_FAILURE_ATOMICITY=PASS");
    (void)puts("VIEW0_V1N1_G05_R3A_ANCHOR_EQUIVALENCE=PASS");
    (void)puts("PASS: G05 R3A global mechanism failure atomicity");
    return 0;
}
