#ifndef ARBORCORE_RASTER_FOUNDATION_CANDIDATES_H
#define ARBORCORE_RASTER_FOUNDATION_CANDIDATES_H

#include <stdbool.h>
#include <stdint.h>

#define ARBOR_R0_MIN_COLOR_CHANNEL_BITS 16u
#define ARBOR_R0_MIN_COVERAGE_FRACTION_BITS 24u
#define ARBOR_R0_MAX_SURFACE_DIMENSION INT32_MAX
#define ARBOR_R0_COORD_ONE INT64_C(4294967296)
#define ARBOR_R0_COORD_HALF INT64_C(2147483648)

typedef struct arbor_r0_rgba8 {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} arbor_r0_rgba8;

typedef struct arbor_r0_rgba16 {
    uint16_t r;
    uint16_t g;
    uint16_t b;
    uint16_t a;
} arbor_r0_rgba16;

typedef struct arbor_r0_rgba32 {
    uint32_t r;
    uint32_t g;
    uint32_t b;
    uint32_t a;
} arbor_r0_rgba32;

typedef uint32_t arbor_r0_coverage_q24;
typedef uint64_t arbor_r0_coverage_q32;

#define ARBOR_R0_COVERAGE_Q24_ONE UINT32_C(16777216)
#define ARBOR_R0_COVERAGE_Q32_ONE UINT64_C(4294967296)

typedef struct arbor_r1_surface {
    uint8_t *pixels;
    uint32_t width;
    uint32_t height;
    uint64_t stride_bytes;
    uint64_t buffer_bytes;
    uint32_t bytes_per_pixel;
} arbor_r1_surface;

typedef struct arbor_r2_pixel_bounds {
    int64_t left;
    int64_t top;
    int64_t right;
    int64_t bottom;
} arbor_r2_pixel_bounds;

typedef struct arbor_r0_device_rect_raw {
    int64_t x;
    int64_t y;
    int64_t width;
    int64_t height;
} arbor_r0_device_rect_raw;

static inline bool arbor_r0_i64_add_checked(int64_t left, int64_t right, int64_t *out)
{
    if ((right > 0 && left > INT64_MAX - right) ||
        (right < 0 && left < INT64_MIN - right)) {
        return false;
    }
    *out = left + right;
    return true;
}

static inline bool arbor_r0_u64_mul_checked(uint64_t left, uint64_t right, uint64_t *out)
{
    if (left != 0u && right > UINT64_MAX / left) {
        return false;
    }
    *out = left * right;
    return true;
}

static inline bool arbor_r1_surface_prepare(
    uint8_t *pixels,
    uint64_t buffer_bytes,
    uint32_t width,
    uint32_t height,
    uint64_t stride_bytes,
    uint32_t bytes_per_pixel,
    arbor_r1_surface *out)
{
    uint64_t row_bytes;
    uint64_t required_bytes;

    if (out == (arbor_r1_surface *)0 || bytes_per_pixel == 0u ||
        width > (uint32_t)ARBOR_R0_MAX_SURFACE_DIMENSION ||
        height > (uint32_t)ARBOR_R0_MAX_SURFACE_DIMENSION) {
        return false;
    }

    if (!arbor_r0_u64_mul_checked((uint64_t)width, (uint64_t)bytes_per_pixel, &row_bytes)) {
        return false;
    }
    if (stride_bytes < row_bytes) {
        return false;
    }
    if (!arbor_r0_u64_mul_checked(stride_bytes, (uint64_t)height, &required_bytes)) {
        return false;
    }
    if (required_bytes > buffer_bytes || (required_bytes != 0u && pixels == (uint8_t *)0)) {
        return false;
    }

    out->pixels = pixels;
    out->width = width;
    out->height = height;
    out->stride_bytes = stride_bytes;
    out->buffer_bytes = buffer_bytes;
    out->bytes_per_pixel = bytes_per_pixel;
    return true;
}

