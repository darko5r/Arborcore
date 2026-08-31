#include "ecma_lexer.h"
#include "ecma_unicode.h"

#include <errno.h>
#include <stddef.h>
#include <string.h>

static arbor_status status_value(int value) { return arbor_status_from_native(-(int64_t)value); }
static arbor_status ok(void) { return arbor_status_from_native(0); }
static bool ws(uint8_t b) { return b == 9u || b == 10u || b == 11u || b == 12u || b == 13u || b == 32u; }

bool arbor_view0_native_v1n3_span_contains_ascii(
    arbor_span source, const char *needle, uint64_t *offset_out)
{
    if (needle == NULL) return false;
    const size_t n = strlen(needle);
    if (n == 0u || (uint64_t)n > source.length) return false;
    for (uint64_t i = 0u; i <= source.length - (uint64_t)n; ++i) {
        if (memcmp(source.data + i, needle, n) == 0) {
            if (offset_out != NULL) *offset_out = i;
            return true;
        }
    }
    return false;
}

arbor_status arbor_view0_native_v1n3_lex(
    arbor_span source, arbor_view0_native_v1n3_lex_result *result_out)
{
    if (result_out == NULL || (source.length != 0u && source.data == NULL)) return status_value(EINVAL);
    arbor_view0_native_v1n3_lex_result result = {1u, 0u, 0u, 0u, UINT64_MAX, UINT64_MAX};
    uint8_t stack[ARBOR_VIEW0_NATIVE_V1N3_MAX_ECMA_NESTING];
    uint64_t depth = 0u;
    uint64_t i = 0u;
    while (i < source.length) {
        const uint8_t b = source.data[i];
        if (ws(b)) { i += 1u; continue; }
        if (result.first_token_offset == UINT64_MAX) result.first_token_offset = i;
        if (b == (uint8_t)'/' && i + 1u < source.length && source.data[i + 1u] == (uint8_t)'/') {
            i += 2u; while (i < source.length && source.data[i] != (uint8_t)'\n') i += 1u; continue;
        }
        if (b == (uint8_t)'/' && i + 1u < source.length && source.data[i + 1u] == (uint8_t)'*') {
            const uint64_t start = i; i += 2u;
            while (i + 1u < source.length && !(source.data[i] == (uint8_t)'*' && source.data[i + 1u] == (uint8_t)'/')) i += 1u;
            if (i + 1u >= source.length) { result.accepted = 0u; result.error_offset = start; break; }
            i += 2u; continue;
        }
        if (b == (uint8_t)'\'' || b == (uint8_t)'"' || b == (uint8_t)'`') {
            const uint8_t quote = b; const uint64_t start = i++; bool closed = false;
            while (i < source.length) {
                if (source.data[i] == (uint8_t)'\\') { i += source.length - i >= 2u ? 2u : 1u; continue; }
                if (source.data[i] == quote) { i += 1u; closed = true; break; }
                if (quote != (uint8_t)'`' && (source.data[i] == (uint8_t)'\n' || source.data[i] == (uint8_t)'\r')) break;
                uint32_t scalar = 0u; uint64_t width = 0u;
                if (!arbor_view0_native_v1n3_utf8_scalar(source, i, &scalar, &width)) break;
                (void)scalar; i += width;
            }
            if (!closed) { result.accepted = 0u; result.error_offset = start; break; }
            result.token_count += 1u; continue;
        }
        if (b == (uint8_t)'(' || b == (uint8_t)'[' || b == (uint8_t)'{') {
            if (depth == ARBOR_VIEW0_NATIVE_V1N3_MAX_ECMA_NESTING) return status_value(ENOSPC);
            stack[depth++] = b;
            if (depth > result.max_nesting) result.max_nesting = depth;
            result.token_count += 1u; i += 1u; continue;
        }
        if (b == (uint8_t)')' || b == (uint8_t)']' || b == (uint8_t)'}') {
            const uint8_t expected = b == (uint8_t)')' ? (uint8_t)'(' : b == (uint8_t)']' ? (uint8_t)'[' : (uint8_t)'{';
            if (depth == 0u || stack[depth - 1u] != expected) { result.accepted = 0u; result.error_offset = i; break; }
            depth -= 1u; result.token_count += 1u; i += 1u; continue;
        }
        if (b == (uint8_t)';' && depth == 0u && result.first_statement_end == UINT64_MAX) result.first_statement_end = i + 1u;
        uint32_t scalar = 0u; uint64_t width = 0u;
        if (!arbor_view0_native_v1n3_utf8_scalar(source, i, &scalar, &width)) {
            result.accepted = 0u; result.error_offset = i; break;
        }
        if (arbor_view0_native_v1n3_identifier_start(scalar)) {
            i += width;
            while (i < source.length) {
                if (!arbor_view0_native_v1n3_utf8_scalar(source, i, &scalar, &width) ||
                    !arbor_view0_native_v1n3_identifier_continue(scalar)) break;
                i += width;
            }
        } else {
            i += width;
        }
        result.token_count += 1u;
        if (result.token_count > ARBOR_VIEW0_NATIVE_V1N3_MAX_ECMA_TOKENS) return status_value(ENOSPC);
    }
    if (result.accepted != 0u && depth != 0u) {
        result.accepted = 0u; result.error_offset = source.length == 0u ? 0u : source.length - 1u;
    }
    if (result.first_token_offset == UINT64_MAX) result.first_token_offset = source.length;
    if (result.first_statement_end == UINT64_MAX) result.first_statement_end = source.length;
    *result_out = result;
    return ok();
}
