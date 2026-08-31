#ifndef ARBORCORE_VIEW0_CONFORMANCE_G12_H
#define ARBORCORE_VIEW0_CONFORMANCE_G12_H

#include "v1n3_c0.h"

#define ARBOR_VIEW0_NATIVE_V1N3_G12_RULE_COUNT UINT64_C(8)

typedef arbor_view0_native_v1n3_anchor arbor_view0_native_v1n3_g12_anchor;
typedef arbor_view0_native_v1n3_evaluation arbor_view0_native_v1n3_g12_evaluation;

arbor_status arbor_view0_native_v1n3_g12_measure(
    arbor_span input, const arbor_view0_native_v1n3_options *options,
    void *arena, arbor_view0_native_v1n3_g12_evaluation *evaluation_out);
arbor_status arbor_view0_native_v1n3_g12_collect_anchors(
    arbor_span input, const arbor_view0_native_v1n3_options *options,
    void *arena, arbor_view0_native_v1n3_g12_anchor *anchors,
    uint64_t anchor_capacity, arbor_view0_native_v1n3_g12_evaluation *evaluation_out);
void arbor_view0_native_v1n3_g12_materialize_anchor(
    const arbor_view0_native_v1n3_g12_anchor *anchor,
    uint64_t discovery_sequence, arbor_view0_native_diagnostic *diagnostic);
arbor_status arbor_view0_native_v1n3_g12_validate_element(
    arbor_view0_native_v1n3_context *context,
    const arbor_view0_native_v1n3_element *element);
arbor_status arbor_view0_native_v1n3_g12_finalize(
    arbor_view0_native_v1n3_context *context);

#endif
