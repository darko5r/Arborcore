#include "ecma_unicode.h"

static bool continuation(uint8_t byte)
{
    return byte >= UINT8_C(0x80) && byte <= UINT8_C(0xbf);
}

bool arbor_view0_native_v1n3_utf8_scalar(
    arbor_span source, uint64_t offset, uint32_t *scalar_out, uint64_t *width_out)
{
    if (scalar_out == NULL || width_out == NULL || offset >= source.length) return false;
    const uint8_t b0 = source.data[offset];
    if (b0 < UINT8_C(0x80)) {
        *scalar_out = b0; *width_out = 1u; return true;
    }
    if (b0 >= UINT8_C(0xc2) && b0 <= UINT8_C(0xdf) &&
        source.length - offset >= 2u && continuation(source.data[offset + 1u])) {
        *scalar_out = ((uint32_t)(b0 & UINT8_C(0x1f)) << 6u) |
            (uint32_t)(source.data[offset + 1u] & UINT8_C(0x3f));
        *width_out = 2u; return true;
    }
    if (b0 >= UINT8_C(0xe0) && b0 <= UINT8_C(0xef) && source.length - offset >= 3u &&
        continuation(source.data[offset + 1u]) && continuation(source.data[offset + 2u])) {
        const uint8_t b1 = source.data[offset + 1u];
        if ((b0 == UINT8_C(0xe0) && b1 < UINT8_C(0xa0)) ||
            (b0 == UINT8_C(0xed) && b1 >= UINT8_C(0xa0))) return false;
        *scalar_out = ((uint32_t)(b0 & UINT8_C(0x0f)) << 12u) |
            ((uint32_t)(b1 & UINT8_C(0x3f)) << 6u) |
            (uint32_t)(source.data[offset + 2u] & UINT8_C(0x3f));
        *width_out = 3u; return true;
    }
    if (b0 >= UINT8_C(0xf0) && b0 <= UINT8_C(0xf4) && source.length - offset >= 4u &&
        continuation(source.data[offset + 1u]) && continuation(source.data[offset + 2u]) &&
        continuation(source.data[offset + 3u])) {
        const uint8_t b1 = source.data[offset + 1u];
        if ((b0 == UINT8_C(0xf0) && b1 < UINT8_C(0x90)) ||
            (b0 == UINT8_C(0xf4) && b1 >= UINT8_C(0x90))) return false;
        *scalar_out = ((uint32_t)(b0 & UINT8_C(0x07)) << 18u) |
            ((uint32_t)(b1 & UINT8_C(0x3f)) << 12u) |
            ((uint32_t)(source.data[offset + 2u] & UINT8_C(0x3f)) << 6u) |
            (uint32_t)(source.data[offset + 3u] & UINT8_C(0x3f));
        *width_out = 4u; return true;
    }
    return false;
}

bool arbor_view0_native_v1n3_identifier_start(uint32_t scalar)
{
    return scalar == (uint32_t)'$' || scalar == (uint32_t)'_' ||
        (scalar >= (uint32_t)'A' && scalar <= (uint32_t)'Z') ||
        (scalar >= (uint32_t)'a' && scalar <= (uint32_t)'z') || scalar >= UINT32_C(0x80);
}

bool arbor_view0_native_v1n3_identifier_continue(uint32_t scalar)
{
    return arbor_view0_native_v1n3_identifier_start(scalar) ||
        (scalar >= (uint32_t)'0' && scalar <= (uint32_t)'9') ||
        scalar == UINT32_C(0x200c) || scalar == UINT32_C(0x200d);
}
