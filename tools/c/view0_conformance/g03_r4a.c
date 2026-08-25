#include "g03_r4a.h"
#include "g03_r1a.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define G03_R4A_MAX_DEPTH UINT64_C(4097)
#define G03_R4A_FRAME_COUNT UINT64_C(4098)
#define G03_R4A_NOINLINE __attribute__((noinline))
#define G03_R4_ATTR_LABEL (UINT64_C(1) << 0)
#define G03_R4_ATTR_VALUE (UINT64_C(1) << 1)
#define G03_R4_ATTR_SPAN  (UINT64_C(1) << 2)

_Static_assert(sizeof("ARBOR_VIEW_V1_G03_NOTHING_MODEL") <= ARBOR_VIEW0_NATIVE_RULE_SYMBOL_CAP,
               "G03 R4A symbolic name exceeds diagnostic capacity");
_Static_assert(sizeof("HTML element with a Nothing content model has disallowed contents") <=
                   ARBOR_VIEW0_NATIVE_MESSAGE_CAP,
               "G03 R4A message exceeds diagnostic capacity");

typedef struct g03_r4a_frame {
    uint64_t standard_element_id;
    uint64_t source_offset;
    uint64_t source_length;
    uint64_t attribute_flags;
    bool authored;
    bool nonwhitespace_text_violation;
    bool unowned_element_child_violation;
} g03_r4a_frame;

typedef struct g03_r4a_context {
    union {
        arbor_view0_native_diagnostic *diagnostics;
        uint64_t *source_offsets;
        arbor_view0_native_source_anchor *anchors;
    } output;
    uint64_t diagnostic_capacity;
    uint64_t discovery_sequence_base;
    uint64_t diagnostic_count;
    uint64_t deferred_flags;
    uint64_t current_depth;
    const uint64_t *r1_source_offsets;
    uint64_t r1_source_offset_count;
    bool publish;
    bool collect_offsets;
    bool collect_anchors;
    g03_r4a_frame frames[4098];
} g03_r4a_context;

_Static_assert(sizeof(g03_r4a_frame) == 40u,
               "G03 R4A frame layout drift on x86-64");
_Static_assert(sizeof(g03_r4a_context) == 163992u,
               "G03 R4A bounded evaluator workspace layout drift on x86-64");
_Static_assert(sizeof(g03_r4a_context) <= 1048576u,
               "G03 R4A evaluator workspace exceeds 1 MiB admission");

static arbor_status status_from_errno_value(int value)
{
    return arbor_status_from_native(-(int64_t)value);
}

static arbor_status ok_status(void)
{
    return arbor_status_from_native(0);
}

static uint8_t ascii_lower(uint8_t value)
{
    return value >= (uint8_t)'A' && value <= (uint8_t)'Z'
        ? (uint8_t)(value + ((uint8_t)'a' - (uint8_t)'A'))
        : value;
}

static bool span_ascii_ci_equals(arbor_span span, const char *literal)
{
    if (literal == NULL) return false;
    const size_t length = strlen(literal);
    if ((uint64_t)length != span.length || (span.length != 0u && span.data == NULL)) return false;
    for (uint64_t i = 0u; i < span.length; ++i) {
        if (ascii_lower(span.data[i]) != ascii_lower((uint8_t)literal[i])) return false;
    }
    return true;
}

static bool r1_owns_source_offset(const g03_r4a_context *context, uint64_t offset)
{
    if (context == NULL || offset == ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE) return false;
    for (uint64_t i = 0u; i < context->r1_source_offset_count; ++i) {
        if (context->r1_source_offsets[i] == offset) return true;
    }
    return false;
}

static bool frame_is_active_nothing(const g03_r4a_frame *frame)
{
    if (frame == NULL || !frame->authored) return false;
    switch (frame->standard_element_id) {
    case ARBOR_VIEW0_NATIVE_ELEMENT_IFRAME:
        return true;
    case ARBOR_VIEW0_NATIVE_ELEMENT_OPTION:
        return (frame->attribute_flags & (G03_R4_ATTR_LABEL | G03_R4_ATTR_VALUE)) ==
            (G03_R4_ATTR_LABEL | G03_R4_ATTR_VALUE);
    case ARBOR_VIEW0_NATIVE_ELEMENT_COLGROUP:
        return (frame->attribute_flags & G03_R4_ATTR_SPAN) != 0u;
    default:
        return false;
    }
}

