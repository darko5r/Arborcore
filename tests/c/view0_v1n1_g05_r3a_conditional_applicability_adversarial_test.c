#include <arborcore/view0_conformance/native.h>

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int diagnostic_equal(const arbor_view0_native_diagnostic *a,
                            const arbor_view0_native_diagnostic *b)
{
    return memcmp(a, b, sizeof(*a)) == 0;
}

static uint64_t rule_count(const arbor_view0_native_diagnostic *diagnostics,
                           uint64_t count,
                           uint64_t rule)
{
    uint64_t n = 0u;
    for (uint64_t i = 0u; i < count; ++i) {
        if (diagnostics[i].rule_id == rule) n += 1u;
    }
    return n;
}

int main(void)
{
    static const char mixed[] =
        "<!doctype html><title>x</title><body>"
        "<a target=\"_blank\">x</a>"
        "<input type=\"date\" accept=\"x\">"
        "<textarea wrap=\"hard\"></textarea>"
        "</body>";
    const arbor_span input = {(const uint8_t *)mixed, (uint64_t)(sizeof(mixed) - 1u)};
    arbor_view0_native_diagnostic d1[128] = {{0}}, d2[128] = {{0}};
    arbor_view0_native_result r1 = {0}, r2 = {0};
    const arbor_status s1 = arbor_view0_native_check(input, d1, UINT64_C(128), &r1);
    const arbor_status s2 = arbor_view0_native_check(input, d2, UINT64_C(128), &r2);
    if (s1.native != 0 || s2.native != 0 || memcmp(&r1, &r2, sizeof(r1)) != 0) return 10;
    for (uint64_t i = 0u; i < r1.diagnostic_count; ++i) {
        if (!diagnostic_equal(&d1[i], &d2[i])) return 11;
    }
    if (rule_count(d1, r1.diagnostic_count,
                   ARBOR_VIEW_V1_G05_CONDITIONAL_ATTRIBUTE_APPLICABILITY) != UINT64_C(3))
        return 12;

    /* N-1 capacity must fail before any caller-visible publication. */
    arbor_view0_native_diagnostic sentinel[128];
    arbor_view0_native_diagnostic before[128];
    arbor_view0_native_result result;
    arbor_view0_native_result before_result;
    (void)memset(sentinel, 0xa5, sizeof(sentinel));
    (void)memcpy(before, sentinel, sizeof(sentinel));
    (void)memset(&result, 0x5a, sizeof(result));
    before_result = result;
    const uint64_t insufficient = r1.diagnostic_count == 0u ? 0u : r1.diagnostic_count - 1u;
    const arbor_status cap = arbor_view0_native_check(input, sentinel, insufficient, &result);
    if (cap.native != -(int64_t)ENOSPC ||
        memcmp(sentinel, before, sizeof(sentinel)) != 0 ||
        memcmp(&result, &before_result, sizeof(result)) != 0)
        return 13;

    /* R1/R2 diagnostics remain their own owners and receive no R3 duplicate. */
    static const char earlier[] =
        "<!doctype html><title>x</title><body><p bogus=\"x\" href=\"/\">x</p></body>";
    arbor_view0_native_diagnostic owned[64] = {{0}};
    arbor_view0_native_result owned_result = {0};
    const arbor_span owned_input = {(const uint8_t *)earlier, (uint64_t)(sizeof(earlier) - 1u)};
    if (arbor_view0_native_check(owned_input, owned, UINT64_C(64), &owned_result).native != 0)
        return 14;
    if (rule_count(owned, owned_result.diagnostic_count,
                   ARBOR_VIEW_V1_G05_GLOBAL_ATTRIBUTE_APPLICABILITY) != UINT64_C(1) ||
        rule_count(owned, owned_result.diagnostic_count,
                   ARBOR_VIEW_V1_G05_ELEMENT_ATTRIBUTE_APPLICABILITY) != UINT64_C(1) ||
        rule_count(owned, owned_result.diagnostic_count,
                   ARBOR_VIEW_V1_G05_CONDITIONAL_ATTRIBUTE_APPLICABILITY) != UINT64_C(0))
        return 15;

    const uint8_t invalid_utf8[] = {UINT8_C(0xff)};
    arbor_view0_native_diagnostic utf8_diag[4] = {{0}};
    arbor_view0_native_result utf8_result = {0};
    const arbor_status utf8_status = arbor_view0_native_check(
        (arbor_span){invalid_utf8, UINT64_C(1)}, utf8_diag, UINT64_C(4), &utf8_result);
    if (utf8_status.native != 0 || utf8_result.diagnostic_count != UINT64_C(1) ||
        utf8_diag[0].rule_id != ARBOR_VIEW0_NATIVE_RULE_UTF8_INVALID)
        return 16;

    (void)puts("VIEW0_V1N1_G05_R3A_DETERMINISM=PASS");
    (void)puts("VIEW0_V1N1_G05_R3A_CAPACITY_FAILURE_ATOMICITY=PASS");
    (void)puts("VIEW0_V1N1_G05_R3A_R1_R2_PRIOR_OWNER_NON_DUPLICATION=PASS");
    (void)puts("VIEW0_V1N1_G05_R3A_UTF8_PRECEDENCE=PASS");
    return 0;
}
