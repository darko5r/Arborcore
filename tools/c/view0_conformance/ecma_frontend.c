#include "ecma_frontend.h"

#include "ecma_early_errors.h"
#include "ecma_parser.h"
#include "ecma_pattern.h"

#include <errno.h>

arbor_status arbor_view0_native_v1n3_ecma_parse(
    uint64_t operation,
    arbor_span source,
    void *support_arena,
    arbor_view0_native_v1n3_ecma_result *result_out)
{
    if (result_out == NULL || support_arena == NULL ||
        (source.length != 0u && source.data == NULL)) {
        return arbor_status_from_native(-(int64_t)EINVAL);
    }
    arbor_view0_native_v1n3_ecma_result candidate = {0};
    arbor_status status;
    switch (operation) {
        case ARBOR_VIEW0_NATIVE_V1N3_ECMA_CONSTRUCTOR_SUBSET:
            status = arbor_view0_native_v1n3_parse_constructor_subset(source, &candidate);
            break;
        case ARBOR_VIEW0_NATIVE_V1N3_ECMA_FUNCTION_BODY:
            status = arbor_view0_native_v1n3_parse_function_body(source, &candidate);
            break;
        case ARBOR_VIEW0_NATIVE_V1N3_ECMA_PATTERN_V:
            status = arbor_view0_native_v1n3_parse_pattern_v(source, &candidate);
            break;
        default:
            return arbor_status_from_native(-(int64_t)EINVAL);
    }
    if (status.native != 0) return status;
    arbor_view0_native_v1n3_apply_early_errors(operation, source, &candidate);
    *result_out = candidate;
    return arbor_status_from_native(0);
}
