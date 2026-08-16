#ifndef ARBORCORE_BROWSER_HARDENING_V2_H
#define ARBORCORE_BROWSER_HARDENING_V2_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ARBOR_BROWSER_HARDENING_V2_VERSION UINT32_C(0x00010001)
#define ARBOR_BROWSER_HARDENING_V2_TIMING_STAGE_COUNT UINT32_C(7)
#define ARBOR_BROWSER_HARDENING_V2_SAMPLE_CAPACITY UINT32_C(128)
#define ARBOR_BROWSER_HARDENING_V2_METRIC_COUNT UINT32_C(13)

typedef enum arbor_browser_hardening_v2_status {
    ARBOR_BROWSER_HARDENING_V2_OK = 0,
    ARBOR_BROWSER_HARDENING_V2_INVALID_ARGUMENT = 1,
    ARBOR_BROWSER_HARDENING_V2_OVERFLOW = 2,
    ARBOR_BROWSER_HARDENING_V2_INVALID_TRANSITION = 3
} arbor_browser_hardening_v2_status;

typedef enum arbor_browser_hardening_v2_lifecycle_state {
    ARBOR_BROWSER_HARDENING_V2_COLD = 0,
    ARBOR_BROWSER_HARDENING_V2_PROBING = 1,
    ARBOR_BROWSER_HARDENING_V2_WEBGPU_READY = 2,
    ARBOR_BROWSER_HARDENING_V2_FALLBACK_READY = 3,
    ARBOR_BROWSER_HARDENING_V2_DEVICE_LOST = 4,
    ARBOR_BROWSER_HARDENING_V2_RECOVERING = 5,
    ARBOR_BROWSER_HARDENING_V2_DESTROYED = 6
} arbor_browser_hardening_v2_lifecycle_state;

typedef enum arbor_browser_hardening_v2_lifecycle_event {
    ARBOR_BROWSER_HARDENING_V2_EVENT_PROBE_BEGIN = 0,
    ARBOR_BROWSER_HARDENING_V2_EVENT_WEBGPU_READY = 1,
    ARBOR_BROWSER_HARDENING_V2_EVENT_PLATFORM_FALLBACK = 2,
    ARBOR_BROWSER_HARDENING_V2_EVENT_DEVICE_LOST = 3,
    ARBOR_BROWSER_HARDENING_V2_EVENT_RECOVER_BEGIN = 4,
    ARBOR_BROWSER_HARDENING_V2_EVENT_RECOVER_SUCCESS = 5,
    ARBOR_BROWSER_HARDENING_V2_EVENT_RECOVER_FALLBACK = 6,
    ARBOR_BROWSER_HARDENING_V2_EVENT_DESTROY = 7,
    ARBOR_BROWSER_HARDENING_V2_EVENT_RUNTIME_FALLBACK = 8
} arbor_browser_hardening_v2_lifecycle_event;

typedef enum arbor_browser_hardening_v2_timing_stage {
    ARBOR_BROWSER_HARDENING_V2_TIMING_VALIDATE = 0,
    ARBOR_BROWSER_HARDENING_V2_TIMING_SOURCE_VIEW = 1,
    ARBOR_BROWSER_HARDENING_V2_TIMING_UPLOAD = 2,
    ARBOR_BROWSER_HARDENING_V2_TIMING_ENCODE_SUBMIT = 3,
    ARBOR_BROWSER_HARDENING_V2_TIMING_QUEUE_COMPLETION_LATENCY = 4,
    ARBOR_BROWSER_HARDENING_V2_TIMING_SYNCHRONIZED_PRESENT = 5,
    ARBOR_BROWSER_HARDENING_V2_TIMING_HOST_ENQUEUE = 6
} arbor_browser_hardening_v2_timing_stage;

typedef enum arbor_browser_hardening_v2_metric {
    ARBOR_BROWSER_HARDENING_V2_METRIC_PRESENTATIONS = 0,
    ARBOR_BROWSER_HARDENING_V2_METRIC_SOURCE_VIEW_CREATIONS = 1,
    ARBOR_BROWSER_HARDENING_V2_METRIC_SOURCE_VIEW_REUSES = 2,
    ARBOR_BROWSER_HARDENING_V2_METRIC_TEXTURE_CREATIONS = 3,
    ARBOR_BROWSER_HARDENING_V2_METRIC_TEXTURE_REUSES = 4,
    ARBOR_BROWSER_HARDENING_V2_METRIC_BIND_GROUP_CREATIONS = 5,
    ARBOR_BROWSER_HARDENING_V2_METRIC_BIND_GROUP_REUSES = 6,
    ARBOR_BROWSER_HARDENING_V2_METRIC_PIPELINE_CREATIONS = 7,
    ARBOR_BROWSER_HARDENING_V2_METRIC_PIPELINE_REUSES = 8,
    ARBOR_BROWSER_HARDENING_V2_METRIC_MEMORY_BUFFER_CHANGES = 9,
    ARBOR_BROWSER_HARDENING_V2_METRIC_DEVICE_LOSSES = 10,
    ARBOR_BROWSER_HARDENING_V2_METRIC_RECOVERIES = 11,
    ARBOR_BROWSER_HARDENING_V2_METRIC_FALLBACKS = 12
} arbor_browser_hardening_v2_metric;

uint32_t arbor_browser_hardening_v2_version(void);
void arbor_browser_hardening_v2_reset(void);

uint32_t arbor_browser_hardening_v2_lifecycle_state_value(void);
uint32_t arbor_browser_hardening_v2_lifecycle_generation(void);
arbor_browser_hardening_v2_status arbor_browser_hardening_v2_transition(uint32_t event);

arbor_browser_hardening_v2_status arbor_browser_hardening_v2_metric_increment(uint32_t metric);
uint64_t arbor_browser_hardening_v2_metric_value(uint32_t metric);

arbor_browser_hardening_v2_status arbor_browser_hardening_v2_timing_record(
    uint32_t stage,
    uint64_t nanoseconds);
uint32_t arbor_browser_hardening_v2_timing_count(uint32_t stage);
uint64_t arbor_browser_hardening_v2_timing_min(uint32_t stage);
uint64_t arbor_browser_hardening_v2_timing_max(uint32_t stage);
uint64_t arbor_browser_hardening_v2_timing_mean(uint32_t stage);
uint64_t arbor_browser_hardening_v2_timing_percentile_permille(
    uint32_t stage,
    uint32_t percentile_permille);

#ifdef __cplusplus
}
#endif

#endif
