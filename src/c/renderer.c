#include "arborcore/renderer.h"

#include <limits.h>

__extension__ typedef __int128 arbor_renderer_i128;
__extension__ typedef unsigned __int128 arbor_renderer_u128;

#if defined(__GNUC__) || defined(__clang__)
#define ARBOR_RENDERER_ALWAYS_INLINE static inline __attribute__((always_inline))
#else
#define ARBOR_RENDERER_ALWAYS_INLINE static inline
#endif

#include "../../renderer/srgb8_linear16_lut.h"


static bool arbor_renderer_u64_mul_checked(uint64_t left, uint64_t right, uint64_t *out)
{
    if (out == (uint64_t *)0) {
        return false;
    }
    if (left != 0u && right > UINT64_MAX / left) {
        return false;
    }
    *out = left * right;
    return true;
}

static bool arbor_renderer_i64_add_checked(int64_t left, int64_t right, int64_t *out)
{
    if (out == (int64_t *)0) {
        return false;
    }
    if ((right > 0 && left > INT64_MAX - right) ||
        (right < 0 && left < INT64_MIN - right)) {
        return false;
    }
    *out = left + right;
    return true;
}

ARBOR_RENDERER_ALWAYS_INLINE uint16_t arbor_renderer_load_u16_le(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

ARBOR_RENDERER_ALWAYS_INLINE void arbor_renderer_store_u16_le(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)(value & UINT16_C(0xff));
    p[1] = (uint8_t)(value >> 8);
}

ARBOR_RENDERER_ALWAYS_INLINE uint16_t arbor_renderer_unorm16_mul(uint16_t left, uint16_t right)
{
    uint64_t product = (uint64_t)left * (uint64_t)right;
    uint64_t quotient = product / UINT64_C(65535);
    uint64_t remainder = product % UINT64_C(65535);
    uint64_t twice = remainder * UINT64_C(2);

    if (twice > UINT64_C(65535) ||
        (twice == UINT64_C(65535) && (quotient & UINT64_C(1)) != 0u)) {
        ++quotient;
    }
    if (quotient > UINT16_MAX) {
        quotient = UINT16_MAX;
    }
    return (uint16_t)quotient;
}

ARBOR_RENDERER_ALWAYS_INLINE uint16_t arbor_renderer_scale_u16_q32(uint16_t value, arbor_coverage coverage)
{
    uint64_t product;
    uint64_t quotient;
    uint64_t remainder;

    if (coverage == 0u || value == 0u) {
        return 0u;
    }
    if (coverage >= ARBOR_COVERAGE_ONE) {
        return value;
    }

    product = (uint64_t)value * coverage;
    quotient = product >> 32;
    remainder = product & UINT64_C(0xffffffff);
    if (remainder > UINT64_C(0x80000000) ||
        (remainder == UINT64_C(0x80000000) && (quotient & UINT64_C(1)) != 0u)) {
        ++quotient;
    }
    if (quotient > UINT16_MAX) {
        quotient = UINT16_MAX;
    }
    return (uint16_t)quotient;
}

ARBOR_RENDERER_ALWAYS_INLINE bool arbor_renderer_rgba16_is_valid_impl(arbor_rgba16 color)
{
    return color.r <= color.a && color.g <= color.a && color.b <= color.a;
}

ARBOR_RENDERER_ALWAYS_INLINE arbor_rgba16 arbor_renderer_apply_coverage_impl(
    arbor_rgba16 color,
    arbor_coverage coverage)
{
    arbor_rgba16 out;
    if (coverage > ARBOR_COVERAGE_ONE) {
        coverage = ARBOR_COVERAGE_ONE;
    }
    if (coverage == 0u) {
        return (arbor_rgba16){0u, 0u, 0u, 0u};
    }
    if (coverage == ARBOR_COVERAGE_ONE) {
        return color;
    }
    out.r = arbor_renderer_scale_u16_q32(color.r, coverage);
    out.g = arbor_renderer_scale_u16_q32(color.g, coverage);
    out.b = arbor_renderer_scale_u16_q32(color.b, coverage);
    out.a = arbor_renderer_scale_u16_q32(color.a, coverage);
    return out;
}

ARBOR_RENDERER_ALWAYS_INLINE arbor_rgba16 arbor_renderer_source_over_impl(
    arbor_rgba16 source,
    arbor_rgba16 destination)
{
    arbor_rgba16 out;
    uint16_t inverse_alpha = (uint16_t)(UINT16_MAX - source.a);
    uint32_t value;

#define ARBOR_BLEND_CHANNEL_IMPL(field) \
    do { \
        value = (uint32_t)source.field + \
            (uint32_t)arbor_renderer_unorm16_mul(destination.field, inverse_alpha); \
        out.field = (uint16_t)(value > UINT16_MAX ? UINT16_MAX : value); \
    } while (0)

    ARBOR_BLEND_CHANNEL_IMPL(r);
    ARBOR_BLEND_CHANNEL_IMPL(g);
    ARBOR_BLEND_CHANNEL_IMPL(b);
    ARBOR_BLEND_CHANNEL_IMPL(a);
#undef ARBOR_BLEND_CHANNEL_IMPL

    if (out.r > out.a) out.r = out.a;
    if (out.g > out.a) out.g = out.a;
    if (out.b > out.a) out.b = out.a;
    return out;
}

