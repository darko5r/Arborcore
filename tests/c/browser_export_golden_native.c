#include "arborcore/browser_surface.h"
#include "renderer_golden_scene.h"

#include <stdint.h>
#include <stdio.h>

#define BROWSER_GOLDEN_RGBA8_STRIDE (ARBOR_RENDERER_GOLDEN_WIDTH * 4u)
#define BROWSER_GOLDEN_RGBA8_BYTES \
    (BROWSER_GOLDEN_RGBA8_STRIDE * ARBOR_RENDERER_GOLDEN_HEIGHT)

int main(void)
{
    uint8_t rgba16[ARBOR_RENDERER_GOLDEN_BYTES];
    uint8_t rgba8[BROWSER_GOLDEN_RGBA8_BYTES];

    if (arbor_renderer_render_golden_scene(rgba16, sizeof(rgba16)) != 0) {
        return 1;
    }
    if (arbor_browser_export_rgba8(
            rgba16,
            sizeof(rgba16),
            ARBOR_RENDERER_GOLDEN_STRIDE,
            rgba8,
            sizeof(rgba8),
            BROWSER_GOLDEN_RGBA8_STRIDE,
            ARBOR_RENDERER_GOLDEN_WIDTH,
            ARBOR_RENDERER_GOLDEN_HEIGHT) != ARBOR_BROWSER_OK) {
        return 2;
    }
    if (fwrite(rgba8, 1u, sizeof(rgba8), stdout) != sizeof(rgba8)) {
        return 3;
    }
    return 0;
}
