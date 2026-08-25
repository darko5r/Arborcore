#include <arborcore/view0_conformance/native.h>

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static arbor_span span_from_cstr(const char *text)
{
    return (arbor_span){(const uint8_t *)text, (uint64_t)strlen(text)};
}

static uint64_t count_rule(
    const arbor_view0_native_diagnostic *diagnostics,
    uint64_t count,
    uint64_t rule_id)
{
    uint64_t found = 0u;
    for (uint64_t i = 0u; i < count; ++i) {
        if (diagnostics[i].rule_id == rule_id) found += 1u;
    }
    return found;
}

static int sorted(const arbor_view0_native_diagnostic *d, uint64_t n)
{
    for (uint64_t i = 1u; i < n; ++i) {
        if (d[i - 1u].byte_offset > d[i].byte_offset) return 0;
        if (d[i - 1u].byte_offset < d[i].byte_offset) continue;
        if (d[i - 1u].rule_id > d[i].rule_id) return 0;
        if (d[i - 1u].rule_id < d[i].rule_id) continue;
        if (d[i - 1u].severity > d[i].severity) return 0;
        if (d[i - 1u].severity < d[i].severity) continue;
        if (d[i - 1u].discovery_sequence > d[i].discovery_sequence) return 0;
    }
    return 1;
}

int main(void)
{
    static const char combined[] =
        "<!doctype html><title>x</title>"
        "<details><p>x</p></details>"
        "<figure><figcaption>a</figcaption><p>x</p><figcaption>b</figcaption></figure>"
        "<hgroup>text</hgroup><picture></picture>";
    arbor_view0_native_diagnostic ample[32] = {{0}};
    arbor_view0_native_result ample_result = {0};
    arbor_status status = arbor_view0_native_check(
        span_from_cstr(combined), ample, 32u, &ample_result);
    if (status.native != 0 ||
        count_rule(ample, ample_result.diagnostic_count,
                   ARBOR_VIEW_V1_G03_CONTENT_MODEL) != 4u ||
        !sorted(ample, ample_result.diagnostic_count)) return 1;

    const uint64_t exact_count = ample_result.diagnostic_count;
    arbor_view0_native_diagnostic small[32];
    arbor_view0_native_diagnostic before[32];
    (void)memset(small, 0x5a, sizeof(small));
    (void)memcpy(before, small, sizeof(small));
    arbor_view0_native_result sentinel = {
        UINT64_C(0x11), UINT64_C(0x22), UINT64_C(0x33), UINT64_C(0x44)};
    const arbor_view0_native_result sentinel_before = sentinel;
    status = arbor_view0_native_check(
        span_from_cstr(combined), small, exact_count - 1u, &sentinel);
    if (status.native != -(int64_t)ENOSPC ||
        memcmp(small, before, sizeof(small)) != 0 ||
        memcmp(&sentinel, &sentinel_before, sizeof(sentinel)) != 0) return 2;

    arbor_view0_native_diagnostic exact[32] = {{0}};
    arbor_view0_native_result exact_result = {0};
    status = arbor_view0_native_check(
        span_from_cstr(combined), exact, exact_count, &exact_result);
    if (status.native != 0 ||
        memcmp(&ample_result, &exact_result, sizeof(ample_result)) != 0 ||
        memcmp(ample, exact, (size_t)(exact_count * sizeof(exact[0]))) != 0) return 3;

    /* Multiple residual defects in one parent still produce one R2 diagnostic. */
    static const char one_parent[] =
        "<!doctype html><title>x</title><hgroup>bad text</hgroup>";
    arbor_view0_native_diagnostic one[8] = {{0}};
    arbor_view0_native_result one_result = {0};
    status = arbor_view0_native_check(span_from_cstr(one_parent), one, 8u, &one_result);
    if (status.native != 0 ||
        count_rule(one, one_result.diagnostic_count,
                   ARBOR_VIEW_V1_G03_CONTENT_MODEL) != 1u) return 4;

    /* A lone dt remains R1-only: no duplicate R2 diagnostic. */
    static const char r1_owned[] =
        "<!doctype html><title>x</title><dl><dt>x</dt></dl>";
    arbor_view0_native_diagnostic r1d[8] = {{0}};
    arbor_view0_native_result r1r = {0};
    status = arbor_view0_native_check(span_from_cstr(r1_owned), r1d, 8u, &r1r);
    if (status.native != 0 ||
        count_rule(r1d, r1r.diagnostic_count, ARBOR_VIEW_V1_G03_ELEMENT_CONTEXT) != 1u ||
        count_rule(r1d, r1r.diagnostic_count, ARBOR_VIEW_V1_G03_CONTENT_MODEL) != 0u) return 5;

    /* UTF-8 validation remains first and suppresses all later authoring rules. */
    static const uint8_t invalid_utf8[] = {
        '<','!','d','o','c','t','y','p','e',' ','h','t','m','l','>',
        '<','t','i','t','l','e','>','x','<','/','t','i','t','l','e','>',
        '<','d','e','t','a','i','l','s','>','<','/','d','e','t','a','i','l','s','>',
        UINT8_C(0xf0), UINT8_C(0x80), UINT8_C(0x80), UINT8_C(0x80)};
    arbor_view0_native_diagnostic utf8d[8] = {{0}};
    arbor_view0_native_result utf8r = {0};
    status = arbor_view0_native_check(
        (arbor_span){invalid_utf8, (uint64_t)sizeof(invalid_utf8)}, utf8d, 8u, &utf8r);
    if (status.native != 0 || utf8r.diagnostic_count != 1u ||
        utf8d[0].origin != (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_UTF8 ||
        count_rule(utf8d, utf8r.diagnostic_count, ARBOR_VIEW_V1_G03_CONTENT_MODEL) != 0u) return 6;

    /* R2A owns the same accepted 4097-inclusive observation ceiling as R1A. */
    static char deep[65536];
    size_t used = 0u;
    static const char prefix[] = "<!doctype html><title>x</title>";
    (void)memcpy(deep + used, prefix, sizeof(prefix) - 1u);
    used += sizeof(prefix) - 1u;
    for (size_t i = 0u; i < 4096u; ++i) {
        static const char open[] = "<div>";
        (void)memcpy(deep + used, open, sizeof(open) - 1u);
        used += sizeof(open) - 1u;
    }
    for (size_t i = 0u; i < 4096u; ++i) {
        static const char close[] = "</div>";
        (void)memcpy(deep + used, close, sizeof(close) - 1u);
        used += sizeof(close) - 1u;
    }
    arbor_view0_native_diagnostic deepd[8] = {{0}};
    arbor_view0_native_result deepr = {0};
    status = arbor_view0_native_check(
        (arbor_span){(const uint8_t *)deep, (uint64_t)used}, deepd, 8u, &deepr);
    if (status.native != 0 ||
        count_rule(deepd, deepr.diagnostic_count, ARBOR_VIEW_V1_G03_CONTENT_MODEL) != 0u ||
        (deepr.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_PARTIAL) == 0u) return 7;

    puts("PASS: VIEW0 V1N1 G03 R2A capacity atomicity, one-parent bound, deterministic ordering, R1 suppression and UTF-8 precedence");
    return 0;
}
