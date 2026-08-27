#include "g03_r7a.h"
#include "g04_r1a.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define G03_R7A_MAX_DEPTH UINT64_C(4097)
#define G03_R7A_FRAME_COUNT UINT64_C(4098)
#define G03_R7A_NOINLINE __attribute__((noinline))

#define R7_ATTR_HIDDEN   (UINT64_C(1) << 0)
#define R7_ATTR_CONTROLS (UINT64_C(1) << 1)
#define R7_ATTR_TYPE_HIDDEN (UINT64_C(1) << 2)
#define R7_ATTR_DATETIME (UINT64_C(1) << 3)
#define R7_ATTR_LABEL    (UINT64_C(1) << 4)
#define R7_ATTR_VALUE    (UINT64_C(1) << 5)

typedef enum r7_child_class {
    R7_CHILD_NOT_PALPABLE = 0,
    R7_CHILD_PALPABLE = 1,
    R7_CHILD_UNKNOWN = 2
} r7_child_class;

typedef enum r7_subject_class {
    R7_SUBJECT_NO = 0,
    R7_SUBJECT_YES = 1,
    R7_SUBJECT_DEFER_G13 = 2
} r7_subject_class;

typedef struct g03_r7a_frame {
    uint64_t standard_element_id;
    uint64_t namespace_id;
    uint64_t parent_standard_element_id;
    uint64_t source_offset;
    uint64_t source_length;
    uint64_t attribute_flags;
    uint8_t name_value_state;
    bool authored;
    bool autonomous_custom;
    bool foreign_math;
    bool foreign_svg;
    bool direct_child_satisfies;
    bool direct_child_unknown;
    bool has_li_child;
    bool has_name_value_group;
    bool datalist_ancestor;
} g03_r7a_frame;

typedef struct g03_r7a_context {
    union {
        arbor_view0_native_diagnostic *diagnostics;
        arbor_view0_native_source_anchor *anchors;
    } output;
    uint64_t diagnostic_capacity;
    uint64_t discovery_sequence_base;
    uint64_t diagnostic_count;
    uint64_t deferred_flags;
    uint64_t g04_deferred_count;
    uint64_t g13_deferred_count;
    uint64_t current_depth;
    bool publish;
    bool collect_anchors;
    g03_r7a_frame frames[G03_R7A_FRAME_COUNT];
} g03_r7a_context;

_Static_assert(ARBOR_VIEW0_NATIVE_STANDARD_ELEMENT_COUNT == UINT64_C(113),
               "G03 R7A standard-element inventory drift");
_Static_assert(G03_R7A_FRAME_COUNT == G03_R7A_MAX_DEPTH + UINT64_C(1),
               "G03 R7A depth/workspace relationship drift");
_Static_assert(sizeof("ARBOR_VIEW_V1_G03_PALPABLE_PHRASING_NONEMPTY") <=
                   ARBOR_VIEW0_NATIVE_RULE_SYMBOL_CAP,
               "G03 R7A symbolic name exceeds diagnostic capacity");
_Static_assert(sizeof("Element allowing flow or phrasing content should have a non-hidden palpable direct child") <=
                   ARBOR_VIEW0_NATIVE_MESSAGE_CAP,
               "G03 R7A message exceeds diagnostic capacity");
_Static_assert(sizeof(g03_r7a_context) <= 1048576u,
               "G03 R7A evaluator workspace exceeds 1 MiB admission");

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

static bool span_has_hyphen(arbor_span span)
{
    if (span.length != 0u && span.data == NULL) return false;
    for (uint64_t i = 0u; i < span.length; ++i) if (span.data[i] == (uint8_t)'-') return true;
    return false;
}

static bool observation_has_ancestor(
    const arbor_view0_native_element_observation *observation, uint64_t id)
{
    if (observation == NULL || id == 0u || id > ARBOR_VIEW0_NATIVE_STANDARD_ELEMENT_COUNT)
        return false;
    const uint64_t bit = id - 1u;
    return (observation->ancestor_bits[bit / 64u] & (UINT64_C(1) << (bit % 64u))) != 0u;
}

