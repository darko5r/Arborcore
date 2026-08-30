#ifndef ARBORCORE_VIEW0_CONFORMANCE_G09_H
#define ARBORCORE_VIEW0_CONFORMANCE_G09_H

#include <arborcore/view0_conformance/native.h>
#include "v1n2_c0.h"

#include <stddef.h>
#include <stdint.h>

#define ARBOR_VIEW0_NATIVE_V1N2_G09_RULE_COUNT UINT64_C(6)
#define ARBOR_VIEW0_NATIVE_V1N2_G09_MAX_DEPTH UINT64_C(512)

typedef struct arbor_view0_native_v1n2_g09_anchor {
    arbor_view0_native_v1n2_anchor shared;
} arbor_view0_native_v1n2_g09_anchor;

typedef struct arbor_view0_native_v1n2_g09_evaluation {
    uint64_t diagnostic_count;
    uint64_t table_count;
    uint64_t row_count;
    uint64_t cell_count;
    uint64_t column_group_count;
    uint64_t row_group_count;
    uint64_t header_token_count;
    uint64_t implicit_header_association_count;
    uint64_t prior_owner_suppression_count;
    uint64_t deferred_external_semantics_count;
    uint64_t rule_violation_count[6];
} arbor_view0_native_v1n2_g09_evaluation;

/* Lexbor-owned transient support allocation seam used by the atomicity gate. */
void *arbor_view0_native_v1n2_g09_support_calloc(void *arena, size_t size);

arbor_status arbor_view0_native_v1n2_g09_measure(
    arbor_span input,
    arbor_view0_native_v1n2_g09_evaluation *evaluation_out);

arbor_status arbor_view0_native_v1n2_g09_collect_anchors(
    arbor_span input,
    arbor_view0_native_v1n2_g09_anchor *anchors,
    uint64_t anchor_capacity,
    arbor_view0_native_v1n2_g09_evaluation *evaluation_out);

void arbor_view0_native_v1n2_g09_materialize_anchor(
    const arbor_view0_native_v1n2_g09_anchor *anchor,
    uint64_t discovery_sequence,
    arbor_view0_native_diagnostic *diagnostic);

#endif
