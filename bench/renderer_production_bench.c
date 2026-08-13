#define _POSIX_C_SOURCE 200809L

#include "arborcore/renderer.h"
#include "raster_foundation_candidates.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#if defined(__GNUC__) || defined(__clang__)
#define ARBOR_BENCH_NOINLINE __attribute__((noinline))
#else
#define ARBOR_BENCH_NOINLINE
#endif

#define WIDTH 64u
#define HEIGHT 64u
#define STRIDE (WIDTH * 8u)
#define BYTES (STRIDE * HEIGHT)

static volatile uint64_t arbor_bench_sink;
static volatile uint32_t arbor_bench_width = WIDTH;
static volatile uint32_t arbor_bench_height = HEIGHT;
static volatile uint64_t arbor_bench_stride = STRIDE;
static volatile int64_t arbor_bench_rect_x = ARBOR_COORD_ONE * 7 + ARBOR_COORD_ONE / 4;
static volatile int64_t arbor_bench_rect_y = ARBOR_COORD_ONE * 5 + ARBOR_COORD_ONE / 8;
static volatile int64_t arbor_bench_rect_width = ARBOR_COORD_ONE * 39 + ARBOR_COORD_ONE / 2;
static volatile int64_t arbor_bench_rect_height = ARBOR_COORD_ONE * 31 + ARBOR_COORD_ONE / 4;
static volatile uint8_t arbor_bench_color_r = 180u;
static volatile uint8_t arbor_bench_color_g = 90u;
static volatile uint8_t arbor_bench_color_b = 220u;
static volatile uint8_t arbor_bench_color_a = 180u;

static uint64_t now_ns(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0u;
    return (uint64_t)ts.tv_sec * UINT64_C(1000000000) + (uint64_t)ts.tv_nsec;
}

static void store_u16_le(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)(value & UINT16_C(0xff));
    p[1] = (uint8_t)(value >> 8);
}

