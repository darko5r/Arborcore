#include <arborcore/view0_conformance/native.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct fail_capture {
    uint64_t calls;
    uint64_t seen_source;
} fail_capture;

static arbor_span span_from_cstr(const char *text)
{
    return (arbor_span){(const uint8_t *)text, (uint64_t)strlen(text)};
}

static arbor_status fail_source_repair(
    void *context,
    const arbor_view0_native_source_repair_context *record)
{
    fail_capture *capture = (fail_capture *)context;
    if (capture == NULL || record == NULL) return arbor_status_from_native(-2);
    capture->calls += 1u;
    capture->seen_source = record->source_offset;
    return arbor_status_from_native(-77);
}

static arbor_status count_source_repair(
    void *context,
    const arbor_view0_native_source_repair_context *record)
{
    uint64_t *count = (uint64_t *)context;
    if (count == NULL || record == NULL) return arbor_status_from_native(-2);
    *count += 1u;
    return arbor_status_from_native(0);
}

int main(void)
{
    static const char html[] = "<!doctype html><title>x</title><body><p>x</p></body>";
    arbor_view0_native_parse_counts parse_counts = {
        UINT64_C(0x1111111111111111), UINT64_C(0x2222222222222222)};
    arbor_view0_native_document_facts facts;
    arbor_view0_native_observation_counts observations;
    (void)memset(&facts, 0xA5, sizeof(facts));
    (void)memset(&observations, 0x5A, sizeof(observations));
    const arbor_view0_native_parse_counts parse_before = parse_counts;
    const arbor_view0_native_document_facts facts_before = facts;
    const arbor_view0_native_observation_counts observations_before = observations;

    fail_capture capture = {0};
    const arbor_view0_native_semantic_observer failing = {
        .context = &capture,
        .source_repair = fail_source_repair
    };
    arbor_status status = arbor_view0_native_lexbor_observe(
        span_from_cstr(html), &failing, &parse_counts, &facts, &observations);
    if (status.native != -77 || capture.calls != 1u ||
        memcmp(&parse_counts, &parse_before, sizeof(parse_counts)) != 0 ||
        memcmp(&facts, &facts_before, sizeof(facts)) != 0 ||
        memcmp(&observations, &observations_before, sizeof(observations)) != 0) {
        return 1;
    }

    /* A successful call after injected failure proves parse-local state did not leak. */
    uint64_t success_count = 0u;
    const arbor_view0_native_semantic_observer success = {
        .context = &success_count,
        .source_repair = count_source_repair
    };
    parse_counts = (arbor_view0_native_parse_counts){0};
    facts = (arbor_view0_native_document_facts){0};
    observations = (arbor_view0_native_observation_counts){0};
    status = arbor_view0_native_lexbor_observe(
        span_from_cstr(html), &success, &parse_counts, &facts, &observations);
    if (status.native != 0 || success_count < 3u) return 2;

    puts("PASS: VIEW0 V1N1 G03 C0-SR1 observer failure atomicity and parse-local recovery");
    return 0;
}
