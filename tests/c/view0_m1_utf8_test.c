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

static int expect_valid(const uint8_t *data, uint64_t length)
{
    arbor_status status = arbor_view_utf8_validate((arbor_span){data, length});
    return status.native == 0 ? 0 : 1;
}

int main(void)
{
    static const uint8_t ascii[] = "Arborcore HTML";
    static const uint8_t two_min[] = {0xc2u, 0x80u};
    static const uint8_t two_max[] = {0xdfu, 0xbfu};
    static const uint8_t three_min[] = {0xe0u, 0xa0u, 0x80u};
    static const uint8_t before_surrogate[] = {0xedu, 0x9fu, 0xbfu};
    static const uint8_t after_surrogate[] = {0xeeu, 0x80u, 0x80u};
    static const uint8_t three_max[] = {0xefu, 0xbfu, 0xbfu};
    static const uint8_t four_min[] = {0xf0u, 0x90u, 0x80u, 0x80u};
    static const uint8_t four_max[] = {0xf4u, 0x8fu, 0xbfu, 0xbfu};
    static const uint8_t bom[] = {0xefu, 0xbbu, 0xbfu};
    static const uint8_t mixed[] = {
        'O', 'l', 0xc3u, 0xa1u, ' ',
        0xe2u, 0x82u, 0xacu, ' ',
        0xf0u, 0x9fu, 0x98u, 0x80u
    };

    if (arbor_view_utf8_validate((arbor_span){NULL, 0u}).native != 0 ||
        expect_valid(ascii, sizeof(ascii) - 1u) != 0 ||
        expect_valid(two_min, sizeof(two_min)) != 0 ||
        expect_valid(two_max, sizeof(two_max)) != 0 ||
        expect_valid(three_min, sizeof(three_min)) != 0 ||
        expect_valid(before_surrogate, sizeof(before_surrogate)) != 0 ||
        expect_valid(after_surrogate, sizeof(after_surrogate)) != 0 ||
        expect_valid(three_max, sizeof(three_max)) != 0 ||
        expect_valid(four_min, sizeof(four_min)) != 0 ||
        expect_valid(four_max, sizeof(four_max)) != 0 ||
        expect_valid(bom, sizeof(bom)) != 0 ||
        expect_valid(mixed, sizeof(mixed)) != 0) {
        return fail("valid UTF-8 boundary sequence");
    }

    for (uint32_t value = 0u; value <= 0xffu; ++value) {
        const uint8_t one[] = {(uint8_t)value};
        arbor_status status = arbor_view_utf8_validate((arbor_span){one, 1u});
        const bool should_be_valid = value <= 0x7fu;
        if ((status.native == 0) != should_be_valid) {
            return fail("single-byte UTF-8 classification");
        }
    }

    puts("PASS: VIEW0 M1 UTF-8 validator accepts exact scalar-value boundary encodings");
    return 0;
}