static inline bool arbor_r1_surface_pixel_offset(
    arbor_r1_surface surface,
    uint32_t x,
    uint32_t y,
    uint64_t *out_offset)
{
    uint64_t row_offset;
    uint64_t column_offset;
    uint64_t offset;

    if (out_offset == (uint64_t *)0 || x >= surface.width || y >= surface.height) {
        return false;
    }
    if (!arbor_r0_u64_mul_checked((uint64_t)y, surface.stride_bytes, &row_offset) ||
        !arbor_r0_u64_mul_checked((uint64_t)x, (uint64_t)surface.bytes_per_pixel, &column_offset) ||
        row_offset > UINT64_MAX - column_offset) {
        return false;
    }
    offset = row_offset + column_offset;
    if ((uint64_t)surface.bytes_per_pixel > surface.buffer_bytes ||
        offset > surface.buffer_bytes - (uint64_t)surface.bytes_per_pixel) {
        return false;
    }
    *out_offset = offset;
    return true;
}

static inline int64_t arbor_r2_floor_q32(int64_t value)
{
    int64_t quotient = value / ARBOR_R0_COORD_ONE;
    int64_t remainder = value % ARBOR_R0_COORD_ONE;
    if (value < 0 && remainder != 0) {
        --quotient;
    }
    return quotient;
}

static inline int64_t arbor_r2_ceil_q32(int64_t value)
{
    int64_t quotient = value / ARBOR_R0_COORD_ONE;
    int64_t remainder = value % ARBOR_R0_COORD_ONE;
    if (value > 0 && remainder != 0) {
        ++quotient;
    }
    return quotient;
}

static inline bool arbor_r2_rect_pixel_bounds(
    arbor_r0_device_rect_raw rect,
    uint32_t surface_width,
    uint32_t surface_height,
    arbor_r2_pixel_bounds *out)
{
    int64_t right_raw;
    int64_t bottom_raw;
    int64_t left;
    int64_t top;
    int64_t right;
    int64_t bottom;
    int64_t max_x = (int64_t)surface_width;
    int64_t max_y = (int64_t)surface_height;

    if (out == (arbor_r2_pixel_bounds *)0 || rect.width < 0 || rect.height < 0 ||
        surface_width > (uint32_t)ARBOR_R0_MAX_SURFACE_DIMENSION ||
        surface_height > (uint32_t)ARBOR_R0_MAX_SURFACE_DIMENSION ||
        !arbor_r0_i64_add_checked(rect.x, rect.width, &right_raw) ||
        !arbor_r0_i64_add_checked(rect.y, rect.height, &bottom_raw)) {
        return false;
    }

    left = arbor_r2_floor_q32(rect.x);
    top = arbor_r2_floor_q32(rect.y);
    right = arbor_r2_ceil_q32(right_raw);
    bottom = arbor_r2_ceil_q32(bottom_raw);

    if (left < 0) left = 0;
    if (top < 0) top = 0;
    if (right < 0) right = 0;
    if (bottom < 0) bottom = 0;
    if (left > max_x) left = max_x;
    if (right > max_x) right = max_x;
    if (top > max_y) top = max_y;
    if (bottom > max_y) bottom = max_y;
    if (right < left) right = left;
    if (bottom < top) bottom = top;

    out->left = left;
    out->top = top;
    out->right = right;
    out->bottom = bottom;
    return true;
}

static inline uint64_t arbor_r3_overlap_q32(
    int64_t shape_start,
    int64_t shape_end,
    int64_t pixel_start,
    int64_t pixel_end)
{
    int64_t start = shape_start > pixel_start ? shape_start : pixel_start;
    int64_t end = shape_end < pixel_end ? shape_end : pixel_end;
    if (end <= start) {
        return 0u;
    }
    return (uint64_t)(end - start);
}

