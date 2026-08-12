#ifndef ARBORCORE_EXPERIMENTS_GEOMETRY_FIXED_POINT_CANDIDATES_H
#define ARBORCORE_EXPERIMENTS_GEOMETRY_FIXED_POINT_CANDIDATES_H

#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <stddef.h>

/*
 * G0-G1 experimental fixed-point candidates.
 *
 * This header is intentionally outside include/arborcore/. It is not a public
 * API and does not freeze any geometry representation.  All arithmetic is
 * checked and transactional: *out is written only when the result is
 * representable. Multiplication/division use round-to-nearest, ties-to-even.
 */

#define ARBOR_GEOMETRY_MIN_INTEGER_RANGE UINT64_C(1048576)
#define ARBOR_GEOMETRY_MIN_FRACTION_BITS 16u

#define ARBOR_Q16_16_FRACTION_BITS 16u
#define ARBOR_Q26_6_FRACTION_BITS   6u
#define ARBOR_Q32_32_FRACTION_BITS 32u
#define ARBOR_Q24_40_FRACTION_BITS 40u

#define ARBOR_Q16_16_ONE INT32_C(65536)
#define ARBOR_Q26_6_ONE  INT32_C(64)
#define ARBOR_Q32_32_ONE INT64_C(4294967296)
#define ARBOR_Q24_40_ONE INT64_C(1099511627776)

__extension__ typedef __int128 arbor_geometry_i128;

static inline int64_t
arbor_geometry_round_nearest_even_i64(int64_t numerator, int64_t denominator)
{
    int64_t quotient = numerator / denominator;
    int64_t remainder = numerator % denominator;
    uint64_t remainder_abs = (remainder < 0)
        ? (uint64_t)(-remainder)
        : (uint64_t)remainder;
    uint64_t denominator_u = (uint64_t)denominator;
    uint64_t twice_remainder = remainder_abs * UINT64_C(2);

    if (twice_remainder > denominator_u ||
        (twice_remainder == denominator_u && (quotient & INT64_C(1)) != 0)) {
        quotient += (numerator < 0) ? INT64_C(-1) : INT64_C(1);
    }
    return quotient;
}

static inline arbor_geometry_i128
arbor_geometry_round_nearest_even_i128(arbor_geometry_i128 numerator,
                                        arbor_geometry_i128 denominator)
{
    arbor_geometry_i128 quotient = numerator / denominator;
    arbor_geometry_i128 remainder = numerator % denominator;
    arbor_geometry_i128 remainder_abs = (remainder < 0) ? -remainder : remainder;
    arbor_geometry_i128 twice_remainder = remainder_abs * 2;

    if (twice_remainder > denominator ||
        (twice_remainder == denominator && (quotient & 1) != 0)) {
        quotient += (numerator < 0) ? -1 : 1;
    }
    return quotient;
}

static inline bool
arbor_geometry_q32_add(int32_t left, int32_t right, int32_t *out)
{
    int64_t sum;
    if (out == NULL) {
        return false;
    }
    sum = (int64_t)left + (int64_t)right;
    if (sum < (int64_t)INT32_MIN || sum > (int64_t)INT32_MAX) {
        return false;
    }
    *out = (int32_t)sum;
    return true;
}

static inline bool
arbor_geometry_q64_add(int64_t left, int64_t right, int64_t *out)
{
    if (out == NULL) {
        return false;
    }
    if ((right > 0 && left > INT64_MAX - right) ||
        (right < 0 && left < INT64_MIN - right)) {
        return false;
    }
    *out = left + right;
    return true;
}

static inline bool
arbor_geometry_q32_sub(int32_t left, int32_t right, int32_t *out)
{
    int64_t difference;
    if (out == NULL) {
        return false;
    }
    difference = (int64_t)left - (int64_t)right;
    if (difference < (int64_t)INT32_MIN || difference > (int64_t)INT32_MAX) {
        return false;
    }
    *out = (int32_t)difference;
    return true;
}

static inline bool
arbor_geometry_q64_sub(int64_t left, int64_t right, int64_t *out)
{
    if (out == NULL) {
        return false;
    }
    if ((right < 0 && left > INT64_MAX + right) ||
        (right > 0 && left < INT64_MIN + right)) {
        return false;
    }
    *out = left - right;
    return true;
}

