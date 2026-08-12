#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "raster_foundation_candidates.h"

static int fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    return 1;
}

int main(void)
{
    uint8_t storage[256];
    arbor_r1_surface surface;
    uint64_t offset = UINT64_MAX;
    arbor_r2_pixel_bounds bounds;
    arbor_r0_device_rect_raw rect;
    arbor_r0_coverage_q32 c32;
    arbor_r0_coverage_q24 c24;

    _Static_assert(sizeof(arbor_r0_rgba8) == 4u, "RGBA8 size");
    _Static_assert(sizeof(arbor_r0_rgba16) == 8u, "RGBA16 size");
    _Static_assert(sizeof(arbor_r0_rgba32) == 16u, "RGBA32 size");

    memset(storage, 0, sizeof(storage));
    if (!arbor_r1_surface_prepare(storage, sizeof(storage), 4u, 4u, 40u, 8u, &surface)) {
        return fail("valid RGBA16 padded surface rejected");
    }
    if (!arbor_r1_surface_pixel_offset(surface, 3u, 2u, &offset) || offset != 104u) {
        return fail("surface row-major stride offset mismatch");
    }
    if (arbor_r1_surface_pixel_offset(surface, 4u, 0u, &offset)) {
        return fail("out-of-range surface x accepted");
    }
    if (arbor_r1_surface_prepare(storage, sizeof(storage), 4u, 4u, 31u, 8u, &surface)) {
        return fail("short stride accepted");
    }

    rect.x = -(ARBOR_R0_COORD_ONE / 4);
    rect.y = ARBOR_R0_COORD_ONE / 2;
    rect.width = ARBOR_R0_COORD_ONE * 2;
    rect.height = ARBOR_R0_COORD_ONE;
    if (!arbor_r2_rect_pixel_bounds(rect, 4u, 4u, &bounds)) {
        return fail("valid raster bounds rejected");
    }
    if (bounds.left != 0 || bounds.top != 0 || bounds.right != 2 || bounds.bottom != 2) {
        return fail("conservative half-open raster bounds mismatch");
    }

    rect.x = (ARBOR_R0_COORD_ONE * 48) + (ARBOR_R0_COORD_ONE * 3 / 4);
    rect.y = ARBOR_R0_COORD_ONE * 12;
    rect.width = ARBOR_R0_COORD_ONE / 4;
    rect.height = ARBOR_R0_COORD_ONE;
    c32 = arbor_r3_rect_coverage_q32(rect, 48, 12);
    if (c32 != ARBOR_R0_COVERAGE_Q32_ONE / 4u) {
        return fail("quarter-pixel Q0.32 coverage mismatch");
    }

    rect.x = 0;
    rect.y = 0;
    rect.width = INT64_C(128);
    rect.height = ARBOR_R0_COORD_ONE;
    c32 = arbor_r3_rect_coverage_q32(rect, 0, 0);
    c24 = arbor_r3_rect_coverage_q24(rect, 0, 0);
    if (c32 != UINT64_C(128)) {
        return fail("Q0.32 failed to retain 2^-25 coverage");
    }
    if (c24 != 0u) {
        return fail("Q0.24 precision vector unexpectedly retained 2^-25 coverage");
    }

    rect.x = 0;
    rect.y = 0;
    rect.width = ARBOR_R0_COORD_ONE;
    rect.height = ARBOR_R0_COORD_ONE;
    if (arbor_r3_rect_coverage_q32(rect, 0, 0) != ARBOR_R0_COVERAGE_Q32_ONE ||
        arbor_r3_rect_coverage_q24(rect, 0, 0) != ARBOR_R0_COVERAGE_Q24_ONE) {
        return fail("full coverage mismatch");
    }

    printf("R0_MIN_COLOR_CHANNEL_BITS=%u\n", ARBOR_R0_MIN_COLOR_CHANNEL_BITS);
    printf("R0_MIN_COVERAGE_FRACTION_BITS=%u\n", ARBOR_R0_MIN_COVERAGE_FRACTION_BITS);
    printf("color=RGBA8 channel_bits=8 bytes_per_pixel=%zu eligible=0 role=DISPLAY_EXPORT_REFERENCE\n", sizeof(arbor_r0_rgba8));
    printf("color=RGBA16 channel_bits=16 bytes_per_pixel=%zu eligible=1 role=INTERNAL_CANDIDATE\n", sizeof(arbor_r0_rgba16));
    printf("color=RGBA32 channel_bits=32 bytes_per_pixel=%zu eligible=1 role=HIGH_PRECISION_REFERENCE\n", sizeof(arbor_r0_rgba32));
    printf("coverage=Q0.24 fraction_bits=24 storage_bits=32 eligible=1\n");
    printf("coverage=Q0.32 fraction_bits=32 storage_bits=64 eligible=1\n");
    printf("R1_SURFACE_LAYOUT=ROW_MAJOR_EXPLICIT_STRIDE\n");
    printf("R2_PIXEL_CELL=HALF_OPEN_[x,x+1)x[y,y+1)\n");
    printf("R3_ANALYTIC_RECT_COVERAGE=PASS\n");
    printf("R0_R3_CANDIDATE_PROPERTIES=PASS\n");
    return 0;
}
