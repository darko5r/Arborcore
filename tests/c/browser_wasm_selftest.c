#include "arborcore/browser_surface.h"
#include "renderer_golden_scene.h"

#include <stdint.h>

#define BROWSER_GOLDEN_RGBA8_STRIDE (ARBOR_RENDERER_GOLDEN_WIDTH * 4u)
#define BROWSER_GOLDEN_RGBA8_BYTES \
    (BROWSER_GOLDEN_RGBA8_STRIDE * ARBOR_RENDERER_GOLDEN_HEIGHT)
#define BROWSER_OPAQUE_WIDTH 2u
#define BROWSER_OPAQUE_HEIGHT 2u
#define BROWSER_OPAQUE_RGBA16_STRIDE 16u
#define BROWSER_OPAQUE_RGBA16_BYTES 32u
#define BROWSER_OPAQUE_RGBA8_STRIDE 8u
#define BROWSER_OPAQUE_RGBA8_BYTES 16u

static uint8_t browser_rgba16[ARBOR_RENDERER_GOLDEN_BYTES];
static uint8_t browser_rgba8[BROWSER_GOLDEN_RGBA8_BYTES];
static uint8_t browser_opaque_rgba16[BROWSER_OPAQUE_RGBA16_BYTES];
static uint8_t browser_opaque_rgba8[BROWSER_OPAQUE_RGBA8_BYTES];

static int prepare_opaque_probe(void)
{
    arbor_raster_surface surface;
    const arbor_rgba8_srgb colors[4] = {
        {255u, 0u, 0u, 255u},
        {0u, 255u, 0u, 255u},
        {0u, 0u, 255u, 255u},
        {255u, 255u, 255u, 255u}
    };
    uint32_t i;

    if (arbor_raster_surface_init(
            &surface,
            browser_opaque_rgba16,
            sizeof(browser_opaque_rgba16),
            BROWSER_OPAQUE_WIDTH,
            BROWSER_OPAQUE_HEIGHT,
            BROWSER_OPAQUE_RGBA16_STRIDE) != ARBOR_RENDERER_OK) {
        return 20;
    }
    for (i = 0u; i < 4u; ++i) {
        if (arbor_raster_surface_set_pixel(
                surface,
                i % 2u,
                i / 2u,
                arbor_rgba16_from_srgb8(colors[i])) != ARBOR_RENDERER_OK) {
            return 21;
        }
    }
    if (arbor_browser_export_rgba8(
            browser_opaque_rgba16,
            sizeof(browser_opaque_rgba16),
            BROWSER_OPAQUE_RGBA16_STRIDE,
            browser_opaque_rgba8,
            sizeof(browser_opaque_rgba8),
            BROWSER_OPAQUE_RGBA8_STRIDE,
            BROWSER_OPAQUE_WIDTH,
            BROWSER_OPAQUE_HEIGHT) != ARBOR_BROWSER_OK) {
        return 22;
    }
    return 0;
}

int32_t browser_wasm_prepare(void)
{
    int result = arbor_renderer_render_golden_scene(browser_rgba16, sizeof(browser_rgba16));
    if (result != 0) return (int32_t)result;
    if (arbor_browser_export_rgba8(
            browser_rgba16,
            sizeof(browser_rgba16),
            ARBOR_RENDERER_GOLDEN_STRIDE,
            browser_rgba8,
            sizeof(browser_rgba8),
            BROWSER_GOLDEN_RGBA8_STRIDE,
            ARBOR_RENDERER_GOLDEN_WIDTH,
            ARBOR_RENDERER_GOLDEN_HEIGHT) != ARBOR_BROWSER_OK) {
        return 10;
    }
    return (int32_t)prepare_opaque_probe();
}

#define EXPORT_PTR_FN(name, array) \
    uint32_t name(void) { return (uint32_t)(uintptr_t)(array); }
#define EXPORT_SIZE_FN(name, array) \
    uint32_t name(void) { return (uint32_t)sizeof(array); }

EXPORT_PTR_FN(browser_wasm_rgba16_ptr, browser_rgba16)
EXPORT_SIZE_FN(browser_wasm_rgba16_size, browser_rgba16)
EXPORT_PTR_FN(browser_wasm_rgba8_ptr, browser_rgba8)
EXPORT_SIZE_FN(browser_wasm_rgba8_size, browser_rgba8)
EXPORT_PTR_FN(browser_wasm_opaque_rgba8_ptr, browser_opaque_rgba8)
EXPORT_SIZE_FN(browser_wasm_opaque_rgba8_size, browser_opaque_rgba8)

uint32_t browser_wasm_width(void) { return ARBOR_RENDERER_GOLDEN_WIDTH; }
uint32_t browser_wasm_height(void) { return ARBOR_RENDERER_GOLDEN_HEIGHT; }
uint32_t browser_wasm_rgba8_stride(void) { return BROWSER_GOLDEN_RGBA8_STRIDE; }
uint32_t browser_wasm_opaque_width(void) { return BROWSER_OPAQUE_WIDTH; }
uint32_t browser_wasm_opaque_height(void) { return BROWSER_OPAQUE_HEIGHT; }
uint32_t browser_wasm_opaque_rgba8_stride(void) { return BROWSER_OPAQUE_RGBA8_STRIDE; }
