#ifndef ARBORCORE_VIEW0_CONFORMANCE_G03_R7A_H
#define ARBORCORE_VIEW0_CONFORMANCE_G03_R7A_H

#include <stdint.h>
#include <arborcore/view0_conformance/native.h>

typedef struct arbor_view0_native_g03_r7a_evaluation {
    uint64_t diagnostic_count;
    uint64_t deferred_flags;
    uint64_t g04_deferred_count;
    uint64_t g13_deferred_count;
} arbor_view0_native_g03_r7a_evaluation;

arbor_status arbor_view0_native_g03_r7a_measure(
    arbor_span input, arbor_view0_native_g03_r7a_evaluation *evaluation_out);

arbor_status arbor_view0_native_g03_r7a_collect(
    arbor_span input,
    arbor_view0_native_diagnostic *diagnostics,
    uint64_t diagnostic_capacity,
    uint64_t discovery_sequence_base,
    arbor_view0_native_g03_r7a_evaluation *evaluation_out);

arbor_status arbor_view0_native_g03_r7a_collect_anchors(
    arbor_span input,
    arbor_view0_native_source_anchor *anchors,
    uint64_t anchor_capacity,
    arbor_view0_native_g03_r7a_evaluation *evaluation_out);

void arbor_view0_native_g03_r7a_materialize_anchor(
    const arbor_view0_native_source_anchor *anchor,
    uint64_t discovery_sequence,
    arbor_view0_native_diagnostic *diagnostic);

#endif
