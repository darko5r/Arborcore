#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <arborcore/profile.h>

typedef struct scripted_clock {
    int64_t values[32];
    uint64_t count;
    uint64_t index;
} scripted_clock;

static int64_t scripted_clock_read(void *context)
{
    scripted_clock *clock = (scripted_clock *)context;
    if (clock == NULL || clock->index >= clock->count) {
        return -EIO;
    }
    int64_t value = clock->values[clock->index];
    clock->index += UINT64_C(1);
    return value;
}

static int status_is(arbor_status status, int64_t native)
{
    return status.native == native;
}

static int test_bytes_equal(const void *left, const void *right, size_t length)
{
    return memcmp(left, right, length) == 0;
}

static int prepare_one(
    arbor_profile_session *session,
    arbor_profile_region_descriptor *descriptor,
    arbor_profile_aggregate *aggregate,
    scripted_clock *clock)
{
    *descriptor = (arbor_profile_region_descriptor){UINT64_C(0x101)};
    *aggregate = (arbor_profile_aggregate){0};
    (void)memset(session, 0, sizeof(*session));
    return arbor_profile_session_prepare(
        session, descriptor, aggregate, 1u, scripted_clock_read, clock).native == 0 ? 0 : 1;
}

int main(void)
{
    scripted_clock null_context_clock = {{0}, 1u, 0u};
    null_context_clock.values[0] = 0;
    arbor_profile_region_descriptor descriptor = {UINT64_C(1)};
    arbor_profile_aggregate aggregate = {9u, 9u, 9u, 9u};
    arbor_profile_session session = {0};

    if (!status_is(arbor_profile_session_prepare(
            NULL, &descriptor, &aggregate, 1u, scripted_clock_read, NULL), -EINVAL) ||
        !status_is(arbor_profile_session_prepare(
            &session, NULL, &aggregate, 1u, scripted_clock_read, NULL), -EINVAL) ||
        !status_is(arbor_profile_session_prepare(
            &session, &descriptor, NULL, 1u, scripted_clock_read, NULL), -EINVAL) ||
        !status_is(arbor_profile_session_prepare(
            &session, &descriptor, &aggregate, 1u, NULL, NULL), -EINVAL)) {
        return 1;
    }

    aggregate = (arbor_profile_aggregate){9u, 9u, 9u, 9u};
    arbor_profile_session untouched_session;
    (void)memset(&untouched_session, 0xa5, sizeof(untouched_session));
    session = untouched_session;
    descriptor.id = 0u;
    arbor_profile_aggregate aggregate_before = aggregate;
    if (!status_is(arbor_profile_session_prepare(
            &session, &descriptor, &aggregate, 1u,
            scripted_clock_read, &null_context_clock), -EINVAL) ||
        !test_bytes_equal(&session, &untouched_session, sizeof(session)) ||
        !test_bytes_equal(&aggregate, &aggregate_before, sizeof(aggregate))) {
        return 1;
    }

    descriptor.id = UINT64_C(7);
    arbor_profile_region_descriptor duplicates[3] = {
        {7u}, {8u}, {7u}
    };
    arbor_profile_aggregate duplicate_aggregates[3] = {{0}};
    session = untouched_session;
    if (!status_is(arbor_profile_session_prepare(
            &session, duplicates, duplicate_aggregates, 3u,
            scripted_clock_read, NULL), -EINVAL) ||
        !test_bytes_equal(&session, &untouched_session, sizeof(session))) {
        return 1;
    }

    _Alignas(8) uint8_t shared[128] = {0};
    session = untouched_session;
    if (!status_is(arbor_profile_session_prepare(
            &session,
            (const arbor_profile_region_descriptor *)(const void *)shared,
            (arbor_profile_aggregate *)(void *)shared,
            1u,
            scripted_clock_read,
            NULL), -EINVAL)) {
        return 1;
    }

    _Alignas(8) uint8_t session_alias_storage[128] = {0};
    arbor_profile_aggregate session_alias_aggregate = {0};
    if (!status_is(arbor_profile_session_prepare(
            (arbor_profile_session *)(void *)session_alias_storage,
            (const arbor_profile_region_descriptor *)(const void *)session_alias_storage,
            &session_alias_aggregate,
            1u,
            scripted_clock_read,
            NULL), -EINVAL)) {
        return 1;
    }

    if (prepare_one(&session, &descriptor, &aggregate, &null_context_clock) != 0 ||
        session.clock_context != &null_context_clock) {
        return 1;
    }
    arbor_profile_session session_copy = session;
    session_copy.identity = &session_copy;
    if (!status_is(arbor_profile_session_validate(&session_copy), -EINVAL) ||
        !status_is(arbor_profile_session_validate(NULL), -EINVAL)) {
        return 1;
    }

    arbor_profile_region_result output = {UINT64_MAX, {1u, 2u, 3u, 4u}};
    arbor_profile_region_result output_before = output;
    uint64_t clock_before = null_context_clock.index;
    if (!status_is(arbor_profile_region_get(&session, 1u, &output), -EINVAL) ||
        !test_bytes_equal(&output, &output_before, sizeof(output)) ||
        null_context_clock.index != clock_before ||
        !status_is(arbor_profile_region_get(NULL, 0u, &output), -EINVAL) ||
        !status_is(arbor_profile_region_get(&session, 0u, NULL), -EINVAL)) {
        return 1;
    }
    if (!status_is(arbor_profile_region_get(
            &session,
            0u,
            (arbor_profile_region_result *)(void *)&aggregate), -EINVAL)) {
        return 1;
    }

    scripted_clock begin_failure = {{-EAGAIN}, 1u, 0u};
    if (prepare_one(&session, &descriptor, &aggregate, &begin_failure) != 0) {
        return 1;
    }
    arbor_profile_span span = {0};
    arbor_profile_span span_before = span;
    if (!status_is(arbor_profile_span_begin(&session, 0u, &span), -EAGAIN) ||
        !test_bytes_equal(&span, &span_before, sizeof(span)) ||
        begin_failure.index != 1u) {
        return 1;
    }

    scripted_clock good_clock = {{10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120}, 12u, 0u};
    if (prepare_one(&session, &descriptor, &aggregate, &good_clock) != 0) {
        return 1;
    }
    (void)memset(&span, 0, sizeof(span));
    span_before = span;
    if (!status_is(arbor_profile_span_begin(&session, 1u, &span), -EINVAL) ||
        !test_bytes_equal(&span, &span_before, sizeof(span)) ||
        good_clock.index != 0u) {
        return 1;
    }
    span.reserved0[0] = 1u;
    span_before = span;
    if (!status_is(arbor_profile_span_begin(&session, 0u, &span), -EINVAL) ||
        !test_bytes_equal(&span, &span_before, sizeof(span)) ||
        good_clock.index != 0u) {
        return 1;
    }
    (void)memset(&span, 0, sizeof(span));
    arbor_profile_region_descriptor alias_descriptors[3] = {
        {UINT64_C(0x201)}, {UINT64_C(0x202)}, {UINT64_C(0x203)}
    };
    arbor_profile_aggregate alias_aggregates[3] = {{0}};
    arbor_profile_session alias_session = {0};
    if (arbor_profile_session_prepare(
            &alias_session,
            alias_descriptors,
            alias_aggregates,
            3u,
            scripted_clock_read,
            &good_clock).native != 0 ||
        !status_is(arbor_profile_span_begin(
            &alias_session,
            0u,
            (arbor_profile_span *)(void *)alias_aggregates), -EINVAL) ||
        good_clock.index != 0u) {
        return 1;
    }

    if (!status_is(arbor_profile_span_end(NULL), -EINVAL) ||
        !status_is(arbor_profile_span_end(&span), -EINVAL) ||
        good_clock.index != 0u) {
        return 1;
    }

    if (arbor_profile_span_begin(&session, 0u, &span).native != 0 ||
        good_clock.index != 1u) {
        return 1;
    }
    arbor_profile_span active_before = span;
    span.reserved0[3] = 1u;
    if (!status_is(arbor_profile_span_end(&span), -EINVAL) ||
        good_clock.index != 1u) {
        return 1;
    }
    span = active_before;

    arbor_profile_span copied = span;
    if (!status_is(arbor_profile_span_end(&copied), -EINVAL) ||
        good_clock.index != 1u) {
        return 1;
    }
    if (arbor_profile_span_end(&span).native != 0 ||
        good_clock.index != 2u ||
        !status_is(arbor_profile_span_end(&span), -EINVAL) ||
        good_clock.index != 2u) {
        return 1;
    }

    (void)memset(&span, 0, sizeof(span));
    if (arbor_profile_span_begin(&session, 0u, &span).native != 0) {
        return 1;
    }
    arbor_profile_span begin_active_before = span;
    if (!status_is(arbor_profile_span_begin(&session, 0u, &span), -EINVAL) ||
        !test_bytes_equal(&span, &begin_active_before, sizeof(span))) {
        return 1;
    }
    if (arbor_profile_span_end(&span).native != 0) {
        return 1;
    }

    scripted_clock end_failure = {{100, -EIO, 105}, 3u, 0u};
    if (prepare_one(&session, &descriptor, &aggregate, &end_failure) != 0) {
        return 1;
    }
    (void)memset(&span, 0, sizeof(span));
    if (arbor_profile_span_begin(&session, 0u, &span).native != 0) {
        return 1;
    }
    active_before = span;
    aggregate_before = aggregate;
    if (!status_is(arbor_profile_span_end(&span), -EIO) ||
        !test_bytes_equal(&span, &active_before, sizeof(span)) ||
        !test_bytes_equal(&aggregate, &aggregate_before, sizeof(aggregate)) ||
        arbor_profile_span_end(&span).native != 0) {
        return 1;
    }

    scripted_clock regression = {{200, 199, 205}, 3u, 0u};
    if (prepare_one(&session, &descriptor, &aggregate, &regression) != 0) {
        return 1;
    }
    (void)memset(&span, 0, sizeof(span));
    if (arbor_profile_span_begin(&session, 0u, &span).native != 0) {
        return 1;
    }
    active_before = span;
    aggregate_before = aggregate;
    if (!status_is(arbor_profile_span_end(&span), -ERANGE) ||
        !test_bytes_equal(&span, &active_before, sizeof(span)) ||
        !test_bytes_equal(&aggregate, &aggregate_before, sizeof(aggregate)) ||
        arbor_profile_span_end(&span).native != 0) {
        return 1;
    }

    scripted_clock count_overflow_clock = {{300, 301}, 2u, 0u};
    if (prepare_one(
            &session, &descriptor, &aggregate, &count_overflow_clock) != 0) {
        return 1;
    }
    aggregate.sample_count = UINT64_MAX;
    aggregate.total_ns = 7u;
    aggregate.min_ns = 1u;
    aggregate.max_ns = 7u;
    (void)memset(&span, 0, sizeof(span));
    if (arbor_profile_span_begin(&session, 0u, &span).native != 0) {
        return 1;
    }
    active_before = span;
    aggregate_before = aggregate;
    if (!status_is(arbor_profile_span_end(&span), -EOVERFLOW) ||
        !test_bytes_equal(&span, &active_before, sizeof(span)) ||
        !test_bytes_equal(&aggregate, &aggregate_before, sizeof(aggregate))) {
        return 1;
    }

    scripted_clock total_overflow_clock = {{400, 402}, 2u, 0u};
    arbor_profile_session total_session = {0};
    arbor_profile_region_descriptor total_descriptor = {0};
    arbor_profile_aggregate total_aggregate = {0};
    arbor_profile_span total_span = {0};
    if (prepare_one(
            &total_session,
            &total_descriptor,
            &total_aggregate,
            &total_overflow_clock) != 0) {
        return 1;
    }
    total_aggregate.sample_count = 1u;
    total_aggregate.total_ns = UINT64_MAX;
    total_aggregate.min_ns = 1u;
    total_aggregate.max_ns = 1u;
    if (arbor_profile_span_begin(
            &total_session, 0u, &total_span).native != 0) {
        return 1;
    }
    arbor_profile_span total_span_before = total_span;
    arbor_profile_aggregate total_aggregate_before = total_aggregate;
    if (!status_is(arbor_profile_span_end(&total_span), -EOVERFLOW) ||
        !test_bytes_equal(
            &total_span, &total_span_before, sizeof(total_span)) ||
        !test_bytes_equal(
            &total_aggregate,
            &total_aggregate_before,
            sizeof(total_aggregate))) {
        return 1;
    }

    arbor_profile_session invalid_session = total_session;
    invalid_session.prepared_guard ^= UINT64_C(1);
    arbor_profile_aggregate reset_before = total_aggregate;
    if (!status_is(arbor_profile_session_reset(&invalid_session), -EINVAL) ||
        !test_bytes_equal(
            &total_aggregate, &reset_before, sizeof(total_aggregate))) {
        return 1;
    }

    puts("PASS: PROFILE0 adversarial null/alias/clock/token/failure-atomicity/overflow evidence");
    return 0;
}
