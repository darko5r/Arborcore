#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <lexbor/html/parser.h>

#include <arborcore/view0_conformance/native.h>
#include "g04_r1a.h"

typedef enum injection_mode {
    INJECTION_NONE = 0,
    INJECTION_G04_R1_ANCHOR_FAILURE = 1,
    INJECTION_FINAL_LEXBOR_PARSER_FAILURE = 2
} injection_mode;

static injection_mode current_mode = INJECTION_NONE;
static bool inside_exact_lexbor_publication = false;

arbor_status __real_arbor_view0_native_g04_r1a_collect_anchors(
    arbor_span input,
    arbor_view0_native_source_anchor *anchors,
    uint64_t anchor_capacity,
    arbor_view0_native_g04_r1a_evaluation *evaluation_out);
arbor_status __wrap_arbor_view0_native_g04_r1a_collect_anchors(
    arbor_span input,
    arbor_view0_native_source_anchor *anchors,
    uint64_t anchor_capacity,
    arbor_view0_native_g04_r1a_evaluation *evaluation_out);

arbor_status __wrap_arbor_view0_native_g04_r1a_collect_anchors(
    arbor_span input,
    arbor_view0_native_source_anchor *anchors,
    uint64_t anchor_capacity,
    arbor_view0_native_g04_r1a_evaluation *evaluation_out)
{
    if (current_mode == INJECTION_G04_R1_ANCHOR_FAILURE) {
        return arbor_status_from_native(-(int64_t)ENOMEM);
    }
    return __real_arbor_view0_native_g04_r1a_collect_anchors(
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
        inside_exact_lexbor_publication) {
        return NULL;
    }
    return __real_lxb_html_parser_create();
}

static int outputs_unchanged_on_failure(injection_mode mode, const char *label)
{
    static const char html[] =
        "<!doctype html><title>x</title><body><p><a href=\"/\"><div>x</div></a></p></body>";
    arbor_view0_native_diagnostic diagnostics[64];
    arbor_view0_native_diagnostic before_diagnostics[64];
    arbor_view0_native_result result;
    arbor_view0_native_result before_result;

    (void)memset(diagnostics, 0x5a, sizeof(diagnostics));
    (void)memset(&result, 0xa5, sizeof(result));
    (void)memcpy(before_diagnostics, diagnostics, sizeof(diagnostics));
    (void)memcpy(&before_result, &result, sizeof(result));

    current_mode = mode;
    const arbor_status status = arbor_view0_native_check(
        (arbor_span){(const uint8_t *)html, (uint64_t)(sizeof(html) - 1u)},
        diagnostics, 64u, &result);
    current_mode = INJECTION_NONE;

    if (status.native != -(int64_t)ENOMEM) {
        (void)fprintf(stderr, "%s: status=%lld expected=%d\n",
                      label, (long long)status.native, -ENOMEM);
        return 1;
    }
    if (memcmp(diagnostics, before_diagnostics, sizeof(diagnostics)) != 0) {
        (void)fprintf(stderr, "%s: diagnostics changed on failure\n", label);
        return 2;
    }
    if (memcmp(&result, &before_result, sizeof(result)) != 0) {
        (void)fprintf(stderr, "%s: result changed on failure\n", label);
        return 3;
    }
    return 0;
}

static int anchor_equivalence(void)
{
    static const char html[] =
        "<!doctype html><title>x</title><body><p><a href=\"/\"><div>x</div></a></p></body>";
    const arbor_span input = {
        (const uint8_t *)html, (uint64_t)(sizeof(html) - 1u)
    };

    arbor_view0_native_g04_r1a_evaluation measured = {0};
    arbor_status status = arbor_view0_native_g04_r1a_measure(input, &measured);
    if (status.native != 0 || measured.diagnostic_count != 1u) return 1;

    arbor_view0_native_diagnostic direct[4] = {{0}};
    arbor_view0_native_g04_r1a_evaluation direct_eval = {0};
    status = arbor_view0_native_g04_r1a_collect(
        input, direct, measured.diagnostic_count, 41u, &direct_eval);
    if (status.native != 0 || memcmp(&measured, &direct_eval, sizeof(measured)) != 0)
        return 2;

    arbor_view0_native_source_anchor anchors[4] = {{0}};
    arbor_view0_native_g04_r1a_evaluation anchor_eval = {0};
    status = arbor_view0_native_g04_r1a_collect_anchors(
        input, anchors, measured.diagnostic_count, &anchor_eval);
    if (status.native != 0 || memcmp(&measured, &anchor_eval, sizeof(measured)) != 0)
        return 3;

    arbor_view0_native_diagnostic recreated[4] = {{0}};
    arbor_view0_native_g04_r1a_materialize_anchor(&anchors[0], 41u, &recreated[0]);
    if (memcmp(direct, recreated, sizeof(direct[0])) != 0) return 4;
    return 0;
}

int main(void)
{
    int rc = outputs_unchanged_on_failure(
        INJECTION_G04_R1_ANCHOR_FAILURE, "G04 R1 anchor failure");
    if (rc != 0) return 10 + rc;

    rc = outputs_unchanged_on_failure(
        INJECTION_FINAL_LEXBOR_PARSER_FAILURE, "final Lexbor parser failure");
    if (rc != 0) return 20 + rc;

    rc = anchor_equivalence();
    if (rc != 0) return 30 + rc;

    (void)puts("VIEW0_V1N1_G04_R1A_ANCHOR_FAILURE_ATOMICITY=PASS");
    (void)puts("VIEW0_V1N1_G04_R1A_FINAL_LEXBOR_FAILURE_ATOMICITY=PASS");
    (void)puts("VIEW0_V1N1_G04_R1A_ANCHOR_EQUIVALENCE=PASS");
    (void)puts("PASS: VIEW0 V1N1 G04 R1A global mechanism failure atomicity");
    return 0;
}
