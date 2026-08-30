#ifndef ARBORCORE_VIEW0_CONFORMANCE_G10_H
#define ARBORCORE_VIEW0_CONFORMANCE_G10_H

#include <arborcore/view0_conformance/native.h>
#include "v1n2_c0.h"

#include <stddef.h>
#include <stdint.h>

#define ARBOR_VIEW0_NATIVE_V1N2_G10_RULE_COUNT UINT64_C(13)
#define ARBOR_VIEW0_NATIVE_V1N2_G10_MAX_DEPTH UINT64_C(512)

typedef struct arbor_view0_native_v1n2_g10_anchor {
    arbor_view0_native_v1n2_anchor shared;
} arbor_view0_native_v1n2_g10_anchor;

typedef struct arbor_view0_native_v1n2_g10_evaluation {
    uint64_t diagnostic_count;
    uint64_t form_count;
    uint64_t control_count;
    uint64_t input_count;
    uint64_t label_count;
    uint64_t option_count;
    uint64_t idref_token_count;
    uint64_t prior_owner_suppression_count;
    uint64_t deferred_external_semantics_count;
    uint64_t rule_violation_count[13];
} arbor_view0_native_v1n2_g10_evaluation;

/* Lexbor-owned transient support allocation seam used by the atomicity gate. */
void *arbor_view0_native_v1n2_g10_support_calloc(void *arena, size_t size);

arbor_status arbor_view0_native_v1n2_g10_measure(
    arbor_span input,
    arbor_view0_native_v1n2_g10_evaluation *evaluation_out);

arbor_status arbor_view0_native_v1n2_g10_collect_anchors(
    arbor_span input,
    arbor_view0_native_v1n2_g10_anchor *anchors,
    uint64_t anchor_capacity,
    arbor_view0_native_v1n2_g10_evaluation *evaluation_out);

void arbor_view0_native_v1n2_g10_materialize_anchor(
    const arbor_view0_native_v1n2_g10_anchor *anchor,
    uint64_t discovery_sequence,
    arbor_view0_native_diagnostic *diagnostic);

#endif
