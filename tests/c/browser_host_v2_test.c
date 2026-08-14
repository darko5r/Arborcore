#include "arborcore/browser_host_v2.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int expect_css(int64_t raw, const char *expected)
{
    char out[ARBOR_BROWSER_HOST_V2_CSS_BUFFER_BYTES];
    uint32_t written = 0u;
    arbor_browser_host_v2_status status = arbor_browser_host_v2_format_q32_css(
        raw, out, (uint32_t)sizeof(out), &written);
    if (status != ARBOR_BROWSER_HOST_V2_OK) return 1;
    if (strcmp(out, expected) != 0) return 2;
    if (written != (uint32_t)strlen(expected)) return 3;
    return 0;
}

int main(void)
{
    arbor_browser_host_v2_size size;
    arbor_browser_host_v2_layout layout;
    arbor_browser_host_v2_state state;
    static const char chrome_oom[] = "unknown:vkAllocateMemory failed with VK_ERROR_OUT_OF_DEVICE_MEMORY";
    static const char chrome_map_lost[] = "AbortError:Failed to execute 'mapAsync' on 'GPUBuffer': [Device] is lost.";
    static const char programming[] = "validation:bad bind group";

    if (arbor_browser_host_v2_version() != ARBOR_BROWSER_HOST_V2_VERSION) return 10;
    if (expect_css(INT64_C(0), "0.000000000px") != 0) return 11;
    if (expect_css(INT64_C(4294967296), "1.000000000px") != 0) return 12;
    if (expect_css(-INT64_C(4294967296), "-1.000000000px") != 0) return 13;
    if (expect_css(INT64_C(2147483648), "0.500000000px") != 0) return 14;

    if (arbor_browser_host_v2_resolve_device_size(1u, 640u, 360u, 0.0, 0.0, 0.0, &size) != ARBOR_BROWSER_HOST_V2_OK) return 20;
    if (size.width != 640u || size.height != 360u || size.mode != ARBOR_BROWSER_HOST_V2_SIZE_DEVICE_PIXEL_BOX) return 21;
    if (arbor_browser_host_v2_resolve_device_size(0u, 0u, 0u, 100.25, 50.25, 2.0, &size) != ARBOR_BROWSER_HOST_V2_OK) return 22;
    if (size.width != 201u || size.height != 101u || size.mode != ARBOR_BROWSER_HOST_V2_SIZE_CONTENT_DPR) return 23;

    if (arbor_browser_host_v2_validate_rgba8(16u, 16u, 64u, &layout) != ARBOR_BROWSER_HOST_V2_OK) return 30;
    if (layout.byte_length != UINT64_C(1024)) return 31;
    if (arbor_browser_host_v2_validate_rgba8(16u, 16u, 68u, &layout) != ARBOR_BROWSER_HOST_V2_INVALID_ARGUMENT) return 32;
    if (arbor_browser_host_v2_validate_rgba16(16u, 16u, 128u, &layout) != ARBOR_BROWSER_HOST_V2_OK) return 33;
    if (layout.byte_length != UINT64_C(2048)) return 34;
    if (arbor_browser_host_v2_rgba8_output_bytes() != 1024u) return 35;

    if (arbor_browser_host_v2_classify_gpu_failure(chrome_oom, (uint32_t)(sizeof(chrome_oom) - 1u)) != ARBOR_BROWSER_HOST_V2_GPU_FAILURE_DEVICE_LOSS_OR_ALLOCATION) return 40;
    if (arbor_browser_host_v2_classify_gpu_failure(chrome_map_lost, (uint32_t)(sizeof(chrome_map_lost) - 1u)) != ARBOR_BROWSER_HOST_V2_GPU_FAILURE_DEVICE_LOSS_OR_ALLOCATION) return 41;
    if (arbor_browser_host_v2_classify_gpu_failure(programming, (uint32_t)(sizeof(programming) - 1u)) != ARBOR_BROWSER_HOST_V2_GPU_FAILURE_PROGRAMMING) return 42;

    arbor_browser_host_v2_state_init(&state);
    if (state.present_mode != ARBOR_BROWSER_HOST_V2_PRESENT_FALLBACK) return 50;
    arbor_browser_host_v2_state_webgpu_ready(&state);
    if (state.present_mode != ARBOR_BROWSER_HOST_V2_PRESENT_WEBGPU || state.generation != 1u) return 51;
    arbor_browser_host_v2_state_failure(&state, ARBOR_BROWSER_HOST_V2_GPU_FAILURE_DEVICE_LOSS_OR_ALLOCATION);
    if (state.present_mode != ARBOR_BROWSER_HOST_V2_PRESENT_FALLBACK || state.failure_class != ARBOR_BROWSER_HOST_V2_GPU_FAILURE_DEVICE_LOSS_OR_ALLOCATION) return 52;

    puts("PASS: Browser Host Boundary v2 C authority semantics");
    return 0;
}
