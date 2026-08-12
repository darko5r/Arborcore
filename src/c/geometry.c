#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#include <arborcore/geometry.h>

__extension__ typedef __int128 arbor_i128;
__extension__ typedef unsigned __int128 arbor_u128;

#define ARBOR_Q32_SCALE_I128 (((arbor_i128)1) << ARBOR_COORD_FRACTION_BITS)
#define ARBOR_Q64_SCALE_I128 (((arbor_i128)1) << 64)
#define ARBOR_CORDIC_INTERNAL_SHIFT 62u
#define ARBOR_CORDIC_TO_Q32_SHIFT (ARBOR_CORDIC_INTERNAL_SHIFT - ARBOR_COORD_FRACTION_BITS)
#define ARBOR_CORDIC_K_Q62 INT64_C(2800459870029453824)
#define ARBOR_CORDIC_ITERATIONS 31u

static const int64_t arbor_cordic_atan_turns_q32[ARBOR_CORDIC_ITERATIONS] = {
    INT64_C(536870912), INT64_C(316933406), INT64_C(167458907),
    INT64_C(85004756),  INT64_C(42667331),  INT64_C(21354465),
    INT64_C(10679838),  INT64_C(5340245),   INT64_C(2670163),
    INT64_C(1335087),   INT64_C(667544),    INT64_C(333772),
    INT64_C(166886),    INT64_C(83443),     INT64_C(41722),
    INT64_C(20861),     INT64_C(10430),     INT64_C(5215),
    INT64_C(2608),      INT64_C(1304),      INT64_C(652),
    INT64_C(326),       INT64_C(163),       INT64_C(81),
    INT64_C(41),        INT64_C(20),        INT64_C(10),
    INT64_C(5),         INT64_C(3),         INT64_C(1),
    INT64_C(1)
};

static bool arbor_rounding_mode_valid(arbor_rounding_mode mode)
{
    return mode == ARBOR_ROUND_TOWARD_ZERO ||
           mode == ARBOR_ROUND_FLOOR ||
           mode == ARBOR_ROUND_CEIL ||
           mode == ARBOR_ROUND_NEAREST_EVEN;
}

static arbor_u128 arbor_i128_magnitude(arbor_i128 value)
{
    if (value >= 0) {
        return (arbor_u128)value;
    }
    return (arbor_u128)(-(value + 1)) + (arbor_u128)1;
}

static arbor_geometry_status arbor_round_div_i128_to_i64(
    arbor_i128 numerator,
    arbor_i128 denominator,
    arbor_rounding_mode mode,
    int64_t *out)
{
    arbor_u128 numerator_magnitude;
    arbor_u128 denominator_magnitude;
    arbor_u128 quotient;
    arbor_u128 remainder;
    arbor_u128 negative_limit;
    bool negative;
    bool increment = false;
    int64_t result;

    if (out == NULL || denominator == 0 || !arbor_rounding_mode_valid(mode)) {
        return ARBOR_GEOMETRY_INVALID_ARGUMENT;
    }

    negative = (numerator < 0) != (denominator < 0);
    numerator_magnitude = arbor_i128_magnitude(numerator);
    denominator_magnitude = arbor_i128_magnitude(denominator);
    quotient = numerator_magnitude / denominator_magnitude;
    remainder = numerator_magnitude % denominator_magnitude;

    if (remainder != 0) {
        switch (mode) {
        case ARBOR_ROUND_TOWARD_ZERO:
            break;
        case ARBOR_ROUND_FLOOR:
            increment = negative;
            break;
        case ARBOR_ROUND_CEIL:
            increment = !negative;
            break;
        case ARBOR_ROUND_NEAREST_EVEN:
            if (remainder > denominator_magnitude - remainder) {
                increment = true;
            } else if (remainder == denominator_magnitude - remainder &&
                       (quotient & (arbor_u128)1) != 0) {
                increment = true;
            }
            break;
        }
    }

    if (increment) {
        if (quotient == ~(arbor_u128)0) {
            return ARBOR_GEOMETRY_OVERFLOW;
        }
        ++quotient;
    }

    if (!negative) {
        if (quotient > (arbor_u128)INT64_MAX) {
            return ARBOR_GEOMETRY_OVERFLOW;
        }
        result = (int64_t)quotient;
    } else {
        negative_limit = (arbor_u128)INT64_MAX + (arbor_u128)1;
        if (quotient > negative_limit) {
            return ARBOR_GEOMETRY_OVERFLOW;
        }
        if (quotient == negative_limit) {
            result = INT64_MIN;
        } else {
            result = -(int64_t)quotient;
        }
    }

    *out = result;
    return ARBOR_GEOMETRY_OK;
}

static arbor_geometry_status arbor_coord_from_i128_scaled(arbor_i128 numerator, arbor_i128 denominator, arbor_coord *out)
{
    return arbor_round_div_i128_to_i64(
        numerator,
        denominator,
        ARBOR_ROUND_NEAREST_EVEN,
        out);
}

#if defined(__x86_64__) && !defined(__wasm__)
/*
 * Native x86-64 hot path for Q32.32 division/rounding.  The hardware DIV
 * instruction divides an unsigned 128-bit RDX:RAX dividend by a 64-bit
 * divisor and returns both quotient and remainder.  Checking high<divisor
 * before DIV makes quotient overflow impossible; the signed result is then
 * range-checked after nearest-even rounding.  This avoids compiler-runtime
 * 128-bit division calls while preserving the portable numerical contract.
 */
