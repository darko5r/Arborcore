#ifndef ARBORCORE_VIEW0_CONFORMANCE_G08_H
#define ARBORCORE_VIEW0_CONFORMANCE_G08_H

#include <arborcore/view0_conformance/native.h>
#include "v1n2_c0.h"

#include <stdint.h>

#define ARBOR_VIEW0_NATIVE_V1N2_G08_RULE_COUNT UINT64_C(12)
#define ARBOR_VIEW0_NATIVE_V1N2_G08_MAX_SOURCE_ATTRIBUTES UINT64_C(8192)
#define ARBOR_VIEW0_NATIVE_V1N2_G08_MAX_DEPTH UINT64_C(512)
#define ARBOR_VIEW0_NATIVE_V1N2_G08_MAX_TRACKS UINT64_C(512)
#define ARBOR_VIEW0_NATIVE_V1N2_G08_MAX_MAPS UINT64_C(256)
#define ARBOR_VIEW0_NATIVE_V1N2_G08_MAX_USEMAPS UINT64_C(512)
#define ARBOR_VIEW0_NATIVE_V1N2_G08_MAX_DEFERRED_ALT UINT64_C(256)

typedef struct arbor_view0_native_v1n2_g08_anchor {
    arbor_view0_native_v1n2_anchor shared;
} arbor_view0_native_v1n2_g08_anchor;

typedef struct arbor_view0_native_v1n2_g08_evaluation {
    uint64_t diagnostic_count;
    uint64_t embedded_element_count;
    uint64_t responsive_source_count;
    uint64_t media_source_count;
    uint64_t text_track_count;
    uint64_t image_map_reference_count;
    uint64_t foreign_integration_count;
    uint64_t prior_owner_suppression_count;
    uint64_t deferred_external_semantics_count;
    uint64_t rule_violation_count[12];
} arbor_view0_native_v1n2_g08_evaluation;

arbor_status arbor_view0_native_v1n2_g08_measure(
    arbor_span input,
    arbor_view0_native_v1n2_g08_evaluation *evaluation_out);

arbor_status arbor_view0_native_v1n2_g08_collect_anchors(
    arbor_span input,
    arbor_view0_native_v1n2_g08_anchor *anchors,
    uint64_t anchor_capacity,
    arbor_view0_native_v1n2_g08_evaluation *evaluation_out);

void arbor_view0_native_v1n2_g08_materialize_anchor(
    const arbor_view0_native_v1n2_g08_anchor *anchor,
    uint64_t discovery_sequence,
    arbor_view0_native_diagnostic *diagnostic);

#endif
