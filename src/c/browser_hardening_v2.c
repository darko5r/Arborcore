#include "arborcore/browser_hardening_v2.h"

#include <limits.h>
#include <stdint.h>

#define SAMPLE_CAP ARBOR_BROWSER_HARDENING_V2_SAMPLE_CAPACITY
#define STAGE_COUNT ARBOR_BROWSER_HARDENING_V2_TIMING_STAGE_COUNT
#define METRIC_COUNT ARBOR_BROWSER_HARDENING_V2_METRIC_COUNT
#define MAX_SAFE_SAMPLE (UINT64_MAX / UINT64_C(128))

typedef struct arbor_browser_hardening_v2_stage_store {
    uint64_t samples[ARBOR_BROWSER_HARDENING_V2_SAMPLE_CAPACITY];
    uint64_t sum;
    uint64_t min;
    uint64_t max;
    uint32_t count;
    uint32_t next;
} arbor_browser_hardening_v2_stage_store;

static uint32_t lifecycle_state;
static uint32_t lifecycle_generation;
static uint64_t metrics[ARBOR_BROWSER_HARDENING_V2_METRIC_COUNT];
static arbor_browser_hardening_v2_stage_store stages[ARBOR_BROWSER_HARDENING_V2_TIMING_STAGE_COUNT];
static uint64_t percentile_scratch[ARBOR_BROWSER_HARDENING_V2_SAMPLE_CAPACITY];

static int valid_stage(uint32_t stage)
{
    return stage < STAGE_COUNT;
}

static int valid_metric(uint32_t metric)
{
    return metric < METRIC_COUNT;
}

static void clear_stage(arbor_browser_hardening_v2_stage_store *stage)
{
    uint32_t i;
    stage->sum = UINT64_C(0);
    stage->min = UINT64_MAX;
    stage->max = UINT64_C(0);
    stage->count = 0u;
    stage->next = 0u;
    for (i = 0u; i < SAMPLE_CAP; ++i) stage->samples[i] = UINT64_C(0);
}

static arbor_browser_hardening_v2_status transition_to(uint32_t next_state)
{
    lifecycle_state = next_state;
    lifecycle_generation += 1u;
    return ARBOR_BROWSER_HARDENING_V2_OK;
}

uint32_t arbor_browser_hardening_v2_version(void)
{
    return ARBOR_BROWSER_HARDENING_V2_VERSION;
}

void arbor_browser_hardening_v2_reset(void)
{
    uint32_t i;
    lifecycle_state = ARBOR_BROWSER_HARDENING_V2_COLD;
    lifecycle_generation = 0u;
    for (i = 0u; i < METRIC_COUNT; ++i) metrics[i] = UINT64_C(0);
    for (i = 0u; i < STAGE_COUNT; ++i) clear_stage(&stages[i]);
}

uint32_t arbor_browser_hardening_v2_lifecycle_state_value(void)
{
    return lifecycle_state;
}

uint32_t arbor_browser_hardening_v2_lifecycle_generation(void)
{
    return lifecycle_generation;
}

