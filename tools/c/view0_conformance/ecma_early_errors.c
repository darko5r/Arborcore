#include "ecma_early_errors.h"

#include "ecma_lexer.h"

void arbor_view0_native_v1n3_apply_early_errors(
    uint64_t operation, arbor_span source,
    arbor_view0_native_v1n3_ecma_result *result)
{
    if (result == NULL || result->accepted == 0u) return;
    uint64_t offset = 0u;
    if (operation == ARBOR_VIEW0_NATIVE_V1N3_ECMA_FUNCTION_BODY &&
        arbor_view0_native_v1n3_span_contains_ascii(source, "super(", &offset)) {
        result->accepted = 0u; result->error_offset = offset;
    }
    if (operation == ARBOR_VIEW0_NATIVE_V1N3_ECMA_CONSTRUCTOR_SUBSET &&
        arbor_view0_native_v1n3_span_contains_ascii(source, "super.", &offset)) {
        result->accepted = 0u; result->error_offset = offset;
    }
}
