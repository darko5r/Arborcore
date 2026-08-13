#include "renderer_golden_scene.h"

#include <stdint.h>

static uint8_t arbor_renderer_wasm_pixels[ARBOR_RENDERER_GOLDEN_BYTES];

int32_t renderer_wasm_render(void)
{
    return (int32_t)arbor_renderer_render_golden_scene(
        arbor_renderer_wasm_pixels,
        sizeof(arbor_renderer_wasm_pixels));
}

uint32_t renderer_wasm_data_ptr(void)
{
    return (uint32_t)(uintptr_t)arbor_renderer_wasm_pixels;
}

uint32_t renderer_wasm_data_size(void)
{
    return (uint32_t)sizeof(arbor_renderer_wasm_pixels);
}
