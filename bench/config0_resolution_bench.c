#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include <arborcore/config.h>

#define SPAN_LITERAL(text) \
    { (const uint8_t *)(text), (uint64_t)(sizeof(text) - 1u) }

static const arbor_config_descriptor descriptor = {
    {SPAN_LITERAL("value"), SPAN_LITERAL("value"),
     SPAN_LITERAL("VALUE"), SPAN_LITERAL("value")},
    ARBOR_CONFIG_KIND_U64, 0u, ARBOR_CONFIG_DESCRIPTOR_HAS_DEFAULT,
    {7u, 0, {NULL, 0u}, 0, ARBOR_CONFIG_KIND_U64, false, {0u, 0u, 0u}},
    0u, UINT64_MAX, 0, 0, 0u, NULL, 0u
};

static uint64_t nanoseconds(struct timespec value)
{
    return (uint64_t)value.tv_sec * UINT64_C(1000000000) +
        (uint64_t)value.tv_nsec;
}

int main(void)
{
    const arbor_config_schema schema = {
        ARBOR_CONFIG_ABI_VERSION,
        sizeof(arbor_config_schema),
        0u,
        &descriptor,
        1u
    };
    static const arbor_span command_line[] = {SPAN_LITERAL("--value=99")};
    const arbor_config_sources sources = {
        {NULL, 0u}, NULL, 0u, command_line, 1u
    };
    arbor_config_requirements requirements = {0};
    struct timespec begin;
    struct timespec end;
    if (clock_gettime(CLOCK_MONOTONIC, &begin) != 0) {
        return 1;
    }
    const uint64_t repetitions = 10000u;
    for (uint64_t iteration = 0u; iteration < repetitions; ++iteration) {
        if (arbor_config_measure(
                &schema, &sources, &requirements, NULL).native != 0) {
            return 1;
        }
    }
    if (clock_gettime(CLOCK_MONOTONIC, &end) != 0) {
        return 1;
    }
    uint64_t elapsed = nanoseconds(end) - nanoseconds(begin);
    printf(
        "CONFIG0_RESOLUTION_DIAGNOSTIC\n"
        "descriptors,sources,repetitions,ns_per_measure\n"
        "1,command_line,%" PRIu64 ",%" PRIu64 "\n"
        "THRESHOLD_GATE=NONE_DIAGNOSTIC_ONLY\n"
        "PASS: CONFIG0 deterministic resolution diagnostic completed\n",
        repetitions, elapsed / repetitions);
    return 0;
}
