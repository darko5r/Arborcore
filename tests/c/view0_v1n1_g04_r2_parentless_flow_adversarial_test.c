#include <arborcore/view0_conformance/native.h>

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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
    static const char negative[] = "<a><html></html></a>";
    const arbor_span input = span_from_cstr(negative);
    arbor_view0_native_diagnostic first[64] = {{0}}, repeat[64] = {{0}};
    arbor_view0_native_result first_result = {0}, repeat_result = {0};
    arbor_status status = arbor_view0_native_check_fragment_model(input, first, 64u, &first_result);
    if (status.native != 0 ||
        count_rule(first, first_result.diagnostic_count,
                   ARBOR_VIEW_V1_G04_TRANSPARENT_PARENTLESS_FLOW) != 1u)
        return 1;
    status = arbor_view0_native_check_fragment_model(input, repeat, 64u, &repeat_result);
    if (status.native != 0 || memcmp(&first_result, &repeat_result, sizeof(first_result)) != 0 ||
        memcmp(first, repeat, (size_t)first_result.diagnostic_count * sizeof(first[0])) != 0)
        return 2;

    arbor_view0_native_diagnostic failed[64], failed_before[64];
    arbor_view0_native_result failed_result, failed_result_before;
    (void)memset(failed, 0x4d, sizeof(failed));
    (void)memcpy(failed_before, failed, sizeof(failed));
    (void)memset(&failed_result, 0xd4, sizeof(failed_result));
    failed_result_before = failed_result;
    if (first_result.diagnostic_count == 0u) return 3;
    status = arbor_view0_native_check_fragment_model(
        input, failed, first_result.diagnostic_count - 1u, &failed_result);
    if (status.native != -(int64_t)ENOSPC ||
        memcmp(failed, failed_before, sizeof(failed)) != 0 ||
        memcmp(&failed_result, &failed_result_before, sizeof(failed_result)) != 0)
        return 4;

    static const uint8_t invalid_utf8[] = {'<','a','>',0xc0u,0xafu,'<','/','a','>'};
    arbor_view0_native_diagnostic utf8[8] = {{0}};
    arbor_view0_native_result utf8_result = {0};
    status = arbor_view0_native_check_fragment_model(
        (arbor_span){invalid_utf8, (uint64_t)sizeof(invalid_utf8)}, utf8, 8u, &utf8_result);
    if (status.native != 0 || utf8_result.diagnostic_count != 1u ||
        utf8[0].rule_id != ARBOR_VIEW0_NATIVE_RULE_UTF8_INVALID ||
        utf8[0].origin != (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_UTF8)
        return 5;

    arbor_view0_native_diagnostic custom[16] = {{0}};
    arbor_view0_native_result custom_result = {0};
    status = arbor_view0_native_check_fragment_model(
        span_from_cstr("<a><x-r2></x-r2></a>"), custom, 16u, &custom_result);
    if (status.native != 0 ||
        (custom_result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G04_R2_DEFERRED_G13_CUSTOM) == 0u ||
        count_rule(custom, custom_result.diagnostic_count,
                   ARBOR_VIEW_V1_G04_TRANSPARENT_PARENTLESS_FLOW) != 0u)
        return 6;

    (void)puts("VIEW0_V1N1_G04_R2_DETERMINISM=PASS");
    (void)puts("VIEW0_V1N1_G04_R2_CAPACITY_FAILURE_ATOMICITY=PASS");
    (void)puts("VIEW0_V1N1_G04_R2_UTF8_PRECEDENCE=PASS");
    (void)puts("VIEW0_V1N1_G04_R2_G13_DEFER_NO_WARNING=PASS");
    (void)puts("PASS: VIEW0 V1N1 G04 R2 adversarial integration");
    return 0;
}
