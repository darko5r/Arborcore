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
    static const char duplicate_body[] =
        "<!doctype html><html><head><title>x</title></head>"
        "<body>x</body><body></body></html>";

    arbor_view0_native_diagnostic two_slots[2];
    (void)memset(two_slots, 0x5a, sizeof(two_slots));
    const arbor_view0_native_diagnostic two_slots_before[2] = {
        two_slots[0], two_slots[1]
    };
    arbor_view0_native_result sentinel_result = {
        UINT64_C(0x1111111111111111),
        UINT64_C(0x2222222222222222),
        UINT64_C(0x3333333333333333),
        UINT64_C(0x4444444444444444)
    };
    const arbor_view0_native_result result_before = sentinel_result;

    arbor_status status = arbor_view0_native_check(
        span_from_cstr(duplicate_body), two_slots, 2u, &sentinel_result);
    if (status.native != -(int64_t)ENOSPC ||
        unchanged(two_slots, two_slots_before, sizeof(two_slots)) != 0 ||
        unchanged(&sentinel_result, &result_before, sizeof(sentinel_result)) != 0) {
        return 1;
    }

    arbor_view0_native_diagnostic exact[3] = {{0}};
    arbor_view0_native_result result = {0};
    status = arbor_view0_native_check(
        span_from_cstr(duplicate_body), exact, 3u, &result);
    if (status.native != 0 || result.diagnostic_count != 3u ||
        result.tree_error_count != 2u ||
        count_rule(exact, result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_BODY_SINGLETON) != 1u) {
        return 2;
    }

    arbor_view0_native_diagnostic repeat_a[8] = {{0}};
    arbor_view0_native_diagnostic repeat_b[8] = {{0}};
    arbor_view0_native_result result_a = {0};
    arbor_view0_native_result result_b = {0};
    status = arbor_view0_native_check(
        span_from_cstr(duplicate_body), repeat_a, 8u, &result_a);
    if (status.native != 0) {
        return 3;
    }
    status = arbor_view0_native_check(
        span_from_cstr(duplicate_body), repeat_b, 8u, &result_b);
    if (status.native != 0 ||
        unchanged(&result_a, &result_b, sizeof(result_a)) != 0 ||
        unchanged(repeat_a, repeat_b,
                  (size_t)(result_a.diagnostic_count * sizeof(repeat_a[0]))) != 0) {
        return 4;
    }

    static const char all_cardinality[] =
        "<!doctype html><html><head><title>a</title><title>b</title>"
        "<base href=\"/\"><base href=\"/x\"></head>"
        "<body>x</body><body></body></html>";
    arbor_view0_native_diagnostic all[16] = {{0}};
    result = (arbor_view0_native_result){0};
    status = arbor_view0_native_check(
        span_from_cstr(all_cardinality), all, 16u, &result);
    if (status.native != 0 ||
        count_rule(all, result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_HEAD_TITLE_CARDINALITY) != 1u ||
        count_rule(all, result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_HEAD_BASE_CARDINALITY) != 1u ||
        count_rule(all, result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_BODY_SINGLETON) != 1u) {
        return 5;
    }

    static const char legacy_duplicate_body[] =
        "<!DOCTYPE html SYSTEM \"about:legacy-compat\"><title>x</title>"
        "<body>x</body><body></body>";
    arbor_view0_native_diagnostic legacy[16] = {{0}};
    result = (arbor_view0_native_result){0};
    status = arbor_view0_native_check(
        span_from_cstr(legacy_duplicate_body), legacy, 16u, &result);
    if (status.native != 0 ||
        count_rule(legacy, result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_DOCTYPE_LEGACY_DISCOURAGED) != 1u ||
        count_rule(legacy, result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_BODY_SINGLETON) != 1u) {
        return 6;
    }

    static const uint8_t invalid_utf8[] = {
        '<','!','d','o','c','t','y','p','e',' ','h','t','m','l','>',
        '<','t','i','t','l','e','>','x','<','/','t','i','t','l','e','>',
        '<','b','o','d','y','>','<','/','b','o','d','y','>',
        '<','b','o','d','y','>','<','/','b','o','d','y','>',
        UINT8_C(0xf0), UINT8_C(0x80), UINT8_C(0x80), UINT8_C(0x80)
    };
    arbor_view0_native_diagnostic utf8_diagnostics[8] = {{0}};
    result = (arbor_view0_native_result){0};
    status = arbor_view0_native_check(
        (arbor_span){invalid_utf8, (uint64_t)sizeof(invalid_utf8)},
        utf8_diagnostics, 8u, &result);
    if (status.native != 0 || result.diagnostic_count != 1u ||
        utf8_diagnostics[0].origin != (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_UTF8 ||
        count_rule(utf8_diagnostics, result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_BODY_SINGLETON) != 0u) {
        return 7;
    }

    puts("PASS: VIEW0 V1N1 G02 R6 body-singleton parser-repair capacity atomicity, determinism, G02 accumulation and UTF-8 precedence");
    return 0;
}
