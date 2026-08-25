#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <lexbor/html/parser.h>

#include <arborcore/view0_conformance/native.h>
#include "g03_r1a.h"
#include "g03_r2a.h"
#include "g03_r3a.h"
#include "g03_r4a.h"
#include "g03_r5a.h"
#include "g03_r7a.h"

typedef enum injection_mode {
    INJECTION_NONE = 0,
    INJECTION_R7_ANCHOR_FAILURE = 1,
    INJECTION_FINAL_LEXBOR_PARSER_FAILURE = 2
} injection_mode;

static injection_mode current_mode = INJECTION_NONE;
static bool inside_exact_lexbor_publication = false;

arbor_status __real_arbor_view0_native_g03_r7a_collect_anchors(
    arbor_span input,
    arbor_view0_native_source_anchor *anchors,
    uint64_t anchor_capacity,
    arbor_view0_native_g03_r7a_evaluation *evaluation_out);
arbor_status __wrap_arbor_view0_native_g03_r7a_collect_anchors(
    arbor_span input,
    arbor_view0_native_source_anchor *anchors,
    uint64_t anchor_capacity,
    arbor_view0_native_g03_r7a_evaluation *evaluation_out);

arbor_status __wrap_arbor_view0_native_g03_r7a_collect_anchors(
    arbor_span input,
    arbor_view0_native_source_anchor *anchors,
    uint64_t anchor_capacity,
    arbor_view0_native_g03_r7a_evaluation *evaluation_out)
{
    if (current_mode == INJECTION_R7_ANCHOR_FAILURE) {
        return arbor_status_from_native(-(int64_t)ENOMEM);
    }
    return __real_arbor_view0_native_g03_r7a_collect_anchors(
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
    static const char html[] = "<title>x</title><body><p></p></body>";
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
        diagnostics,
        64u,
        &result);
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

#define DEFINE_ANCHOR_EQ_FN(fn_name, eval_type, measure_fn, collect_fn, anchor_fn, materialize_fn) \
static int fn_name(const char *html) \
{ \
    const arbor_span input = {(const uint8_t *)html, (uint64_t)strlen(html)}; \
    eval_type measured = {0}; \
    arbor_status status = measure_fn(input, &measured); \
    if (status.native != 0 || measured.diagnostic_count == 0u || measured.diagnostic_count > 16u) return 1; \
    arbor_view0_native_diagnostic direct[16] = {{0}}; \
    eval_type direct_eval = {0}; \
    status = collect_fn(input, direct, measured.diagnostic_count, 17u, &direct_eval); \
    if (status.native != 0 || memcmp(&measured, &direct_eval, sizeof(measured)) != 0) return 2; \
    arbor_view0_native_source_anchor anchors[16] = {{0}}; \
    eval_type anchor_eval = {0}; \
    status = anchor_fn(input, anchors, measured.diagnostic_count, &anchor_eval); \
    if (status.native != 0 || memcmp(&measured, &anchor_eval, sizeof(measured)) != 0) return 3; \
    arbor_view0_native_diagnostic recreated[16] = {{0}}; \
    for (uint64_t i = 0u; i < measured.diagnostic_count; ++i) { \
        materialize_fn(&anchors[i], 17u + i, &recreated[i]); \
    } \
    if (memcmp(direct, recreated, measured.diagnostic_count * sizeof(direct[0])) != 0) return 4; \
    return 0; \
}

DEFINE_ANCHOR_EQ_FN(anchor_eq_r1, arbor_view0_native_g03_r1a_evaluation,
    arbor_view0_native_g03_r1a_measure, arbor_view0_native_g03_r1a_collect,
    arbor_view0_native_g03_r1a_collect_anchors, arbor_view0_native_g03_r1a_materialize_anchor)
DEFINE_ANCHOR_EQ_FN(anchor_eq_r2, arbor_view0_native_g03_r2a_evaluation,
    arbor_view0_native_g03_r2a_measure, arbor_view0_native_g03_r2a_collect,
    arbor_view0_native_g03_r2a_collect_anchors, arbor_view0_native_g03_r2a_materialize_anchor)
DEFINE_ANCHOR_EQ_FN(anchor_eq_r3, arbor_view0_native_g03_r3a_evaluation,
    arbor_view0_native_g03_r3a_measure, arbor_view0_native_g03_r3a_collect,
    arbor_view0_native_g03_r3a_collect_anchors, arbor_view0_native_g03_r3a_materialize_anchor)
DEFINE_ANCHOR_EQ_FN(anchor_eq_r4, arbor_view0_native_g03_r4a_evaluation,
    arbor_view0_native_g03_r4a_measure, arbor_view0_native_g03_r4a_collect,
    arbor_view0_native_g03_r4a_collect_anchors, arbor_view0_native_g03_r4a_materialize_anchor)
DEFINE_ANCHOR_EQ_FN(anchor_eq_r5, arbor_view0_native_g03_r5a_evaluation,
    arbor_view0_native_g03_r5a_measure, arbor_view0_native_g03_r5a_collect,
    arbor_view0_native_g03_r5a_collect_anchors, arbor_view0_native_g03_r5a_materialize_anchor)
DEFINE_ANCHOR_EQ_FN(anchor_eq_r7, arbor_view0_native_g03_r7a_evaluation,
    arbor_view0_native_g03_r7a_measure, arbor_view0_native_g03_r7a_collect,
    arbor_view0_native_g03_r7a_collect_anchors, arbor_view0_native_g03_r7a_materialize_anchor)

static int anchor_equivalence(void)
{
    if (anchor_eq_r1("<!doctype html><title>x</title><dl><dd>x</dd></dl>") != 0) return 1;
    if (anchor_eq_r2("<!doctype html><title>x</title><ul>text<li>x</li></ul>") != 0) return 2;
    if (anchor_eq_r3("<!doctype html><title>x</title><header><div><footer>x</footer></div></header>") != 0) return 3;
    if (anchor_eq_r4("<!doctype html><title>x</title><body><iframe>text</iframe></body>") != 0) return 4;
    if (anchor_eq_r5("<!doctype html><title>x</title><body><table><p>x</p></table></body>") != 0) return 5;
    if (anchor_eq_r7("<!doctype html><title>x</title><body><p></p></body>") != 0) return 6;
    return 0;
}

static int exact_mismatch_is_atomic(void)
{
    static const char html[] = "<title>x</title><body><p></p></body>";
    const arbor_span input = {
        (const uint8_t *)html, (uint64_t)(sizeof(html) - 1u)
    };
    arbor_view0_native_parse_counts counts = {0};
    arbor_view0_native_document_facts facts = {0};
    arbor_status status = arbor_view0_native_lexbor_measure(input, &counts, &facts);
    if (status.native != 0) return 1;

    arbor_view0_native_diagnostic diagnostics[64];
    arbor_view0_native_diagnostic before[64];
    (void)memset(diagnostics, 0x6b, sizeof(diagnostics));
    (void)memcpy(before, diagnostics, sizeof(diagnostics));

    arbor_view0_native_parse_counts wrong = counts;
    wrong.tree_error_count += 1u;
    status = arbor_view0_native_lexbor_collect_exact(
        input, diagnostics, 64u, &wrong, &facts);
    if (status.native != -(int64_t)EIO) return 2;
    if (memcmp(diagnostics, before, sizeof(diagnostics)) != 0) return 3;
    return 0;
}

int main(void)
{
    int rc = outputs_unchanged_on_failure(
        INJECTION_R7_ANCHOR_FAILURE, "R7 anchor failure");
    if (rc != 0) return 10 + rc;
    rc = outputs_unchanged_on_failure(
        INJECTION_FINAL_LEXBOR_PARSER_FAILURE, "final Lexbor parser failure");
    if (rc != 0) return 20 + rc;
    rc = exact_mismatch_is_atomic();
    if (rc != 0) return 30 + rc;
    rc = anchor_equivalence();
    if (rc != 0) return 40 + rc;

    (void)puts("VIEW0_V1N1_G03_R7A_SR2_R7_ANCHOR_FAILURE_ATOMICITY=PASS");
    (void)puts("VIEW0_V1N1_G03_R7A_SR2_FINAL_LEXBOR_FAILURE_ATOMICITY=PASS");
    (void)puts("VIEW0_V1N1_G03_R7A_SR2_EXACT_PARSE_MISMATCH_ATOMICITY=PASS");
    (void)puts("VIEW0_V1N1_G03_R7A_SR2_ANCHOR_EQUIVALENCE=R1_R5_R7_PASS");
    (void)puts("PASS: VIEW0 V1N1 G03 R7A SR2 global mechanism failure atomicity");
    return 0;
}
