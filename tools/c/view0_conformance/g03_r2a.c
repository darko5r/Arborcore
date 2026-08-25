#include "g03_r2a.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define G03_R2A_MAX_DEPTH UINT64_C(4097)
#define G03_R2A_FRAME_COUNT UINT64_C(4098)

#define G03_R2_CAT_FLOW UINT64_C(0x01)
#define G03_R2_CAT_PHRASING UINT64_C(0x02)
#define G03_R2_CAT_METADATA UINT64_C(0x04)
#define G03_R2_CAT_HEADING UINT64_C(0x08)
#define G03_R2_CAT_SCRIPT_SUPPORTING UINT64_C(0x40)

#define G03_R2_ATTR_SPAN UINT64_C(1) << 0
#define G03_R2_ATTR_DATETIME UINT64_C(1) << 1
#define G03_R2_ATTR_LABEL UINT64_C(1) << 2
#define G03_R2_ATTR_VALUE UINT64_C(1) << 3
#define G03_R2_ATTR_SIZE UINT64_C(1) << 4
#define G03_R2_ATTR_MULTIPLE UINT64_C(1) << 5

#define G03_R2_RUBY_NEED_BASE UINT64_C(0)
#define G03_R2_RUBY_BASE UINT64_C(1)
#define G03_R2_RUBY_SIMPLE_RT UINT64_C(2)
#define G03_R2_RUBY_RP_NEED_RT UINT64_C(3)
#define G03_R2_RUBY_RP_NEED_RP UINT64_C(4)
#define G03_R2_RUBY_RP_COMPLETE UINT64_C(5)

_Static_assert(ARBOR_VIEW0_NATIVE_STANDARD_ELEMENT_COUNT == UINT64_C(113),
               "G03 R2A standard-element inventory drift");
_Static_assert(G03_R2A_FRAME_COUNT == G03_R2A_MAX_DEPTH + UINT64_C(1),
               "G03 R2A depth/workspace relationship drift");
_Static_assert(sizeof("ARBOR_VIEW_V1_G03_CONTENT_MODEL") <=
                   ARBOR_VIEW0_NATIVE_RULE_SYMBOL_CAP,
               "G03 R2A symbolic name exceeds diagnostic capacity");
_Static_assert(sizeof("HTML element contents do not match its content model") <=
                   ARBOR_VIEW0_NATIVE_MESSAGE_CAP,
               "G03 R2A message exceeds diagnostic capacity");

/* Same frozen 113-token category facts consumed by R1A; private duplication
 * deliberately avoids changing the accepted R1A implementation surface. */
static const uint64_t g03_r2_element_categories[114] = {
    [1] = UINT64_C(0x00), [2] = UINT64_C(0x00), [3] = UINT64_C(0x04),
    [4] = UINT64_C(0x04), [5] = UINT64_C(0x07), [6] = UINT64_C(0x07),
    [7] = UINT64_C(0x04), [8] = UINT64_C(0x00), [9] = UINT64_C(0x11),
    [10] = UINT64_C(0x11), [11] = UINT64_C(0x11), [12] = UINT64_C(0x11),
    [13] = UINT64_C(0x09), [14] = UINT64_C(0x09), [15] = UINT64_C(0x09),
    [16] = UINT64_C(0x09), [17] = UINT64_C(0x09), [18] = UINT64_C(0x09),
    [19] = UINT64_C(0x09), [20] = UINT64_C(0x01), [21] = UINT64_C(0x01),
    [22] = UINT64_C(0x01), [23] = UINT64_C(0x01), [24] = UINT64_C(0x01),
    [25] = UINT64_C(0x01), [26] = UINT64_C(0x01), [27] = UINT64_C(0x01),
    [28] = UINT64_C(0x01), [29] = UINT64_C(0x01), [30] = UINT64_C(0x00),
    [31] = UINT64_C(0x01), [32] = UINT64_C(0x00), [33] = UINT64_C(0x00),
    [34] = UINT64_C(0x01), [35] = UINT64_C(0x00), [36] = UINT64_C(0x01),
    [37] = UINT64_C(0x01), [38] = UINT64_C(0x01), [39] = UINT64_C(0x03),
    [40] = UINT64_C(0x03), [41] = UINT64_C(0x03), [42] = UINT64_C(0x03),
    [43] = UINT64_C(0x03), [44] = UINT64_C(0x03), [45] = UINT64_C(0x03),
    [46] = UINT64_C(0x03), [47] = UINT64_C(0x03), [48] = UINT64_C(0x03),
    [49] = UINT64_C(0x00), [50] = UINT64_C(0x00), [51] = UINT64_C(0x03),
    [52] = UINT64_C(0x03), [53] = UINT64_C(0x03), [54] = UINT64_C(0x03),
    [55] = UINT64_C(0x03), [56] = UINT64_C(0x03), [57] = UINT64_C(0x03),
    [58] = UINT64_C(0x03), [59] = UINT64_C(0x03), [60] = UINT64_C(0x03),
    [61] = UINT64_C(0x03), [62] = UINT64_C(0x03), [63] = UINT64_C(0x03),
    [64] = UINT64_C(0x03), [65] = UINT64_C(0x03), [66] = UINT64_C(0x03),
    [67] = UINT64_C(0x03), [68] = UINT64_C(0x03), [69] = UINT64_C(0x03),
    [70] = UINT64_C(0x23), [71] = UINT64_C(0x00), [72] = UINT64_C(0x23),
    [73] = UINT64_C(0x23), [74] = UINT64_C(0x23), [75] = UINT64_C(0x23),
    [76] = UINT64_C(0x23), [77] = UINT64_C(0x23), [78] = UINT64_C(0x00),
    [79] = UINT64_C(0x03), [80] = UINT64_C(0x03), [81] = UINT64_C(0x01),
    [82] = UINT64_C(0x00), [83] = UINT64_C(0x00), [84] = UINT64_C(0x00),
    [85] = UINT64_C(0x00), [86] = UINT64_C(0x00), [87] = UINT64_C(0x00),
    [88] = UINT64_C(0x00), [89] = UINT64_C(0x00), [90] = UINT64_C(0x00),
    [91] = UINT64_C(0x01), [92] = UINT64_C(0x03), [93] = UINT64_C(0x03),
    [94] = UINT64_C(0x03), [95] = UINT64_C(0x03), [96] = UINT64_C(0x03),
    [97] = UINT64_C(0x00), [98] = UINT64_C(0x00), [99] = UINT64_C(0x03),
    [100] = UINT64_C(0x03), [101] = UINT64_C(0x03), [102] = UINT64_C(0x03),
    [103] = UINT64_C(0x01), [104] = UINT64_C(0x00), [105] = UINT64_C(0x02),
    [106] = UINT64_C(0x01), [107] = UINT64_C(0x00), [108] = UINT64_C(0x01),
    [109] = UINT64_C(0x47), [110] = UINT64_C(0x07), [111] = UINT64_C(0x47),
    [112] = UINT64_C(0x03), [113] = UINT64_C(0x23)
};

