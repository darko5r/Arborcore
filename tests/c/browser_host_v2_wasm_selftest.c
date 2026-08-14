#include "arborcore/browser_host_v2.h"

#include <stdint.h>

uint32_t browser_host_v2_selftest(void)
{
    arbor_browser_host_v2_size *size = (arbor_browser_host_v2_size *)(uintptr_t)arbor_browser_host_v2_size_scratch_ptr();
    arbor_browser_host_v2_layout *layout = (arbor_browser_host_v2_layout *)(uintptr_t)arbor_browser_host_v2_layout_scratch_ptr();
    uint32_t *written = (uint32_t *)(uintptr_t)arbor_browser_host_v2_written_scratch_ptr();
    char *css = (char *)(uintptr_t)arbor_browser_host_v2_css_scratch_ptr();

    if (arbor_browser_host_v2_format_q32_css(
            INT64_C(4294967296), css,
            arbor_browser_host_v2_css_scratch_bytes(), written) != ARBOR_BROWSER_HOST_V2_OK) return 1u;
    if (*written != 13u) return 2u;
    if (arbor_browser_host_v2_resolve_device_size(1u, 640u, 360u, 0.0, 0.0, 0.0, size) != ARBOR_BROWSER_HOST_V2_OK) return 3u;
    if (arbor_browser_host_v2_validate_rgba8(16u, 16u, 64u, layout) != ARBOR_BROWSER_HOST_V2_OK) return 4u;
    arbor_browser_host_v2_prepare_gpu_tables();
    if (arbor_browser_host_v2_bucket12_bytes() != 4096u * 4u) return 5u;
    if (arbor_browser_host_v2_forward_lut_bytes() != 256u * 4u) return 6u;
    return 0u;
}
