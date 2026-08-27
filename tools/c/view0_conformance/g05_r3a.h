#ifndef ARBORCORE_VIEW0_CONFORMANCE_G05_R3A_H
#define ARBORCORE_VIEW0_CONFORMANCE_G05_R3A_H

#include <arborcore/view0_conformance/native.h>

#include <stdint.h>

#define ARBOR_VIEW0_NATIVE_G05_R3A_CLAUSE_COUNT UINT64_C(43)
#define ARBOR_VIEW0_NATIVE_G05_R3A_MAX_TRACKED_SOURCE_ATTRIBUTES UINT64_C(8192)

typedef struct arbor_view0_native_g05_r3a_evaluation {
    uint64_t diagnostic_count;
    uint64_t predicate_evaluation_count;
    uint64_t input_element_count;
    uint64_t missing_required_count;
    uint64_t forbidden_present_count;
    uint64_t tracked_source_attribute_count;
    uint64_t clause_violation_count[43];
} arbor_view0_native_g05_r3a_evaluation;

arbor_status arbor_view0_native_g05_r3a_measure(
    arbor_span input,
    arbor_view0_native_g05_r3a_evaluation *evaluation_out);

arbor_status arbor_view0_native_g05_r3a_collect_anchors(
    arbor_span input,
    arbor_view0_native_source_anchor *anchors,
    uint64_t anchor_capacity,
    arbor_view0_native_g05_r3a_evaluation *evaluation_out);

void arbor_view0_native_g05_r3a_materialize_anchor(
    const arbor_view0_native_source_anchor *anchor,
    uint64_t discovery_sequence,
    arbor_view0_native_diagnostic *diagnostic);

#endif
