#include <limits.h>
#include <stdint.h>

#include <arborcore/geometry.h>

static int expect_ok(arbor_geometry_status status)
{
    return status == ARBOR_GEOMETRY_OK;
}

static int expect_code(arbor_geometry_status status, arbor_geometry_status code)
{
    return status == code;
}

int main(void)
{
    arbor_coord value;
    arbor_coord out;
    int64_t integer;
    arbor_geometry_status status;

    status = arbor_coord_from_integer(INT64_C(1), &value);
    if (!expect_ok(status) || value != ARBOR_COORD_ONE) return 1;
    status = arbor_coord_from_integer(INT64_C(-1), &value);
    if (!expect_ok(status) || value != -ARBOR_COORD_ONE) return 2;
    status = arbor_coord_from_integer(INT64_C(2147483647), &value);
    if (!expect_ok(status) || value != INT64_C(9223372032559808512)) return 3;
    status = arbor_coord_from_integer(INT64_C(-2147483648), &value);
    if (!expect_ok(status) || value != INT64_MIN) return 4;

    out = INT64_C(1234);
    status = arbor_coord_from_integer(INT64_C(2147483648), &out);
    if (!expect_code(status, ARBOR_GEOMETRY_OVERFLOW) || out != INT64_C(1234)) return 5;

    status = arbor_coord_from_ratio(INT64_C(1), UINT64_C(2), ARBOR_ROUND_NEAREST_EVEN, &value);
    if (!expect_ok(status) || value != ARBOR_COORD_HALF) return 6;

    status = arbor_coord_from_ratio(INT64_C(1), UINT64_C(8589934592), ARBOR_ROUND_NEAREST_EVEN, &value);
    if (!expect_ok(status) || value != 0) return 7;
    status = arbor_coord_from_ratio(INT64_C(3), UINT64_C(8589934592), ARBOR_ROUND_NEAREST_EVEN, &value);
    if (!expect_ok(status) || value != 2) return 8;

    value = -(ARBOR_COORD_ONE + ARBOR_COORD_HALF);
    status = arbor_coord_to_integer(value, ARBOR_ROUND_TOWARD_ZERO, &integer);
    if (!expect_ok(status) || integer != -1) return 9;
    status = arbor_coord_to_integer(value, ARBOR_ROUND_FLOOR, &integer);
    if (!expect_ok(status) || integer != -2) return 10;
    status = arbor_coord_to_integer(value, ARBOR_ROUND_CEIL, &integer);
    if (!expect_ok(status) || integer != -1) return 11;
    status = arbor_coord_to_integer(value, ARBOR_ROUND_NEAREST_EVEN, &integer);
    if (!expect_ok(status) || integer != -2) return 12;

    value = INT64_C(2) * ARBOR_COORD_ONE + ARBOR_COORD_HALF;
    status = arbor_coord_to_integer(value, ARBOR_ROUND_NEAREST_EVEN, &integer);
    if (!expect_ok(status) || integer != 2) return 13;
    value = INT64_C(3) * ARBOR_COORD_ONE + ARBOR_COORD_HALF;
    status = arbor_coord_to_integer(value, ARBOR_ROUND_NEAREST_EVEN, &integer);
    if (!expect_ok(status) || integer != 4) return 14;

    out = INT64_C(555);
    status = arbor_coord_add(INT64_MAX, INT64_C(1), &out);
    if (!expect_code(status, ARBOR_GEOMETRY_OVERFLOW) || out != INT64_C(555)) return 15;
    status = arbor_coord_sub(INT64_MIN, INT64_C(1), &out);
    if (!expect_code(status, ARBOR_GEOMETRY_OVERFLOW) || out != INT64_C(555)) return 16;
    status = arbor_coord_neg(INT64_MIN, &out);
    if (!expect_code(status, ARBOR_GEOMETRY_OVERFLOW) || out != INT64_C(555)) return 17;
    status = arbor_coord_abs(INT64_MIN, &out);
    if (!expect_code(status, ARBOR_GEOMETRY_OVERFLOW) || out != INT64_C(555)) return 18;

    status = arbor_coord_mul(ARBOR_COORD_ONE + ARBOR_COORD_HALF, INT64_C(2) * ARBOR_COORD_ONE, &out);
    if (!expect_ok(status) || out != INT64_C(3) * ARBOR_COORD_ONE) return 19;

    status = arbor_coord_div(ARBOR_COORD_ONE, INT64_C(3) * ARBOR_COORD_ONE, &out);
    if (!expect_ok(status) || out != INT64_C(1431655765)) return 20;

    out = INT64_C(777);
    status = arbor_coord_div(ARBOR_COORD_ONE, 0, &out);
    if (!expect_code(status, ARBOR_GEOMETRY_INVALID_ARGUMENT) || out != INT64_C(777)) return 21;

    status = arbor_coord_mul(INT64_MAX, INT64_MAX, &out);
    if (!expect_code(status, ARBOR_GEOMETRY_OVERFLOW)) return 22;

    status = arbor_coord_div(ARBOR_COORD_ONE, -INT64_C(3) * ARBOR_COORD_ONE, &out);
    if (!expect_ok(status) || out != -INT64_C(1431655765)) return 23;
    status = arbor_coord_div(-ARBOR_COORD_ONE, INT64_C(3) * ARBOR_COORD_ONE, &out);
    if (!expect_ok(status) || out != -INT64_C(1431655765)) return 24;
    status = arbor_coord_div(-ARBOR_COORD_ONE, -INT64_C(3) * ARBOR_COORD_ONE, &out);
    if (!expect_ok(status) || out != INT64_C(1431655765)) return 25;

    status = arbor_coord_mul(INT64_C(1), ARBOR_COORD_HALF, &out);
    if (!expect_ok(status) || out != INT64_C(0)) return 26;
    status = arbor_coord_mul(INT64_C(3), ARBOR_COORD_HALF, &out);
    if (!expect_ok(status) || out != INT64_C(2)) return 27;
    status = arbor_coord_mul(-INT64_C(3), ARBOR_COORD_HALF, &out);
    if (!expect_ok(status) || out != -INT64_C(2)) return 28;

    status = arbor_coord_div(INT64_C(1), INT64_C(2) * ARBOR_COORD_ONE, &out);
    if (!expect_ok(status) || out != INT64_C(0)) return 29;
    status = arbor_coord_div(INT64_C(3), INT64_C(2) * ARBOR_COORD_ONE, &out);
    if (!expect_ok(status) || out != INT64_C(2)) return 30;
    status = arbor_coord_div(-INT64_C(3), INT64_C(2) * ARBOR_COORD_ONE, &out);
    if (!expect_ok(status) || out != -INT64_C(2)) return 31;

    /* Native x86-64 DIV fast-path boundary cases; portable/WASM semantics
     * must produce the same results. */
    status = arbor_coord_div(INT64_MIN, ARBOR_COORD_ONE, &out);
    if (!expect_ok(status) || out != INT64_MIN) return 32;
    status = arbor_coord_div(INT64_MAX, ARBOR_COORD_ONE, &out);
    if (!expect_ok(status) || out != INT64_MAX) return 33;
    status = arbor_coord_div(ARBOR_COORD_ONE, INT64_MIN, &out);
    if (!expect_ok(status) || out != -INT64_C(2)) return 34;

    out = INT64_C(909);
    status = arbor_coord_div(INT64_MAX, INT64_C(1), &out);
    if (!expect_code(status, ARBOR_GEOMETRY_OVERFLOW) || out != INT64_C(909)) return 35;
    status = arbor_coord_div(INT64_MIN, -ARBOR_COORD_ONE, &out);
    if (!expect_code(status, ARBOR_GEOMETRY_OVERFLOW) || out != INT64_C(909)) return 36;

    status = arbor_coord_mul(INT64_MAX, ARBOR_COORD_ONE, &out);
    if (!expect_ok(status) || out != INT64_MAX) return 37;
    status = arbor_coord_mul(INT64_MIN, ARBOR_COORD_ONE, &out);
    if (!expect_ok(status) || out != INT64_MIN) return 38;

    if (sizeof(arbor_coord) != 8u || ARBOR_COORD_FRACTION_BITS != 32u) return 39;
    return 0;
}
