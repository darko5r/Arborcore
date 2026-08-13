#include "arborcore/browser_surface.h"

#include "../../browser/linear16_srgb8_bucket12.h"
#include "../../renderer/srgb8_linear16_lut.h"

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__GNUC__) || defined(__clang__)
#define ARBOR_BROWSER_ALWAYS_INLINE static inline __attribute__((always_inline))
#else
#define ARBOR_BROWSER_ALWAYS_INLINE static inline
#endif

static int arbor_browser_u64_mul_checked(uint64_t left, uint64_t right, uint64_t *out)
{
    if (out == (uint64_t *)0) {
        return 0;
    }
    if (left != 0u && right > UINT64_MAX / left) {
        return 0;
    }
    *out = left * right;
    return 1;
}

static int arbor_browser_u64_add_checked(uint64_t left, uint64_t right, uint64_t *out)
{
    if (out == (uint64_t *)0 || right > UINT64_MAX - left) {
        return 0;
    }
    *out = left + right;
    return 1;
}

ARBOR_BROWSER_ALWAYS_INLINE uint16_t arbor_browser_load_u16_le(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

ARBOR_BROWSER_ALWAYS_INLINE uint16_t arbor_browser_unpremultiply(
    uint16_t value,
    uint16_t alpha)
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
    if (alpha == UINT16_MAX) {
        return value;
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

/*
 * Exact inverse accelerator for the frozen sRGB8 -> linear16 table.
 *
 * The 12-bit bucket table stores the exact nearest-even sRGB code for the
 * first linear16 value in each 16-value bucket. Adjacent frozen transfer
 * boundaries are farther apart than one bucket, so at most one exact
 * midpoint comparison is required for the requested value. The exhaustive
 * B1 equivalence test verifies all 65,536 linear16 inputs against the frozen
 * renderer conversion.
 */
ARBOR_BROWSER_ALWAYS_INLINE uint8_t arbor_browser_linear16_to_srgb8(uint16_t value)
{
    uint8_t code = arbor_browser_linear16_to_srgb8_bucket12[value >> ARBOR_BROWSER_LINEAR16_BUCKET_SHIFT];

    if (code < UINT8_MAX) {
        uint32_t lower = (uint32_t)arbor_srgb8_to_linear16[code];
        uint32_t upper = (uint32_t)arbor_srgb8_to_linear16[(uint32_t)code + 1u];
        uint32_t twice_value = (uint32_t)value * UINT32_C(2);
        uint32_t midpoint_sum = lower + upper;

        if (twice_value > midpoint_sum ||
            (twice_value == midpoint_sum && (code & UINT8_C(1)) != 0u)) {
            ++code;
        }
    }
    return code;
}

ARBOR_BROWSER_ALWAYS_INLINE uint8_t arbor_browser_alpha16_to_alpha8(uint16_t alpha)
{
    uint32_t quotient = (uint32_t)alpha / UINT32_C(257);
    uint32_t remainder = (uint32_t)alpha % UINT32_C(257);
    uint32_t twice = remainder * UINT32_C(2);

    if (twice > UINT32_C(257) ||
        (twice == UINT32_C(257) && (quotient & UINT32_C(1)) != 0u)) {
        ++quotient;
    }
    if (quotient > UINT8_MAX) {
        quotient = UINT8_MAX;
    }
    return (uint8_t)quotient;
}

ARBOR_BROWSER_ALWAYS_INLINE arbor_rgba8_srgb arbor_browser_rgba16_to_srgb8_exact(
    arbor_rgba16 color)
{
    arbor_rgba8_srgb out;

    if (color.a == 0u) {
        out.r = 0u;
        out.g = 0u;
        out.b = 0u;
        out.a = 0u;
        return out;
    }

    if (color.a == UINT16_MAX) {
        out.r = arbor_browser_linear16_to_srgb8(color.r);
        out.g = arbor_browser_linear16_to_srgb8(color.g);
        out.b = arbor_browser_linear16_to_srgb8(color.b);
        out.a = UINT8_MAX;
        return out;
    }

    out.r = arbor_browser_linear16_to_srgb8(
        arbor_browser_unpremultiply(color.r, color.a));
    out.g = arbor_browser_linear16_to_srgb8(
        arbor_browser_unpremultiply(color.g, color.a));
    out.b = arbor_browser_linear16_to_srgb8(
        arbor_browser_unpremultiply(color.b, color.a));
    out.a = arbor_browser_alpha16_to_alpha8(color.a);
    return out;
}

arbor_browser_status arbor_browser_export_layout_make(
    uint32_t width,
    uint32_t height,
    uint64_t rgba16_stride_bytes,
    uint64_t rgba8_stride_bytes,
    arbor_browser_export_layout *out)
{
    uint64_t rgba16_row_bytes;
    uint64_t rgba8_row_bytes;
    uint64_t rgba16_prefix;
    uint64_t rgba8_prefix;
    uint64_t rows_minus_one;
    arbor_browser_export_layout candidate;

    if (out == (arbor_browser_export_layout *)0 || width == 0u || height == 0u) {
        return ARBOR_BROWSER_INVALID_ARGUMENT;
    }
    if (!arbor_browser_u64_mul_checked(
            (uint64_t)width,
            (uint64_t)ARBOR_RENDERER_RGBA16_BYTES_PER_PIXEL,
            &rgba16_row_bytes) ||
        !arbor_browser_u64_mul_checked(
            (uint64_t)width,
            (uint64_t)ARBOR_BROWSER_RGBA8_BYTES_PER_PIXEL,
            &rgba8_row_bytes)) {
        return ARBOR_BROWSER_OVERFLOW;
    }
    if (rgba16_stride_bytes < rgba16_row_bytes || rgba8_stride_bytes < rgba8_row_bytes) {
        return ARBOR_BROWSER_INVALID_ARGUMENT;
    }

    rows_minus_one = (uint64_t)height - UINT64_C(1);
    if (!arbor_browser_u64_mul_checked(rows_minus_one, rgba16_stride_bytes, &rgba16_prefix) ||
        !arbor_browser_u64_mul_checked(rows_minus_one, rgba8_stride_bytes, &rgba8_prefix) ||
        !arbor_browser_u64_add_checked(rgba16_prefix, rgba16_row_bytes, &candidate.rgba16_required_bytes) ||
        !arbor_browser_u64_add_checked(rgba8_prefix, rgba8_row_bytes, &candidate.rgba8_required_bytes)) {
        return ARBOR_BROWSER_OVERFLOW;
    }

    candidate.width = width;
    candidate.height = height;
    candidate.rgba16_stride_bytes = rgba16_stride_bytes;
    candidate.rgba8_stride_bytes = rgba8_stride_bytes;
    *out = candidate;
    return ARBOR_BROWSER_OK;
}

arbor_browser_status arbor_browser_export_rgba8(
    const void *rgba16_pixels,
    uint64_t rgba16_buffer_bytes,
    uint64_t rgba16_stride_bytes,
    void *rgba8_pixels,
    uint64_t rgba8_buffer_bytes,
    uint64_t rgba8_stride_bytes,
    uint32_t width,
    uint32_t height)
{
    const uint8_t *source = (const uint8_t *)rgba16_pixels;
    uint8_t *destination = (uint8_t *)rgba8_pixels;
    arbor_browser_export_layout layout;
    arbor_browser_status status;
    uint32_t y;

    if (source == (const uint8_t *)0 || destination == (uint8_t *)0) {
        return ARBOR_BROWSER_INVALID_ARGUMENT;
    }
    status = arbor_browser_export_layout_make(
        width,
        height,
        rgba16_stride_bytes,
        rgba8_stride_bytes,
        &layout);
    if (status != ARBOR_BROWSER_OK) {
        return status;
    }
    if (rgba16_buffer_bytes < layout.rgba16_required_bytes ||
        rgba8_buffer_bytes < layout.rgba8_required_bytes) {
        return ARBOR_BROWSER_BUFFER_TOO_SMALL;
    }

    for (y = 0u; y < height; ++y) {
        const uint8_t *src = source + ((uint64_t)y * rgba16_stride_bytes);
        uint8_t *dst = destination + ((uint64_t)y * rgba8_stride_bytes);
        uint32_t x;

        for (x = 0u; x < width; ++x) {
            const uint8_t *p = src + ((uint64_t)x * UINT64_C(8));
            uint8_t *q = dst + ((uint64_t)x * UINT64_C(4));
            arbor_rgba16 in;
            arbor_rgba8_srgb out;

            in.r = arbor_browser_load_u16_le(p + 0);
            in.g = arbor_browser_load_u16_le(p + 2);
            in.b = arbor_browser_load_u16_le(p + 4);
            in.a = arbor_browser_load_u16_le(p + 6);
            out = arbor_browser_rgba16_to_srgb8_exact(in);
            q[0] = out.r;
            q[1] = out.g;
            q[2] = out.b;
            q[3] = out.a;
        }
    }
    return ARBOR_BROWSER_OK;
}
