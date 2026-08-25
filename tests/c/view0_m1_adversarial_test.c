#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <arborcore/view.h>

static int fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    return 1;
}

static int expect_invalid(const uint8_t *data, uint64_t length)
{
    arbor_status status = arbor_view_utf8_validate((arbor_span){data, length});
    return status.native == -EILSEQ && status.code == ARBOR_STATUS_NATIVE_ERROR ? 0 : 1;
}

int main(void)
{
    static const uint8_t stray_cont[] = {0x80u};
    static const uint8_t c0[] = {0xc0u, 0x80u};
    static const uint8_t c1[] = {0xc1u, 0xbfu};
    static const uint8_t truncated2[] = {0xc2u};
    static const uint8_t bad_cont2[] = {0xc2u, 0x7fu};
    static const uint8_t overlong3[] = {0xe0u, 0x80u, 0x80u};
    static const uint8_t surrogate_first[] = {0xedu, 0xa0u, 0x80u};
    static const uint8_t surrogate_last[] = {0xedu, 0xbfu, 0xbfu};
    static const uint8_t truncated3[] = {0xe2u, 0x82u};
    static const uint8_t bad_cont3[] = {0xe2u, 0x28u, 0xa1u};
    static const uint8_t overlong4[] = {0xf0u, 0x80u, 0x80u, 0x80u};
    static const uint8_t above_max[] = {0xf4u, 0x90u, 0x80u, 0x80u};
    static const uint8_t f5[] = {0xf5u, 0x80u, 0x80u, 0x80u};
    static const uint8_t truncated4[] = {0xf0u, 0x90u, 0x80u};
    static const uint8_t five_byte[] = {0xf8u, 0x88u, 0x80u, 0x80u, 0x80u};
    static const uint8_t ff[] = {0xffu};

    const struct {
        const uint8_t *data;
        uint64_t length;
    } invalid[] = {
        {stray_cont, sizeof(stray_cont)},
        {c0, sizeof(c0)},
        {c1, sizeof(c1)},
        {truncated2, sizeof(truncated2)},
        {bad_cont2, sizeof(bad_cont2)},
        {overlong3, sizeof(overlong3)},
        {surrogate_first, sizeof(surrogate_first)},
        {surrogate_last, sizeof(surrogate_last)},
        {truncated3, sizeof(truncated3)},
        {bad_cont3, sizeof(bad_cont3)},
        {overlong4, sizeof(overlong4)},
        {above_max, sizeof(above_max)},
        {f5, sizeof(f5)},
        {truncated4, sizeof(truncated4)},
        {five_byte, sizeof(five_byte)},
        {ff, sizeof(ff)}
    };

    for (size_t i = 0u; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        if (expect_invalid(invalid[i].data, invalid[i].length) != 0) {
            return fail("invalid UTF-8 sequence accepted");
        }
    }

    arbor_status bad_span = arbor_view_utf8_validate((arbor_span){NULL, 1u});
    if (bad_span.native != -EINVAL || bad_span.code != ARBOR_STATUS_INVALID_ARGUMENT) {
        return fail("invalid source span status");
    }

    uint8_t stable[] = {0x61u, 0xc3u, 0xa1u, 0xf0u, 0x9fu, 0x98u, 0x80u};
    uint8_t before[sizeof(stable)] = {0};
    memcpy(before, stable, sizeof(stable));
    if (arbor_view_utf8_validate((arbor_span){stable, sizeof(stable)}).native != 0 ||
        memcmp(before, stable, sizeof(stable)) != 0) {
        return fail("UTF-8 validation mutates input");
    }

    puts("PASS: VIEW0 M1 UTF-8 validator rejects overlong/surrogate/truncated/out-of-range encodings without mutation");
    return 0;
}
