#include <stdint.h>

#include <arborcore/geometry.h>

static arbor_coord ci(int64_t value)
{
    arbor_coord out = 0;
    (void)arbor_coord_from_integer(value, &out);
    return out;
}

int main(void)
{
    arbor_device_scale dpr;
    arbor_device_scale zoom;
    arbor_device_scale composed;
    arbor_device_map map;
    arbor_device_point device;
    arbor_geometry_status status;

    status = arbor_device_scale_make(UINT64_C(200), UINT64_C(100), &dpr);
    if (status != ARBOR_GEOMETRY_OK || dpr.numerator != 2 || dpr.denominator != 1) return 1;
    status = arbor_device_scale_make(UINT64_C(5), UINT64_C(4), &zoom);
    if (status != ARBOR_GEOMETRY_OK) return 2;
    status = arbor_device_scale_compose(dpr, zoom, &composed);
    if (status != ARBOR_GEOMETRY_OK || composed.numerator != 5 || composed.denominator != 2) return 3;

    status = arbor_device_map_make(dpr, 0, 0, &map);
    if (status != ARBOR_GEOMETRY_OK) return 4;
    status = arbor_device_map_point(
        map,
        (arbor_logical_point){INT64_C(24) * ARBOR_COORD_ONE + ARBOR_COORD_ONE * INT64_C(3) / INT64_C(8), 0},
        &device);
    if (status != ARBOR_GEOMETRY_OK ||
        device.x != INT64_C(48) * ARBOR_COORD_ONE + ARBOR_COORD_ONE * INT64_C(3) / INT64_C(4)) return 5;

    {
        arbor_device_scale ratio;
        status = arbor_device_scale_make(UINT64_C(3), UINT64_C(2), &ratio);
        if (status != ARBOR_GEOMETRY_OK) return 6;
        status = arbor_device_map_make(ratio, ci(10), ci(-2), &map);
        if (status != ARBOR_GEOMETRY_OK) return 7;
        status = arbor_device_map_point(map, (arbor_logical_point){ci(4), ci(6)}, &device);
        if (status != ARBOR_GEOMETRY_OK || device.x != ci(16) || device.y != ci(7)) return 8;
    }

    {
        arbor_logical_rect logical = {ci(1), ci(2), ci(3), ci(4)};
        arbor_device_rect result;
        arbor_device_scale ratio;
        status = arbor_device_scale_make(UINT64_C(3), UINT64_C(2), &ratio);
        if (status != ARBOR_GEOMETRY_OK) return 9;
        status = arbor_device_map_make(ratio, 0, 0, &map);
        if (status != ARBOR_GEOMETRY_OK) return 10;
        status = arbor_device_map_rect_bounds(map, logical, &result);
        if (status != ARBOR_GEOMETRY_OK || result.x != ci(1) + ARBOR_COORD_HALF ||
            result.y != ci(3) || result.width != ci(4) + ARBOR_COORD_HALF ||
            result.height != ci(6)) return 11;
    }

    {
        arbor_device_scale sentinel = {7, 9};
        arbor_device_scale out = sentinel;
        status = arbor_device_scale_make(0, 1, &out);
        if (status != ARBOR_GEOMETRY_INVALID_ARGUMENT || out.numerator != 7 || out.denominator != 9) return 12;
    }

    return 0;
}
