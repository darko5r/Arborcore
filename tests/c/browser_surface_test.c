#include "arborcore/browser_surface.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define EQUIVALENCE_WIDTH UINT32_C(65536)
#define EQUIVALENCE_RGBA16_BYTES ((size_t)EQUIVALENCE_WIDTH * 8u)
#define EQUIVALENCE_RGBA8_BYTES ((size_t)EQUIVALENCE_WIDTH * 4u)

static uint8_t equivalence_rgba16[EQUIVALENCE_RGBA16_BYTES];
static uint8_t equivalence_rgba8[EQUIVALENCE_RGBA8_BYTES];

static int fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    return 1;
}

static void store_u16_le(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)(value & UINT16_C(0xff));
    p[1] = (uint8_t)(value >> 8);
}

static void store_rgba16(uint8_t *p, arbor_rgba16 color)
{
    store_u16_le(p + 0, color.r);
    store_u16_le(p + 2, color.g);
    store_u16_le(p + 4, color.b);
    store_u16_le(p + 6, color.a);
}

static int compare_export_pixel(uint32_t index, arbor_rgba16 color)
{
    arbor_rgba8_srgb expected = arbor_rgba16_to_srgb8(color);
    const uint8_t *actual = equivalence_rgba8 + ((uint64_t)index * UINT64_C(4));

    return actual[0] == expected.r &&
        actual[1] == expected.g &&
        actual[2] == expected.b &&
        actual[3] == expected.a;
}

static int verify_exhaustive_opaque_linear_mapping(void)
{
    uint32_t value;

    for (value = 0u; value <= UINT16_MAX; ++value) {
        arbor_rgba16 color = {
            (uint16_t)value,
            (uint16_t)value,
            (uint16_t)value,
            UINT16_MAX
        };
        store_rgba16(equivalence_rgba16 + ((uint64_t)value * UINT64_C(8)), color);
    }

    if (arbor_browser_export_rgba8(
            equivalence_rgba16,
            sizeof(equivalence_rgba16),
            (uint64_t)EQUIVALENCE_WIDTH * UINT64_C(8),
            equivalence_rgba8,
            sizeof(equivalence_rgba8),
            (uint64_t)EQUIVALENCE_WIDTH * UINT64_C(4),
            EQUIVALENCE_WIDTH,
            1u) != ARBOR_BROWSER_OK) {
        return fail("exhaustive opaque export");
    }

    for (value = 0u; value <= UINT16_MAX; ++value) {
        arbor_rgba16 color = {
            (uint16_t)value,
            (uint16_t)value,
            (uint16_t)value,
            UINT16_MAX
        };
        if (!compare_export_pixel(value, color)) {
            return fail("exhaustive linear16 to sRGB8 equivalence");
        }
    }
    return 0;
}

static int verify_all_alpha_structured_equivalence(void)
{
    uint32_t seed = UINT32_C(0x6d2b79f5);
    uint32_t alpha;

    for (alpha = 0u; alpha <= UINT16_MAX; ++alpha) {
        uint32_t range = alpha + UINT32_C(1);
        arbor_rgba16 color;

        seed = seed * UINT32_C(1664525) + UINT32_C(1013904223);
        color.r = (uint16_t)(seed % range);
        seed = seed * UINT32_C(1664525) + UINT32_C(1013904223);
        color.g = (uint16_t)(seed % range);
        seed = seed * UINT32_C(1664525) + UINT32_C(1013904223);
        color.b = (uint16_t)(seed % range);
        color.a = (uint16_t)alpha;
        store_rgba16(equivalence_rgba16 + ((uint64_t)alpha * UINT64_C(8)), color);
    }

    if (arbor_browser_export_rgba8(
            equivalence_rgba16,
            sizeof(equivalence_rgba16),
            (uint64_t)EQUIVALENCE_WIDTH * UINT64_C(8),
            equivalence_rgba8,
            sizeof(equivalence_rgba8),
            (uint64_t)EQUIVALENCE_WIDTH * UINT64_C(4),
            EQUIVALENCE_WIDTH,
            1u) != ARBOR_BROWSER_OK) {
        return fail("all-alpha structured export");
    }

    seed = UINT32_C(0x6d2b79f5);
    for (alpha = 0u; alpha <= UINT16_MAX; ++alpha) {
        uint32_t range = alpha + UINT32_C(1);
        arbor_rgba16 color;

        seed = seed * UINT32_C(1664525) + UINT32_C(1013904223);
        color.r = (uint16_t)(seed % range);
        seed = seed * UINT32_C(1664525) + UINT32_C(1013904223);
        color.g = (uint16_t)(seed % range);
        seed = seed * UINT32_C(1664525) + UINT32_C(1013904223);
        color.b = (uint16_t)(seed % range);
        color.a = (uint16_t)alpha;
        if (!compare_export_pixel(alpha, color)) {
            return fail("all-alpha structured renderer equivalence");
        }
    }
    return 0;
}

