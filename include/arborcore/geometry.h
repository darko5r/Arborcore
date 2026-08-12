#ifndef ARBORCORE_GEOMETRY_H
#define ARBORCORE_GEOMETRY_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ARBOR_GEOMETRY_NUMERICAL_CONTRACT_MAJOR 1u
#define ARBOR_GEOMETRY_NUMERICAL_CONTRACT_MINOR 0u

#define ARBOR_COORD_FRACTION_BITS 32u
#define ARBOR_COORD_ONE INT64_C(4294967296)
#define ARBOR_COORD_HALF INT64_C(2147483648)

/*
 * Production geometry coordinate selected by G0-G1 qualification.
 * The numerical contract is signed Q32.32 stored in int64_t.
 */
typedef int64_t arbor_coord;

typedef enum arbor_geometry_status {
    ARBOR_GEOMETRY_OK = 0,
    ARBOR_GEOMETRY_INVALID_ARGUMENT,
    ARBOR_GEOMETRY_OVERFLOW,
    ARBOR_GEOMETRY_SINGULAR
} arbor_geometry_status;

typedef enum arbor_rounding_mode {
    ARBOR_ROUND_TOWARD_ZERO = 0,
    ARBOR_ROUND_FLOOR,
    ARBOR_ROUND_CEIL,
    ARBOR_ROUND_NEAREST_EVEN
} arbor_rounding_mode;

typedef struct arbor_point {
    arbor_coord x;
    arbor_coord y;
} arbor_point;

typedef struct arbor_size {
    arbor_coord width;
    arbor_coord height;
} arbor_size;

typedef struct arbor_rect {
    arbor_coord x;
    arbor_coord y;
    arbor_coord width;
    arbor_coord height;
} arbor_rect;

typedef struct arbor_line {
    arbor_point start;
    arbor_point end;
} arbor_line;

typedef struct arbor_insets {
    arbor_coord top;
    arbor_coord right;
    arbor_coord bottom;
    arbor_coord left;
} arbor_insets;

/*
 * 2-D affine matrix:
 *
 *     | a c tx |
 *     | b d ty |
 *     | 0 0  1 |
 *
 * Composition is explicit: arbor_affine_compose(left, right) produces
 * left o right, meaning right is applied first and left second.
 */
typedef struct arbor_affine {
    arbor_coord a;
    arbor_coord b;
    arbor_coord c;
    arbor_coord d;
    arbor_coord tx;
    arbor_coord ty;
} arbor_affine;

/* Exact positive rational used for DPR/browser-zoom/device scaling. */
typedef struct arbor_device_scale {
    uint64_t numerator;
    uint64_t denominator;
} arbor_device_scale;

/* Distinct coordinate-space types prevent implicit logical/device mixing. */
typedef struct arbor_logical_point {
    arbor_coord x;
    arbor_coord y;
} arbor_logical_point;

typedef struct arbor_device_point {
    arbor_coord x;
    arbor_coord y;
} arbor_device_point;

typedef struct arbor_logical_rect {
    arbor_coord x;
    arbor_coord y;
    arbor_coord width;
    arbor_coord height;
} arbor_logical_rect;

typedef struct arbor_device_rect {
    arbor_coord x;
    arbor_coord y;
    arbor_coord width;
    arbor_coord height;
} arbor_device_rect;

typedef struct arbor_device_map {
    arbor_device_scale scale;
    arbor_coord origin_x;
    arbor_coord origin_y;
} arbor_device_map;

/* G2: checked Q32.32 scalar arithmetic. Outputs are transactional on failure. */
arbor_geometry_status arbor_coord_from_integer(int64_t integer, arbor_coord *out);
arbor_geometry_status arbor_coord_from_ratio(
    int64_t numerator,
    uint64_t denominator,
    arbor_rounding_mode mode,
    arbor_coord *out);
arbor_geometry_status arbor_coord_to_integer(
    arbor_coord value,
    arbor_rounding_mode mode,
    int64_t *out);
arbor_geometry_status arbor_coord_add(arbor_coord left, arbor_coord right, arbor_coord *out);
arbor_geometry_status arbor_coord_sub(arbor_coord left, arbor_coord right, arbor_coord *out);
arbor_geometry_status arbor_coord_neg(arbor_coord value, arbor_coord *out);
arbor_geometry_status arbor_coord_abs(arbor_coord value, arbor_coord *out);
arbor_geometry_status arbor_coord_mul(arbor_coord left, arbor_coord right, arbor_coord *out);
arbor_geometry_status arbor_coord_div(arbor_coord numerator, arbor_coord denominator, arbor_coord *out);

/* G2: points, sizes, rectangles and lines. Rectangles are half-open. */
arbor_geometry_status arbor_size_make(arbor_coord width, arbor_coord height, arbor_size *out);
arbor_geometry_status arbor_rect_make(
    arbor_coord x,
    arbor_coord y,
    arbor_coord width,
    arbor_coord height,
    arbor_rect *out);
arbor_geometry_status arbor_rect_from_edges(
    arbor_coord left,
    arbor_coord top,
    arbor_coord right,
    arbor_coord bottom,
    arbor_rect *out);
arbor_geometry_status arbor_point_translate(arbor_point point, arbor_point delta, arbor_point *out);
arbor_geometry_status arbor_rect_translate(arbor_rect rect, arbor_point delta, arbor_rect *out);
bool arbor_rect_contains_point(arbor_rect rect, arbor_point point);
arbor_geometry_status arbor_rect_intersection(
    arbor_rect left,
    arbor_rect right,
    arbor_rect *out,
    bool *intersects);
arbor_geometry_status arbor_rect_union(arbor_rect left, arbor_rect right, arbor_rect *out);
arbor_geometry_status arbor_line_bounds(arbor_line line, arbor_rect *out);

/* G3: deterministic affine transforms and clipping. */
arbor_affine arbor_affine_identity(void);
arbor_affine arbor_affine_translation(arbor_coord tx, arbor_coord ty);
arbor_affine arbor_affine_scale(arbor_coord sx, arbor_coord sy);
arbor_geometry_status arbor_affine_rotation_turns(arbor_coord turns, arbor_affine *out);
arbor_geometry_status arbor_affine_compose(arbor_affine left, arbor_affine right, arbor_affine *out);
arbor_geometry_status arbor_affine_invert(arbor_affine transform, arbor_affine *out);
arbor_geometry_status arbor_affine_transform_point(arbor_affine transform, arbor_point point, arbor_point *out);
arbor_geometry_status arbor_affine_transform_rect_bounds(arbor_affine transform, arbor_rect rect, arbor_rect *out);
arbor_geometry_status arbor_clip_rect(arbor_rect current_clip, arbor_rect requested_clip, arbor_rect *out, bool *visible);

/* G4: exact rational logical -> device mapping with nearest-even reduction. */
arbor_geometry_status arbor_device_scale_make(uint64_t numerator, uint64_t denominator, arbor_device_scale *out);
arbor_geometry_status arbor_device_scale_compose(arbor_device_scale left, arbor_device_scale right, arbor_device_scale *out);
arbor_geometry_status arbor_device_map_make(
    arbor_device_scale scale,
    arbor_coord origin_x,
    arbor_coord origin_y,
    arbor_device_map *out);
arbor_geometry_status arbor_device_map_point(
    arbor_device_map map,
    arbor_logical_point logical,
    arbor_device_point *out);
arbor_geometry_status arbor_device_map_rect_bounds(
    arbor_device_map map,
    arbor_logical_rect logical,
    arbor_device_rect *out);

#ifdef __cplusplus
}
#endif

#endif