static inline arbor_r0_coverage_q32 arbor_r3_rect_coverage_q32(
    arbor_r0_device_rect_raw rect,
    int32_t pixel_x,
    int32_t pixel_y)
{
    int64_t right_raw;
    int64_t bottom_raw;
    int64_t pixel_left;
    int64_t pixel_top;
    int64_t pixel_right;
    int64_t pixel_bottom;
    uint64_t overlap_x;
    uint64_t overlap_y;
    uint64_t product;
    uint64_t quotient;
    uint64_t remainder;

    if (rect.width <= 0 || rect.height <= 0 || pixel_x < 0 || pixel_y < 0 ||
        pixel_x >= ARBOR_R0_MAX_SURFACE_DIMENSION || pixel_y >= ARBOR_R0_MAX_SURFACE_DIMENSION ||
        !arbor_r0_i64_add_checked(rect.x, rect.width, &right_raw) ||
        !arbor_r0_i64_add_checked(rect.y, rect.height, &bottom_raw)) {
        return 0u;
    }

    pixel_left = (int64_t)pixel_x * ARBOR_R0_COORD_ONE;
    pixel_top = (int64_t)pixel_y * ARBOR_R0_COORD_ONE;
    pixel_right = pixel_left + ARBOR_R0_COORD_ONE;
    pixel_bottom = pixel_top + ARBOR_R0_COORD_ONE;

    overlap_x = arbor_r3_overlap_q32(rect.x, right_raw, pixel_left, pixel_right);
    overlap_y = arbor_r3_overlap_q32(rect.y, bottom_raw, pixel_top, pixel_bottom);

    if (overlap_x == 0u || overlap_y == 0u) {
        return 0u;
    }
    if (overlap_x == ARBOR_R0_COVERAGE_Q32_ONE && overlap_y == ARBOR_R0_COVERAGE_Q32_ONE) {
        return ARBOR_R0_COVERAGE_Q32_ONE;
    }

    product = overlap_x * overlap_y;
    quotient = product >> 32;
    remainder = product & UINT64_C(0xffffffff);
    if (remainder > UINT64_C(0x80000000) ||
        (remainder == UINT64_C(0x80000000) && (quotient & 1u) != 0u)) {
        ++quotient;
    }
    return quotient;
}

static inline arbor_r0_coverage_q24 arbor_r3_rect_coverage_q24(
    arbor_r0_device_rect_raw rect,
    int32_t pixel_x,
    int32_t pixel_y)
{
    int64_t right_raw;
    int64_t bottom_raw;
    int64_t pixel_left;
    int64_t pixel_top;
    int64_t pixel_right;
    int64_t pixel_bottom;
    uint64_t overlap_x;
    uint64_t overlap_y;
    uint64_t product;
    uint64_t quotient;
    uint64_t remainder;
    const uint64_t mask = (UINT64_C(1) << 40) - UINT64_C(1);
    const uint64_t half = UINT64_C(1) << 39;

    if (rect.width <= 0 || rect.height <= 0 || pixel_x < 0 || pixel_y < 0 ||
        pixel_x >= ARBOR_R0_MAX_SURFACE_DIMENSION || pixel_y >= ARBOR_R0_MAX_SURFACE_DIMENSION ||
        !arbor_r0_i64_add_checked(rect.x, rect.width, &right_raw) ||
        !arbor_r0_i64_add_checked(rect.y, rect.height, &bottom_raw)) {
        return 0u;
    }

    pixel_left = (int64_t)pixel_x * ARBOR_R0_COORD_ONE;
    pixel_top = (int64_t)pixel_y * ARBOR_R0_COORD_ONE;
    pixel_right = pixel_left + ARBOR_R0_COORD_ONE;
    pixel_bottom = pixel_top + ARBOR_R0_COORD_ONE;
    overlap_x = arbor_r3_overlap_q32(rect.x, right_raw, pixel_left, pixel_right);
    overlap_y = arbor_r3_overlap_q32(rect.y, bottom_raw, pixel_top, pixel_bottom);

    if (overlap_x == 0u || overlap_y == 0u) return 0u;
    if (overlap_x == ARBOR_R0_COVERAGE_Q32_ONE && overlap_y == ARBOR_R0_COVERAGE_Q32_ONE) {
        return ARBOR_R0_COVERAGE_Q24_ONE;
    }

    product = overlap_x * overlap_y;
    quotient = product >> 40;
    remainder = product & mask;
    if (remainder > half || (remainder == half && (quotient & 1u) != 0u)) {
        ++quotient;
    }
    return (arbor_r0_coverage_q24)quotient;
}

#endif
