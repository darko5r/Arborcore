#include <stdint.h>

__extension__ typedef unsigned __int128 arbor_wasm_u128;


#if defined(__clang__)
#define ARBOR_WASM_RUNTIME_OPAQUE __attribute__((noinline, optnone))
#else
#define ARBOR_WASM_RUNTIME_OPAQUE __attribute__((noinline))
#endif

typedef union arbor_wasm_u128_words {
    arbor_wasm_u128 all;
    struct {
        uint64_t low;
        uint64_t high;
    } words;
} arbor_wasm_u128_words;

static ARBOR_WASM_RUNTIME_OPAQUE uint64_t arbor_mul64_high(uint64_t left, uint64_t right)
{
    uint64_t left_low = (uint64_t)(uint32_t)left;
    uint64_t left_high = left >> 32;
    uint64_t right_low = (uint64_t)(uint32_t)right;
    uint64_t right_high = right >> 32;
    uint64_t p0 = left_low * right_low;
    uint64_t p1 = left_low * right_high;
    uint64_t p2 = left_high * right_low;
    uint64_t p3 = left_high * right_high;
    uint64_t middle = (p0 >> 32) + (uint64_t)(uint32_t)p1 + (uint64_t)(uint32_t)p2;

    return p3 + (p1 >> 32) + (p2 >> 32) + (middle >> 32);
}

ARBOR_WASM_RUNTIME_OPAQUE arbor_wasm_u128 __multi3(arbor_wasm_u128 left, arbor_wasm_u128 right)
{
    arbor_wasm_u128_words a;
    arbor_wasm_u128_words b;
    arbor_wasm_u128_words result;
    uint64_t low_high;

    a.all = left;
    b.all = right;
    result.words.low = a.words.low * b.words.low;
    low_high = arbor_mul64_high(a.words.low, b.words.low);
    result.words.high = low_high + a.words.high * b.words.low + a.words.low * b.words.high;
    return result.all;
}

static int arbor_u128_words_compare(arbor_wasm_u128_words left, arbor_wasm_u128_words right)
{
    if (left.words.high < right.words.high) return -1;
    if (left.words.high > right.words.high) return 1;
    if (left.words.low < right.words.low) return -1;
    if (left.words.low > right.words.low) return 1;
    return 0;
}

static arbor_wasm_u128_words arbor_u128_words_sub(arbor_wasm_u128_words left, arbor_wasm_u128_words right)
{
    arbor_wasm_u128_words result;
    uint64_t borrow = (left.words.low < right.words.low) ? UINT64_C(1) : UINT64_C(0);
    result.words.low = left.words.low - right.words.low;
    result.words.high = left.words.high - right.words.high - borrow;
    return result;
}

static arbor_wasm_u128_words arbor_u128_words_shift_left_one(arbor_wasm_u128_words value)
{
    arbor_wasm_u128_words result;
    result.words.high = (value.words.high << 1) | (value.words.low >> 63);
    result.words.low = value.words.low << 1;
    return result;
}

static uint64_t arbor_u128_words_bit(arbor_wasm_u128_words value, unsigned int bit)
{
    if (bit >= 64u) {
        return (value.words.high >> (bit - 64u)) & UINT64_C(1);
    }
    return (value.words.low >> bit) & UINT64_C(1);
}

static void arbor_u128_words_set_bit(arbor_wasm_u128_words *value, unsigned int bit)
{
    if (bit >= 64u) {
        value->words.high |= UINT64_C(1) << (bit - 64u);
    } else {
        value->words.low |= UINT64_C(1) << bit;
    }
}

ARBOR_WASM_RUNTIME_OPAQUE arbor_wasm_u128 __udivti3(arbor_wasm_u128 numerator, arbor_wasm_u128 denominator)
{
    arbor_wasm_u128_words n;
    arbor_wasm_u128_words d;
    arbor_wasm_u128_words quotient;
    arbor_wasm_u128_words remainder;
    unsigned int i;

    n.all = numerator;
    d.all = denominator;
    quotient.words.low = 0;
    quotient.words.high = 0;
    remainder.words.low = 0;
    remainder.words.high = 0;

    for (i = 128u; i > 0u; --i) {
        unsigned int bit = i - 1u;
        remainder = arbor_u128_words_shift_left_one(remainder);
        remainder.words.low |= arbor_u128_words_bit(n, bit);
        if (arbor_u128_words_compare(remainder, d) >= 0) {
            remainder = arbor_u128_words_sub(remainder, d);
            arbor_u128_words_set_bit(&quotient, bit);
        }
    }
    return quotient.all;
}