int main(void)
{
    arbor_browser_export_layout layout = {0};
    arbor_browser_export_layout unchanged = {7u, 8u, 9u, 10u, 11u, 12u};
    uint8_t rgba16[2u * 24u];
    uint8_t rgba8[2u * 16u];
    arbor_rgba16 red;
    arbor_rgba16 semi;
    arbor_rgba8_srgb expected;
    uint32_t y;

    if (arbor_browser_export_layout_make(3u, 2u, 24u, 16u, &layout) != ARBOR_BROWSER_OK ||
        layout.rgba16_required_bytes != 48u || layout.rgba8_required_bytes != 28u) {
        return fail("layout calculation");
    }
    if (arbor_browser_export_layout_make(3u, 2u, 23u, 16u, &unchanged) != ARBOR_BROWSER_INVALID_ARGUMENT ||
        unchanged.width != 7u || unchanged.rgba8_required_bytes != 12u) {
        return fail("transactional invalid stride");
    }
    if (arbor_browser_export_layout_make(0u, 2u, 0u, 0u, &unchanged) != ARBOR_BROWSER_INVALID_ARGUMENT) {
        return fail("zero-width rejection");
    }

    memset(rgba16, 0xa5, sizeof(rgba16));
    memset(rgba8, 0x5a, sizeof(rgba8));
    red = arbor_rgba16_from_srgb8((arbor_rgba8_srgb){255u, 0u, 0u, 255u});
    semi = arbor_rgba16_from_srgb8((arbor_rgba8_srgb){64u, 128u, 192u, 128u});

    for (y = 0u; y < 2u; ++y) {
        uint8_t *row = rgba16 + ((uint64_t)y * UINT64_C(24));
        arbor_rgba16 colors[3] = {
            {0u, 0u, 0u, 0u}, red, semi
        };
        uint32_t x;
        for (x = 0u; x < 3u; ++x) {
            store_rgba16(row + ((uint64_t)x * UINT64_C(8)), colors[x]);
        }
    }

    if (arbor_browser_export_rgba8(
            rgba16, sizeof(rgba16), 24u,
            rgba8, sizeof(rgba8), 16u,
            3u, 2u) != ARBOR_BROWSER_OK) {
        return fail("RGBA8 export");
    }
    if (rgba8[0] != 0u || rgba8[1] != 0u || rgba8[2] != 0u || rgba8[3] != 0u ||
        rgba8[4] != 255u || rgba8[5] != 0u || rgba8[6] != 0u || rgba8[7] != 255u) {
        return fail("transparent/opaque export values");
    }
    expected = arbor_rgba16_to_srgb8(semi);
    if (rgba8[8] != expected.r || rgba8[9] != expected.g ||
        rgba8[10] != expected.b || rgba8[11] != expected.a) {
        return fail("semi-transparent export value");
    }
    if (rgba8[12] != 0x5au || rgba8[13] != 0x5au || rgba8[14] != 0x5au || rgba8[15] != 0x5au ||
        rgba8[28] != 0x5au || rgba8[29] != 0x5au || rgba8[30] != 0x5au || rgba8[31] != 0x5au) {
        return fail("destination row padding modified");
    }
    if (arbor_browser_export_rgba8(
            rgba16, sizeof(rgba16), 24u,
            rgba8, 27u, 16u,
            3u, 2u) != ARBOR_BROWSER_BUFFER_TOO_SMALL) {
        return fail("small destination rejection");
    }

    if (verify_exhaustive_opaque_linear_mapping() != 0 ||
        verify_all_alpha_structured_equivalence() != 0) {
        return 1;
    }

    puts("PASS: B0/B1 checked browser export layout and deterministic RGBA8 conversion");
    puts("PASS: B1 accelerated export is equivalent to frozen renderer conversion");
    return 0;
}