static bool stable_subject(uint64_t id)
{
    switch (id) {
    case ARBOR_VIEW0_NATIVE_ELEMENT_BODY:
    case ARBOR_VIEW0_NATIVE_ELEMENT_ARTICLE:
    case ARBOR_VIEW0_NATIVE_ELEMENT_SECTION:
    case ARBOR_VIEW0_NATIVE_ELEMENT_NAV:
    case ARBOR_VIEW0_NATIVE_ELEMENT_ASIDE:
    case ARBOR_VIEW0_NATIVE_ELEMENT_H1:
    case ARBOR_VIEW0_NATIVE_ELEMENT_H2:
    case ARBOR_VIEW0_NATIVE_ELEMENT_H3:
    case ARBOR_VIEW0_NATIVE_ELEMENT_H4:
    case ARBOR_VIEW0_NATIVE_ELEMENT_H5:
    case ARBOR_VIEW0_NATIVE_ELEMENT_H6:
    case ARBOR_VIEW0_NATIVE_ELEMENT_HEADER:
    case ARBOR_VIEW0_NATIVE_ELEMENT_FOOTER:
    case ARBOR_VIEW0_NATIVE_ELEMENT_ADDRESS:
    case ARBOR_VIEW0_NATIVE_ELEMENT_P:
    case ARBOR_VIEW0_NATIVE_ELEMENT_PRE:
    case ARBOR_VIEW0_NATIVE_ELEMENT_BLOCKQUOTE:
    case ARBOR_VIEW0_NATIVE_ELEMENT_LI:
    case ARBOR_VIEW0_NATIVE_ELEMENT_DT:
    case ARBOR_VIEW0_NATIVE_ELEMENT_DD:
    case ARBOR_VIEW0_NATIVE_ELEMENT_FIGURE:
    case ARBOR_VIEW0_NATIVE_ELEMENT_FIGCAPTION:
    case ARBOR_VIEW0_NATIVE_ELEMENT_MAIN:
    case ARBOR_VIEW0_NATIVE_ELEMENT_SEARCH:
    case ARBOR_VIEW0_NATIVE_ELEMENT_EM:
    case ARBOR_VIEW0_NATIVE_ELEMENT_STRONG:
    case ARBOR_VIEW0_NATIVE_ELEMENT_SMALL:
    case ARBOR_VIEW0_NATIVE_ELEMENT_S:
    case ARBOR_VIEW0_NATIVE_ELEMENT_CITE:
    case ARBOR_VIEW0_NATIVE_ELEMENT_Q:
    case ARBOR_VIEW0_NATIVE_ELEMENT_DFN:
    case ARBOR_VIEW0_NATIVE_ELEMENT_ABBR:
    case ARBOR_VIEW0_NATIVE_ELEMENT_RT:
    case ARBOR_VIEW0_NATIVE_ELEMENT_DATA:
    case ARBOR_VIEW0_NATIVE_ELEMENT_CODE:
    case ARBOR_VIEW0_NATIVE_ELEMENT_VAR:
    case ARBOR_VIEW0_NATIVE_ELEMENT_SAMP:
    case ARBOR_VIEW0_NATIVE_ELEMENT_KBD:
    case ARBOR_VIEW0_NATIVE_ELEMENT_SUB:
    case ARBOR_VIEW0_NATIVE_ELEMENT_SUP:
    case ARBOR_VIEW0_NATIVE_ELEMENT_I:
    case ARBOR_VIEW0_NATIVE_ELEMENT_B:
    case ARBOR_VIEW0_NATIVE_ELEMENT_U:
    case ARBOR_VIEW0_NATIVE_ELEMENT_MARK:
    case ARBOR_VIEW0_NATIVE_ELEMENT_BDI:
    case ARBOR_VIEW0_NATIVE_ELEMENT_BDO:
    case ARBOR_VIEW0_NATIVE_ELEMENT_SPAN:
    case ARBOR_VIEW0_NATIVE_ELEMENT_CAPTION:
    case ARBOR_VIEW0_NATIVE_ELEMENT_TD:
    case ARBOR_VIEW0_NATIVE_ELEMENT_TH:
    case ARBOR_VIEW0_NATIVE_ELEMENT_FORM:
    case ARBOR_VIEW0_NATIVE_ELEMENT_LABEL:
    case ARBOR_VIEW0_NATIVE_ELEMENT_BUTTON:
    case ARBOR_VIEW0_NATIVE_ELEMENT_DATALIST:
    case ARBOR_VIEW0_NATIVE_ELEMENT_OUTPUT:
    case ARBOR_VIEW0_NATIVE_ELEMENT_PROGRESS:
    case ARBOR_VIEW0_NATIVE_ELEMENT_METER:
    case ARBOR_VIEW0_NATIVE_ELEMENT_FIELDSET:
    case ARBOR_VIEW0_NATIVE_ELEMENT_LEGEND:
    case ARBOR_VIEW0_NATIVE_ELEMENT_DETAILS:
    case ARBOR_VIEW0_NATIVE_ELEMENT_SUMMARY:
    case ARBOR_VIEW0_NATIVE_ELEMENT_DIALOG:
        return true;
    default:
        return false;
    }
}

