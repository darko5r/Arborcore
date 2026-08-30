#ifndef ARBORCORE_VIEW0_CONFORMANCE_G07_H
#define ARBORCORE_VIEW0_CONFORMANCE_G07_H

#include <arborcore/view0_conformance/native.h>
#include "v1n2_c0.h"

#include <stdint.h>

#define ARBOR_VIEW0_NATIVE_V1N2_G07_RULE_COUNT UINT64_C(5)
#define ARBOR_VIEW0_NATIVE_V1N2_G07_MAX_SOURCE_ATTRIBUTES UINT64_C(8192)

typedef struct arbor_view0_native_v1n2_g07_anchor {
    arbor_view0_native_v1n2_anchor shared;
} arbor_view0_native_v1n2_g07_anchor;

typedef struct arbor_view0_native_v1n2_g07_evaluation {
    uint64_t diagnostic_count;
    uint64_t hyperlink_element_count;
    uint64_t rel_consumer_count;
    uint64_t prior_owner_suppression_count;
    uint64_t extension_relation_deferred_count;
    uint64_t rule_violation_count[5];
} arbor_view0_native_v1n2_g07_evaluation;

arbor_status arbor_view0_native_v1n2_g07_measure(
    arbor_span input,
    arbor_view0_native_v1n2_g07_evaluation *evaluation_out);

arbor_status arbor_view0_native_v1n2_g07_collect_anchors(
    arbor_span input,
    arbor_view0_native_v1n2_g07_anchor *anchors,
    uint64_t anchor_capacity,
    arbor_view0_native_v1n2_g07_evaluation *evaluation_out);

void arbor_view0_native_v1n2_g07_materialize_anchor(
    const arbor_view0_native_v1n2_g07_anchor *anchor,
    uint64_t discovery_sequence,
    arbor_view0_native_diagnostic *diagnostic);

#endif