static inline bool
arbor_geometry_q32_from_integer(int64_t integer,
                                 unsigned int fractional_bits,
                                 int32_t *out)
{
    int64_t scale = INT64_C(1) << fractional_bits;
    int64_t raw;
    if (out == NULL) {
        return false;
    }
    if (integer > (int64_t)INT32_MAX / scale ||
        integer < (int64_t)INT32_MIN / scale) {
        return false;
    }
    raw = integer * scale;
    *out = (int32_t)raw;
    return true;
}

static inline bool
arbor_geometry_q64_from_integer(int64_t integer,
                                 unsigned int fractional_bits,
                                 int64_t *out)
{
    arbor_geometry_i128 scale = ((arbor_geometry_i128)1) << fractional_bits;
    arbor_geometry_i128 raw;
    if (out == NULL) {
        return false;
    }
    raw = (arbor_geometry_i128)integer * scale;
    if (raw < (arbor_geometry_i128)INT64_MIN ||
        raw > (arbor_geometry_i128)INT64_MAX) {
        return false;
    }
    *out = (int64_t)raw;
    return true;
}

static inline bool
arbor_geometry_q32_mul(int32_t left,
                        int32_t right,
                        unsigned int fractional_bits,
                        int32_t *out)
{
    int64_t product;
    int64_t scale;
    int64_t rounded;
    if (out == NULL) {
        return false;
    }
    product = (int64_t)left * (int64_t)right;
    scale = INT64_C(1) << fractional_bits;
    rounded = arbor_geometry_round_nearest_even_i64(product, scale);
    if (rounded < (int64_t)INT32_MIN || rounded > (int64_t)INT32_MAX) {
        return false;
    }
    *out = (int32_t)rounded;
    return true;
}

static inline bool
arbor_geometry_q64_mul(int64_t left,
                        int64_t right,
                        unsigned int fractional_bits,
                        int64_t *out)
{
    arbor_geometry_i128 product;
    arbor_geometry_i128 scale;
    arbor_geometry_i128 rounded;
    if (out == NULL) {
        return false;
    }
    product = (arbor_geometry_i128)left * (arbor_geometry_i128)right;
    scale = ((arbor_geometry_i128)1) << fractional_bits;
    rounded = arbor_geometry_round_nearest_even_i128(product, scale);
    if (rounded < (arbor_geometry_i128)INT64_MIN ||
        rounded > (arbor_geometry_i128)INT64_MAX) {
        return false;
    }
    *out = (int64_t)rounded;
    return true;
}

static inline bool
arbor_geometry_q32_div(int32_t numerator,
                        int32_t denominator,
                        unsigned int fractional_bits,
                        int32_t *out)
{
    int64_t scaled_numerator;
    int64_t scale;
    int64_t rounded;
    if (out == NULL || denominator == 0) {
        return false;
    }
    scale = INT64_C(1) << fractional_bits;
    scaled_numerator = (int64_t)numerator * scale;
    rounded = arbor_geometry_round_nearest_even_i64(
        scaled_numerator, (int64_t)denominator);
    if (rounded < (int64_t)INT32_MIN || rounded > (int64_t)INT32_MAX) {
        return false;
    }
    *out = (int32_t)rounded;
    return true;
}

static inline bool
arbor_geometry_q64_div(int64_t numerator,
                        int64_t denominator,
                        unsigned int fractional_bits,
                        int64_t *out)
{
    arbor_geometry_i128 scaled_numerator;
    arbor_geometry_i128 scale;
    arbor_geometry_i128 rounded;
    if (out == NULL || denominator == 0) {
        return false;
    }
    scale = ((arbor_geometry_i128)1) << fractional_bits;
    scaled_numerator = (arbor_geometry_i128)numerator * scale;
    rounded = arbor_geometry_round_nearest_even_i128(
        scaled_numerator, (arbor_geometry_i128)denominator);
    if (rounded < (arbor_geometry_i128)INT64_MIN ||
        rounded > (arbor_geometry_i128)INT64_MAX) {
        return false;
    }
    *out = (int64_t)rounded;
    return true;
}

static inline int64_t
arbor_geometry_floor_raw_i64(int64_t raw, int64_t scale)
{
    int64_t quotient = raw / scale;
    int64_t remainder = raw % scale;
    if (raw < 0 && remainder != 0) {
        --quotient;
    }
    return quotient;
}

