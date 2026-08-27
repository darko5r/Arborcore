#include <arborcore/view0_conformance/native.h>
#include "g04_r1a.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define R1_RULE_ID UINT64_C(0x0000000030040001)

static arbor_span span_from_cstr(const char *s)
{
    return (arbor_span){(const uint8_t *)s, (uint64_t)strlen(s)};
}

static uint64_t count_rule(const arbor_view0_native_diagnostic *d, uint64_t n, uint64_t id)
{
    uint64_t c = 0u;
    for (uint64_t i = 0u; i < n; ++i) if (d[i].rule_id == id) c += 1u;
    return c;
}

static arbor_status failing_source_text(
    void *context, const arbor_view0_native_source_text_observation *observation)
{
    (void)context; (void)observation;
    return arbor_status_from_native(-(int64_t)ENOMEM);
}


static arbor_status failing_source_attribute(
    void *context, const arbor_view0_native_source_attribute_observation *observation)
{
    (void)context; (void)observation;
    return arbor_status_from_native(-(int64_t)ENOMEM);
}

static int source_attribute_observer_failure_atomicity(void)
{
    static const char html[] =
        "<!doctype html><title>x</title><body><select><option label=x><div>x</div></option></select></body>";
    const arbor_view0_native_semantic_observer observer = {
        .context = NULL,
        .source_attribute = failing_source_attribute
    };
    arbor_view0_native_parse_counts counts, counts_before;
    arbor_view0_native_document_facts facts, facts_before;
    arbor_view0_native_observation_counts observations, observations_before;
    (void)memset(&counts, 0x44, sizeof(counts));
    (void)memset(&facts, 0x55, sizeof(facts));
    (void)memset(&observations, 0x66, sizeof(observations));
    counts_before = counts; facts_before = facts; observations_before = observations;
    arbor_status status = arbor_view0_native_lexbor_observe(
        span_from_cstr(html), &observer, &counts, &facts, &observations);
    if (status.native != -(int64_t)ENOMEM) return 1;
    if (memcmp(&counts, &counts_before, sizeof(counts)) != 0 ||
        memcmp(&facts, &facts_before, sizeof(facts)) != 0 ||
        memcmp(&observations, &observations_before, sizeof(observations)) != 0) return 2;
    return 0;
}

