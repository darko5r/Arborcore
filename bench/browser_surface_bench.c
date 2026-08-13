#define _POSIX_C_SOURCE 200809L
#include "arborcore/browser_surface.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define SMALL_W 640u
#define SMALL_H 360u
#define LARGE_W 1280u
#define LARGE_H 720u
#define RGBA16_STRIDE(width) ((uint64_t)(width) * UINT64_C(8))
#define RGBA8_STRIDE(width) ((uint64_t)(width) * UINT64_C(4))
#define LARGE_RGBA16_BYTES ((size_t)LARGE_W * (size_t)LARGE_H * 8u)
#define LARGE_RGBA8_BYTES ((size_t)LARGE_W * (size_t)LARGE_H * 4u)

static uint8_t source_pixels[LARGE_RGBA16_BYTES];
static uint8_t destination_pixels[LARGE_RGBA8_BYTES];
static volatile uint64_t checksum_sink;

static uint64_t monotonic_ns(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC_RAW, &ts) != 0) return 0u;
    return (uint64_t)ts.tv_sec * UINT64_C(1000000000) + (uint64_t)ts.tv_nsec;
}

static void store_u16_le(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)(value & UINT16_C(0xff));
    p[1] = (uint8_t)(value >> 8);
}

static void fill_source(uint32_t width, uint32_t height, int mixed_alpha)
{
    arbor_rgba16 opaque = arbor_rgba16_from_srgb8((arbor_rgba8_srgb){28u, 90u, 180u, 255u});
    arbor_rgba16 semi = arbor_rgba16_from_srgb8((arbor_rgba8_srgb){210u, 85u, 35u, 151u});
    uint32_t y;
    for (y = 0u; y < height; ++y) {
        uint32_t x;
        uint8_t *row = source_pixels + ((uint64_t)y * RGBA16_STRIDE(width));
        for (x = 0u; x < width; ++x) {
            arbor_rgba16 c = (mixed_alpha != 0 && ((x + y) & 1u) != 0u) ? semi : opaque;
            uint8_t *p = row + ((uint64_t)x * UINT64_C(8));
            store_u16_le(p + 0, c.r);
            store_u16_le(p + 2, c.g);
            store_u16_le(p + 4, c.b);
            store_u16_le(p + 6, c.a);
        }
    }
}

static uint64_t measure_export(uint32_t width, uint32_t height, uint32_t iterations)
{
    uint64_t start;
    uint64_t end;
    uint32_t i;
    start = monotonic_ns();
    for (i = 0u; i < iterations; ++i) {
        if (arbor_browser_export_rgba8(
                source_pixels,
                RGBA16_STRIDE(width) * (uint64_t)height,
                RGBA16_STRIDE(width),
                destination_pixels,
                RGBA8_STRIDE(width) * (uint64_t)height,
                RGBA8_STRIDE(width),
                width,
                height) != ARBOR_BROWSER_OK) {
            return 0u;
        }
        checksum_sink += destination_pixels[((uint64_t)(i * 7919u)) %
            (RGBA8_STRIDE(width) * (uint64_t)height)];
    }
    end = monotonic_ns();
    if (end <= start || iterations == 0u) return 0u;
    return (end - start) / (uint64_t)iterations;
}

int main(void)
{
    uint64_t small_ns;
    uint64_t large_ns;

    fill_source(SMALL_W, SMALL_H, 1);
    (void)measure_export(SMALL_W, SMALL_H, 1u);
    small_ns = measure_export(SMALL_W, SMALL_H, 5u);

    fill_source(LARGE_W, LARGE_H, 0);
    (void)measure_export(LARGE_W, LARGE_H, 1u);
    large_ns = measure_export(LARGE_W, LARGE_H, 3u);

    if (small_ns == 0u || large_ns == 0u) return 1;
    printf("export_mixed_640x360_ns=%" PRIu64 "\n", small_ns);
    printf("export_opaque_1280x720_ns=%" PRIu64 "\n", large_ns);
    printf("checksum=%" PRIu64 "\n", checksum_sink);
    return 0;
}
