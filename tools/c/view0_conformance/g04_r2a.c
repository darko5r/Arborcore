#include "g04_r2a.h"
#include "g04_r1a.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define G04_R2A_FRAME_COUNT UINT64_C(4098)

#define G04_R2A_FRAME_VALID UINT32_C(0x1)
#define G04_R2A_FRAME_TRANSPARENT_FLOW UINT32_C(0x2)

typedef struct g04_r2a_frame {
    uint64_t standard_element_id;
    uint64_t source_offset;
    uint64_t source_length;
    uint32_t flags;
    uint32_t reserved;
} g04_r2a_frame;

typedef struct g04_r2a_context {
    arbor_view0_native_source_anchor *anchors;
    uint64_t anchor_capacity;
    uint64_t diagnostic_count;
    uint64_t g13_custom_deferred_count;
    uint64_t deferred_flags;
    uint64_t known_depth;
    bool collect_anchors;
    g04_r2a_frame frames[4098];
} g04_r2a_context;

_Static_assert(sizeof(g04_r2a_frame) == 32u, "G04 R2 frame layout drift");
_Static_assert(sizeof(g04_r2a_context) <= 1048576u,
               "G04 R2 evaluator workspace exceeds 1 MiB admission");
_Static_assert(sizeof("ARBOR_VIEW_V1_G04_TRANSPARENT_PARENTLESS_FLOW") <=
                   ARBOR_VIEW0_NATIVE_RULE_SYMBOL_CAP,
               "G04 R2 symbolic name exceeds diagnostic capacity");
_Static_assert(sizeof("Parentless transparent element content is not flow content") <=
                   ARBOR_VIEW0_NATIVE_MESSAGE_CAP,
               "G04 R2 message exceeds diagnostic capacity");

static arbor_status status_from_errno_value(int value)
{
    return arbor_status_from_native(-(int64_t)value);
}

static arbor_status ok_status(void)
{
    return arbor_status_from_native(0);
}

static bool span_has_ascii_hyphen(arbor_span span)
{
    if (span.length != 0u && span.data == NULL) return false;
    for (uint64_t i = 0u; i < span.length; ++i) {
        if (span.data[i] == (uint8_t)'-') return true;
    }
    return false;
}

static bool standard_parentless_transparent(uint64_t id)
{
    switch (id) {
    case ARBOR_VIEW0_NATIVE_ELEMENT_A:
    case ARBOR_VIEW0_NATIVE_ELEMENT_INS:
    case ARBOR_VIEW0_NATIVE_ELEMENT_DEL:
    case ARBOR_VIEW0_NATIVE_ELEMENT_OBJECT:
    case ARBOR_VIEW0_NATIVE_ELEMENT_VIDEO:
    case ARBOR_VIEW0_NATIVE_ELEMENT_AUDIO:
    case ARBOR_VIEW0_NATIVE_ELEMENT_MAP:
    case ARBOR_VIEW0_NATIVE_ELEMENT_NOSCRIPT:
    case ARBOR_VIEW0_NATIVE_ELEMENT_SLOT:
    case ARBOR_VIEW0_NATIVE_ELEMENT_CANVAS:
        return true;
    default:
        return false;
    }
}

static bool frame_matches_current(
    const g04_r2a_context *context,
    uint64_t depth,
    uint64_t current_source_offset)
{
    if (context == NULL || depth == 0u || depth >= G04_R2A_FRAME_COUNT ||
        current_source_offset == ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE) return false;
    const g04_r2a_frame *frame = &context->frames[depth - 1u];
    return (frame->flags & G04_R2A_FRAME_VALID) != 0u &&
        frame->source_offset == current_source_offset;
}

static void clear_from(g04_r2a_context *context, uint64_t depth)
{
    if (context == NULL || depth >= G04_R2A_FRAME_COUNT) return;
    for (uint64_t i = depth; i < context->known_depth && i < G04_R2A_FRAME_COUNT; ++i)
        context->frames[i] = (g04_r2a_frame){0};
    context->known_depth = depth;
}

