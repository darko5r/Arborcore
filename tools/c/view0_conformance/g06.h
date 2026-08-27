#ifndef ARBORCORE_VIEW0_CONFORMANCE_G06_H
#define ARBORCORE_VIEW0_CONFORMANCE_G06_H

#include <arborcore/view0_conformance/native.h>

#include <stdint.h>

#define ARBOR_VIEW0_NATIVE_G06_RULE_COUNT UINT64_C(17)
#define ARBOR_VIEW0_NATIVE_G06_MAX_SOURCE_ATTRIBUTES UINT64_C(8192)
#define ARBOR_VIEW0_NATIVE_G06_TIME_TEXT_CAP UINT64_C(4096)

typedef struct arbor_view0_native_g06_anchor {
    arbor_view0_native_source_anchor source;
    uint16_t rule_ordinal;
    uint16_t reserved16;
    uint32_t reserved32;
} arbor_view0_native_g06_anchor;

typedef struct arbor_view0_native_g06_evaluation {
    uint64_t diagnostic_count;
    uint64_t matched_consumer_count;
    uint64_t prior_owner_suppression_count;
    uint64_t time_union_year_admission_count;
    uint64_t time_union_fallback_count;
    uint64_t rule_violation_count[17];
} arbor_view0_native_g06_evaluation;

arbor_status arbor_view0_native_g06_measure(
    arbor_span input,
    arbor_view0_native_g06_evaluation *evaluation_out);

arbor_status arbor_view0_native_g06_collect_anchors(
    arbor_span input,
    arbor_view0_native_g06_anchor *anchors,
    uint64_t anchor_capacity,
    arbor_view0_native_g06_evaluation *evaluation_out);

void arbor_view0_native_g06_materialize_anchor(
    const arbor_view0_native_g06_anchor *anchor,
    uint64_t discovery_sequence,
    arbor_view0_native_diagnostic *diagnostic);

#endif
