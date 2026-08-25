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
        "<a href=/><button>x</button></a>"
        "<header><footer>x</footer></header>"
        "<p><dfn><dfn>x</dfn></dfn></p>"
        "<canvas><textarea>x</textarea></canvas>";
    arbor_view0_native_diagnostic ample[32] = {{0}};
    arbor_view0_native_result ample_result = {0};
    arbor_status status = arbor_view0_native_check(
        span_from_cstr(combined), ample, 32u, &ample_result);
    if (status.native != 0 ||
        count_rule(ample, ample_result.diagnostic_count,
                   ARBOR_VIEW_V1_G03_DESCENDANT_EXCLUSIONS) != 4u ||
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

    /* textarea is forbidden by both active a and button ancestors, but R3 emits
     * one diagnostic for the single forbidden descendant element. */
    static const char overlap[] =
        "<!doctype html><title>x</title><a href=/><button><textarea>x</textarea></button></a>";
    arbor_view0_native_diagnostic overlap_d[16] = {{0}};
    arbor_view0_native_result overlap_r = {0};
    status = arbor_view0_native_check(span_from_cstr(overlap), overlap_d, 16u, &overlap_r);
    if (status.native != 0 ||
        count_rule(overlap_d, overlap_r.diagnostic_count,
                   ARBOR_VIEW_V1_G03_DESCENDANT_EXCLUSIONS) != 2u) return 4;

    /* R1 owns the h1/span placement; the same h1 is not duplicated by R3 even
     * though address also forbids heading descendants. */
    static const char r1_owned[] =
        "<!doctype html><title>x</title><address><span><h1>x</h1></span></address>";
    arbor_view0_native_diagnostic r1d[8] = {{0}};
    arbor_view0_native_result r1r = {0};
    status = arbor_view0_native_check(span_from_cstr(r1_owned), r1d, 8u, &r1r);
    if (status.native != 0 ||
        count_rule(r1d, r1r.diagnostic_count, ARBOR_VIEW_V1_G03_ELEMENT_CONTEXT) != 1u ||
        count_rule(r1d, r1r.diagnostic_count, ARBOR_VIEW_V1_G03_DESCENDANT_EXCLUSIONS) != 0u) return 5;

    /* A custom descendant with tabindex is still definitely forbidden by a. */
    static const char custom_tabindex[] =
        "<!doctype html><title>x</title><a href=/><x-widget tabindex=0></x-widget></a>";
    arbor_view0_native_diagnostic custom_d[8] = {{0}};
    arbor_view0_native_result custom_r = {0};
    status = arbor_view0_native_check(
        span_from_cstr(custom_tabindex), custom_d, 8u, &custom_r);
    if (status.native != 0 ||
        count_rule(custom_d, custom_r.diagnostic_count,
                   ARBOR_VIEW_V1_G03_DESCENDANT_EXCLUSIONS) != 1u) return 6;

    const uint64_t deferred =
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_DEFERRED_INPUT_TYPE |
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_DEFERRED_LABELED_CONTROL |
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_DEFERRED_CANVAS_INPUT_STATE |
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_DEFERRED_CANVAS_SELECT_SIZE |
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_DEFERRED_NOSCRIPT;
    static const char deferred_doc[] =
        "<!doctype html><title>x</title><body>"
        "<a href=/><input type=hidden></a>"
        "<label><input></label>"
        "<canvas><input type=checkbox><select size=2><option>x</option></select></canvas>"
        "<noscript><span>x</span></noscript></body>";
    arbor_view0_native_diagnostic deferred_d[32] = {{0}};
    arbor_view0_native_result deferred_r = {0};
    status = arbor_view0_native_check(
        span_from_cstr(deferred_doc), deferred_d, 32u, &deferred_r);
    if (status.native != 0 ||
        (deferred_r.flags & deferred) != deferred ||
        count_rule(deferred_d, deferred_r.diagnostic_count,
                   ARBOR_VIEW_V1_G03_DESCENDANT_EXCLUSIONS) != 0u) return 7;

    /* UTF-8 validation remains first and suppresses all later authoring rules. */
    static const uint8_t invalid_utf8[] = {
        '<','!','d','o','c','t','y','p','e',' ','h','t','m','l','>',
        '<','t','i','t','l','e','>','x','<','/','t','i','t','l','e','>',
        '<','a',' ','h','r','e','f','=','/','>','<','b','u','t','t','o','n','>',
        UINT8_C(0xf0), UINT8_C(0x80), UINT8_C(0x80), UINT8_C(0x80)};
    arbor_view0_native_diagnostic utf8d[8] = {{0}};
    arbor_view0_native_result utf8r = {0};
    status = arbor_view0_native_check(
        (arbor_span){invalid_utf8, (uint64_t)sizeof(invalid_utf8)}, utf8d, 8u, &utf8r);
    if (status.native != 0 || utf8r.diagnostic_count != 1u ||
        utf8d[0].origin != (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_UTF8 ||
        count_rule(utf8d, utf8r.diagnostic_count,
                   ARBOR_VIEW_V1_G03_DESCENDANT_EXCLUSIONS) != 0u) return 8;

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
        count_rule(deepd, deepr.diagnostic_count,
                   ARBOR_VIEW_V1_G03_DESCENDANT_EXCLUSIONS) != 0u ||
        (deepr.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_PARTIAL) == 0u) return 9;

    puts("PASS: VIEW0 V1N1 G03 R3A capacity atomicity, overlap bound, R1 suppression, deferred flags, deterministic ordering and UTF-8 precedence");
    return 0;
}
