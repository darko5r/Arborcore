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
    uint64_t enter_calls;
    uint64_t leave_calls;
    uint64_t fail_enter_at;
    uint64_t fail_leave_at;
} fail_context;

static arbor_status fail_enter(
    void *opaque,
    const arbor_view0_native_element_observation *observation)
{
    fail_context *context = (fail_context *)opaque;
    if (context == NULL || observation == NULL) {
        return arbor_status_from_native(-(int64_t)EINVAL);
    }
    context->enter_calls += 1u;
    if (context->enter_calls == context->fail_enter_at) {
        return arbor_status_from_native(-(int64_t)ECANCELED);
    }
    return arbor_status_from_native(0);
}

static arbor_status fail_leave(
    void *opaque,
    const arbor_view0_native_element_observation *observation)
{
    fail_context *context = (fail_context *)opaque;
    if (context == NULL || observation == NULL) {
        return arbor_status_from_native(-(int64_t)EINVAL);
    }
    context->leave_calls += 1u;
    if (context->leave_calls == context->fail_leave_at) {
        return arbor_status_from_native(-(int64_t)ECANCELED);
    }
    return arbor_status_from_native(0);
}

typedef struct depth_context {
    uint64_t active;
    uint64_t enter_count;
    uint64_t leave_count;
    uint64_t max_depth;
} depth_context;

static arbor_status depth_enter(
    void *opaque,
    const arbor_view0_native_element_observation *observation)
{
    depth_context *context = (depth_context *)opaque;
    if (context == NULL || observation == NULL || observation->depth != context->active) {
        return arbor_status_from_native(-(int64_t)EINVAL);
    }
    context->active += 1u;
    context->enter_count += 1u;
    if (observation->depth > context->max_depth) {
        context->max_depth = observation->depth;
    }
    return arbor_status_from_native(0);
}

static arbor_status depth_leave(
    void *opaque,
    const arbor_view0_native_element_observation *observation)
{
    depth_context *context = (depth_context *)opaque;
    if (context == NULL || observation == NULL || context->active == 0u ||
        observation->depth + 1u != context->active) {
        return arbor_status_from_native(-(int64_t)EINVAL);
    }
    context->active -= 1u;
    context->leave_count += 1u;
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

    fail_context enter_failure = {0u, 0u, 2u, UINT64_MAX};
    const arbor_view0_native_semantic_observer enter_failing_observer = {
        .context = &enter_failure,
        .traversal_enter = fail_enter,
        .element_begin = NULL,
        .attribute = NULL,
        .direct_child = NULL,
        .element_complete = NULL,
        .traversal_leave = fail_leave
    };
    arbor_status status = arbor_view0_native_lexbor_observe(
        span_from_cstr(valid),
        &enter_failing_observer,
        &parse_counts,
        &facts,
        &observations);
    if (status.native != -(int64_t)ECANCELED || enter_failure.enter_calls != 2u ||
        unchanged(&parse_counts, &parse_before, sizeof(parse_counts)) != 0 ||
        unchanged(&facts, &facts_before, sizeof(facts)) != 0 ||
        unchanged(&observations, &observations_before, sizeof(observations)) != 0) {
        return 1;
    }

    fail_context leave_failure = {0u, 0u, UINT64_MAX, 1u};
    const arbor_view0_native_semantic_observer leave_failing_observer = {
        .context = &leave_failure,
        .traversal_enter = fail_enter,
        .element_begin = NULL,
        .attribute = NULL,
        .direct_child = NULL,
        .element_complete = NULL,
        .traversal_leave = fail_leave
    };
    status = arbor_view0_native_lexbor_observe(
        span_from_cstr(valid),
        &leave_failing_observer,
        &parse_counts,
        &facts,
        &observations);
    if (status.native != -(int64_t)ECANCELED || leave_failure.leave_calls != 1u ||
        unchanged(&parse_counts, &parse_before, sizeof(parse_counts)) != 0 ||
        unchanged(&facts, &facts_before, sizeof(facts)) != 0 ||
        unchanged(&observations, &observations_before, sizeof(observations)) != 0) {
        return 2;
    }

    const uint64_t nesting = 4096u;
    const size_t capacity = (size_t)(nesting * 11u + 128u);
    char *deep = malloc(capacity);
    if (deep == NULL) {
        return 3;
    }
    size_t used = 0u;
    static const char prefix[] = "<!doctype html><title>x</title>";
    const size_t prefix_len = sizeof(prefix) - 1u;
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

    depth_context depth = {0};
    const arbor_view0_native_semantic_observer depth_observer = {
        .context = &depth,
        .traversal_enter = depth_enter,
        .element_begin = NULL,
        .attribute = NULL,
        .direct_child = NULL,
        .element_complete = NULL,
        .traversal_leave = depth_leave
    };
    parse_counts = (arbor_view0_native_parse_counts){0};
    facts = (arbor_view0_native_document_facts){0};
    observations = (arbor_view0_native_observation_counts){0};
    status = arbor_view0_native_lexbor_observe(
        (arbor_span){(const uint8_t *)deep, (uint64_t)used},
        &depth_observer,
        &parse_counts,
        &facts,
        &observations);
    free(deep);
    if (status.native != 0 || depth.active != 0u ||
        depth.enter_count != observations.element_count ||
        depth.leave_count != observations.element_count ||
        depth.max_depth != 4097u || observations.max_depth != 4097u) {
        return 4;
    }

    puts("PASS: VIEW0 V1N1 G03 C0 lifecycle callback failure atomicity, unbalanced-failure allowance and exact depth-4097 stress evidence");
    return 0;
}
