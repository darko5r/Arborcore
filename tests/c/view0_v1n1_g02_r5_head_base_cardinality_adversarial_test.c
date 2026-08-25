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
    static const char duplicate_base[] =
        "<!doctype html><title>x</title><base href=\"/\"><base href=\"/x\">";
    arbor_view0_native_result sentinel_result = {
        UINT64_C(0x1111111111111111),
        UINT64_C(0x2222222222222222),
        UINT64_C(0x3333333333333333),
        UINT64_C(0x4444444444444444)
    };
    const arbor_view0_native_result result_before = sentinel_result;

    arbor_status status = arbor_view0_native_check(
        span_from_cstr(duplicate_base), NULL, 0u, &sentinel_result);
    if (status.native != -(int64_t)ENOSPC ||
        unchanged(&sentinel_result, &result_before, sizeof(sentinel_result)) != 0) {
        return 1;
    }

    arbor_view0_native_diagnostic exact[1] = {{0}};
    arbor_view0_native_result result = {0};
    status = arbor_view0_native_check(
        span_from_cstr(duplicate_base), exact, 1u, &result);
    if (status.native != 0 || result.diagnostic_count != 1u ||
        exact[0].rule_id != ARBOR_VIEW_V1_G02_HEAD_BASE_CARDINALITY ||
        exact[0].byte_offset != 47u || exact[0].source_length != 4u ||
        exact[0].line != 1u || exact[0].column != 48u ||
        (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_PARSE_CLEAN) == 0u) {
        return 2;
    }

    arbor_view0_native_diagnostic repeat_a[2] = {{0}};
    arbor_view0_native_diagnostic repeat_b[2] = {{0}};
    arbor_view0_native_result result_a = {0};
    arbor_view0_native_result result_b = {0};
    status = arbor_view0_native_check(
        span_from_cstr(duplicate_base), repeat_a, 2u, &result_a);
    if (status.native != 0) {
        return 3;
    }
    status = arbor_view0_native_check(
        span_from_cstr(duplicate_base), repeat_b, 2u, &result_b);
    if (status.native != 0 ||
        unchanged(&result_a, &result_b, sizeof(result_a)) != 0 ||
        unchanged(repeat_a, repeat_b,
                  (size_t)(result_a.diagnostic_count * sizeof(repeat_a[0]))) != 0) {
        return 4;
    }

    static const char duplicate_title_and_base[] =
        "<!doctype html><title>a</title><title>b</title>"
        "<base href=\"/\"><base href=\"/x\"><p>x</p>";
    arbor_view0_native_diagnostic one_slot[1];
    (void)memset(one_slot, 0x5a, sizeof(one_slot));
    arbor_view0_native_diagnostic one_slot_before[1];
    (void)memcpy(one_slot_before, one_slot, sizeof(one_slot));
    sentinel_result = result_before;
    status = arbor_view0_native_check(
        span_from_cstr(duplicate_title_and_base), one_slot, 1u, &sentinel_result);
    if (status.native != -(int64_t)ENOSPC ||
        unchanged(one_slot, one_slot_before, sizeof(one_slot)) != 0 ||
        unchanged(&sentinel_result, &result_before, sizeof(sentinel_result)) != 0) {
        return 5;
    }

    arbor_view0_native_diagnostic two_slots[2] = {{0}};
    result = (arbor_view0_native_result){0};
    status = arbor_view0_native_check(
        span_from_cstr(duplicate_title_and_base), two_slots, 2u, &result);
    if (status.native != 0 || result.diagnostic_count != 2u ||
        count_rule(two_slots, result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_HEAD_TITLE_CARDINALITY) != 1u ||
        count_rule(two_slots, result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_HEAD_BASE_CARDINALITY) != 1u) {
        return 6;
    }

    static const char legacy_duplicate_base[] =
        "<!DOCTYPE html SYSTEM \"about:legacy-compat\"><title>x</title>"
        "<base href=\"/\"><base href=\"/x\">";
    arbor_view0_native_diagnostic legacy[4] = {{0}};
    result = (arbor_view0_native_result){0};
    status = arbor_view0_native_check(
        span_from_cstr(legacy_duplicate_base), legacy, 4u, &result);
    if (status.native != 0 ||
        count_rule(legacy, result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_DOCTYPE_LEGACY_DISCOURAGED) != 1u ||
        count_rule(legacy, result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_HEAD_BASE_CARDINALITY) != 1u) {
        return 7;
    }

    static const uint8_t invalid_utf8[] = {
        '<','!','d','o','c','t','y','p','e',' ','h','t','m','l','>',
        '<','t','i','t','l','e','>','x','<','/','t','i','t','l','e','>',
        '<','b','a','s','e',' ','h','r','e','f','=','\"','/','\"','>',
        '<','b','a','s','e',' ','h','r','e','f','=','\"','/','x','\"','>',
        UINT8_C(0xf0), UINT8_C(0x80), UINT8_C(0x80), UINT8_C(0x80)
    };
    arbor_view0_native_diagnostic utf8_diagnostics[4] = {{0}};
    result = (arbor_view0_native_result){0};
    status = arbor_view0_native_check(
        (arbor_span){invalid_utf8, (uint64_t)sizeof(invalid_utf8)},
        utf8_diagnostics, 4u, &result);
    if (status.native != 0 || result.diagnostic_count != 1u ||
        utf8_diagnostics[0].origin != (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_UTF8 ||
        count_rule(utf8_diagnostics, result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_HEAD_BASE_CARDINALITY) != 0u) {
        return 8;
    }

    puts("PASS: VIEW0 V1N1 G02 R5 base-cardinality capacity atomicity, determinism, R4/R3 accumulation and UTF-8 precedence");
    return 0;
}
