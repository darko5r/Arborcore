#ifndef ARBORCORE_VIEW0_CONFORMANCE_G05_R4A_H
#define ARBORCORE_VIEW0_CONFORMANCE_G05_R4A_H

#include <arborcore/view0_conformance/native.h>

#include <stdint.h>

typedef struct arbor_view0_native_g05_r4a_evaluation {
    uint64_t diagnostic_count;
    uint64_t matched_body_window_event_count;
    uint64_t admitted_body_count;
    uint64_t misplaced_owner_count;
} arbor_view0_native_g05_r4a_evaluation;

arbor_status arbor_view0_native_g05_r4a_measure(
    arbor_span input,
    arbor_view0_native_g05_r4a_evaluation *evaluation_out);

arbor_status arbor_view0_native_g05_r4a_collect_anchors(
    arbor_span input,
    arbor_view0_native_source_anchor *anchors,
    uint64_t anchor_capacity,
    arbor_view0_native_g05_r4a_evaluation *evaluation_out);

void arbor_view0_native_g05_r4a_materialize_anchor(
    const arbor_view0_native_source_anchor *anchor,
    uint64_t discovery_sequence,
    arbor_view0_native_diagnostic *diagnostic);

#endif
