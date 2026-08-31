#include "g16.h"

#include "ecma_frontend.h"

#include <errno.h>

static bool event_attribute(arbor_span name)
{
    return name.length > 2u && (name.data[0] == (uint8_t)'o' || name.data[0] == (uint8_t)'O') &&
        (name.data[1] == (uint8_t)'n' || name.data[1] == (uint8_t)'N');
}

arbor_status arbor_view0_native_v1n3_g16_validate_element(
    arbor_view0_native_v1n3_context *c, const arbor_view0_native_v1n3_element *e)
{
    if (c == NULL || e == NULL) return arbor_status_from_native(-(int64_t)EINVAL);
    if (e->template_opaque) return arbor_status_from_native(0);
    for (const arbor_view0_native_v1n3_attr *a = e->attributes; a != NULL; a = a->next) {
        uint64_t operation = 0u; uint16_t ordinal = 0u;
        if (event_attribute(a->name)) { operation = ARBOR_VIEW0_NATIVE_V1N3_ECMA_FUNCTION_BODY; ordinal = 1u; }
        else if (arbor_view0_native_v1n3_ascii_case_equal(e->name, "input") &&
                 arbor_view0_native_v1n3_ascii_case_equal(a->name, "pattern")) {
            operation = ARBOR_VIEW0_NATIVE_V1N3_ECMA_PATTERN_V; ordinal = 2u;
        }
        if (operation == 0u) continue;
        arbor_view0_native_v1n3_ecma_result result = {0};
        arbor_status status = arbor_view0_native_v1n3_ecma_parse(operation, a->value, c->arena, &result);
        if (status.native != 0) return status;
        c->evaluation.frontend_parse_count += 1u;
        if (result.accepted == 0u) {
            status = arbor_view0_native_v1n3_emit(c, ordinal, a->name_offset, a->source_length, 0u);
            if (status.native != 0) return status;
        }
    }
    return arbor_status_from_native(0);
}

arbor_status arbor_view0_native_v1n3_g16_finalize(arbor_view0_native_v1n3_context *c)
{ (void)c; return arbor_status_from_native(0); }
arbor_status arbor_view0_native_v1n3_g16_measure(arbor_span i, const arbor_view0_native_v1n3_options *o, void *a, arbor_view0_native_v1n3_g16_evaluation *e)
{ return arbor_view0_native_v1n3_run_group(16u, i, o, a, NULL, 0u, e); }
arbor_status arbor_view0_native_v1n3_g16_collect_anchors(arbor_span i, const arbor_view0_native_v1n3_options *o, void *a, arbor_view0_native_v1n3_g16_anchor *n, uint64_t c, arbor_view0_native_v1n3_g16_evaluation *e)
{ return arbor_view0_native_v1n3_run_group(16u, i, o, a, n, c, e); }
void arbor_view0_native_v1n3_g16_materialize_anchor(const arbor_view0_native_v1n3_g16_anchor *a, uint64_t s, arbor_view0_native_diagnostic *d)
{ arbor_view0_native_v1n3_materialize(a, s, d); }
