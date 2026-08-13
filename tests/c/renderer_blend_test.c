#include "arborcore/renderer.h"

#include <stdio.h>

static int fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    return 1;
}

static int same16(arbor_rgba16 a, arbor_rgba16 b)
{
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

int main(void)
{
    static const arbor_rgba8_srgb samples[] = {
        {0u, 0u, 0u, 255u},
        {255u, 255u, 255u, 255u},
        {255u, 0u, 0u, 255u},
        {12u, 34u, 56u, 255u},
        {128u, 64u, 192u, 255u}
    };
    size_t i;
    arbor_rgba16 transparent = {0u, 0u, 0u, 0u};
    arbor_rgba16 opaque_blue = {0u, 0u, 65535u, 65535u};
    arbor_rgba16 half_red = {32768u, 0u, 0u, 32768u};
    arbor_rgba16 expected = {32768u, 0u, 32767u, 65535u};
    arbor_rgba16 result;

    for (i = 0u; i < sizeof(samples) / sizeof(samples[0]); ++i) {
        arbor_rgba16 linear = arbor_rgba16_from_srgb8(samples[i]);
        arbor_rgba8_srgb roundtrip = arbor_rgba16_to_srgb8(linear);
        if (roundtrip.r != samples[i].r || roundtrip.g != samples[i].g ||
            roundtrip.b != samples[i].b || roundtrip.a != samples[i].a) {
            return fail("opaque sRGB8 LUT round-trip");
        }
    }

    if (!same16(arbor_rgba16_source_over(transparent, opaque_blue), opaque_blue)) {
        return fail("transparent source-over identity");
    }
    if (!same16(arbor_rgba16_source_over(opaque_blue, half_red), opaque_blue)) {
        return fail("opaque source-over identity");
    }
    result = arbor_rgba16_source_over(half_red, opaque_blue);
    if (!same16(result, expected)) {
        return fail("linear-light premultiplied source-over vector");
    }
    if (!same16(arbor_rgba16_apply_coverage(opaque_blue, 0u), transparent)) {
        return fail("zero coverage");
    }
    if (!same16(arbor_rgba16_apply_coverage(opaque_blue, ARBOR_COVERAGE_ONE), opaque_blue)) {
        return fail("full coverage");
    }

    puts("PASS: R5 linear-light premultiplied RGBA16 source-over semantics");
    return 0;
}
