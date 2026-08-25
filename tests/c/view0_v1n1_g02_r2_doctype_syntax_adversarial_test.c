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
    static const char invalid[] = "<!DOCTYPE svg><title>x</title>";
    arbor_view0_native_result sentinel_result = {
        UINT64_C(0x1111111111111111),
        UINT64_C(0x2222222222222222),
        UINT64_C(0x3333333333333333),
        UINT64_C(0x4444444444444444)
    };
    const arbor_view0_native_result result_before = sentinel_result;
    arbor_view0_native_diagnostic too_small[1];
    (void)memset(too_small, 0xa5, sizeof(too_small));
    arbor_view0_native_diagnostic too_small_before[1];
    (void)memcpy(too_small_before, too_small, sizeof(too_small));

    arbor_status status = arbor_view0_native_check(
        span_from_cstr(invalid), too_small, 1u, &sentinel_result);
    if (status.native != -(int64_t)ENOSPC ||
        unchanged(too_small, too_small_before, sizeof(too_small)) != 0 ||
        unchanged(&sentinel_result, &result_before, sizeof(sentinel_result)) != 0) {
        return 1;
    }

    arbor_view0_native_diagnostic exact[2] = {{0}};
    arbor_view0_native_result result = {0};
    status = arbor_view0_native_check(
        span_from_cstr(invalid), exact, 2u, &result);
    if (status.native != 0 || result.diagnostic_count != 2u ||
        result.tokenizer_error_count != 0u || result.tree_error_count != 1u ||
        count_rule(exact,
                   result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX) != 1u ||
        count_rule(exact,
                   result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED) != 0u) {
        return 2;
    }

    static const char multiline_public[] =
        "<!--lead-->\n<!DOCTYPE html PUBLIC \"foo\" \"bar\"><title>x</title>";
    arbor_view0_native_diagnostic multiline[8] = {{0}};
    result = (arbor_view0_native_result){0};
    status = arbor_view0_native_check(
        span_from_cstr(multiline_public), multiline, 8u, &result);
    const arbor_view0_native_diagnostic *syntax = NULL;
    for (uint64_t i = 0u; i < result.diagnostic_count; ++i) {
        if (multiline[i].rule_id == ARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX) {
            syntax = &multiline[i];
            break;
        }
    }
    if (status.native != 0 || syntax == NULL || syntax->line != 2u ||
        syntax->column != 16u || syntax->source_length != 6u) {
        return 3;
    }

    static const char wrong_quote[] =
        "<!DOCTYPE html SYSTEM \"about:legacy-compat'><title>x</title>";
    arbor_view0_native_diagnostic quote_a[8] = {{0}};
    arbor_view0_native_diagnostic quote_b[8] = {{0}};
    arbor_view0_native_result result_a = {0};
    arbor_view0_native_result result_b = {0};
    status = arbor_view0_native_check(
        span_from_cstr(wrong_quote), quote_a, 8u, &result_a);
    if (status.native != 0 ||
        count_rule(quote_a,
                   result_a.diagnostic_count,
                   ARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX) != 1u) {
        return 4;
    }
    status = arbor_view0_native_check(
        span_from_cstr(wrong_quote), quote_b, 8u, &result_b);
    if (status.native != 0 ||
        unchanged(&result_a, &result_b, sizeof(result_a)) != 0 ||
        unchanged(quote_a,
                  quote_b,
                  (size_t)(result_a.diagnostic_count * sizeof(quote_a[0]))) != 0) {
        return 5;
    }

    static const char legacy[] =
        "<!DOCTYPE html SYSTEM \"about:legacy-compat\"><title>x</title>";
    arbor_view0_native_diagnostic legacy_diagnostics[4] = {{0}};
    result = (arbor_view0_native_result){0};
    status = arbor_view0_native_check(
        span_from_cstr(legacy), legacy_diagnostics, 4u, &result);
    if (status.native != 0 || result.tokenizer_error_count != 0u ||
        result.tree_error_count != 0u ||
        (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_PARSE_CLEAN) == 0u ||
        count_rule(legacy_diagnostics,
                   result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX) != 0u) {
        return 6;
    }

    static const uint8_t invalid_utf8[] = {
        '<','!','D','O','C','T','Y','P','E',' ','s','v','g','>',
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
                   ARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX) != 0u) {
        return 7;
    }

    puts("PASS: VIEW0 V1N1 G02 R2 syntax capacity atomicity, line anchors, determinism, legacy admission and UTF-8 precedence");
    return 0;
}
