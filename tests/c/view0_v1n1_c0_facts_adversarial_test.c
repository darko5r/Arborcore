#include <arborcore/view0_conformance/native.h>

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static arbor_span span_from_cstr(const char *text)
{
    return (arbor_span){(const uint8_t *)text, (uint64_t)strlen(text)};
}

static int unchanged(const void *left, const void *right, size_t length)
{
    return memcmp(left, right, length) == 0 ? 0 : 1;
}

int main(void)
{
    static const char valid[] =
        "<!doctype html><title>x</title><p>x</p>";

    arbor_view0_native_parse_counts counts = {
        UINT64_C(0x1111111111111111),
        UINT64_C(0x2222222222222222)
    };
    const arbor_view0_native_parse_counts counts_before = counts;
    arbor_status status = arbor_view0_native_lexbor_collect(
        span_from_cstr(valid),
        NULL,
        0u,
        &counts,
        NULL);
    if (status.native != -(int64_t)EINVAL ||
        unchanged(&counts, &counts_before, sizeof(counts)) != 0) {
        return 1;
    }

    arbor_view0_native_document_facts overlap_facts;
    (void)memset(&overlap_facts, 0x5a, sizeof(overlap_facts));
    const arbor_view0_native_document_facts overlap_facts_before = overlap_facts;
    status = arbor_view0_native_lexbor_collect(
        span_from_cstr(valid),
        NULL,
        0u,
        (arbor_view0_native_parse_counts *)&overlap_facts,
        &overlap_facts);
    if (status.native != -(int64_t)EINVAL ||
        unchanged(&overlap_facts, &overlap_facts_before, sizeof(overlap_facts)) != 0) {
        return 2;
    }

    static const char parse_error[] =
        "<!doctype html><title>x</title><body><?bad></body>";
    arbor_view0_native_document_facts facts;
    (void)memset(&facts, 0xa5, sizeof(facts));
    const arbor_view0_native_document_facts facts_before = facts;
    counts = counts_before;
    status = arbor_view0_native_lexbor_collect(
        span_from_cstr(parse_error),
        NULL,
        0u,
        &counts,
        &facts);
    if (status.native != -(int64_t)ENOSPC ||
        unchanged(&counts, &counts_before, sizeof(counts)) != 0 ||
        unchanged(&facts, &facts_before, sizeof(facts)) != 0) {
        return 3;
    }

    arbor_view0_native_diagnostic diagnostics_a[16] = {{0}};
    arbor_view0_native_diagnostic diagnostics_b[16] = {{0}};
    arbor_view0_native_parse_counts counts_a = {0};
    arbor_view0_native_parse_counts counts_b = {0};
    arbor_view0_native_document_facts facts_a = {0};
    arbor_view0_native_document_facts facts_b = {0};

    static const char repaired[] =
        "<!doctype html><html><head><title>x</title></head>"
        "<body></body><body></body></html>";
    status = arbor_view0_native_lexbor_collect(
        span_from_cstr(repaired),
        diagnostics_a,
        16u,
        &counts_a,
        &facts_a);
    if (status.native != 0) {
        return 4;
    }
    status = arbor_view0_native_lexbor_collect(
        span_from_cstr(repaired),
        diagnostics_b,
        16u,
        &counts_b,
        &facts_b);
    if (status.native != 0 ||
        unchanged(&counts_a, &counts_b, sizeof(counts_a)) != 0 ||
        unchanged(&facts_a, &facts_b, sizeof(facts_a)) != 0 ||
        unchanged(
            diagnostics_a,
            diagnostics_b,
            (size_t)(counts_a.tokenizer_error_count + counts_a.tree_error_count) *
                sizeof(diagnostics_a[0])) != 0) {
        return 5;
    }

    if (facts_a.source_body_start_tag_count != 2u ||
        facts_a.dom_html_body_element_count != 1u ||
        facts_a.source_second_title_start_tag_offset != ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE ||
        facts_a.source_second_base_start_tag_offset != ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE ||
        facts_a.source_second_body_start_tag_offset == ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE) {
        return 6;
    }

    static const char raw_text[] =
        "<!doctype html><title>x</title>"
        "<script>const a=\"<body>\"; const b=\"<base>\";</script>"
        "<!-- <body><base><title> --><p>x</p>";
    arbor_view0_native_diagnostic raw_diagnostics[16] = {{0}};
    arbor_view0_native_parse_counts raw_counts = {0};
    arbor_view0_native_document_facts raw_facts = {0};
    status = arbor_view0_native_lexbor_collect(
        span_from_cstr(raw_text),
        raw_diagnostics,
        16u,
        &raw_counts,
        &raw_facts);
    if (status.native != 0 ||
        raw_facts.source_title_start_tag_count != 1u ||
        raw_facts.source_base_start_tag_count != 0u ||
        raw_facts.source_body_start_tag_count != 0u ||
        raw_facts.dom_html_body_element_count != 1u) {
        return 7;
    }

    puts("PASS: VIEW0 V1N1 C0 facts failure atomicity, deterministic publication, sentinels and raw-text/comment isolation");
    return 0;
}