static int source_text_observer_failure_atomicity(void)
{
    static const char html[] =
        "<!doctype html><title>x</title><body><select><div>bad</div></select></body>";
    const arbor_view0_native_semantic_observer observer = {
        .context = NULL,
        .source_text = failing_source_text
    };
    arbor_view0_native_parse_counts counts, counts_before;
    arbor_view0_native_document_facts facts, facts_before;
    arbor_view0_native_observation_counts observations, observations_before;
    (void)memset(&counts, 0x11, sizeof(counts));
    (void)memset(&facts, 0x22, sizeof(facts));
    (void)memset(&observations, 0x33, sizeof(observations));
    counts_before = counts; facts_before = facts; observations_before = observations;
    arbor_status status = arbor_view0_native_lexbor_observe(
        span_from_cstr(html), &observer, &counts, &facts, &observations);
    if (status.native != -(int64_t)ENOMEM) return 1;
    if (memcmp(&counts, &counts_before, sizeof(counts)) != 0 ||
        memcmp(&facts, &facts_before, sizeof(facts)) != 0 ||
        memcmp(&observations, &observations_before, sizeof(observations)) != 0) return 2;
    return 0;
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

    static const char select_text[] =
        "<!doctype html><title>x</title><body><select><div>bad</div></select></body>";
    arbor_view0_native_diagnostic first[64] = {{0}}, repeat[64] = {{0}};
    arbor_view0_native_result first_result = {0}, repeat_result = {0};
    arbor_status status = arbor_view0_native_check(
        span_from_cstr(select_text), first, 64u, &first_result);
    if (status.native != 0 || count_rule(first, first_result.diagnostic_count, R1_RULE_ID) != 1u ||
        (first_result.flags & active_partial) != active_partial ||
        (first_result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G04_R1_DEFERRED_OPTION_BRANCH) != 0u)
        return 1;
    status = arbor_view0_native_check(span_from_cstr(select_text), repeat, 64u, &repeat_result);
    if (status.native != 0 || memcmp(&first_result, &repeat_result, sizeof(first_result)) != 0 ||
        memcmp(first, repeat, (size_t)first_result.diagnostic_count * sizeof(first[0])) != 0)
        return 2;

    arbor_view0_native_diagnostic failed[64], failed_before[64];
    arbor_view0_native_result failed_result, failed_result_before;
    (void)memset(failed, 0x5a, sizeof(failed));
    (void)memcpy(failed_before, failed, sizeof(failed));
    (void)memset(&failed_result, 0xa5, sizeof(failed_result));
    failed_result_before = failed_result;
    status = arbor_view0_native_check(
        span_from_cstr(select_text), failed, first_result.diagnostic_count - 1u, &failed_result);
    if (status.native != -(int64_t)ENOSPC ||
        memcmp(failed, failed_before, sizeof(failed)) != 0 ||
        memcmp(&failed_result, &failed_result_before, sizeof(failed_result)) != 0)
        return 3;

    static const uint8_t invalid_utf8[] = {'<','p','>',0xf0,0x80,0x80,0x80};
    arbor_view0_native_diagnostic utf8[8] = {{0}};
    arbor_view0_native_result utf8_result = {0};
    status = arbor_view0_native_check(
        (arbor_span){invalid_utf8, sizeof(invalid_utf8)}, utf8, 8u, &utf8_result);
    if (status.native != 0 || utf8_result.diagnostic_count != 1u ||
        utf8[0].origin != (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_UTF8 ||
        count_rule(utf8, utf8_result.diagnostic_count, R1_RULE_ID) != 0u)
        return 4;

    static const char option[] =
        "<!doctype html><title>x</title><body><select><option><div><span>x</span></div></option></select></body>";
    arbor_view0_native_diagnostic option_d[32] = {{0}};
    arbor_view0_native_result option_r = {0};
    status = arbor_view0_native_check(span_from_cstr(option), option_d, 32u, &option_r);
    if (status.native != 0 ||
        (option_r.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G04_R1_DEFERRED_OPTION_BRANCH) != 0u)
        return 5;

    static const char noscript[] =
        "<!doctype html><title>x</title><body><noscript><span>x</span></noscript></body>";
    arbor_view0_native_diagnostic noscript_d[32] = {{0}};
    arbor_view0_native_result noscript_r = {0};
    status = arbor_view0_native_check(span_from_cstr(noscript), noscript_d, 32u, &noscript_r);
    if (status.native != 0 ||
        (noscript_r.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G04_R1_DEFERRED_NOSCRIPT_SCRIPTING) != 0u)
        return 6;

    static const char custom[] =
        "<!doctype html><title>x</title><body><x-r1><span>x</span></x-r1></body>";
    arbor_view0_native_diagnostic custom_d[32] = {{0}};
    arbor_view0_native_result custom_r = {0};
    status = arbor_view0_native_check(span_from_cstr(custom), custom_d, 32u, &custom_r);
    if (status.native != 0 ||
        (custom_r.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G04_R1_DEFERRED_G13_CUSTOM) == 0u)
        return 7;

    int rc = source_attribute_observer_failure_atomicity();
    if (rc != 0) return 20 + rc;
    rc = source_text_observer_failure_atomicity();
    if (rc != 0) return 30 + rc;

    (void)puts("VIEW0_V1N1_G04_R1B_CAPACITY_FAILURE_ATOMICITY=PASS");
    (void)puts("VIEW0_V1N1_G04_R1B_DETERMINISM=PASS");
    (void)puts("VIEW0_V1N1_G04_R1B_UTF8_PRECEDENCE=PASS");
    (void)puts("VIEW0_V1N1_G04_R1B_OPTION_DEFERRAL_RETIRED=PASS");
    (void)puts("VIEW0_V1N1_G04_R1B_SOURCE_ATTRIBUTE_OBSERVER_FAILURE_ATOMICITY=PASS");
    (void)puts("VIEW0_V1N1_G04_R1B_SOURCE_TEXT_OBSERVER_FAILURE_ATOMICITY=PASS");
    (void)puts("VIEW0_V1N1_G04_R1B_RETAINED_UNDER_R1C=PASS_G13_ONLY");
    (void)puts("PASS: VIEW0 V1N1 G04 R1B adversarial integration");
    return 0;
}
