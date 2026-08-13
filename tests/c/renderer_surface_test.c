#include "arborcore/renderer.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    return 1;
}

int main(void)
{
    uint8_t guarded[16 + (4 * 4 * 8) + 16];
    uint8_t *pixels = guarded + 16;
    arbor_raster_surface surface;
    arbor_rgba16 color = {100u, 200u, 300u, 400u};
    arbor_rgba16 readback;
    uint64_t offset = UINT64_MAX;
    size_t i;

    memset(guarded, 0xa5, sizeof(guarded));
    if (arbor_raster_surface_init(&surface, pixels, 128u, 4u, 4u, 32u) != ARBOR_RENDERER_OK) {
        return fail("surface init");
    }
    if (arbor_raster_surface_pixel_offset(surface, 3u, 2u, &offset) != ARBOR_RENDERER_OK || offset != 88u) {
        return fail("pixel offset");
    }
    if (arbor_raster_surface_pixel_offset(surface, 4u, 0u, &offset) == ARBOR_RENDERER_OK) {
        return fail("out-of-range pixel accepted");
    }
    if (arbor_raster_surface_init(&surface, pixels, 128u, 4u, 4u, 31u) == ARBOR_RENDERER_OK) {
        return fail("short stride accepted");
    }
    if (arbor_raster_surface_init(&surface, pixels, 127u, 4u, 4u, 32u) != ARBOR_RENDERER_BUFFER_TOO_SMALL) {
        return fail("small buffer classification");
    }
    if (arbor_raster_surface_init(&surface, pixels, 128u, 4u, 4u, 32u) != ARBOR_RENDERER_OK) {
        return fail("surface re-init");
    }
    if (arbor_renderer_clear(surface, color) != ARBOR_RENDERER_OK) {
        return fail("clear");
    }
    if (arbor_raster_surface_get_pixel(surface, 3u, 3u, &readback) != ARBOR_RENDERER_OK ||
        readback.r != 100u || readback.g != 200u || readback.b != 300u || readback.a != 400u) {
        return fail("clear readback");
    }
    for (i = 0u; i < 16u; ++i) {
        if (guarded[i] != 0xa5u || guarded[16u + 128u + i] != 0xa5u) {
            return fail("surface guard modified");
        }
    }

    {
        uint8_t padded[48u];
        arbor_raster_surface padded_surface;
        size_t row;
        memset(padded, 0x5au, sizeof(padded));
        if (arbor_raster_surface_init(&padded_surface, padded, sizeof(padded), 2u, 2u, 24u) != ARBOR_RENDERER_OK) {
            return fail("padded surface init");
        }
        if (arbor_renderer_clear(padded_surface, color) != ARBOR_RENDERER_OK) {
            return fail("padded clear");
        }
        for (row = 0u; row < 2u; ++row) {
            size_t padding;
            for (padding = 16u; padding < 24u; ++padding) {
                if (padded[row * 24u + padding] != 0x5au) {
                    return fail("clear modified row padding");
                }
            }
        }
    }

    puts("PASS: R4 checked RGBA16 surface memory and clear semantics");
    return 0;
}
