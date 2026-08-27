#ifndef ARBORCORE_VIEW0_CONFORMANCE_G04_R1A_H
#define ARBORCORE_VIEW0_CONFORMANCE_G04_R1A_H

#include <stdbool.h>
#include <stdint.h>
#include <arborcore/view0_conformance/native.h>

typedef struct arbor_view0_native_g04_r1a_evaluation {
    uint64_t diagnostic_count;
    uint64_t prior_owner_suppression_count;
    uint64_t deferred_flags;
    uint64_t noscript_deferred_count; /* reserved; R1C no longer publishes it */
    uint64_t noscript_resolved_count;
    uint64_t option_branch_deferred_count;
    uint64_t option_branch_resolved_count;
    uint64_t select_text_violation_count;
    uint64_t g13_custom_deferred_count;
} arbor_view0_native_g04_r1a_evaluation;

arbor_status arbor_view0_native_g04_r1a_measure(
    arbor_span input, arbor_view0_native_g04_r1a_evaluation *evaluation_out);

arbor_status arbor_view0_native_g04_r1a_collect(
    arbor_span input,
    arbor_view0_native_diagnostic *diagnostics,
    uint64_t diagnostic_capacity,
    uint64_t discovery_sequence_base,
    arbor_view0_native_g04_r1a_evaluation *evaluation_out);

arbor_status arbor_view0_native_g04_r1a_collect_anchors(
    arbor_span input,
    arbor_view0_native_source_anchor *anchors,
    uint64_t anchor_capacity,
    arbor_view0_native_g04_r1a_evaluation *evaluation_out);

void arbor_view0_native_g04_r1a_materialize_anchor(
    const arbor_view0_native_source_anchor *anchor,
    uint64_t discovery_sequence,
    arbor_view0_native_diagnostic *diagnostic);

bool arbor_view0_native_g04_standard_element_is_flow_content(uint64_t standard_element_id);
bool arbor_view0_native_g04_select_transparent_div_is_r7_subject(
    uint64_t containing_standard_element_id,
    bool option_has_label,
    bool option_in_datalist);

#endif
