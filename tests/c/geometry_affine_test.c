#include <stdint.h>

#include <arborcore/geometry.h>

static arbor_coord ci(int64_t value)
{
    arbor_coord out = 0;
    (void)arbor_coord_from_integer(value, &out);
    return out;
}

static int64_t distance(arbor_coord left, arbor_coord right)
{
    int64_t delta = left - right;
    return (delta < 0) ? -delta : delta;
}

static int affine_near_identity(arbor_affine value, int64_t tolerance)
{
    return distance(value.a, ARBOR_COORD_ONE) <= tolerance &&
           distance(value.b, 0) <= tolerance &&
           distance(value.c, 0) <= tolerance &&
           distance(value.d, ARBOR_COORD_ONE) <= tolerance &&
           distance(value.tx, 0) <= tolerance &&
           distance(value.ty, 0) <= tolerance;
}

int main(void)
{
    arbor_affine identity = arbor_affine_identity();
    arbor_point p = {ci(3), ci(-4)};
    arbor_point out;
    arbor_geometry_status status;

    status = arbor_affine_transform_point(identity, p, &out);
    if (status != ARBOR_GEOMETRY_OK || out.x != p.x || out.y != p.y) return 1;

    {
        arbor_affine translation = arbor_affine_translation(ci(5), ci(7));
        arbor_affine scale = arbor_affine_scale(ci(2), ci(3));
        arbor_affine composed;
        status = arbor_affine_compose(translation, scale, &composed);
        if (status != ARBOR_GEOMETRY_OK) return 2;
        status = arbor_affine_transform_point(composed, (arbor_point){ci(1), ci(2)}, &out);
        if (status != ARBOR_GEOMETRY_OK || out.x != ci(7) || out.y != ci(13)) return 3;
    }

    {
        arbor_affine transform;
        arbor_affine inverse;
        arbor_affine product;
        arbor_affine scale = arbor_affine_scale(ci(2), ci(3));
        arbor_affine translation = arbor_affine_translation(ci(5), ci(-7));
        status = arbor_affine_compose(translation, scale, &transform);
        if (status != ARBOR_GEOMETRY_OK) return 4;
        status = arbor_affine_invert(transform, &inverse);
        if (status != ARBOR_GEOMETRY_OK) return 5;
        status = arbor_affine_compose(inverse, transform, &product);
        if (status != ARBOR_GEOMETRY_OK || !affine_near_identity(product, INT64_C(2))) return 6;
    }

    {
        arbor_affine rotation;
        arbor_coord angle;
        status = arbor_coord_from_ratio(1, 4, ARBOR_ROUND_NEAREST_EVEN, &angle);
        if (status != ARBOR_GEOMETRY_OK) return 7;
        status = arbor_affine_rotation_turns(angle, &rotation);
        if (status != ARBOR_GEOMETRY_OK || rotation.a != 0 ||
            rotation.b != ARBOR_COORD_ONE || rotation.c != -ARBOR_COORD_ONE ||
            rotation.d != 0) return 8;
        status = arbor_affine_transform_point(rotation, (arbor_point){ARBOR_COORD_ONE, 0}, &out);
        if (status != ARBOR_GEOMETRY_OK || out.x != 0 || out.y != ARBOR_COORD_ONE) return 9;
    }

    {
        arbor_affine rotation;
        arbor_coord angle;
        const arbor_coord expected = INT64_C(3037000500);
        status = arbor_coord_from_ratio(1, 8, ARBOR_ROUND_NEAREST_EVEN, &angle);
        if (status != ARBOR_GEOMETRY_OK) return 10;
        status = arbor_affine_rotation_turns(angle, &rotation);
        if (status != ARBOR_GEOMETRY_OK) return 11;
        if (distance(rotation.a, expected) > INT64_C(32) ||
            distance(rotation.b, expected) > INT64_C(32)) return 12;
    }

    {
        arbor_affine singular = {ARBOR_COORD_ONE, 0, 0, 0, 0, 0};
        arbor_affine sentinel = {1, 2, 3, 4, 5, 6};
        arbor_affine result = sentinel;
        status = arbor_affine_invert(singular, &result);
        if (status != ARBOR_GEOMETRY_SINGULAR) return 13;
        if (result.a != sentinel.a || result.d != sentinel.d || result.tx != sentinel.tx) return 14;
    }

    {
        arbor_rect rect;
        arbor_rect bounds;
        arbor_affine translation = arbor_affine_translation(ci(10), ci(-3));
        if (arbor_rect_make(ci(1), ci(2), ci(4), ci(5), &rect) != ARBOR_GEOMETRY_OK) return 15;
        status = arbor_affine_transform_rect_bounds(translation, rect, &bounds);
        if (status != ARBOR_GEOMETRY_OK || bounds.x != ci(11) || bounds.y != ci(-1) ||
            bounds.width != ci(4) || bounds.height != ci(5)) return 16;
    }

    return 0;
}
