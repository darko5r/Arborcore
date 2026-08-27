#ifndef ARBORCORE_VIEW0_CONFORMANCE_G04_R2A_H
#define ARBORCORE_VIEW0_CONFORMANCE_G04_R2A_H

#include <stdint.h>
#include <arborcore/view0_conformance/native.h>

typedef struct arbor_view0_native_g04_r2a_evaluation {
    uint64_t diagnostic_count;
    uint64_t g13_custom_deferred_count;
    uint64_t deferred_flags;
    arbor_view0_native_parse_counts parse_counts;
} arbor_view0_native_g04_r2a_evaluation;

arbor_status arbor_view0_native_g04_r2a_measure_fragment_model(
    arbor_span input,
    arbor_view0_native_g04_r2a_evaluation *evaluation_out);

arbor_status arbor_view0_native_g04_r2a_collect_fragment_anchors(
    arbor_span input,
    arbor_view0_native_source_anchor *anchors,
    uint64_t anchor_capacity,
    arbor_view0_native_g04_r2a_evaluation *evaluation_out);

void arbor_view0_native_g04_r2a_materialize_anchor(
    const arbor_view0_native_source_anchor *anchor,
    uint64_t discovery_sequence,
    arbor_view0_native_diagnostic *diagnostic);

#endif