static void fill_diagnostic(
    arbor_view0_native_diagnostic *diagnostic,
    uint64_t source_offset,
    uint64_t source_length,
    uint64_t discovery_sequence)
{
    static const char symbolic[] = "ARBOR_VIEW_V1_G03_NOTHING_MODEL";
    static const char message[] =
        "HTML element with a Nothing content model has disallowed contents";
    (void)memset(diagnostic, 0, sizeof(*diagnostic));
    diagnostic->rule_id = ARBOR_VIEW_V1_G03_NOTHING_MODEL;
    diagnostic->byte_offset = source_offset;
    diagnostic->source_length = source_length;
    diagnostic->discovery_sequence = discovery_sequence;
    diagnostic->severity = (uint32_t)ARBOR_VIEW0_NATIVE_SEVERITY_ERROR;
    diagnostic->origin = (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_AUTHORING;
    (void)memcpy(diagnostic->symbolic_name, symbolic, sizeof(symbolic));
    (void)memcpy(diagnostic->message, message, sizeof(message));
}

void arbor_view0_native_g03_r4a_materialize_anchor(
    const arbor_view0_native_source_anchor *anchor,
    uint64_t discovery_sequence,
    arbor_view0_native_diagnostic *diagnostic)
{
    fill_diagnostic(
        diagnostic, (uint64_t)anchor->byte_offset,
        (uint64_t)anchor->source_length, discovery_sequence);
}

static arbor_status report_invalid(g03_r4a_context *context, const g03_r4a_frame *frame)
{
    if (context == NULL || frame == NULL ||
        frame->source_offset == ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE ||
        frame->source_length == 0u) {
        return status_from_errno_value(EINVAL);
    }
    if (context->diagnostic_count == UINT64_MAX) return status_from_errno_value(EOVERFLOW);
    if (context->publish || context->collect_offsets || context->collect_anchors) {
        if (context->diagnostic_count >= context->diagnostic_capacity)
            return status_from_errno_value(ENOSPC);
        if (context->publish) {
            if (context->output.diagnostics == NULL) return status_from_errno_value(ENOSPC);
            if (context->discovery_sequence_base > UINT64_MAX - context->diagnostic_count)
                return status_from_errno_value(EOVERFLOW);
            fill_diagnostic(
                context->output.diagnostics + context->diagnostic_count,
                frame->source_offset, frame->source_length,
                context->discovery_sequence_base + context->diagnostic_count);
        } else if (context->collect_offsets && context->output.source_offsets != NULL) {
            context->output.source_offsets[context->diagnostic_count] = frame->source_offset;
        } else if (context->collect_anchors && context->output.anchors != NULL) {
            if (frame->source_offset > UINT32_MAX || frame->source_length > UINT32_MAX)
                return status_from_errno_value(EOVERFLOW);
            context->output.anchors[context->diagnostic_count] =
                (arbor_view0_native_source_anchor){
                    .byte_offset = (uint32_t)frame->source_offset,
                    .source_length = (uint32_t)frame->source_length
                };
        } else {
            return status_from_errno_value(ENOSPC);
        }
    }
    context->diagnostic_count += 1u;
    return ok_status();
}

static arbor_status r4a_traversal_enter(
    void *context_void,
    const arbor_view0_native_element_observation *observation)
{
    g03_r4a_context *context = context_void;
    if (context == NULL || observation == NULL) return status_from_errno_value(EINVAL);
    if (observation->depth > G03_R4A_MAX_DEPTH) return status_from_errno_value(E2BIG);
    context->current_depth = observation->depth;
    g03_r4a_frame *frame = &context->frames[observation->depth];
    (void)memset(frame, 0, sizeof(*frame));
    frame->standard_element_id = observation->standard_element_id;
    frame->source_offset = observation->source_offset;
    frame->source_length = observation->source_length;
    frame->authored = (observation->flags & ARBOR_VIEW0_NATIVE_ELEMENT_FLAG_SYNTHETIC) == 0u;
    if (frame->authored && frame->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_SELECTEDCONTENT) {
        context->deferred_flags |= ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R4_DEFERRED_SELECTEDCONTENT_PROVENANCE;
    }
    return ok_status();
}

static arbor_status r4a_attribute(
    void *context_void,
    const arbor_view0_native_attribute_observation *observation)
{
    g03_r4a_context *context = context_void;
    if (context == NULL || observation == NULL || context->current_depth > G03_R4A_MAX_DEPTH)
        return status_from_errno_value(EINVAL);
    g03_r4a_frame *frame = &context->frames[context->current_depth];
    if (span_ascii_ci_equals(observation->local_name, "label")) {
        frame->attribute_flags |= G03_R4_ATTR_LABEL;
    } else if (span_ascii_ci_equals(observation->local_name, "value")) {
        frame->attribute_flags |= G03_R4_ATTR_VALUE;
    } else if (span_ascii_ci_equals(observation->local_name, "span")) {
        frame->attribute_flags |= G03_R4_ATTR_SPAN;
    }
    return ok_status();
}

static arbor_status r4a_direct_child(
    void *context_void,
    const arbor_view0_native_direct_child_observation *child)
{
    g03_r4a_context *context = context_void;
    if (context == NULL || child == NULL || context->current_depth > G03_R4A_MAX_DEPTH)
        return status_from_errno_value(EINVAL);
    g03_r4a_frame *parent = &context->frames[context->current_depth];
    if (!frame_is_active_nothing(parent)) return ok_status();
    if (child->kind == ARBOR_VIEW0_NATIVE_DIRECT_CHILD_TEXT) {
        if ((child->flags & ARBOR_VIEW0_NATIVE_CHILD_FLAG_TEXT_INTER_ELEMENT_WHITESPACE) == 0u)
            parent->nonwhitespace_text_violation = true;
    } else if (child->kind == ARBOR_VIEW0_NATIVE_DIRECT_CHILD_ELEMENT &&
               !r1_owns_source_offset(context, child->source_offset)) {
        parent->unowned_element_child_violation = true;
    }
    return ok_status();
}

static arbor_status r4a_traversal_leave(
    void *context_void,
    const arbor_view0_native_element_observation *observation)
{
    g03_r4a_context *context = context_void;
    if (context == NULL || observation == NULL || observation->depth > G03_R4A_MAX_DEPTH)
        return status_from_errno_value(EINVAL);
    g03_r4a_frame *frame = &context->frames[observation->depth];
    arbor_status status = ok_status();
    if (frame_is_active_nothing(frame) &&
        (frame->nonwhitespace_text_violation || frame->unowned_element_child_violation)) {
        status = report_invalid(context, frame);
    }
    if (observation->depth != 0u) context->current_depth = observation->depth - 1u;
    return status;
}

static G03_R4A_NOINLINE arbor_status evaluate_with_r1_offsets(
    arbor_span input,
    arbor_view0_native_diagnostic *diagnostics,
    uint64_t *source_offsets,
    arbor_view0_native_source_anchor *anchors,
    uint64_t diagnostic_capacity,
    uint64_t discovery_sequence_base,
    bool publish,
    bool collect_offsets,
    bool collect_anchors,
    const uint64_t *r1_source_offsets,
    uint64_t r1_source_offset_count,
    arbor_view0_native_g03_r4a_evaluation *evaluation_out)
{
    if (evaluation_out == NULL ||
        (publish && (collect_offsets || collect_anchors)) ||
        (collect_offsets && collect_anchors) ||
        (publish && diagnostic_capacity != 0u && diagnostics == NULL) ||
        (collect_offsets && diagnostic_capacity != 0u && source_offsets == NULL) ||
        (collect_anchors && diagnostic_capacity != 0u && anchors == NULL))
        return status_from_errno_value(EINVAL);
    g03_r4a_context context = {
        .output = {.diagnostics = diagnostics},
        .diagnostic_capacity = diagnostic_capacity,
        .discovery_sequence_base = discovery_sequence_base,
        .diagnostic_count = 0u,
        .deferred_flags = 0u,
        .current_depth = 0u,
        .r1_source_offsets = r1_source_offsets,
        .r1_source_offset_count = r1_source_offset_count,
        .publish = publish,
        .collect_offsets = collect_offsets,
        .collect_anchors = collect_anchors,
        .frames = {{0}}
    };
    if (collect_offsets) {
        context.output.source_offsets = source_offsets;
    } else if (collect_anchors) {
        context.output.anchors = anchors;
    }
    arbor_view0_native_semantic_observer observer = {
        .context = &context,
        .element_begin = NULL,
        .attribute = r4a_attribute,
        .direct_child = r4a_direct_child,
        .element_complete = NULL,
        .traversal_enter = r4a_traversal_enter,
        .traversal_leave = r4a_traversal_leave
    };
    arbor_view0_native_parse_counts parse_counts = {0};
    arbor_view0_native_document_facts facts = {0};
    arbor_view0_native_observation_counts observations = {0};
    arbor_status status = arbor_view0_native_lexbor_observe(
        input, &observer, &parse_counts, &facts, &observations);
    if (status.native != 0) return status;
    if (observations.max_depth > G03_R4A_MAX_DEPTH) return status_from_errno_value(E2BIG);
    if ((publish || collect_offsets || collect_anchors) &&
        context.diagnostic_count != diagnostic_capacity)
        return status_from_errno_value(EIO);
    *evaluation_out = (arbor_view0_native_g03_r4a_evaluation){
        .diagnostic_count = context.diagnostic_count,
        .deferred_flags = context.deferred_flags
    };
    return ok_status();
}

static arbor_status evaluate(
    arbor_span input,
    arbor_view0_native_diagnostic *diagnostics,
    uint64_t *source_offsets,
    arbor_view0_native_source_anchor *anchors,
    uint64_t diagnostic_capacity,
    uint64_t discovery_sequence_base,
    bool publish,
    bool collect_offsets,
    bool collect_anchors,
    arbor_view0_native_g03_r4a_evaluation *evaluation_out)
{
    arbor_view0_native_g03_r1a_evaluation r1_measured = {0};
    arbor_status status = arbor_view0_native_g03_r1a_measure(input, &r1_measured);
    if (status.native != 0) return status;
    if (r1_measured.diagnostic_count > ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS)
        return status_from_errno_value(E2BIG);
    uint64_t r1_offsets[4096] = {0u};
    arbor_view0_native_g03_r1a_evaluation r1_collected = {0};
    status = arbor_view0_native_g03_r1a_collect_offsets(
        input, r1_offsets, r1_measured.diagnostic_count, &r1_collected);
    if (status.native != 0) return status;
    if (r1_collected.diagnostic_count != r1_measured.diagnostic_count ||
        r1_collected.deferred_main_form_count != r1_measured.deferred_main_form_count)
        return status_from_errno_value(EIO);
    return evaluate_with_r1_offsets(
        input, diagnostics, source_offsets, anchors, diagnostic_capacity, discovery_sequence_base,
        publish, collect_offsets, collect_anchors, r1_offsets, r1_collected.diagnostic_count,
        evaluation_out);
}

arbor_status arbor_view0_native_g03_r4a_measure(
    arbor_span input,
    arbor_view0_native_g03_r4a_evaluation *evaluation_out)
{
    return evaluate(input, NULL, NULL, NULL, 0u, 0u, false, false, false, evaluation_out);
}

arbor_status arbor_view0_native_g03_r4a_collect(
    arbor_span input,
    arbor_view0_native_diagnostic *diagnostics,
    uint64_t diagnostic_capacity,
    uint64_t discovery_sequence_base,
    arbor_view0_native_g03_r4a_evaluation *evaluation_out)
{
    return evaluate(input, diagnostics, NULL, NULL, diagnostic_capacity, discovery_sequence_base,
                    true, false, false, evaluation_out);
}

arbor_status arbor_view0_native_g03_r4a_collect_offsets(
    arbor_span input,
    uint64_t *source_offsets,
    uint64_t offset_capacity,
    arbor_view0_native_g03_r4a_evaluation *evaluation_out)
{
    return evaluate(input, NULL, source_offsets, NULL, offset_capacity, 0u,
                    false, true, false, evaluation_out);
}

arbor_status arbor_view0_native_g03_r4a_collect_anchors(
    arbor_span input,
    arbor_view0_native_source_anchor *anchors,
    uint64_t anchor_capacity,
    arbor_view0_native_g03_r4a_evaluation *evaluation_out)
{
    return evaluate(input, NULL, NULL, anchors, anchor_capacity, 0u,
                    false, false, true, evaluation_out);
}