static uint16_t load_u16_le(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint16_t unorm_mul(uint16_t left, uint16_t right)
{
    uint64_t product = (uint64_t)left * (uint64_t)right;
    uint64_t q = product / UINT64_C(65535);
    uint64_t r = product % UINT64_C(65535);
    if (r * UINT64_C(2) > UINT64_C(65535)) ++q;
    return (uint16_t)(q > UINT16_MAX ? UINT16_MAX : q);
}

static uint16_t scale_q32(uint16_t value, uint64_t coverage)
{
    uint64_t product;
    uint64_t q;
    uint64_t r;
    if (coverage == 0u) return 0u;
    if (coverage >= ARBOR_COVERAGE_ONE) return value;
    product = (uint64_t)value * coverage;
    q = product >> 32;
    r = product & UINT64_C(0xffffffff);
    if (r > UINT64_C(0x80000000) ||
        (r == UINT64_C(0x80000000) && (q & UINT64_C(1)) != 0u)) ++q;
    return (uint16_t)q;
}

static ARBOR_BENCH_NOINLINE void reference_clear(
    uint8_t *pixels,
    uint32_t width,
    uint32_t height,
    uint64_t stride,
    arbor_rgba16 color)
{
    uint32_t y;
    uint32_t x;
    for (y = 0u; y < height; ++y) {
        uint8_t *row = pixels + (uint64_t)y * stride;
        for (x = 0u; x < width; ++x) {
            uint8_t *p = row + (uint64_t)x * 8u;
            store_u16_le(p + 0, color.r);
            store_u16_le(p + 2, color.g);
            store_u16_le(p + 4, color.b);
            store_u16_le(p + 6, color.a);
        }
    }
}

static ARBOR_BENCH_NOINLINE void reference_blend_pixel(
    uint8_t *pixels,
    uint64_t stride,
    uint32_t x,
    uint32_t y,
    arbor_rgba16 color,
    uint64_t coverage)
{
    uint8_t *p = pixels + (uint64_t)y * stride + (uint64_t)x * 8u;
    uint16_t sr = scale_q32(color.r, coverage);
    uint16_t sg = scale_q32(color.g, coverage);
    uint16_t sb = scale_q32(color.b, coverage);
    uint16_t sa = scale_q32(color.a, coverage);
    uint16_t inv = (uint16_t)(UINT16_MAX - sa);
    uint32_t v;

#define BLEND_AT(offset, src) \
    do { \
        v = (uint32_t)(src) + (uint32_t)unorm_mul(load_u16_le(p + (offset)), inv); \
        store_u16_le(p + (offset), (uint16_t)(v > UINT16_MAX ? UINT16_MAX : v)); \
    } while (0)
    BLEND_AT(0, sr);
    BLEND_AT(2, sg);
    BLEND_AT(4, sb);
    BLEND_AT(6, sa);
#undef BLEND_AT
}

static ARBOR_BENCH_NOINLINE void reference_fill_rect(
    uint8_t *pixels,
    uint32_t width,
    uint32_t height,
    uint64_t stride,
    arbor_r0_device_rect_raw rect,
    arbor_rgba16 color)
{
    arbor_r2_pixel_bounds bounds;
    int64_t y;
    int64_t x;
    if (!arbor_r2_rect_pixel_bounds(rect, width, height, &bounds)) return;
    for (y = bounds.top; y < bounds.bottom; ++y) {
        for (x = bounds.left; x < bounds.right; ++x) {
            uint64_t coverage = arbor_r3_rect_coverage_q32(rect, (int32_t)x, (int32_t)y);
            reference_blend_pixel(pixels, stride, (uint32_t)x, (uint32_t)y, color, coverage);
        }
    }
}

static double measure_reference_clear(
    uint8_t *pixels,
    uint32_t width,
    uint32_t height,
    uint64_t stride,
    arbor_rgba16 color)
{
    const uint32_t iterations = 3000u;
    uint64_t start = now_ns();
    uint32_t i;
    for (i = 0u; i < iterations; ++i) reference_clear(pixels, width, height, stride, color);
    arbor_bench_sink += pixels[(iterations + 7u) % BYTES];
    return (double)(now_ns() - start) / (double)iterations;
}

static double measure_production_clear(arbor_raster_surface surface, arbor_rgba16 color)
{
    const uint32_t iterations = 3000u;
    uint64_t start = now_ns();
    uint32_t i;
    for (i = 0u; i < iterations; ++i) {
        if (arbor_renderer_clear(surface, color) != ARBOR_RENDERER_OK) return -1.0;
    }
    arbor_bench_sink += surface.pixels[(iterations + 7u) % BYTES];
    return (double)(now_ns() - start) / (double)iterations;
}

static double measure_reference_rect(
    uint8_t *pixels,
    uint32_t width,
    uint32_t height,
    uint64_t stride,
    arbor_r0_device_rect_raw rect,
    arbor_rgba16 color)
{
    const uint32_t iterations = 1200u;
    uint64_t start = now_ns();
    uint32_t i;
    for (i = 0u; i < iterations; ++i) reference_fill_rect(pixels, width, height, stride, rect, color);
    arbor_bench_sink += pixels[(iterations + 13u) % BYTES];
    return (double)(now_ns() - start) / (double)iterations;
}

static double measure_production_rect(arbor_raster_surface surface, arbor_device_rect rect, arbor_rgba16 color)
{
    const uint32_t iterations = 1200u;
    uint64_t start = now_ns();
    uint32_t i;
    for (i = 0u; i < iterations; ++i) {
        if (arbor_renderer_fill_rect(surface, rect, color) != ARBOR_RENDERER_OK) return -1.0;
    }
    arbor_bench_sink += surface.pixels[(iterations + 13u) % BYTES];
    return (double)(now_ns() - start) / (double)iterations;
}

static double measure_line(arbor_raster_surface surface, arbor_line line, arbor_rgba16 color)
{
    const uint32_t iterations = 2000u;
    uint64_t start = now_ns();
    uint32_t i;
    for (i = 0u; i < iterations; ++i) {
        if (arbor_renderer_draw_line(surface, line, color) != ARBOR_RENDERER_OK) return -1.0;
    }
    return (double)(now_ns() - start) / (double)iterations;
}

static double measure_path(arbor_raster_surface surface, const arbor_path_command *path, uint32_t count, arbor_rgba16 color)
{
    const uint32_t iterations = 300u;
    uint64_t start = now_ns();
    uint32_t i;
    for (i = 0u; i < iterations; ++i) {
        if (arbor_renderer_stroke_path(surface, path, count, color) != ARBOR_RENDERER_OK) return -1.0;
    }
    return (double)(now_ns() - start) / (double)iterations;
}

int main(void)
{
    static uint8_t reference_pixels[BYTES];
    static uint8_t production_pixels[BYTES];
    uint32_t width = arbor_bench_width;
    uint32_t height = arbor_bench_height;
    uint64_t stride = arbor_bench_stride;
    arbor_raster_surface surface;
    arbor_rgba8_srgb color_input = {
        arbor_bench_color_r,
        arbor_bench_color_g,
        arbor_bench_color_b,
        arbor_bench_color_a
    };
    arbor_rgba16 color = arbor_rgba16_from_srgb8(color_input);
    arbor_r0_device_rect_raw ref_rect = {
        arbor_bench_rect_x,
        arbor_bench_rect_y,
        arbor_bench_rect_width,
        arbor_bench_rect_height
    };
    arbor_device_rect rect = {ref_rect.x, ref_rect.y, ref_rect.width, ref_rect.height};
    arbor_line line = {{ARBOR_COORD_ONE / 2, ARBOR_COORD_ONE},
                       {ARBOR_COORD_ONE * 62, ARBOR_COORD_ONE * 49 + ARBOR_COORD_ONE / 3}};
    arbor_path_command path[] = {
        {ARBOR_PATH_MOVE_TO, {ARBOR_COORD_ONE * 2, ARBOR_COORD_ONE * 50}, {0,0}, {0,0}},
        {ARBOR_PATH_QUAD_TO, {ARBOR_COORD_ONE * 18, ARBOR_COORD_ONE * 8},
                             {ARBOR_COORD_ONE * 34, ARBOR_COORD_ONE * 51}, {0,0}},
        {ARBOR_PATH_CUBIC_TO, {ARBOR_COORD_ONE * 42, ARBOR_COORD_ONE * 62},
                              {ARBOR_COORD_ONE * 54, ARBOR_COORD_ONE * 7},
                              {ARBOR_COORD_ONE * 62, ARBOR_COORD_ONE * 48}}
    };

    memset(reference_pixels, 0, sizeof(reference_pixels));
    memset(production_pixels, 0, sizeof(production_pixels));
    if (width != WIDTH || height != HEIGHT || stride != STRIDE) return 3;
    if (arbor_raster_surface_init(&surface, production_pixels, BYTES, width, height, stride) != ARBOR_RENDERER_OK) {
        return 2;
    }

    puts("benchmark_operand_mode=runtime_nonconstant_shared_surface_rect_color");
    printf("candidate_clear_ns=%.6f\n", measure_reference_clear(reference_pixels, width, height, stride, color));
    printf("production_clear_ns=%.6f\n", measure_production_clear(surface, color));
    memset(reference_pixels, 0, sizeof(reference_pixels));
    memset(production_pixels, 0, sizeof(production_pixels));
    printf("candidate_rect_ns=%.6f\n", measure_reference_rect(reference_pixels, width, height, stride, ref_rect, color));
    printf("production_rect_ns=%.6f\n", measure_production_rect(surface, rect, color));
    printf("production_line_ns=%.6f\n", measure_line(surface, line, color));
    printf("production_path_ns=%.6f\n", measure_path(surface, path, (uint32_t)(sizeof(path) / sizeof(path[0])), color));
    printf("checksum=%llu\n", (unsigned long long)arbor_bench_sink);
    return 0;
}
