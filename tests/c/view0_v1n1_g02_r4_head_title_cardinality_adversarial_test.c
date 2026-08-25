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
    static const char missing[] =
        "<!doctype html><head></head><body><p>x</p></body>";
    arbor_view0_native_result sentinel_result = {
        UINT64_C(0x1111111111111111),
        UINT64_C(0x2222222222222222),
        UINT64_C(0x3333333333333333),
        UINT64_C(0x4444444444444444)
    };
    const arbor_view0_native_result result_before = sentinel_result;

    arbor_status status = arbor_view0_native_check(
        span_from_cstr(missing), NULL, 0u, &sentinel_result);
    if (status.native != -(int64_t)ENOSPC ||
        unchanged(&sentinel_result, &result_before, sizeof(sentinel_result)) != 0) {
        return 1;
    }

    arbor_view0_native_diagnostic exact[1] = {{0}};
    arbor_view0_native_result result = {0};
    status = arbor_view0_native_check(
        span_from_cstr(missing), exact, 1u, &result);
    if (status.native != 0 || result.diagnostic_count != 1u ||
        exact[0].rule_id != ARBOR_VIEW_V1_G02_HEAD_TITLE_CARDINALITY ||
        exact[0].byte_offset != 0u || exact[0].source_length != 0u ||
        (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_PARSE_CLEAN) == 0u) {
        return 2;
    }

    static const char duplicate[] =
        "<!doctype html><title>a</title><title>b</title><p>x</p>";
    arbor_view0_native_diagnostic repeat_a[2] = {{0}};
    arbor_view0_native_diagnostic repeat_b[2] = {{0}};
    arbor_view0_native_result result_a = {0};
    arbor_view0_native_result result_b = {0};
    status = arbor_view0_native_check(
        span_from_cstr(duplicate), repeat_a, 2u, &result_a);
    if (status.native != 0 || result_a.diagnostic_count != 1u ||
        repeat_a[0].rule_id != ARBOR_VIEW_V1_G02_HEAD_TITLE_CARDINALITY ||
        repeat_a[0].byte_offset != 32u || repeat_a[0].source_length != 5u) {
        return 3;
    }
    status = arbor_view0_native_check(
        span_from_cstr(duplicate), repeat_b, 2u, &result_b);
    if (status.native != 0 ||
        unchanged(&result_a, &result_b, sizeof(result_a)) != 0 ||
        unchanged(repeat_a,
                  repeat_b,
                  (size_t)(result_a.diagnostic_count * sizeof(repeat_a[0]))) != 0) {
        return 4;
    }

    arbor_view0_native_diagnostic empty_small[1];
    (void)memset(empty_small, 0xa5, sizeof(empty_small));
    arbor_view0_native_diagnostic empty_small_before[1];
    (void)memcpy(empty_small_before, empty_small, sizeof(empty_small));
    sentinel_result = result_before;
    status = arbor_view0_native_check(
        (arbor_span){NULL, 0u}, empty_small, 1u, &sentinel_result);
    if (status.native != -(int64_t)ENOSPC ||
        unchanged(empty_small, empty_small_before, sizeof(empty_small)) != 0 ||
        unchanged(&sentinel_result, &result_before, sizeof(sentinel_result)) != 0) {
        return 5;
    }

    arbor_view0_native_diagnostic empty_exact[2] = {{0}};
    result = (arbor_view0_native_result){0};
    status = arbor_view0_native_check(
        (arbor_span){NULL, 0u}, empty_exact, 2u, &result);
    if (status.native != 0 || result.diagnostic_count != 2u ||
        result.tokenizer_error_count != 0u || result.tree_error_count != 0u ||
        (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_PARSE_CLEAN) == 0u ||
        empty_exact[0].rule_id != ARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED ||
        empty_exact[1].rule_id != ARBOR_VIEW_V1_G02_HEAD_TITLE_CARDINALITY) {
        return 6;
    }

    static const char legacy_missing_title[] =
        "<!DOCTYPE html SYSTEM \"about:legacy-compat\"><head></head><body><p>x</p></body>";
    arbor_view0_native_diagnostic legacy_small[1];
    (void)memset(legacy_small, 0x5a, sizeof(legacy_small));
    arbor_view0_native_diagnostic legacy_small_before[1];
    (void)memcpy(legacy_small_before, legacy_small, sizeof(legacy_small));
    sentinel_result = result_before;
    status = arbor_view0_native_check(
        span_from_cstr(legacy_missing_title),
        legacy_small,
        1u,
        &sentinel_result);
    if (status.native != -(int64_t)ENOSPC ||
        unchanged(legacy_small, legacy_small_before, sizeof(legacy_small)) != 0 ||
        unchanged(&sentinel_result, &result_before, sizeof(sentinel_result)) != 0) {
        return 7;
    }

    arbor_view0_native_diagnostic legacy_exact[2] = {{0}};
    result = (arbor_view0_native_result){0};
    status = arbor_view0_native_check(
        span_from_cstr(legacy_missing_title), legacy_exact, 2u, &result);
    if (status.native != 0 || result.diagnostic_count != 2u ||
        count_rule(legacy_exact,
                   result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_HEAD_TITLE_CARDINALITY) != 1u ||
        count_rule(legacy_exact,
                   result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_DOCTYPE_LEGACY_DISCOURAGED) != 1u) {
        return 8;
    }

    static const uint8_t invalid_utf8[] = {
        '<','!','d','o','c','t','y','p','e',' ','h','t','m','l','>',
        '<','h','e','a','d','>','<','/','h','e','a','d','>',
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
                   ARBOR_VIEW_V1_G02_HEAD_TITLE_CARDINALITY) != 0u) {
        return 9;
    }

    puts("PASS: VIEW0 V1N1 G02 R4 title-cardinality capacity atomicity, empty-document accumulation, determinism and UTF-8/R3 coexistence");
    return 0;
}
