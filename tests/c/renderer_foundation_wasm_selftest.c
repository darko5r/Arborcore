#include <stdint.h>
#include "raster_foundation_candidates.h"

__attribute__((visibility("default"))) int renderer_foundation_wasm_selftest(void)
{
    arbor_r0_device_rect_raw rect;
    arbor_r2_pixel_bounds bounds;

    rect.x = -(ARBOR_R0_COORD_ONE / 4);
    rect.y = ARBOR_R0_COORD_ONE / 2;
    rect.width = ARBOR_R0_COORD_ONE * 2;
    rect.height = ARBOR_R0_COORD_ONE;
    if (!arbor_r2_rect_pixel_bounds(rect, 4u, 4u, &bounds)) return 1;
    if (bounds.left != 0 || bounds.top != 0 || bounds.right != 2 || bounds.bottom != 2) return 2;

    rect.x = (ARBOR_R0_COORD_ONE * 48) + (ARBOR_R0_COORD_ONE * 3 / 4);
    rect.y = ARBOR_R0_COORD_ONE * 12;
    rect.width = ARBOR_R0_COORD_ONE / 4;
    rect.height = ARBOR_R0_COORD_ONE;
    if (arbor_r3_rect_coverage_q32(rect, 48, 12) != ARBOR_R0_COVERAGE_Q32_ONE / 4u) return 3;

    rect.x = 0;
    rect.y = 0;
    rect.width = INT64_C(128);
    rect.height = ARBOR_R0_COORD_ONE;
    if (arbor_r3_rect_coverage_q32(rect, 0, 0) != UINT64_C(128)) return 4;
    if (arbor_r3_rect_coverage_q24(rect, 0, 0) != 0u) return 5;

    rect.width = ARBOR_R0_COORD_ONE;
    if (arbor_r3_rect_coverage_q32(rect, 0, 0) != ARBOR_R0_COVERAGE_Q32_ONE) return 6;
    return 0;
}