typedef struct g03_r2a_frame {
    uint64_t standard_element_id;
    uint64_t namespace_id;
    uint64_t parent_standard_element_id;
    uint64_t source_offset;
    uint64_t source_length;
    uint64_t attribute_flags;
    uint64_t meaningful_count;
    uint64_t figcaption_count;
    uint64_t first_figcaption_meaningful_index;
    uint64_t last_figcaption_meaningful_index;
    uint64_t summary_count;
    uint64_t heading_count;
    uint64_t img_count;
    uint64_t selectedcontent_descendant_count;
    uint64_t ruby_state;
    bool authored;
    bool violation;
    bool nonwhitespace_text;
    bool datalist_phrasing_branch;
    bool datalist_option_branch;
    bool dl_dtdd_branch;
    bool dl_div_branch;
    bool dl_dt_seen;
    bool dl_dd_seen;
    bool select_first_meaningful_button;
    bool select_button_admitted;
    bool select_transparent_context;
    bool datalist_ancestor;
    bool ruby_r2_violation;
    bool ruby_r1_owned_rp_error;
    bool ruby_pending_rp;
    bool ruby_pending_rp_rt_before;
    bool option_div_branch;
    bool option_phrasing_branch;
    uint64_t previous_direct_kind;
    uint64_t previous_direct_element_id;
} g03_r2a_frame;

typedef struct g03_r2a_context {
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
    g03_r2a_frame frames[4098];
} g03_r2a_context;

_Static_assert(sizeof(g03_r2a_frame) == 160u,
               "G03 R2A frame layout drift on x86-64");
_Static_assert(sizeof(g03_r2a_context) == 655736u,
               "G03 R2A bounded evaluator workspace layout drift on x86-64");
_Static_assert(sizeof(g03_r2a_context) <= 1048576u,
               "G03 R2A evaluator workspace exceeds 1 MiB admission");

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
    if ((uint64_t)length != span.length || (span.length != 0u && span.data == NULL)) {
        return false;
    }
    for (uint64_t i = 0u; i < span.length; ++i) {
        if (ascii_lower(span.data[i]) != ascii_lower((uint8_t)literal[i])) {
            return false;
        }
    }
    return true;
}

static bool frame_has_ancestor_id(
    const g03_r2a_context *context,
    uint64_t parent_depth,
    uint64_t id)
{
    for (uint64_t depth = 0u; depth <= parent_depth; ++depth) {
        if (context->frames[depth].standard_element_id == id) {
            return true;
        }
    }
    return false;
}

static bool is_script_supporting(uint64_t id)
{
    return id == ARBOR_VIEW0_NATIVE_ELEMENT_SCRIPT ||
        id == ARBOR_VIEW0_NATIVE_ELEMENT_TEMPLATE;
}

static bool is_heading(uint64_t id)
{
    return id >= ARBOR_VIEW0_NATIVE_ELEMENT_H1 &&
        id <= ARBOR_VIEW0_NATIVE_ELEMENT_H6;
}

static bool known_category(uint64_t id, uint64_t category)
{
    return id != 0u && id <= ARBOR_VIEW0_NATIVE_STANDARD_ELEMENT_COUNT &&
        (g03_r2_element_categories[id] & category) != 0u;
}

static bool child_is_unknown(
    const arbor_view0_native_direct_child_observation *child)
{
    return child->kind == ARBOR_VIEW0_NATIVE_DIRECT_CHILD_ELEMENT &&
        (child->namespace_id != ARBOR_VIEW0_NATIVE_NAMESPACE_HTML ||
         child->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_NONE ||
         child->standard_element_id > ARBOR_VIEW0_NATIVE_STANDARD_ELEMENT_COUNT);
}

static bool parent_is_select_transparent_div(const g03_r2a_frame *frame)
{
    return frame->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_DIV &&
        frame->select_transparent_context;
}

