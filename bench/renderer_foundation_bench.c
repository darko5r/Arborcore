#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "raster_foundation_candidates.h"

#define ITERATIONS UINT64_C(2000000)
#define PIXELS 4096u

static volatile uint64_t sink64;
static volatile uint32_t sink32;
static arbor_r0_rgba8 rgba8_pixels[PIXELS];
static arbor_r0_rgba16 rgba16_pixels[PIXELS];
static arbor_r0_rgba32 rgba32_pixels[PIXELS];

static uint64_t now_ns(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0u;
    return (uint64_t)ts.tv_sec * UINT64_C(1000000000) + (uint64_t)ts.tv_nsec;
}

static double bench_coverage_q24(void)
{
    uint64_t start = now_ns();
    uint64_t i;
    uint32_t acc = 0u;
    for (i = 0; i < ITERATIONS; ++i) {
        arbor_r0_device_rect_raw rect;
        uint64_t frac = (i * UINT64_C(2654435761)) & UINT64_C(0xffffffff);
        rect.x = (int64_t)frac;
        rect.y = (int64_t)((frac >> 3) & UINT64_C(0xffffffff));
        rect.width = ARBOR_R0_COORD_ONE - (int64_t)(frac >> 2);
        rect.height = ARBOR_R0_COORD_ONE - (int64_t)(frac >> 4);
        acc ^= arbor_r3_rect_coverage_q24(rect, 0, 0);
    }
    sink32 = acc;
    return (double)(now_ns() - start) / (double)ITERATIONS;
}

static double bench_coverage_q32(void)
{
    uint64_t start = now_ns();
    uint64_t i;
    uint64_t acc = 0u;
    for (i = 0; i < ITERATIONS; ++i) {
        arbor_r0_device_rect_raw rect;
        uint64_t frac = (i * UINT64_C(2654435761)) & UINT64_C(0xffffffff);
        rect.x = (int64_t)frac;
        rect.y = (int64_t)((frac >> 3) & UINT64_C(0xffffffff));
        rect.width = ARBOR_R0_COORD_ONE - (int64_t)(frac >> 2);
        rect.height = ARBOR_R0_COORD_ONE - (int64_t)(frac >> 4);
        acc ^= arbor_r3_rect_coverage_q32(rect, 0, 0);
    }
    sink64 = acc;
    return (double)(now_ns() - start) / (double)ITERATIONS;
}

static double bench_rgba8_fill(void)
{
    uint64_t start = now_ns();
    uint64_t i;
    for (i = 0; i < ITERATIONS / 1024u; ++i) {
        uint32_t p;
        uint8_t v = (uint8_t)i;
        for (p = 0; p < PIXELS; ++p) {
            rgba8_pixels[p].r = v;
            rgba8_pixels[p].g = (uint8_t)(v + 1u);
            rgba8_pixels[p].b = (uint8_t)(v + 2u);
            rgba8_pixels[p].a = UINT8_MAX;
        }
    }
    sink32 = rgba8_pixels[ITERATIONS % PIXELS].r;
    return (double)(now_ns() - start) / (double)((ITERATIONS / 1024u) * PIXELS);
}

static double bench_rgba16_fill(void)
{
    uint64_t start = now_ns();
    uint64_t i;
    for (i = 0; i < ITERATIONS / 1024u; ++i) {
        uint32_t p;
        uint16_t v = (uint16_t)i;
        for (p = 0; p < PIXELS; ++p) {
            rgba16_pixels[p].r = v;
            rgba16_pixels[p].g = (uint16_t)(v + 1u);
            rgba16_pixels[p].b = (uint16_t)(v + 2u);
            rgba16_pixels[p].a = UINT16_MAX;
        }
    }
    sink32 = rgba16_pixels[ITERATIONS % PIXELS].r;
    return (double)(now_ns() - start) / (double)((ITERATIONS / 1024u) * PIXELS);
}

static double bench_rgba32_fill(void)
{
    uint64_t start = now_ns();
    uint64_t i;
    for (i = 0; i < ITERATIONS / 1024u; ++i) {
        uint32_t p;
        uint32_t v = (uint32_t)i;
        for (p = 0; p < PIXELS; ++p) {
            rgba32_pixels[p].r = v;
            rgba32_pixels[p].g = v + 1u;
            rgba32_pixels[p].b = v + 2u;
            rgba32_pixels[p].a = UINT32_MAX;
        }
    }
    sink32 = rgba32_pixels[ITERATIONS % PIXELS].r;
    return (double)(now_ns() - start) / (double)((ITERATIONS / 1024u) * PIXELS);
}

int main(void)
{
    printf("metric\tns_per_op\n");
    printf("coverage_q24\t%.6f\n", bench_coverage_q24());
    printf("coverage_q32\t%.6f\n", bench_coverage_q32());
    printf("rgba8_fill\t%.6f\n", bench_rgba8_fill());
    printf("rgba16_fill\t%.6f\n", bench_rgba16_fill());
    printf("rgba32_fill\t%.6f\n", bench_rgba32_fill());
    return 0;
}