arbor_browser_hardening_v2_status arbor_browser_hardening_v2_transition(uint32_t event)
{
    switch (event) {
    case ARBOR_BROWSER_HARDENING_V2_EVENT_PROBE_BEGIN:
        if (lifecycle_state != ARBOR_BROWSER_HARDENING_V2_COLD &&
            lifecycle_state != ARBOR_BROWSER_HARDENING_V2_FALLBACK_READY) {
            return ARBOR_BROWSER_HARDENING_V2_INVALID_TRANSITION;
        }
        return transition_to(ARBOR_BROWSER_HARDENING_V2_PROBING);

    case ARBOR_BROWSER_HARDENING_V2_EVENT_WEBGPU_READY:
        if (lifecycle_state != ARBOR_BROWSER_HARDENING_V2_PROBING) {
            return ARBOR_BROWSER_HARDENING_V2_INVALID_TRANSITION;
        }
        return transition_to(ARBOR_BROWSER_HARDENING_V2_WEBGPU_READY);

    case ARBOR_BROWSER_HARDENING_V2_EVENT_PLATFORM_FALLBACK:
        if (lifecycle_state != ARBOR_BROWSER_HARDENING_V2_PROBING &&
            lifecycle_state != ARBOR_BROWSER_HARDENING_V2_RECOVERING) {
            return ARBOR_BROWSER_HARDENING_V2_INVALID_TRANSITION;
        }
        metrics[ARBOR_BROWSER_HARDENING_V2_METRIC_FALLBACKS] += UINT64_C(1);
        return transition_to(ARBOR_BROWSER_HARDENING_V2_FALLBACK_READY);

    case ARBOR_BROWSER_HARDENING_V2_EVENT_DEVICE_LOST:
        if (lifecycle_state != ARBOR_BROWSER_HARDENING_V2_WEBGPU_READY) {
            return ARBOR_BROWSER_HARDENING_V2_INVALID_TRANSITION;
        }
        metrics[ARBOR_BROWSER_HARDENING_V2_METRIC_DEVICE_LOSSES] += UINT64_C(1);
        return transition_to(ARBOR_BROWSER_HARDENING_V2_DEVICE_LOST);

    case ARBOR_BROWSER_HARDENING_V2_EVENT_RECOVER_BEGIN:
        if (lifecycle_state != ARBOR_BROWSER_HARDENING_V2_DEVICE_LOST) {
            return ARBOR_BROWSER_HARDENING_V2_INVALID_TRANSITION;
        }
        return transition_to(ARBOR_BROWSER_HARDENING_V2_RECOVERING);

    case ARBOR_BROWSER_HARDENING_V2_EVENT_RECOVER_SUCCESS:
        if (lifecycle_state != ARBOR_BROWSER_HARDENING_V2_RECOVERING) {
            return ARBOR_BROWSER_HARDENING_V2_INVALID_TRANSITION;
        }
        metrics[ARBOR_BROWSER_HARDENING_V2_METRIC_RECOVERIES] += UINT64_C(1);
        return transition_to(ARBOR_BROWSER_HARDENING_V2_WEBGPU_READY);

    case ARBOR_BROWSER_HARDENING_V2_EVENT_RECOVER_FALLBACK:
        if (lifecycle_state != ARBOR_BROWSER_HARDENING_V2_RECOVERING) {
            return ARBOR_BROWSER_HARDENING_V2_INVALID_TRANSITION;
        }
        metrics[ARBOR_BROWSER_HARDENING_V2_METRIC_FALLBACKS] += UINT64_C(1);
        return transition_to(ARBOR_BROWSER_HARDENING_V2_FALLBACK_READY);

    case ARBOR_BROWSER_HARDENING_V2_EVENT_DESTROY:
        if (lifecycle_state == ARBOR_BROWSER_HARDENING_V2_DESTROYED) {
            return ARBOR_BROWSER_HARDENING_V2_INVALID_TRANSITION;
        }
        return transition_to(ARBOR_BROWSER_HARDENING_V2_DESTROYED);

    case ARBOR_BROWSER_HARDENING_V2_EVENT_RUNTIME_FALLBACK:
        if (lifecycle_state != ARBOR_BROWSER_HARDENING_V2_WEBGPU_READY) {
            return ARBOR_BROWSER_HARDENING_V2_INVALID_TRANSITION;
        }
        metrics[ARBOR_BROWSER_HARDENING_V2_METRIC_FALLBACKS] += UINT64_C(1);
        return transition_to(ARBOR_BROWSER_HARDENING_V2_FALLBACK_READY);

    default:
        return ARBOR_BROWSER_HARDENING_V2_INVALID_ARGUMENT;
    }
}

arbor_browser_hardening_v2_status arbor_browser_hardening_v2_metric_increment(uint32_t metric)
{
    if (!valid_metric(metric)) return ARBOR_BROWSER_HARDENING_V2_INVALID_ARGUMENT;
    if (metrics[metric] == UINT64_MAX) return ARBOR_BROWSER_HARDENING_V2_OVERFLOW;
    metrics[metric] += UINT64_C(1);
    return ARBOR_BROWSER_HARDENING_V2_OK;
}

