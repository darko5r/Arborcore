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
        "<html><head><title>x</title></head><body><p>x</p></body></html>";
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

    static const char parse_and_missing[] =
        "<html><head><title>x</title></head><body><?bad><p>x</p></body></html>";
    arbor_view0_native_diagnostic too_small[2];
    (void)memset(too_small, 0xa5, sizeof(too_small));
    arbor_view0_native_diagnostic too_small_before[2];
    (void)memcpy(too_small_before, too_small, sizeof(too_small));
    sentinel_result = result_before;
    status = arbor_view0_native_check(
        span_from_cstr(parse_and_missing), too_small, 2u, &sentinel_result);
    if (status.native != -(int64_t)ENOSPC ||
        unchanged(too_small, too_small_before, sizeof(too_small)) != 0 ||
        unchanged(&sentinel_result, &result_before, sizeof(sentinel_result)) != 0) {
        return 2;
    }

    arbor_view0_native_diagnostic combined_a[4] = {{0}};
    arbor_view0_native_diagnostic combined_b[4] = {{0}};
    arbor_view0_native_result result_a = {0};
    arbor_view0_native_result result_b = {0};
    status = arbor_view0_native_check(
        span_from_cstr(parse_and_missing), combined_a, 4u, &result_a);
    if (status.native != 0 || result_a.diagnostic_count != 3u ||
        result_a.tokenizer_error_count != 1u || result_a.tree_error_count != 1u ||
        (result_a.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_PARSE_CLEAN) != 0u ||
        count_rule(combined_a,
                   result_a.diagnostic_count,
                   ARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED) != 1u) {
        return 3;
    }
    status = arbor_view0_native_check(
        span_from_cstr(parse_and_missing), combined_b, 4u, &result_b);
    if (status.native != 0 ||
        unchanged(&result_a, &result_b, sizeof(result_a)) != 0 ||
        unchanged(combined_a,
                  combined_b,
                  (size_t)(result_a.diagnostic_count * sizeof(combined_a[0]))) != 0) {
        return 4;
    }

    arbor_view0_native_parse_counts measured_counts = {
        UINT64_C(0x1111111111111111), UINT64_C(0x2222222222222222)
    };
    arbor_view0_native_document_facts measured_facts;
    (void)memset(&measured_facts, 0x5a, sizeof(measured_facts));
    status = arbor_view0_native_lexbor_measure(
        span_from_cstr(parse_and_missing), &measured_counts, &measured_facts);
    if (status.native != 0 || measured_counts.tokenizer_error_count != 1u ||
        measured_counts.tree_error_count != 1u ||
        measured_facts.dom_doctype_node_count != 0u) {
        return 5;
    }

    union measure_alias {
        arbor_view0_native_parse_counts counts;
        arbor_view0_native_document_facts facts;
    } alias = {0};
    status = arbor_view0_native_lexbor_measure(
        span_from_cstr(parse_and_missing), &alias.counts, &alias.facts);
    if (status.native != -(int64_t)EINVAL) {
        return 6;
    }

    static const uint8_t invalid_utf8[] = {
        '<','p','>', UINT8_C(0xf0), UINT8_C(0x80), UINT8_C(0x80), UINT8_C(0x80),
        '<','/','p','>'
    };
    arbor_view0_native_diagnostic utf8_diagnostics[2] = {{0}};
    arbor_view0_native_result utf8_result = {0};
    status = arbor_view0_native_check(
        (arbor_span){invalid_utf8, (uint64_t)sizeof(invalid_utf8)},
        utf8_diagnostics,
        2u,
        &utf8_result);
    if (status.native != 0 || utf8_result.diagnostic_count != 1u ||
        utf8_diagnostics[0].origin != (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_UTF8 ||
        count_rule(utf8_diagnostics,
                   utf8_result.diagnostic_count,
                   ARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED) != 0u) {
        return 7;
    }

    puts("PASS: VIEW0 V1N1 G02 R1 capacity failure atomicity, two-pass measurement, determinism and UTF-8 precedence");
    return 0;
}
