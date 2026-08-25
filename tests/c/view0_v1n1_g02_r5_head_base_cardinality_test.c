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
    arbor_view0_native_diagnostic diagnostics[8] = {{0}};
    arbor_view0_native_result result = {0};

    static const char zero_base[] = "<!doctype html><title>x</title>";
    arbor_status status = arbor_view0_native_check(
        span_from_cstr(zero_base), diagnostics, 8u, &result);
    if (status.native != 0 || result.diagnostic_count != 0u ||
        count_rule(diagnostics, result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_HEAD_BASE_CARDINALITY) != 0u) {
        return 1;
    }

    static const char one_base[] =
        "<!doctype html><title>x</title><base href=\"/\">";
    (void)memset(diagnostics, 0, sizeof(diagnostics));
    result = (arbor_view0_native_result){0};
    status = arbor_view0_native_check(
        span_from_cstr(one_base), diagnostics, 8u, &result);
    if (status.native != 0 || result.diagnostic_count != 0u ||
        result.tokenizer_error_count != 0u || result.tree_error_count != 0u ||
        (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_PARSE_CLEAN) == 0u) {
        return 2;
    }

    static const char duplicate_base[] =
        "<!doctype html><title>x</title><base href=\"/\"><base href=\"/x\">";
    (void)memset(diagnostics, 0, sizeof(diagnostics));
    result = (arbor_view0_native_result){0};
    status = arbor_view0_native_check(
        span_from_cstr(duplicate_base), diagnostics, 8u, &result);
    const arbor_view0_native_diagnostic *base = find_rule(
        diagnostics,
        result.diagnostic_count,
        ARBOR_VIEW_V1_G02_HEAD_BASE_CARDINALITY);
    if (status.native != 0 || result.diagnostic_count != 1u ||
        result.tokenizer_error_count != 0u || result.tree_error_count != 0u ||
        (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_PARSE_CLEAN) == 0u ||
        base == NULL ||
        base->severity != (uint32_t)ARBOR_VIEW0_NATIVE_SEVERITY_ERROR ||
        base->origin != (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_AUTHORING ||
        base->byte_offset != 47u || base->source_length != 4u ||
        base->line != 1u || base->column != 48u ||
        strcmp(base->symbolic_name,
               "ARBOR_VIEW_V1_G02_HEAD_BASE_CARDINALITY") != 0 ||
        strcmp(base->message,
               "HTML document head must contain no more than one base element") != 0) {
        return 3;
    }

    static const char duplicate_title_and_base[] =
        "<!doctype html><title>a</title><title>b</title>"
        "<base href=\"/\"><base href=\"/x\"><p>x</p>";
    (void)memset(diagnostics, 0, sizeof(diagnostics));
    result = (arbor_view0_native_result){0};
    status = arbor_view0_native_check(
        span_from_cstr(duplicate_title_and_base), diagnostics, 8u, &result);
    if (status.native != 0 ||
        count_rule(diagnostics, result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_HEAD_TITLE_CARDINALITY) != 1u ||
        count_rule(diagnostics, result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_HEAD_BASE_CARDINALITY) != 1u) {
        return 4;
    }

    puts("PASS: VIEW0 V1N1 G02 R5 head-base maximum cardinality, zero/one admission and second-base source anchor");
    return 0;
}
