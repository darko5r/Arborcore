#ifndef ARBORCORE_RENDERER_H
#define ARBORCORE_RENDERER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "arborcore/geometry.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ARBOR_RENDERER_REFERENCE_CONTRACT_MAJOR 1u
#define ARBOR_RENDERER_REFERENCE_CONTRACT_MINOR 0u
#define ARBOR_COVERAGE_FRACTION_BITS 32u
#define ARBOR_COVERAGE_ONE UINT64_C(4294967296)
#define ARBOR_RGBA16_MAX UINT16_C(65535)
#define ARBOR_RENDERER_RGBA16_BYTES_PER_PIXEL 8u
#define ARBOR_RENDERER_MAX_PATH_COMMANDS UINT32_C(4096)
#define ARBOR_RENDERER_CURVE_MAX_DEPTH 12u
#define ARBOR_RENDERER_CURVE_TOLERANCE_RAW INT64_C(16777216)

typedef uint64_t arbor_coverage;

typedef enum arbor_renderer_status {
    ARBOR_RENDERER_OK = 0,
    ARBOR_RENDERER_INVALID_ARGUMENT,
    ARBOR_RENDERER_OVERFLOW,
    ARBOR_RENDERER_BUFFER_TOO_SMALL,
    ARBOR_RENDERER_PATH_LIMIT
} arbor_renderer_status;

typedef struct arbor_rgba8_srgb {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} arbor_rgba8_srgb;

typedef struct arbor_rgba16 {
    uint16_t r;
    uint16_t g;
    uint16_t b;
    uint16_t a;
} arbor_rgba16;

typedef struct arbor_raster_surface {
    uint8_t *pixels;
    uint32_t width;
    uint32_t height;
    uint64_t stride_bytes;
    uint64_t buffer_bytes;
} arbor_raster_surface;

typedef struct arbor_pixel_bounds {
    int64_t left;
    int64_t top;
    int64_t right;
    int64_t bottom;
} arbor_pixel_bounds;

typedef enum arbor_path_command_type {
    ARBOR_PATH_MOVE_TO = 0,
    ARBOR_PATH_LINE_TO,
    ARBOR_PATH_QUAD_TO,
    ARBOR_PATH_CUBIC_TO,
    ARBOR_PATH_CLOSE
} arbor_path_command_type;

typedef struct arbor_path_command {
    arbor_path_command_type type;
    arbor_point p1;
    arbor_point p2;
    arbor_point p3;
} arbor_path_command;

arbor_renderer_status arbor_raster_surface_init(
    arbor_raster_surface *out,
    void *pixels,
    uint64_t buffer_bytes,
    uint32_t width,
    uint32_t height,
    uint64_t stride_bytes);
arbor_renderer_status arbor_raster_surface_pixel_offset(
    arbor_raster_surface surface,
    uint32_t x,
    uint32_t y,
    uint64_t *out_offset);
arbor_renderer_status arbor_raster_surface_get_pixel(
    arbor_raster_surface surface,
    uint32_t x,
    uint32_t y,
    arbor_rgba16 *out);
arbor_renderer_status arbor_raster_surface_set_pixel(
    arbor_raster_surface surface,
    uint32_t x,
    uint32_t y,
    arbor_rgba16 pixel);
arbor_renderer_status arbor_renderer_clear(arbor_raster_surface surface, arbor_rgba16 color);

bool arbor_rgba16_is_valid_premultiplied(arbor_rgba16 color);
arbor_rgba16 arbor_rgba16_from_srgb8(arbor_rgba8_srgb color);
arbor_rgba8_srgb arbor_rgba16_to_srgb8(arbor_rgba16 color);
arbor_rgba16 arbor_rgba16_apply_coverage(arbor_rgba16 color, arbor_coverage coverage);
arbor_rgba16 arbor_rgba16_source_over(arbor_rgba16 source, arbor_rgba16 destination);

arbor_renderer_status arbor_renderer_rect_pixel_bounds(
    arbor_device_rect rect,
    uint32_t surface_width,
    uint32_t surface_height,
    arbor_pixel_bounds *out);
arbor_coverage arbor_renderer_rect_coverage(
    arbor_device_rect rect,
    int32_t pixel_x,
    int32_t pixel_y);

arbor_renderer_status arbor_renderer_fill_rect(
    arbor_raster_surface surface,
    arbor_device_rect rect,
    arbor_rgba16 color);
arbor_renderer_status arbor_renderer_draw_line(
    arbor_raster_surface surface,
    arbor_line line,
    arbor_rgba16 color);
arbor_renderer_status arbor_renderer_stroke_path(
    arbor_raster_surface surface,
    const arbor_path_command *commands,
    uint32_t command_count,
    arbor_rgba16 color);

#ifdef __cplusplus
}
#endif

#endif
