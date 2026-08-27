#include <arborcore/view0_conformance/native.h>

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint64_t count_rule(
    const arbor_view0_native_diagnostic *diagnostics,
    const arbor_view0_native_result *result,
    uint64_t rule_id)
{
    uint64_t count = 0u;
    for (uint64_t i = 0u; i < result->diagnostic_count; ++i)
        if (diagnostics[i].rule_id == rule_id) count += 1u;
    return count;
}

static int run(
    const char *input,
    arbor_view0_native_diagnostic *diagnostics,
    uint64_t capacity,
    arbor_view0_native_result *result)
{
    return (int)arbor_view0_native_check(
        (arbor_span){(const uint8_t *)input, (uint64_t)strlen(input)},
        diagnostics, capacity, result).native;
}

static int expect_mechanism_capacity_unchanged(const char *input)
{
    arbor_view0_native_diagnostic diagnostics[4];
    arbor_view0_native_diagnostic before[4];
    arbor_view0_native_result result;
    arbor_view0_native_result result_before;
    (void)memset(diagnostics, 0xa5, sizeof(diagnostics));
    (void)memcpy(before, diagnostics, sizeof(before));
    (void)memset(&result, 0x5a, sizeof(result));
    result_before = result;
    const arbor_status status = arbor_view0_native_check(
        (arbor_span){(const uint8_t *)input, (uint64_t)strlen(input)},
        diagnostics, 4u, &result);
    if (status.native != -(int64_t)ENOSPC) return 1;
    if (memcmp(diagnostics, before, sizeof(before)) != 0 ||
        memcmp(&result, &result_before, sizeof(result)) != 0) return 2;
    return 0;
}

