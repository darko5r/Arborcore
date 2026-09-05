#include <errno.h>
#include <stdint.h>
#include <stdio.h>

#include "counter1.h"
#include <arborcore/profile.h>

#define PROFILE0_COUNTER1_SERVICE_GET UINT64_C(0x434f554e54474554)

typedef struct integration_clock {
    int64_t values[2];
    uint64_t index;
} integration_clock;

static int64_t integration_clock_read(void *context)
{
    integration_clock *clock = (integration_clock *)context;
    if (clock == NULL || clock->index >= 2u) {
        return -EIO;
    }
    int64_t value = clock->values[clock->index];
    clock->index += UINT64_C(1);
    return value;
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

    arbor_profile_region_descriptor descriptor = {
        PROFILE0_COUNTER1_SERVICE_GET
    };
    arbor_profile_aggregate aggregate = {0};
    integration_clock clock = {{1000, 1009}, 0u};
    arbor_profile_session profile = {0};
    arbor_profile_span span = {0};

    if (arbor_profile_session_prepare(
            &profile, &descriptor, &aggregate, 1u,
            integration_clock_read, &clock).native != 0 ||
        arbor_profile_span_begin(&profile, 0u, &span).native != 0) {
        (void)counter1_application_stop(&application);
        return 1;
    }

    const counter1_service_v1 *service =
        (const counter1_service_v1 *)application.service_binding.interface_table;
    counter1_service_request request = {&arena, 1u};
    counter1_service_result result = {0};
    int64_t native = service->get_counter(
        application.service_binding.provider_context,
        &request,
        &result);
    if (native != 0 ||
        result.outcome_code != COUNTER1_SERVICE_FOUND ||
        result.id != 1u ||
        result.value != 0u ||
        arbor_profile_span_end(&span).native != 0 ||
        arena_reset(&arena).status != 0) {
        (void)counter1_application_stop(&application);
        return 1;
    }

    arbor_profile_region_result profile_result = {0};
    if (arbor_profile_region_get(&profile, 0u, &profile_result).native != 0 ||
        profile_result.id != PROFILE0_COUNTER1_SERVICE_GET ||
        profile_result.aggregate.sample_count != 1u ||
        profile_result.aggregate.total_ns != 9u ||
        profile_result.aggregate.min_ns != 9u ||
        profile_result.aggregate.max_ns != 9u ||
        counter1_application_stop(&application).native != 0) {
        return 1;
    }

    puts("PASS: PROFILE0 wraps frozen COUNTER1 GET without changing typed result or arena semantics");
    return 0;
}
