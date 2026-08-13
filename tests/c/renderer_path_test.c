#include "arborcore/renderer.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    return 1;
}

static uint64_t alpha_sum(arbor_raster_surface surface)
{
    uint64_t sum = 0u;
    uint32_t y;
    uint32_t x;
    for (y = 0u; y < surface.height; ++y) {
        for (x = 0u; x < surface.width; ++x) {
            arbor_rgba16 pixel;
            if (arbor_raster_surface_get_pixel(surface, x, y, &pixel) == ARBOR_RENDERER_OK) {
                sum += pixel.a;
            }
        }
    }
    return sum;
}

int main(void)
{
    uint8_t pixels[16u * 16u * 8u];
    arbor_raster_surface surface;
    arbor_rgba16 white = {65535u, 65535u, 65535u, 65535u};
    arbor_line line = {{ARBOR_COORD_ONE / 2, ARBOR_COORD_ONE / 2},
                       {ARBOR_COORD_ONE * 14 + ARBOR_COORD_ONE / 2,
                        ARBOR_COORD_ONE * 10 + ARBOR_COORD_ONE / 4}};
    arbor_path_command path[] = {
        {ARBOR_PATH_MOVE_TO, {ARBOR_COORD_ONE, ARBOR_COORD_ONE * 12}, {0,0}, {0,0}},
        {ARBOR_PATH_QUAD_TO, {ARBOR_COORD_ONE * 5, ARBOR_COORD_ONE * 4},
                             {ARBOR_COORD_ONE * 9, ARBOR_COORD_ONE * 12}, {0,0}},
        {ARBOR_PATH_CUBIC_TO, {ARBOR_COORD_ONE * 11, ARBOR_COORD_ONE * 15},
                              {ARBOR_COORD_ONE * 14, ARBOR_COORD_ONE * 6},
                              {ARBOR_COORD_ONE * 15, ARBOR_COORD_ONE * 12}},
        {ARBOR_PATH_CLOSE, {0,0}, {0,0}, {0,0}}
    };
    uint64_t line_sum;
    uint64_t path_sum;

    memset(pixels, 0, sizeof(pixels));
    if (arbor_raster_surface_init(&surface, pixels, sizeof(pixels), 16u, 16u, 128u) != ARBOR_RENDERER_OK) {
        return fail("surface init");
    }
    if (arbor_renderer_draw_line(surface, line, white) != ARBOR_RENDERER_OK) {
        return fail("antialiased line");
    }
    line_sum = alpha_sum(surface);
    if (line_sum == 0u || line_sum > UINT64_C(16) * UINT64_C(65535)) {
        return fail("line coverage range");
    }

    memset(pixels, 0, sizeof(pixels));
    if (arbor_renderer_stroke_path(
            surface, path, (uint32_t)(sizeof(path) / sizeof(path[0])), white) != ARBOR_RENDERER_OK) {
        return fail("path stroke");
    }
    path_sum = alpha_sum(surface);
    if (path_sum == 0u || path_sum <= line_sum) {
        return fail("curve/path raster coverage");
    }

    puts("PASS: R6/R7 deterministic Q32.32 hairline and curve-path rasterization");
    return 0;
}