static arbor_geometry_status arbor_round_nearest_even_i128_by_i64_to_i64(
    arbor_i128 numerator,
    int64_t denominator,
    int64_t *out)
{
    arbor_u128 magnitude;
    arbor_u128 denominator_magnitude;
    uint64_t high;
    uint64_t low;
    uint64_t divisor;
    uint64_t quotient;
    uint64_t remainder;
    uint64_t limit;
    bool negative;

    if (out == NULL || denominator == 0) {
        return ARBOR_GEOMETRY_INVALID_ARGUMENT;
    }

    negative = (numerator < 0) != (denominator < 0);
    magnitude = arbor_i128_magnitude(numerator);
    denominator_magnitude = arbor_i128_magnitude((arbor_i128)denominator);
    high = (uint64_t)(magnitude >> 64);
    low = (uint64_t)magnitude;
    divisor = (uint64_t)denominator_magnitude;

    /* high >= divisor would make the unsigned quotient at least 2^64. */
    if (high >= divisor) {
        return ARBOR_GEOMETRY_OVERFLOW;
    }

    __asm__ volatile (
        "divq %[divisor]"
        : "=a" (quotient), "=d" (remainder)
        : "a" (low), "d" (high), [divisor] "r" (divisor)
        : "cc");

    if (remainder > divisor - remainder ||
        (remainder == divisor - remainder && (quotient & UINT64_C(1)) != 0)) {
        if (quotient == UINT64_MAX) {
            return ARBOR_GEOMETRY_OVERFLOW;
        }
        ++quotient;
    }

    limit = negative ? (uint64_t)INT64_MAX + UINT64_C(1) : (uint64_t)INT64_MAX;
    if (quotient > limit) {
        return ARBOR_GEOMETRY_OVERFLOW;
    }

    if (!negative) {
        *out = (int64_t)quotient;
    } else if (quotient == (uint64_t)INT64_MAX + UINT64_C(1)) {
        *out = INT64_MIN;
    } else {
        *out = -(int64_t)quotient;
    }
    return ARBOR_GEOMETRY_OK;
}
#elif !defined(__wasm__)
/*
 * Portable native fallback for non-x86-64 targets.  Signed __int128 div/mod
 * preserves the same nearest-even and transactional semantics.
 */
static arbor_geometry_status arbor_round_nearest_even_i128_by_i64_to_i64(
    arbor_i128 numerator,
    int64_t denominator,
    int64_t *out)
{
    arbor_i128 divisor;
    arbor_i128 quotient;
    arbor_i128 remainder;
    arbor_u128 remainder_abs;
    arbor_u128 divisor_abs;
    arbor_u128 twice_remainder;
    bool negative;

    if (out == NULL || denominator == 0) {
        return ARBOR_GEOMETRY_INVALID_ARGUMENT;
    }

    divisor = (arbor_i128)denominator;
    quotient = numerator / divisor;
    remainder = numerator % divisor;
    remainder_abs = (remainder < 0) ? (arbor_u128)(-remainder) : (arbor_u128)remainder;
    divisor_abs = (divisor < 0) ? (arbor_u128)(-divisor) : (arbor_u128)divisor;
    twice_remainder = remainder_abs * (arbor_u128)2;
    negative = (numerator < 0) != (divisor < 0);

    if (twice_remainder > divisor_abs ||
        (twice_remainder == divisor_abs && (quotient & (arbor_i128)1) != 0)) {
        quotient += negative ? (arbor_i128)-1 : (arbor_i128)1;
    }

    if (quotient < (arbor_i128)INT64_MIN || quotient > (arbor_i128)INT64_MAX) {
        return ARBOR_GEOMETRY_OVERFLOW;
    }

    *out = (int64_t)quotient;
    return ARBOR_GEOMETRY_OK;
}
#endif

static arbor_geometry_status arbor_round_nearest_even_i128_q32_to_i64(
    arbor_i128 numerator,
    int64_t *out)
{
    arbor_u128 magnitude;
    arbor_u128 quotient;
    uint64_t remainder;
    uint64_t limit;
    bool negative;

    if (out == NULL) {
        return ARBOR_GEOMETRY_INVALID_ARGUMENT;
    }

    negative = numerator < 0;
    magnitude = arbor_i128_magnitude(numerator);
    quotient = magnitude >> ARBOR_COORD_FRACTION_BITS;
    remainder = (uint64_t)magnitude & UINT64_C(0xffffffff);

    if (remainder > UINT64_C(0x80000000) ||
        (remainder == UINT64_C(0x80000000) &&
         (quotient & (arbor_u128)1) != 0)) {
        ++quotient;
    }

    limit = negative ? (uint64_t)INT64_MAX + UINT64_C(1) : (uint64_t)INT64_MAX;
    if (quotient > (arbor_u128)limit) {
        return ARBOR_GEOMETRY_OVERFLOW;
    }

    if (!negative) {
        *out = (int64_t)quotient;
    } else if ((uint64_t)quotient == (uint64_t)INT64_MAX + UINT64_C(1)) {
        *out = INT64_MIN;
    } else {
        *out = -(int64_t)quotient;
    }
    return ARBOR_GEOMETRY_OK;
}

static arbor_geometry_status arbor_coord_from_i128_q32_hot(arbor_i128 numerator, arbor_coord *out)
{
    return arbor_round_nearest_even_i128_q32_to_i64(numerator, out);
}