static inline int64_t
arbor_geometry_ceil_raw_i64(int64_t raw, int64_t scale)
{
    int64_t quotient = raw / scale;
    int64_t remainder = raw % scale;
    if (raw > 0 && remainder != 0) {
        ++quotient;
    }
    return quotient;
}

static inline int64_t
arbor_geometry_trunc_raw_i64(int64_t raw, int64_t scale)
{
    return raw / scale;
}

static inline int64_t
arbor_geometry_nearest_even_raw_i64(int64_t raw, int64_t scale)
{
    return arbor_geometry_round_nearest_even_i64(raw, scale);
}

static inline bool
arbor_q16_16_from_integer(int64_t integer, int32_t *out)
{
    return arbor_geometry_q32_from_integer(integer, ARBOR_Q16_16_FRACTION_BITS, out);
}

static inline bool
arbor_q26_6_from_integer(int64_t integer, int32_t *out)
{
    return arbor_geometry_q32_from_integer(integer, ARBOR_Q26_6_FRACTION_BITS, out);
}

static inline bool
arbor_q32_32_from_integer(int64_t integer, int64_t *out)
{
    return arbor_geometry_q64_from_integer(integer, ARBOR_Q32_32_FRACTION_BITS, out);
}

static inline bool
arbor_q24_40_from_integer(int64_t integer, int64_t *out)
{
    return arbor_geometry_q64_from_integer(integer, ARBOR_Q24_40_FRACTION_BITS, out);
}

static inline bool arbor_q16_16_add(int32_t a, int32_t b, int32_t *out)
{ return arbor_geometry_q32_add(a, b, out); }
static inline bool arbor_q26_6_add(int32_t a, int32_t b, int32_t *out)
{ return arbor_geometry_q32_add(a, b, out); }
static inline bool arbor_q32_32_add(int64_t a, int64_t b, int64_t *out)
{ return arbor_geometry_q64_add(a, b, out); }
static inline bool arbor_q24_40_add(int64_t a, int64_t b, int64_t *out)
{ return arbor_geometry_q64_add(a, b, out); }

static inline bool arbor_q16_16_sub(int32_t a, int32_t b, int32_t *out)
{ return arbor_geometry_q32_sub(a, b, out); }
static inline bool arbor_q26_6_sub(int32_t a, int32_t b, int32_t *out)
{ return arbor_geometry_q32_sub(a, b, out); }
static inline bool arbor_q32_32_sub(int64_t a, int64_t b, int64_t *out)
{ return arbor_geometry_q64_sub(a, b, out); }
static inline bool arbor_q24_40_sub(int64_t a, int64_t b, int64_t *out)
{ return arbor_geometry_q64_sub(a, b, out); }

static inline bool arbor_q16_16_mul(int32_t a, int32_t b, int32_t *out)
{ return arbor_geometry_q32_mul(a, b, ARBOR_Q16_16_FRACTION_BITS, out); }
static inline bool arbor_q26_6_mul(int32_t a, int32_t b, int32_t *out)
{ return arbor_geometry_q32_mul(a, b, ARBOR_Q26_6_FRACTION_BITS, out); }
static inline bool arbor_q32_32_mul(int64_t a, int64_t b, int64_t *out)
{ return arbor_geometry_q64_mul(a, b, ARBOR_Q32_32_FRACTION_BITS, out); }
static inline bool arbor_q24_40_mul(int64_t a, int64_t b, int64_t *out)
{ return arbor_geometry_q64_mul(a, b, ARBOR_Q24_40_FRACTION_BITS, out); }

static inline bool arbor_q16_16_div(int32_t a, int32_t b, int32_t *out)
{ return arbor_geometry_q32_div(a, b, ARBOR_Q16_16_FRACTION_BITS, out); }
static inline bool arbor_q26_6_div(int32_t a, int32_t b, int32_t *out)
{ return arbor_geometry_q32_div(a, b, ARBOR_Q26_6_FRACTION_BITS, out); }
static inline bool arbor_q32_32_div(int64_t a, int64_t b, int64_t *out)
{ return arbor_geometry_q64_div(a, b, ARBOR_Q32_32_FRACTION_BITS, out); }
static inline bool arbor_q24_40_div(int64_t a, int64_t b, int64_t *out)
{ return arbor_geometry_q64_div(a, b, ARBOR_Q24_40_FRACTION_BITS, out); }

#endif
