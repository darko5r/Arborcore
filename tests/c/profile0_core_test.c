#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <arborcore/profile.h>

typedef struct fake_clock {
    int64_t values[16];
    uint64_t count;
    uint64_t index;
} fake_clock;

static int64_t fake_clock_read(void *context)
{
    fake_clock *clock = (fake_clock *)context;
    if (clock == NULL || clock->index >= clock->count) {
        return -EIO;
    }
    int64_t value = clock->values[clock->index];
    clock->index += UINT64_C(1);
    return value;
}

static int aggregate_equal(
    const arbor_profile_aggregate *aggregate,
    uint64_t count,
    uint64_t total,
    uint64_t minimum,
    uint64_t maximum)
{
    return aggregate->sample_count == count &&
        aggregate->total_ns == total &&
        aggregate->min_ns == minimum &&
        aggregate->max_ns == maximum;
}

int main(void)
{
    arbor_profile_region_descriptor descriptors[3] = {
        {UINT64_C(11)}, {UINT64_C(22)}, {UINT64_C(33)}
    };
    arbor_profile_aggregate aggregates[3] = {
        {9u, 9u, 9u, 9u}, {8u, 8u, 8u, 8u}, {7u, 7u, 7u, 7u}
    };
    fake_clock clock = {
        {100, 105, 200, 200, 300, 310, 320, 335, 400, 406},
        10u,
        0u
    };
    arbor_profile_session session = {0};

    if (arbor_profile_session_prepare(
            &session, descriptors, aggregates, 3u, fake_clock_read, &clock).native != 0 ||
        arbor_profile_session_validate(&session).native != 0 ||
        clock.index != 0u) {
        return 1;
    }
    for (uint64_t i = 0u; i < 3u; ++i) {
        if (!aggregate_equal(&aggregates[i], 0u, 0u, 0u, 0u)) {
            return 1;
        }
    }

    arbor_profile_region_result result = {UINT64_MAX, {1u, 2u, 3u, 4u}};
    if (arbor_profile_region_get(&session, 1u, &result).native != 0 ||
        result.id != 22u ||
        !aggregate_equal(&result.aggregate, 0u, 0u, 0u, 0u) ||
        clock.index != 0u) {
        return 1;
    }

    arbor_profile_span first = {0};
    if (arbor_profile_span_begin(&session, 0u, &first).native != 0 ||
        arbor_profile_span_end(&first).native != 0 ||
        !aggregate_equal(&aggregates[0], 1u, 5u, 5u, 5u) ||
        memcmp(&first, &(arbor_profile_span){0}, sizeof(first)) != 0) {
        return 1;
    }

    arbor_profile_span zero = {0};
    if (arbor_profile_span_begin(&session, 0u, &zero).native != 0 ||
        arbor_profile_span_end(&zero).native != 0 ||
        !aggregate_equal(&aggregates[0], 2u, 5u, 0u, 5u)) {
        return 1;
    }

    arbor_profile_span outer = {0};
    arbor_profile_span inner = {0};
    if (arbor_profile_span_begin(&session, 2u, &outer).native != 0 ||
        arbor_profile_span_begin(&session, 2u, &inner).native != 0 ||
        arbor_profile_span_end(&inner).native != 0 ||
        arbor_profile_span_end(&outer).native != 0 ||
        !aggregate_equal(&aggregates[2], 2u, 45u, 10u, 35u)) {
        return 1;
    }

    arbor_profile_span later = {0};
    if (arbor_profile_span_begin(&session, 0u, &later).native != 0 ||
        arbor_profile_span_end(&later).native != 0 ||
        !aggregate_equal(&aggregates[0], 3u, 11u, 0u, 6u)) {
        return 1;
    }

    arbor_profile_session session_before = session;
    if (arbor_profile_session_reset(&session).native != 0 ||
        memcmp(&session, &session_before, sizeof(session)) != 0 ||
        clock.index != 10u) {
        return 1;
    }
    for (uint64_t i = 0u; i < 3u; ++i) {
        if (!aggregate_equal(&aggregates[i], 0u, 0u, 0u, 0u)) {
            return 1;
        }
    }
    if (arbor_profile_session_validate(&session).native != 0 ||
        clock.index != 10u) {
        return 1;
    }

    puts("PASS: PROFILE0 core prepare/validate/get/reset/span/zero-duration/nesting aggregates");
    return 0;
}
