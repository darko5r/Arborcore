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

int main(void)
{
    static const char positive[] =
        "<!doctype html><html><head><title>x</title></head><body><p>x</p></body></html>";
    arbor_view0_native_result result = {0};
    arbor_status status = arbor_view0_native_check(
        span_from_cstr(positive), NULL, 0u, &result);
    if (status.native != 0 || result.diagnostic_count != 0u ||
        result.tokenizer_error_count != 0u || result.tree_error_count != 0u ||
        (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_PARSE_CLEAN) == 0u) {
        return 1;
    }

    static const char negative[] =
        "<html><head><title>x</title></head><body><p>x</p></body></html>";
    arbor_view0_native_diagnostic diagnostic[2] = {{0}};
    result = (arbor_view0_native_result){0};
    status = arbor_view0_native_check(
        span_from_cstr(negative), diagnostic, 2u, &result);
    if (status.native != 0 || result.diagnostic_count != 2u ||
        result.tokenizer_error_count != 0u || result.tree_error_count != 1u ||
        (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_PARSE_CLEAN) != 0u ||
        diagnostic[0].rule_id != ARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED ||
        diagnostic[0].origin != (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_AUTHORING ||
        diagnostic[0].severity != (uint32_t)ARBOR_VIEW0_NATIVE_SEVERITY_ERROR ||
        diagnostic[0].byte_offset != 0u || diagnostic[0].source_length != 0u ||
        diagnostic[0].line != 1u || diagnostic[0].column != 1u ||
        strcmp(diagnostic[0].symbolic_name,
               "ARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED") != 0 ||
        strcmp(diagnostic[0].message,
               "HTML document is missing the required DOCTYPE preamble") != 0) {
        return 2;
    }

    static const char comment_before_doctype[] =
        "<!-- lead --><!DOCTYPE HTML><title>x</title><p>ok</p>";
    arbor_view0_native_diagnostic comment_diagnostics[8] = {{0}};
    result = (arbor_view0_native_result){0};
    status = arbor_view0_native_check(
        span_from_cstr(comment_before_doctype),
        comment_diagnostics,
        8u,
        &result);
    if (status.native != 0 ||
        count_rule(comment_diagnostics,
                   result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED) != 0u) {
        return 3;
    }

    static const char malformed_but_present[] =
        "<!DOCTYPE svg><title>x</title><p>ok</p>";
    arbor_view0_native_diagnostic malformed_diagnostics[8] = {{0}};
    result = (arbor_view0_native_result){0};
    status = arbor_view0_native_check(
        span_from_cstr(malformed_but_present),
        malformed_diagnostics,
        8u,
        &result);
    if (status.native != 0 ||
        count_rule(malformed_diagnostics,
                   result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED) != 0u) {
        return 4;
    }

    static const char comment_only_fake_doctype[] =
        "<!-- <!doctype html> --><title>x</title><p>ok</p>";
    arbor_view0_native_diagnostic comment_fake_diagnostics[8] = {{0}};
    result = (arbor_view0_native_result){0};
    status = arbor_view0_native_check(
        span_from_cstr(comment_only_fake_doctype),
        comment_fake_diagnostics,
        8u,
        &result);
    if (status.native != 0 ||
        count_rule(comment_fake_diagnostics,
                   result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED) != 1u) {
        return 5;
    }

    static const char body_doctype[] =
        "<html><head><title>x</title></head><body><!doctype html></body></html>";
    arbor_view0_native_diagnostic body_diagnostics[8] = {{0}};
    result = (arbor_view0_native_result){0};
    status = arbor_view0_native_check(
        span_from_cstr(body_doctype), body_diagnostics, 8u, &result);
    if (status.native != 0 || result.tree_error_count == 0u ||
        count_rule(body_diagnostics,
                   result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED) != 1u) {
        return 6;
    }

    arbor_view0_native_diagnostic empty_diagnostic[4] = {{0}};
    result = (arbor_view0_native_result){0};
    status = arbor_view0_native_check(
        (arbor_span){NULL, 0u}, empty_diagnostic, 4u, &result);
    const arbor_view0_native_diagnostic *empty_required = NULL;
    for (uint64_t i = 0u; i < result.diagnostic_count; ++i) {
        if (empty_diagnostic[i].rule_id == ARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED) {
            empty_required = &empty_diagnostic[i];
            break;
        }
    }
    if (status.native != 0 || result.diagnostic_count < 1u ||
        result.tokenizer_error_count != 0u || result.tree_error_count != 0u ||
        (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_PARSE_CLEAN) == 0u ||
        empty_required == NULL || empty_required->byte_offset != 0u ||
        empty_required->line != 1u || empty_required->column != 1u) {
        return 7;
    }

    puts("PASS: VIEW0 V1N1 G02 R1 required-doctype exact fixture, source semantics and empty-input authoring diagnostic");
    return 0;
}
