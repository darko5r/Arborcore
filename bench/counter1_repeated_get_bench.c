#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "counter1.h"

#define COUNTER1_BENCH_ITERATIONS UINT64_C(200000)
#define COUNTER1_BENCH_WARMUP UINT64_C(1000)

static uint64_t now_ns(void)
{
    struct timespec time_value;
    if (clock_gettime(CLOCK_MONOTONIC, &time_value) != 0) {
        return 0u;
    }
    return (uint64_t)time_value.tv_sec * UINT64_C(1000000000) +
        (uint64_t)time_value.tv_nsec;
}

static int run_get(
    counter1_application *application,
    arbor_asm_arena *arena,
    uint64_t expected_value)
{
    const counter1_service_v1 *service =
        (const counter1_service_v1 *)application->service_binding.interface_table;
    counter1_service_request request = {arena, 1u};
    counter1_service_result result = {0};
    int64_t native = service->get_counter(
        application->service_binding.provider_context,
        &request,
        &result);
    if (native != 0 || result.outcome_code != COUNTER1_SERVICE_FOUND ||
        result.id != 1u || result.value != expected_value) {
        return 1;
    }
    return arena_reset(arena).status == 0 ? 0 : 1;
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

    for (uint64_t index = 0u; index < COUNTER1_BENCH_WARMUP; ++index) {
        if (run_get(&application, &arena, 0u) != 0) {
            (void)counter1_application_stop(&application);
            return 1;
        }
    }

    const uint64_t start = now_ns();
    if (start == 0u) {
        (void)counter1_application_stop(&application);
        return 1;
    }
    for (uint64_t index = 0u; index < COUNTER1_BENCH_ITERATIONS; ++index) {
        if (run_get(&application, &arena, 0u) != 0) {
            (void)counter1_application_stop(&application);
            return 1;
        }
    }
    const uint64_t finish = now_ns();
    if (finish < start || counter1_application_stop(&application).native != 0) {
        return 1;
    }

    const uint64_t elapsed = finish - start;
    printf(
        "COUNTER1_REPEATED_GET iterations=%" PRIu64
        " total_ns=%" PRIu64 " ns_per_operation=%" PRIu64 "\n",
        COUNTER1_BENCH_ITERATIONS,
        elapsed,
        elapsed / COUNTER1_BENCH_ITERATIONS);
    puts("ADVISORY_ONLY=YES_NO_UNIVERSAL_THRESHOLD");
    return 0;
}