uint64_t arbor_browser_hardening_v2_metric_value(uint32_t metric)
{
    if (!valid_metric(metric)) return UINT64_C(0);
    return metrics[metric];
}

arbor_browser_hardening_v2_status arbor_browser_hardening_v2_timing_record(
    uint32_t stage,
    uint64_t nanoseconds)
{
    arbor_browser_hardening_v2_stage_store *store;
    uint64_t old;
    if (!valid_stage(stage)) return ARBOR_BROWSER_HARDENING_V2_INVALID_ARGUMENT;
    if (nanoseconds > MAX_SAFE_SAMPLE) return ARBOR_BROWSER_HARDENING_V2_OVERFLOW;

    store = &stages[stage];
    if (store->count < SAMPLE_CAP) {
        store->samples[store->next] = nanoseconds;
        store->count += 1u;
        store->sum += nanoseconds;
        store->next = (store->next + 1u) % SAMPLE_CAP;
        if (nanoseconds < store->min) store->min = nanoseconds;
        if (nanoseconds > store->max) store->max = nanoseconds;
        return ARBOR_BROWSER_HARDENING_V2_OK;
    }

    old = store->samples[store->next];
    store->samples[store->next] = nanoseconds;
    store->next = (store->next + 1u) % SAMPLE_CAP;
    store->sum -= old;
    store->sum += nanoseconds;

    if (old == store->min || old == store->max) {
        uint32_t i;
        store->min = UINT64_MAX;
        store->max = UINT64_C(0);
        for (i = 0u; i < SAMPLE_CAP; ++i) {
            uint64_t value = store->samples[i];
            if (value < store->min) store->min = value;
            if (value > store->max) store->max = value;
        }
    } else {
        if (nanoseconds < store->min) store->min = nanoseconds;
        if (nanoseconds > store->max) store->max = nanoseconds;
    }
    return ARBOR_BROWSER_HARDENING_V2_OK;
}

uint32_t arbor_browser_hardening_v2_timing_count(uint32_t stage)
{
    if (!valid_stage(stage)) return 0u;
    return stages[stage].count;
}

uint64_t arbor_browser_hardening_v2_timing_min(uint32_t stage)
{
    if (!valid_stage(stage) || stages[stage].count == 0u) return UINT64_C(0);
    return stages[stage].min;
}

uint64_t arbor_browser_hardening_v2_timing_max(uint32_t stage)
{
    if (!valid_stage(stage) || stages[stage].count == 0u) return UINT64_C(0);
    return stages[stage].max;
}

uint64_t arbor_browser_hardening_v2_timing_mean(uint32_t stage)
{
    if (!valid_stage(stage) || stages[stage].count == 0u) return UINT64_C(0);
    return stages[stage].sum / (uint64_t)stages[stage].count;
}

uint64_t arbor_browser_hardening_v2_timing_percentile_permille(
    uint32_t stage,
    uint32_t percentile_permille)
{
    uint32_t count;
    uint32_t i;
    uint32_t rank;
    if (!valid_stage(stage) || percentile_permille > 1000u) return UINT64_C(0);
    count = stages[stage].count;
    if (count == 0u) return UINT64_C(0);

    for (i = 0u; i < count; ++i) percentile_scratch[i] = stages[stage].samples[i];
    for (i = 1u; i < count; ++i) {
        uint64_t key = percentile_scratch[i];
        uint32_t j = i;
        while (j != 0u && percentile_scratch[j - 1u] > key) {
            percentile_scratch[j] = percentile_scratch[j - 1u];
            j -= 1u;
        }
        percentile_scratch[j] = key;
    }

    if (percentile_permille == 0u) return percentile_scratch[0];
    rank = (percentile_permille * count + 999u) / 1000u;
    if (rank == 0u) rank = 1u;
    if (rank > count) rank = count;
    return percentile_scratch[rank - 1u];
}
