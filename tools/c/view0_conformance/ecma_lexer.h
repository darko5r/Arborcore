#ifndef ARBORCORE_VIEW0_CONFORMANCE_ECMA_LEXER_H
#define ARBORCORE_VIEW0_CONFORMANCE_ECMA_LEXER_H

#include <arborcore/view0_conformance/native.h>

#include <stdbool.h>
#include <stdint.h>

typedef struct arbor_view0_native_v1n3_lex_result {
    uint64_t accepted;
    uint64_t error_offset;
    uint64_t token_count;
    uint64_t max_nesting;
    uint64_t first_token_offset;
    uint64_t first_statement_end;
} arbor_view0_native_v1n3_lex_result;

arbor_status arbor_view0_native_v1n3_lex(
    arbor_span source, arbor_view0_native_v1n3_lex_result *result_out);

bool arbor_view0_native_v1n3_span_contains_ascii(
    arbor_span source, const char *needle, uint64_t *offset_out);

#endif
