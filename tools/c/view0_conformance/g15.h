#ifndef ARBORCORE_VIEW0_CONFORMANCE_G15_H
#define ARBORCORE_VIEW0_CONFORMANCE_G15_H

#include "v1n3_c0.h"

#define ARBOR_VIEW0_NATIVE_V1N3_G15_RULE_COUNT UINT64_C(8)

typedef arbor_view0_native_v1n3_anchor arbor_view0_native_v1n3_g15_anchor;
typedef arbor_view0_native_v1n3_evaluation arbor_view0_native_v1n3_g15_evaluation;

arbor_status arbor_view0_native_v1n3_g15_measure(
    arbor_span input, const arbor_view0_native_v1n3_options *options,
    void *arena, arbor_view0_native_v1n3_g15_evaluation *evaluation_out);
arbor_status arbor_view0_native_v1n3_g15_collect_anchors(
    arbor_span input, const arbor_view0_native_v1n3_options *options,
    void *arena, arbor_view0_native_v1n3_g15_anchor *anchors,
    uint64_t anchor_capacity, arbor_view0_native_v1n3_g15_evaluation *evaluation_out);
void arbor_view0_native_v1n3_g15_materialize_anchor(
    const arbor_view0_native_v1n3_g15_anchor *anchor,
    uint64_t discovery_sequence, arbor_view0_native_diagnostic *diagnostic);
arbor_status arbor_view0_native_v1n3_g15_validate_element(
    arbor_view0_native_v1n3_context *context,
    const arbor_view0_native_v1n3_element *element);
arbor_status arbor_view0_native_v1n3_g15_finalize(
    arbor_view0_native_v1n3_context *context);

#endif