int main(void)
{
    static const char multi[] =
        "<!doctype html><title>x</title><body>"
        "<details open=false>x</details><div dir=sideways class='a a'>x</div>"
        "<input type=date value=2026-02-30></body>";
    arbor_view0_native_diagnostic first[128] = {{0}};
    arbor_view0_native_diagnostic second[128] = {{0}};
    arbor_view0_native_result first_result = {0};
    arbor_view0_native_result second_result = {0};
    if (run(multi, first, 128u, &first_result) != 0 ||
        run(multi, second, 128u, &second_result) != 0) return 10;
    if (memcmp(&first_result, &second_result, sizeof(first_result)) != 0 ||
        memcmp(first, second, sizeof(first)) != 0) return 11;
    if (count_rule(first, &first_result, UINT64_C(0x0000000030060001)) != 1u ||
        count_rule(first, &first_result, UINT64_C(0x0000000030060002)) != 1u ||
        count_rule(first, &first_result, UINT64_C(0x0000000030060007)) != 1u ||
        count_rule(first, &first_result, UINT64_C(0x0000000030060010)) != 1u)
        return 12;

    arbor_view0_native_diagnostic sentinel;
    arbor_view0_native_diagnostic sentinel_before;
    arbor_view0_native_result capacity_result;
    arbor_view0_native_result capacity_before;
    (void)memset(&sentinel, 0x3c, sizeof(sentinel));
    sentinel_before = sentinel;
    (void)memset(&capacity_result, 0xc3, sizeof(capacity_result));
    capacity_before = capacity_result;
    const arbor_status diagnostic_capacity = arbor_view0_native_check(
        (arbor_span){(const uint8_t *)multi, (uint64_t)(sizeof(multi) - 1u)},
        &sentinel, 0u, &capacity_result);
    if (diagnostic_capacity.native != -(int64_t)ENOSPC ||
        memcmp(&sentinel, &sentinel_before, sizeof(sentinel)) != 0 ||
        memcmp(&capacity_result, &capacity_before, sizeof(capacity_result)) != 0)
        return 13;

    static const char prior_owner[] =
        "<!doctype html><title>x</title><body><input type=text checked=false></body>";
    arbor_view0_native_diagnostic prior_diagnostics[32] = {{0}};
    arbor_view0_native_result prior_result = {0};
    if (run(prior_owner, prior_diagnostics, 32u, &prior_result) != 0) return 14;
    if (count_rule(prior_diagnostics, &prior_result,
                   ARBOR_VIEW_V1_G05_CONDITIONAL_ATTRIBUTE_APPLICABILITY) != 1u ||
        count_rule(prior_diagnostics, &prior_result,
                   UINT64_C(0x0000000030060001)) != 0u)
        return 15;

    const uint8_t invalid_utf8[] = {UINT8_C(0xff)};
    arbor_view0_native_diagnostic utf8_diagnostics[4] = {{0}};
    arbor_view0_native_result utf8_result = {0};
    const arbor_status utf8_status = arbor_view0_native_check(
        (arbor_span){invalid_utf8, 1u}, utf8_diagnostics, 4u, &utf8_result);
    if (utf8_status.native != 0 || utf8_result.diagnostic_count != 1u ||
        utf8_diagnostics[0].rule_id != ARBOR_VIEW0_NATIVE_RULE_UTF8_INVALID)
        return 16;

    static char accept_capacity[131072];
    size_t used = 0u;
    int written = snprintf(accept_capacity, sizeof(accept_capacity),
                           "<!doctype html><title>x</title><body>"
                           "<input type=file accept='");
    if (written < 0 || (size_t)written >= sizeof(accept_capacity)) return 17;
    used = (size_t)written;
    for (uint64_t i = 0u; i <= UINT64_C(4096); ++i) {
        written = snprintf(accept_capacity + used, sizeof(accept_capacity) - used,
                           "%s.x%llu", i == 0u ? "" : ",",
                           (unsigned long long)i);
        if (written < 0 || (size_t)written >= sizeof(accept_capacity) - used)
            return 18;
        used += (size_t)written;
    }
    written = snprintf(accept_capacity + used, sizeof(accept_capacity) - used,
                       "'></body>");
    if (written < 0 || (size_t)written >= sizeof(accept_capacity) - used) return 19;
    if (expect_mechanism_capacity_unchanged(accept_capacity) != 0) return 20;

    static char time_capacity[8192];
    written = snprintf(time_capacity, sizeof(time_capacity),
                       "<!doctype html><title>x</title><body><time>");
    if (written < 0 || (size_t)written >= sizeof(time_capacity)) return 21;
    used = (size_t)written;
    (void)memset(time_capacity + used, '2', 4097u);
    used += 4097u;
    written = snprintf(time_capacity + used, sizeof(time_capacity) - used,
                       "</time></body>");
    if (written < 0 || (size_t)written >= sizeof(time_capacity) - used) return 22;
    if (expect_mechanism_capacity_unchanged(time_capacity) != 0) return 23;

    static char prior_time[8192];
    written = snprintf(prior_time, sizeof(prior_time),
                       "<!doctype html><title>x</title><body><time>");
    if (written < 0 || (size_t)written >= sizeof(prior_time)) return 24;
    used = (size_t)written;
    (void)memset(prior_time + used, '2', 4097u);
    used += 4097u;
    written = snprintf(prior_time + used, sizeof(prior_time) - used,
                       "<span>x</span></time></body>");
    if (written < 0 || (size_t)written >= sizeof(prior_time) - used) return 25;
    arbor_view0_native_diagnostic time_diagnostics[32] = {{0}};
    arbor_view0_native_result time_result = {0};
    if (run(prior_time, time_diagnostics, 32u, &time_result) != 0) return 26;
    for (uint64_t rule = UINT64_C(0x0000000030060006);
         rule <= UINT64_C(0x000000003006000e); ++rule)
        if (count_rule(time_diagnostics, &time_result, rule) != 0u) return 27;

    (void)puts("VIEW0_V1N1_G06_WAVE_DETERMINISM=PASS");
    (void)puts("VIEW0_V1N1_G06_WAVE_DIAGNOSTIC_CAPACITY_FAILURE_ATOMICITY=PASS");
    (void)puts("VIEW0_V1N1_G06_WAVE_BOUNDED_WORKSPACE_FAILURE_ATOMICITY=PASS");
    (void)puts("VIEW0_V1N1_G06_WAVE_G05_PRIOR_OWNER_NON_DUPLICATION=PASS");
    (void)puts("VIEW0_V1N1_G06_WAVE_TIME_DESCENDANT_PRIOR_OWNER=PASS");
    (void)puts("VIEW0_V1N1_G06_WAVE_UTF8_PRECEDENCE=PASS");
    return 0;
}
