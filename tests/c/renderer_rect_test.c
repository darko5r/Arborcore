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
    uint8_t pixels[4u * 4u * 8u];
    arbor_raster_surface surface;
    arbor_device_rect rect = {
        ARBOR_COORD_ONE / 4,
        ARBOR_COORD_ONE / 4,
        ARBOR_COORD_ONE,
        ARBOR_COORD_ONE
    };
    arbor_rgba16 white = {65535u, 65535u, 65535u, 65535u};
    arbor_rgba16 pixel;
    arbor_pixel_bounds bounds;

    memset(pixels, 0, sizeof(pixels));
    if (arbor_raster_surface_init(&surface, pixels, sizeof(pixels), 4u, 4u, 32u) != ARBOR_RENDERER_OK) {
        return fail("surface init");
    }
    if (arbor_renderer_rect_pixel_bounds(rect, 4u, 4u, &bounds) != ARBOR_RENDERER_OK ||
        bounds.left != 0 || bounds.top != 0 || bounds.right != 2 || bounds.bottom != 2) {
        return fail("fractional rectangle bounds");
    }
    if (arbor_renderer_rect_coverage(rect, 0, 0) != UINT64_C(0x90000000) ||
        arbor_renderer_rect_coverage(rect, 1, 0) != UINT64_C(0x30000000) ||
        arbor_renderer_rect_coverage(rect, 0, 1) != UINT64_C(0x30000000) ||
        arbor_renderer_rect_coverage(rect, 1, 1) != UINT64_C(0x10000000)) {
        return fail("analytical Q0.32 rectangle coverage");
    }
    if (arbor_renderer_fill_rect(surface, rect, white) != ARBOR_RENDERER_OK) {
        return fail("fill rect");
    }
    if (arbor_raster_surface_get_pixel(surface, 0u, 0u, &pixel) != ARBOR_RENDERER_OK ||
        pixel.a != 36863u || pixel.r != 36863u) {
        return fail("fractional pixel 0,0");
    }
    if (arbor_raster_surface_get_pixel(surface, 1u, 0u, &pixel) != ARBOR_RENDERER_OK ||
        pixel.a != 12288u) {
        return fail("fractional pixel 1,0");
    }
    if (arbor_raster_surface_get_pixel(surface, 1u, 1u, &pixel) != ARBOR_RENDERER_OK ||
        pixel.a != 4096u) {
        return fail("fractional pixel 1,1");
    }

    puts("PASS: R4/R6 analytical rectangle rasterization and Q0.32 coverage");
    return 0;
}