static arbor_geometry_status arbor_coord_from_i128_divisor_hot(
    arbor_i128 numerator,
    arbor_coord denominator,
    arbor_coord *out)
{
#if defined(__wasm__)
    return arbor_coord_from_i128_scaled(numerator, (arbor_i128)denominator, out);
#else
    return arbor_round_nearest_even_i128_by_i64_to_i64(numerator, denominator, out);
#endif
}

static arbor_geometry_status arbor_affine_fused2(
    arbor_coord left_a,
    arbor_coord left_b,
    arbor_coord right_a,
    arbor_coord right_b,
    arbor_coord *out)
{
    arbor_i128 first = (arbor_i128)left_a * (arbor_i128)right_a;
    arbor_i128 second = (arbor_i128)left_b * (arbor_i128)right_b;
    arbor_i128 sum;

    if (__builtin_add_overflow(first, second, &sum)) {
        return ARBOR_GEOMETRY_OVERFLOW;
    }
    return arbor_coord_from_i128_q32_hot(sum, out);
}

static arbor_geometry_status arbor_affine_fused2_translate(
    arbor_coord left_a,
    arbor_coord left_b,
    arbor_coord right_a,
    arbor_coord right_b,
    arbor_coord translation,
    arbor_coord *out)
{
    arbor_i128 first = (arbor_i128)left_a * (arbor_i128)right_a;
    arbor_i128 second = (arbor_i128)left_b * (arbor_i128)right_b;
    arbor_i128 translated = (arbor_i128)translation * ARBOR_Q32_SCALE_I128;
    arbor_i128 sum;

    if (__builtin_add_overflow(first, second, &sum) ||
        __builtin_add_overflow(sum, translated, &sum)) {
        return ARBOR_GEOMETRY_OVERFLOW;
    }
    return arbor_coord_from_i128_q32_hot(sum, out);
}

static arbor_geometry_status arbor_rect_edges(
    arbor_rect rect,
    arbor_coord *right,
    arbor_coord *bottom)
{
    arbor_geometry_status status;
    arbor_coord computed_right;
    arbor_coord computed_bottom;

    if (right == NULL || bottom == NULL || rect.width < 0 || rect.height < 0) {
        return ARBOR_GEOMETRY_INVALID_ARGUMENT;
    }

    status = arbor_coord_add(rect.x, rect.width, &computed_right);
    if (status != ARBOR_GEOMETRY_OK) {
        return status;
    }
    status = arbor_coord_add(rect.y, rect.height, &computed_bottom);
    if (status != ARBOR_GEOMETRY_OK) {
        return status;
    }

    *right = computed_right;
    *bottom = computed_bottom;
    return ARBOR_GEOMETRY_OK;
}

static int64_t arbor_shift_right_arithmetic_i64(int64_t value, unsigned int shift)
{
    uint64_t magnitude;
    uint64_t quotient;
    uint64_t remainder_mask;

    if (shift == 0u) {
        return value;
    }
    if (value >= 0) {
        return value >> shift;
    }

    magnitude = (uint64_t)(-(value + 1)) + UINT64_C(1);
    quotient = magnitude >> shift;
    remainder_mask = (UINT64_C(1) << shift) - UINT64_C(1);
    if ((magnitude & remainder_mask) != 0) {
        ++quotient;
    }
    if (quotient == (UINT64_C(1) << 63)) {
        return INT64_MIN;
    }
    return -(int64_t)quotient;
}

static uint64_t arbor_u64_gcd(uint64_t left, uint64_t right)
{
    while (right != 0) {
        uint64_t remainder = left % right;
        left = right;
        right = remainder;
    }
    return left;
}

arbor_geometry_status arbor_coord_from_integer(int64_t integer, arbor_coord *out)
{
    arbor_i128 raw;

    if (out == NULL) {
        return ARBOR_GEOMETRY_INVALID_ARGUMENT;
    }
    raw = (arbor_i128)integer * ARBOR_Q32_SCALE_I128;
    if (raw < (arbor_i128)INT64_MIN || raw > (arbor_i128)INT64_MAX) {
        return ARBOR_GEOMETRY_OVERFLOW;
    }
    *out = (arbor_coord)raw;
    return ARBOR_GEOMETRY_OK;
}

arbor_geometry_status arbor_coord_from_ratio(
    int64_t numerator,
    uint64_t denominator,
    arbor_rounding_mode mode,
    arbor_coord *out)
{
    arbor_i128 scaled;

    if (out == NULL || denominator == 0 || denominator > (uint64_t)INT64_MAX ||
        !arbor_rounding_mode_valid(mode)) {
        return ARBOR_GEOMETRY_INVALID_ARGUMENT;
    }
    scaled = (arbor_i128)numerator * ARBOR_Q32_SCALE_I128;
    return arbor_round_div_i128_to_i64(scaled, (arbor_i128)denominator, mode, out);
}

arbor_geometry_status arbor_coord_to_integer(
    arbor_coord value,
    arbor_rounding_mode mode,
    int64_t *out)
{
    if (out == NULL || !arbor_rounding_mode_valid(mode)) {
        return ARBOR_GEOMETRY_INVALID_ARGUMENT;
    }
    return arbor_round_div_i128_to_i64(
        (arbor_i128)value,
        ARBOR_Q32_SCALE_I128,
        mode,
        out);
}

