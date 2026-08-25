#include <arborcore/view0_conformance/native.h>

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static arbor_span span_from_cstr(const char *text)
{
    return (arbor_span){(const uint8_t *)text, (uint64_t)strlen(text)};
}

static int unchanged(const void *left, const void *right, size_t length)
{
    return memcmp(left, right, length) == 0 ? 0 : 1;
}

typedef struct fail_context {
    uint64_t calls;
    uint64_t fail_at;
} fail_context;

static arbor_status fail_element(
    void *opaque,
    const arbor_view0_native_element_observation *observation)
{
    fail_context *context = (fail_context *)opaque;
    if (context == NULL || observation == NULL) {
        return arbor_status_from_native(-(int64_t)EINVAL);
    }
    context->calls += 1u;
    if (context->calls == context->fail_at) {
        return arbor_status_from_native(-(int64_t)ECANCELED);
    }
    return arbor_status_from_native(0);
}

int main(void)
{
    static const char valid[] = "<!doctype html><title>x</title><p>x</p>";
    arbor_view0_native_parse_counts parse_counts;
    arbor_view0_native_document_facts facts;
    arbor_view0_native_observation_counts observations;
    (void)memset(&parse_counts, 0x11, sizeof(parse_counts));
    (void)memset(&facts, 0x22, sizeof(facts));
    (void)memset(&observations, 0x33, sizeof(observations));
    const arbor_view0_native_parse_counts parse_before = parse_counts;
    const arbor_view0_native_document_facts facts_before = facts;
    const arbor_view0_native_observation_counts observations_before = observations;

    arbor_status status = arbor_view0_native_lexbor_observe(
        span_from_cstr(valid),
        NULL,
        &parse_counts,
        &facts,
        &observations);
    if (status.native != -(int64_t)EINVAL ||
        unchanged(&parse_counts, &parse_before, sizeof(parse_counts)) != 0 ||
        unchanged(&facts, &facts_before, sizeof(facts)) != 0 ||
        unchanged(&observations, &observations_before, sizeof(observations)) != 0) {
        return 1;
    }

    const arbor_view0_native_semantic_observer empty_observer = {0};
    status = arbor_view0_native_lexbor_observe(
        span_from_cstr(valid),
        &empty_observer,
        &parse_counts,
        &facts,
        (arbor_view0_native_observation_counts *)&parse_counts);
    if (status.native != -(int64_t)EINVAL ||
        unchanged(&parse_counts, &parse_before, sizeof(parse_counts)) != 0 ||
        unchanged(&facts, &facts_before, sizeof(facts)) != 0) {
        return 2;
    }

    fail_context failure = {0u, 2u};
    const arbor_view0_native_semantic_observer failing_observer = {
        .context = &failure,
        .element_begin = fail_element,
        .attribute = NULL,
        .direct_child = NULL,
        .element_complete = NULL
    };
    status = arbor_view0_native_lexbor_observe(
        span_from_cstr(valid),
        &failing_observer,
        &parse_counts,
        &facts,
        &observations);
    if (status.native != -(int64_t)ECANCELED || failure.calls != 2u ||
        unchanged(&parse_counts, &parse_before, sizeof(parse_counts)) != 0 ||
        unchanged(&facts, &facts_before, sizeof(facts)) != 0 ||
        unchanged(&observations, &observations_before, sizeof(observations)) != 0) {
        return 3;
    }

    const uint64_t nesting = 4096u;
    const size_t capacity = (size_t)(nesting * 11u + 128u);
    char *deep = malloc(capacity);
    if (deep == NULL) {
        return 4;
    }
    size_t used = 0u;
    const char *prefix = "<!doctype html><title>x</title>";
    size_t prefix_len = strlen(prefix);
    (void)memcpy(deep + used, prefix, prefix_len);
    used += prefix_len;
    for (uint64_t i = 0u; i < nesting; ++i) {
        (void)memcpy(deep + used, "<div>", 5u);
        used += 5u;
    }
    for (uint64_t i = 0u; i < nesting; ++i) {
        (void)memcpy(deep + used, "</div>", 6u);
        used += 6u;
    }
    deep[used] = '\0';

    parse_counts = (arbor_view0_native_parse_counts){0};
    facts = (arbor_view0_native_document_facts){0};
    observations = (arbor_view0_native_observation_counts){0};
    status = arbor_view0_native_lexbor_observe(
        (arbor_span){(const uint8_t *)deep, (uint64_t)used},
        &empty_observer,
        &parse_counts,
        &facts,
        &observations);
    free(deep);
    if (status.native != 0 || observations.element_count < nesting ||
        observations.max_depth < nesting) {
        return 5;
    }

    uint8_t oversized_byte = 0u;
    parse_counts = parse_before;
    facts = facts_before;
    observations = observations_before;
    status = arbor_view0_native_lexbor_observe(
        (arbor_span){&oversized_byte, ARBOR_VIEW0_NATIVE_MAX_INPUT_BYTES + 1u},
        &empty_observer,
        &parse_counts,
        &facts,
        &observations);
    if (status.native != -(int64_t)E2BIG ||
        unchanged(&parse_counts, &parse_before, sizeof(parse_counts)) != 0 ||
        unchanged(&facts, &facts_before, sizeof(facts)) != 0 ||
        unchanged(&observations, &observations_before, sizeof(observations)) != 0) {
        return 6;
    }

    puts("PASS: VIEW0 V1N1 G03 C0 observation failure atomicity, alias rejection, explicit-depth traversal and input bounds");
    return 0;
}
