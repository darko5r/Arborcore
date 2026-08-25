#include "g03_r1a.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define G03_R1A_MAX_DEPTH UINT64_C(4097)
#define G03_R1A_FRAME_COUNT UINT64_C(4098)

#define G03_CAT_FLOW UINT64_C(0x01)
#define G03_CAT_PHRASING UINT64_C(0x02)
#define G03_CAT_METADATA UINT64_C(0x04)
#define G03_CAT_HEADING UINT64_C(0x08)
#define G03_CAT_SECTIONING UINT64_C(0x10)
#define G03_CAT_EMBEDDED UINT64_C(0x20)
#define G03_CAT_SCRIPT_SUPPORTING UINT64_C(0x40)

#define G03_ATTR_SPAN UINT64_C(1) << 0
#define G03_ATTR_DATETIME UINT64_C(1) << 1
#define G03_ATTR_LABEL UINT64_C(1) << 2
#define G03_ATTR_VALUE UINT64_C(1) << 3
#define G03_ATTR_ARIA_LABEL UINT64_C(1) << 4
#define G03_ATTR_ARIA_LABELLEDBY UINT64_C(1) << 5
#define G03_ATTR_TITLE UINT64_C(1) << 6
#define G03_ATTR_ITEMPROP UINT64_C(1) << 7
#define G03_ATTR_REL UINT64_C(1) << 8
#define G03_ATTR_HTTP_EQUIV UINT64_C(1) << 9
#define G03_ATTR_CHARSET UINT64_C(1) << 10
#define G03_ATTR_NAME UINT64_C(1) << 11

#define G03_R1A_VERDICT_INVALID 0
#define G03_R1A_VERDICT_VALID 1
#define G03_R1A_VERDICT_DELAYED 2

#define G03_R1A_OUTPUT_MEASURE UINT8_C(0)
#define G03_R1A_OUTPUT_DIAGNOSTICS UINT8_C(1)
#define G03_R1A_OUTPUT_OFFSETS UINT8_C(2)
#define G03_R1A_OUTPUT_ANCHORS UINT8_C(3)

_Static_assert(ARBOR_VIEW0_NATIVE_STANDARD_ELEMENT_COUNT == UINT64_C(113),
               "G03 R1A standard-element inventory drift");
_Static_assert(G03_R1A_FRAME_COUNT == G03_R1A_MAX_DEPTH + UINT64_C(1),
               "G03 R1A depth/workspace relationship drift");
_Static_assert(sizeof("ARBOR_VIEW_V1_G03_ELEMENT_CONTEXT") <=
                   ARBOR_VIEW0_NATIVE_RULE_SYMBOL_CAP,
               "G03 R1A symbolic name exceeds diagnostic capacity");
_Static_assert(sizeof("HTML element is not permitted in this structural context") <=
                   ARBOR_VIEW0_NATIVE_MESSAGE_CAP,
               "G03 R1A message exceeds diagnostic capacity");