arbor_geometry_status arbor_coord_add(arbor_coord left, arbor_coord right, arbor_coord *out)
{
    arbor_coord result;

    if (out == NULL) {
        return ARBOR_GEOMETRY_INVALID_ARGUMENT;
    }
    if ((right > 0 && left > INT64_MAX - right) ||
        (right < 0 && left < INT64_MIN - right)) {
        return ARBOR_GEOMETRY_OVERFLOW;
    }
    result = left + right;
    *out = result;
    return ARBOR_GEOMETRY_OK;
}

arbor_geometry_status arbor_coord_sub(arbor_coord left, arbor_coord right, arbor_coord *out)
{
    arbor_coord result;

    if (out == NULL) {
        return ARBOR_GEOMETRY_INVALID_ARGUMENT;
    }
    if ((right < 0 && left > INT64_MAX + right) ||
        (right > 0 && left < INT64_MIN + right)) {
        return ARBOR_GEOMETRY_OVERFLOW;
    }
    result = left - right;
    *out = result;
    return ARBOR_GEOMETRY_OK;
}

arbor_geometry_status arbor_coord_neg(arbor_coord value, arbor_coord *out)
{
    if (out == NULL) {
        return ARBOR_GEOMETRY_INVALID_ARGUMENT;
    }
    if (value == INT64_MIN) {
        return ARBOR_GEOMETRY_OVERFLOW;
    }
    *out = -value;
    return ARBOR_GEOMETRY_OK;
}

arbor_geometry_status arbor_coord_abs(arbor_coord value, arbor_coord *out)
{
    if (out == NULL) {
        return ARBOR_GEOMETRY_INVALID_ARGUMENT;
    }
    if (value == INT64_MIN) {
        return ARBOR_GEOMETRY_OVERFLOW;
    }
    *out = (value < 0) ? -value : value;
    return ARBOR_GEOMETRY_OK;
}

arbor_geometry_status arbor_coord_mul(arbor_coord left, arbor_coord right, arbor_coord *out)
{
    arbor_i128 product;

    if (out == NULL) {
        return ARBOR_GEOMETRY_INVALID_ARGUMENT;
    }
    product = (arbor_i128)left * (arbor_i128)right;
    return arbor_coord_from_i128_q32_hot(product, out);
}

arbor_geometry_status arbor_coord_div(arbor_coord numerator, arbor_coord denominator, arbor_coord *out)
{
    arbor_i128 scaled;

    if (out == NULL || denominator == 0) {
        return ARBOR_GEOMETRY_INVALID_ARGUMENT;
    }
    scaled = (arbor_i128)numerator * ARBOR_Q32_SCALE_I128;
    return arbor_coord_from_i128_divisor_hot(scaled, denominator, out);
}

arbor_geometry_status arbor_size_make(arbor_coord width, arbor_coord height, arbor_size *out)
{
    arbor_size result;

    if (out == NULL || width < 0 || height < 0) {
        return ARBOR_GEOMETRY_INVALID_ARGUMENT;
    }
    result.width = width;
    result.height = height;
    *out = result;
    return ARBOR_GEOMETRY_OK;
}

arbor_geometry_status arbor_rect_make(
    arbor_coord x,
    arbor_coord y,
    arbor_coord width,
    arbor_coord height,
    arbor_rect *out)
{
    arbor_rect result;
    arbor_coord edge;
    arbor_geometry_status status;

    if (out == NULL || width < 0 || height < 0) {
        return ARBOR_GEOMETRY_INVALID_ARGUMENT;
    }
    status = arbor_coord_add(x, width, &edge);
    if (status != ARBOR_GEOMETRY_OK) {
        return status;
    }
    status = arbor_coord_add(y, height, &edge);
    if (status != ARBOR_GEOMETRY_OK) {
        return status;
    }
    result.x = x;
    result.y = y;
    result.width = width;
    result.height = height;
    *out = result;
    return ARBOR_GEOMETRY_OK;
}

arbor_geometry_status arbor_rect_from_edges(
    arbor_coord left,
    arbor_coord top,
    arbor_coord right,
    arbor_coord bottom,
    arbor_rect *out)
{
    arbor_coord width;
    arbor_coord height;
    arbor_geometry_status status;

    if (out == NULL || right < left || bottom < top) {
        return ARBOR_GEOMETRY_INVALID_ARGUMENT;
    }
    status = arbor_coord_sub(right, left, &width);
    if (status != ARBOR_GEOMETRY_OK) {
        return status;
    }
    status = arbor_coord_sub(bottom, top, &height);
    if (status != ARBOR_GEOMETRY_OK) {
        return status;
    }
    return arbor_rect_make(left, top, width, height, out);
}

arbor_geometry_status arbor_point_translate(arbor_point point, arbor_point delta, arbor_point *out)
{
    arbor_point result;
    arbor_geometry_status status;

    if (out == NULL) {
        return ARBOR_GEOMETRY_INVALID_ARGUMENT;
    }
    status = arbor_coord_add(point.x, delta.x, &result.x);
    if (status != ARBOR_GEOMETRY_OK) {
        return status;
    }
    status = arbor_coord_add(point.y, delta.y, &result.y);
    if (status != ARBOR_GEOMETRY_OK) {
        return status;
    }
    *out = result;
    return ARBOR_GEOMETRY_OK;
}

