#ifndef ARBORCORE_RENDERER_GOLDEN_SCENE_H
#define ARBORCORE_RENDERER_GOLDEN_SCENE_H

#include "arborcore/renderer.h"

#define ARBOR_RENDERER_GOLDEN_WIDTH 16u
#define ARBOR_RENDERER_GOLDEN_HEIGHT 16u
#define ARBOR_RENDERER_GOLDEN_STRIDE (ARBOR_RENDERER_GOLDEN_WIDTH * 8u)
#define ARBOR_RENDERER_GOLDEN_BYTES \
    (ARBOR_RENDERER_GOLDEN_STRIDE * ARBOR_RENDERER_GOLDEN_HEIGHT)

static int arbor_renderer_render_golden_scene(uint8_t *pixels, uint64_t bytes)
{
    arbor_raster_surface surface;
    arbor_rgba16 clear = {0u, 0u, 0u, 0u};
    arbor_rgba16 red = arbor_rgba16_from_srgb8((arbor_rgba8_srgb){255u, 32u, 16u, 220u});
    arbor_rgba16 green = arbor_rgba16_from_srgb8((arbor_rgba8_srgb){20u, 230u, 90u, 160u});
    arbor_rgba16 blue = arbor_rgba16_from_srgb8((arbor_rgba8_srgb){40u, 90u, 255u, 255u});
    arbor_rgba16 yellow = arbor_rgba16_from_srgb8((arbor_rgba8_srgb){255u, 210u, 30u, 220u});
    arbor_device_rect rect1 = {
        ARBOR_COORD_ONE / 4,
        ARBOR_COORD_ONE / 2,
        ARBOR_COORD_ONE * 7 + ARBOR_COORD_ONE / 2,
        ARBOR_COORD_ONE * 5 + ARBOR_COORD_ONE / 4
    };
    arbor_device_rect rect2 = {
        ARBOR_COORD_ONE * 5 + ARBOR_COORD_ONE / 8,
        ARBOR_COORD_ONE * 3 + ARBOR_COORD_ONE / 4,
        ARBOR_COORD_ONE * 8 + ARBOR_COORD_ONE / 2,
        ARBOR_COORD_ONE * 6 + ARBOR_COORD_ONE / 2
    };
    arbor_line line = {
        {ARBOR_COORD_ONE / 2, ARBOR_COORD_ONE * 14 + ARBOR_COORD_ONE / 4},
        {ARBOR_COORD_ONE * 15 - ARBOR_COORD_ONE / 4, ARBOR_COORD_ONE * 7 + ARBOR_COORD_ONE / 2}
    };
    arbor_path_command path[] = {
        {ARBOR_PATH_MOVE_TO, {ARBOR_COORD_ONE, ARBOR_COORD_ONE * 10}, {0,0}, {0,0}},
        {ARBOR_PATH_QUAD_TO, {ARBOR_COORD_ONE * 4, ARBOR_COORD_ONE * 5},
                             {ARBOR_COORD_ONE * 8, ARBOR_COORD_ONE * 11}, {0,0}},
        {ARBOR_PATH_CUBIC_TO, {ARBOR_COORD_ONE * 10, ARBOR_COORD_ONE * 15},
                              {ARBOR_COORD_ONE * 13, ARBOR_COORD_ONE * 4},
                              {ARBOR_COORD_ONE * 15, ARBOR_COORD_ONE * 10}},
        {ARBOR_PATH_CLOSE, {0,0}, {0,0}, {0,0}}
    };

    if (arbor_raster_surface_init(
            &surface,
            pixels,
            bytes,
            ARBOR_RENDERER_GOLDEN_WIDTH,
            ARBOR_RENDERER_GOLDEN_HEIGHT,
            ARBOR_RENDERER_GOLDEN_STRIDE) != ARBOR_RENDERER_OK) return 1;
    if (arbor_renderer_clear(surface, clear) != ARBOR_RENDERER_OK) return 2;
    if (arbor_renderer_fill_rect(surface, rect1, red) != ARBOR_RENDERER_OK) return 3;
    if (arbor_renderer_fill_rect(surface, rect2, green) != ARBOR_RENDERER_OK) return 4;
    if (arbor_renderer_draw_line(surface, line, blue) != ARBOR_RENDERER_OK) return 5;
    if (arbor_renderer_stroke_path(
            surface, path, (uint32_t)(sizeof(path) / sizeof(path[0])), yellow) != ARBOR_RENDERER_OK) return 6;
    return 0;
}

#endif
