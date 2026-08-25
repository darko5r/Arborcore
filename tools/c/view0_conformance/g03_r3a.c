#include "g03_r3a.h"
#include "g03_r1a.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define G03_R3A_MAX_DEPTH UINT64_C(4097)
#define G03_R3A_FRAME_COUNT UINT64_C(4098)
#define G03_R3A_NOINLINE __attribute__((noinline))

#define R3_ATTR_HREF UINT64_C(1) << 0
#define R3_ATTR_CONTROLS UINT64_C(1) << 1
#define R3_ATTR_USEMAP UINT64_C(1) << 2
#define R3_ATTR_TYPE UINT64_C(1) << 3
#define R3_ATTR_TABINDEX UINT64_C(1) << 4
#define R3_ATTR_LABEL UINT64_C(1) << 5
#define R3_ATTR_MULTIPLE UINT64_C(1) << 6
#define R3_ATTR_SIZE UINT64_C(1) << 7

#define R3_ACT_NO_TABLE UINT64_C(1) << 0
#define R3_ACT_NO_FORM UINT64_C(1) << 1
#define R3_ACT_NO_HF_SECTIONING_HEADING UINT64_C(1) << 2
#define R3_ACT_NO_HF UINT64_C(1) << 3
#define R3_ACT_NO_HEADING_SECTIONING_HF_ADDRESS UINT64_C(1) << 4
#define R3_ACT_OPTION UINT64_C(1) << 5
#define R3_ACT_NO_MEDIA UINT64_C(1) << 6
#define R3_ACT_LEGEND UINT64_C(1) << 7
#define R3_ACT_NO_DFN UINT64_C(1) << 8
#define R3_ACT_BUTTON UINT64_C(1) << 9
#define R3_ACT_NO_METER UINT64_C(1) << 10
#define R3_ACT_NO_PROGRESS UINT64_C(1) << 11
#define R3_ACT_LABEL UINT64_C(1) << 12
#define R3_ACT_RUBY UINT64_C(1) << 13
#define R3_ACT_A UINT64_C(1) << 14
#define R3_ACT_CANVAS UINT64_C(1) << 15

#define R3_ACT_COUNT 16u

typedef enum r3_interactive_class {
    R3_INTERACTIVE_NO = 0,
    R3_INTERACTIVE_YES = 1,
    R3_INTERACTIVE_INPUT_TYPE_DEFERRED = 2
} r3_interactive_class;

typedef struct g03_r3a_frame {
    uint64_t standard_element_id;
    uint64_t namespace_id;
    uint64_t parent_standard_element_id;
    uint64_t source_offset;
    uint64_t source_length;
    uint64_t attribute_flags;
    uint64_t activated_bits;
    bool authored;
    bool datalist_ancestor;
} g03_r3a_frame;

typedef struct g03_r3a_context {
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
    bool publish;
    bool collect_offsets;
    bool collect_anchors;
    const uint64_t *r1_source_offsets;
    uint64_t r1_source_offset_count;
    uint64_t active[R3_ACT_COUNT];
    g03_r3a_frame frames[4098];
} g03_r3a_context;

_Static_assert(ARBOR_VIEW0_NATIVE_STANDARD_ELEMENT_COUNT == UINT64_C(113),
               "G03 R3A standard-element inventory drift");
_Static_assert(G03_R3A_FRAME_COUNT == G03_R3A_MAX_DEPTH + UINT64_C(1),
               "G03 R3A depth/workspace relationship drift");
_Static_assert(sizeof("ARBOR_VIEW_V1_G03_DESCENDANT_EXCLUSIONS") <=
                   ARBOR_VIEW0_NATIVE_RULE_SYMBOL_CAP,
               "G03 R3A symbolic name exceeds diagnostic capacity");
_Static_assert(sizeof("HTML element is forbidden as a descendant in this structural context") <=
                   ARBOR_VIEW0_NATIVE_MESSAGE_CAP,
               "G03 R3A message exceeds diagnostic capacity");
_Static_assert(sizeof(g03_r3a_frame) == 64u,
               "G03 R3A frame layout drift on x86-64");
_Static_assert(sizeof(g03_r3a_context) == 262472u,
               "G03 R3A bounded evaluator workspace layout drift on x86-64");