static arbor_status report_invalid(
    g04_r2a_context *context,
    const arbor_view0_native_source_repair_context *record)
{
    if (context == NULL || record == NULL ||
        record->source_offset == ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE ||
        record->source_length == 0u) return status_from_errno_value(EINVAL);
    if (context->diagnostic_count == UINT64_MAX) return status_from_errno_value(EOVERFLOW);
    if (context->collect_anchors) {
        if (context->anchors == NULL || context->diagnostic_count >= context->anchor_capacity)
            return status_from_errno_value(ENOBUFS);
        if (record->source_offset > UINT32_MAX || record->source_length > UINT32_MAX)
            return status_from_errno_value(E2BIG);
        context->anchors[context->diagnostic_count] = (arbor_view0_native_source_anchor){
            .byte_offset = (uint32_t)record->source_offset,
            .source_length = (uint32_t)record->source_length
        };
    }
    context->diagnostic_count += 1u;
    return ok_status();
}

static arbor_status source_repair(
    void *context_void,
    const arbor_view0_native_source_repair_context *record)
{
    g04_r2a_context *context = (g04_r2a_context *)context_void;
    if (context == NULL || record == NULL) return status_from_errno_value(EINVAL);
    if (record->initial_open_elements_depth >= G04_R2A_FRAME_COUNT ||
        record->insertion_open_elements_depth >= G04_R2A_FRAME_COUNT)
        return status_from_errno_value(EIO);

    if (context->known_depth > record->initial_open_elements_depth)
        clear_from(context, record->initial_open_elements_depth);
    else if (context->known_depth < record->initial_open_elements_depth)
        context->known_depth = record->initial_open_elements_depth;

    bool parent_transparent_flow = false;
    uint64_t parent_id = record->initial_current_standard_element_id;
    if (frame_matches_current(
            context,
            record->initial_open_elements_depth,
            record->initial_current_source_offset)) {
        const g04_r2a_frame *parent =
            &context->frames[record->initial_open_elements_depth - 1u];
        parent_id = parent->standard_element_id;
        parent_transparent_flow =
            (parent->flags & G04_R2A_FRAME_TRANSPARENT_FLOW) != 0u;
    }

    if (parent_transparent_flow) {
        const bool media_prefix =
            (parent_id == ARBOR_VIEW0_NATIVE_ELEMENT_VIDEO ||
             parent_id == ARBOR_VIEW0_NATIVE_ELEMENT_AUDIO) &&
            (record->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_SOURCE ||
             record->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_TRACK);
        if (!media_prefix &&
            record->standard_element_id != ARBOR_VIEW0_NATIVE_ELEMENT_NONE &&
            !arbor_view0_native_g04_standard_element_is_flow_content(
                record->standard_element_id)) {
            arbor_status status = report_invalid(context, record);
            if (status.native != 0) return status;
        }
    }

    const bool top_level_parentless =
        record->initial_open_elements_depth == 1u &&
        record->initial_current_standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_HTML &&
        record->initial_current_source_offset == ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE;
    const bool transparent = standard_parentless_transparent(record->standard_element_id);
    const bool transparent_flow =
        transparent && (top_level_parentless || parent_transparent_flow);

    if (record->insertion_seen == 1u) {
        clear_from(context, record->insertion_open_elements_depth);
        const uint64_t slot = record->insertion_open_elements_depth;
        context->frames[slot] = (g04_r2a_frame){
            .standard_element_id = record->standard_element_id,
            .source_offset = record->source_offset,
            .source_length = record->source_length,
            .flags = G04_R2A_FRAME_VALID |
                (transparent_flow ? G04_R2A_FRAME_TRANSPARENT_FLOW : 0u),
            .reserved = 0u
        };
        context->known_depth = slot + 1u;
    }
    return ok_status();
}