static bool unconditional_standard_palpable(uint64_t id)
{
    switch (id) {
    case ARBOR_VIEW0_NATIVE_ELEMENT_A:
    case ARBOR_VIEW0_NATIVE_ELEMENT_ABBR:
    case ARBOR_VIEW0_NATIVE_ELEMENT_ADDRESS:
    case ARBOR_VIEW0_NATIVE_ELEMENT_ARTICLE:
    case ARBOR_VIEW0_NATIVE_ELEMENT_ASIDE:
    case ARBOR_VIEW0_NATIVE_ELEMENT_B:
    case ARBOR_VIEW0_NATIVE_ELEMENT_BDI:
    case ARBOR_VIEW0_NATIVE_ELEMENT_BDO:
    case ARBOR_VIEW0_NATIVE_ELEMENT_BLOCKQUOTE:
    case ARBOR_VIEW0_NATIVE_ELEMENT_BUTTON:
    case ARBOR_VIEW0_NATIVE_ELEMENT_CANVAS:
    case ARBOR_VIEW0_NATIVE_ELEMENT_CITE:
    case ARBOR_VIEW0_NATIVE_ELEMENT_CODE:
    case ARBOR_VIEW0_NATIVE_ELEMENT_DATA:
    case ARBOR_VIEW0_NATIVE_ELEMENT_DEL:
    case ARBOR_VIEW0_NATIVE_ELEMENT_DETAILS:
    case ARBOR_VIEW0_NATIVE_ELEMENT_DFN:
    case ARBOR_VIEW0_NATIVE_ELEMENT_DIV:
    case ARBOR_VIEW0_NATIVE_ELEMENT_EM:
    case ARBOR_VIEW0_NATIVE_ELEMENT_EMBED:
    case ARBOR_VIEW0_NATIVE_ELEMENT_FIELDSET:
    case ARBOR_VIEW0_NATIVE_ELEMENT_FIGURE:
    case ARBOR_VIEW0_NATIVE_ELEMENT_FOOTER:
    case ARBOR_VIEW0_NATIVE_ELEMENT_FORM:
    case ARBOR_VIEW0_NATIVE_ELEMENT_H1:
    case ARBOR_VIEW0_NATIVE_ELEMENT_H2:
    case ARBOR_VIEW0_NATIVE_ELEMENT_H3:
    case ARBOR_VIEW0_NATIVE_ELEMENT_H4:
    case ARBOR_VIEW0_NATIVE_ELEMENT_H5:
    case ARBOR_VIEW0_NATIVE_ELEMENT_H6:
    case ARBOR_VIEW0_NATIVE_ELEMENT_HEADER:
    case ARBOR_VIEW0_NATIVE_ELEMENT_HGROUP:
    case ARBOR_VIEW0_NATIVE_ELEMENT_I:
    case ARBOR_VIEW0_NATIVE_ELEMENT_IFRAME:
    case ARBOR_VIEW0_NATIVE_ELEMENT_IMG:
    case ARBOR_VIEW0_NATIVE_ELEMENT_INS:
    case ARBOR_VIEW0_NATIVE_ELEMENT_KBD:
    case ARBOR_VIEW0_NATIVE_ELEMENT_LABEL:
    case ARBOR_VIEW0_NATIVE_ELEMENT_MAIN:
    case ARBOR_VIEW0_NATIVE_ELEMENT_MAP:
    case ARBOR_VIEW0_NATIVE_ELEMENT_MARK:
    case ARBOR_VIEW0_NATIVE_ELEMENT_METER:
    case ARBOR_VIEW0_NATIVE_ELEMENT_NAV:
    case ARBOR_VIEW0_NATIVE_ELEMENT_OBJECT:
    case ARBOR_VIEW0_NATIVE_ELEMENT_OUTPUT:
    case ARBOR_VIEW0_NATIVE_ELEMENT_P:
    case ARBOR_VIEW0_NATIVE_ELEMENT_PICTURE:
    case ARBOR_VIEW0_NATIVE_ELEMENT_PRE:
    case ARBOR_VIEW0_NATIVE_ELEMENT_PROGRESS:
    case ARBOR_VIEW0_NATIVE_ELEMENT_Q:
    case ARBOR_VIEW0_NATIVE_ELEMENT_RUBY:
    case ARBOR_VIEW0_NATIVE_ELEMENT_S:
    case ARBOR_VIEW0_NATIVE_ELEMENT_SAMP:
    case ARBOR_VIEW0_NATIVE_ELEMENT_SEARCH:
    case ARBOR_VIEW0_NATIVE_ELEMENT_SECTION:
    case ARBOR_VIEW0_NATIVE_ELEMENT_SELECT:
    case ARBOR_VIEW0_NATIVE_ELEMENT_SMALL:
    case ARBOR_VIEW0_NATIVE_ELEMENT_SPAN:
    case ARBOR_VIEW0_NATIVE_ELEMENT_STRONG:
    case ARBOR_VIEW0_NATIVE_ELEMENT_SUB:
    case ARBOR_VIEW0_NATIVE_ELEMENT_SUP:
    case ARBOR_VIEW0_NATIVE_ELEMENT_TABLE:
    case ARBOR_VIEW0_NATIVE_ELEMENT_TEXTAREA:
    case ARBOR_VIEW0_NATIVE_ELEMENT_TIME:
    case ARBOR_VIEW0_NATIVE_ELEMENT_U:
    case ARBOR_VIEW0_NATIVE_ELEMENT_VAR:
    case ARBOR_VIEW0_NATIVE_ELEMENT_VIDEO:
        return true;
    default:
        return false;
    }
}

