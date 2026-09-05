#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "counter1.h"
#include <arborcore/profile.h>

#define PROFILE0_COUNTER1_SERVICE_GET UINT64_C(0x434f554e54474554)
#define PROFILE0_BENCH_ITERATIONS UINT64_C(200000)
#define PROFILE0_BENCH_WARMUP UINT64_C(1000)

static uint64_t now_ns(void)
{
    struct timespec value;
    if (clock_gettime(CLOCK_MONOTONIC, &value) != 0) {
        return 0u;
    }
    return (uint64_t)value.tv_sec * UINT64_C(1000000000) +
        (uint64_t)value.tv_nsec;
}

static int64_t profile_clock(void *context)
{
    (void)context;
    uint64_t value = now_ns();
    if (value == 0u || value > (uint64_t)INT64_MAX) {
        return -ERANGE;
    }
    return (int64_t)value;
}

static int run_get(
    counter1_application *application,
    arbor_asm_arena *arena)
{
    const counter1_service_v1 *service =
        (const counter1_service_v1 *)application->service_binding.interface_table;
    counter1_service_request request = {arena, 1u};
    counter1_service_result result = {0};
    int64_t native = service->get_counter(
        application->service_binding.provider_context,
        &request,
        &result);
    if (native != 0 ||
        result.outcome_code != COUNTER1_SERVICE_FOUND ||
        result.id != 1u ||
        result.value != 0u) {
        return 1;
    }
    return arena_reset(arena).status == 0 ? 0 : 1;
}

static int run_profiled_get(
    counter1_application *application,
    arbor_asm_arena *arena,
    arbor_profile_session *profile,
    arbor_profile_span *span)
{
    if (arbor_profile_span_begin(profile, 0u, span).native != 0) {
        return 1;
    }
    if (run_get(application, arena) != 0) {
        return 1;
    }
    return arbor_profile_span_end(span).native == 0 ? 0 : 1;
}

int main(void)
{
    counter1_in_memory_repository repository = {0};
    counter1_repository_provider provider = {0};
    counter1_application application = {0};
    if (counter1_in_memory_repository_prepare(&repository, &provider).native != 0 ||
        counter1_application_prepare(&provider, &application).native != 0) {
        return 1;
    }

    uint8_t arena_bytes[4096] = {0};
    arbor_asm_arena arena = {0};
    if (arena_init(&arena, arena_bytes, sizeof(arena_bytes)).status != 0) {
        (void)counter1_application_stop(&application);
        return 1;
    }

    for (uint64_t index = 0u; index < PROFILE0_BENCH_WARMUP; ++index) {
        if (run_get(&application, &arena) != 0) {
            (void)counter1_application_stop(&application);
            return 1;
        }
    }
    uint64_t baseline_start = now_ns();
    if (baseline_start == 0u) {
        (void)counter1_application_stop(&application);
        return 1;
    }
    for (uint64_t index = 0u; index < PROFILE0_BENCH_ITERATIONS; ++index) {
        if (run_get(&application, &arena) != 0) {
            (void)counter1_application_stop(&application);
            return 1;
        }
    }
    uint64_t baseline_finish = now_ns();
    if (baseline_finish < baseline_start) {
        (void)counter1_application_stop(&application);
        return 1;
    }

    arbor_profile_region_descriptor descriptor = {
        PROFILE0_COUNTER1_SERVICE_GET
    };
    arbor_profile_aggregate aggregate = {0};
    arbor_profile_session profile = {0};
    arbor_profile_span span = {0};
    if (arbor_profile_session_prepare(
            &profile, &descriptor, &aggregate, 1u,
            profile_clock, NULL).native != 0) {
        (void)counter1_application_stop(&application);
        return 1;
    }

    for (uint64_t index = 0u; index < PROFILE0_BENCH_WARMUP; ++index) {
        if (run_profiled_get(&application, &arena, &profile, &span) != 0) {
            (void)counter1_application_stop(&application);
            return 1;
        }
    }
    if (arbor_profile_session_reset(&profile).native != 0) {
        (void)counter1_application_stop(&application);
        return 1;
    }

    uint64_t profiled_start = now_ns();
    if (profiled_start == 0u) {
        (void)counter1_application_stop(&application);
        return 1;
    }
    for (uint64_t index = 0u; index < PROFILE0_BENCH_ITERATIONS; ++index) {
        if (run_profiled_get(&application, &arena, &profile, &span) != 0) {
            (void)counter1_application_stop(&application);
            return 1;
        }
    }
    uint64_t profiled_finish = now_ns();
    if (profiled_finish < profiled_start ||
        counter1_application_stop(&application).native != 0) {
        return 1;
    }

    arbor_profile_region_result result = {0};
    if (arbor_profile_region_get(&profile, 0u, &result).native != 0 ||
        result.id != PROFILE0_COUNTER1_SERVICE_GET ||
        result.aggregate.sample_count != PROFILE0_BENCH_ITERATIONS) {
        return 1;
    }

    uint64_t baseline_total = baseline_finish - baseline_start;
    uint64_t profiled_total = profiled_finish - profiled_start;
    printf(
        "PROFILE0_COUNTER1_BASELINE iterations=%" PRIu64
        " total_ns=%" PRIu64 " ns_per_operation=%" PRIu64 "\n",
        PROFILE0_BENCH_ITERATIONS,
        baseline_total,
        baseline_total / PROFILE0_BENCH_ITERATIONS);
    printf(
        "PROFILE0_COUNTER1_PROFILED iterations=%" PRIu64
        " total_ns=%" PRIu64 " ns_per_operation=%" PRIu64 "\n",
        PROFILE0_BENCH_ITERATIONS,
        profiled_total,
        profiled_total / PROFILE0_BENCH_ITERATIONS);
    printf(
        "PROFILE0_COUNTER1_AGGREGATE samples=%" PRIu64
        " total_ns=%" PRIu64 " min_ns=%" PRIu64 " max_ns=%" PRIu64 "\n",
        result.aggregate.sample_count,
        result.aggregate.total_ns,
        result.aggregate.min_ns,
        result.aggregate.max_ns);
    puts("ADVISORY_ONLY=YES_NO_UNIVERSAL_THRESHOLD");
    return 0;
}
