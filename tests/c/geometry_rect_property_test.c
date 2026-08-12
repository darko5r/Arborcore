#include <stdbool.h>
#include <stdint.h>

#include <arborcore/geometry.h>

static arbor_coord coord_i(int64_t value)
{
    arbor_coord result = 0;
    if (arbor_coord_from_integer(value, &result) != ARBOR_GEOMETRY_OK) {
        return 0;
    }
    return result;
}

static uint64_t next_u64(uint64_t *state)
{
    *state = *state * UINT64_C(6364136223846793005) + UINT64_C(1442695040888963407);
    return *state;
}

static int rect_equal(arbor_rect left, arbor_rect right)
{
    return left.x == right.x && left.y == right.y &&
           left.width == right.width && left.height == right.height;
}

int main(void)
{
    uint64_t state = UINT64_C(0x9e3779b97f4a7c15);
    unsigned int i;

    for (i = 0; i < 10000u; ++i) {
        int64_t ax = (int64_t)(next_u64(&state) % UINT64_C(2001)) - INT64_C(1000);
        int64_t ay = (int64_t)(next_u64(&state) % UINT64_C(2001)) - INT64_C(1000);
        int64_t bx = (int64_t)(next_u64(&state) % UINT64_C(2001)) - INT64_C(1000);
        int64_t by = (int64_t)(next_u64(&state) % UINT64_C(2001)) - INT64_C(1000);
        int64_t aw = (int64_t)(next_u64(&state) % UINT64_C(101));
        int64_t ah = (int64_t)(next_u64(&state) % UINT64_C(101));
        int64_t bw = (int64_t)(next_u64(&state) % UINT64_C(101));
        int64_t bh = (int64_t)(next_u64(&state) % UINT64_C(101));
        arbor_rect a;
        arbor_rect b;
        arbor_rect ab;
        arbor_rect ba;
        arbor_rect united;
        bool ab_visible;
        bool ba_visible;

        if (arbor_rect_make(coord_i(ax), coord_i(ay), coord_i(aw), coord_i(ah), &a) != ARBOR_GEOMETRY_OK) return 1;
        if (arbor_rect_make(coord_i(bx), coord_i(by), coord_i(bw), coord_i(bh), &b) != ARBOR_GEOMETRY_OK) return 2;
        if (arbor_rect_intersection(a, b, &ab, &ab_visible) != ARBOR_GEOMETRY_OK) return 3;
        if (arbor_rect_intersection(b, a, &ba, &ba_visible) != ARBOR_GEOMETRY_OK) return 4;
        if (ab_visible != ba_visible || !rect_equal(ab, ba)) return 5;
        if (arbor_rect_union(a, b, &united) != ARBOR_GEOMETRY_OK) return 6;

        if (a.width > 0 && a.height > 0) {
            arbor_point p = {a.x, a.y};
            if (!arbor_rect_contains_point(united, p)) return 7;
        }
        if (b.width > 0 && b.height > 0) {
            arbor_point p = {b.x, b.y};
            if (!arbor_rect_contains_point(united, p)) return 8;
        }
    }

    {
        arbor_rect rect;
        arbor_rect sentinel = {INT64_C(11), INT64_C(22), INT64_C(33), INT64_C(44)};
        arbor_rect out = sentinel;
        arbor_geometry_status status;
        status = arbor_rect_make(INT64_MAX, 0, ARBOR_COORD_ONE, ARBOR_COORD_ONE, &out);
        if (status != ARBOR_GEOMETRY_OVERFLOW || !rect_equal(out, sentinel)) return 9;

        status = arbor_rect_make(0, 0, -ARBOR_COORD_ONE, ARBOR_COORD_ONE, &rect);
        if (status != ARBOR_GEOMETRY_INVALID_ARGUMENT) return 10;
    }

    {
        arbor_line line = {
            {coord_i(-3), coord_i(7)},
            {coord_i(5), coord_i(-2)}
        };
        arbor_rect bounds;
        if (arbor_line_bounds(line, &bounds) != ARBOR_GEOMETRY_OK) return 11;
        if (bounds.x != coord_i(-3) || bounds.y != coord_i(-2) ||
            bounds.width != coord_i(8) || bounds.height != coord_i(9)) return 12;
    }

    return 0;
}
