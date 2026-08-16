#include "arborcore/browser_hardening_v2.h"

#include <stdint.h>

uint32_t browser_hardening_v2_selftest(void)
{
    arbor_browser_hardening_v2_reset();
    if (arbor_browser_hardening_v2_version() != ARBOR_BROWSER_HARDENING_V2_VERSION) return 1u;
    if (arbor_browser_hardening_v2_transition(ARBOR_BROWSER_HARDENING_V2_EVENT_PROBE_BEGIN) != 0) return 2u;
    if (arbor_browser_hardening_v2_transition(ARBOR_BROWSER_HARDENING_V2_EVENT_WEBGPU_READY) != 0) return 3u;
    if (arbor_browser_hardening_v2_metric_increment(ARBOR_BROWSER_HARDENING_V2_METRIC_PRESENTATIONS) != 0) return 4u;
    if (arbor_browser_hardening_v2_timing_record(ARBOR_BROWSER_HARDENING_V2_TIMING_SYNCHRONIZED_PRESENT, UINT64_C(1000)) != 0) return 5u;
    if (arbor_browser_hardening_v2_timing_record(ARBOR_BROWSER_HARDENING_V2_TIMING_SYNCHRONIZED_PRESENT, UINT64_C(3000)) != 0) return 6u;
    if (arbor_browser_hardening_v2_timing_mean(ARBOR_BROWSER_HARDENING_V2_TIMING_SYNCHRONIZED_PRESENT) != UINT64_C(2000)) return 7u;
    if (arbor_browser_hardening_v2_timing_percentile_permille(ARBOR_BROWSER_HARDENING_V2_TIMING_SYNCHRONIZED_PRESENT, 950u) != UINT64_C(3000)) return 8u;
    return 0u;
}