static void fill_diagnostic(
    arbor_view0_native_diagnostic *diagnostic,
    uint64_t source_offset,
    uint64_t source_length,
    uint64_t discovery_sequence)
{
    static const char symbolic[] = "ARBOR_VIEW_V1_G03_CONTENT_MODEL";
    static const char message[] =
        "HTML element contents do not match its content model";
    (void)memset(diagnostic, 0, sizeof(*diagnostic));
    diagnostic->rule_id = ARBOR_VIEW_V1_G03_CONTENT_MODEL;
    diagnostic->byte_offset = source_offset;
    diagnostic->source_length = source_length;
    diagnostic->discovery_sequence = discovery_sequence;
    diagnostic->severity = (uint32_t)ARBOR_VIEW0_NATIVE_SEVERITY_ERROR;
    diagnostic->origin = (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_AUTHORING;
    (void)memcpy(diagnostic->symbolic_name, symbolic, sizeof(symbolic));
    (void)memcpy(diagnostic->message, message, sizeof(message));
}

void arbor_view0_native_g03_r2a_materialize_anchor(
    const arbor_view0_native_source_anchor *anchor,
    uint64_t discovery_sequence,
    arbor_view0_native_diagnostic *diagnostic)
{
    fill_diagnostic(
        diagnostic, (uint64_t)anchor->byte_offset,
        (uint64_t)anchor->source_length, discovery_sequence);
}

static arbor_status report_invalid(g03_r2a_context *context, const g03_r2a_frame *frame)
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

static void set_deferred(g03_r2a_context *context, uint64_t flag)
{
    context->deferred_flags |= flag;
}

static arbor_status r2a_traversal_enter(
    void *context_void,
    const arbor_view0_native_element_observation *observation)
{
    g03_r2a_context *context = context_void;
    if (context == NULL || observation == NULL || observation->depth > G03_R2A_MAX_DEPTH) {
        return status_from_errno_value(observation != NULL ? E2BIG : EINVAL);
    }
    context->current_depth = observation->depth;
    g03_r2a_frame *frame = &context->frames[observation->depth];
    (void)memset(frame, 0, sizeof(*frame));
    frame->standard_element_id = observation->standard_element_id;
    frame->namespace_id = observation->namespace_id;
    frame->parent_standard_element_id = observation->parent_standard_element_id;
    frame->source_offset = observation->source_offset;
    frame->source_length = observation->source_length;
    frame->authored = (observation->flags & ARBOR_VIEW0_NATIVE_ELEMENT_FLAG_SYNTHETIC) == 0u;
    frame->first_figcaption_meaningful_index = ARBOR_VIEW0_NATIVE_OBSERVATION_INDEX_NONE;
    frame->last_figcaption_meaningful_index = ARBOR_VIEW0_NATIVE_OBSERVATION_INDEX_NONE;
    frame->ruby_state = G03_R2_RUBY_NEED_BASE;
    if (observation->depth != 0u) {
        const uint64_t parent_depth = observation->depth - 1u;
        frame->select_transparent_context =
            frame_has_ancestor_id(context, parent_depth, ARBOR_VIEW0_NATIVE_ELEMENT_SELECT) ||
            frame_has_ancestor_id(context, parent_depth, ARBOR_VIEW0_NATIVE_ELEMENT_OPTGROUP) ||
            frame_has_ancestor_id(context, parent_depth, ARBOR_VIEW0_NATIVE_ELEMENT_OPTION);
        frame->datalist_ancestor =
            frame_has_ancestor_id(context, parent_depth, ARBOR_VIEW0_NATIVE_ELEMENT_DATALIST);
        const g03_r2a_frame *parent = &context->frames[parent_depth];
        if (frame->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_BUTTON &&
            parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_SELECT &&
            parent->select_first_meaningful_button) {
            frame->select_button_admitted = true;
        }
        if (frame->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_SELECTEDCONTENT) {
            for (uint64_t depth = parent_depth + 1u; depth != 0u; --depth) {
                g03_r2a_frame *ancestor = &context->frames[depth - 1u];
                if (ancestor->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_BUTTON &&
                    ancestor->select_button_admitted) {
                    if (ancestor->selectedcontent_descendant_count == UINT64_MAX) {
                        return status_from_errno_value(EOVERFLOW);
                    }
                    ancestor->selectedcontent_descendant_count += 1u;
                    break;
                }
            }
        }
    }
    if (frame->authored) {
        if (frame->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_STYLE) {
            set_deferred(context, ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_DEFERRED_STYLE);
        } else if (frame->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_SCRIPT) {
            set_deferred(context, ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_DEFERRED_SCRIPT);
        } else if (frame->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_NOSCRIPT) {
            set_deferred(context, ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_DEFERRED_NOSCRIPT);
        }
    }
    return ok_status();
}

static arbor_status r2a_attribute(
    void *context_void,
    const arbor_view0_native_attribute_observation *observation)
{
    g03_r2a_context *context = context_void;
    if (context == NULL || observation == NULL || context->current_depth > G03_R2A_MAX_DEPTH) {
        return status_from_errno_value(EINVAL);
    }
    g03_r2a_frame *frame = &context->frames[context->current_depth];
    if (span_ascii_ci_equals(observation->local_name, "span")) {
        frame->attribute_flags |= G03_R2_ATTR_SPAN;
    } else if (span_ascii_ci_equals(observation->local_name, "datetime")) {
        frame->attribute_flags |= G03_R2_ATTR_DATETIME;
    } else if (span_ascii_ci_equals(observation->local_name, "label")) {
        frame->attribute_flags |= G03_R2_ATTR_LABEL;
    } else if (span_ascii_ci_equals(observation->local_name, "value")) {
        frame->attribute_flags |= G03_R2_ATTR_VALUE;
    } else if (span_ascii_ci_equals(observation->local_name, "size")) {
        frame->attribute_flags |= G03_R2_ATTR_SIZE;
        if (frame->authored && frame->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_SELECT) {
            set_deferred(context, ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_DEFERRED_SELECT_SIZE);
        }
    } else if (span_ascii_ci_equals(observation->local_name, "multiple")) {
        frame->attribute_flags |= G03_R2_ATTR_MULTIPLE;
    }
    return ok_status();
}

static void ruby_finalize_pending_rp(
    g03_r2a_frame *frame,
    const arbor_view0_native_direct_child_observation *next)
{
    if (!frame->ruby_pending_rp) {
        return;
    }
    const bool next_rt = next != NULL &&
        next->kind == ARBOR_VIEW0_NATIVE_DIRECT_CHILD_ELEMENT &&
        next->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_RT;
    if (!frame->ruby_pending_rp_rt_before && !next_rt) {
        frame->ruby_r1_owned_rp_error = true;
    }
    frame->ruby_pending_rp = false;
    frame->ruby_pending_rp_rt_before = false;
}

static bool ruby_base_token(const arbor_view0_native_direct_child_observation *child)
{
    if (child->kind == ARBOR_VIEW0_NATIVE_DIRECT_CHILD_TEXT) {
        return true;
    }
    if (child_is_unknown(child)) {
        return false;
    }
    const uint64_t id = child->standard_element_id;
    if (id == ARBOR_VIEW0_NATIVE_ELEMENT_RT || id == ARBOR_VIEW0_NATIVE_ELEMENT_RP) {
        return false;
    }
    return id == ARBOR_VIEW0_NATIVE_ELEMENT_RUBY || known_category(id, G03_R2_CAT_PHRASING);
}

static void ruby_consume(
    g03_r2a_context *context,
    g03_r2a_frame *frame,
    const arbor_view0_native_direct_child_observation *child)
{
    ruby_finalize_pending_rp(frame, child);
    if (child_is_unknown(child)) {
        set_deferred(context, ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_DEFERRED_UNCLASSIFIED);
        return;
    }
    if (ruby_base_token(child)) {
        if (frame->ruby_state == G03_R2_RUBY_NEED_BASE ||
            frame->ruby_state == G03_R2_RUBY_BASE) {
            frame->ruby_state = G03_R2_RUBY_BASE;
        } else if (frame->ruby_state == G03_R2_RUBY_SIMPLE_RT ||
                   frame->ruby_state == G03_R2_RUBY_RP_COMPLETE) {
            frame->ruby_state = G03_R2_RUBY_BASE;
        } else if (frame->ruby_state == G03_R2_RUBY_RP_NEED_RP) {
            frame->ruby_r2_violation = true;
        } else {
            /* An opening rp not followed by rt is already R1-owned. */
            frame->ruby_r1_owned_rp_error = true;
        }
        frame->previous_direct_kind = child->kind;
        frame->previous_direct_element_id = child->standard_element_id;
        return;
    }

    if (child->kind != ARBOR_VIEW0_NATIVE_DIRECT_CHILD_ELEMENT) {
        return;
    }
    const uint64_t id = child->standard_element_id;
    if (id == ARBOR_VIEW0_NATIVE_ELEMENT_RT) {
        switch (frame->ruby_state) {
        case G03_R2_RUBY_BASE:
            frame->ruby_state = G03_R2_RUBY_SIMPLE_RT;
            break;
        case G03_R2_RUBY_SIMPLE_RT:
            break;
        case G03_R2_RUBY_RP_NEED_RT:
            frame->ruby_state = G03_R2_RUBY_RP_NEED_RP;
            break;
        case G03_R2_RUBY_RP_COMPLETE:
            frame->ruby_state = G03_R2_RUBY_RP_NEED_RP;
            break;
        case G03_R2_RUBY_NEED_BASE:
        case G03_R2_RUBY_RP_NEED_RP:
            frame->ruby_r2_violation = true;
            break;
        default:
            frame->ruby_r2_violation = true;
            break;
        }
    } else if (id == ARBOR_VIEW0_NATIVE_ELEMENT_RP) {
        frame->ruby_pending_rp = true;
        frame->ruby_pending_rp_rt_before =
            frame->previous_direct_kind == ARBOR_VIEW0_NATIVE_DIRECT_CHILD_ELEMENT &&
            frame->previous_direct_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_RT;
        switch (frame->ruby_state) {
        case G03_R2_RUBY_BASE:
            frame->ruby_state = G03_R2_RUBY_RP_NEED_RT;
            break;
        case G03_R2_RUBY_RP_NEED_RP:
            frame->ruby_state = G03_R2_RUBY_RP_COMPLETE;
            break;
        case G03_R2_RUBY_NEED_BASE:
            frame->ruby_r2_violation = true;
            frame->ruby_state = G03_R2_RUBY_RP_NEED_RT;
            break;
        case G03_R2_RUBY_SIMPLE_RT:
            frame->ruby_r2_violation = true;
            break;
        default:
            break;
        }
    }
    frame->previous_direct_kind = child->kind;
    frame->previous_direct_element_id = id;
}

static uint64_t r2_parent_expected_category_mask(const g03_r2a_frame *frame)
{
    if (frame == NULL) {
        return 0u;
    }
    switch (frame->standard_element_id) {
    case ARBOR_VIEW0_NATIVE_ELEMENT_HEAD:
        return G03_R2_CAT_METADATA;
    case ARBOR_VIEW0_NATIVE_ELEMENT_BODY:
    case ARBOR_VIEW0_NATIVE_ELEMENT_ARTICLE:
    case ARBOR_VIEW0_NATIVE_ELEMENT_SECTION:
    case ARBOR_VIEW0_NATIVE_ELEMENT_NAV:
    case ARBOR_VIEW0_NATIVE_ELEMENT_ASIDE:
    case ARBOR_VIEW0_NATIVE_ELEMENT_HEADER:
    case ARBOR_VIEW0_NATIVE_ELEMENT_FOOTER:
    case ARBOR_VIEW0_NATIVE_ELEMENT_ADDRESS:
    case ARBOR_VIEW0_NATIVE_ELEMENT_BLOCKQUOTE:
    case ARBOR_VIEW0_NATIVE_ELEMENT_LI:
    case ARBOR_VIEW0_NATIVE_ELEMENT_DT:
    case ARBOR_VIEW0_NATIVE_ELEMENT_DD:
    case ARBOR_VIEW0_NATIVE_ELEMENT_FIGURE:
    case ARBOR_VIEW0_NATIVE_ELEMENT_FIGCAPTION:
    case ARBOR_VIEW0_NATIVE_ELEMENT_MAIN:
    case ARBOR_VIEW0_NATIVE_ELEMENT_SEARCH:
    case ARBOR_VIEW0_NATIVE_ELEMENT_CAPTION:
    case ARBOR_VIEW0_NATIVE_ELEMENT_TD:
    case ARBOR_VIEW0_NATIVE_ELEMENT_TH:
    case ARBOR_VIEW0_NATIVE_ELEMENT_FORM:
    case ARBOR_VIEW0_NATIVE_ELEMENT_FIELDSET:
    case ARBOR_VIEW0_NATIVE_ELEMENT_DETAILS:
    case ARBOR_VIEW0_NATIVE_ELEMENT_DIALOG:
        return G03_R2_CAT_FLOW;
    case ARBOR_VIEW0_NATIVE_ELEMENT_H1:
    case ARBOR_VIEW0_NATIVE_ELEMENT_H2:
    case ARBOR_VIEW0_NATIVE_ELEMENT_H3:
    case ARBOR_VIEW0_NATIVE_ELEMENT_H4:
    case ARBOR_VIEW0_NATIVE_ELEMENT_H5:
    case ARBOR_VIEW0_NATIVE_ELEMENT_H6:
    case ARBOR_VIEW0_NATIVE_ELEMENT_P:
    case ARBOR_VIEW0_NATIVE_ELEMENT_PRE:
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
    case ARBOR_VIEW0_NATIVE_ELEMENT_LABEL:
    case ARBOR_VIEW0_NATIVE_ELEMENT_BUTTON:
    case ARBOR_VIEW0_NATIVE_ELEMENT_OUTPUT:
    case ARBOR_VIEW0_NATIVE_ELEMENT_PROGRESS:
    case ARBOR_VIEW0_NATIVE_ELEMENT_METER:
        return G03_R2_CAT_PHRASING;
    case ARBOR_VIEW0_NATIVE_ELEMENT_TIME:
        return (frame->attribute_flags & G03_R2_ATTR_DATETIME) != 0u
            ? G03_R2_CAT_PHRASING
            : 0u;
    case ARBOR_VIEW0_NATIVE_ELEMENT_DATALIST:
        return G03_R2_CAT_PHRASING | G03_R2_CAT_SCRIPT_SUPPORTING;
    case ARBOR_VIEW0_NATIVE_ELEMENT_LEGEND:
    case ARBOR_VIEW0_NATIVE_ELEMENT_SUMMARY:
        return G03_R2_CAT_PHRASING | G03_R2_CAT_HEADING;
    default:
        return 0u;
    }
}

static bool div_relation_is_residual(const g03_r2a_frame *parent)
{
    if (parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_SELECT ||
        parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_OPTGROUP ||
        parent_is_select_transparent_div(parent)) {
        return false;
    }
    if (parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_OPTION) {
        /* The option branch is evaluated with its own attribute-sensitive state. */
        return false;
    }
    const uint64_t expected = r2_parent_expected_category_mask(parent);
    return expected == 0u || (expected & G03_R2_CAT_FLOW) == 0u;
}

static bool direct_relation_residual(
    const g03_r2a_context *context,
    const g03_r2a_frame *parent,
    const arbor_view0_native_direct_child_observation *child)
{
    if (child->kind != ARBOR_VIEW0_NATIVE_DIRECT_CHILD_ELEMENT ||
        child->namespace_id != ARBOR_VIEW0_NATIVE_NAMESPACE_HTML ||
        child->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_NONE) {
        return false;
    }
    const uint64_t id = child->standard_element_id;
    if (id == ARBOR_VIEW0_NATIVE_ELEMENT_LI) {
        const bool r1_admits = frame_has_ancestor_id(
            context, context->current_depth, ARBOR_VIEW0_NATIVE_ELEMENT_OL) ||
            frame_has_ancestor_id(context, context->current_depth, ARBOR_VIEW0_NATIVE_ELEMENT_UL) ||
            frame_has_ancestor_id(context, context->current_depth, ARBOR_VIEW0_NATIVE_ELEMENT_MENU);
        return r1_admits && parent->standard_element_id != ARBOR_VIEW0_NATIVE_ELEMENT_OL &&
            parent->standard_element_id != ARBOR_VIEW0_NATIVE_ELEMENT_UL &&
            parent->standard_element_id != ARBOR_VIEW0_NATIVE_ELEMENT_MENU;
    }
    if (id == ARBOR_VIEW0_NATIVE_ELEMENT_OPTGROUP) {
        const bool r1_admits = frame_has_ancestor_id(
            context, context->current_depth, ARBOR_VIEW0_NATIVE_ELEMENT_SELECT);
        return r1_admits && parent->standard_element_id != ARBOR_VIEW0_NATIVE_ELEMENT_SELECT &&
            !parent_is_select_transparent_div(parent);
    }
    if (id == ARBOR_VIEW0_NATIVE_ELEMENT_OPTION) {
        const bool r1_admits = frame_has_ancestor_id(
            context, context->current_depth, ARBOR_VIEW0_NATIVE_ELEMENT_DATALIST) ||
            frame_has_ancestor_id(context, context->current_depth, ARBOR_VIEW0_NATIVE_ELEMENT_SELECT) ||
            frame_has_ancestor_id(context, context->current_depth, ARBOR_VIEW0_NATIVE_ELEMENT_OPTGROUP);
        const bool direct_ok = parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_DATALIST ||
            parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_SELECT ||
            parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_OPTGROUP;
        return r1_admits && !direct_ok && !parent_is_select_transparent_div(parent);
    }
    if (id == ARBOR_VIEW0_NATIVE_ELEMENT_DIV) {
        const bool r1_admits = frame_has_ancestor_id(
            context, context->current_depth, ARBOR_VIEW0_NATIVE_ELEMENT_SELECT) ||
            frame_has_ancestor_id(context, context->current_depth, ARBOR_VIEW0_NATIVE_ELEMENT_OPTGROUP) ||
            frame_has_ancestor_id(context, context->current_depth, ARBOR_VIEW0_NATIVE_ELEMENT_OPTION);
        return r1_admits && div_relation_is_residual(parent);
    }
    if (id == ARBOR_VIEW0_NATIVE_ELEMENT_HR) {
        const bool r1_admits = frame_has_ancestor_id(
            context, context->current_depth, ARBOR_VIEW0_NATIVE_ELEMENT_SELECT);
        if (!r1_admits || parent_is_select_transparent_div(parent)) {
            return false;
        }
        if (parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_SELECT) {
            return false;
        }
        if (known_category(ARBOR_VIEW0_NATIVE_ELEMENT_HR, G03_R2_CAT_FLOW) &&
            known_category(parent->standard_element_id, G03_R2_CAT_FLOW)) {
            return false;
        }
        return parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_OPTION ||
            parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_OPTGROUP ||
            known_category(parent->standard_element_id, G03_R2_CAT_PHRASING);
    }
    return false;
}

static arbor_status r2a_direct_child(
    void *context_void,
    const arbor_view0_native_direct_child_observation *child)
{
    g03_r2a_context *context = context_void;
    if (context == NULL || child == NULL || context->current_depth > G03_R2A_MAX_DEPTH) {
        return status_from_errno_value(EINVAL);
    }
    g03_r2a_frame *parent = &context->frames[context->current_depth];
    const bool nonws_text = child->kind == ARBOR_VIEW0_NATIVE_DIRECT_CHILD_TEXT &&
        (child->flags & ARBOR_VIEW0_NATIVE_CHILD_FLAG_TEXT_INTER_ELEMENT_WHITESPACE) == 0u;
    const bool unknown = child_is_unknown(child);
    const bool meaningful = child->kind == ARBOR_VIEW0_NATIVE_DIRECT_CHILD_ELEMENT || nonws_text;
    const uint64_t meaningful_index = parent->meaningful_count;

    if (unknown && parent->authored) {
        set_deferred(context, ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_DEFERRED_UNCLASSIFIED);
    }
    if (nonws_text) {
        parent->nonwhitespace_text = true;
    }

    if (parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_RUBY) {
        ruby_consume(context, parent, child);
    }

    if (child->kind == ARBOR_VIEW0_NATIVE_DIRECT_CHILD_ELEMENT && !unknown) {
        const uint64_t id = child->standard_element_id;
        if (parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_DL) {
            if (id == ARBOR_VIEW0_NATIVE_ELEMENT_DIV) {
                parent->dl_div_branch = true;
            } else if (id == ARBOR_VIEW0_NATIVE_ELEMENT_DT || id == ARBOR_VIEW0_NATIVE_ELEMENT_DD) {
                parent->dl_dtdd_branch = true;
            }
        }
        if (parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_DIV &&
            parent->parent_standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_DL) {
            if (id == ARBOR_VIEW0_NATIVE_ELEMENT_DT) {
                parent->dl_dt_seen = true;
            } else if (id == ARBOR_VIEW0_NATIVE_ELEMENT_DD) {
                parent->dl_dd_seen = true;
            }
        }
        if (parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_FIGURE &&
            id == ARBOR_VIEW0_NATIVE_ELEMENT_FIGCAPTION) {
            parent->figcaption_count += 1u;
            if (parent->first_figcaption_meaningful_index == ARBOR_VIEW0_NATIVE_OBSERVATION_INDEX_NONE) {
                parent->first_figcaption_meaningful_index = meaningful_index;
            }
            parent->last_figcaption_meaningful_index = meaningful_index;
        }
        if (parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_DATALIST) {
            if (id == ARBOR_VIEW0_NATIVE_ELEMENT_OPTION) {
                parent->datalist_option_branch = true;
            } else if (!is_script_supporting(id) && known_category(id, G03_R2_CAT_PHRASING)) {
                parent->datalist_phrasing_branch = true;
            }
        }
        if (parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_DETAILS &&
            id == ARBOR_VIEW0_NATIVE_ELEMENT_SUMMARY) {
            parent->summary_count += 1u;
        }
        if (parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_HGROUP && is_heading(id)) {
            parent->heading_count += 1u;
        }
        if (parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_PICTURE &&
            id == ARBOR_VIEW0_NATIVE_ELEMENT_IMG) {
            parent->img_count += 1u;
        }
        if (parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_SELECT &&
            !parent->select_first_meaningful_button && parent->meaningful_count == 0u &&
            id == ARBOR_VIEW0_NATIVE_ELEMENT_BUTTON) {
            parent->select_first_meaningful_button = true;
        }
        if (parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_OPTGROUP &&
            (id == ARBOR_VIEW0_NATIVE_ELEMENT_HR ||
             id == ARBOR_VIEW0_NATIVE_ELEMENT_OPTGROUP)) {
            parent->violation = true;
        }
        if (parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_OPTION) {
            if ((parent->attribute_flags & G03_R2_ATTR_LABEL) == 0u && !parent->datalist_ancestor) {
                if (id == ARBOR_VIEW0_NATIVE_ELEMENT_DIV) {
                    parent->option_div_branch = true;
                } else if (known_category(id, G03_R2_CAT_PHRASING)) {
                    parent->option_phrasing_branch = true;
                }
                if (parent->option_div_branch && parent->option_phrasing_branch) {
                    parent->violation = true;
                }
            }
            const bool text_only =
                ((parent->attribute_flags & G03_R2_ATTR_LABEL) != 0u &&
                 (parent->attribute_flags & G03_R2_ATTR_VALUE) == 0u) ||
                ((parent->attribute_flags & G03_R2_ATTR_LABEL) == 0u &&
                 parent->datalist_ancestor);
            const bool nothing =
                (parent->attribute_flags & (G03_R2_ATTR_LABEL | G03_R2_ATTR_VALUE)) ==
                    (G03_R2_ATTR_LABEL | G03_R2_ATTR_VALUE);
            if (!nothing) {
                if (text_only) {
                    if (id == ARBOR_VIEW0_NATIVE_ELEMENT_DIV ||
                        id == ARBOR_VIEW0_NATIVE_ELEMENT_HR ||
                        id == ARBOR_VIEW0_NATIVE_ELEMENT_OPTION ||
                        id == ARBOR_VIEW0_NATIVE_ELEMENT_OPTGROUP) {
                        parent->violation = true;
                    }
                } else if (id != ARBOR_VIEW0_NATIVE_ELEMENT_DIV &&
                           !known_category(id, G03_R2_CAT_PHRASING) &&
                           (id == ARBOR_VIEW0_NATIVE_ELEMENT_HR ||
                            id == ARBOR_VIEW0_NATIVE_ELEMENT_OPTION ||
                            id == ARBOR_VIEW0_NATIVE_ELEMENT_OPTGROUP)) {
                    parent->violation = true;
                }
            }
        }
        if (direct_relation_residual(context, parent, child)) {
            parent->violation = true;
        }
    }

    if (parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_DATALIST && nonws_text) {
        parent->datalist_phrasing_branch = true;
    }
    if (parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_OPTION && nonws_text &&
        (parent->attribute_flags & G03_R2_ATTR_LABEL) == 0u && !parent->datalist_ancestor) {
        parent->option_phrasing_branch = true;
    }

    if (meaningful) {
        if (parent->meaningful_count == UINT64_MAX) {
            return status_from_errno_value(EOVERFLOW);
        }
        parent->meaningful_count += 1u;
    }
    return ok_status();
}

static void finalize_ruby(g03_r2a_frame *frame)
{
    ruby_finalize_pending_rp(frame, NULL);
    if (frame->ruby_state == G03_R2_RUBY_NEED_BASE ||
        frame->ruby_state == G03_R2_RUBY_BASE ||
        frame->ruby_state == G03_R2_RUBY_RP_NEED_RP) {
        frame->ruby_r2_violation = true;
    } else if (frame->ruby_state == G03_R2_RUBY_RP_NEED_RT) {
        frame->ruby_r1_owned_rp_error = true;
    }
    if (frame->ruby_r2_violation && !frame->ruby_r1_owned_rp_error) {
        frame->violation = true;
    }
}

static void finalize_parent(g03_r2a_context *context, g03_r2a_frame *frame)
{
    if (!frame->authored || frame->namespace_id != ARBOR_VIEW0_NATIVE_NAMESPACE_HTML) {
        return;
    }
    switch (frame->standard_element_id) {
    case ARBOR_VIEW0_NATIVE_ELEMENT_DL:
        if (frame->nonwhitespace_text || (frame->dl_dtdd_branch && frame->dl_div_branch)) {
            frame->violation = true;
        }
        break;
    case ARBOR_VIEW0_NATIVE_ELEMENT_DIV:
        if (frame->parent_standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_DL) {
            if (frame->nonwhitespace_text || (!frame->dl_dt_seen && !frame->dl_dd_seen)) {
                frame->violation = true;
            }
        }
        break;
    case ARBOR_VIEW0_NATIVE_ELEMENT_FIGURE:
        if (frame->figcaption_count == 2u &&
            frame->first_figcaption_meaningful_index == 0u &&
            frame->last_figcaption_meaningful_index + 1u == frame->meaningful_count) {
            frame->violation = true;
        }
        break;
    case ARBOR_VIEW0_NATIVE_ELEMENT_DATALIST:
        if (frame->datalist_phrasing_branch && frame->datalist_option_branch) {
            frame->violation = true;
        }
        break;
    case ARBOR_VIEW0_NATIVE_ELEMENT_COLGROUP:
        if ((frame->attribute_flags & G03_R2_ATTR_SPAN) == 0u && frame->nonwhitespace_text) {
            frame->violation = true;
        }
        break;
    case ARBOR_VIEW0_NATIVE_ELEMENT_TABLE:
        if (frame->nonwhitespace_text) {
            frame->violation = true;
        }
        break;
    case ARBOR_VIEW0_NATIVE_ELEMENT_DETAILS:
        if (frame->summary_count == 0u) {
            frame->violation = true;
        }
        break;
    case ARBOR_VIEW0_NATIVE_ELEMENT_BUTTON:
        if (frame->select_button_admitted && frame->selectedcontent_descendant_count > 1u) {
            frame->violation = true;
        }
        break;
    case ARBOR_VIEW0_NATIVE_ELEMENT_TITLE:
        if (!frame->nonwhitespace_text) {
            frame->violation = true;
        }
        break;
    case ARBOR_VIEW0_NATIVE_ELEMENT_OL:
    case ARBOR_VIEW0_NATIVE_ELEMENT_UL:
    case ARBOR_VIEW0_NATIVE_ELEMENT_MENU:
    case ARBOR_VIEW0_NATIVE_ELEMENT_TR:
    case ARBOR_VIEW0_NATIVE_ELEMENT_TBODY:
    case ARBOR_VIEW0_NATIVE_ELEMENT_THEAD:
    case ARBOR_VIEW0_NATIVE_ELEMENT_TFOOT:
    case ARBOR_VIEW0_NATIVE_ELEMENT_OPTGROUP:
        if (frame->nonwhitespace_text) {
            frame->violation = true;
        }
        break;
    case ARBOR_VIEW0_NATIVE_ELEMENT_HGROUP:
        if (frame->heading_count != 1u || frame->nonwhitespace_text) {
            frame->violation = true;
        }
        break;
    case ARBOR_VIEW0_NATIVE_ELEMENT_PICTURE:
        if (frame->img_count != 1u || frame->nonwhitespace_text) {
            frame->violation = true;
        }
        break;
    case ARBOR_VIEW0_NATIVE_ELEMENT_SELECT:
        if (frame->nonwhitespace_text) {
            frame->violation = true;
        }
        if ((frame->attribute_flags & G03_R2_ATTR_SIZE) == 0u) {
            if ((frame->attribute_flags & G03_R2_ATTR_MULTIPLE) != 0u &&
                frame->select_first_meaningful_button) {
                frame->violation = true;
            }
        } else if ((frame->attribute_flags & G03_R2_ATTR_MULTIPLE) != 0u) {
            set_deferred(context, ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_DEFERRED_SELECT_PLATFORM);
        }
        break;
    case ARBOR_VIEW0_NATIVE_ELEMENT_RUBY:
        finalize_ruby(frame);
        break;
    default:
        break;
    }
}

static arbor_status r2a_traversal_leave(
    void *context_void,
    const arbor_view0_native_element_observation *observation)
{
    g03_r2a_context *context = context_void;
    if (context == NULL || observation == NULL || observation->depth > G03_R2A_MAX_DEPTH) {
        return status_from_errno_value(EINVAL);
    }
    g03_r2a_frame *frame = &context->frames[observation->depth];
    finalize_parent(context, frame);
    arbor_status status = ok_status();
    if (frame->violation && frame->authored) {
        status = report_invalid(context, frame);
        if (status.native != 0) {
            return status;
        }
    }
    context->current_depth = observation->depth == 0u ? 0u : observation->depth - 1u;
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
    arbor_view0_native_g03_r2a_evaluation *evaluation_out)
{
    if (evaluation_out == NULL ||
        (publish && (collect_offsets || collect_anchors)) ||
        (collect_offsets && collect_anchors) ||
        (publish && diagnostic_capacity != 0u && diagnostics == NULL) ||
        (collect_offsets && diagnostic_capacity != 0u && source_offsets == NULL) ||
        (collect_anchors && diagnostic_capacity != 0u && anchors == NULL)) {
        return status_from_errno_value(EINVAL);
    }
    g03_r2a_context context = {
        .output = {.diagnostics = diagnostics},
        .diagnostic_capacity = diagnostic_capacity,
        .discovery_sequence_base = discovery_sequence_base,
        .diagnostic_count = 0u,
        .deferred_flags = 0u,
        .current_depth = 0u,
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
        .attribute = r2a_attribute,
        .direct_child = r2a_direct_child,
        .element_complete = NULL,
        .traversal_enter = r2a_traversal_enter,
        .traversal_leave = r2a_traversal_leave
    };
    arbor_view0_native_parse_counts parse_counts = {0};
    arbor_view0_native_document_facts facts = {0};
    arbor_view0_native_observation_counts observations = {0};
    arbor_status status = arbor_view0_native_lexbor_observe(
        input, &observer, &parse_counts, &facts, &observations);
    if (status.native != 0) {
        return status;
    }
    if (observations.max_depth > G03_R2A_MAX_DEPTH) {
        return status_from_errno_value(E2BIG);
    }
    if ((publish || collect_offsets || collect_anchors) &&
        context.diagnostic_count != diagnostic_capacity) {
        return status_from_errno_value(EIO);
    }
    const arbor_view0_native_g03_r2a_evaluation result = {
        .diagnostic_count = context.diagnostic_count,
        .deferred_flags = context.deferred_flags
    };
    *evaluation_out = result;
    return ok_status();
}

arbor_status arbor_view0_native_g03_r2a_measure(
    arbor_span input,
    arbor_view0_native_g03_r2a_evaluation *evaluation_out)
{
    return evaluate(input, NULL, NULL, NULL, 0u, 0u, false, false, false, evaluation_out);
}

arbor_status arbor_view0_native_g03_r2a_collect(
    arbor_span input,
    arbor_view0_native_diagnostic *diagnostics,
    uint64_t diagnostic_capacity,
    uint64_t discovery_sequence_base,
    arbor_view0_native_g03_r2a_evaluation *evaluation_out)
{
    return evaluate(
        input, diagnostics, NULL, NULL, diagnostic_capacity, discovery_sequence_base,
        true, false, false, evaluation_out);
}

arbor_status arbor_view0_native_g03_r2a_collect_offsets(
    arbor_span input,
    uint64_t *source_offsets,
    uint64_t offset_capacity,
    arbor_view0_native_g03_r2a_evaluation *evaluation_out)
{
    return evaluate(
        input, NULL, source_offsets, NULL, offset_capacity, 0u,
        false, true, false, evaluation_out);
}

arbor_status arbor_view0_native_g03_r2a_collect_anchors(
    arbor_span input,
    arbor_view0_native_source_anchor *anchors,
    uint64_t anchor_capacity,
    arbor_view0_native_g03_r2a_evaluation *evaluation_out)
{
    return evaluate(
        input, NULL, NULL, anchors, anchor_capacity, 0u,
        false, false, true, evaluation_out);
}
