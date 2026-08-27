#include <arborcore/view0_conformance/native.h>
#include "g04_r1a.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static arbor_span span_from_cstr(const char *s)
{
    return (arbor_span){(const uint8_t *)s, (uint64_t)strlen(s)};
}

static uint64_t count_rule(const arbor_view0_native_diagnostic *d, uint64_t n, uint64_t id)
{
    uint64_t count = 0u;
    for (uint64_t i = 0u; i < n; ++i) if (d[i].rule_id == id) count += 1u;
    return count;
}

int main(void)
{
    static const char html[] =
        "<!doctype html><title>x</title><body><noscript><div>x</div></noscript></body>";
    arbor_view0_native_diagnostic first[64] = {{0}}, repeat[64] = {{0}};
    arbor_view0_native_result first_result = {0}, repeat_result = {0};
    arbor_status status = arbor_view0_native_check(
        span_from_cstr(html), first, 64u, &first_result);
    if (status.native != 0 ||
        (first_result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G04_R1_DEFERRED_NOSCRIPT_SCRIPTING) != 0u ||
        (first_result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G04_R1_DEFERRED_G13_CUSTOM) != 0u)
        return 1;

    status = arbor_view0_native_check(span_from_cstr(html), repeat, 64u, &repeat_result);
    if (status.native != 0 || memcmp(&first_result, &repeat_result, sizeof(first_result)) != 0 ||
        memcmp(first, repeat, (size_t)first_result.diagnostic_count * sizeof(first[0])) != 0)
        return 2;

    arbor_view0_native_diagnostic failed[64], failed_before[64];
    arbor_view0_native_result failed_result, failed_result_before;
    (void)memset(failed, 0x3c, sizeof(failed));
    (void)memcpy(failed_before, failed, sizeof(failed));
    (void)memset(&failed_result, 0xc3, sizeof(failed_result));
    failed_result_before = failed_result;
    if (first_result.diagnostic_count == 0u) {
        /* Add a known G04 diagnostic so the capacity path remains exercised. */
        static const char negative[] =
            "<!doctype html><title>x</title><body><p><a href=\"/\"><div>x</div></a></p></body>";
        arbor_view0_native_diagnostic known[64] = {{0}};
        arbor_view0_native_result known_result = {0};
        status = arbor_view0_native_check(span_from_cstr(negative), known, 64u, &known_result);
        if (status.native != 0 ||
            count_rule(known, known_result.diagnostic_count, ARBOR_VIEW_V1_G04_TRANSPARENT_PARENT_MODEL) != 1u)
            return 3;
        status = arbor_view0_native_check(
            span_from_cstr(negative), failed, known_result.diagnostic_count - 1u, &failed_result);
    } else {
        status = arbor_view0_native_check(
            span_from_cstr(html), failed, first_result.diagnostic_count - 1u, &failed_result);
    }
    if (status.native != -(int64_t)ENOSPC ||
        memcmp(failed, failed_before, sizeof(failed)) != 0 ||
        memcmp(&failed_result, &failed_result_before, sizeof(failed_result)) != 0)
        return 4;

    static const uint8_t invalid_utf8[] = {'<','p','>',0xf0,0x80,0x80,0x80};
    arbor_view0_native_diagnostic utf8[8] = {{0}};
    arbor_view0_native_result utf8_result = {0};
    status = arbor_view0_native_check(
        (arbor_span){invalid_utf8, sizeof(invalid_utf8)}, utf8, 8u, &utf8_result);
    if (status.native != 0 || utf8_result.diagnostic_count != 1u ||
        utf8[0].origin != (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_UTF8)
        return 5;

    static const char custom[] =
        "<!doctype html><title>x</title><body><x-r1><span>x</span></x-r1></body>";
    arbor_view0_native_diagnostic custom_d[32] = {{0}};
    arbor_view0_native_result custom_r = {0};
    status = arbor_view0_native_check(span_from_cstr(custom), custom_d, 32u, &custom_r);
    if (status.native != 0 ||
        (custom_r.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G04_R1_DEFERRED_G13_CUSTOM) == 0u ||
        (custom_r.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G04_R1_DEFERRED_NOSCRIPT_SCRIPTING) != 0u)
        return 6;

    (void)puts("VIEW0_V1N1_G04_R1C_DETERMINISM=PASS");
    (void)puts("VIEW0_V1N1_G04_R1C_CAPACITY_FAILURE_ATOMICITY=PASS");
    (void)puts("VIEW0_V1N1_G04_R1C_UTF8_PRECEDENCE=PASS");
    (void)puts("VIEW0_V1N1_G04_R1C_NOSCRIPT_DEFERRAL_RETIRED=PASS");
    (void)puts("VIEW0_V1N1_G04_R1C_G13_EXTERNAL_DEPENDENCY_RETAINED=PASS");
    (void)puts("PASS: VIEW0 V1N1 G04 R1C adversarial integration");
    return 0;
}