static const uint64_t g03_element_categories[114] = {
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

typedef struct g03_r1a_frame {
    uint64_t standard_element_id;
    uint64_t namespace_id;
    uint64_t parent_standard_element_id;
    uint64_t grandparent_standard_element_id;
    uint64_t inherited_context_mask;
    uint64_t attribute_flags;
    uint64_t source_offset;
    uint64_t source_length;
    bool authored;
    bool html_hyphen_name;
    bool rel_only_body_ok;
    bool http_equiv_content_type;
    bool select_transparent_context;
    bool datalist_ancestor;
    bool seen_dt_or_dd;
    bool seen_media_flow;
    bool seen_track;
    bool seen_picture_img;
    bool meaningful_child_seen;
    bool pending_figcaption;
    bool table_seen_thead;
    bool table_seen_tbody;
    bool table_seen_tfoot;
    bool table_seen_tr;
    bool pending_rp;
    bool pending_rp_has_rt_before;
    bool pending_dt;
    bool pending_tfoot;
    uint64_t pending_rp_source_offset;
    uint64_t pending_rp_source_length;
    uint64_t pending_dt_source_offset;
    uint64_t pending_dt_source_length;
    uint64_t pending_tfoot_source_offset;
    uint64_t pending_tfoot_source_length;
    uint64_t pending_figcaption_source_offset;
    uint64_t pending_figcaption_source_length;
    uint64_t direct_scan_element_count;
    uint64_t first_nonwhitespace_text_element_boundary;
    uint64_t traversal_element_index;
    uint64_t previous_direct_kind;
    uint64_t previous_direct_element_id;
} g03_r1a_frame;

typedef struct g03_r1a_context {
    void *output;
    uint64_t output_capacity;
    uint64_t discovery_sequence_base;
    uint64_t diagnostic_count;
    uint64_t deferred_main_form_count;
    uint64_t current_depth;
    uint8_t output_mode;
    g03_r1a_frame frames[4098];
} g03_r1a_context;

_Static_assert(sizeof(g03_r1a_frame) == 192u,
               "G03 R1A frame layout drift on x86-64");
_Static_assert(sizeof(g03_r1a_context) == 786872u,
               "G03 R1A bounded evaluator workspace layout drift on x86-64");
_Static_assert(sizeof(g03_r1a_context) <= 1048576u,
               "G03 R1A evaluator workspace exceeds 1 MiB admission");

static arbor_status status_from_errno_value(int value)
{
    return arbor_status_from_native(-(int64_t)value);
}

static arbor_status ok_status(void)
{
    return arbor_status_from_native(0);
}

static bool ascii_space(uint8_t value)
{
    return value == (uint8_t)' ' || value == (uint8_t)'\t' ||
        value == (uint8_t)'\n' || value == (uint8_t)'\f' ||
        value == (uint8_t)'\r';
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

static bool span_contains_ascii_hyphen(arbor_span span)
{
    if (span.length != 0u && span.data == NULL) {
        return false;
    }
    for (uint64_t i = 0u; i < span.length; ++i) {
        if (span.data[i] == (uint8_t)'-') {
            return true;
        }
    }
    return false;
}

static bool body_ok_keyword(arbor_span token)
{
    static const char *const keywords[] = {
        "dns-prefetch",
        "modulepreload",
        "pingback",
        "preconnect",
        "prefetch",
        "preload",
        "stylesheet"
    };
    for (size_t i = 0u; i < sizeof(keywords) / sizeof(keywords[0]); ++i) {
        if (span_ascii_ci_equals(token, keywords[i])) {
            return true;
        }
    }
    return false;
}

static bool rel_contains_only_body_ok(arbor_span value)
{
    if (value.length != 0u && value.data == NULL) {
        return false;
    }
    uint64_t position = 0u;
    while (position < value.length) {
        while (position < value.length && ascii_space(value.data[position])) {
            position += 1u;
        }
        const uint64_t start = position;
        while (position < value.length && !ascii_space(value.data[position])) {
            position += 1u;
        }
        if (position != start &&
            !body_ok_keyword((arbor_span){value.data + start, position - start})) {
            return false;
        }
    }
    return true;
}

static bool is_media_id(uint64_t id)
{
    return id == ARBOR_VIEW0_NATIVE_ELEMENT_VIDEO ||
        id == ARBOR_VIEW0_NATIVE_ELEMENT_AUDIO;
}

static bool frame_has_ancestor_id(
    const g03_r1a_context *context,
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

static uint64_t frame_expected_child_mask(const g03_r1a_frame *frame)
{
    if (frame == NULL) {
        return 0u;
    }
    switch (frame->standard_element_id) {
    case ARBOR_VIEW0_NATIVE_ELEMENT_HEAD:
        return G03_CAT_METADATA;
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
        return G03_CAT_FLOW;
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
    case ARBOR_VIEW0_NATIVE_ELEMENT_RUBY:
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
        return G03_CAT_PHRASING;
    case ARBOR_VIEW0_NATIVE_ELEMENT_TIME:
        return (frame->attribute_flags & G03_ATTR_DATETIME) != 0u
            ? G03_CAT_PHRASING
            : 0u;
    case ARBOR_VIEW0_NATIVE_ELEMENT_HGROUP:
    case ARBOR_VIEW0_NATIVE_ELEMENT_OL:
    case ARBOR_VIEW0_NATIVE_ELEMENT_UL:
    case ARBOR_VIEW0_NATIVE_ELEMENT_MENU:
    case ARBOR_VIEW0_NATIVE_ELEMENT_DL:
    case ARBOR_VIEW0_NATIVE_ELEMENT_PICTURE:
    case ARBOR_VIEW0_NATIVE_ELEMENT_TABLE:
    case ARBOR_VIEW0_NATIVE_ELEMENT_TBODY:
    case ARBOR_VIEW0_NATIVE_ELEMENT_THEAD:
    case ARBOR_VIEW0_NATIVE_ELEMENT_TFOOT:
    case ARBOR_VIEW0_NATIVE_ELEMENT_TR:
    case ARBOR_VIEW0_NATIVE_ELEMENT_SELECT:
    case ARBOR_VIEW0_NATIVE_ELEMENT_OPTGROUP:
        return G03_CAT_SCRIPT_SUPPORTING;
    case ARBOR_VIEW0_NATIVE_ELEMENT_DATALIST:
        return G03_CAT_PHRASING | G03_CAT_SCRIPT_SUPPORTING;
    case ARBOR_VIEW0_NATIVE_ELEMENT_DIV:
        if (frame->parent_standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_DL) {
            return G03_CAT_SCRIPT_SUPPORTING;
        }
        if (frame->select_transparent_context) {
            return frame->inherited_context_mask;
        }
        return G03_CAT_FLOW;
    case ARBOR_VIEW0_NATIVE_ELEMENT_A:
    case ARBOR_VIEW0_NATIVE_ELEMENT_INS:
    case ARBOR_VIEW0_NATIVE_ELEMENT_DEL:
    case ARBOR_VIEW0_NATIVE_ELEMENT_OBJECT:
    case ARBOR_VIEW0_NATIVE_ELEMENT_VIDEO:
    case ARBOR_VIEW0_NATIVE_ELEMENT_AUDIO:
    case ARBOR_VIEW0_NATIVE_ELEMENT_MAP:
    case ARBOR_VIEW0_NATIVE_ELEMENT_SLOT:
    case ARBOR_VIEW0_NATIVE_ELEMENT_CANVAS:
        return frame->inherited_context_mask;
    case ARBOR_VIEW0_NATIVE_ELEMENT_NOSCRIPT:
        return frame->parent_standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_HEAD
            ? 0u
            : frame->inherited_context_mask;
    case ARBOR_VIEW0_NATIVE_ELEMENT_OPTION:
        if (frame->datalist_ancestor ||
            (frame->attribute_flags & G03_ATTR_LABEL) != 0u) {
            return 0u;
        }
        return frame->inherited_context_mask == 0u
            ? G03_CAT_PHRASING
            : frame->inherited_context_mask;
    case ARBOR_VIEW0_NATIVE_ELEMENT_LEGEND:
    case ARBOR_VIEW0_NATIVE_ELEMENT_SUMMARY:
        return G03_CAT_PHRASING | G03_CAT_HEADING;
    case ARBOR_VIEW0_NATIVE_ELEMENT_NONE:
        return frame->inherited_context_mask;
    default:
        return 0u;
    }
}

static uint64_t placement_context_mask(
    const g03_r1a_context *context,
    const arbor_view0_native_element_observation *observation)
{
    if (observation->depth == 0u) {
        return 0u;
    }
    const g03_r1a_frame *parent = &context->frames[observation->depth - 1u];
    const uint64_t expected = frame_expected_child_mask(parent);
    if (observation->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_NONE ||
        observation->standard_element_id > ARBOR_VIEW0_NATIVE_STANDARD_ELEMENT_COUNT) {
        return expected;
    }
    const uint64_t categories = g03_element_categories[observation->standard_element_id];
    uint64_t placement = expected & categories;

    if (placement == 0u &&
        (observation->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_DIV ||
         observation->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_NOSCRIPT) &&
        (frame_has_ancestor_id(context,
                               observation->depth - 1u,
                               ARBOR_VIEW0_NATIVE_ELEMENT_SELECT) ||
         frame_has_ancestor_id(context,
                               observation->depth - 1u,
                               ARBOR_VIEW0_NATIVE_ELEMENT_OPTGROUP) ||
         frame_has_ancestor_id(context,
                               observation->depth - 1u,
                               ARBOR_VIEW0_NATIVE_ELEMENT_OPTION))) {
        placement = expected;
    }
    return placement;
}

static void fill_diagnostic(
    arbor_view0_native_diagnostic *diagnostic,
    uint64_t source_offset,
    uint64_t source_length,
    uint64_t discovery_sequence)
{
    static const char symbolic[] = "ARBOR_VIEW_V1_G03_ELEMENT_CONTEXT";
    static const char message[] =
        "HTML element is not permitted in this structural context";
    (void)memset(diagnostic, 0, sizeof(*diagnostic));
    diagnostic->rule_id = ARBOR_VIEW_V1_G03_ELEMENT_CONTEXT;
    diagnostic->byte_offset = source_offset;
    diagnostic->source_length = source_length;
    diagnostic->discovery_sequence = discovery_sequence;
    diagnostic->severity = (uint32_t)ARBOR_VIEW0_NATIVE_SEVERITY_ERROR;
    diagnostic->origin = (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_AUTHORING;
    (void)memcpy(diagnostic->symbolic_name, symbolic, sizeof(symbolic));
    (void)memcpy(diagnostic->message, message, sizeof(message));
}

void arbor_view0_native_g03_r1a_materialize_anchor(
    const arbor_view0_native_source_anchor *anchor,
    uint64_t discovery_sequence,
    arbor_view0_native_diagnostic *diagnostic)
{
    fill_diagnostic(
        diagnostic, (uint64_t)anchor->byte_offset,
        (uint64_t)anchor->source_length, discovery_sequence);
}

static arbor_status report_invalid(
    g03_r1a_context *context,
    uint64_t source_offset,
    uint64_t source_length)
{
    if (context == NULL || source_offset == ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE ||
        source_length == 0u) {
        return status_from_errno_value(EIO);
    }
    if (context->diagnostic_count == UINT64_MAX) {
        return status_from_errno_value(EOVERFLOW);
    }
    if (context->output_mode != G03_R1A_OUTPUT_MEASURE) {
        if (context->diagnostic_count >= context->output_capacity ||
            context->output == NULL) {
            return status_from_errno_value(ENOSPC);
        }
        if (context->output_mode == G03_R1A_OUTPUT_DIAGNOSTICS) {
            if (context->discovery_sequence_base > UINT64_MAX - context->diagnostic_count) {
                return status_from_errno_value(EOVERFLOW);
            }
            arbor_view0_native_diagnostic *diagnostics = context->output;
            fill_diagnostic(
                diagnostics + context->diagnostic_count, source_offset, source_length,
                context->discovery_sequence_base + context->diagnostic_count);
        } else if (context->output_mode == G03_R1A_OUTPUT_OFFSETS) {
            uint64_t *offsets = context->output;
            offsets[context->diagnostic_count] = source_offset;
        } else if (context->output_mode == G03_R1A_OUTPUT_ANCHORS) {
            if (source_offset > UINT32_MAX || source_length > UINT32_MAX) {
                return status_from_errno_value(EOVERFLOW);
            }
            arbor_view0_native_source_anchor *anchors = context->output;
            anchors[context->diagnostic_count] = (arbor_view0_native_source_anchor){
                .byte_offset = (uint32_t)source_offset,
                .source_length = (uint32_t)source_length
            };
        } else {
            return status_from_errno_value(EINVAL);
        }
    }
    context->diagnostic_count += 1u;
    return ok_status();
}

static bool child_authored(const arbor_view0_native_direct_child_observation *observation)
{
    return observation->kind == ARBOR_VIEW0_NATIVE_DIRECT_CHILD_ELEMENT &&
        (observation->flags & ARBOR_VIEW0_NATIVE_CHILD_FLAG_SYNTHETIC) == 0u;
}

static bool generic_category_valid(
    const g03_r1a_frame *parent,
    uint64_t child_id)
{
    if (parent == NULL || child_id == 0u ||
        child_id > ARBOR_VIEW0_NATIVE_STANDARD_ELEMENT_COUNT) {
        return false;
    }
    return (frame_expected_child_mask(parent) & g03_element_categories[child_id]) != 0u;
}

static bool main_hierarchy_valid_or_deferred(
    g03_r1a_context *context,
    uint64_t parent_depth,
    bool *form_deferred_out)
{
    bool form_deferred = false;
    for (uint64_t depth = 0u; depth <= parent_depth; ++depth) {
        const g03_r1a_frame *frame = &context->frames[depth];
        switch (frame->standard_element_id) {
        case ARBOR_VIEW0_NATIVE_ELEMENT_HTML:
        case ARBOR_VIEW0_NATIVE_ELEMENT_BODY:
        case ARBOR_VIEW0_NATIVE_ELEMENT_DIV:
            break;
        case ARBOR_VIEW0_NATIVE_ELEMENT_FORM:
            if ((frame->attribute_flags &
                 (G03_ATTR_ARIA_LABEL | G03_ATTR_ARIA_LABELLEDBY | G03_ATTR_TITLE)) != 0u) {
                form_deferred = true;
            }
            break;
        case ARBOR_VIEW0_NATIVE_ELEMENT_NONE:
            if (frame->namespace_id == ARBOR_VIEW0_NATIVE_NAMESPACE_HTML &&
                frame->html_hyphen_name) {
                break;
            }
            return false;
        default:
            return false;
        }
    }
    *form_deferred_out = form_deferred;
    return true;
}

static int direct_element_verdict(
    g03_r1a_context *context,
    g03_r1a_frame *parent,
    const arbor_view0_native_direct_child_observation *child)
{
    const uint64_t id = child->standard_element_id;
    const bool generic = generic_category_valid(parent, id);

    switch (id) {
    case ARBOR_VIEW0_NATIVE_ELEMENT_LINK:
    case ARBOR_VIEW0_NATIVE_ELEMENT_META:
        return G03_R1A_VERDICT_DELAYED;
    case ARBOR_VIEW0_NATIVE_ELEMENT_HTML:
        return G03_R1A_VERDICT_INVALID;
    case ARBOR_VIEW0_NATIVE_ELEMENT_HEAD:
        return parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_HTML &&
            child->element_index == 0u;
    case ARBOR_VIEW0_NATIVE_ELEMENT_BODY:
        return parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_HTML &&
            child->element_index == 1u;
    case ARBOR_VIEW0_NATIVE_ELEMENT_TITLE:
    case ARBOR_VIEW0_NATIVE_ELEMENT_BASE:
        return parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_HEAD;
    case ARBOR_VIEW0_NATIVE_ELEMENT_STYLE:
        return generic ||
            (parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_NOSCRIPT &&
             parent->parent_standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_HEAD);
    case ARBOR_VIEW0_NATIVE_ELEMENT_H1:
    case ARBOR_VIEW0_NATIVE_ELEMENT_H2:
    case ARBOR_VIEW0_NATIVE_ELEMENT_H3:
    case ARBOR_VIEW0_NATIVE_ELEMENT_H4:
    case ARBOR_VIEW0_NATIVE_ELEMENT_H5:
    case ARBOR_VIEW0_NATIVE_ELEMENT_H6:
    case ARBOR_VIEW0_NATIVE_ELEMENT_P:
        return generic || parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_HGROUP;
    case ARBOR_VIEW0_NATIVE_ELEMENT_LI:
        return frame_has_ancestor_id(context, context->current_depth,
                                     ARBOR_VIEW0_NATIVE_ELEMENT_UL) ||
            frame_has_ancestor_id(context, context->current_depth,
                                  ARBOR_VIEW0_NATIVE_ELEMENT_OL) ||
            frame_has_ancestor_id(context, context->current_depth,
                                  ARBOR_VIEW0_NATIVE_ELEMENT_MENU);
    case ARBOR_VIEW0_NATIVE_ELEMENT_DT:
        return parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_DL ||
            (parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_DIV &&
             parent->parent_standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_DL);
    case ARBOR_VIEW0_NATIVE_ELEMENT_DD:
        return (parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_DL ||
                (parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_DIV &&
                 parent->parent_standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_DL)) &&
            parent->seen_dt_or_dd;
    case ARBOR_VIEW0_NATIVE_ELEMENT_DIV:
        return generic || parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_DL ||
            frame_has_ancestor_id(context, context->current_depth,
                                  ARBOR_VIEW0_NATIVE_ELEMENT_SELECT) ||
            frame_has_ancestor_id(context, context->current_depth,
                                  ARBOR_VIEW0_NATIVE_ELEMENT_OPTGROUP) ||
            frame_has_ancestor_id(context, context->current_depth,
                                  ARBOR_VIEW0_NATIVE_ELEMENT_OPTION);
    case ARBOR_VIEW0_NATIVE_ELEMENT_RT:
        return parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_RUBY;
    case ARBOR_VIEW0_NATIVE_ELEMENT_RP:
        return G03_R1A_VERDICT_DELAYED;
    case ARBOR_VIEW0_NATIVE_ELEMENT_SOURCE:
    case ARBOR_VIEW0_NATIVE_ELEMENT_TRACK:
        return G03_R1A_VERDICT_DELAYED;
    case ARBOR_VIEW0_NATIVE_ELEMENT_IMG:
        return parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_PICTURE || generic;
    case ARBOR_VIEW0_NATIVE_ELEMENT_AREA:
        return generic && frame_has_ancestor_id(
            context, context->current_depth, ARBOR_VIEW0_NATIVE_ELEMENT_MAP);
    case ARBOR_VIEW0_NATIVE_ELEMENT_CAPTION:
        return parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_TABLE &&
            (child->flags & ARBOR_VIEW0_NATIVE_CHILD_FLAG_FIRST_ELEMENT) != 0u;
    case ARBOR_VIEW0_NATIVE_ELEMENT_COLGROUP:
        return parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_TABLE &&
            !parent->table_seen_thead && !parent->table_seen_tbody &&
            !parent->table_seen_tfoot && !parent->table_seen_tr;
    case ARBOR_VIEW0_NATIVE_ELEMENT_COL:
        return parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_COLGROUP &&
            (parent->attribute_flags & G03_ATTR_SPAN) == 0u;
    case ARBOR_VIEW0_NATIVE_ELEMENT_THEAD:
        return parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_TABLE &&
            !parent->table_seen_thead && !parent->table_seen_tbody &&
            !parent->table_seen_tfoot && !parent->table_seen_tr;
    case ARBOR_VIEW0_NATIVE_ELEMENT_TBODY:
        return parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_TABLE &&
            !parent->table_seen_tr;
    case ARBOR_VIEW0_NATIVE_ELEMENT_TFOOT:
        return parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_TABLE &&
            !parent->table_seen_tfoot;
    case ARBOR_VIEW0_NATIVE_ELEMENT_TR:
        return parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_TBODY ||
            parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_THEAD ||
            parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_TFOOT ||
            (parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_TABLE &&
             !parent->table_seen_tbody);
    case ARBOR_VIEW0_NATIVE_ELEMENT_TD:
    case ARBOR_VIEW0_NATIVE_ELEMENT_TH:
        return parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_TR;
    case ARBOR_VIEW0_NATIVE_ELEMENT_BUTTON:
        return generic ||
            (parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_SELECT &&
             !parent->meaningful_child_seen);
    case ARBOR_VIEW0_NATIVE_ELEMENT_OPTGROUP:
        return frame_has_ancestor_id(context, context->current_depth,
                                     ARBOR_VIEW0_NATIVE_ELEMENT_SELECT);
    case ARBOR_VIEW0_NATIVE_ELEMENT_OPTION:
        return frame_has_ancestor_id(context, context->current_depth,
                                     ARBOR_VIEW0_NATIVE_ELEMENT_DATALIST) ||
            frame_has_ancestor_id(context, context->current_depth,
                                  ARBOR_VIEW0_NATIVE_ELEMENT_SELECT) ||
            frame_has_ancestor_id(context, context->current_depth,
                                  ARBOR_VIEW0_NATIVE_ELEMENT_OPTGROUP);
    case ARBOR_VIEW0_NATIVE_ELEMENT_LEGEND:
        return (parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_FIELDSET ||
                parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_OPTGROUP) &&
            !parent->meaningful_child_seen;
    case ARBOR_VIEW0_NATIVE_ELEMENT_SELECTEDCONTENT:
        for (uint64_t depth = 1u; depth <= context->current_depth; ++depth) {
            if (context->frames[depth].standard_element_id ==
                    ARBOR_VIEW0_NATIVE_ELEMENT_BUTTON &&
                context->frames[depth - 1u].standard_element_id ==
                    ARBOR_VIEW0_NATIVE_ELEMENT_SELECT) {
                return G03_R1A_VERDICT_VALID;
            }
        }
        return G03_R1A_VERDICT_INVALID;
    case ARBOR_VIEW0_NATIVE_ELEMENT_SUMMARY:
        return parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_DETAILS &&
            !parent->meaningful_child_seen;
    case ARBOR_VIEW0_NATIVE_ELEMENT_NOSCRIPT:
        if (frame_has_ancestor_id(context, context->current_depth,
                                  ARBOR_VIEW0_NATIVE_ELEMENT_NOSCRIPT)) {
            return G03_R1A_VERDICT_INVALID;
        }
        return generic || parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_HEAD ||
            frame_has_ancestor_id(context, context->current_depth,
                                  ARBOR_VIEW0_NATIVE_ELEMENT_SELECT) ||
            frame_has_ancestor_id(context, context->current_depth,
                                  ARBOR_VIEW0_NATIVE_ELEMENT_OPTGROUP);
    case ARBOR_VIEW0_NATIVE_ELEMENT_TEMPLATE:
        return generic ||
            (parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_COLGROUP &&
             (parent->attribute_flags & G03_ATTR_SPAN) == 0u);
    case ARBOR_VIEW0_NATIVE_ELEMENT_FIGCAPTION:
        return G03_R1A_VERDICT_DELAYED;
    case ARBOR_VIEW0_NATIVE_ELEMENT_HR:
        return generic || frame_has_ancestor_id(
            context, context->current_depth, ARBOR_VIEW0_NATIVE_ELEMENT_SELECT);
    case ARBOR_VIEW0_NATIVE_ELEMENT_MAIN: {
        bool deferred = false;
        if (!generic || !main_hierarchy_valid_or_deferred(
                context, context->current_depth, &deferred)) {
            return G03_R1A_VERDICT_INVALID;
        }
        if (deferred) {
            if (context->deferred_main_form_count == UINT64_MAX) {
                return G03_R1A_VERDICT_INVALID;
            }
            context->deferred_main_form_count += 1u;
        }
        return G03_R1A_VERDICT_VALID;
    }
    default:
        return generic ? G03_R1A_VERDICT_VALID : G03_R1A_VERDICT_INVALID;
    }
}

static void update_parent_prefix(
    g03_r1a_frame *parent,
    const arbor_view0_native_direct_child_observation *child)
{
    if (child->kind == ARBOR_VIEW0_NATIVE_DIRECT_CHILD_TEXT) {
        if ((child->flags & ARBOR_VIEW0_NATIVE_CHILD_FLAG_TEXT_INTER_ELEMENT_WHITESPACE) == 0u) {
            parent->meaningful_child_seen = true;
            if (parent->first_nonwhitespace_text_element_boundary ==
                ARBOR_VIEW0_NATIVE_OBSERVATION_INDEX_NONE) {
                parent->first_nonwhitespace_text_element_boundary =
                    parent->direct_scan_element_count;
            }
        }
        parent->previous_direct_kind = child->kind;
        parent->previous_direct_element_id = ARBOR_VIEW0_NATIVE_ELEMENT_NONE;
        return;
    }

    const uint64_t id = child->standard_element_id;
    if (id == ARBOR_VIEW0_NATIVE_ELEMENT_DT || id == ARBOR_VIEW0_NATIVE_ELEMENT_DD) {
        parent->seen_dt_or_dd = true;
    }
    if (id == ARBOR_VIEW0_NATIVE_ELEMENT_THEAD) {
        parent->table_seen_thead = true;
    } else if (id == ARBOR_VIEW0_NATIVE_ELEMENT_TBODY) {
        parent->table_seen_tbody = true;
    } else if (id == ARBOR_VIEW0_NATIVE_ELEMENT_TFOOT) {
        parent->table_seen_tfoot = true;
    } else if (id == ARBOR_VIEW0_NATIVE_ELEMENT_TR) {
        parent->table_seen_tr = true;
    }
    parent->meaningful_child_seen = true;
    if (parent->direct_scan_element_count != UINT64_MAX) {
        parent->direct_scan_element_count += 1u;
    }
    parent->previous_direct_kind = child->kind;
    parent->previous_direct_element_id = id;
}

static arbor_status finalize_pending_rp(
    g03_r1a_context *context,
    g03_r1a_frame *parent,
    const arbor_view0_native_direct_child_observation *next)
{
    if (!parent->pending_rp) {
        return ok_status();
    }
    const bool next_is_rt = next != NULL &&
        next->kind == ARBOR_VIEW0_NATIVE_DIRECT_CHILD_ELEMENT &&
        next->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_RT;
    if (!parent->pending_rp_has_rt_before && !next_is_rt) {
        arbor_status status = report_invalid(
            context,
            parent->pending_rp_source_offset,
            parent->pending_rp_source_length);
        if (status.native != 0) {
            return status;
        }
    }
    parent->pending_rp = false;
    parent->pending_rp_has_rt_before = false;
    parent->pending_rp_source_offset = ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE;
    parent->pending_rp_source_length = 0u;
    return ok_status();
}

static arbor_status finalize_pending_figcaption_before(
    g03_r1a_context *context,
    g03_r1a_frame *parent,
    const arbor_view0_native_direct_child_observation *next)
{
    if (!parent->pending_figcaption || next == NULL) {
        return ok_status();
    }
    const bool meaningful = next->kind == ARBOR_VIEW0_NATIVE_DIRECT_CHILD_ELEMENT ||
        (next->kind == ARBOR_VIEW0_NATIVE_DIRECT_CHILD_TEXT &&
         (next->flags & ARBOR_VIEW0_NATIVE_CHILD_FLAG_TEXT_INTER_ELEMENT_WHITESPACE) == 0u);
    if (!meaningful) {
        return ok_status();
    }
    arbor_status status = report_invalid(
        context,
        parent->pending_figcaption_source_offset,
        parent->pending_figcaption_source_length);
    if (status.native != 0) {
        return status;
    }
    parent->pending_figcaption = false;
    parent->pending_figcaption_source_offset = ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE;
    parent->pending_figcaption_source_length = 0u;
    return ok_status();
}

static bool frame_is_flow_content(const g03_r1a_frame *frame)
{
    if (frame == NULL || frame->namespace_id != ARBOR_VIEW0_NATIVE_NAMESPACE_HTML) {
        return false;
    }
    if (frame->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_LINK) {
        return (frame->attribute_flags & G03_ATTR_ITEMPROP) != 0u ||
            ((frame->attribute_flags & G03_ATTR_REL) != 0u && frame->rel_only_body_ok);
    }
    if (frame->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_META) {
        return (frame->attribute_flags & G03_ATTR_ITEMPROP) != 0u;
    }
    if (frame->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_NONE ||
        frame->standard_element_id > ARBOR_VIEW0_NATIVE_STANDARD_ELEMENT_COUNT) {
        return false;
    }
    return (g03_element_categories[frame->standard_element_id] & G03_CAT_FLOW) != 0u;
}

static bool parent_has_nonwhitespace_text_before_current_element(
    const g03_r1a_frame *parent)
{
    return parent != NULL &&
        parent->first_nonwhitespace_text_element_boundary !=
            ARBOR_VIEW0_NATIVE_OBSERVATION_INDEX_NONE &&
        parent->first_nonwhitespace_text_element_boundary <= parent->traversal_element_index;
}


static arbor_status r1a_traversal_enter(
    void *context_void,
    const arbor_view0_native_element_observation *observation)
{
    g03_r1a_context *context = context_void;
    if (context == NULL || observation == NULL ||
        observation->depth > G03_R1A_MAX_DEPTH) {
        return status_from_errno_value(observation != NULL ? E2BIG : EINVAL);
    }
    context->current_depth = observation->depth;
    g03_r1a_frame *frame = &context->frames[observation->depth];
    (void)memset(frame, 0, sizeof(*frame));
    frame->standard_element_id = observation->standard_element_id;
    frame->namespace_id = observation->namespace_id;
    frame->parent_standard_element_id = observation->parent_standard_element_id;
    frame->grandparent_standard_element_id = observation->grandparent_standard_element_id;
    frame->source_offset = observation->source_offset;
    frame->source_length = observation->source_length;
    frame->authored = (observation->flags & ARBOR_VIEW0_NATIVE_ELEMENT_FLAG_SYNTHETIC) == 0u;
    frame->html_hyphen_name = observation->namespace_id == ARBOR_VIEW0_NATIVE_NAMESPACE_HTML &&
        observation->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_NONE &&
        span_contains_ascii_hyphen(observation->local_name);
    frame->rel_only_body_ok = true;
    if (observation->depth != 0u) {
        const uint64_t parent_depth = observation->depth - 1u;
        frame->select_transparent_context =
            frame_has_ancestor_id(context, parent_depth, ARBOR_VIEW0_NATIVE_ELEMENT_SELECT) ||
            frame_has_ancestor_id(context, parent_depth, ARBOR_VIEW0_NATIVE_ELEMENT_OPTGROUP) ||
            frame_has_ancestor_id(context, parent_depth, ARBOR_VIEW0_NATIVE_ELEMENT_OPTION);
        frame->datalist_ancestor =
            frame_has_ancestor_id(context, parent_depth, ARBOR_VIEW0_NATIVE_ELEMENT_DATALIST);
    }
    frame->inherited_context_mask = placement_context_mask(context, observation);
    frame->pending_rp_source_offset = ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE;
    frame->pending_dt_source_offset = ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE;
    frame->pending_tfoot_source_offset = ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE;
    frame->pending_figcaption_source_offset = ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE;
    frame->first_nonwhitespace_text_element_boundary =
        ARBOR_VIEW0_NATIVE_OBSERVATION_INDEX_NONE;

    if (frame->authored && observation->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_HTML) {
        if (observation->depth != 0u ||
            observation->parent_standard_element_id != ARBOR_VIEW0_NATIVE_ELEMENT_NONE) {
            return report_invalid(context, observation->source_offset, observation->source_length);
        }
    }

    if (frame->authored && observation->depth != 0u &&
        (observation->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_SOURCE ||
         observation->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_TRACK)) {
        g03_r1a_frame *parent = &context->frames[observation->depth - 1u];
        const bool text_before = parent_has_nonwhitespace_text_before_current_element(parent);
        bool valid = false;
        if (observation->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_SOURCE) {
            if (parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_PICTURE) {
                valid = !parent->seen_picture_img;
            } else if (is_media_id(parent->standard_element_id)) {
                valid = !parent->seen_media_flow && !parent->seen_track && !text_before;
            }
        } else if (is_media_id(parent->standard_element_id)) {
            valid = !parent->seen_media_flow && !text_before;
        }
        if (!valid) {
            return report_invalid(context, observation->source_offset, observation->source_length);
        }
    }
    return ok_status();
}

static arbor_status r1a_attribute(
    void *context_void,
    const arbor_view0_native_attribute_observation *observation)
{
    g03_r1a_context *context = context_void;
    if (context == NULL || observation == NULL || context->current_depth > G03_R1A_MAX_DEPTH) {
        return status_from_errno_value(EINVAL);
    }
    g03_r1a_frame *frame = &context->frames[context->current_depth];

    if (span_ascii_ci_equals(observation->local_name, "span")) {
        frame->attribute_flags |= G03_ATTR_SPAN;
    } else if (span_ascii_ci_equals(observation->local_name, "datetime")) {
        frame->attribute_flags |= G03_ATTR_DATETIME;
    } else if (span_ascii_ci_equals(observation->local_name, "label")) {
        frame->attribute_flags |= G03_ATTR_LABEL;
    } else if (span_ascii_ci_equals(observation->local_name, "value")) {
        frame->attribute_flags |= G03_ATTR_VALUE;
    } else if (span_ascii_ci_equals(observation->local_name, "aria-label")) {
        frame->attribute_flags |= G03_ATTR_ARIA_LABEL;
    } else if (span_ascii_ci_equals(observation->local_name, "aria-labelledby")) {
        frame->attribute_flags |= G03_ATTR_ARIA_LABELLEDBY;
    } else if (span_ascii_ci_equals(observation->local_name, "title")) {
        frame->attribute_flags |= G03_ATTR_TITLE;
    } else if (span_ascii_ci_equals(observation->local_name, "itemprop")) {
        frame->attribute_flags |= G03_ATTR_ITEMPROP;
    } else if (span_ascii_ci_equals(observation->local_name, "rel")) {
        frame->attribute_flags |= G03_ATTR_REL;
        frame->rel_only_body_ok = rel_contains_only_body_ok(observation->value);
    } else if (span_ascii_ci_equals(observation->local_name, "http-equiv")) {
        frame->attribute_flags |= G03_ATTR_HTTP_EQUIV;
        frame->http_equiv_content_type =
            span_ascii_ci_equals(observation->value, "content-type");
    } else if (span_ascii_ci_equals(observation->local_name, "charset")) {
        frame->attribute_flags |= G03_ATTR_CHARSET;
    } else if (span_ascii_ci_equals(observation->local_name, "name")) {
        frame->attribute_flags |= G03_ATTR_NAME;
    }
    return ok_status();
}

static arbor_status r1a_direct_child(
    void *context_void,
    const arbor_view0_native_direct_child_observation *observation)
{
    g03_r1a_context *context = context_void;
    if (context == NULL || observation == NULL || context->current_depth > G03_R1A_MAX_DEPTH) {
        return status_from_errno_value(EINVAL);
    }
    g03_r1a_frame *parent = &context->frames[context->current_depth];
    if (observation->parent_standard_element_id != parent->standard_element_id) {
        return status_from_errno_value(EIO);
    }

    arbor_status status = finalize_pending_rp(context, parent, observation);
    if (status.native != 0) {
        return status;
    }
    status = finalize_pending_figcaption_before(context, parent, observation);
    if (status.native != 0) {
        return status;
    }

    /*
     * A dt is admitted only before a later dt/dd in the same dl grouping
     * container. Script-supporting or other intervening nodes do not consume
     * that pending relation; the parent completion callback diagnoses a
     * trailing dt. A following dt/dd satisfies the previous pending dt.
     */
    if (observation->kind == ARBOR_VIEW0_NATIVE_DIRECT_CHILD_ELEMENT &&
        (observation->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_DT ||
         observation->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_DD)) {
        parent->pending_dt = false;
        parent->pending_dt_source_offset = ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE;
        parent->pending_dt_source_length = 0u;
    }

    /*
     * tbody/direct-row contexts do not themselves prohibit a preceding tfoot;
     * the frozen tfoot context instead requires tfoot to follow them. Keep the
     * first tfoot anchor and blame that earlier element if later tbody/tr
     * content appears. Synthetic tbody is intentionally sufficient evidence
     * for authored direct rows repaired by the parser.
     */
    if (parent->pending_tfoot &&
        observation->kind == ARBOR_VIEW0_NATIVE_DIRECT_CHILD_ELEMENT &&
        (observation->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_TBODY ||
         observation->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_TR)) {
        status = report_invalid(
            context,
            parent->pending_tfoot_source_offset,
            parent->pending_tfoot_source_length);
        if (status.native != 0) {
            return status;
        }
        parent->pending_tfoot = false;
        parent->pending_tfoot_source_offset = ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE;
        parent->pending_tfoot_source_length = 0u;
    }

    if (observation->kind == ARBOR_VIEW0_NATIVE_DIRECT_CHILD_ELEMENT &&
        observation->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_FIGCAPTION) {
        if (parent->standard_element_id != ARBOR_VIEW0_NATIVE_ELEMENT_FIGURE) {
            if (child_authored(observation)) {
                status = report_invalid(context, observation->source_offset, observation->source_length);
                if (status.native != 0) {
                    return status;
                }
            }
        } else if (child_authored(observation) && parent->meaningful_child_seen) {
            parent->pending_figcaption = true;
            parent->pending_figcaption_source_offset = observation->source_offset;
            parent->pending_figcaption_source_length = observation->source_length;
        }
        update_parent_prefix(parent, observation);
        return ok_status();
    }

    if (observation->kind == ARBOR_VIEW0_NATIVE_DIRECT_CHILD_ELEMENT &&
        observation->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_RP) {
        if (parent->standard_element_id != ARBOR_VIEW0_NATIVE_ELEMENT_RUBY) {
            if (child_authored(observation)) {
                status = report_invalid(context, observation->source_offset, observation->source_length);
                if (status.native != 0) {
                    return status;
                }
            }
        } else if (child_authored(observation)) {
            parent->pending_rp = true;
            parent->pending_rp_has_rt_before =
                parent->previous_direct_kind == ARBOR_VIEW0_NATIVE_DIRECT_CHILD_ELEMENT &&
                parent->previous_direct_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_RT;
            parent->pending_rp_source_offset = observation->source_offset;
            parent->pending_rp_source_length = observation->source_length;
        }
        update_parent_prefix(parent, observation);
        return ok_status();
    }

    if (observation->kind == ARBOR_VIEW0_NATIVE_DIRECT_CHILD_ELEMENT &&
        observation->namespace_id == ARBOR_VIEW0_NATIVE_NAMESPACE_HTML &&
        observation->standard_element_id != ARBOR_VIEW0_NATIVE_ELEMENT_NONE &&
        child_authored(observation)) {
        const int verdict = direct_element_verdict(context, parent, observation);
        if (verdict == G03_R1A_VERDICT_INVALID) {
            status = report_invalid(context, observation->source_offset, observation->source_length);
            if (status.native != 0) {
                return status;
            }
        } else if (verdict == G03_R1A_VERDICT_VALID) {
            if (observation->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_DT &&
                (parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_DL ||
                 (parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_DIV &&
                  parent->parent_standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_DL))) {
                parent->pending_dt = true;
                parent->pending_dt_source_offset = observation->source_offset;
                parent->pending_dt_source_length = observation->source_length;
            }
            if (observation->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_TFOOT &&
                parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_TABLE &&
                !parent->table_seen_tfoot) {
                parent->pending_tfoot = true;
                parent->pending_tfoot_source_offset = observation->source_offset;
                parent->pending_tfoot_source_length = observation->source_length;
            }
        }
    }

    update_parent_prefix(parent, observation);
    return ok_status();
}

static arbor_status evaluate_delayed_element(
    g03_r1a_context *context,
    g03_r1a_frame *frame)
{
    if (!frame->authored || frame->namespace_id != ARBOR_VIEW0_NATIVE_NAMESPACE_HTML) {
        return ok_status();
    }
    if (frame->standard_element_id != ARBOR_VIEW0_NATIVE_ELEMENT_LINK &&
        frame->standard_element_id != ARBOR_VIEW0_NATIVE_ELEMENT_META) {
        return ok_status();
    }
    if (context->current_depth == 0u) {
        return report_invalid(context, frame->source_offset, frame->source_length);
    }
    const g03_r1a_frame *parent = &context->frames[context->current_depth - 1u];
    const uint64_t expected = frame_expected_child_mask(parent);
    bool valid = false;

    if (frame->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_LINK) {
        valid = (expected & G03_CAT_METADATA) != 0u ||
            (parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_NOSCRIPT &&
             parent->parent_standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_HEAD);
        const bool body_allowed =
            (frame->attribute_flags & G03_ATTR_ITEMPROP) != 0u ||
            ((frame->attribute_flags & G03_ATTR_REL) != 0u && frame->rel_only_body_ok);
        if (body_allowed && (expected & (G03_CAT_FLOW | G03_CAT_PHRASING)) != 0u) {
            valid = true;
        }
    } else {
        const bool has_charset = (frame->attribute_flags & G03_ATTR_CHARSET) != 0u;
        const bool has_http = (frame->attribute_flags & G03_ATTR_HTTP_EQUIV) != 0u;
        const bool has_name = (frame->attribute_flags & G03_ATTR_NAME) != 0u;
        const bool has_itemprop = (frame->attribute_flags & G03_ATTR_ITEMPROP) != 0u;

        if ((has_charset || (has_http && frame->http_equiv_content_type)) &&
            parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_HEAD) {
            valid = true;
        }
        if (has_http && !frame->http_equiv_content_type &&
            (parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_HEAD ||
             (parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_NOSCRIPT &&
              parent->parent_standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_HEAD))) {
            valid = true;
        }
        if (has_name && (expected & G03_CAT_METADATA) != 0u) {
            valid = true;
        }
        if (has_itemprop &&
            (expected & (G03_CAT_METADATA | G03_CAT_FLOW | G03_CAT_PHRASING)) != 0u) {
            valid = true;
        }
    }

    return valid ? ok_status() : report_invalid(
        context, frame->source_offset, frame->source_length);
}

static arbor_status r1a_element_complete(
    void *context_void,
    const arbor_view0_native_element_observation *observation)
{
    g03_r1a_context *context = context_void;
    if (context == NULL || observation == NULL || observation->depth > G03_R1A_MAX_DEPTH ||
        context->current_depth != observation->depth) {
        return status_from_errno_value(EINVAL);
    }
    g03_r1a_frame *frame = &context->frames[observation->depth];
    arbor_status status = finalize_pending_rp(context, frame, NULL);
    if (status.native != 0) {
        return status;
    }
    if (frame->pending_dt) {
        status = report_invalid(
            context, frame->pending_dt_source_offset, frame->pending_dt_source_length);
        if (status.native != 0) {
            return status;
        }
        frame->pending_dt = false;
        frame->pending_dt_source_offset = ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE;
        frame->pending_dt_source_length = 0u;
    }
    frame->pending_figcaption = false;
    frame->pending_figcaption_source_offset = ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE;
    frame->pending_figcaption_source_length = 0u;
    return evaluate_delayed_element(context, frame);
}

static arbor_status r1a_traversal_leave(
    void *context_void,
    const arbor_view0_native_element_observation *observation)
{
    g03_r1a_context *context = context_void;
    if (context == NULL || observation == NULL || observation->depth > G03_R1A_MAX_DEPTH) {
        return status_from_errno_value(EINVAL);
    }
    if (observation->depth != 0u) {
        g03_r1a_frame *child = &context->frames[observation->depth];
        g03_r1a_frame *parent = &context->frames[observation->depth - 1u];
        if (is_media_id(parent->standard_element_id)) {
            if (child->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_TRACK) {
                parent->seen_track = true;
            }
            if (frame_is_flow_content(child)) {
                parent->seen_media_flow = true;
            }
        }
        if (parent->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_PICTURE &&
            child->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_IMG) {
            parent->seen_picture_img = true;
        }
        if (parent->traversal_element_index == UINT64_MAX) {
            return status_from_errno_value(EOVERFLOW);
        }
        parent->traversal_element_index += 1u;
    }
    context->current_depth = observation->depth == 0u
        ? 0u
        : observation->depth - 1u;
    return ok_status();
}

static arbor_status evaluate(
    arbor_span input,
    void *output,
    uint64_t output_capacity,
    uint64_t discovery_sequence_base,
    uint8_t output_mode,
    arbor_view0_native_g03_r1a_evaluation *evaluation_out)
{
    if (evaluation_out == NULL || output_mode > G03_R1A_OUTPUT_ANCHORS ||
        (output_mode != G03_R1A_OUTPUT_MEASURE &&
         output_capacity != 0u && output == NULL)) {
        return status_from_errno_value(EINVAL);
    }

    g03_r1a_context context = {
        .output = output,
        .output_capacity = output_capacity,
        .discovery_sequence_base = discovery_sequence_base,
        .diagnostic_count = 0u,
        .deferred_main_form_count = 0u,
        .current_depth = 0u,
        .output_mode = output_mode,
        .frames = {{0}}
    };
    arbor_view0_native_semantic_observer observer = {
        .context = &context,
        .element_begin = NULL,
        .attribute = r1a_attribute,
        .direct_child = r1a_direct_child,
        .element_complete = r1a_element_complete,
        .traversal_enter = r1a_traversal_enter,
        .traversal_leave = r1a_traversal_leave
    };
    arbor_view0_native_parse_counts parse_counts = {0};
    arbor_view0_native_document_facts facts = {0};
    arbor_view0_native_observation_counts observation_counts = {0};
    arbor_status status = arbor_view0_native_lexbor_observe(
        input,
        &observer,
        &parse_counts,
        &facts,
        &observation_counts);
    if (status.native != 0) {
        return status;
    }
    if (observation_counts.max_depth > G03_R1A_MAX_DEPTH) {
        return status_from_errno_value(E2BIG);
    }
    if (output_mode != G03_R1A_OUTPUT_MEASURE &&
        context.diagnostic_count != output_capacity) {
        return status_from_errno_value(EIO);
    }

    const arbor_view0_native_g03_r1a_evaluation result = {
        .diagnostic_count = context.diagnostic_count,
        .deferred_main_form_count = context.deferred_main_form_count
    };
    *evaluation_out = result;
    return ok_status();
}

arbor_status arbor_view0_native_g03_r1a_measure(
    arbor_span input,
    arbor_view0_native_g03_r1a_evaluation *evaluation_out)
{
    return evaluate(input, NULL, 0u, 0u, G03_R1A_OUTPUT_MEASURE, evaluation_out);
}

arbor_status arbor_view0_native_g03_r1a_collect(
    arbor_span input,
    arbor_view0_native_diagnostic *diagnostics,
    uint64_t diagnostic_capacity,
    uint64_t discovery_sequence_base,
    arbor_view0_native_g03_r1a_evaluation *evaluation_out)
{
    return evaluate(
        input,
        diagnostics,
        diagnostic_capacity,
        discovery_sequence_base,
        G03_R1A_OUTPUT_DIAGNOSTICS,
        evaluation_out);
}

arbor_status arbor_view0_native_g03_r1a_collect_offsets(
    arbor_span input,
    uint64_t *source_offsets,
    uint64_t offset_capacity,
    arbor_view0_native_g03_r1a_evaluation *evaluation_out)
{
    return evaluate(
        input,
        source_offsets,
        offset_capacity,
        0u,
        G03_R1A_OUTPUT_OFFSETS,
        evaluation_out);
}

arbor_status arbor_view0_native_g03_r1a_collect_anchors(
    arbor_span input,
    arbor_view0_native_source_anchor *anchors,
    uint64_t anchor_capacity,
    arbor_view0_native_g03_r1a_evaluation *evaluation_out)
{
    return evaluate(
        input,
        anchors,
        anchor_capacity,
        0u,
        G03_R1A_OUTPUT_ANCHORS,
        evaluation_out);
}
