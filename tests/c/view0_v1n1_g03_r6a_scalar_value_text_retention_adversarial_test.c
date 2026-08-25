#include <arborcore/view0_conformance/native.h>

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define R6_RULE_ID UINT64_C(0x0000000030030006)

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

int main(void)
{
    const uint64_t active_partial_flags =
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R1_PARTIAL |
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_PARTIAL |
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_PARTIAL |
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R4_PARTIAL |
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R5_PARTIAL;

    static const char two_owner_errors[] =
        "<!doctype html><html><head><title>x</title></head><body>"
        "<p>&#xFDD0;&#x0B;</p></body></html>";
    arbor_view0_native_diagnostic ample[8] = {{0}};
    arbor_view0_native_result ample_result = {0};
    arbor_status status = arbor_view0_native_check(
        span_from_cstr(two_owner_errors), ample, 8u, &ample_result);
    if (status.native != 0 || ample_result.diagnostic_count != 2u ||
        ample_result.tokenizer_error_count != 2u || ample_result.tree_error_count != 0u ||
        count_rule(ample, ample_result.diagnostic_count, R6_RULE_ID) != 0u) return 1;

    arbor_view0_native_diagnostic small[8];
    arbor_view0_native_diagnostic small_before[8];
    (void)memset(small, 0x5a, sizeof(small));
    (void)memcpy(small_before, small, sizeof(small));
    arbor_view0_native_result failed = {11u, 22u, 33u, 44u};
    const arbor_view0_native_result failed_before = failed;
    status = arbor_view0_native_check(
        span_from_cstr(two_owner_errors), small, 1u, &failed);
    if (status.native != -(int64_t)ENOSPC ||
        memcmp(small, small_before, sizeof(small)) != 0 ||
        memcmp(&failed, &failed_before, sizeof(failed)) != 0) return 2;

    arbor_view0_native_diagnostic exact[8] = {{0}};
    arbor_view0_native_result exact_result = {0};
    status = arbor_view0_native_check(
        span_from_cstr(two_owner_errors), exact, 2u, &exact_result);
    if (status.native != 0 ||
        memcmp(&ample_result, &exact_result, sizeof(ample_result)) != 0 ||
        memcmp(ample, exact, 2u * sizeof(exact[0])) != 0) return 3;

    arbor_view0_native_diagnostic repeat[8] = {{0}};
    arbor_view0_native_result repeat_result = {0};
    status = arbor_view0_native_check(
        span_from_cstr(two_owner_errors), repeat, 8u, &repeat_result);
    if (status.native != 0 ||
        memcmp(&ample_result, &repeat_result, sizeof(ample_result)) != 0 ||
        memcmp(ample, repeat, 2u * sizeof(repeat[0])) != 0) return 4;

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
        strcmp(utf8_diagnostics[0].symbolic_name, "html.utf8.invalid") != 0 ||
        count_rule(utf8_diagnostics, utf8_result.diagnostic_count, R6_RULE_ID) != 0u ||
        (utf8_result.flags & active_partial_flags) != active_partial_flags) return 5;

    arbor_view0_native_diagnostic empty_diagnostics[8] = {{0}};
    arbor_view0_native_result empty_result = {0};
    status = arbor_view0_native_check(
        (arbor_span){NULL, 0u}, empty_diagnostics, 8u, &empty_result);
    if (status.native != 0 ||
        count_rule(empty_diagnostics, empty_result.diagnostic_count, R6_RULE_ID) != 0u ||
        (empty_result.flags & active_partial_flags) != active_partial_flags) return 6;

    (void)puts("VIEW0_V1N1_G03_R6A_CAPACITY_FAILURE_ATOMICITY=PASS");
    (void)puts("VIEW0_V1N1_G03_R6A_DETERMINISM=PASS");
    (void)puts("VIEW0_V1N1_G03_R6A_UTF8_PRECEDENCE=PASS");
    (void)puts("VIEW0_V1N1_G03_R6A_RESULT_FLAG_GROWTH=NO");
    (void)puts("PASS: VIEW0 V1N1 G03 R6A retained-owner integration preserves capacity atomicity, determinism and result metadata");
    return 0;
}