static arbor_status element_begin(
    void *context_void,
    const arbor_view0_native_element_observation *observation)
{
    g04_r2a_context *context = (g04_r2a_context *)context_void;
    if (context == NULL || observation == NULL) return status_from_errno_value(EINVAL);
    if (observation->namespace_id == ARBOR_VIEW0_NATIVE_NAMESPACE_HTML &&
        observation->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_NONE &&
        (observation->flags & ARBOR_VIEW0_NATIVE_ELEMENT_FLAG_SYNTHETIC) == 0u &&
        span_has_ascii_hyphen(observation->local_name)) {
        if (context->g13_custom_deferred_count == UINT64_MAX)
            return status_from_errno_value(EOVERFLOW);
        context->g13_custom_deferred_count += 1u;
        context->deferred_flags |=
            ARBOR_VIEW0_NATIVE_RESULT_FLAG_G04_R2_DEFERRED_G13_CUSTOM;
    }
    return ok_status();
}

static arbor_status evaluate(
    arbor_span input,
    arbor_view0_native_source_anchor *anchors,
    uint64_t anchor_capacity,
    bool collect_anchors,
    arbor_view0_native_g04_r2a_evaluation *evaluation_out)
{
    if (evaluation_out == NULL) return status_from_errno_value(EINVAL);
    g04_r2a_context context = {
        .anchors = anchors,
        .anchor_capacity = anchor_capacity,
        .diagnostic_count = 0u,
        .g13_custom_deferred_count = 0u,
        .deferred_flags = 0u,
        .known_depth = 0u,
        .collect_anchors = collect_anchors,
        .frames = {{0}}
    };
    const arbor_view0_native_semantic_observer observer = {
        .context = &context,
        .element_begin = element_begin,
        .source_repair = source_repair
    };
    arbor_view0_native_parse_counts parse_counts = {0};
    arbor_view0_native_observation_counts observation_counts = {0};
    arbor_status status = arbor_view0_native_lexbor_observe_fragment_model(
        input,
        &observer,
        &parse_counts,
        &observation_counts);
    if (status.native != 0) return status;
    (void)observation_counts;
    *evaluation_out = (arbor_view0_native_g04_r2a_evaluation){
        .diagnostic_count = context.diagnostic_count,
        .g13_custom_deferred_count = context.g13_custom_deferred_count,
        .deferred_flags = context.deferred_flags,
        .parse_counts = parse_counts
    };
    return ok_status();
}

arbor_status arbor_view0_native_g04_r2a_measure_fragment_model(
    arbor_span input,
    arbor_view0_native_g04_r2a_evaluation *evaluation_out)
{
    return evaluate(input, NULL, 0u, false, evaluation_out);
}

arbor_status arbor_view0_native_g04_r2a_collect_fragment_anchors(
    arbor_span input,
    arbor_view0_native_source_anchor *anchors,
    uint64_t anchor_capacity,
    arbor_view0_native_g04_r2a_evaluation *evaluation_out)
{
    if (anchor_capacity != 0u && anchors == NULL) return status_from_errno_value(EINVAL);
    return evaluate(input, anchors, anchor_capacity, true, evaluation_out);
}

void arbor_view0_native_g04_r2a_materialize_anchor(
    const arbor_view0_native_source_anchor *anchor,
    uint64_t discovery_sequence,
    arbor_view0_native_diagnostic *diagnostic)
{
    static const char symbolic[] = "ARBOR_VIEW_V1_G04_TRANSPARENT_PARENTLESS_FLOW";
    static const char message[] = "Parentless transparent element content is not flow content";
    if (anchor == NULL || diagnostic == NULL) return;
    (void)memset(diagnostic, 0, sizeof(*diagnostic));
    diagnostic->rule_id = ARBOR_VIEW_V1_G04_TRANSPARENT_PARENTLESS_FLOW;
    diagnostic->byte_offset = anchor->byte_offset;
    diagnostic->source_length = anchor->source_length;
    diagnostic->discovery_sequence = discovery_sequence;
    diagnostic->severity = (uint32_t)ARBOR_VIEW0_NATIVE_SEVERITY_ERROR;
    diagnostic->origin = (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_AUTHORING;
    (void)memcpy(diagnostic->symbolic_name, symbolic, sizeof(symbolic));
    (void)memcpy(diagnostic->message, message, sizeof(message));
}