_Static_assert(sizeof(g03_r3a_context) <= 1048576u,
               "G03 R3A evaluator workspace exceeds 1 MiB admission");

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
    if (literal == NULL) {
        return false;
    }
    const size_t length = strlen(literal);
    if (span.length != (uint64_t)length || (length != 0u && span.data == NULL)) {
        return false;
    }
    for (size_t i = 0u; i < length; ++i) {
        if (ascii_lower(span.data[i]) != (uint8_t)literal[i]) {
            return false;
        }
    }
    return true;
}

static bool observation_has_ancestor(
    const arbor_view0_native_element_observation *observation,
    uint64_t standard_element_id)
{
    if (observation == NULL || standard_element_id == 0u ||
        standard_element_id > ARBOR_VIEW0_NATIVE_STANDARD_ELEMENT_COUNT) {
        return false;
    }
    const uint64_t bit = standard_element_id - 1u;
    return (observation->ancestor_bits[bit / 64u] &
            (UINT64_C(1) << (bit % 64u))) != 0u;
}

static bool is_heading(uint64_t id)
{
    return (id >= ARBOR_VIEW0_NATIVE_ELEMENT_H1 &&
            id <= ARBOR_VIEW0_NATIVE_ELEMENT_HGROUP);
}

static bool is_sectioning(uint64_t id)
{
    return id == ARBOR_VIEW0_NATIVE_ELEMENT_ARTICLE ||
        id == ARBOR_VIEW0_NATIVE_ELEMENT_SECTION ||
        id == ARBOR_VIEW0_NATIVE_ELEMENT_NAV ||
        id == ARBOR_VIEW0_NATIVE_ELEMENT_ASIDE;
}

static bool is_labelable(uint64_t id)
{
    return id == ARBOR_VIEW0_NATIVE_ELEMENT_BUTTON ||
        id == ARBOR_VIEW0_NATIVE_ELEMENT_INPUT ||
        id == ARBOR_VIEW0_NATIVE_ELEMENT_METER ||
        id == ARBOR_VIEW0_NATIVE_ELEMENT_OUTPUT ||
        id == ARBOR_VIEW0_NATIVE_ELEMENT_PROGRESS ||
        id == ARBOR_VIEW0_NATIVE_ELEMENT_SELECT ||
        id == ARBOR_VIEW0_NATIVE_ELEMENT_TEXTAREA;
}

static r3_interactive_class interactive_class(const g03_r3a_frame *frame)
{
    if (frame == NULL || frame->namespace_id != ARBOR_VIEW0_NATIVE_NAMESPACE_HTML) {
        return R3_INTERACTIVE_NO;
    }
    switch (frame->standard_element_id) {
    case ARBOR_VIEW0_NATIVE_ELEMENT_A:
        return (frame->attribute_flags & R3_ATTR_HREF) != 0u
            ? R3_INTERACTIVE_YES : R3_INTERACTIVE_NO;
    case ARBOR_VIEW0_NATIVE_ELEMENT_AUDIO:
    case ARBOR_VIEW0_NATIVE_ELEMENT_VIDEO:
        return (frame->attribute_flags & R3_ATTR_CONTROLS) != 0u
            ? R3_INTERACTIVE_YES : R3_INTERACTIVE_NO;
    case ARBOR_VIEW0_NATIVE_ELEMENT_BUTTON:
    case ARBOR_VIEW0_NATIVE_ELEMENT_DETAILS:
    case ARBOR_VIEW0_NATIVE_ELEMENT_EMBED:
    case ARBOR_VIEW0_NATIVE_ELEMENT_IFRAME:
    case ARBOR_VIEW0_NATIVE_ELEMENT_LABEL:
    case ARBOR_VIEW0_NATIVE_ELEMENT_SELECT:
    case ARBOR_VIEW0_NATIVE_ELEMENT_TEXTAREA:
        return R3_INTERACTIVE_YES;
    case ARBOR_VIEW0_NATIVE_ELEMENT_IMG:
        return (frame->attribute_flags & (R3_ATTR_USEMAP | R3_ATTR_CONTROLS)) != 0u
            ? R3_INTERACTIVE_YES : R3_INTERACTIVE_NO;
    case ARBOR_VIEW0_NATIVE_ELEMENT_INPUT:
        return (frame->attribute_flags & R3_ATTR_TYPE) != 0u
            ? R3_INTERACTIVE_INPUT_TYPE_DEFERRED
            : R3_INTERACTIVE_YES;
    default:
        return R3_INTERACTIVE_NO;
    }
}