arbor_geometry_status arbor_rect_translate(arbor_rect rect, arbor_point delta, arbor_rect *out)
{
    arbor_rect result;
    arbor_geometry_status status;
    arbor_coord edge;

    if (out == NULL) {
        return ARBOR_GEOMETRY_INVALID_ARGUMENT;
    }
    status = arbor_rect_edges(rect, &edge, &edge);
    if (status != ARBOR_GEOMETRY_OK) {
        return status;
    }
    status = arbor_coord_add(rect.x, delta.x, &result.x);
    if (status != ARBOR_GEOMETRY_OK) {
        return status;
    }
    status = arbor_coord_add(rect.y, delta.y, &result.y);
    if (status != ARBOR_GEOMETRY_OK) {
        return status;
    }
    result.width = rect.width;
    result.height = rect.height;
    status = arbor_rect_edges(result, &edge, &edge);
    if (status != ARBOR_GEOMETRY_OK) {
        return status;
    }
    *out = result;
    return ARBOR_GEOMETRY_OK;
}

bool arbor_rect_contains_point(arbor_rect rect, arbor_point point)
{
    arbor_coord right;
    arbor_coord bottom;

    if (arbor_rect_edges(rect, &right, &bottom) != ARBOR_GEOMETRY_OK) {
        return false;
    }
    if (rect.width == 0 || rect.height == 0) {
        return false;
    }
    return point.x >= rect.x && point.x < right && point.y >= rect.y && point.y < bottom;
}

arbor_geometry_status arbor_rect_intersection(
    arbor_rect left,
    arbor_rect right,
    arbor_rect *out,
    bool *intersects)
{
    arbor_coord left_right;
    arbor_coord left_bottom;
    arbor_coord right_right;
    arbor_coord right_bottom;
    arbor_coord x0;
    arbor_coord y0;
    arbor_coord x1;
    arbor_coord y1;
    arbor_rect result;
    arbor_geometry_status status;
    bool visible;

    if (out == NULL || intersects == NULL) {
        return ARBOR_GEOMETRY_INVALID_ARGUMENT;
    }
    status = arbor_rect_edges(left, &left_right, &left_bottom);
    if (status != ARBOR_GEOMETRY_OK) {
        return status;
    }
    status = arbor_rect_edges(right, &right_right, &right_bottom);
    if (status != ARBOR_GEOMETRY_OK) {
        return status;
    }

    x0 = (left.x > right.x) ? left.x : right.x;
    y0 = (left.y > right.y) ? left.y : right.y;
    x1 = (left_right < right_right) ? left_right : right_right;
    y1 = (left_bottom < right_bottom) ? left_bottom : right_bottom;
    visible = x1 > x0 && y1 > y0;

    if (!visible) {
        result.x = x0;
        result.y = y0;
        result.width = 0;
        result.height = 0;
    } else {
        status = arbor_rect_from_edges(x0, y0, x1, y1, &result);
        if (status != ARBOR_GEOMETRY_OK) {
            return status;
        }
    }

    *out = result;
    *intersects = visible;
    return ARBOR_GEOMETRY_OK;
}

arbor_geometry_status arbor_rect_union(arbor_rect left, arbor_rect right, arbor_rect *out)
{
    arbor_coord left_right;
    arbor_coord left_bottom;
    arbor_coord right_right;
    arbor_coord right_bottom;
    arbor_coord x0;
    arbor_coord y0;
    arbor_coord x1;
    arbor_coord y1;
    arbor_geometry_status status;

    if (out == NULL) {
        return ARBOR_GEOMETRY_INVALID_ARGUMENT;
    }
    status = arbor_rect_edges(left, &left_right, &left_bottom);
    if (status != ARBOR_GEOMETRY_OK) {
        return status;
    }
    status = arbor_rect_edges(right, &right_right, &right_bottom);
    if (status != ARBOR_GEOMETRY_OK) {
        return status;
    }

    x0 = (left.x < right.x) ? left.x : right.x;
    y0 = (left.y < right.y) ? left.y : right.y;
    x1 = (left_right > right_right) ? left_right : right_right;
    y1 = (left_bottom > right_bottom) ? left_bottom : right_bottom;
    return arbor_rect_from_edges(x0, y0, x1, y1, out);
}

arbor_geometry_status arbor_line_bounds(arbor_line line, arbor_rect *out)
{
    arbor_coord left;
    arbor_coord right;
    arbor_coord top;
    arbor_coord bottom;

    if (out == NULL) {
        return ARBOR_GEOMETRY_INVALID_ARGUMENT;
    }
    left = (line.start.x < line.end.x) ? line.start.x : line.end.x;
    right = (line.start.x > line.end.x) ? line.start.x : line.end.x;
    top = (line.start.y < line.end.y) ? line.start.y : line.end.y;
    bottom = (line.start.y > line.end.y) ? line.start.y : line.end.y;
    return arbor_rect_from_edges(left, top, right, bottom, out);
}

arbor_affine arbor_affine_identity(void)
{
    arbor_affine result = {
        ARBOR_COORD_ONE, 0,
        0, ARBOR_COORD_ONE,
        0, 0
    };
    return result;
}

arbor_affine arbor_affine_translation(arbor_coord tx, arbor_coord ty)
{
    arbor_affine result = arbor_affine_identity();
    result.tx = tx;
    result.ty = ty;
    return result;
}

arbor_affine arbor_affine_scale(arbor_coord sx, arbor_coord sy)
{
    arbor_affine result = {
        sx, 0,
        0, sy,
        0, 0
    };
    return result;
}

