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

static int expect_clean(const char *html)
{
    arbor_view0_native_diagnostic diagnostics[8] = {{0}};
    arbor_view0_native_result result = {0};
    const arbor_status status = arbor_view0_native_check(
        span_from_cstr(html), diagnostics, 8u, &result);
    return status.native == 0 && result.diagnostic_count == 0u &&
           result.tokenizer_error_count == 0u && result.tree_error_count == 0u &&
           (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_PARSE_CLEAN) != 0u
        ? 0
        : 1;
}

int main(void)
{
    static const char canonical[] = "<!DOCTYPE HTML><title>x</title>";
    if (expect_clean(canonical) != 0) {
        return 1;
    }

    static const char canonical_whitespace[] =
        "<!DoCtYpE\tHtMl \n><title>x</title>";
    if (expect_clean(canonical_whitespace) != 0) {
        return 2;
    }

    static const char legacy_double[] =
        "<!doctype html system \"about:legacy-compat\"><title>x</title>";
    arbor_view0_native_diagnostic legacy_diagnostics[8] = {{0}};
    arbor_view0_native_result legacy_result = {0};
    arbor_status legacy_status = arbor_view0_native_check(
        span_from_cstr(legacy_double), legacy_diagnostics, 8u, &legacy_result);
    if (legacy_status.native != 0 || legacy_result.tokenizer_error_count != 0u ||
        legacy_result.tree_error_count != 0u ||
        (legacy_result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_PARSE_CLEAN) == 0u ||
        count_rule(legacy_diagnostics,
                   legacy_result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX) != 0u) {
        return 3;
    }

    static const char legacy_single[] =
        "<!DOCTYPE HTML\nSYSTEM\t'about:legacy-compat' \r><title>x</title>";
    (void)memset(legacy_diagnostics, 0, sizeof(legacy_diagnostics));
    legacy_result = (arbor_view0_native_result){0};
    legacy_status = arbor_view0_native_check(
        span_from_cstr(legacy_single), legacy_diagnostics, 8u, &legacy_result);
    if (legacy_status.native != 0 || legacy_result.tokenizer_error_count != 0u ||
        legacy_result.tree_error_count != 0u ||
        (legacy_result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_PARSE_CLEAN) == 0u ||
        count_rule(legacy_diagnostics,
                   legacy_result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX) != 0u) {
        return 4;
    }

    static const char bad_name[] = "<!DOCTYPE svg><title>x</title>";
    arbor_view0_native_diagnostic diagnostics[8] = {{0}};
    arbor_view0_native_result result = {0};
    arbor_status status = arbor_view0_native_check(
        span_from_cstr(bad_name), diagnostics, 8u, &result);
    const arbor_view0_native_diagnostic *syntax = find_rule(
        diagnostics, result.diagnostic_count, ARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX);
    if (status.native != 0 || result.tokenizer_error_count != 0u ||
        result.tree_error_count != 1u ||
        count_rule(diagnostics,
                   result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED) != 0u ||
        syntax == NULL || syntax->severity != (uint32_t)ARBOR_VIEW0_NATIVE_SEVERITY_ERROR ||
        syntax->origin != (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_AUTHORING ||
        syntax->byte_offset != 10u || syntax->source_length != 3u ||
        syntax->line != 1u || syntax->column != 11u ||
        strcmp(syntax->symbolic_name, "ARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX") != 0 ||
        strcmp(syntax->message,
               "HTML DOCTYPE does not match an admitted authoring form") != 0) {
        return 5;
    }

    static const char public_id[] =
        "<!DOCTYPE html PUBLIC \"foo\" \"bar\"><title>x</title>";
    (void)memset(diagnostics, 0, sizeof(diagnostics));
    result = (arbor_view0_native_result){0};
    status = arbor_view0_native_check(
        span_from_cstr(public_id), diagnostics, 8u, &result);
    syntax = find_rule(
        diagnostics, result.diagnostic_count, ARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX);
    if (status.native != 0 || syntax == NULL || syntax->byte_offset != 15u ||
        syntax->source_length != 6u ||
        count_rule(diagnostics,
                   result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED) != 0u) {
        return 6;
    }

    static const char bad_legacy_value[] =
        "<!DOCTYPE html SYSTEM \"ABOUT:legacy-compat\"><title>x</title>";
    (void)memset(diagnostics, 0, sizeof(diagnostics));
    result = (arbor_view0_native_result){0};
    status = arbor_view0_native_check(
        span_from_cstr(bad_legacy_value), diagnostics, 8u, &result);
    syntax = find_rule(
        diagnostics, result.diagnostic_count, ARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX);
    if (status.native != 0 || syntax == NULL || syntax->byte_offset != 23u ||
        syntax->source_length != 19u) {
        return 7;
    }

    static const char no_space_before_system[] =
        "<!DOCTYPE htmlSYSTEM \"about:legacy-compat\"><title>x</title>";
    (void)memset(diagnostics, 0, sizeof(diagnostics));
    result = (arbor_view0_native_result){0};
    status = arbor_view0_native_check(
        span_from_cstr(no_space_before_system), diagnostics, 8u, &result);
    syntax = find_rule(
        diagnostics, result.diagnostic_count, ARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX);
    if (status.native != 0 || syntax == NULL || syntax->byte_offset != 14u ||
        syntax->source_length != 1u || result.tokenizer_error_count == 0u ||
        result.tree_error_count == 0u) {
        return 8;
    }

    static const char missing[] = "<title>x</title>";
    (void)memset(diagnostics, 0, sizeof(diagnostics));
    result = (arbor_view0_native_result){0};
    status = arbor_view0_native_check(
        span_from_cstr(missing), diagnostics, 8u, &result);
    if (status.native != 0 ||
        count_rule(diagnostics,
                   result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED) != 1u ||
        count_rule(diagnostics,
                   result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX) != 0u) {
        return 9;
    }

    puts("PASS: VIEW0 V1N1 G02 R2 canonical/legacy DOCTYPE syntax and deterministic invalid-component anchors");
    return 0;
}
