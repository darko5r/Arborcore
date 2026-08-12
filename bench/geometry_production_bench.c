#define _POSIX_C_SOURCE 200809L

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <arborcore/geometry.h>
#include "fixed_point_candidates.h"

#define SAMPLE_COUNT 1024u
#define ITERATIONS 500000u

static volatile int64_t sink64;
/* Volatile inputs prevent constant-specialized candidate code from being
 * compared against the separately compiled production API.  Both paths load
 * the same runtime operands once per measured block. */
static volatile int64_t runtime_coeff_a = ARBOR_COORD_ONE + ARBOR_COORD_ONE / 8;
static volatile int64_t runtime_coeff_b = ARBOR_COORD_ONE / 8;
static volatile int64_t runtime_divisor = ARBOR_COORD_ONE - ARBOR_COORD_ONE / 4;
static volatile int64_t runtime_translation = ARBOR_COORD_ONE / 3;

typedef struct geometry_bench_result {
    double add_ns;
    double mul_ns;
    double div_ns;
    double affine_ns;
} geometry_bench_result;

static uint64_t now_ns(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        perror("clock_gettime");
        exit(2);
    }
    return (uint64_t)ts.tv_sec * UINT64_C(1000000000) + (uint64_t)ts.tv_nsec;
}

static double per_op(uint64_t start, uint64_t end)
{
    return (double)(end - start) / (double)ITERATIONS;
}

static void fill_values(int64_t values[SAMPLE_COUNT])
{
    uint32_t i;
    for (i = 0; i < SAMPLE_COUNT; ++i) {
        int64_t integer = (int64_t)(i % 4096u) - INT64_C(2048);
        values[i] = integer * ARBOR_COORD_ONE + (int64_t)(i & 31u) * (ARBOR_COORD_ONE / 64);
    }
}

/*
 * G1 candidate reference for one affine axis with the same numerical
 * semantics as production: form the exact two-product sum plus translation
 * in 128 bits, then round once to Q32.32 using nearest-even.
 */
static bool candidate_affine_axis(
    int64_t left_a,
    int64_t left_b,
    int64_t right_a,
    int64_t right_b,
    int64_t translation,
    int64_t *out)
{
    arbor_geometry_i128 first;
    arbor_geometry_i128 second;
    arbor_geometry_i128 translated;
    arbor_geometry_i128 sum;
    arbor_geometry_i128 rounded;

    if (out == NULL) {
        return false;
    }
    first = (arbor_geometry_i128)left_a * (arbor_geometry_i128)right_a;
    second = (arbor_geometry_i128)left_b * (arbor_geometry_i128)right_b;
    translated = (arbor_geometry_i128)translation * (arbor_geometry_i128)ARBOR_COORD_ONE;
    if (__builtin_add_overflow(first, second, &sum) ||
        __builtin_add_overflow(sum, translated, &sum)) {
        return false;
    }
    rounded = arbor_geometry_round_nearest_even_i128(
        sum, (arbor_geometry_i128)ARBOR_COORD_ONE);
    if (rounded < (arbor_geometry_i128)INT64_MIN ||
        rounded > (arbor_geometry_i128)INT64_MAX) {
        return false;
    }
    *out = (int64_t)rounded;
    return true;
}

