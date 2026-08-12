#define _POSIX_C_SOURCE 200809L

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "fixed_point_candidates.h"

#define SAMPLE_COUNT 1024u
#define ITERATIONS 600000u

static volatile int32_t sink32;
static volatile int64_t sink64;

static uint64_t now_ns(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        perror("clock_gettime");
        exit(2);
    }
    return (uint64_t)ts.tv_sec * UINT64_C(1000000000) + (uint64_t)ts.tv_nsec;
}

static double elapsed_per_op(uint64_t start, uint64_t end, uint64_t operations)
{
    return (double)(end - start) / (double)operations;
}

#define DEFINE_BENCH32(ID, LABEL, ONE, ADD_FN, MUL_FN, DIV_FN) \
static void bench_##ID(void) \
{ \
    int32_t values[SAMPLE_COUNT]; \
    int32_t coeff_a = (ONE) + (ONE) / 8; \
    int32_t coeff_b = (ONE) / 8; \
    int32_t divisor = (ONE) - (ONE) / 4; \
    int32_t translation = (ONE) / 3; \
    uint32_t i; \
    uint64_t start; \
    uint64_t end; \
    int32_t out = 0; \
    double add_ns; \
    double mul_ns; \
    double div_ns; \
    double affine_ns; \
    for (i = 0; i < SAMPLE_COUNT; ++i) { \
        int32_t integer = (int32_t)(i % 4096u) - INT32_C(2048); \
        values[i] = integer * (ONE) + (int32_t)(i & 31u) * ((ONE) / 64); \
    } \
    start = now_ns(); \
    for (i = 0; i < ITERATIONS; ++i) { \
        int32_t left = values[i & (SAMPLE_COUNT - 1u)]; \
        int32_t right = ((int32_t)(i & 15u) - INT32_C(8)) * ((ONE) / 64); \
        if (!ADD_FN(left, right, &out)) exit(3); \
    } \
    end = now_ns(); \
    sink32 = out; \
    add_ns = elapsed_per_op(start, end, ITERATIONS); \
    start = now_ns(); \
    for (i = 0; i < ITERATIONS; ++i) { \
        int32_t left = values[i & (SAMPLE_COUNT - 1u)]; \
        if (!MUL_FN(left, coeff_a, &out)) exit(3); \
    } \
    end = now_ns(); \
    sink32 = out; \
    mul_ns = elapsed_per_op(start, end, ITERATIONS); \
    start = now_ns(); \
    for (i = 0; i < ITERATIONS; ++i) { \
        int32_t left = values[i & (SAMPLE_COUNT - 1u)]; \
        if (!DIV_FN(left, divisor, &out)) exit(3); \
    } \
    end = now_ns(); \
    sink32 = out; \
    div_ns = elapsed_per_op(start, end, ITERATIONS); \
    start = now_ns(); \
    for (i = 0; i < ITERATIONS; ++i) { \
        int32_t x = values[i & (SAMPLE_COUNT - 1u)]; \
        int32_t y = values[(i + 137u) & (SAMPLE_COUNT - 1u)]; \
        int32_t ax; \
        int32_t by; \
        int32_t sum; \
        if (!MUL_FN(x, coeff_a, &ax) || \
            !MUL_FN(y, coeff_b, &by) || \
            !ADD_FN(ax, by, &sum) || \
            !ADD_FN(sum, translation, &out)) exit(3); \
    } \
    end = now_ns(); \
    sink32 = out; \
    affine_ns = elapsed_per_op(start, end, ITERATIONS); \
    (void)printf(LABEL "\t%.6f\t%.6f\t%.6f\t%.6f\n", add_ns, mul_ns, div_ns, affine_ns); \
}

