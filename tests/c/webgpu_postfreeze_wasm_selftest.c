#include "arborcore/browser_surface.h"

#include <stdint.h>

#define OPT_VECTOR_WIDTH 256u
#define OPT_VECTOR_HEIGHT 320u
#define OPT_VECTOR_RGBA16_STRIDE (OPT_VECTOR_WIDTH * 8u)
#define OPT_VECTOR_RGBA8_STRIDE (OPT_VECTOR_WIDTH * 4u)
#define OPT_VECTOR_RGBA16_BYTES (OPT_VECTOR_RGBA16_STRIDE * OPT_VECTOR_HEIGHT)
#define OPT_VECTOR_RGBA8_BYTES (OPT_VECTOR_RGBA8_STRIDE * OPT_VECTOR_HEIGHT)

#define OPT_PERF_WIDTH 640u
#define OPT_PERF_HEIGHT 360u
#define OPT_PERF_RGBA16_STRIDE (OPT_PERF_WIDTH * 8u)
#define OPT_PERF_RGBA8_STRIDE (OPT_PERF_WIDTH * 4u)
#define OPT_PERF_RGBA16_BYTES (OPT_PERF_RGBA16_STRIDE * OPT_PERF_HEIGHT)
#define OPT_PERF_RGBA8_BYTES (OPT_PERF_RGBA8_STRIDE * OPT_PERF_HEIGHT)

static uint8_t vector_rgba16[OPT_VECTOR_RGBA16_BYTES];
static uint8_t vector_rgba8[OPT_VECTOR_RGBA8_BYTES];
static uint8_t perf_rgba16[OPT_PERF_RGBA16_BYTES];
static uint8_t perf_rgba8[OPT_PERF_RGBA8_BYTES];

static void store_u16_le(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)(value & UINT16_C(0x00ff));
    p[1] = (uint8_t)(value >> 8);
}

static void store_pixel(uint8_t *p, uint16_t r, uint16_t g, uint16_t b, uint16_t a)
{
    store_u16_le(p + 0, r);
    store_u16_le(p + 2, g);
    store_u16_le(p + 4, b);
    store_u16_le(p + 6, a);
}

static void fill_vectors(void)
{
    uint32_t i;

    /* First 65,536 pixels exhaust every opaque linear16 value. */
    for (i = 0u; i < UINT32_C(65536); ++i) {
        uint8_t *p = vector_rgba16 + ((uint64_t)i * UINT64_C(8));
        uint16_t value = (uint16_t)i;
        store_pixel(p, value, value, value, UINT16_MAX);
    }

    /* Remaining 16,384 pixels cover transparent, alpha-edge and deterministic mixed-alpha cases. */
    for (i = 0u; i < UINT32_C(16384); ++i) {
        uint32_t pixel = UINT32_C(65536) + i;
        uint8_t *p = vector_rgba16 + ((uint64_t)pixel * UINT64_C(8));
        uint16_t alpha;
        uint16_t r;
        uint16_t g;
        uint16_t b;

        if (i == 0u) {
            store_pixel(p, UINT16_MAX, UINT16_MAX, UINT16_MAX, 0u);
            continue;
        }
        if (i == 1u) {
            store_pixel(p, 0u, 1u, 1u, 1u);
            continue;
        }
        if (i == 2u) {
            store_pixel(p, UINT16_C(32767), UINT16_C(32768), UINT16_C(32767), UINT16_C(32768));
            continue;
        }
        if (i == 3u) {
            store_pixel(p, UINT16_C(65534), UINT16_C(1), UINT16_C(32767), UINT16_C(65534));
            continue;
        }

        alpha = (uint16_t)(((uint32_t)i * UINT32_C(40503) + UINT32_C(17)) & UINT32_C(0xffff));
        if (alpha == 0u) alpha = UINT16_C(65534);
        r = (uint16_t)(((uint32_t)i * UINT32_C(97) + UINT32_C(13)) % ((uint32_t)alpha + 1u));
        g = (uint16_t)(((uint32_t)i * UINT32_C(193) + UINT32_C(29)) % ((uint32_t)alpha + 1u));
        b = (uint16_t)(((uint32_t)i * UINT32_C(389) + UINT32_C(47)) % ((uint32_t)alpha + 1u));
        store_pixel(p, r, g, b, alpha);
    }
}

