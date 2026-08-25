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
    static const char canonical[] = "<!DOCTYPE HTML><title>x</title>";
    arbor_view0_native_diagnostic diagnostics[8] = {{0}};
    arbor_view0_native_result result = {0};
    arbor_status status = arbor_view0_native_check(
        span_from_cstr(canonical), diagnostics, 8u, &result);
    if (status.native != 0 || result.diagnostic_count != 0u ||
        result.tokenizer_error_count != 0u || result.tree_error_count != 0u ||
        (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_PARSE_CLEAN) == 0u) {
        return 1;
    }

    static const char legacy_double[] =
        "<!DOCTYPE html SYSTEM \"about:legacy-compat\"><title>x</title>";
    (void)memset(diagnostics, 0, sizeof(diagnostics));
    result = (arbor_view0_native_result){0};
    status = arbor_view0_native_check(
        span_from_cstr(legacy_double), diagnostics, 8u, &result);
    const arbor_view0_native_diagnostic *warning = find_rule(
        diagnostics,
        result.diagnostic_count,
        ARBOR_VIEW_V1_G02_DOCTYPE_LEGACY_DISCOURAGED);
    if (status.native != 0 || result.diagnostic_count != 1u ||
        result.tokenizer_error_count != 0u || result.tree_error_count != 0u ||
        (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_PARSE_CLEAN) == 0u ||
        warning == NULL ||
        warning->severity != (uint32_t)ARBOR_VIEW0_NATIVE_SEVERITY_WARNING ||
        warning->origin != (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_AUTHORING ||
        warning->byte_offset != 23u || warning->source_length != 19u ||
        warning->line != 1u || warning->column != 24u ||
        strcmp(warning->symbolic_name,
               "ARBOR_VIEW_V1_G02_DOCTYPE_LEGACY_DISCOURAGED") != 0 ||
        strcmp(warning->message,
               "Legacy DOCTYPE compatibility string should not be used unless required by a generator limitation") != 0 ||
        count_rule(diagnostics,
                   result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX) != 0u ||
        count_rule(diagnostics,
                   result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED) != 0u) {
        return 2;
    }

    static const char legacy_single[] =
        "<!DOCTYPE HTML\nSYSTEM\t'about:legacy-compat' \r><title>x</title>";
    (void)memset(diagnostics, 0, sizeof(diagnostics));
    result = (arbor_view0_native_result){0};
    status = arbor_view0_native_check(
        span_from_cstr(legacy_single), diagnostics, 8u, &result);
    warning = find_rule(
        diagnostics,
        result.diagnostic_count,
        ARBOR_VIEW_V1_G02_DOCTYPE_LEGACY_DISCOURAGED);
    if (status.native != 0 || warning == NULL || warning->line != 2u ||
        warning->column != 9u || warning->source_length != 19u ||
        count_rule(diagnostics,
                   result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX) != 0u) {
        return 3;
    }

    static const char bad_case[] =
        "<!DOCTYPE html SYSTEM \"ABOUT:legacy-compat\"><title>x</title>";
    (void)memset(diagnostics, 0, sizeof(diagnostics));
    result = (arbor_view0_native_result){0};
    status = arbor_view0_native_check(
        span_from_cstr(bad_case), diagnostics, 8u, &result);
    if (status.native != 0 ||
        count_rule(diagnostics,
                   result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX) != 1u ||
        count_rule(diagnostics,
                   result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_DOCTYPE_LEGACY_DISCOURAGED) != 0u) {
        return 4;
    }

    static const char public_id[] =
        "<!DOCTYPE html PUBLIC \"foo\" \"bar\"><title>x</title>";
    (void)memset(diagnostics, 0, sizeof(diagnostics));
    result = (arbor_view0_native_result){0};
    status = arbor_view0_native_check(
        span_from_cstr(public_id), diagnostics, 8u, &result);
    if (status.native != 0 ||
        count_rule(diagnostics,
                   result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX) != 1u ||
        count_rule(diagnostics,
                   result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_DOCTYPE_LEGACY_DISCOURAGED) != 0u) {
        return 5;
    }

    puts("PASS: VIEW0 V1N1 G02 R3 legacy-compat warning severity, exact anchor and R2 syntax separation");
    return 0;
}