#define DEFINE_BENCH64(ID, LABEL, ONE, ADD_FN, MUL_FN, DIV_FN) \
static void bench_##ID(void) \
{ \
    int64_t values[SAMPLE_COUNT]; \
    int64_t coeff_a = (ONE) + (ONE) / 8; \
    int64_t coeff_b = (ONE) / 8; \
    int64_t divisor = (ONE) - (ONE) / 4; \
    int64_t translation = (ONE) / 3; \
    uint32_t i; \
    uint64_t start; \
    uint64_t end; \
    int64_t out = 0; \
    double add_ns; \
    double mul_ns; \
    double div_ns; \
    double affine_ns; \
    for (i = 0; i < SAMPLE_COUNT; ++i) { \
        int64_t integer = (int64_t)(i % 4096u) - INT64_C(2048); \
        values[i] = integer * (ONE) + (int64_t)(i & 31u) * ((ONE) / 64); \
    } \
    start = now_ns(); \
    for (i = 0; i < ITERATIONS; ++i) { \
        int64_t left = values[i & (SAMPLE_COUNT - 1u)]; \
        int64_t right = (int64_t)((int32_t)(i & 15u) - INT32_C(8)) * ((ONE) / 64); \
        if (!ADD_FN(left, right, &out)) exit(3); \
    } \
    end = now_ns(); \
    sink64 = out; \
    add_ns = elapsed_per_op(start, end, ITERATIONS); \
    start = now_ns(); \
    for (i = 0; i < ITERATIONS; ++i) { \
        int64_t left = values[i & (SAMPLE_COUNT - 1u)]; \
        if (!MUL_FN(left, coeff_a, &out)) exit(3); \
    } \
    end = now_ns(); \
    sink64 = out; \
    mul_ns = elapsed_per_op(start, end, ITERATIONS); \
    start = now_ns(); \
    for (i = 0; i < ITERATIONS; ++i) { \
        int64_t left = values[i & (SAMPLE_COUNT - 1u)]; \
        if (!DIV_FN(left, divisor, &out)) exit(3); \
    } \
    end = now_ns(); \
    sink64 = out; \
    div_ns = elapsed_per_op(start, end, ITERATIONS); \
    start = now_ns(); \
    for (i = 0; i < ITERATIONS; ++i) { \
        int64_t x = values[i & (SAMPLE_COUNT - 1u)]; \
        int64_t y = values[(i + 137u) & (SAMPLE_COUNT - 1u)]; \
        int64_t ax; \
        int64_t by; \
        int64_t sum; \
        if (!MUL_FN(x, coeff_a, &ax) || \
            !MUL_FN(y, coeff_b, &by) || \
            !ADD_FN(ax, by, &sum) || \
            !ADD_FN(sum, translation, &out)) exit(3); \
    } \
    end = now_ns(); \
    sink64 = out; \
    affine_ns = elapsed_per_op(start, end, ITERATIONS); \
    (void)printf(LABEL "\t%.6f\t%.6f\t%.6f\t%.6f\n", add_ns, mul_ns, div_ns, affine_ns); \
}

DEFINE_BENCH32(Q16_16, "Q16.16", ARBOR_Q16_16_ONE, arbor_q16_16_add, arbor_q16_16_mul, arbor_q16_16_div)
DEFINE_BENCH32(Q26_6, "Q26.6", ARBOR_Q26_6_ONE, arbor_q26_6_add, arbor_q26_6_mul, arbor_q26_6_div)
DEFINE_BENCH64(Q32_32, "Q32.32", ARBOR_Q32_32_ONE, arbor_q32_32_add, arbor_q32_32_mul, arbor_q32_32_div)
DEFINE_BENCH64(Q24_40, "Q24.40", ARBOR_Q24_40_ONE, arbor_q24_40_add, arbor_q24_40_mul, arbor_q24_40_div)

int main(void)
{
    (void)puts("candidate\tadd_ns\tmul_ns\tdiv_ns\taffine_ns");
    bench_Q16_16();
    bench_Q26_6();
    bench_Q32_32();
    bench_Q24_40();
    return 0;
}
