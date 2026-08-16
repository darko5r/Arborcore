#include "arborcore/browser_hardening_v2.h"

#include <stdint.h>
#include <stdio.h>

static int fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    return 1;
}

int main(void)
{
    uint32_t i;
    arbor_browser_hardening_v2_reset();

    if (arbor_browser_hardening_v2_version() != ARBOR_BROWSER_HARDENING_V2_VERSION) {
        return fail("version mismatch");
    }
    if (arbor_browser_hardening_v2_lifecycle_state_value() != ARBOR_BROWSER_HARDENING_V2_COLD) {
        return fail("reset state is not COLD");
    }
    if (arbor_browser_hardening_v2_transition(ARBOR_BROWSER_HARDENING_V2_EVENT_WEBGPU_READY) !=
        ARBOR_BROWSER_HARDENING_V2_INVALID_TRANSITION) {
        return fail("invalid cold->ready transition accepted");
    }
    if (arbor_browser_hardening_v2_transition(ARBOR_BROWSER_HARDENING_V2_EVENT_PROBE_BEGIN) != 0 ||
        arbor_browser_hardening_v2_transition(ARBOR_BROWSER_HARDENING_V2_EVENT_WEBGPU_READY) != 0) {
        return fail("probe->webgpu transition failed");
    }
    if (arbor_browser_hardening_v2_lifecycle_state_value() != ARBOR_BROWSER_HARDENING_V2_WEBGPU_READY) {
        return fail("WEBGPU_READY state missing");
    }
    if (arbor_browser_hardening_v2_transition(ARBOR_BROWSER_HARDENING_V2_EVENT_RUNTIME_FALLBACK) != 0 ||
        arbor_browser_hardening_v2_lifecycle_state_value() != ARBOR_BROWSER_HARDENING_V2_FALLBACK_READY ||
        arbor_browser_hardening_v2_metric_value(ARBOR_BROWSER_HARDENING_V2_METRIC_FALLBACKS) != 1u) {
        return fail("runtime fallback transition failed");
    }
    arbor_browser_hardening_v2_reset();
    if (arbor_browser_hardening_v2_transition(ARBOR_BROWSER_HARDENING_V2_EVENT_PROBE_BEGIN) != 0 ||
        arbor_browser_hardening_v2_transition(ARBOR_BROWSER_HARDENING_V2_EVENT_WEBGPU_READY) != 0) {
        return fail("second probe->webgpu transition failed");
    }
    if (arbor_browser_hardening_v2_transition(ARBOR_BROWSER_HARDENING_V2_EVENT_DEVICE_LOST) != 0 ||
        arbor_browser_hardening_v2_transition(ARBOR_BROWSER_HARDENING_V2_EVENT_RECOVER_BEGIN) != 0 ||
        arbor_browser_hardening_v2_transition(ARBOR_BROWSER_HARDENING_V2_EVENT_RECOVER_SUCCESS) != 0) {
        return fail("device-loss recovery transition failed");
    }
    if (arbor_browser_hardening_v2_metric_value(ARBOR_BROWSER_HARDENING_V2_METRIC_DEVICE_LOSSES) != 1u ||
        arbor_browser_hardening_v2_metric_value(ARBOR_BROWSER_HARDENING_V2_METRIC_RECOVERIES) != 1u) {
        return fail("lifecycle counters mismatch");
    }

    for (i = 1u; i <= 100u; ++i) {
        if (arbor_browser_hardening_v2_timing_record(
                ARBOR_BROWSER_HARDENING_V2_TIMING_SYNCHRONIZED_PRESENT,
                (uint64_t)i * UINT64_C(1000)) != 0) {
            return fail("timing record failed");
        }
    }
    if (arbor_browser_hardening_v2_timing_count(ARBOR_BROWSER_HARDENING_V2_TIMING_SYNCHRONIZED_PRESENT) != 100u) {
        return fail("timing count mismatch");
    }
    if (arbor_browser_hardening_v2_timing_min(ARBOR_BROWSER_HARDENING_V2_TIMING_SYNCHRONIZED_PRESENT) != UINT64_C(1000) ||
        arbor_browser_hardening_v2_timing_max(ARBOR_BROWSER_HARDENING_V2_TIMING_SYNCHRONIZED_PRESENT) != UINT64_C(100000) ||
        arbor_browser_hardening_v2_timing_mean(ARBOR_BROWSER_HARDENING_V2_TIMING_SYNCHRONIZED_PRESENT) != UINT64_C(50500)) {
        return fail("timing aggregate mismatch");
    }
    if (arbor_browser_hardening_v2_timing_percentile_permille(
            ARBOR_BROWSER_HARDENING_V2_TIMING_SYNCHRONIZED_PRESENT, 500u) != UINT64_C(50000) ||
        arbor_browser_hardening_v2_timing_percentile_permille(
            ARBOR_BROWSER_HARDENING_V2_TIMING_SYNCHRONIZED_PRESENT, 950u) != UINT64_C(95000) ||
        arbor_browser_hardening_v2_timing_percentile_permille(
            ARBOR_BROWSER_HARDENING_V2_TIMING_SYNCHRONIZED_PRESENT, 990u) != UINT64_C(99000)) {
        return fail("timing percentile mismatch");
    }

    for (i = 0u; i < 160u; ++i) {
        if (arbor_browser_hardening_v2_timing_record(
                ARBOR_BROWSER_HARDENING_V2_TIMING_UPLOAD,
                (uint64_t)i) != 0) {
            return fail("ring timing record failed");
        }
    }
    if (arbor_browser_hardening_v2_timing_count(ARBOR_BROWSER_HARDENING_V2_TIMING_UPLOAD) !=
        ARBOR_BROWSER_HARDENING_V2_SAMPLE_CAPACITY) {
        return fail("ring capacity mismatch");
    }
    if (arbor_browser_hardening_v2_timing_min(ARBOR_BROWSER_HARDENING_V2_TIMING_UPLOAD) != UINT64_C(32) ||
        arbor_browser_hardening_v2_timing_max(ARBOR_BROWSER_HARDENING_V2_TIMING_UPLOAD) != UINT64_C(159)) {
        return fail("ring window min/max mismatch");
    }

    if (arbor_browser_hardening_v2_metric_increment(ARBOR_BROWSER_HARDENING_V2_METRIC_PRESENTATIONS) != 0 ||
        arbor_browser_hardening_v2_metric_increment(ARBOR_BROWSER_HARDENING_V2_METRIC_SOURCE_VIEW_REUSES) != 0) {
        return fail("metric increment failed");
    }
    if (arbor_browser_hardening_v2_metric_value(ARBOR_BROWSER_HARDENING_V2_METRIC_PRESENTATIONS) != 1u ||
        arbor_browser_hardening_v2_metric_value(ARBOR_BROWSER_HARDENING_V2_METRIC_SOURCE_VIEW_REUSES) != 1u) {
        return fail("metric values mismatch");
    }

    puts("PASS: Browser v2 hardening lifecycle, metrics and deterministic timing aggregates");
    return 0;
}
