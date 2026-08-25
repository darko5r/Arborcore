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


static int diagnostics_sorted(const arbor_view0_native_diagnostic *diagnostics, uint64_t count)
{
    for (uint64_t i = 1u; i < count; ++i) {
        const arbor_view0_native_diagnostic *left = diagnostics + (i - 1u);
        const arbor_view0_native_diagnostic *right = diagnostics + i;
        if (left->byte_offset > right->byte_offset) return 0;
        if (left->byte_offset < right->byte_offset) continue;
        if (left->rule_id > right->rule_id) return 0;
        if (left->rule_id < right->rule_id) continue;
        if (left->severity > right->severity) return 0;
        if (left->severity < right->severity) continue;
        if (left->discovery_sequence > right->discovery_sequence) return 0;
    }
    return 1;
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
    static const char combined[] =
        "<!doctype html><title>x</title>"
        "<dl><dd>x</dd></dl>"
        "<div><li>x</li></div>"
        "<details>text<summary>x</summary></details>"
        "<p><link rel=alternate href=x>x</p>";

    arbor_view0_native_diagnostic ample[32] = {{0}};
    arbor_view0_native_result ample_result = {0};
    arbor_status status = arbor_view0_native_check(
        span_from_cstr(combined), ample, 32u, &ample_result);
    if (status.native != 0 ||
        count_rule(ample, ample_result.diagnostic_count,
                   ARBOR_VIEW_V1_G03_ELEMENT_CONTEXT) != 4u ||
        (ample_result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R1_PARTIAL) == 0u) {
        return 1;
    }

    const uint64_t exact_count = ample_result.diagnostic_count;
    if (exact_count == 0u || exact_count > 32u) {
        return 2;
    }

    arbor_view0_native_diagnostic too_small[32];
    (void)memset(too_small, 0x5a, sizeof(too_small));
    arbor_view0_native_diagnostic too_small_before[32];
    (void)memcpy(too_small_before, too_small, sizeof(too_small));
    arbor_view0_native_result sentinel = {
        UINT64_C(0x1111111111111111),
        UINT64_C(0x2222222222222222),
        UINT64_C(0x3333333333333333),
        UINT64_C(0x4444444444444444)
    };
    const arbor_view0_native_result sentinel_before = sentinel;
    status = arbor_view0_native_check(
        span_from_cstr(combined),
        too_small,
        exact_count - 1u,
        &sentinel);
    if (status.native != -(int64_t)ENOSPC ||
        unchanged(too_small, too_small_before, sizeof(too_small)) != 0 ||
        unchanged(&sentinel, &sentinel_before, sizeof(sentinel)) != 0) {
        return 3;
    }

    arbor_view0_native_diagnostic exact[32] = {{0}};
    arbor_view0_native_result exact_result = {0};
    status = arbor_view0_native_check(
        span_from_cstr(combined), exact, exact_count, &exact_result);
    if (status.native != 0 ||
        unchanged(&ample_result, &exact_result, sizeof(ample_result)) != 0 ||
        unchanged(ample, exact,
                  (size_t)(exact_count * sizeof(exact[0]))) != 0) {
        return 4;
    }

    arbor_view0_native_diagnostic repeat[32] = {{0}};
    arbor_view0_native_result repeat_result = {0};
    status = arbor_view0_native_check(
        span_from_cstr(combined), repeat, 32u, &repeat_result);
    if (status.native != 0 ||
        unchanged(&ample_result, &repeat_result, sizeof(ample_result)) != 0 ||
        unchanged(ample, repeat,
                  (size_t)(exact_count * sizeof(repeat[0]))) != 0) {
        return 5;
    }

    static const char named_form[] =
        "<!doctype html><title>x</title><form title=n><main><p>x</p></main></form>";
    arbor_view0_native_diagnostic deferred_diags[8] = {{0}};
    arbor_view0_native_result deferred_result = {0};
    status = arbor_view0_native_check(
        span_from_cstr(named_form), deferred_diags, 8u, &deferred_result);
    if (status.native != 0 ||
        count_rule(deferred_diags, deferred_result.diagnostic_count,
                   ARBOR_VIEW_V1_G03_ELEMENT_CONTEXT) != 0u ||
        (deferred_result.flags &
         ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R1_DEFERRED_MAIN_FORM) == 0u) {
        return 6;
    }

    static const char deep_select[] =
        "<!doctype html><title>x</title><select><option><div><div><div>"
        "<span>x</span></div></div></div></option></select>";
    arbor_view0_native_diagnostic deep_diags[8] = {{0}};
    arbor_view0_native_result deep_result = {0};
    status = arbor_view0_native_check(
        span_from_cstr(deep_select), deep_diags, 8u, &deep_result);
    if (status.native != 0 ||
        count_rule(deep_diags, deep_result.diagnostic_count,
                   ARBOR_VIEW_V1_G03_ELEMENT_CONTEXT) != 0u) {
        return 7;
    }

    static const char datalist_option_div[] =
        "<!doctype html><title>x</title><datalist><option><div>x</div></option>"
        "</datalist>";
    arbor_view0_native_diagnostic datalist_diags[8] = {{0}};
    arbor_view0_native_result datalist_result = {0};
    status = arbor_view0_native_check(
        span_from_cstr(datalist_option_div), datalist_diags, 8u, &datalist_result);
    if (status.native != 0 ||
        count_rule(datalist_diags, datalist_result.diagnostic_count,
                   ARBOR_VIEW_V1_G03_ELEMENT_CONTEXT) != 0u) {
        return 8;
    }

    static const char g02_ownership[] =
        "<!doctype html><html><head><title>x</title><title>y</title>"
        "<base href=/><base href=/x></head><body></body></html>";
    arbor_view0_native_diagnostic ownership_diags[16] = {{0}};
    arbor_view0_native_result ownership_result = {0};
    status = arbor_view0_native_check(
        span_from_cstr(g02_ownership), ownership_diags, 16u, &ownership_result);
    if (status.native != 0 ||
        count_rule(ownership_diags, ownership_result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_HEAD_TITLE_CARDINALITY) != 1u ||
        count_rule(ownership_diags, ownership_result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_HEAD_BASE_CARDINALITY) != 1u ||
        count_rule(ownership_diags, ownership_result.diagnostic_count,
                   ARBOR_VIEW_V1_G03_ELEMENT_CONTEXT) != 0u) {
        return 10;
    }

    static const char mixed_order[] =
        "<title>x</title><div><li>x</li></div>";
    arbor_view0_native_diagnostic mixed_diags[16] = {{0}};
    arbor_view0_native_result mixed_result = {0};
    status = arbor_view0_native_check(
        span_from_cstr(mixed_order), mixed_diags, 16u, &mixed_result);
    if (status.native != 0 ||
        count_rule(mixed_diags, mixed_result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED) != 1u ||
        count_rule(mixed_diags, mixed_result.diagnostic_count,
                   ARBOR_VIEW_V1_G03_ELEMENT_CONTEXT) != 1u ||
        diagnostics_sorted(mixed_diags, mixed_result.diagnostic_count) == 0) {
        return 11;
    }

    static const uint8_t invalid_utf8[] = {
        '<','!','d','o','c','t','y','p','e',' ','h','t','m','l','>',
        '<','t','i','t','l','e','>','x','<','/','t','i','t','l','e','>',
        '<','d','i','v','>','<','l','i','>','x','<','/','l','i','>',
        '<','/','d','i','v','>', UINT8_C(0xf0), UINT8_C(0x80), UINT8_C(0x80), UINT8_C(0x80)
    };
    arbor_view0_native_diagnostic utf8_diags[8] = {{0}};
    arbor_view0_native_result utf8_result = {0};
    status = arbor_view0_native_check(
        (arbor_span){invalid_utf8, (uint64_t)sizeof(invalid_utf8)},
        utf8_diags, 8u, &utf8_result);
    if (status.native != 0 || utf8_result.diagnostic_count != 1u ||
        utf8_diags[0].origin != (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_UTF8 ||
        count_rule(utf8_diags, utf8_result.diagnostic_count,
                   ARBOR_VIEW_V1_G03_ELEMENT_CONTEXT) != 0u ||
        (utf8_result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R1_PARTIAL) == 0u) {
        return 12;
    }

    puts("PASS: VIEW0 V1N1 G03 R1A aggregate capacity atomicity, deterministic mixed-rule ordering, G02 ownership, transparent-depth behavior, explicit deferred branch and UTF-8 precedence");
    return 0;
}
