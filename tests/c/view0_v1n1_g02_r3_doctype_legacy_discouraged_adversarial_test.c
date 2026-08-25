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
    static const char legacy[] =
        "<!DOCTYPE html SYSTEM \"about:legacy-compat\"><title>x</title>";

    arbor_view0_native_result sentinel_result = {
        UINT64_C(0x1111111111111111),
        UINT64_C(0x2222222222222222),
        UINT64_C(0x3333333333333333),
        UINT64_C(0x4444444444444444)
    };
    const arbor_view0_native_result result_before = sentinel_result;
    arbor_status status = arbor_view0_native_check(
        span_from_cstr(legacy), NULL, 0u, &sentinel_result);
    if (status.native != -(int64_t)ENOSPC ||
        unchanged(&sentinel_result, &result_before, sizeof(sentinel_result)) != 0) {
        return 1;
    }

    arbor_view0_native_diagnostic exact[1] = {{0}};
    arbor_view0_native_result result = {0};
    status = arbor_view0_native_check(
        span_from_cstr(legacy), exact, 1u, &result);
    if (status.native != 0 || result.diagnostic_count != 1u ||
        result.tokenizer_error_count != 0u || result.tree_error_count != 0u ||
        (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_PARSE_CLEAN) == 0u ||
        exact[0].rule_id != ARBOR_VIEW_V1_G02_DOCTYPE_LEGACY_DISCOURAGED ||
        exact[0].severity != (uint32_t)ARBOR_VIEW0_NATIVE_SEVERITY_WARNING) {
        return 2;
    }

    static const char multiline[] =
        "<!--lead-->\n<!DOCTYPE html SYSTEM \"about:legacy-compat\"><title>x</title>";
    arbor_view0_native_diagnostic line_diagnostics[4] = {{0}};
    result = (arbor_view0_native_result){0};
    status = arbor_view0_native_check(
        span_from_cstr(multiline), line_diagnostics, 4u, &result);
    const arbor_view0_native_diagnostic *warning = NULL;
    for (uint64_t i = 0u; i < result.diagnostic_count; ++i) {
        if (line_diagnostics[i].rule_id ==
            ARBOR_VIEW_V1_G02_DOCTYPE_LEGACY_DISCOURAGED) {
            warning = &line_diagnostics[i];
            break;
        }
    }
    if (status.native != 0 || warning == NULL || warning->line != 2u ||
        warning->column != 24u || warning->source_length != 19u) {
        return 3;
    }

    arbor_view0_native_diagnostic repeat_a[4] = {{0}};
    arbor_view0_native_diagnostic repeat_b[4] = {{0}};
    arbor_view0_native_result result_a = {0};
    arbor_view0_native_result result_b = {0};
    status = arbor_view0_native_check(
        span_from_cstr(legacy), repeat_a, 4u, &result_a);
    if (status.native != 0) {
        return 4;
    }
    status = arbor_view0_native_check(
        span_from_cstr(legacy), repeat_b, 4u, &result_b);
    if (status.native != 0 ||
        unchanged(&result_a, &result_b, sizeof(result_a)) != 0 ||
        unchanged(repeat_a,
                  repeat_b,
                  (size_t)(result_a.diagnostic_count * sizeof(repeat_a[0]))) != 0) {
        return 5;
    }

    static const uint8_t invalid_utf8[] = {
        '<','!','D','O','C','T','Y','P','E',' ','h','t','m','l',' ',
        'S','Y','S','T','E','M',' ','\"','a','b','o','u','t',':','l','e','g','a','c','y','-','c','o','m','p','a','t','\"','>',
        UINT8_C(0xf0), UINT8_C(0x80), UINT8_C(0x80), UINT8_C(0x80)
    };
    arbor_view0_native_diagnostic utf8_diagnostics[4] = {{0}};
    result = (arbor_view0_native_result){0};
    status = arbor_view0_native_check(
        (arbor_span){invalid_utf8, (uint64_t)sizeof(invalid_utf8)},
        utf8_diagnostics,
        4u,
        &result);
    if (status.native != 0 || result.diagnostic_count != 1u ||
        utf8_diagnostics[0].origin != (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_UTF8 ||
        count_rule(utf8_diagnostics,
                   result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_DOCTYPE_LEGACY_DISCOURAGED) != 0u) {
        return 6;
    }

    static const char bad_legacy[] =
        "<!DOCTYPE html SYSTEM \"ABOUT:legacy-compat\"><title>x</title>";
    arbor_view0_native_diagnostic bad_diagnostics[8] = {{0}};
    result = (arbor_view0_native_result){0};
    status = arbor_view0_native_check(
        span_from_cstr(bad_legacy), bad_diagnostics, 8u, &result);
    if (status.native != 0 ||
        count_rule(bad_diagnostics,
                   result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX) != 1u ||
        count_rule(bad_diagnostics,
                   result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_DOCTYPE_LEGACY_DISCOURAGED) != 0u) {
        return 7;
    }

    puts("PASS: VIEW0 V1N1 G02 R3 warning capacity atomicity, determinism, line anchors and UTF-8/R2 precedence");
    return 0;
}
