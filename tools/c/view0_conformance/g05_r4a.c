#include "g05_r4a.h"
#include "g05_c0.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef struct g05_r4a_context {
    arbor_view0_native_source_anchor *anchors;
    uint64_t anchor_capacity;
    uint64_t diagnostic_count;
    uint64_t matched_body_window_event_count;
    uint64_t admitted_body_count;
    uint64_t misplaced_owner_count;
    bool collect_anchors;
} g05_r4a_context;

#define G05_R4_BODY_STANDARD_ELEMENT_ID UINT64_C(8)

_Static_assert(sizeof("ARBOR_VIEW_V1_G05_BODY_WINDOW_EVENT_ATTRIBUTE_APPLICABILITY") <=
                   ARBOR_VIEW0_NATIVE_RULE_SYMBOL_CAP,
               "G05 R4 symbolic name exceeds diagnostic capacity");
_Static_assert(sizeof("Window-reflecting body event-handler attribute is only applicable to body") <=
                   ARBOR_VIEW0_NATIVE_MESSAGE_CAP,
               "G05 R4 message exceeds diagnostic capacity");

static arbor_status err_status(int e) { return arbor_status_from_native(-(int64_t)e); }
static arbor_status ok_status(void) { return arbor_status_from_native(0); }

static arbor_status source_attribute(
    void *opaque,
    const arbor_view0_native_source_attribute_observation *observation)
{
    if (opaque == NULL || observation == NULL) return err_status(EINVAL);
    g05_r4a_context *context = (g05_r4a_context *)opaque;

    if (!arbor_view0_native_g05_c0_body_window_event_listed(observation->local_name))
        return ok_status();

    if (context->matched_body_window_event_count == UINT64_MAX) return err_status(EOVERFLOW);
    context->matched_body_window_event_count += 1u;

    if (observation->owner_standard_element_id == G05_R4_BODY_STANDARD_ELEMENT_ID) {
        if (context->admitted_body_count == UINT64_MAX) return err_status(EOVERFLOW);
        context->admitted_body_count += 1u;
        return ok_status();
    }

    if (observation->source_offset == ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE ||
        observation->source_length == 0u || observation->source_offset > UINT32_MAX ||
        observation->source_length > UINT32_MAX)
        return err_status(EIO);
    if (context->diagnostic_count == UINT64_MAX || context->misplaced_owner_count == UINT64_MAX)
        return err_status(EOVERFLOW);
    if (context->collect_anchors) {
        if (context->diagnostic_count >= context->anchor_capacity || context->anchors == NULL)
            return err_status(ENOSPC);
        context->anchors[context->diagnostic_count] = (arbor_view0_native_source_anchor){
            .byte_offset = (uint32_t)observation->source_offset,
            .source_length = (uint32_t)observation->source_length
        };
    }
    context->diagnostic_count += 1u;
    context->misplaced_owner_count += 1u;
    return ok_status();
}

static arbor_status evaluate(
    arbor_span input,
    arbor_view0_native_source_anchor *anchors,
    uint64_t anchor_capacity,
    bool collect_anchors,
    arbor_view0_native_g05_r4a_evaluation *evaluation_out)
{
    if (evaluation_out == NULL || (collect_anchors && anchor_capacity != 0u && anchors == NULL))
        return err_status(EINVAL);
    g05_r4a_context context = {
        .anchors = anchors,
        .anchor_capacity = anchor_capacity,
        .collect_anchors = collect_anchors
    };
    const arbor_view0_native_semantic_observer observer = {
        .context = &context,
        .source_attribute = source_attribute
    };
    arbor_view0_native_parse_counts parse_counts = {0};
    arbor_view0_native_document_facts facts = {0};
    arbor_view0_native_observation_counts observation_counts = {0};
    arbor_status status = arbor_view0_native_lexbor_observe(
        input, &observer, &parse_counts, &facts, &observation_counts);
    if (status.native != 0) return status;
    if (collect_anchors && context.diagnostic_count != anchor_capacity) return err_status(EIO);
    *evaluation_out = (arbor_view0_native_g05_r4a_evaluation){
        .diagnostic_count = context.diagnostic_count,
        .matched_body_window_event_count = context.matched_body_window_event_count,
        .admitted_body_count = context.admitted_body_count,
        .misplaced_owner_count = context.misplaced_owner_count
    };
    return ok_status();
}

arbor_status arbor_view0_native_g05_r4a_measure(
    arbor_span input,
    arbor_view0_native_g05_r4a_evaluation *evaluation_out)
{
    return evaluate(input, NULL, 0u, false, evaluation_out);
}

arbor_status arbor_view0_native_g05_r4a_collect_anchors(
    arbor_span input,
    arbor_view0_native_source_anchor *anchors,
    uint64_t anchor_capacity,
    arbor_view0_native_g05_r4a_evaluation *evaluation_out)
{
    return evaluate(input, anchors, anchor_capacity, true, evaluation_out);
}

void arbor_view0_native_g05_r4a_materialize_anchor(
    const arbor_view0_native_source_anchor *anchor,
    uint64_t discovery_sequence,
    arbor_view0_native_diagnostic *diagnostic)
{
    static const char symbolic[] = "ARBOR_VIEW_V1_G05_BODY_WINDOW_EVENT_ATTRIBUTE_APPLICABILITY";
    static const char message[] =
        "Window-reflecting body event-handler attribute is only applicable to body";
    if (anchor == NULL || diagnostic == NULL) return;
    (void)memset(diagnostic, 0, sizeof(*diagnostic));
    diagnostic->rule_id = ARBOR_VIEW_V1_G05_BODY_WINDOW_EVENT_ATTRIBUTE_APPLICABILITY;
    diagnostic->byte_offset = (uint64_t)anchor->byte_offset;
    diagnostic->source_length = (uint64_t)anchor->source_length;
    diagnostic->discovery_sequence = discovery_sequence;
    diagnostic->severity = (uint32_t)ARBOR_VIEW0_NATIVE_SEVERITY_ERROR;
    diagnostic->origin = (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_AUTHORING;
    (void)memcpy(diagnostic->symbolic_name, symbolic, sizeof(symbolic));
    (void)memcpy(diagnostic->message, message, sizeof(message));
}