arbor_geometry_status arbor_affine_rotation_turns(arbor_coord turns, arbor_affine *out)
{
    const int64_t quarter = ARBOR_COORD_ONE / INT64_C(4);
    int64_t normalized;
    int64_t remainder;
    int64_t quadrant;
    int64_t x;
    int64_t y;
    int64_t z;
    int64_t cosine;
    int64_t sine;
    int64_t mapped_cosine;
    int64_t mapped_sine;
    unsigned int i;
    arbor_affine result;
    arbor_geometry_status status;

    if (out == NULL) {
        return ARBOR_GEOMETRY_INVALID_ARGUMENT;
    }

    normalized = turns % ARBOR_COORD_ONE;
    if (normalized < 0) {
        normalized += ARBOR_COORD_ONE;
    }
    quadrant = normalized / quarter;
    remainder = normalized % quarter;

    if (remainder == 0) {
        switch (quadrant) {
        case 0:
            cosine = ARBOR_COORD_ONE;
            sine = 0;
            break;
        case 1:
            cosine = 0;
            sine = ARBOR_COORD_ONE;
            break;
        case 2:
            cosine = -ARBOR_COORD_ONE;
            sine = 0;
            break;
        default:
            cosine = 0;
            sine = -ARBOR_COORD_ONE;
            break;
        }
    } else {
        x = ARBOR_CORDIC_K_Q62;
        y = 0;
        z = remainder;
        for (i = 0; i < ARBOR_CORDIC_ITERATIONS; ++i) {
            int64_t shifted_x = arbor_shift_right_arithmetic_i64(x, i);
            int64_t shifted_y = arbor_shift_right_arithmetic_i64(y, i);
            int64_t next_x;
            int64_t next_y;

            if (z >= 0) {
                next_x = x - shifted_y;
                next_y = y + shifted_x;
                z -= arbor_cordic_atan_turns_q32[i];
            } else {
                next_x = x + shifted_y;
                next_y = y - shifted_x;
                z += arbor_cordic_atan_turns_q32[i];
            }
            x = next_x;
            y = next_y;
        }
        status = arbor_round_div_i128_to_i64(
            (arbor_i128)x,
            ((arbor_i128)1) << ARBOR_CORDIC_TO_Q32_SHIFT,
            ARBOR_ROUND_NEAREST_EVEN,
            &cosine);
        if (status != ARBOR_GEOMETRY_OK) {
            return status;
        }
        status = arbor_round_div_i128_to_i64(
            (arbor_i128)y,
            ((arbor_i128)1) << ARBOR_CORDIC_TO_Q32_SHIFT,
            ARBOR_ROUND_NEAREST_EVEN,
            &sine);
        if (status != ARBOR_GEOMETRY_OK) {
            return status;
        }

        switch (quadrant) {
        case 0:
            mapped_cosine = cosine;
            mapped_sine = sine;
            break;
        case 1:
            mapped_cosine = -sine;
            mapped_sine = cosine;
            break;
        case 2:
            mapped_cosine = -cosine;
            mapped_sine = -sine;
            break;
        default:
            mapped_cosine = sine;
            mapped_sine = -cosine;
            break;
        }
        cosine = mapped_cosine;
        sine = mapped_sine;
    }

    result.a = cosine;
    result.b = sine;
    result.c = -sine;
    result.d = cosine;
    result.tx = 0;
    result.ty = 0;
    *out = result;
    return ARBOR_GEOMETRY_OK;
}

arbor_geometry_status arbor_affine_compose(arbor_affine left, arbor_affine right, arbor_affine *out)
{
    arbor_affine result;
    arbor_geometry_status status;

    if (out == NULL) {
        return ARBOR_GEOMETRY_INVALID_ARGUMENT;
    }
    status = arbor_affine_fused2(left.a, left.c, right.a, right.b, &result.a);
    if (status != ARBOR_GEOMETRY_OK) return status;
    status = arbor_affine_fused2(left.b, left.d, right.a, right.b, &result.b);
    if (status != ARBOR_GEOMETRY_OK) return status;
    status = arbor_affine_fused2(left.a, left.c, right.c, right.d, &result.c);
    if (status != ARBOR_GEOMETRY_OK) return status;
    status = arbor_affine_fused2(left.b, left.d, right.c, right.d, &result.d);
    if (status != ARBOR_GEOMETRY_OK) return status;
    status = arbor_affine_fused2_translate(left.a, left.c, right.tx, right.ty, left.tx, &result.tx);
    if (status != ARBOR_GEOMETRY_OK) return status;
    status = arbor_affine_fused2_translate(left.b, left.d, right.tx, right.ty, left.ty, &result.ty);
    if (status != ARBOR_GEOMETRY_OK) return status;

    *out = result;
    return ARBOR_GEOMETRY_OK;
}