static uint16_t arbor_renderer_unpremultiply(uint16_t value, uint16_t alpha)
{
    uint64_t numerator;
    uint64_t quotient;
    uint64_t remainder;
    uint64_t twice;

    if (alpha == 0u || value == 0u) {
        return 0u;
    }
    if (value >= alpha) {
        return UINT16_MAX;
    }

    numerator = (uint64_t)value * UINT64_C(65535);
    quotient = numerator / (uint64_t)alpha;
    remainder = numerator % (uint64_t)alpha;
    twice = remainder * UINT64_C(2);
    if (twice > (uint64_t)alpha ||
        (twice == (uint64_t)alpha && (quotient & UINT64_C(1)) != 0u)) {
        ++quotient;
    }
    if (quotient > UINT16_MAX) {
        quotient = UINT16_MAX;
    }
    return (uint16_t)quotient;
}

static uint8_t arbor_renderer_linear16_to_srgb8(uint16_t value)
{
    uint32_t low = 0u;
    uint32_t high = 255u;

    while (low < high) {
        uint32_t mid = low + ((high - low) / 2u);
        if (arbor_srgb8_to_linear16[mid] < value) {
            low = mid + 1u;
        } else {
            high = mid;
        }
    }

    if (low == 0u) {
        return 0u;
    }
    {
        uint32_t upper = low;
        uint32_t lower = low - 1u;
        uint32_t upper_distance = (uint32_t)arbor_srgb8_to_linear16[upper] - (uint32_t)value;
        uint32_t lower_distance = (uint32_t)value - (uint32_t)arbor_srgb8_to_linear16[lower];
        if (lower_distance < upper_distance ||
            (lower_distance == upper_distance && (lower & 1u) == 0u)) {
            return (uint8_t)lower;
        }
        return (uint8_t)upper;
    }
}

static int64_t arbor_renderer_floor_q32(arbor_coord value)
{
    int64_t quotient = value / ARBOR_COORD_ONE;
    int64_t remainder = value % ARBOR_COORD_ONE;
    if (value < 0 && remainder != 0) {
        --quotient;
    }
    return quotient;
}

static int64_t arbor_renderer_ceil_q32(arbor_coord value)
{
    int64_t quotient = value / ARBOR_COORD_ONE;
    int64_t remainder = value % ARBOR_COORD_ONE;
    if (value > 0 && remainder != 0) {
        ++quotient;
    }
    return quotient;
}

