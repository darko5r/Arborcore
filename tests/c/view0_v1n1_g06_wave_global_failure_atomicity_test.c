#include <arborcore/view0_conformance/native.h>
#include "g06.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <lexbor/html/parser.h>

typedef enum injection_mode {
    INJECTION_NONE = 0,
    INJECTION_G06_ANCHOR_FAILURE = 1,
    INJECTION_FINAL_LEXBOR_PARSER_FAILURE = 2
} injection_mode;

static injection_mode current_mode = INJECTION_NONE;
static bool inside_exact = false;

arbor_status __real_arbor_view0_native_g06_collect_anchors(
    arbor_span, arbor_view0_native_g06_anchor *, uint64_t,
    arbor_view0_native_g06_evaluation *);
arbor_status __wrap_arbor_view0_native_g06_collect_anchors(
    arbor_span, arbor_view0_native_g06_anchor *, uint64_t,
    arbor_view0_native_g06_evaluation *);

arbor_status __wrap_arbor_view0_native_g06_collect_anchors(
    arbor_span input,
    arbor_view0_native_g06_anchor *anchors,
    uint64_t capacity,
    arbor_view0_native_g06_evaluation *evaluation)
{
    if (current_mode == INJECTION_G06_ANCHOR_FAILURE)
        return arbor_status_from_native(-(int64_t)ENOMEM);
    return __real_arbor_view0_native_g06_collect_anchors(
        input, anchors, capacity, evaluation);
}

arbor_status __real_arbor_view0_native_lexbor_collect_exact(
    arbor_span, arbor_view0_native_diagnostic *, uint64_t,
    const arbor_view0_native_parse_counts *,
    const arbor_view0_native_document_facts *);
arbor_status __wrap_arbor_view0_native_lexbor_collect_exact(
    arbor_span, arbor_view0_native_diagnostic *, uint64_t,
    const arbor_view0_native_parse_counts *,
    const arbor_view0_native_document_facts *);

arbor_status __wrap_arbor_view0_native_lexbor_collect_exact(
    arbor_span input,
    arbor_view0_native_diagnostic *diagnostics,
    uint64_t capacity,
    const arbor_view0_native_parse_counts *counts,
    const arbor_view0_native_document_facts *facts)
{
    const bool previous = inside_exact;
    inside_exact = true;
    const arbor_status status = __real_arbor_view0_native_lexbor_collect_exact(
        input, diagnostics, capacity, counts, facts);
    inside_exact = previous;
    return status;
}

lxb_html_parser_t *__real_lxb_html_parser_create(void);
lxb_html_parser_t *__wrap_lxb_html_parser_create(void);

lxb_html_parser_t *__wrap_lxb_html_parser_create(void)
{
    if (current_mode == INJECTION_FINAL_LEXBOR_PARSER_FAILURE && inside_exact)
        return NULL;
    return __real_lxb_html_parser_create();
}

static int unchanged(injection_mode mode)
{
    static const char html[] =
        "<!doctype html><title>x</title><body><details open=false>x</details></body>";
    arbor_view0_native_diagnostic diagnostics[64];
    arbor_view0_native_diagnostic before[64];
    arbor_view0_native_result result;
    arbor_view0_native_result result_before;
    (void)memset(diagnostics, 0x6a, sizeof(diagnostics));
    (void)memcpy(before, diagnostics, sizeof(before));
    (void)memset(&result, 0xa6, sizeof(result));
    result_before = result;
    current_mode = mode;
    const arbor_status status = arbor_view0_native_check(
        (arbor_span){(const uint8_t *)html, (uint64_t)(sizeof(html) - 1u)},
        diagnostics, 64u, &result);
    current_mode = INJECTION_NONE;
    if (status.native != -(int64_t)ENOMEM) return 1;
    if (memcmp(diagnostics, before, sizeof(before)) != 0 ||
        memcmp(&result, &result_before, sizeof(result)) != 0) return 2;
    return 0;
}

static int equivalence(void)
{
    static const char html[] =
        "<!doctype html><title>x</title><body><details open=false>x</details></body>";
    const arbor_span input = {
        (const uint8_t *)html, (uint64_t)(sizeof(html) - 1u)
    };
    arbor_view0_native_g06_evaluation measured = {0};
    arbor_view0_native_g06_evaluation collected = {0};
    arbor_view0_native_g06_anchor anchor = {0};
    if (arbor_view0_native_g06_measure(input, &measured).native != 0 ||
        measured.diagnostic_count != 1u) return 1;
    if (arbor_view0_native_g06_collect_anchors(
            input, &anchor, 1u, &collected).native != 0 ||
        memcmp(&measured, &collected, sizeof(measured)) != 0) return 2;
    const char *position = strstr(html, "open");
    if (position == NULL || anchor.source.byte_offset != (uint32_t)(position - html) ||
        anchor.source.source_length != 4u || anchor.rule_ordinal != UINT16_C(1))
        return 3;
    arbor_view0_native_diagnostic diagnostic = {0};
    arbor_view0_native_g06_materialize_anchor(&anchor, 91u, &diagnostic);
    if (diagnostic.rule_id != UINT64_C(0x0000000030060001) ||
        diagnostic.byte_offset != (uint64_t)(position - html) ||
        diagnostic.source_length != 4u || diagnostic.discovery_sequence != 91u)
        return 4;
    return 0;
}

int main(void)
{
    int result = unchanged(INJECTION_G06_ANCHOR_FAILURE);
    if (result != 0) return 10 + result;
    result = unchanged(INJECTION_FINAL_LEXBOR_PARSER_FAILURE);
    if (result != 0) return 20 + result;
    result = equivalence();
    if (result != 0) return 30 + result;
    (void)puts("VIEW0_V1N1_G06_WAVE_ANCHOR_FAILURE_ATOMICITY=PASS");
    (void)puts("VIEW0_V1N1_G06_WAVE_FINAL_LEXBOR_FAILURE_ATOMICITY=PASS");
    (void)puts("VIEW0_V1N1_G06_WAVE_ANCHOR_EQUIVALENCE=PASS");
    (void)puts("PASS: G06 R1-R17 global mechanism failure atomicity");
    return 0;
}
