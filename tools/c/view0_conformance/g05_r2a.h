#ifndef ARBORCORE_VIEW0_CONFORMANCE_G05_R2A_H
#define ARBORCORE_VIEW0_CONFORMANCE_G05_R2A_H

#include <stdint.h>
#include <arborcore/view0_conformance/native.h>

typedef struct arbor_view0_native_g05_r2a_evaluation {
    uint64_t diagnostic_count;
    uint64_t admitted_element_attribute_count;
    uint64_t global_attribute_handoff_count;
    uint64_t r1_owned_unknown_count;
    uint64_t r4_body_event_handoff_count;
    uint64_t nonstandard_owner_ignored_count;
} arbor_view0_native_g05_r2a_evaluation;

arbor_status arbor_view0_native_g05_r2a_measure(
    arbor_span input, arbor_view0_native_g05_r2a_evaluation *evaluation_out);

arbor_status arbor_view0_native_g05_r2a_collect_anchors(
    arbor_span input,
    arbor_view0_native_source_anchor *anchors,
    uint64_t anchor_capacity,
    arbor_view0_native_g05_r2a_evaluation *evaluation_out);

void arbor_view0_native_g05_r2a_materialize_anchor(
    const arbor_view0_native_source_anchor *anchor,
    uint64_t discovery_sequence,
    arbor_view0_native_diagnostic *diagnostic);

#endif
