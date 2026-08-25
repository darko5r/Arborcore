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
    arbor_view0_native_diagnostic diagnostics[16] = {{0}};
    arbor_view0_native_result result = {0};

    static const char omitted_body[] =
        "<!doctype html><title>x</title><p>ok</p>";
    arbor_status status = arbor_view0_native_check(
        span_from_cstr(omitted_body), diagnostics, 16u, &result);
    if (status.native != 0 || result.diagnostic_count != 0u ||
        result.tokenizer_error_count != 0u || result.tree_error_count != 0u ||
        (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_PARSE_CLEAN) == 0u ||
        count_rule(diagnostics, result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_BODY_SINGLETON) != 0u) {
        return 1;
    }

    static const char explicit_body[] =
        "<!doctype html><html><head><title>x</title></head>"
        "<body><p>ok</p></body></html>";
    (void)memset(diagnostics, 0, sizeof(diagnostics));
    result = (arbor_view0_native_result){0};
    status = arbor_view0_native_check(
        span_from_cstr(explicit_body), diagnostics, 16u, &result);
    if (status.native != 0 || result.diagnostic_count != 0u ||
        result.tokenizer_error_count != 0u || result.tree_error_count != 0u ||
        (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_PARSE_CLEAN) == 0u) {
        return 2;
    }

    static const char duplicate_body[] =
        "<!doctype html><html><head><title>x</title></head>"
        "<body>x</body><body></body></html>";
    (void)memset(diagnostics, 0, sizeof(diagnostics));
    result = (arbor_view0_native_result){0};
    status = arbor_view0_native_check(
        span_from_cstr(duplicate_body), diagnostics, 16u, &result);
    const arbor_view0_native_diagnostic *body = find_rule(
        diagnostics,
        result.diagnostic_count,
        ARBOR_VIEW_V1_G02_BODY_SINGLETON);
    if (status.native != 0 || result.diagnostic_count != 3u ||
        result.tokenizer_error_count != 0u || result.tree_error_count != 2u ||
        (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_PARSE_CLEAN) != 0u ||
        body == NULL ||
        body->severity != (uint32_t)ARBOR_VIEW0_NATIVE_SEVERITY_ERROR ||
        body->origin != (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_AUTHORING ||
        body->byte_offset != 65u || body->source_length != 4u ||
        body->line != 1u || body->column != 66u ||
        strcmp(body->symbolic_name, "ARBOR_VIEW_V1_G02_BODY_SINGLETON") != 0 ||
        strcmp(body->message,
               "HTML document must contain exactly one logical body element") != 0) {
        return 3;
    }

    static const char missing_logical_body[] =
        "<!doctype html><html><head><title>x</title></head>"
        "<frameset></frameset></html>";
    (void)memset(diagnostics, 0, sizeof(diagnostics));
    result = (arbor_view0_native_result){0};
    status = arbor_view0_native_check(
        span_from_cstr(missing_logical_body), diagnostics, 16u, &result);
    body = find_rule(
        diagnostics,
        result.diagnostic_count,
        ARBOR_VIEW_V1_G02_BODY_SINGLETON);
    if (status.native != 0 || result.diagnostic_count != 1u ||
        result.tokenizer_error_count != 0u || result.tree_error_count != 0u ||
        (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_PARSE_CLEAN) == 0u ||
        body == NULL || body->byte_offset != 0u || body->source_length != 0u ||
        body->line != 1u || body->column != 1u) {
        return 4;
    }

    static const char raw_text_body[] =
        "<!doctype html><title>x</title>"
        "<script>const s=\"<body>\";</script><!-- <body> --><p>x</p>";
    (void)memset(diagnostics, 0, sizeof(diagnostics));
    result = (arbor_view0_native_result){0};
    status = arbor_view0_native_check(
        span_from_cstr(raw_text_body), diagnostics, 16u, &result);
    if (status.native != 0 ||
        count_rule(diagnostics, result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_BODY_SINGLETON) != 0u) {
        return 5;
    }

    puts("PASS: VIEW0 V1N1 G02 R6 logical body singleton, legal omission, duplicate source anchor and missing-body detection");
    return 0;
}