arbor_geometry_status arbor_affine_invert(arbor_affine transform, arbor_affine *out)
{
    arbor_i128 ad = (arbor_i128)transform.a * (arbor_i128)transform.d;
    arbor_i128 bc = (arbor_i128)transform.b * (arbor_i128)transform.c;
    arbor_i128 determinant;
    arbor_affine result;
    arbor_coord b_unnegated;
    arbor_coord c_unnegated;
    arbor_coord translated_x;
    arbor_coord translated_y;
    arbor_geometry_status status;

    if (out == NULL) {
        return ARBOR_GEOMETRY_INVALID_ARGUMENT;
    }
    if (__builtin_sub_overflow(ad, bc, &determinant)) {
        return ARBOR_GEOMETRY_OVERFLOW;
    }
    if (determinant == 0) {
        return ARBOR_GEOMETRY_SINGULAR;
    }

    status = arbor_coord_from_i128_scaled((arbor_i128)transform.d * ARBOR_Q64_SCALE_I128, determinant, &result.a);
    if (status != ARBOR_GEOMETRY_OK) return status;
    status = arbor_coord_from_i128_scaled((arbor_i128)transform.b * ARBOR_Q64_SCALE_I128, determinant, &b_unnegated);
    if (status != ARBOR_GEOMETRY_OK) return status;
    status = arbor_coord_neg(b_unnegated, &result.b);
    if (status != ARBOR_GEOMETRY_OK) return status;
    status = arbor_coord_from_i128_scaled((arbor_i128)transform.c * ARBOR_Q64_SCALE_I128, determinant, &c_unnegated);
    if (status != ARBOR_GEOMETRY_OK) return status;
    status = arbor_coord_neg(c_unnegated, &result.c);
    if (status != ARBOR_GEOMETRY_OK) return status;
    status = arbor_coord_from_i128_scaled((arbor_i128)transform.a * ARBOR_Q64_SCALE_I128, determinant, &result.d);
    if (status != ARBOR_GEOMETRY_OK) return status;

    status = arbor_affine_fused2(result.a, result.c, transform.tx, transform.ty, &translated_x);
    if (status != ARBOR_GEOMETRY_OK) return status;
    status = arbor_affine_fused2(result.b, result.d, transform.tx, transform.ty, &translated_y);
    if (status != ARBOR_GEOMETRY_OK) return status;
    status = arbor_coord_neg(translated_x, &result.tx);
    if (status != ARBOR_GEOMETRY_OK) return status;
    status = arbor_coord_neg(translated_y, &result.ty);
    if (status != ARBOR_GEOMETRY_OK) return status;

    *out = result;
    return ARBOR_GEOMETRY_OK;
}

arbor_geometry_status arbor_affine_transform_point(arbor_affine transform, arbor_point point, arbor_point *out)
{
    arbor_point result;
    arbor_geometry_status status;

    if (out == NULL) {
        return ARBOR_GEOMETRY_INVALID_ARGUMENT;
    }
    status = arbor_affine_fused2_translate(transform.a, transform.c, point.x, point.y, transform.tx, &result.x);
    if (status != ARBOR_GEOMETRY_OK) return status;
    status = arbor_affine_fused2_translate(transform.b, transform.d, point.x, point.y, transform.ty, &result.y);
    if (status != ARBOR_GEOMETRY_OK) return status;
    *out = result;
    return ARBOR_GEOMETRY_OK;
}

arbor_geometry_status arbor_affine_transform_rect_bounds(arbor_affine transform, arbor_rect rect, arbor_rect *out)
{
    arbor_coord right;
    arbor_coord bottom;
    arbor_point corners[4];
    arbor_point transformed[4];
    arbor_coord min_x;
    arbor_coord max_x;
    arbor_coord min_y;
    arbor_coord max_y;
    arbor_geometry_status status;
    unsigned int i;

    if (out == NULL) {
        return ARBOR_GEOMETRY_INVALID_ARGUMENT;
    }
    status = arbor_rect_edges(rect, &right, &bottom);
    if (status != ARBOR_GEOMETRY_OK) return status;

    corners[0] = (arbor_point){rect.x, rect.y};
    corners[1] = (arbor_point){right, rect.y};
    corners[2] = (arbor_point){rect.x, bottom};
    corners[3] = (arbor_point){right, bottom};

    for (i = 0; i < 4u; ++i) {
        status = arbor_affine_transform_point(transform, corners[i], &transformed[i]);
        if (status != ARBOR_GEOMETRY_OK) return status;
    }

    min_x = max_x = transformed[0].x;
    min_y = max_y = transformed[0].y;
    for (i = 1; i < 4u; ++i) {
        if (transformed[i].x < min_x) min_x = transformed[i].x;
        if (transformed[i].x > max_x) max_x = transformed[i].x;
        if (transformed[i].y < min_y) min_y = transformed[i].y;
        if (transformed[i].y > max_y) max_y = transformed[i].y;
    }
    return arbor_rect_from_edges(min_x, min_y, max_x, max_y, out);
}

arbor_geometry_status arbor_clip_rect(arbor_rect current_clip, arbor_rect requested_clip, arbor_rect *out, bool *visible)
{
    return arbor_rect_intersection(current_clip, requested_clip, out, visible);
}

arbor_geometry_status arbor_device_scale_make(uint64_t numerator, uint64_t denominator, arbor_device_scale *out)
{
    arbor_device_scale result;
    uint64_t divisor;

    if (out == NULL || numerator == 0 || denominator == 0) {
        return ARBOR_GEOMETRY_INVALID_ARGUMENT;
    }
    divisor = arbor_u64_gcd(numerator, denominator);
    numerator /= divisor;
    denominator /= divisor;
    if (numerator > (uint64_t)INT64_MAX || denominator > (uint64_t)INT64_MAX) {
        return ARBOR_GEOMETRY_OVERFLOW;
    }
    result.numerator = numerator;
    result.denominator = denominator;
    *out = result;
    return ARBOR_GEOMETRY_OK;
}