static uint64_t arbor_renderer_overlap_q32(
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

ARBOR_RENDERER_ALWAYS_INLINE arbor_coverage arbor_renderer_coverage_mul(arbor_coverage left, arbor_coverage right)
{
    uint64_t product;
    uint64_t quotient;
    uint64_t remainder;

    if (left == 0u || right == 0u) {
        return 0u;
    }
    if (left >= ARBOR_COVERAGE_ONE) {
        return right > ARBOR_COVERAGE_ONE ? ARBOR_COVERAGE_ONE : right;
    }
    if (right >= ARBOR_COVERAGE_ONE) {
        return left > ARBOR_COVERAGE_ONE ? ARBOR_COVERAGE_ONE : left;
    }

    product = left * right;
    quotient = product >> 32;
    remainder = product & UINT64_C(0xffffffff);
    if (remainder > UINT64_C(0x80000000) ||
        (remainder == UINT64_C(0x80000000) && (quotient & UINT64_C(1)) != 0u)) {
        ++quotient;
    }
    return quotient;
}

static arbor_coord arbor_renderer_midpoint_coord(arbor_coord left, arbor_coord right);

static arbor_renderer_status arbor_renderer_surface_validate(arbor_raster_surface surface)
{
    uint64_t row_bytes;
    uint64_t required_bytes;

    if (!arbor_renderer_u64_mul_checked(
            (uint64_t)surface.width,
            (uint64_t)ARBOR_RENDERER_RGBA16_BYTES_PER_PIXEL,
            &row_bytes) ||
        surface.stride_bytes < row_bytes ||
        !arbor_renderer_u64_mul_checked(
            surface.stride_bytes,
            (uint64_t)surface.height,
            &required_bytes)) {
        return ARBOR_RENDERER_OVERFLOW;
    }
    if (required_bytes > surface.buffer_bytes) {
        return ARBOR_RENDERER_BUFFER_TOO_SMALL;
    }
    if (required_bytes != 0u && surface.pixels == (uint8_t *)0) {
        return ARBOR_RENDERER_INVALID_ARGUMENT;
    }
    return ARBOR_RENDERER_OK;
}

static arbor_renderer_status arbor_renderer_blend_pixel(
    arbor_raster_surface surface,
    int64_t x,
    int64_t y,
    arbor_rgba16 color,
    arbor_coverage coverage)
{
    uint64_t offset;
    uint8_t *p;
    arbor_rgba16 destination;
    arbor_rgba16 source;
    arbor_rgba16 output;
    arbor_renderer_status status;

    if (coverage == 0u || x < 0 || y < 0 ||
        x >= (int64_t)surface.width || y >= (int64_t)surface.height) {
        return ARBOR_RENDERER_OK;
    }

    status = arbor_raster_surface_pixel_offset(
        surface, (uint32_t)x, (uint32_t)y, &offset);
    if (status != ARBOR_RENDERER_OK) {
        return status;
    }
    p = surface.pixels + offset;
    destination.r = arbor_renderer_load_u16_le(p + 0);
    destination.g = arbor_renderer_load_u16_le(p + 2);
    destination.b = arbor_renderer_load_u16_le(p + 4);
    destination.a = arbor_renderer_load_u16_le(p + 6);

    source = arbor_renderer_apply_coverage_impl(color, coverage);
    output = arbor_renderer_source_over_impl(source, destination);

    arbor_renderer_store_u16_le(p + 0, output.r);
    arbor_renderer_store_u16_le(p + 2, output.g);
    arbor_renderer_store_u16_le(p + 4, output.b);
    arbor_renderer_store_u16_le(p + 6, output.a);
    return ARBOR_RENDERER_OK;
}

arbor_renderer_status arbor_raster_surface_init(
    arbor_raster_surface *out,
    void *pixels,
    uint64_t buffer_bytes,
    uint32_t width,
    uint32_t height,
    uint64_t stride_bytes)
{
    uint64_t row_bytes;
    uint64_t required_bytes;

    if (out == (arbor_raster_surface *)0) {
        return ARBOR_RENDERER_INVALID_ARGUMENT;
    }
    if (!arbor_renderer_u64_mul_checked(
            (uint64_t)width,
            (uint64_t)ARBOR_RENDERER_RGBA16_BYTES_PER_PIXEL,
            &row_bytes)) {
        return ARBOR_RENDERER_OVERFLOW;
    }
    if (stride_bytes < row_bytes) {
        return ARBOR_RENDERER_INVALID_ARGUMENT;
    }
    if (!arbor_renderer_u64_mul_checked(stride_bytes, (uint64_t)height, &required_bytes)) {
        return ARBOR_RENDERER_OVERFLOW;
    }
    if (required_bytes > buffer_bytes) {
        return ARBOR_RENDERER_BUFFER_TOO_SMALL;
    }
    if (required_bytes != 0u && pixels == (void *)0) {
        return ARBOR_RENDERER_INVALID_ARGUMENT;
    }

    out->pixels = (uint8_t *)pixels;
    out->width = width;
    out->height = height;
    out->stride_bytes = stride_bytes;
    out->buffer_bytes = buffer_bytes;
    return ARBOR_RENDERER_OK;
}

arbor_renderer_status arbor_raster_surface_pixel_offset(
    arbor_raster_surface surface,
    uint32_t x,
    uint32_t y,
    uint64_t *out_offset)
{
    uint64_t row_offset;
    uint64_t column_offset;
    uint64_t offset;

    if (out_offset == (uint64_t *)0 || x >= surface.width || y >= surface.height ||
        surface.pixels == (uint8_t *)0) {
        return ARBOR_RENDERER_INVALID_ARGUMENT;
    }
    if (!arbor_renderer_u64_mul_checked((uint64_t)y, surface.stride_bytes, &row_offset) ||
        !arbor_renderer_u64_mul_checked(
            (uint64_t)x,
            (uint64_t)ARBOR_RENDERER_RGBA16_BYTES_PER_PIXEL,
            &column_offset) ||
        row_offset > UINT64_MAX - column_offset) {
        return ARBOR_RENDERER_OVERFLOW;
    }
    offset = row_offset + column_offset;
    if (offset > surface.buffer_bytes ||
        (uint64_t)ARBOR_RENDERER_RGBA16_BYTES_PER_PIXEL > surface.buffer_bytes - offset) {
        return ARBOR_RENDERER_BUFFER_TOO_SMALL;
    }
    *out_offset = offset;
    return ARBOR_RENDERER_OK;
}

arbor_renderer_status arbor_raster_surface_get_pixel(
    arbor_raster_surface surface,
    uint32_t x,
    uint32_t y,
    arbor_rgba16 *out)
{
    uint64_t offset;
    arbor_renderer_status status;
    const uint8_t *p;

    if (out == (arbor_rgba16 *)0) {
        return ARBOR_RENDERER_INVALID_ARGUMENT;
    }
    status = arbor_raster_surface_pixel_offset(surface, x, y, &offset);
    if (status != ARBOR_RENDERER_OK) {
        return status;
    }
    p = surface.pixels + offset;
    out->r = arbor_renderer_load_u16_le(p + 0);
    out->g = arbor_renderer_load_u16_le(p + 2);
    out->b = arbor_renderer_load_u16_le(p + 4);
    out->a = arbor_renderer_load_u16_le(p + 6);
    return ARBOR_RENDERER_OK;
}

arbor_renderer_status arbor_raster_surface_set_pixel(
    arbor_raster_surface surface,
    uint32_t x,
    uint32_t y,
    arbor_rgba16 pixel)
{
    uint64_t offset;
    arbor_renderer_status status;
    uint8_t *p;

    if (!arbor_renderer_rgba16_is_valid_impl(pixel)) {
        return ARBOR_RENDERER_INVALID_ARGUMENT;
    }
    status = arbor_raster_surface_pixel_offset(surface, x, y, &offset);
    if (status != ARBOR_RENDERER_OK) {
        return status;
    }
    p = surface.pixels + offset;
    arbor_renderer_store_u16_le(p + 0, pixel.r);
    arbor_renderer_store_u16_le(p + 2, pixel.g);
    arbor_renderer_store_u16_le(p + 4, pixel.b);
    arbor_renderer_store_u16_le(p + 6, pixel.a);
    return ARBOR_RENDERER_OK;
}

arbor_renderer_status arbor_renderer_clear(arbor_raster_surface surface, arbor_rgba16 color)
{
    uint32_t y;
    arbor_renderer_status status;
    const uint64_t packed = (uint64_t)color.r |
        ((uint64_t)color.g << 16) |
        ((uint64_t)color.b << 32) |
        ((uint64_t)color.a << 48);

    if (!arbor_renderer_rgba16_is_valid_impl(color)) {
        return ARBOR_RENDERER_INVALID_ARGUMENT;
    }
    status = arbor_renderer_surface_validate(surface);
    if (status != ARBOR_RENDERER_OK) {
        return status;
    }

    for (y = 0u; y < surface.height; ++y) {
        uint8_t *p = surface.pixels + ((uint64_t)y * surface.stride_bytes);
        uint32_t remaining = surface.width;
        while (remaining >= 8u) {
            __builtin_memcpy(p + 0, &packed, 8u);
            __builtin_memcpy(p + 8, &packed, 8u);
            __builtin_memcpy(p + 16, &packed, 8u);
            __builtin_memcpy(p + 24, &packed, 8u);
            __builtin_memcpy(p + 32, &packed, 8u);
            __builtin_memcpy(p + 40, &packed, 8u);
            __builtin_memcpy(p + 48, &packed, 8u);
            __builtin_memcpy(p + 56, &packed, 8u);
            p += 64u;
            remaining -= 8u;
        }
        while (remaining != 0u) {
            __builtin_memcpy(p, &packed, 8u);
            p += 8u;
            --remaining;
        }
    }
    return ARBOR_RENDERER_OK;
}

bool arbor_rgba16_is_valid_premultiplied(arbor_rgba16 color)
{
    return arbor_renderer_rgba16_is_valid_impl(color);
}

arbor_rgba16 arbor_rgba16_from_srgb8(arbor_rgba8_srgb color)
{
    arbor_rgba16 out;
    uint16_t alpha = (uint16_t)((uint16_t)color.a * UINT16_C(257));
    out.a = alpha;
    out.r = arbor_renderer_unorm16_mul(arbor_srgb8_to_linear16[color.r], alpha);
    out.g = arbor_renderer_unorm16_mul(arbor_srgb8_to_linear16[color.g], alpha);
    out.b = arbor_renderer_unorm16_mul(arbor_srgb8_to_linear16[color.b], alpha);
    return out;
}

arbor_rgba8_srgb arbor_rgba16_to_srgb8(arbor_rgba16 color)
{
    arbor_rgba8_srgb out;
    uint16_t linear_r = arbor_renderer_unpremultiply(color.r, color.a);
    uint16_t linear_g = arbor_renderer_unpremultiply(color.g, color.a);
    uint16_t linear_b = arbor_renderer_unpremultiply(color.b, color.a);
    uint32_t alpha_scaled = (uint32_t)color.a * UINT32_C(255);
    uint32_t alpha_q = alpha_scaled / UINT32_C(65535);
    uint32_t alpha_r = alpha_scaled % UINT32_C(65535);
    uint32_t twice = alpha_r * UINT32_C(2);

    if (twice > UINT32_C(65535) ||
        (twice == UINT32_C(65535) && (alpha_q & 1u) != 0u)) {
        ++alpha_q;
    }
    if (alpha_q > UINT8_MAX) {
        alpha_q = UINT8_MAX;
    }

    out.r = color.a == 0u ? 0u : arbor_renderer_linear16_to_srgb8(linear_r);
    out.g = color.a == 0u ? 0u : arbor_renderer_linear16_to_srgb8(linear_g);
    out.b = color.a == 0u ? 0u : arbor_renderer_linear16_to_srgb8(linear_b);
    out.a = (uint8_t)alpha_q;
    return out;
}

arbor_rgba16 arbor_rgba16_apply_coverage(arbor_rgba16 color, arbor_coverage coverage)
{
    return arbor_renderer_apply_coverage_impl(color, coverage);
}

arbor_rgba16 arbor_rgba16_source_over(arbor_rgba16 source, arbor_rgba16 destination)
{
    return arbor_renderer_source_over_impl(source, destination);
}

static arbor_renderer_status arbor_renderer_rect_pixel_bounds_impl(
    arbor_device_rect rect,
    uint32_t surface_width,
    uint32_t surface_height,
    arbor_pixel_bounds *out)
{
    int64_t right_raw;
    int64_t bottom_raw;
    int64_t left;
    int64_t top;
    int64_t right;
    int64_t bottom;

    if (out == (arbor_pixel_bounds *)0 || rect.width < 0 || rect.height < 0 ||
        !arbor_renderer_i64_add_checked(rect.x, rect.width, &right_raw) ||
        !arbor_renderer_i64_add_checked(rect.y, rect.height, &bottom_raw)) {
        return rect.width < 0 || rect.height < 0 ?
            ARBOR_RENDERER_INVALID_ARGUMENT : ARBOR_RENDERER_OVERFLOW;
    }

    left = arbor_renderer_floor_q32(rect.x);
    top = arbor_renderer_floor_q32(rect.y);
    right = arbor_renderer_ceil_q32(right_raw);
    bottom = arbor_renderer_ceil_q32(bottom_raw);

    if (left < 0) left = 0;
    if (top < 0) top = 0;
    if (right < 0) right = 0;
    if (bottom < 0) bottom = 0;
    if (left > (int64_t)surface_width) left = (int64_t)surface_width;
    if (right > (int64_t)surface_width) right = (int64_t)surface_width;
    if (top > (int64_t)surface_height) top = (int64_t)surface_height;
    if (bottom > (int64_t)surface_height) bottom = (int64_t)surface_height;
    if (right < left) right = left;
    if (bottom < top) bottom = top;

    out->left = left;
    out->top = top;
    out->right = right;
    out->bottom = bottom;
    return ARBOR_RENDERER_OK;
}

arbor_renderer_status arbor_renderer_rect_pixel_bounds(
    arbor_device_rect rect,
    uint32_t surface_width,
    uint32_t surface_height,
    arbor_pixel_bounds *out)
{
    return arbor_renderer_rect_pixel_bounds_impl(
        rect, surface_width, surface_height, out);
}

ARBOR_RENDERER_ALWAYS_INLINE arbor_coverage arbor_renderer_rect_coverage_edges(
    int64_t left_raw,
    int64_t top_raw,
    int64_t right_raw,
    int64_t bottom_raw,
    int32_t pixel_x,
    int32_t pixel_y)
{
    int64_t pixel_left;
    int64_t pixel_top;
    int64_t pixel_right;
    int64_t pixel_bottom;
    uint64_t overlap_x;
    uint64_t overlap_y;

    if (pixel_x < 0 || pixel_y < 0) {
        return 0u;
    }
    if ((uint64_t)(uint32_t)pixel_x > (uint64_t)(INT64_MAX / ARBOR_COORD_ONE) ||
        (uint64_t)(uint32_t)pixel_y > (uint64_t)(INT64_MAX / ARBOR_COORD_ONE)) {
        return 0u;
    }

    pixel_left = (int64_t)pixel_x * ARBOR_COORD_ONE;
    pixel_top = (int64_t)pixel_y * ARBOR_COORD_ONE;
    pixel_right = pixel_left + ARBOR_COORD_ONE;
    pixel_bottom = pixel_top + ARBOR_COORD_ONE;

    overlap_x = arbor_renderer_overlap_q32(left_raw, right_raw, pixel_left, pixel_right);
    overlap_y = arbor_renderer_overlap_q32(top_raw, bottom_raw, pixel_top, pixel_bottom);
    return arbor_renderer_coverage_mul(overlap_x, overlap_y);
}

arbor_coverage arbor_renderer_rect_coverage(
    arbor_device_rect rect,
    int32_t pixel_x,
    int32_t pixel_y)
{
    int64_t right_raw;
    int64_t bottom_raw;

    if (rect.width <= 0 || rect.height <= 0 ||
        !arbor_renderer_i64_add_checked(rect.x, rect.width, &right_raw) ||
        !arbor_renderer_i64_add_checked(rect.y, rect.height, &bottom_raw)) {
        return 0u;
    }
    return arbor_renderer_rect_coverage_edges(
        rect.x, rect.y, right_raw, bottom_raw, pixel_x, pixel_y);
}

arbor_renderer_status arbor_renderer_fill_rect(
    arbor_raster_surface surface,
    arbor_device_rect rect,
    arbor_rgba16 color)
{
    arbor_pixel_bounds bounds;
    arbor_renderer_status status;
    int64_t right_raw;
    int64_t bottom_raw;
    int64_t y;
    int64_t x;

    if (!arbor_renderer_rgba16_is_valid_impl(color)) {
        return ARBOR_RENDERER_INVALID_ARGUMENT;
    }
    status = arbor_renderer_surface_validate(surface);
    if (status != ARBOR_RENDERER_OK) {
        return status;
    }
    if (rect.width < 0 || rect.height < 0) {
        return ARBOR_RENDERER_INVALID_ARGUMENT;
    }
    if (!arbor_renderer_i64_add_checked(rect.x, rect.width, &right_raw) ||
        !arbor_renderer_i64_add_checked(rect.y, rect.height, &bottom_raw)) {
        return ARBOR_RENDERER_OVERFLOW;
    }

    status = arbor_renderer_rect_pixel_bounds_impl(rect, surface.width, surface.height, &bounds);
    if (status != ARBOR_RENDERER_OK) {
        return status;
    }
    for (y = bounds.top; y < bounds.bottom; ++y) {
        uint8_t *row = surface.pixels + ((uint64_t)y * surface.stride_bytes);
        for (x = bounds.left; x < bounds.right; ++x) {
            arbor_coverage coverage = arbor_renderer_rect_coverage_edges(
                rect.x, rect.y, right_raw, bottom_raw, (int32_t)x, (int32_t)y);
            uint8_t *p;
            arbor_rgba16 destination;
            arbor_rgba16 source;
            arbor_rgba16 output;

            if (coverage == 0u) continue;
            p = row + ((uint64_t)x * ARBOR_RENDERER_RGBA16_BYTES_PER_PIXEL);
            destination.r = arbor_renderer_load_u16_le(p + 0);
            destination.g = arbor_renderer_load_u16_le(p + 2);
            destination.b = arbor_renderer_load_u16_le(p + 4);
            destination.a = arbor_renderer_load_u16_le(p + 6);
            source = arbor_renderer_apply_coverage_impl(color, coverage);
            output = arbor_renderer_source_over_impl(source, destination);
            arbor_renderer_store_u16_le(p + 0, output.r);
            arbor_renderer_store_u16_le(p + 2, output.g);
            arbor_renderer_store_u16_le(p + 4, output.b);
            arbor_renderer_store_u16_le(p + 6, output.a);
        }
    }
    return ARBOR_RENDERER_OK;
}

static arbor_renderer_status arbor_renderer_line_device(
    arbor_raster_surface surface,
    arbor_point start,
    arbor_point end,
    arbor_rgba16 color)
{
    arbor_coord x0 = start.x;
    arbor_coord y0 = start.y;
    arbor_coord x1 = end.x;
    arbor_coord y1 = end.y;
    arbor_coord dx;
    arbor_coord dy;
    arbor_coord abs_dx;
    arbor_coord abs_dy;
    bool steep;
    arbor_geometry_status gs;
    int64_t major_start;
    int64_t major_end;
    int64_t major;
    int64_t major_limit;

    if (!arbor_renderer_rgba16_is_valid_impl(color)) {
        return ARBOR_RENDERER_INVALID_ARGUMENT;
    }
    if (x0 == x1 && y0 == y1) {
        int64_t px = arbor_renderer_floor_q32(x0);
        int64_t py = arbor_renderer_floor_q32(y0);
        return arbor_renderer_blend_pixel(surface, px, py, color, ARBOR_COVERAGE_ONE);
    }

    gs = arbor_coord_sub(x1, x0, &dx);
    if (gs != ARBOR_GEOMETRY_OK) return ARBOR_RENDERER_OVERFLOW;
    gs = arbor_coord_sub(y1, y0, &dy);
    if (gs != ARBOR_GEOMETRY_OK) return ARBOR_RENDERER_OVERFLOW;
    gs = arbor_coord_abs(dx, &abs_dx);
    if (gs != ARBOR_GEOMETRY_OK) return ARBOR_RENDERER_OVERFLOW;
    gs = arbor_coord_abs(dy, &abs_dy);
    if (gs != ARBOR_GEOMETRY_OK) return ARBOR_RENDERER_OVERFLOW;

    steep = abs_dy > abs_dx;
    if (steep) {
        arbor_coord tmp;
        tmp = x0; x0 = y0; y0 = tmp;
        tmp = x1; x1 = y1; y1 = tmp;
    }
    if (x0 > x1) {
        arbor_coord tmp;
        tmp = x0; x0 = x1; x1 = tmp;
        tmp = y0; y0 = y1; y1 = tmp;
    }

    gs = arbor_coord_sub(x1, x0, &dx);
    if (gs != ARBOR_GEOMETRY_OK || dx <= 0) return ARBOR_RENDERER_OVERFLOW;
    gs = arbor_coord_sub(y1, y0, &dy);
    if (gs != ARBOR_GEOMETRY_OK) return ARBOR_RENDERER_OVERFLOW;

    major_start = arbor_renderer_floor_q32(x0);
    major_end = arbor_renderer_ceil_q32(x1);
    major_limit = steep ? (int64_t)surface.height : (int64_t)surface.width;
    if (major_start < 0) major_start = 0;
    if (major_end > major_limit) major_end = major_limit;
    if (major_end <= major_start) return ARBOR_RENDERER_OK;

    {
        arbor_coord gradient;
        gs = arbor_coord_div(dy, dx, &gradient);
        if (gs != ARBOR_GEOMETRY_OK) return ARBOR_RENDERER_OVERFLOW;

        for (major = major_start; major < major_end; ++major) {
            arbor_coord cell_start = major * ARBOR_COORD_ONE;
            arbor_coord cell_end = cell_start + ARBOR_COORD_ONE;
            arbor_coord overlap_start = x0 > cell_start ? x0 : cell_start;
            arbor_coord overlap_end = x1 < cell_end ? x1 : cell_end;
            arbor_coord midpoint;
            arbor_coord from_start;
            arbor_coord y_delta;
            arbor_coord y_value;
            arbor_coverage base_coverage;
            uint32_t fraction;
            arbor_coverage lower_coverage;
            arbor_coverage upper_coverage;
            int64_t minor;
            arbor_renderer_status status;

            if (overlap_end <= overlap_start) continue;
            midpoint = arbor_renderer_midpoint_coord(overlap_start, overlap_end);
            gs = arbor_coord_sub(midpoint, x0, &from_start);
            if (gs != ARBOR_GEOMETRY_OK) return ARBOR_RENDERER_OVERFLOW;
            gs = arbor_coord_mul(gradient, from_start, &y_delta);
            if (gs != ARBOR_GEOMETRY_OK) return ARBOR_RENDERER_OVERFLOW;
            gs = arbor_coord_add(y0, y_delta, &y_value);
            if (gs != ARBOR_GEOMETRY_OK) return ARBOR_RENDERER_OVERFLOW;

            base_coverage = (arbor_coverage)(overlap_end - overlap_start);
            fraction = (uint32_t)(uint64_t)y_value;
            lower_coverage = arbor_renderer_coverage_mul(
                base_coverage,
                ARBOR_COVERAGE_ONE - (arbor_coverage)fraction);
            upper_coverage = arbor_renderer_coverage_mul(
                base_coverage,
                (arbor_coverage)fraction);
            minor = arbor_renderer_floor_q32(y_value);

            if (steep) {
                status = arbor_renderer_blend_pixel(surface, minor, major, color, lower_coverage);
                if (status != ARBOR_RENDERER_OK) return status;
                status = arbor_renderer_blend_pixel(surface, minor + 1, major, color, upper_coverage);
            } else {
                status = arbor_renderer_blend_pixel(surface, major, minor, color, lower_coverage);
                if (status != ARBOR_RENDERER_OK) return status;
                status = arbor_renderer_blend_pixel(surface, major, minor + 1, color, upper_coverage);
            }
            if (status != ARBOR_RENDERER_OK) return status;
        }
    }
    return ARBOR_RENDERER_OK;
}

arbor_renderer_status arbor_renderer_draw_line(
    arbor_raster_surface surface,
    arbor_line line,
    arbor_rgba16 color)
{
    arbor_renderer_status status = arbor_renderer_surface_validate(surface);
    if (status != ARBOR_RENDERER_OK) {
        return status;
    }
    return arbor_renderer_line_device(surface, line.start, line.end, color);
}

static arbor_coord arbor_renderer_midpoint_coord(arbor_coord left, arbor_coord right)
{
    arbor_renderer_i128 sum = (arbor_renderer_i128)left + (arbor_renderer_i128)right;
    bool negative = sum < 0;
    arbor_renderer_u128 magnitude = negative ?
        (arbor_renderer_u128)(-(sum + 1)) + (arbor_renderer_u128)1 :
        (arbor_renderer_u128)sum;
    arbor_renderer_u128 quotient = magnitude >> 1;
    arbor_renderer_u128 remainder = magnitude & (arbor_renderer_u128)1;

    if (remainder != 0u && (quotient & (arbor_renderer_u128)1) != 0u) {
        ++quotient;
    }
    if (negative) {
        if (quotient == ((arbor_renderer_u128)1 << 63)) {
            return INT64_MIN;
        }
        return -(arbor_coord)quotient;
    }
    return (arbor_coord)quotient;
}

static arbor_point arbor_renderer_midpoint_point(arbor_point left, arbor_point right)
{
    arbor_point out;
    out.x = arbor_renderer_midpoint_coord(left.x, right.x);
    out.y = arbor_renderer_midpoint_coord(left.y, right.y);
    return out;
}

static arbor_renderer_u128 arbor_renderer_abs_i128(arbor_renderer_i128 value)
{
    if (value >= 0) return (arbor_renderer_u128)value;
    return (arbor_renderer_u128)(-(value + 1)) + (arbor_renderer_u128)1;
}

static bool arbor_renderer_quad_flat_enough(arbor_point p0, arbor_point p1, arbor_point p2)
{
    arbor_renderer_i128 dx = (arbor_renderer_i128)p0.x -
        (arbor_renderer_i128)2 * (arbor_renderer_i128)p1.x +
        (arbor_renderer_i128)p2.x;
    arbor_renderer_i128 dy = (arbor_renderer_i128)p0.y -
        (arbor_renderer_i128)2 * (arbor_renderer_i128)p1.y +
        (arbor_renderer_i128)p2.y;
    arbor_renderer_u128 limit = (arbor_renderer_u128)ARBOR_RENDERER_CURVE_TOLERANCE_RAW * 4u;
    return arbor_renderer_abs_i128(dx) <= limit && arbor_renderer_abs_i128(dy) <= limit;
}

static bool arbor_renderer_cubic_flat_enough(
    arbor_point p0,
    arbor_point p1,
    arbor_point p2,
    arbor_point p3)
{
    arbor_renderer_i128 d1x = (arbor_renderer_i128)p0.x -
        (arbor_renderer_i128)2 * (arbor_renderer_i128)p1.x +
        (arbor_renderer_i128)p2.x;
    arbor_renderer_i128 d1y = (arbor_renderer_i128)p0.y -
        (arbor_renderer_i128)2 * (arbor_renderer_i128)p1.y +
        (arbor_renderer_i128)p2.y;
    arbor_renderer_i128 d2x = (arbor_renderer_i128)p1.x -
        (arbor_renderer_i128)2 * (arbor_renderer_i128)p2.x +
        (arbor_renderer_i128)p3.x;
    arbor_renderer_i128 d2y = (arbor_renderer_i128)p1.y -
        (arbor_renderer_i128)2 * (arbor_renderer_i128)p2.y +
        (arbor_renderer_i128)p3.y;
    arbor_renderer_u128 limit = (arbor_renderer_u128)ARBOR_RENDERER_CURVE_TOLERANCE_RAW * 4u;
    return arbor_renderer_abs_i128(d1x) <= limit &&
        arbor_renderer_abs_i128(d1y) <= limit &&
        arbor_renderer_abs_i128(d2x) <= limit &&
        arbor_renderer_abs_i128(d2y) <= limit;
}

static arbor_renderer_status arbor_renderer_stroke_quad_recursive(
    arbor_raster_surface surface,
    arbor_point p0,
    arbor_point p1,
    arbor_point p2,
    arbor_rgba16 color,
    uint32_t depth)
{
    if (depth >= ARBOR_RENDERER_CURVE_MAX_DEPTH ||
        arbor_renderer_quad_flat_enough(p0, p1, p2)) {
        return arbor_renderer_line_device(surface, p0, p2, color);
    }
    {
        arbor_point p01 = arbor_renderer_midpoint_point(p0, p1);
        arbor_point p12 = arbor_renderer_midpoint_point(p1, p2);
        arbor_point p012 = arbor_renderer_midpoint_point(p01, p12);
        arbor_renderer_status status = arbor_renderer_stroke_quad_recursive(
            surface, p0, p01, p012, color, depth + 1u);
        if (status != ARBOR_RENDERER_OK) return status;
        return arbor_renderer_stroke_quad_recursive(
            surface, p012, p12, p2, color, depth + 1u);
    }
}

static arbor_renderer_status arbor_renderer_stroke_cubic_recursive(
    arbor_raster_surface surface,
    arbor_point p0,
    arbor_point p1,
    arbor_point p2,
    arbor_point p3,
    arbor_rgba16 color,
    uint32_t depth)
{
    if (depth >= ARBOR_RENDERER_CURVE_MAX_DEPTH ||
        arbor_renderer_cubic_flat_enough(p0, p1, p2, p3)) {
        return arbor_renderer_line_device(surface, p0, p3, color);
    }
    {
        arbor_point p01 = arbor_renderer_midpoint_point(p0, p1);
        arbor_point p12 = arbor_renderer_midpoint_point(p1, p2);
        arbor_point p23 = arbor_renderer_midpoint_point(p2, p3);
        arbor_point p012 = arbor_renderer_midpoint_point(p01, p12);
        arbor_point p123 = arbor_renderer_midpoint_point(p12, p23);
        arbor_point p0123 = arbor_renderer_midpoint_point(p012, p123);
        arbor_renderer_status status = arbor_renderer_stroke_cubic_recursive(
            surface, p0, p01, p012, p0123, color, depth + 1u);
        if (status != ARBOR_RENDERER_OK) return status;
        return arbor_renderer_stroke_cubic_recursive(
            surface, p0123, p123, p23, p3, color, depth + 1u);
    }
}

arbor_renderer_status arbor_renderer_stroke_path(
    arbor_raster_surface surface,
    const arbor_path_command *commands,
    uint32_t command_count,
    arbor_rgba16 color)
{
    arbor_point current = {0, 0};
    arbor_point subpath_start = {0, 0};
    bool have_current = false;
    uint32_t i;

    arbor_renderer_status surface_status = arbor_renderer_surface_validate(surface);
    if (surface_status != ARBOR_RENDERER_OK) {
        return surface_status;
    }
    if (!arbor_renderer_rgba16_is_valid_impl(color) ||
        (command_count != 0u && commands == (const arbor_path_command *)0)) {
        return ARBOR_RENDERER_INVALID_ARGUMENT;
    }
    if (command_count > ARBOR_RENDERER_MAX_PATH_COMMANDS) {
        return ARBOR_RENDERER_PATH_LIMIT;
    }

    for (i = 0u; i < command_count; ++i) {
        const arbor_path_command *command = &commands[i];
        arbor_renderer_status status;
        switch (command->type) {
        case ARBOR_PATH_MOVE_TO:
            current = command->p1;
            subpath_start = current;
            have_current = true;
            break;
        case ARBOR_PATH_LINE_TO:
            if (!have_current) return ARBOR_RENDERER_INVALID_ARGUMENT;
            status = arbor_renderer_line_device(surface, current, command->p1, color);
            if (status != ARBOR_RENDERER_OK) return status;
            current = command->p1;
            break;
        case ARBOR_PATH_QUAD_TO:
            if (!have_current) return ARBOR_RENDERER_INVALID_ARGUMENT;
            status = arbor_renderer_stroke_quad_recursive(
                surface, current, command->p1, command->p2, color, 0u);
            if (status != ARBOR_RENDERER_OK) return status;
            current = command->p2;
            break;
        case ARBOR_PATH_CUBIC_TO:
            if (!have_current) return ARBOR_RENDERER_INVALID_ARGUMENT;
            status = arbor_renderer_stroke_cubic_recursive(
                surface, current, command->p1, command->p2, command->p3, color, 0u);
            if (status != ARBOR_RENDERER_OK) return status;
            current = command->p3;
            break;
        case ARBOR_PATH_CLOSE:
            if (!have_current) return ARBOR_RENDERER_INVALID_ARGUMENT;
            status = arbor_renderer_line_device(surface, current, subpath_start, color);
            if (status != ARBOR_RENDERER_OK) return status;
            current = subpath_start;
            break;
        default:
            return ARBOR_RENDERER_INVALID_ARGUMENT;
        }
    }
    return ARBOR_RENDERER_OK;
}