static geometry_bench_result bench_candidate(void)
{
    int64_t values[SAMPLE_COUNT];
    int64_t coeff_a = runtime_coeff_a;
    int64_t coeff_b = runtime_coeff_b;
    int64_t divisor = runtime_divisor;
    int64_t translation = runtime_translation;
    uint32_t i;
    uint64_t start;
    uint64_t end;
    int64_t out = 0;
    geometry_bench_result result;

    fill_values(values);
    start = now_ns();
    for (i = 0; i < ITERATIONS; ++i) {
        int64_t right = (int64_t)((int32_t)(i & 15u) - INT32_C(8)) * (ARBOR_COORD_ONE / 64);
        if (!arbor_q32_32_add(values[i & (SAMPLE_COUNT - 1u)], right, &out)) exit(3);
    }
    end = now_ns();
    sink64 = out;
    result.add_ns = per_op(start, end);

    start = now_ns();
    for (i = 0; i < ITERATIONS; ++i) {
        if (!arbor_q32_32_mul(values[i & (SAMPLE_COUNT - 1u)], coeff_a, &out)) exit(3);
    }
    end = now_ns();
    sink64 = out;
    result.mul_ns = per_op(start, end);

    start = now_ns();
    for (i = 0; i < ITERATIONS; ++i) {
        if (!arbor_q32_32_div(values[i & (SAMPLE_COUNT - 1u)], divisor, &out)) exit(3);
    }
    end = now_ns();
    sink64 = out;
    result.div_ns = per_op(start, end);

    start = now_ns();
    for (i = 0; i < ITERATIONS; ++i) {
        int64_t x = values[i & (SAMPLE_COUNT - 1u)];
        int64_t y = values[(i + 137u) & (SAMPLE_COUNT - 1u)];
        int64_t out_y;
        if (!candidate_affine_axis(coeff_a, coeff_b, x, y, translation, &out) ||
            !candidate_affine_axis(coeff_b, coeff_a, x, y, -translation, &out_y)) {
            exit(3);
        }
        out ^= out_y;
    }
    end = now_ns();
    sink64 = out;
    result.affine_ns = per_op(start, end);
    return result;
}

static geometry_bench_result bench_production(void)
{
    int64_t values[SAMPLE_COUNT];
    arbor_coord coeff_a = runtime_coeff_a;
    arbor_coord coeff_b = runtime_coeff_b;
    arbor_coord divisor = runtime_divisor;
    arbor_coord translation = runtime_translation;
    arbor_affine transform = {
        coeff_a, coeff_b,
        coeff_b, coeff_a,
        translation, -translation
    };
    uint32_t i;
    uint64_t start;
    uint64_t end;
    arbor_coord out = 0;
    arbor_point point_out;
    geometry_bench_result result;

    fill_values(values);
    start = now_ns();
    for (i = 0; i < ITERATIONS; ++i) {
        arbor_coord right = (int64_t)((int32_t)(i & 15u) - INT32_C(8)) * (ARBOR_COORD_ONE / 64);
        if (arbor_coord_add(values[i & (SAMPLE_COUNT - 1u)], right, &out) != ARBOR_GEOMETRY_OK) exit(4);
    }
    end = now_ns();
    sink64 = out;
    result.add_ns = per_op(start, end);

    start = now_ns();
    for (i = 0; i < ITERATIONS; ++i) {
        if (arbor_coord_mul(values[i & (SAMPLE_COUNT - 1u)], coeff_a, &out) != ARBOR_GEOMETRY_OK) exit(4);
    }
    end = now_ns();
    sink64 = out;
    result.mul_ns = per_op(start, end);

    start = now_ns();
    for (i = 0; i < ITERATIONS; ++i) {
        if (arbor_coord_div(values[i & (SAMPLE_COUNT - 1u)], divisor, &out) != ARBOR_GEOMETRY_OK) exit(4);
    }
    end = now_ns();
    sink64 = out;
    result.div_ns = per_op(start, end);

    start = now_ns();
    for (i = 0; i < ITERATIONS; ++i) {
        arbor_point point = {
            values[i & (SAMPLE_COUNT - 1u)],
            values[(i + 137u) & (SAMPLE_COUNT - 1u)]
        };
        if (arbor_affine_transform_point(transform, point, &point_out) != ARBOR_GEOMETRY_OK) exit(4);
    }
    end = now_ns();
    sink64 = point_out.x ^ point_out.y;
    result.affine_ns = per_op(start, end);
    return result;
}

static void print_result(const char *prefix, geometry_bench_result value)
{
    (void)printf("%s_add_ns\t%.6f\n", prefix, value.add_ns);
    (void)printf("%s_mul_ns\t%.6f\n", prefix, value.mul_ns);
    (void)printf("%s_div_ns\t%.6f\n", prefix, value.div_ns);
    (void)printf("%s_affine_ns\t%.6f\n", prefix, value.affine_ns);
}

int main(int argc, char **argv)
{
    geometry_bench_result candidate;
    geometry_bench_result production;
    bool production_first = argc > 1 && argv[1] != NULL && argv[1][0] == 'p';

    if (production_first) {
        production = bench_production();
        candidate = bench_candidate();
    } else {
        candidate = bench_candidate();
        production = bench_production();
    }

    print_result("candidate", candidate);
    print_result("production", production);
    return 0;
}
