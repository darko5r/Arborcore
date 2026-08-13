#include "renderer_golden_scene.h"

#include <stdint.h>
#include <stdio.h>

int main(void)
{
    static uint8_t pixels[ARBOR_RENDERER_GOLDEN_BYTES];
    int result = arbor_renderer_render_golden_scene(pixels, sizeof(pixels));
    if (result != 0) {
        return result;
    }
    if (fwrite(pixels, 1u, sizeof(pixels), stdout) != sizeof(pixels)) {
        return 100;
    }
    return 0;
}
