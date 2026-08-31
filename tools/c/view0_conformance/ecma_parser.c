#include "ecma_parser.h"

#include <errno.h>
#include <stddef.h>
#include <string.h>

static arbor_status ok(void) { return arbor_status_from_native(0); }
static bool ws(uint8_t b) { return b == 9u || b == 10u || b == 11u || b == 12u || b == 13u || b == 32u; }

static uint64_t skip_space_comments(arbor_span source, uint64_t at)
{
    for (;;) {
        while (at < source.length && ws(source.data[at])) at += 1u;
        if (at + 1u < source.length && source.data[at] == (uint8_t)'/' && source.data[at + 1u] == (uint8_t)'/') {
            at += 2u; while (at < source.length && source.data[at] != (uint8_t)'\n') at += 1u; continue;
        }
        if (at + 1u < source.length && source.data[at] == (uint8_t)'/' && source.data[at + 1u] == (uint8_t)'*') {
            at += 2u; while (at + 1u < source.length && !(source.data[at] == (uint8_t)'*' && source.data[at + 1u] == (uint8_t)'/')) at += 1u;
            if (at + 1u < source.length) at += 2u;
            continue;
        }
        return at;
    }
}

static bool at_word(arbor_span source, uint64_t at, const char *word, uint64_t *after_out)
{
    const size_t n = strlen(word);
    if (at > source.length || (uint64_t)n > source.length - at || memcmp(source.data + at, word, n) != 0) return false;
    if (at + (uint64_t)n < source.length) {
        const uint8_t b = source.data[at + (uint64_t)n];
        if ((b >= (uint8_t)'a' && b <= (uint8_t)'z') || (b >= (uint8_t)'A' && b <= (uint8_t)'Z') ||
            (b >= (uint8_t)'0' && b <= (uint8_t)'9') || b == (uint8_t)'_' || b == (uint8_t)'$') return false;
    }
    if (after_out != NULL) *after_out = at + (uint64_t)n;
    return true;
}

static void from_lex(const arbor_view0_native_v1n3_lex_result *lex,
                     arbor_view0_native_v1n3_ecma_result *result)
{
    result->accepted = lex->accepted;
    result->error_offset = lex->error_offset;
    result->token_count = lex->token_count;
    result->node_count = lex->token_count;
    result->max_nesting = lex->max_nesting;
}

arbor_status arbor_view0_native_v1n3_parse_function_body(
    arbor_span source, arbor_view0_native_v1n3_ecma_result *result_out)
{
    if (result_out == NULL) return arbor_status_from_native(-(int64_t)EINVAL);
    arbor_view0_native_v1n3_lex_result lex = {0};
    arbor_status status = arbor_view0_native_v1n3_lex(source, &lex);
    if (status.native != 0) return status;
    arbor_view0_native_v1n3_ecma_result result = {0}; from_lex(&lex, &result);
    uint64_t bad = 0u;
    if (result.accepted != 0u && (arbor_view0_native_v1n3_span_contains_ascii(source, "import.meta", &bad) ||
        arbor_view0_native_v1n3_span_contains_ascii(source, "await ", &bad) ||
        arbor_view0_native_v1n3_span_contains_ascii(source, "yield ", &bad))) {
        result.accepted = 0u; result.error_offset = bad;
    }
    if (result.node_count > ARBOR_VIEW0_NATIVE_V1N3_MAX_ECMA_NODES) return arbor_status_from_native(-(int64_t)ENOSPC);
    *result_out = result; return ok();
}

arbor_status arbor_view0_native_v1n3_parse_constructor_subset(
    arbor_span source, arbor_view0_native_v1n3_ecma_result *result_out)
{
    if (result_out == NULL) return arbor_status_from_native(-(int64_t)EINVAL);
    arbor_view0_native_v1n3_ecma_result result = {0};
    arbor_status status = arbor_view0_native_v1n3_parse_function_body(source, &result);
    if (status.native != 0) return status;
    if (result.accepted == 0u) { *result_out = result; return ok(); }

    uint64_t at = skip_space_comments(source, 0u);
    /* Directive prologues are permitted before the first statement. */
    while (at < source.length && (source.data[at] == (uint8_t)'\'' || source.data[at] == (uint8_t)'"')) {
        const uint8_t quote = source.data[at++];
        while (at < source.length && source.data[at] != quote) {
            if (source.data[at] == (uint8_t)'\\' && at + 1u < source.length) at += 2u; else at += 1u;
        }
        if (at >= source.length) break;
        at += 1u; at = skip_space_comments(source, at);
        if (at < source.length && source.data[at] == (uint8_t)';') at += 1u;
        at = skip_space_comments(source, at);
    }

    uint64_t after = 0u;
    if (!at_word(source, at, "super", &after)) {
        result.accepted = 0u; result.error_offset = at;
    } else {
        after = skip_space_comments(source, after);
        if (after >= source.length || source.data[after] != (uint8_t)'(') {
            result.accepted = 0u; result.error_offset = after;
        } else {
            after = skip_space_comments(source, after + 1u);
            if (after >= source.length || source.data[after] != (uint8_t)')') {
                result.accepted = 0u; result.error_offset = after;
            } else {
                result.constructor_super_first = 1u;
            }
        }
    }

    uint64_t bad = 0u;
    if (result.accepted != 0u &&
        (arbor_view0_native_v1n3_span_contains_ascii(source, "document.write", &bad) ||
         arbor_view0_native_v1n3_span_contains_ascii(source, "document.open", &bad))) {
        result.accepted = 0u; result.error_offset = bad;
    }

    if (result.accepted != 0u) {
        uint64_t search = 0u;
        while (search < source.length) {
            uint64_t relative = 0u;
            const arbor_span tail = {source.data + search, source.length - search};
            if (!arbor_view0_native_v1n3_span_contains_ascii(tail, "return", &relative)) break;
            const uint64_t found = search + relative;
            uint64_t end = 0u;
            if (!at_word(source, found, "return", &end)) { search = found + 1u; continue; }
            end = skip_space_comments(source, end);
            if (end >= source.length || source.data[end] == (uint8_t)';' || source.data[end] == (uint8_t)'}' || source.data[end] == (uint8_t)'\n') {
                result.constructor_return_kind = 1u;
            } else {
                uint64_t after_this = 0u;
                if (!at_word(source, end, "this", &after_this)) {
                    result.accepted = 0u; result.error_offset = end; break;
                }
                after_this = skip_space_comments(source, after_this);
                if (after_this < source.length && source.data[after_this] != (uint8_t)';' && source.data[after_this] != (uint8_t)'}') {
                    result.accepted = 0u; result.error_offset = after_this; break;
                }
                result.constructor_return_kind = 2u;
            }
            search = end + 1u;
        }
    }
    *result_out = result; return ok();
}
