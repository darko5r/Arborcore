#ifndef ARBORCORE_TESTS_GEOMETRY_SEMANTIC_VECTORS_H
#define ARBORCORE_TESTS_GEOMETRY_SEMANTIC_VECTORS_H

#include <stdint.h>

#include <arborcore/geometry.h>

static arbor_coord arbor_geometry_vector_ci(int64_t value)
{
    arbor_coord out = 0;
    if (arbor_coord_from_integer(value, &out) != ARBOR_GEOMETRY_OK) {
        return 0;
    }
    return out;
}

static uint64_t arbor_geometry_vector_distance(arbor_coord left, arbor_coord right)
{
    if (left >= right) {
        return (uint64_t)left - (uint64_t)right;
    }
    return (uint64_t)right - (uint64_t)left;
}

static int arbor_geometry_semantic_vectors(void)
{
    arbor_coord value;
    arbor_coord half;
    arbor_coord angle;
    arbor_affine rotation;
    arbor_affine transform;
    arbor_affine inverse;
    arbor_affine product;
    arbor_device_scale scale;
    arbor_device_map map;
    arbor_device_point device;
    arbor_rect left;
    arbor_rect right;
    arbor_rect intersection;
    bool visible;

    if (arbor_coord_from_ratio(1, 2, ARBOR_ROUND_NEAREST_EVEN, &half) != ARBOR_GEOMETRY_OK ||
        half != ARBOR_COORD_HALF) return 1;
    if (arbor_coord_from_ratio(1, UINT64_C(8589934592), ARBOR_ROUND_NEAREST_EVEN, &value) != ARBOR_GEOMETRY_OK ||
        value != 0) return 2;
    if (arbor_coord_from_ratio(3, UINT64_C(8589934592), ARBOR_ROUND_NEAREST_EVEN, &value) != ARBOR_GEOMETRY_OK ||
        value != 2) return 3;
    if (arbor_coord_div(ARBOR_COORD_ONE, INT64_C(3) * ARBOR_COORD_ONE, &value) != ARBOR_GEOMETRY_OK ||
        value != INT64_C(1431655765)) return 4;

    if (arbor_coord_from_ratio(1, 8, ARBOR_ROUND_NEAREST_EVEN, &angle) != ARBOR_GEOMETRY_OK) return 5;
    if (arbor_affine_rotation_turns(angle, &rotation) != ARBOR_GEOMETRY_OK) return 6;
    if (arbor_geometry_vector_distance(rotation.a, INT64_C(3037000500)) > UINT64_C(32) ||
        arbor_geometry_vector_distance(rotation.b, INT64_C(3037000500)) > UINT64_C(32)) return 7;

    if (arbor_affine_compose(
            arbor_affine_translation(arbor_geometry_vector_ci(5), arbor_geometry_vector_ci(-7)),
            arbor_affine_scale(arbor_geometry_vector_ci(2), arbor_geometry_vector_ci(3)),
            &transform) != ARBOR_GEOMETRY_OK) return 8;
    if (arbor_affine_invert(transform, &inverse) != ARBOR_GEOMETRY_OK) return 9;
    if (arbor_affine_compose(inverse, transform, &product) != ARBOR_GEOMETRY_OK) return 10;
    if (arbor_geometry_vector_distance(product.a, ARBOR_COORD_ONE) > UINT64_C(2) ||
        arbor_geometry_vector_distance(product.d, ARBOR_COORD_ONE) > UINT64_C(2) ||
        arbor_geometry_vector_distance(product.b, 0) > UINT64_C(2) ||
        arbor_geometry_vector_distance(product.c, 0) > UINT64_C(2) ||
        arbor_geometry_vector_distance(product.tx, 0) > UINT64_C(2) ||
        arbor_geometry_vector_distance(product.ty, 0) > UINT64_C(2)) return 11;

    if (arbor_device_scale_make(3, 2, &scale) != ARBOR_GEOMETRY_OK) return 12;
    if (arbor_device_map_make(scale, arbor_geometry_vector_ci(10), arbor_geometry_vector_ci(-2), &map) != ARBOR_GEOMETRY_OK) return 13;
    if (arbor_device_map_point(
            map,
            (arbor_logical_point){arbor_geometry_vector_ci(4), arbor_geometry_vector_ci(6)},
            &device) != ARBOR_GEOMETRY_OK) return 14;
    if (device.x != arbor_geometry_vector_ci(16) || device.y != arbor_geometry_vector_ci(7)) return 15;

    if (arbor_rect_make(arbor_geometry_vector_ci(0), arbor_geometry_vector_ci(0),
                        arbor_geometry_vector_ci(10), arbor_geometry_vector_ci(10), &left) != ARBOR_GEOMETRY_OK) return 16;
    if (arbor_rect_make(arbor_geometry_vector_ci(5), arbor_geometry_vector_ci(7),
                        arbor_geometry_vector_ci(10), arbor_geometry_vector_ci(10), &right) != ARBOR_GEOMETRY_OK) return 17;
    if (arbor_rect_intersection(left, right, &intersection, &visible) != ARBOR_GEOMETRY_OK || !visible) return 18;
    if (intersection.x != arbor_geometry_vector_ci(5) ||
        intersection.y != arbor_geometry_vector_ci(7) ||
        intersection.width != arbor_geometry_vector_ci(5) ||
        intersection.height != arbor_geometry_vector_ci(3)) return 19;

    return 0;
}

#endif