static void fill_perf(void)
{
    uint32_t y;
    for (y = 0u; y < OPT_PERF_HEIGHT; ++y) {
        uint32_t x;
        for (x = 0u; x < OPT_PERF_WIDTH; ++x) {
            uint32_t i = y * OPT_PERF_WIDTH + x;
            uint8_t *p = perf_rgba16 + ((uint64_t)i * UINT64_C(8));
            uint16_t alpha = (uint16_t)(((x * UINT32_C(257)) + (y * UINT32_C(509)) + UINT32_C(1021)) & UINT32_C(0xffff));
            uint16_t r;
            uint16_t g;
            uint16_t b;
            if (alpha < UINT16_C(1024)) alpha = (uint16_t)(alpha + UINT16_C(1024));
            r = (uint16_t)(((x * UINT32_C(911)) + (y * UINT32_C(37))) % ((uint32_t)alpha + 1u));
            g = (uint16_t)(((x * UINT32_C(53)) + (y * UINT32_C(1237))) % ((uint32_t)alpha + 1u));
            b = (uint16_t)(((x * UINT32_C(701)) + (y * UINT32_C(313))) % ((uint32_t)alpha + 1u));
            store_pixel(p, r, g, b, alpha);
        }
    }
}

int32_t postfreeze_wasm_prepare_vectors(void)
{
    fill_vectors();
    return arbor_browser_export_rgba8(
        vector_rgba16,
        sizeof(vector_rgba16),
        OPT_VECTOR_RGBA16_STRIDE,
        vector_rgba8,
        sizeof(vector_rgba8),
        OPT_VECTOR_RGBA8_STRIDE,
        OPT_VECTOR_WIDTH,
        OPT_VECTOR_HEIGHT) == ARBOR_BROWSER_OK ? 0 : 1;
}

int32_t postfreeze_wasm_prepare_perf(void)
{
    fill_perf();
    return arbor_browser_export_rgba8(
        perf_rgba16,
        sizeof(perf_rgba16),
        OPT_PERF_RGBA16_STRIDE,
        perf_rgba8,
        sizeof(perf_rgba8),
        OPT_PERF_RGBA8_STRIDE,
        OPT_PERF_WIDTH,
        OPT_PERF_HEIGHT) == ARBOR_BROWSER_OK ? 0 : 1;
}

int32_t postfreeze_wasm_export_perf(void)
{
    return arbor_browser_export_rgba8(
        perf_rgba16,
        sizeof(perf_rgba16),
        OPT_PERF_RGBA16_STRIDE,
        perf_rgba8,
        sizeof(perf_rgba8),
        OPT_PERF_RGBA8_STRIDE,
        OPT_PERF_WIDTH,
        OPT_PERF_HEIGHT) == ARBOR_BROWSER_OK ? 0 : 1;
}

#define PTR_FN(name, array) uint32_t name(void) { return (uint32_t)(uintptr_t)(array); }
#define SIZE_FN(name, array) uint32_t name(void) { return (uint32_t)sizeof(array); }

PTR_FN(postfreeze_wasm_vector_rgba16_ptr, vector_rgba16)
SIZE_FN(postfreeze_wasm_vector_rgba16_size, vector_rgba16)
PTR_FN(postfreeze_wasm_vector_rgba8_ptr, vector_rgba8)
SIZE_FN(postfreeze_wasm_vector_rgba8_size, vector_rgba8)
PTR_FN(postfreeze_wasm_perf_rgba16_ptr, perf_rgba16)
SIZE_FN(postfreeze_wasm_perf_rgba16_size, perf_rgba16)
PTR_FN(postfreeze_wasm_perf_rgba8_ptr, perf_rgba8)
SIZE_FN(postfreeze_wasm_perf_rgba8_size, perf_rgba8)

uint32_t postfreeze_wasm_vector_width(void) { return OPT_VECTOR_WIDTH; }
uint32_t postfreeze_wasm_vector_height(void) { return OPT_VECTOR_HEIGHT; }
uint32_t postfreeze_wasm_vector_rgba16_stride(void) { return OPT_VECTOR_RGBA16_STRIDE; }
uint32_t postfreeze_wasm_vector_rgba8_stride(void) { return OPT_VECTOR_RGBA8_STRIDE; }
uint32_t postfreeze_wasm_perf_width(void) { return OPT_PERF_WIDTH; }
uint32_t postfreeze_wasm_perf_height(void) { return OPT_PERF_HEIGHT; }
uint32_t postfreeze_wasm_perf_rgba16_stride(void) { return OPT_PERF_RGBA16_STRIDE; }
uint32_t postfreeze_wasm_perf_rgba8_stride(void) { return OPT_PERF_RGBA8_STRIDE; }
