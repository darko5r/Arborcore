#ifndef ARBORCORE_BROWSER_SURFACE_H
#define ARBORCORE_BROWSER_SURFACE_H

#include <stdint.h>

#include "arborcore/renderer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ARBOR_BROWSER_RGBA8_BYTES_PER_PIXEL UINT32_C(4)

typedef enum arbor_browser_status {
    ARBOR_BROWSER_OK = 0,
    ARBOR_BROWSER_INVALID_ARGUMENT,
    ARBOR_BROWSER_OVERFLOW,
    ARBOR_BROWSER_BUFFER_TOO_SMALL
} arbor_browser_status;

typedef struct arbor_browser_export_layout {
    uint32_t width;
    uint32_t height;
    uint64_t rgba16_stride_bytes;
    uint64_t rgba8_stride_bytes;
    uint64_t rgba16_required_bytes;
    uint64_t rgba8_required_bytes;
} arbor_browser_export_layout;

arbor_browser_status arbor_browser_export_layout_make(
    uint32_t width,
    uint32_t height,
    uint64_t rgba16_stride_bytes,
    uint64_t rgba8_stride_bytes,
    arbor_browser_export_layout *out);

arbor_browser_status arbor_browser_export_rgba8(
    const void *rgba16_pixels,
    uint64_t rgba16_buffer_bytes,
    uint64_t rgba16_stride_bytes,
    void *rgba8_pixels,
    uint64_t rgba8_buffer_bytes,
    uint64_t rgba8_stride_bytes,
    uint32_t width,
    uint32_t height);

#ifdef __cplusplus
}
#endif

#endif