arbor_geometry_status arbor_device_scale_compose(arbor_device_scale left, arbor_device_scale right, arbor_device_scale *out)
{
    uint64_t left_n;
    uint64_t left_d;
    uint64_t right_n;
    uint64_t right_d;
    uint64_t divisor;
    arbor_u128 numerator_product;
    arbor_u128 denominator_product;

    if (out == NULL || left.numerator == 0 || left.denominator == 0 ||
        right.numerator == 0 || right.denominator == 0) {
        return ARBOR_GEOMETRY_INVALID_ARGUMENT;
    }

    left_n = left.numerator;
    left_d = left.denominator;
    right_n = right.numerator;
    right_d = right.denominator;

    divisor = arbor_u64_gcd(left_n, right_d);
    left_n /= divisor;
    right_d /= divisor;
    divisor = arbor_u64_gcd(right_n, left_d);
    right_n /= divisor;
    left_d /= divisor;

    numerator_product = (arbor_u128)left_n * (arbor_u128)right_n;
    denominator_product = (arbor_u128)left_d * (arbor_u128)right_d;
    if (numerator_product > (arbor_u128)INT64_MAX ||
        denominator_product > (arbor_u128)INT64_MAX) {
        return ARBOR_GEOMETRY_OVERFLOW;
    }
    return arbor_device_scale_make(
        (uint64_t)numerator_product,
        (uint64_t)denominator_product,
        out);
}

arbor_geometry_status arbor_device_map_make(
    arbor_device_scale scale,
    arbor_coord origin_x,
    arbor_coord origin_y,
    arbor_device_map *out)
{
    arbor_device_map result;
    arbor_geometry_status status;

    if (out == NULL) {
        return ARBOR_GEOMETRY_INVALID_ARGUMENT;
    }
    status = arbor_device_scale_make(scale.numerator, scale.denominator, &result.scale);
    if (status != ARBOR_GEOMETRY_OK) return status;
    result.origin_x = origin_x;
    result.origin_y = origin_y;
    *out = result;
    return ARBOR_GEOMETRY_OK;
}

static arbor_geometry_status arbor_device_scale_coord(arbor_device_scale scale, arbor_coord value, arbor_coord *out)
{
    arbor_i128 scaled;

    if (out == NULL || scale.numerator == 0 || scale.denominator == 0 ||
        scale.numerator > (uint64_t)INT64_MAX || scale.denominator > (uint64_t)INT64_MAX) {
        return ARBOR_GEOMETRY_INVALID_ARGUMENT;
    }
    scaled = (arbor_i128)value * (arbor_i128)scale.numerator;
    return arbor_round_div_i128_to_i64(
        scaled,
        (arbor_i128)scale.denominator,
        ARBOR_ROUND_NEAREST_EVEN,
        out);
}

arbor_geometry_status arbor_device_map_point(
    arbor_device_map map,
    arbor_logical_point logical,
    arbor_device_point *out)
{
    arbor_device_point result;
    arbor_coord scaled_x;
    arbor_coord scaled_y;
    arbor_geometry_status status;

    if (out == NULL) {
        return ARBOR_GEOMETRY_INVALID_ARGUMENT;
    }
    status = arbor_device_scale_coord(map.scale, logical.x, &scaled_x);
    if (status != ARBOR_GEOMETRY_OK) return status;
    status = arbor_device_scale_coord(map.scale, logical.y, &scaled_y);
    if (status != ARBOR_GEOMETRY_OK) return status;
    status = arbor_coord_add(scaled_x, map.origin_x, &result.x);
    if (status != ARBOR_GEOMETRY_OK) return status;
    status = arbor_coord_add(scaled_y, map.origin_y, &result.y);
    if (status != ARBOR_GEOMETRY_OK) return status;
    *out = result;
    return ARBOR_GEOMETRY_OK;
}

arbor_geometry_status arbor_device_map_rect_bounds(
    arbor_device_map map,
    arbor_logical_rect logical,
    arbor_device_rect *out)
{
    arbor_coord logical_right;
    arbor_coord logical_bottom;
    arbor_device_point top_left;
    arbor_device_point bottom_right;
    arbor_device_rect result;
    arbor_geometry_status status;

    if (out == NULL || logical.width < 0 || logical.height < 0) {
        return ARBOR_GEOMETRY_INVALID_ARGUMENT;
    }
    status = arbor_coord_add(logical.x, logical.width, &logical_right);
    if (status != ARBOR_GEOMETRY_OK) return status;
    status = arbor_coord_add(logical.y, logical.height, &logical_bottom);
    if (status != ARBOR_GEOMETRY_OK) return status;

    status = arbor_device_map_point(
        map,
        (arbor_logical_point){logical.x, logical.y},
        &top_left);
    if (status != ARBOR_GEOMETRY_OK) return status;
    status = arbor_device_map_point(
        map,
        (arbor_logical_point){logical_right, logical_bottom},
        &bottom_right);
    if (status != ARBOR_GEOMETRY_OK) return status;
    status = arbor_coord_sub(bottom_right.x, top_left.x, &result.width);
    if (status != ARBOR_GEOMETRY_OK) return status;
    status = arbor_coord_sub(bottom_right.y, top_left.y, &result.height);
    if (status != ARBOR_GEOMETRY_OK) return status;
    if (result.width < 0 || result.height < 0) {
        return ARBOR_GEOMETRY_OVERFLOW;
    }
    result.x = top_left.x;
    result.y = top_left.y;
    *out = result;
    return ARBOR_GEOMETRY_OK;
}
