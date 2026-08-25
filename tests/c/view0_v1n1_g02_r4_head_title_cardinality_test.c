#include <arborcore/view0_conformance/native.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static arbor_span span_from_cstr(const char *text)
{
    return (arbor_span){(const uint8_t *)text, (uint64_t)strlen(text)};
}

static uint64_t count_rule(
    const arbor_view0_native_diagnostic *diagnostics,
    uint64_t diagnostic_count,
    uint64_t rule_id)
{
    uint64_t count = 0u;
    for (uint64_t i = 0u; i < diagnostic_count; ++i) {
        if (diagnostics[i].rule_id == rule_id) {
            count += 1u;
        }
    }
    return count;
}

static const arbor_view0_native_diagnostic *find_rule(
    const arbor_view0_native_diagnostic *diagnostics,
    uint64_t diagnostic_count,
    uint64_t rule_id)
{
    for (uint64_t i = 0u; i < diagnostic_count; ++i) {
        if (diagnostics[i].rule_id == rule_id) {
            return &diagnostics[i];
        }
    }
    return NULL;
}

int main(void)
{
    static const char canonical[] = "<!doctype html><title>x</title>";
    arbor_view0_native_diagnostic diagnostics[8] = {{0}};
    arbor_view0_native_result result = {0};
    arbor_status status = arbor_view0_native_check(
        span_from_cstr(canonical), diagnostics, 8u, &result);
    if (status.native != 0 || result.diagnostic_count != 0u ||
        result.tokenizer_error_count != 0u || result.tree_error_count != 0u ||
        (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_PARSE_CLEAN) == 0u) {
        return 1;
    }

    static const char missing[] =
        "<!doctype html><head></head><body><p>x</p></body>";
    (void)memset(diagnostics, 0, sizeof(diagnostics));
    result = (arbor_view0_native_result){0};
    status = arbor_view0_native_check(
        span_from_cstr(missing), diagnostics, 8u, &result);
    const arbor_view0_native_diagnostic *title = find_rule(
        diagnostics,
        result.diagnostic_count,
        ARBOR_VIEW_V1_G02_HEAD_TITLE_CARDINALITY);
    if (status.native != 0 || result.diagnostic_count != 1u ||
        result.tokenizer_error_count != 0u || result.tree_error_count != 0u ||
        (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_PARSE_CLEAN) == 0u ||
        title == NULL ||
        title->severity != (uint32_t)ARBOR_VIEW0_NATIVE_SEVERITY_ERROR ||
        title->origin != (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_AUTHORING ||
        title->byte_offset != 0u || title->source_length != 0u ||
        title->line != 1u || title->column != 1u ||
        strcmp(title->symbolic_name,
               "ARBOR_VIEW_V1_G02_HEAD_TITLE_CARDINALITY") != 0 ||
        strcmp(title->message,
               "Standalone HTML document head must contain exactly one title element") != 0) {
        return 2;
    }

    static const char duplicate[] =
        "<!doctype html><title>a</title><title>b</title><p>x</p>";
    (void)memset(diagnostics, 0, sizeof(diagnostics));
    result = (arbor_view0_native_result){0};
    status = arbor_view0_native_check(
        span_from_cstr(duplicate), diagnostics, 8u, &result);
    title = find_rule(
        diagnostics,
        result.diagnostic_count,
        ARBOR_VIEW_V1_G02_HEAD_TITLE_CARDINALITY);
    if (status.native != 0 || result.diagnostic_count != 1u ||
        result.tokenizer_error_count != 0u || result.tree_error_count != 0u ||
        (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_PARSE_CLEAN) == 0u ||
        title == NULL || title->byte_offset != 32u ||
        title->source_length != 5u || title->line != 1u ||
        title->column != 33u) {
        return 3;
    }

    static const char omitted_head_tags[] =
        "<!doctype html><title>x</title><p>ok</p>";
    (void)memset(diagnostics, 0, sizeof(diagnostics));
    result = (arbor_view0_native_result){0};
    status = arbor_view0_native_check(
        span_from_cstr(omitted_head_tags), diagnostics, 8u, &result);
    if (status.native != 0 ||
        count_rule(diagnostics,
                   result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_HEAD_TITLE_CARDINALITY) != 0u) {
        return 4;
    }

    static const char title_in_body_after_valid_head[] =
        "<!doctype html><head><title>a</title></head>"
        "<body><title>b</title></body>";
    (void)memset(diagnostics, 0, sizeof(diagnostics));
    result = (arbor_view0_native_result){0};
    status = arbor_view0_native_check(
        span_from_cstr(title_in_body_after_valid_head), diagnostics, 8u, &result);
    if (status.native != 0 || result.tokenizer_error_count != 0u ||
        result.tree_error_count != 0u ||
        count_rule(diagnostics,
                   result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_HEAD_TITLE_CARDINALITY) != 0u) {
        return 5;
    }

    static const char body_title_only[] =
        "<!doctype html><head></head><body><title>x</title></body>";
    (void)memset(diagnostics, 0, sizeof(diagnostics));
    result = (arbor_view0_native_result){0};
    status = arbor_view0_native_check(
        span_from_cstr(body_title_only), diagnostics, 8u, &result);
    if (status.native != 0 || result.tokenizer_error_count != 0u ||
        result.tree_error_count != 0u ||
        count_rule(diagnostics,
                   result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_HEAD_TITLE_CARDINALITY) != 1u) {
        return 6;
    }

    puts("PASS: VIEW0 V1N1 G02 R4 standalone head-title cardinality, duplicate source anchor and parser-implied head semantics");
    return 0;
}
