#include <arborcore/view0_conformance/native.h>

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define R1_RULE_ID UINT64_C(0x0000000030040001)

static arbor_span span_from_cstr(const char *s)
{
    return (arbor_span){(const uint8_t *)s, (uint64_t)strlen(s)};
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

int main(void)
{
    const uint64_t active_partial =
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R1_PARTIAL |
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_PARTIAL |
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_PARTIAL |
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R4_PARTIAL |
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R5_PARTIAL |
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R7_PARTIAL |
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G04_R1_PARTIAL;

    static const char negative[] =
        "<!doctype html><title>x</title><body><p><a href=\"/\"><div>x</div></a></p></body>";
    arbor_view0_native_diagnostic first[64] = {{0}};
    arbor_view0_native_result first_result = {0};
    arbor_status status = arbor_view0_native_check(
        span_from_cstr(negative), first, 64u, &first_result);
    if (status.native != 0 || count_rule(first, first_result.diagnostic_count, R1_RULE_ID) != 1u ||
        (first_result.flags & active_partial) != active_partial) return 1;

    arbor_view0_native_diagnostic repeat[64] = {{0}};
    arbor_view0_native_result repeat_result = {0};
    status = arbor_view0_native_check(
        span_from_cstr(negative), repeat, 64u, &repeat_result);
    if (status.native != 0 ||
        memcmp(&first_result, &repeat_result, sizeof(first_result)) != 0 ||
        memcmp(first, repeat,
               (size_t)first_result.diagnostic_count * sizeof(first[0])) != 0) return 2;

    if (first_result.diagnostic_count == 0u) return 3;
    arbor_view0_native_diagnostic failed_output[64];
    arbor_view0_native_diagnostic failed_before[64];
    (void)memset(failed_output, 0x5a, sizeof(failed_output));
    (void)memcpy(failed_before, failed_output, sizeof(failed_output));
    arbor_view0_native_result failed_result;
    arbor_view0_native_result failed_result_before;
    (void)memset(&failed_result, 0xa5, sizeof(failed_result));
    (void)memcpy(&failed_result_before, &failed_result, sizeof(failed_result));
    status = arbor_view0_native_check(
        span_from_cstr(negative), failed_output,
        first_result.diagnostic_count - 1u, &failed_result);
    if (status.native != -(int64_t)ENOSPC ||
        memcmp(failed_output, failed_before, sizeof(failed_output)) != 0 ||
        memcmp(&failed_result, &failed_result_before, sizeof(failed_result)) != 0) return 4;

    static const uint8_t invalid_utf8[] = {
        '<','!','d','o','c','t','y','p','e',' ','h','t','m','l','>',
        '<','t','i','t','l','e','>','x','<','/','t','i','t','l','e','>',
        '<','p','>',0xf0,0x80,0x80,0x80
    };
    arbor_view0_native_diagnostic utf8_diagnostics[8] = {{0}};
    arbor_view0_native_result utf8_result = {0};
    status = arbor_view0_native_check(
        (arbor_span){invalid_utf8, sizeof(invalid_utf8)},
        utf8_diagnostics, 8u, &utf8_result);
    if (status.native != 0 || utf8_result.diagnostic_count != 1u ||
        utf8_diagnostics[0].origin != (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_UTF8 ||
        count_rule(utf8_diagnostics, utf8_result.diagnostic_count, R1_RULE_ID) != 0u ||
        (utf8_result.flags & active_partial) != active_partial) return 5;

    arbor_view0_native_diagnostic empty_diagnostics[8] = {{0}};
    arbor_view0_native_result empty_result = {0};
    status = arbor_view0_native_check(
        (arbor_span){NULL, 0u}, empty_diagnostics, 8u, &empty_result);
    if (status.native != 0 ||
        count_rule(empty_diagnostics, empty_result.diagnostic_count, R1_RULE_ID) != 0u ||
        (empty_result.flags & active_partial) != active_partial) return 6;

    static const char option_case[] =
        "<!doctype html><title>x</title><body><select><option><div><span>x</span></div></option></select></body>";
    arbor_view0_native_diagnostic option_diagnostics[32] = {{0}};
    arbor_view0_native_result option_result = {0};
    status = arbor_view0_native_check(
        span_from_cstr(option_case), option_diagnostics, 32u, &option_result);
    if (status.native != 0 ||
        (option_result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G04_R1_DEFERRED_OPTION_BRANCH) != 0u)
        return 7;

    static const char noscript_case[] =
        "<!doctype html><title>x</title><body><noscript><span>x</span></noscript></body>";
    arbor_view0_native_diagnostic noscript_diagnostics[32] = {{0}};
    arbor_view0_native_result noscript_result = {0};
    status = arbor_view0_native_check(
        span_from_cstr(noscript_case), noscript_diagnostics, 32u, &noscript_result);
    if (status.native != 0 ||
        (noscript_result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G04_R1_DEFERRED_NOSCRIPT_SCRIPTING) != 0u)
        return 8;

    static const char custom_case[] =
        "<!doctype html><title>x</title><body><x-r1><span>x</span></x-r1></body>";
    arbor_view0_native_diagnostic custom_diagnostics[32] = {{0}};
    arbor_view0_native_result custom_result = {0};
    status = arbor_view0_native_check(
        span_from_cstr(custom_case), custom_diagnostics, 32u, &custom_result);
    if (status.native != 0 ||
        (custom_result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G04_R1_DEFERRED_G13_CUSTOM) == 0u)
        return 9;

    static const char prior_owned[] =
        "<!doctype html><title>x</title><body><select><div><span>x</span></div></select></body>";
    arbor_view0_native_diagnostic prior_diagnostics[32] = {{0}};
    arbor_view0_native_result prior_result = {0};
    status = arbor_view0_native_check(
        span_from_cstr(prior_owned), prior_diagnostics, 32u, &prior_result);
    if (status.native != 0 ||
        count_rule(prior_diagnostics, prior_result.diagnostic_count, R1_RULE_ID) != 0u)
        return 10;

    (void)puts("VIEW0_V1N1_G04_R1A_CAPACITY_FAILURE_ATOMICITY=PASS");
    (void)puts("VIEW0_V1N1_G04_R1A_DETERMINISM=PASS");
    (void)puts("VIEW0_V1N1_G04_R1A_UTF8_PRECEDENCE=PASS");
    (void)puts("VIEW0_V1N1_G04_R1A_PARTIAL_FLAGS_ALL_SUCCESS_PATHS=PASS");
    (void)puts("PASS: VIEW0 V1N1 G04 R1A adversarial core retained under R1C extension");
    return 0;
}
