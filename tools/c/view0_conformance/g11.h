#ifndef ARBORCORE_VIEW0_CONFORMANCE_G11_H
#define ARBORCORE_VIEW0_CONFORMANCE_G11_H

#include <arborcore/view0_conformance/native.h>
#include "v1n2_c0.h"

#include <stddef.h>
#include <stdint.h>

#define ARBOR_VIEW0_NATIVE_V1N2_G11_RULE_COUNT UINT64_C(2)
#define ARBOR_VIEW0_NATIVE_V1N2_G11_MAX_DEPTH UINT64_C(512)

typedef struct arbor_view0_native_v1n2_g11_anchor {
    arbor_view0_native_v1n2_anchor shared;
} arbor_view0_native_v1n2_g11_anchor;

typedef struct arbor_view0_native_v1n2_g11_evaluation {
    uint64_t diagnostic_count;
    uint64_t details_count;
    uint64_t dialog_count;
    uint64_t name_group_relation_count;
    uint64_t prior_owner_suppression_count;
    uint64_t deferred_external_semantics_count;
    uint64_t rule_violation_count[2];
} arbor_view0_native_v1n2_g11_evaluation;

/* Lexbor-owned transient support allocation seam used by the atomicity gate. */
void *arbor_view0_native_v1n2_g11_support_calloc(void *arena, size_t size);

arbor_status arbor_view0_native_v1n2_g11_measure(
    arbor_span input,
    arbor_view0_native_v1n2_g11_evaluation *evaluation_out);

arbor_status arbor_view0_native_v1n2_g11_collect_anchors(
    arbor_span input,
    arbor_view0_native_v1n2_g11_anchor *anchors,
    uint64_t anchor_capacity,
    arbor_view0_native_v1n2_g11_evaluation *evaluation_out);

void arbor_view0_native_v1n2_g11_materialize_anchor(
    const arbor_view0_native_v1n2_g11_anchor *anchor,
    uint64_t discovery_sequence,
    arbor_view0_native_diagnostic *diagnostic);

#endif