static void set_deferred(g03_r3a_context *context, uint64_t flag)
{
    context->deferred_flags |= flag;
}

static void fill_diagnostic(
    arbor_view0_native_diagnostic *diagnostic,
    uint64_t source_offset,
    uint64_t source_length,
    uint64_t discovery_sequence)
{
    static const char symbolic[] = "ARBOR_VIEW_V1_G03_DESCENDANT_EXCLUSIONS";
    static const char message[] =
        "HTML element is forbidden as a descendant in this structural context";
    (void)memset(diagnostic, 0, sizeof(*diagnostic));
    diagnostic->rule_id = ARBOR_VIEW_V1_G03_DESCENDANT_EXCLUSIONS;
    diagnostic->byte_offset = source_offset;
    diagnostic->source_length = source_length;
    diagnostic->discovery_sequence = discovery_sequence;
    diagnostic->severity = (uint32_t)ARBOR_VIEW0_NATIVE_SEVERITY_ERROR;
    diagnostic->origin = (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_AUTHORING;
    (void)memcpy(diagnostic->symbolic_name, symbolic, sizeof(symbolic));
    (void)memcpy(diagnostic->message, message, sizeof(message));
}

void arbor_view0_native_g03_r3a_materialize_anchor(
    const arbor_view0_native_source_anchor *anchor,
    uint64_t discovery_sequence,
    arbor_view0_native_diagnostic *diagnostic)
{
    fill_diagnostic(
        diagnostic, (uint64_t)anchor->byte_offset,
        (uint64_t)anchor->source_length, discovery_sequence);
}

static arbor_status report_invalid(g03_r3a_context *context, const g03_r3a_frame *frame)
{
    if (context == NULL || frame == NULL || !frame->authored ||
        frame->source_offset == ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE ||
        frame->source_length == 0u) {
        return status_from_errno_value(EIO);
    }
    if (context->diagnostic_count == UINT64_MAX) {
        return status_from_errno_value(EOVERFLOW);
    }
    if (context->publish || context->collect_offsets || context->collect_anchors) {
        if (context->diagnostic_count >= context->diagnostic_capacity) {
            return status_from_errno_value(ENOSPC);
        }
        if (context->publish) {
            if (context->output.diagnostics == NULL) {
                return status_from_errno_value(ENOSPC);
            }
            if (context->discovery_sequence_base > UINT64_MAX - context->diagnostic_count) {
                return status_from_errno_value(EOVERFLOW);
            }
            fill_diagnostic(
                context->output.diagnostics + context->diagnostic_count,
                frame->source_offset, frame->source_length,
                context->discovery_sequence_base + context->diagnostic_count);
        } else if (context->collect_offsets && context->output.source_offsets != NULL) {
            context->output.source_offsets[context->diagnostic_count] = frame->source_offset;
        } else if (context->collect_anchors && context->output.anchors != NULL) {
            if (frame->source_offset > UINT32_MAX || frame->source_length > UINT32_MAX) {
                return status_from_errno_value(EOVERFLOW);
            }
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

static arbor_status activate(g03_r3a_context *context, g03_r3a_frame *frame, unsigned index)
{
    if (context == NULL || frame == NULL || index >= R3_ACT_COUNT ||
        context->active[index] == UINT64_MAX) {
        return status_from_errno_value(index < R3_ACT_COUNT ? EOVERFLOW : EINVAL);
    }
    context->active[index] += 1u;
    frame->activated_bits |= UINT64_C(1) << index;
    return ok_status();
}

static bool any_active(const g03_r3a_context *context, unsigned index)
{
    return context != NULL && index < R3_ACT_COUNT && context->active[index] != 0u;
}

static bool static_forbidden(const g03_r3a_context *context, uint64_t id)
{
    if (any_active(context, 0u) && id == ARBOR_VIEW0_NATIVE_ELEMENT_TABLE) return true;
    if (any_active(context, 1u) && id == ARBOR_VIEW0_NATIVE_ELEMENT_FORM) return true;
    if (any_active(context, 2u) &&
        (id == ARBOR_VIEW0_NATIVE_ELEMENT_HEADER || id == ARBOR_VIEW0_NATIVE_ELEMENT_FOOTER ||
         is_sectioning(id) || is_heading(id))) return true;
    if (any_active(context, 3u) &&
        (id == ARBOR_VIEW0_NATIVE_ELEMENT_HEADER || id == ARBOR_VIEW0_NATIVE_ELEMENT_FOOTER)) return true;
    if (any_active(context, 4u) &&
        (is_heading(id) || is_sectioning(id) || id == ARBOR_VIEW0_NATIVE_ELEMENT_HEADER ||
         id == ARBOR_VIEW0_NATIVE_ELEMENT_FOOTER || id == ARBOR_VIEW0_NATIVE_ELEMENT_ADDRESS)) return true;
    if (any_active(context, 6u) &&
        (id == ARBOR_VIEW0_NATIVE_ELEMENT_VIDEO || id == ARBOR_VIEW0_NATIVE_ELEMENT_AUDIO)) return true;
    if (any_active(context, 8u) && id == ARBOR_VIEW0_NATIVE_ELEMENT_DFN) return true;
    if (any_active(context, 10u) && id == ARBOR_VIEW0_NATIVE_ELEMENT_METER) return true;
    if (any_active(context, 11u) && id == ARBOR_VIEW0_NATIVE_ELEMENT_PROGRESS) return true;
    return false;
}

static bool canvas_definite_violation(
    g03_r3a_context *context,
    const g03_r3a_frame *frame,
    r3_interactive_class interactive)
{
    if (interactive == R3_INTERACTIVE_NO) {
        return false;
    }
    switch (frame->standard_element_id) {
    case ARBOR_VIEW0_NATIVE_ELEMENT_A:
    case ARBOR_VIEW0_NATIVE_ELEMENT_BUTTON:
        return false;
    case ARBOR_VIEW0_NATIVE_ELEMENT_IMG:
        return (frame->attribute_flags & R3_ATTR_USEMAP) == 0u;
    case ARBOR_VIEW0_NATIVE_ELEMENT_INPUT:
        if (interactive == R3_INTERACTIVE_INPUT_TYPE_DEFERRED) {
            set_deferred(context, ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_DEFERRED_CANVAS_INPUT_STATE);
            return false;
        }
        return true;
    case ARBOR_VIEW0_NATIVE_ELEMENT_SELECT:
        if ((frame->attribute_flags & R3_ATTR_MULTIPLE) != 0u) {
            return false;
        }
        if ((frame->attribute_flags & R3_ATTR_SIZE) != 0u) {
            set_deferred(context, ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_DEFERRED_CANVAS_SELECT_SIZE);
            return false;
        }
        return true;
    default:
        return true;
    }
}

static bool current_forbidden(g03_r3a_context *context, const g03_r3a_frame *frame)
{
    const uint64_t id = frame->standard_element_id;
    bool invalid = static_forbidden(context, id);
    const bool tabindex = (frame->attribute_flags & R3_ATTR_TABINDEX) != 0u;
    const r3_interactive_class interactive = interactive_class(frame);

    if (any_active(context, 5u)) {
        if (id == ARBOR_VIEW0_NATIVE_ELEMENT_DATALIST ||
            id == ARBOR_VIEW0_NATIVE_ELEMENT_OBJECT || tabindex ||
            interactive == R3_INTERACTIVE_YES) {
            invalid = true;
        } else if (interactive == R3_INTERACTIVE_INPUT_TYPE_DEFERRED) {
            set_deferred(context, ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_DEFERRED_INPUT_TYPE);
        }
    }
    if (any_active(context, 7u) || any_active(context, 9u)) {
        if (tabindex || interactive == R3_INTERACTIVE_YES) {
            invalid = true;
        } else if (interactive == R3_INTERACTIVE_INPUT_TYPE_DEFERRED) {
            set_deferred(context, ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_DEFERRED_INPUT_TYPE);
        }
    }
    if (any_active(context, 14u)) {
        if (id == ARBOR_VIEW0_NATIVE_ELEMENT_A || tabindex || interactive == R3_INTERACTIVE_YES) {
            invalid = true;
        } else if (interactive == R3_INTERACTIVE_INPUT_TYPE_DEFERRED) {
            set_deferred(context, ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_DEFERRED_INPUT_TYPE);
        }
    }
    if (any_active(context, 12u)) {
        if (id == ARBOR_VIEW0_NATIVE_ELEMENT_LABEL) {
            invalid = true;
        }
        if (is_labelable(id)) {
            set_deferred(context, ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_DEFERRED_LABELED_CONTROL);
        }
    }
    if (any_active(context, 15u) && canvas_definite_violation(context, frame, interactive)) {
        invalid = true;
    }
    if (any_active(context, 13u) && id == ARBOR_VIEW0_NATIVE_ELEMENT_RUBY) {
        const uint64_t ruby_ancestors = context->active[13u];
        const bool admitted_direct_nested_base =
            frame->parent_standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_RUBY &&
            ruby_ancestors == 1u;
        if (!admitted_direct_nested_base) {
            invalid = true;
        }
    }
    return invalid;
}

static arbor_status r3a_traversal_enter(
    void *context_void,
    const arbor_view0_native_element_observation *observation)
{
    g03_r3a_context *context = context_void;
    if (context == NULL || observation == NULL || observation->depth > G03_R3A_MAX_DEPTH) {
        return status_from_errno_value(observation != NULL ? E2BIG : EINVAL);
    }
    context->current_depth = observation->depth;
    g03_r3a_frame *frame = &context->frames[observation->depth];
    (void)memset(frame, 0, sizeof(*frame));
    frame->standard_element_id = observation->standard_element_id;
    frame->namespace_id = observation->namespace_id;
    frame->parent_standard_element_id = observation->parent_standard_element_id;
    frame->source_offset = observation->source_offset;
    frame->source_length = observation->source_length;
    frame->authored = (observation->flags & ARBOR_VIEW0_NATIVE_ELEMENT_FLAG_SYNTHETIC) == 0u;
    frame->datalist_ancestor = observation_has_ancestor(
        observation, ARBOR_VIEW0_NATIVE_ELEMENT_DATALIST);
    return ok_status();
}

static arbor_status r3a_attribute(
    void *context_void,
    const arbor_view0_native_attribute_observation *observation)
{
    g03_r3a_context *context = context_void;
    if (context == NULL || observation == NULL || context->current_depth > G03_R3A_MAX_DEPTH) {
        return status_from_errno_value(EINVAL);
    }
    g03_r3a_frame *frame = &context->frames[context->current_depth];
    if (span_ascii_ci_equals(observation->local_name, "href")) {
        frame->attribute_flags |= R3_ATTR_HREF;
    } else if (span_ascii_ci_equals(observation->local_name, "controls")) {
        frame->attribute_flags |= R3_ATTR_CONTROLS;
    } else if (span_ascii_ci_equals(observation->local_name, "usemap")) {
        frame->attribute_flags |= R3_ATTR_USEMAP;
    } else if (span_ascii_ci_equals(observation->local_name, "type")) {
        frame->attribute_flags |= R3_ATTR_TYPE;
    } else if (span_ascii_ci_equals(observation->local_name, "tabindex")) {
        frame->attribute_flags |= R3_ATTR_TABINDEX;
    } else if (span_ascii_ci_equals(observation->local_name, "label")) {
        frame->attribute_flags |= R3_ATTR_LABEL;
    } else if (span_ascii_ci_equals(observation->local_name, "multiple")) {
        frame->attribute_flags |= R3_ATTR_MULTIPLE;
    } else if (span_ascii_ci_equals(observation->local_name, "size")) {
        frame->attribute_flags |= R3_ATTR_SIZE;
    }
    return ok_status();
}

static arbor_status activate_parent_restriction(
    g03_r3a_context *context,
    g03_r3a_frame *frame)
{
    if (!frame->authored || frame->namespace_id != ARBOR_VIEW0_NATIVE_NAMESPACE_HTML) {
        return ok_status();
    }
    switch (frame->standard_element_id) {
    case ARBOR_VIEW0_NATIVE_ELEMENT_CAPTION: return activate(context, frame, 0u);
    case ARBOR_VIEW0_NATIVE_ELEMENT_FORM: return activate(context, frame, 1u);
    case ARBOR_VIEW0_NATIVE_ELEMENT_DT:
    case ARBOR_VIEW0_NATIVE_ELEMENT_TH: return activate(context, frame, 2u);
    case ARBOR_VIEW0_NATIVE_ELEMENT_HEADER:
    case ARBOR_VIEW0_NATIVE_ELEMENT_FOOTER: return activate(context, frame, 3u);
    case ARBOR_VIEW0_NATIVE_ELEMENT_ADDRESS: return activate(context, frame, 4u);
    case ARBOR_VIEW0_NATIVE_ELEMENT_OPTION:
        if ((frame->attribute_flags & R3_ATTR_LABEL) == 0u && !frame->datalist_ancestor) {
            return activate(context, frame, 5u);
        }
        return ok_status();
    case ARBOR_VIEW0_NATIVE_ELEMENT_VIDEO:
    case ARBOR_VIEW0_NATIVE_ELEMENT_AUDIO: return activate(context, frame, 6u);
    case ARBOR_VIEW0_NATIVE_ELEMENT_LEGEND:
        return frame->parent_standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_OPTGROUP
            ? activate(context, frame, 7u) : ok_status();
    case ARBOR_VIEW0_NATIVE_ELEMENT_DFN: return activate(context, frame, 8u);
    case ARBOR_VIEW0_NATIVE_ELEMENT_BUTTON: return activate(context, frame, 9u);
    case ARBOR_VIEW0_NATIVE_ELEMENT_METER: return activate(context, frame, 10u);
    case ARBOR_VIEW0_NATIVE_ELEMENT_PROGRESS: return activate(context, frame, 11u);
    case ARBOR_VIEW0_NATIVE_ELEMENT_LABEL: return activate(context, frame, 12u);
    case ARBOR_VIEW0_NATIVE_ELEMENT_RUBY: return activate(context, frame, 13u);
    case ARBOR_VIEW0_NATIVE_ELEMENT_A: return activate(context, frame, 14u);
    case ARBOR_VIEW0_NATIVE_ELEMENT_CANVAS: return activate(context, frame, 15u);
    case ARBOR_VIEW0_NATIVE_ELEMENT_NOSCRIPT:
        if (frame->parent_standard_element_id != ARBOR_VIEW0_NATIVE_ELEMENT_HEAD) {
            set_deferred(context, ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_DEFERRED_NOSCRIPT);
        }
        return ok_status();
    default:
        return ok_status();
    }
}


static bool r1_owns_source_offset(const g03_r3a_context *context, uint64_t offset)
{
    if (context == NULL || offset == ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE) {
        return false;
    }
    for (uint64_t i = 0u; i < context->r1_source_offset_count; ++i) {
        if (context->r1_source_offsets[i] == offset) {
            return true;
        }
    }
    return false;
}

static arbor_status r3a_element_complete(
    void *context_void,
    const arbor_view0_native_element_observation *observation)
{
    g03_r3a_context *context = context_void;
    if (context == NULL || observation == NULL || observation->depth > G03_R3A_MAX_DEPTH) {
        return status_from_errno_value(EINVAL);
    }
    g03_r3a_frame *frame = &context->frames[observation->depth];
    if (frame->authored && current_forbidden(context, frame) &&
        !r1_owns_source_offset(context, frame->source_offset)) {
        arbor_status status = report_invalid(context, frame);
        if (status.native != 0) {
            return status;
        }
    }
    return activate_parent_restriction(context, frame);
}

static arbor_status r3a_traversal_leave(
    void *context_void,
    const arbor_view0_native_element_observation *observation)
{
    g03_r3a_context *context = context_void;
    if (context == NULL || observation == NULL || observation->depth > G03_R3A_MAX_DEPTH) {
        return status_from_errno_value(EINVAL);
    }
    g03_r3a_frame *frame = &context->frames[observation->depth];
    for (unsigned index = 0u; index < R3_ACT_COUNT; ++index) {
        if ((frame->activated_bits & (UINT64_C(1) << index)) == 0u) {
            continue;
        }
        if (context->active[index] == 0u) {
            return status_from_errno_value(EIO);
        }
        context->active[index] -= 1u;
    }
    context->current_depth = observation->depth == 0u ? 0u : observation->depth - 1u;
    return ok_status();
}

static G03_R3A_NOINLINE arbor_status evaluate_with_r1_offsets(
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
    arbor_view0_native_g03_r3a_evaluation *evaluation_out)
{
    if (evaluation_out == NULL ||
        (publish && (collect_offsets || collect_anchors)) ||
        (collect_offsets && collect_anchors) ||
        (publish && diagnostic_capacity != 0u && diagnostics == NULL) ||
        (collect_offsets && diagnostic_capacity != 0u && source_offsets == NULL) ||
        (collect_anchors && diagnostic_capacity != 0u && anchors == NULL)) {
        return status_from_errno_value(EINVAL);
    }
    g03_r3a_context context = {
        .output = {.diagnostics = diagnostics},
        .diagnostic_capacity = diagnostic_capacity,
        .discovery_sequence_base = discovery_sequence_base,
        .diagnostic_count = 0u,
        .deferred_flags = 0u,
        .current_depth = 0u,
        .publish = publish,
        .collect_offsets = collect_offsets,
        .collect_anchors = collect_anchors,
        .r1_source_offsets = r1_source_offsets,
        .r1_source_offset_count = r1_source_offset_count,
        .active = {0u},
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
        .attribute = r3a_attribute,
        .direct_child = NULL,
        .element_complete = r3a_element_complete,
        .traversal_enter = r3a_traversal_enter,
        .traversal_leave = r3a_traversal_leave
    };
    arbor_view0_native_parse_counts parse_counts = {0};
    arbor_view0_native_document_facts facts = {0};
    arbor_view0_native_observation_counts observations = {0};
    arbor_status status = arbor_view0_native_lexbor_observe(
        input, &observer, &parse_counts, &facts, &observations);
    if (status.native != 0) {
        return status;
    }
    if (observations.max_depth > G03_R3A_MAX_DEPTH) {
        return status_from_errno_value(E2BIG);
    }
    if ((publish || collect_offsets || collect_anchors) &&
        context.diagnostic_count != diagnostic_capacity) {
        return status_from_errno_value(EIO);
    }
    const arbor_view0_native_g03_r3a_evaluation result = {
        .diagnostic_count = context.diagnostic_count,
        .deferred_flags = context.deferred_flags
    };
    *evaluation_out = result;
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
    arbor_view0_native_g03_r3a_evaluation *evaluation_out)
{
    arbor_view0_native_g03_r1a_evaluation r1_measured = {0};
    arbor_status status = arbor_view0_native_g03_r1a_measure(input, &r1_measured);
    if (status.native != 0) {
        return status;
    }
    if (r1_measured.diagnostic_count > ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS) {
        return status_from_errno_value(E2BIG);
    }
    uint64_t r1_offsets[4096] = {0u};
    arbor_view0_native_g03_r1a_evaluation r1_collected = {0};
    status = arbor_view0_native_g03_r1a_collect_offsets(
        input,
        r1_offsets,
        r1_measured.diagnostic_count,
        &r1_collected);
    if (status.native != 0) {
        return status;
    }
    if (r1_collected.diagnostic_count != r1_measured.diagnostic_count ||
        r1_collected.deferred_main_form_count != r1_measured.deferred_main_form_count) {
        return status_from_errno_value(EIO);
    }
    return evaluate_with_r1_offsets(
        input, diagnostics, source_offsets, anchors, diagnostic_capacity, discovery_sequence_base,
        publish, collect_offsets, collect_anchors, r1_offsets, r1_collected.diagnostic_count,
        evaluation_out);
}

arbor_status arbor_view0_native_g03_r3a_measure(
    arbor_span input,
    arbor_view0_native_g03_r3a_evaluation *evaluation_out)
{
    return evaluate(input, NULL, NULL, NULL, 0u, 0u, false, false, false, evaluation_out);
}

arbor_status arbor_view0_native_g03_r3a_collect(
    arbor_span input,
    arbor_view0_native_diagnostic *diagnostics,
    uint64_t diagnostic_capacity,
    uint64_t discovery_sequence_base,
    arbor_view0_native_g03_r3a_evaluation *evaluation_out)
{
    return evaluate(input, diagnostics, NULL, NULL, diagnostic_capacity,
                    discovery_sequence_base, true, false, false, evaluation_out);
}

arbor_status arbor_view0_native_g03_r3a_collect_offsets(
    arbor_span input,
    uint64_t *source_offsets,
    uint64_t offset_capacity,
    arbor_view0_native_g03_r3a_evaluation *evaluation_out)
{
    return evaluate(input, NULL, source_offsets, NULL, offset_capacity,
                    0u, false, true, false, evaluation_out);
}

arbor_status arbor_view0_native_g03_r3a_collect_anchors(
    arbor_span input,
    arbor_view0_native_source_anchor *anchors,
    uint64_t anchor_capacity,
    arbor_view0_native_g03_r3a_evaluation *evaluation_out)
{
    return evaluate(input, NULL, NULL, anchors, anchor_capacity,
                    0u, false, false, true, evaluation_out);
}
