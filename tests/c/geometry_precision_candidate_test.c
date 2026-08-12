#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "fixed_point_candidates.h"

static void require_true(bool condition, const char *message)
{
    if (!condition) {
        (void)fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

static void test_rounding(void)
{
    require_true(arbor_geometry_nearest_even_raw_i64(INT64_C(1), INT64_C(2)) == 0,
                 "+0.5 rounds to even 0");
    require_true(arbor_geometry_nearest_even_raw_i64(INT64_C(3), INT64_C(2)) == 2,
                 "+1.5 rounds to even 2");
    require_true(arbor_geometry_nearest_even_raw_i64(INT64_C(5), INT64_C(2)) == 2,
                 "+2.5 rounds to even 2");
    require_true(arbor_geometry_nearest_even_raw_i64(INT64_C(-1), INT64_C(2)) == 0,
                 "-0.5 rounds to even 0");
    require_true(arbor_geometry_nearest_even_raw_i64(INT64_C(-3), INT64_C(2)) == -2,
                 "-1.5 rounds to even -2");
    require_true(arbor_geometry_nearest_even_raw_i64(INT64_C(-5), INT64_C(2)) == -2,
                 "-2.5 rounds to even -2");

    require_true(arbor_geometry_floor_raw_i64(-3, 2) == -2, "floor negative");
    require_true(arbor_geometry_ceil_raw_i64(-3, 2) == -1, "ceil negative");
    require_true(arbor_geometry_trunc_raw_i64(-3, 2) == -1, "truncate negative");
    require_true(arbor_geometry_floor_raw_i64(3, 2) == 1, "floor positive");
    require_true(arbor_geometry_ceil_raw_i64(3, 2) == 2, "ceil positive");
}

static void test_q16_16(void)
{
    int32_t one = 0;
    int32_t half = ARBOR_Q16_16_ONE / 2;
    int32_t value = 0;
    int32_t unchanged = INT32_C(0x13572468);

    require_true(arbor_q16_16_from_integer(1, &one) && one == ARBOR_Q16_16_ONE,
                 "Q16.16 integer conversion");
    require_true(arbor_q16_16_mul(one, half, &value) && value == half,
                 "Q16.16 multiply identity");
    require_true(arbor_q16_16_div(half, one, &value) && value == half,
                 "Q16.16 divide identity");
    require_true(!arbor_q16_16_add(INT32_MAX, 1, &unchanged),
                 "Q16.16 add overflow detected");
    require_true(unchanged == INT32_C(0x13572468),
                 "Q16.16 overflow is transactional");
    require_true(!arbor_q16_16_div(one, 0, &unchanged),
                 "Q16.16 divide by zero detected");
}

static void test_q26_6(void)
{
    int32_t one = 0;
    int32_t half = ARBOR_Q26_6_ONE / 2;
    int32_t value = 0;
    int32_t unchanged = INT32_C(0x24681357);

    require_true(arbor_q26_6_from_integer(1, &one) && one == ARBOR_Q26_6_ONE,
                 "Q26.6 integer conversion");
    require_true(arbor_q26_6_mul(one, half, &value) && value == half,
                 "Q26.6 multiply identity");
    require_true(arbor_q26_6_div(half, one, &value) && value == half,
                 "Q26.6 divide identity");
    require_true(!arbor_q26_6_sub(INT32_MIN, 1, &unchanged),
                 "Q26.6 subtract overflow detected");
    require_true(unchanged == INT32_C(0x24681357),
                 "Q26.6 overflow is transactional");
}

static void test_q32_32(void)
{
    int64_t one = 0;
    int64_t half = ARBOR_Q32_32_ONE / 2;
    int64_t value = 0;
    int64_t unchanged = INT64_C(0x1357246813572468);

    require_true(arbor_q32_32_from_integer(1, &one) && one == ARBOR_Q32_32_ONE,
                 "Q32.32 integer conversion");
    require_true(arbor_q32_32_mul(one, half, &value) && value == half,
                 "Q32.32 multiply identity");
    require_true(arbor_q32_32_div(half, one, &value) && value == half,
                 "Q32.32 divide identity");
    require_true(!arbor_q32_32_add(INT64_MAX, 1, &unchanged),
                 "Q32.32 add overflow detected");
    require_true(unchanged == INT64_C(0x1357246813572468),
                 "Q32.32 overflow is transactional");
    require_true(!arbor_q32_32_mul(INT64_MAX, ARBOR_Q32_32_ONE * 2, &unchanged),
                 "Q32.32 multiply overflow detected");
}

static void test_q24_40(void)
{
    int64_t one = 0;
    int64_t half = ARBOR_Q24_40_ONE / 2;
    int64_t value = 0;
    int64_t unchanged = INT64_C(0x2468135724681357);

    require_true(arbor_q24_40_from_integer(1, &one) && one == ARBOR_Q24_40_ONE,
                 "Q24.40 integer conversion");
    require_true(arbor_q24_40_mul(one, half, &value) && value == half,
                 "Q24.40 multiply identity");
    require_true(arbor_q24_40_div(half, one, &value) && value == half,
                 "Q24.40 divide identity");
    require_true(!arbor_q24_40_sub(INT64_MIN, 1, &unchanged),
                 "Q24.40 subtract overflow detected");
    require_true(unchanged == INT64_C(0x2468135724681357),
                 "Q24.40 overflow is transactional");
}

static void property_round_trip(void)
{
    uint64_t state = UINT64_C(0x8a5cd789635d2dff);
    unsigned int iteration;

    for (iteration = 0; iteration < 200000u; ++iteration) {
        int64_t integer;
        int32_t q32;
        int64_t q64;

        state = state * UINT64_C(6364136223846793005) + UINT64_C(1442695040888963407);
        integer = (int64_t)(state % UINT64_C(20001)) - INT64_C(10000);

        require_true(arbor_q16_16_from_integer(integer, &q32),
                     "Q16.16 property conversion");
        require_true(arbor_geometry_trunc_raw_i64((int64_t)q32, ARBOR_Q16_16_ONE) == integer,
                     "Q16.16 integer round trip");

        require_true(arbor_q26_6_from_integer(integer, &q32),
                     "Q26.6 property conversion");
        require_true(arbor_geometry_trunc_raw_i64((int64_t)q32, ARBOR_Q26_6_ONE) == integer,
                     "Q26.6 integer round trip");

        require_true(arbor_q32_32_from_integer(integer, &q64),
                     "Q32.32 property conversion");
        require_true(arbor_geometry_trunc_raw_i64(q64, ARBOR_Q32_32_ONE) == integer,
                     "Q32.32 integer round trip");

        require_true(arbor_q24_40_from_integer(integer, &q64),
                     "Q24.40 property conversion");
        require_true(arbor_geometry_trunc_raw_i64(q64, ARBOR_Q24_40_ONE) == integer,
                     "Q24.40 integer round trip");
    }
}

static void print_candidate(const char *name,
                            unsigned int storage_bits,
                            unsigned int fractional_bits,
                            uint64_t max_integer)
{
    int meets_range = (max_integer >= ARBOR_GEOMETRY_MIN_INTEGER_RANGE) ? 1 : 0;
    int meets_precision = (fractional_bits >= ARBOR_GEOMETRY_MIN_FRACTION_BITS) ? 1 : 0;
    int eligible = (meets_range != 0 && meets_precision != 0) ? 1 : 0;

    (void)printf(
        "candidate=%s storage_bits=%u fractional_bits=%u max_integer=%" PRIu64
        " step_denominator=2^%u meets_range=%d meets_precision=%d eligible=%d\n",
        name,
        storage_bits,
        fractional_bits,
        max_integer,
        fractional_bits,
        meets_range,
        meets_precision,
        eligible);
}

int main(void)
{
    _Static_assert(sizeof(int32_t) == 4, "G0 requires 32-bit int32_t");
    _Static_assert(sizeof(int64_t) == 8, "G0 requires 64-bit int64_t");

    test_rounding();
    test_q16_16();
    test_q26_6();
    test_q32_32();
    test_q24_40();
    property_round_trip();

    (void)printf("G0_MIN_INTEGER_RANGE=%" PRIu64 "\n", ARBOR_GEOMETRY_MIN_INTEGER_RANGE);
    (void)printf("G0_MIN_FRACTION_BITS=%u\n", ARBOR_GEOMETRY_MIN_FRACTION_BITS);
    print_candidate("Q16.16", 32u, ARBOR_Q16_16_FRACTION_BITS, UINT64_C(32767));
    print_candidate("Q26.6", 32u, ARBOR_Q26_6_FRACTION_BITS, UINT64_C(33554431));
    print_candidate("Q32.32", 64u, ARBOR_Q32_32_FRACTION_BITS, UINT64_C(2147483647));
    print_candidate("Q24.40", 64u, ARBOR_Q24_40_FRACTION_BITS, UINT64_C(8388607));
    (void)puts("G0_G1_CANDIDATE_PROPERTIES=PASS");
    return 0;
}