static void set_deferred(g03_r7a_context *context, uint64_t flag, bool g04)
{
    if (context == NULL) return;
    context->deferred_flags |= flag;
    if (g04) {
        if (context->g04_deferred_count != UINT64_MAX) context->g04_deferred_count += 1u;
    } else {
        if (context->g13_deferred_count != UINT64_MAX) context->g13_deferred_count += 1u;
    }
}

static void fill_diagnostic(
    arbor_view0_native_diagnostic *diagnostic,
    uint64_t source_offset,
    uint64_t source_length,
    uint64_t discovery_sequence)
{
    static const char symbolic[] = "ARBOR_VIEW_V1_G03_PALPABLE_PHRASING_NONEMPTY";
    static const char message[] =
        "Element allowing flow or phrasing content should have a non-hidden palpable direct child";
    (void)memset(diagnostic, 0, sizeof(*diagnostic));
    diagnostic->rule_id = ARBOR_VIEW_V1_G03_PALPABLE_PHRASING_NONEMPTY;
    diagnostic->byte_offset = source_offset;
    diagnostic->source_length = source_length;
    diagnostic->discovery_sequence = discovery_sequence;
    diagnostic->severity = (uint32_t)ARBOR_VIEW0_NATIVE_SEVERITY_WARNING;
    diagnostic->origin = (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_AUTHORING;
    (void)memcpy(diagnostic->symbolic_name, symbolic, sizeof(symbolic));
    (void)memcpy(diagnostic->message, message, sizeof(message));
}

void arbor_view0_native_g03_r7a_materialize_anchor(
    const arbor_view0_native_source_anchor *anchor,
    uint64_t discovery_sequence,
    arbor_view0_native_diagnostic *diagnostic)
{
    fill_diagnostic(
        diagnostic, (uint64_t)anchor->byte_offset,
        (uint64_t)anchor->source_length, discovery_sequence);
}

static arbor_status report_warning(g03_r7a_context *context, const g03_r7a_frame *frame)
{
    if (context == NULL || frame == NULL || !frame->authored ||
        frame->source_offset == ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE || frame->source_length == 0u)
        return status_from_errno_value(EIO);
    if (context->diagnostic_count == UINT64_MAX) return status_from_errno_value(EOVERFLOW);
    if (context->publish || context->collect_anchors) {
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
        } else if (context->output.anchors != NULL) {
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

static void update_name_value_state(g03_r7a_frame *parent, uint64_t child_id)
{
    if (parent == NULL) return;
    if (child_id == ARBOR_VIEW0_NATIVE_ELEMENT_SCRIPT || child_id == ARBOR_VIEW0_NATIVE_ELEMENT_TEMPLATE)
        return;
    if (child_id == ARBOR_VIEW0_NATIVE_ELEMENT_DT) {
        parent->name_value_state = 1u;
    } else if (child_id == ARBOR_VIEW0_NATIVE_ELEMENT_DD) {
        if (parent->name_value_state == 1u || parent->name_value_state == 2u) {
            parent->has_name_value_group = true;
            parent->name_value_state = 2u;
        } else {
            parent->name_value_state = 0u;
        }
    } else {
        parent->name_value_state = 0u;
    }
}

static r7_child_class classify_element_palpability(const g03_r7a_frame *frame)
{
    if (frame == NULL) return R7_CHILD_NOT_PALPABLE;
    /* The general R7 rule excludes any otherwise-palpable child carrying hidden. */
    if ((frame->attribute_flags & R7_ATTR_HIDDEN) != 0u) return R7_CHILD_NOT_PALPABLE;
    if (frame->foreign_math || frame->foreign_svg) return R7_CHILD_PALPABLE;
    if (frame->namespace_id != ARBOR_VIEW0_NATIVE_NAMESPACE_HTML) return R7_CHILD_NOT_PALPABLE;
    if (frame->autonomous_custom) return R7_CHILD_UNKNOWN;
    if (unconditional_standard_palpable(frame->standard_element_id)) return R7_CHILD_PALPABLE;
    switch (frame->standard_element_id) {
    case ARBOR_VIEW0_NATIVE_ELEMENT_AUDIO:
        return (frame->attribute_flags & R7_ATTR_CONTROLS) != 0u
            ? R7_CHILD_PALPABLE : R7_CHILD_NOT_PALPABLE;
    case ARBOR_VIEW0_NATIVE_ELEMENT_INPUT:
        return (frame->attribute_flags & R7_ATTR_TYPE_HIDDEN) == 0u
            ? R7_CHILD_PALPABLE : R7_CHILD_NOT_PALPABLE;
    case ARBOR_VIEW0_NATIVE_ELEMENT_DL:
        return frame->has_name_value_group ? R7_CHILD_PALPABLE : R7_CHILD_NOT_PALPABLE;
    case ARBOR_VIEW0_NATIVE_ELEMENT_MENU:
    case ARBOR_VIEW0_NATIVE_ELEMENT_OL:
    case ARBOR_VIEW0_NATIVE_ELEMENT_UL:
        return frame->has_li_child ? R7_CHILD_PALPABLE : R7_CHILD_NOT_PALPABLE;
    default:
        return R7_CHILD_NOT_PALPABLE;
    }
}

static r7_subject_class classify_subject(
    g03_r7a_context *context,
    const g03_r7a_frame *frame,
    const arbor_view0_native_element_observation *observation)
{
    if (context == NULL || frame == NULL || observation == NULL || !frame->authored)
        return R7_SUBJECT_NO;
    if (frame->namespace_id != ARBOR_VIEW0_NATIVE_NAMESPACE_HTML) return R7_SUBJECT_NO;
    if (frame->autonomous_custom) {
        set_deferred(context, ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R7_DEFERRED_G13_CUSTOM, false);
        return R7_SUBJECT_DEFER_G13;
    }
    if (stable_subject(frame->standard_element_id)) return R7_SUBJECT_YES;
    if (frame->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_DIV) {
        if (frame->parent_standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_DL)
            return R7_SUBJECT_NO;
        for (uint64_t depth = observation->depth; depth != 0u; --depth) {
            const g03_r7a_frame *ancestor = &context->frames[depth - 1u];
            if (ancestor->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_OPTION) {
                return arbor_view0_native_g04_select_transparent_div_is_r7_subject(
                    ARBOR_VIEW0_NATIVE_ELEMENT_OPTION,
                    (ancestor->attribute_flags & R7_ATTR_LABEL) != 0u,
                    ancestor->datalist_ancestor)
                    ? R7_SUBJECT_YES : R7_SUBJECT_NO;
            }
            if (ancestor->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_OPTGROUP ||
                ancestor->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_SELECT) {
                return R7_SUBJECT_NO;
            }
        }
        return R7_SUBJECT_YES;
    }
    if (frame->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_TIME)
        return (frame->attribute_flags & R7_ATTR_DATETIME) != 0u ? R7_SUBJECT_YES : R7_SUBJECT_NO;
    if (frame->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_OPTION) {
        if ((frame->attribute_flags & R7_ATTR_LABEL) != 0u) return R7_SUBJECT_NO;
        if (observation_has_ancestor(observation, ARBOR_VIEW0_NATIVE_ELEMENT_DATALIST))
            return R7_SUBJECT_NO;
        return R7_SUBJECT_YES;
    }
    return R7_SUBJECT_NO;
}

static arbor_status r7a_traversal_enter(void *context_void, const arbor_view0_native_element_observation *observation)
{
    g03_r7a_context *context = context_void;
    if (context == NULL || observation == NULL) return status_from_errno_value(EINVAL);
    if (observation->depth > G03_R7A_MAX_DEPTH) return status_from_errno_value(E2BIG);
    context->current_depth = observation->depth;
    g03_r7a_frame *frame = &context->frames[observation->depth];
    (void)memset(frame, 0, sizeof(*frame));
    frame->standard_element_id = observation->standard_element_id;
    frame->namespace_id = observation->namespace_id;
    frame->parent_standard_element_id = observation->parent_standard_element_id;
    frame->source_offset = observation->source_offset;
    frame->source_length = observation->source_length;
    frame->authored = (observation->flags & ARBOR_VIEW0_NATIVE_ELEMENT_FLAG_SYNTHETIC) == 0u;
    frame->datalist_ancestor = observation_has_ancestor(
        observation, ARBOR_VIEW0_NATIVE_ELEMENT_DATALIST);
    frame->autonomous_custom = observation->namespace_id == ARBOR_VIEW0_NATIVE_NAMESPACE_HTML &&
        observation->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_NONE &&
        span_has_hyphen(observation->local_name);
    frame->foreign_math = observation->namespace_id == ARBOR_VIEW0_NATIVE_NAMESPACE_MATHML &&
        span_ascii_ci_equals(observation->local_name, "math");
    frame->foreign_svg = observation->namespace_id == ARBOR_VIEW0_NATIVE_NAMESPACE_SVG &&
        span_ascii_ci_equals(observation->local_name, "svg");
    return ok_status();
}

static arbor_status r7a_attribute(void *context_void, const arbor_view0_native_attribute_observation *observation)
{
    g03_r7a_context *context = context_void;
    if (context == NULL || observation == NULL || context->current_depth > G03_R7A_MAX_DEPTH)
        return status_from_errno_value(EINVAL);
    g03_r7a_frame *frame = &context->frames[context->current_depth];
    if (span_ascii_ci_equals(observation->local_name, "hidden")) frame->attribute_flags |= R7_ATTR_HIDDEN;
    else if (span_ascii_ci_equals(observation->local_name, "controls")) frame->attribute_flags |= R7_ATTR_CONTROLS;
    else if (span_ascii_ci_equals(observation->local_name, "type") &&
             span_ascii_ci_equals(observation->value, "hidden")) frame->attribute_flags |= R7_ATTR_TYPE_HIDDEN;
    else if (span_ascii_ci_equals(observation->local_name, "datetime")) frame->attribute_flags |= R7_ATTR_DATETIME;
    else if (span_ascii_ci_equals(observation->local_name, "label")) frame->attribute_flags |= R7_ATTR_LABEL;
    else if (span_ascii_ci_equals(observation->local_name, "value")) frame->attribute_flags |= R7_ATTR_VALUE;
    return ok_status();
}

static arbor_status r7a_direct_child(void *context_void, const arbor_view0_native_direct_child_observation *child)
{
    g03_r7a_context *context = context_void;
    if (context == NULL || child == NULL || context->current_depth > G03_R7A_MAX_DEPTH)
        return status_from_errno_value(EINVAL);
    g03_r7a_frame *parent = &context->frames[context->current_depth];
    if (child->kind == ARBOR_VIEW0_NATIVE_DIRECT_CHILD_TEXT) {
        if ((child->flags & ARBOR_VIEW0_NATIVE_CHILD_FLAG_TEXT_INTER_ELEMENT_WHITESPACE) == 0u)
            parent->direct_child_satisfies = true;
        return ok_status();
    }
    if (child->kind != ARBOR_VIEW0_NATIVE_DIRECT_CHILD_ELEMENT) return status_from_errno_value(EIO);
    if (child->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_LI) parent->has_li_child = true;
    update_name_value_state(parent, child->standard_element_id);
    return ok_status();
}

static arbor_status r7a_traversal_leave(void *context_void, const arbor_view0_native_element_observation *observation)
{
    g03_r7a_context *context = context_void;
    if (context == NULL || observation == NULL || observation->depth > G03_R7A_MAX_DEPTH)
        return status_from_errno_value(EINVAL);
    g03_r7a_frame *frame = &context->frames[observation->depth];
    const r7_subject_class subject = classify_subject(context, frame, observation);
    arbor_status status = ok_status();
    if (subject == R7_SUBJECT_YES && !frame->direct_child_satisfies && !frame->direct_child_unknown)
        status = report_warning(context, frame);
    if (status.native != 0) return status;
    if (observation->depth != 0u) {
        g03_r7a_frame *parent = &context->frames[observation->depth - 1u];
        if (parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_DL &&
            frame->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_DIV &&
            frame->has_name_value_group)
            parent->has_name_value_group = true;
        const r7_child_class child_class = classify_element_palpability(frame);
        if (child_class == R7_CHILD_PALPABLE) parent->direct_child_satisfies = true;
        else if (child_class == R7_CHILD_UNKNOWN) {
            parent->direct_child_unknown = true;
            set_deferred(context, ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R7_DEFERRED_G13_CUSTOM, false);
        }
        context->current_depth = observation->depth - 1u;
    }
    return ok_status();
}

static G03_R7A_NOINLINE arbor_status evaluate(
    arbor_span input,
    arbor_view0_native_diagnostic *diagnostics,
    arbor_view0_native_source_anchor *anchors,
    uint64_t diagnostic_capacity,
    uint64_t discovery_sequence_base,
    bool publish,
    bool collect_anchors,
    arbor_view0_native_g03_r7a_evaluation *evaluation_out)
{
    if (evaluation_out == NULL || (publish && collect_anchors) ||
        (publish && diagnostic_capacity != 0u && diagnostics == NULL) ||
        (collect_anchors && diagnostic_capacity != 0u && anchors == NULL))
        return status_from_errno_value(EINVAL);
    g03_r7a_context context = {
        .output = {.diagnostics = diagnostics},
        .diagnostic_capacity = diagnostic_capacity,
        .discovery_sequence_base = discovery_sequence_base,
        .diagnostic_count = 0u,
        .deferred_flags = 0u,
        .g04_deferred_count = 0u,
        .g13_deferred_count = 0u,
        .current_depth = 0u,
        .publish = publish,
        .collect_anchors = collect_anchors
    };
    if (collect_anchors) context.output.anchors = anchors;
    const arbor_view0_native_semantic_observer observer = {
        .context = &context,
        .attribute = r7a_attribute,
        .direct_child = r7a_direct_child,
        .traversal_enter = r7a_traversal_enter,
        .traversal_leave = r7a_traversal_leave
    };
    arbor_view0_native_parse_counts parse_counts = {0};
    arbor_view0_native_document_facts facts = {0};
    arbor_view0_native_observation_counts observations = {0};
    arbor_status status = arbor_view0_native_lexbor_observe(
        input, &observer, &parse_counts, &facts, &observations);
    if (status.native != 0) return status;
    if (observations.max_depth > G03_R7A_MAX_DEPTH) return status_from_errno_value(E2BIG);
    if ((publish || collect_anchors) && context.diagnostic_count != diagnostic_capacity)
        return status_from_errno_value(EIO);
    *evaluation_out = (arbor_view0_native_g03_r7a_evaluation){
        .diagnostic_count = context.diagnostic_count,
        .deferred_flags = context.deferred_flags,
        .g04_deferred_count = context.g04_deferred_count,
        .g13_deferred_count = context.g13_deferred_count
    };
    return ok_status();
}

arbor_status arbor_view0_native_g03_r7a_measure(
    arbor_span input, arbor_view0_native_g03_r7a_evaluation *evaluation_out)
{
    return evaluate(input, NULL, NULL, 0u, 0u, false, false, evaluation_out);
}

arbor_status arbor_view0_native_g03_r7a_collect(
    arbor_span input, arbor_view0_native_diagnostic *diagnostics, uint64_t diagnostic_capacity,
    uint64_t discovery_sequence_base, arbor_view0_native_g03_r7a_evaluation *evaluation_out)
{
    return evaluate(input, diagnostics, NULL, diagnostic_capacity, discovery_sequence_base, true, false, evaluation_out);
}

arbor_status arbor_view0_native_g03_r7a_collect_anchors(
    arbor_span input,
    arbor_view0_native_source_anchor *anchors,
    uint64_t anchor_capacity,
    arbor_view0_native_g03_r7a_evaluation *evaluation_out)
{
    return evaluate(input, NULL, anchors, anchor_capacity, 0u,
                    false, true, evaluation_out);
}
