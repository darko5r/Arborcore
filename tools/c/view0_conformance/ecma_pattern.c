#include "ecma_pattern.h"

#include <errno.h>
#include <string.h>

static bool syntax_char(uint8_t b)
{
    return b == (uint8_t)'^' || b == (uint8_t)'$' || b == (uint8_t)'\\' ||
        b == (uint8_t)'.' || b == (uint8_t)'*' || b == (uint8_t)'+' ||
        b == (uint8_t)'?' || b == (uint8_t)'(' || b == (uint8_t)')' ||
        b == (uint8_t)'[' || b == (uint8_t)']' || b == (uint8_t)'{' ||
        b == (uint8_t)'}' || b == (uint8_t)'|' || b == (uint8_t)'/';
}

arbor_status arbor_view0_native_v1n3_parse_pattern_v(
    arbor_span source, arbor_view0_native_v1n3_ecma_result *result_out)
{
    if (result_out == NULL || (source.length != 0u && source.data == NULL))
        return arbor_status_from_native(-(int64_t)EINVAL);
    arbor_view0_native_v1n3_ecma_result result = {1u, 0u, 0u, 0u, 0u, 0u, 0u, 0u};
    uint8_t stack[ARBOR_VIEW0_NATIVE_V1N3_MAX_ECMA_NESTING];
    uint64_t depth = 0u; bool in_class = false; bool expect_atom = true;
    for (uint64_t i = 0u; i < source.length; ++i) {
        uint8_t b = source.data[i]; result.token_count += 1u; result.node_count += 1u;
        if (result.token_count > ARBOR_VIEW0_NATIVE_V1N3_MAX_ECMA_TOKENS) return arbor_status_from_native(-(int64_t)ENOSPC);
        if (b == (uint8_t)'\\') {
            if (i + 1u >= source.length) { result.accepted = 0u; result.error_offset = i; break; }
            i += 1u;
            if (source.data[i] == (uint8_t)'p' || source.data[i] == (uint8_t)'P') {
                if (i + 1u >= source.length || source.data[i + 1u] != (uint8_t)'{') {
                    result.accepted = 0u; result.error_offset = i; break;
                }
                i += 2u; const uint64_t start = i;
                while (i < source.length && source.data[i] != (uint8_t)'}') i += 1u;
                if (i >= source.length || i == start) { result.accepted = 0u; result.error_offset = start; break; }
            }
            expect_atom = false; continue;
        }
        if (b == (uint8_t)'[' && !in_class) {
            if (depth == ARBOR_VIEW0_NATIVE_V1N3_MAX_ECMA_NESTING)
                return arbor_status_from_native(-(int64_t)ENOSPC);
            in_class = true; stack[depth++] = b;
            if (depth > result.max_nesting) result.max_nesting = depth;
            expect_atom = true; continue;
        }
        if (b == (uint8_t)']' && in_class) { in_class = false; if (depth == 0u) { result.accepted = 0u; result.error_offset = i; break; } depth -= 1u; expect_atom = false; continue; }
        if (in_class) {
            if ((b == (uint8_t)'&' || b == (uint8_t)'-') && i + 1u < source.length && source.data[i + 1u] == b) i += 1u;
            expect_atom = false; continue;
        }
        if (b == (uint8_t)'(') { if (depth == ARBOR_VIEW0_NATIVE_V1N3_MAX_ECMA_NESTING) return arbor_status_from_native(-(int64_t)ENOSPC); stack[depth++] = b; if (depth > result.max_nesting) result.max_nesting = depth; expect_atom = true; continue; }
        if (b == (uint8_t)')') { if (depth == 0u || stack[depth - 1u] != (uint8_t)'(') { result.accepted = 0u; result.error_offset = i; break; } depth -= 1u; expect_atom = false; continue; }
        if (b == (uint8_t)'*' || b == (uint8_t)'+' || b == (uint8_t)'?' || b == (uint8_t)'{') {
            if (expect_atom) { result.accepted = 0u; result.error_offset = i; break; }
            if (b == (uint8_t)'{') {
                uint64_t j = i + 1u; while (j < source.length && source.data[j] >= (uint8_t)'0' && source.data[j] <= (uint8_t)'9') j += 1u;
                if (j == i + 1u) { result.accepted = 0u; result.error_offset = i; break; }
                if (j < source.length && source.data[j] == (uint8_t)',') { j += 1u; while (j < source.length && source.data[j] >= (uint8_t)'0' && source.data[j] <= (uint8_t)'9') j += 1u; }
                if (j >= source.length || source.data[j] != (uint8_t)'}') { result.accepted = 0u; result.error_offset = i; break; }
                i = j;
            }
            continue;
        }
        if (b == (uint8_t)'|') { expect_atom = true; continue; }
        if (b == (uint8_t)'/' || b == (uint8_t)'\n' || b == (uint8_t)'\r') { result.accepted = 0u; result.error_offset = i; break; }
        (void)syntax_char(b); expect_atom = false;
    }
    if (result.accepted != 0u && (depth != 0u || in_class || (expect_atom && source.length != 0u))) {
        result.accepted = 0u; result.error_offset = source.length == 0u ? 0u : source.length - 1u;
    }
    *result_out = result;
    return arbor_status_from_native(0);
}
