#include "g04_r1a.h"
#include "g03_r1a.h"
#include "g03_r2a.h"
#include "g03_r3a.h"
#include "g03_r4a.h"
#include "g03_r5a.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define G04_R1A_FRAME_COUNT UINT64_C(4098)
#define G04_R1A_PRIOR_ANCHOR_CAPACITY UINT64_C(4096)
#define G04_R1A_NOINLINE __attribute__((noinline))

#define G04_CAT_FLOW UINT64_C(0x01)
#define G04_CAT_PHRASING UINT64_C(0x02)
#define G04_CAT_SCRIPT_SUPPORTING UINT64_C(0x40)

typedef enum g04_r1a_model {
    G04_R1A_MODEL_UNKNOWN = 0,
    G04_R1A_MODEL_FLOW = 1,
    G04_R1A_MODEL_PHRASING = 2,
    G04_R1A_MODEL_SELECT_TAIL = 3,
    G04_R1A_MODEL_OPTGROUP_TAIL = 4,
    G04_R1A_MODEL_OPTION_TAIL = 5
} g04_r1a_model;

typedef struct g04_r1a_frame {
    uint64_t standard_element_id;
    uint64_t source_offset;
    uint64_t source_length;
    uint32_t effective_model;
    uint32_t flags;
} g04_r1a_frame;

#define G04_R1A_FRAME_VALID UINT32_C(0x1)
#define G04_R1A_FRAME_TRANSPARENT UINT32_C(0x2)
#define G04_R1A_FRAME_BLOCKED UINT32_C(0x4)
#define G04_R1A_FRAME_OPTION_LABEL UINT32_C(0x8)
#define G04_R1A_FRAME_OPTION_VALUE UINT32_C(0x10)
#define G04_R1A_FRAME_OPTION_DATALIST UINT32_C(0x20)
#define G04_R1A_FRAME_TEXT_REPORTED UINT32_C(0x40)

typedef struct g04_r1a_context {
    union {
        arbor_view0_native_diagnostic *diagnostics;
        arbor_view0_native_source_anchor *anchors;
    } output;
    uint64_t output_capacity;
    uint64_t discovery_sequence_base;
    uint64_t diagnostic_count;
    uint64_t prior_owner_suppression_count;
    uint64_t deferred_flags;
    uint64_t noscript_deferred_count; /* numerically retained; remains zero under R1C */
    uint64_t noscript_resolved_count;
    uint64_t option_branch_deferred_count;
    uint64_t option_branch_resolved_count;
    uint64_t select_text_violation_count;
    uint64_t g13_custom_deferred_count;
    const arbor_view0_native_source_anchor *prior_error_anchors;
    uint64_t prior_error_anchor_count;
    uint64_t known_depth;
    uint64_t pending_option_source_offset;
    bool pending_option_has_label;
    bool pending_option_has_value;
    bool publish;
    bool collect_anchors;
    g04_r1a_frame frames[4098];
} g04_r1a_context;

_Static_assert(ARBOR_VIEW0_NATIVE_STANDARD_ELEMENT_COUNT == UINT64_C(113),
               "G04 R1A standard-element inventory drift");
_Static_assert(ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS == G04_R1A_PRIOR_ANCHOR_CAPACITY,
               "G04 R1A prior-owner anchor bound drift");
_Static_assert(sizeof(g04_r1a_frame) == 32u,
               "G04 R1B frame layout drift on x86-64");
_Static_assert(sizeof(g04_r1a_context) <= 1048576u,
               "G04 R1A evaluator workspace exceeds 1 MiB admission");
_Static_assert(sizeof("ARBOR_VIEW_V1_G04_TRANSPARENT_PARENT_MODEL") <=
                   ARBOR_VIEW0_NATIVE_RULE_SYMBOL_CAP,
               "G04 R1A symbolic name exceeds diagnostic capacity");
_Static_assert(sizeof("Transparent element content does not satisfy the containing parent content model") <=
                   ARBOR_VIEW0_NATIVE_MESSAGE_CAP,
               "G04 R1A message exceeds diagnostic capacity");

/*
 * Exact standard-element category masks retained from the accepted F1-R2
 * 113-token element-definition authority. This private table intentionally
 * does not classify autonomous custom elements; that overlap remains G13-owned.
 */
static const uint64_t g04_element_categories[114] = {
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

bool arbor_view0_native_g04_standard_element_is_flow_content(uint64_t standard_element_id)
{
    return standard_element_id != ARBOR_VIEW0_NATIVE_ELEMENT_NONE &&
        standard_element_id <= ARBOR_VIEW0_NATIVE_STANDARD_ELEMENT_COUNT &&
        (g04_element_categories[standard_element_id] & G04_CAT_FLOW) != 0u;
}

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

static bool span_ascii_equals(arbor_span span, const char *literal)
{
    if (literal == NULL || (span.length != 0u && span.data == NULL)) return false;
    size_t n = strlen(literal);
    if ((uint64_t)n != span.length) return false;
    for (uint64_t i = 0u; i < span.length; ++i) {
        uint8_t a = span.data[i];
        uint8_t b = (uint8_t)literal[i];
        if (a >= (uint8_t)'A' && a <= (uint8_t)'Z') a = (uint8_t)(a + 0x20u);
        if (b >= (uint8_t)'A' && b <= (uint8_t)'Z') b = (uint8_t)(b + 0x20u);
        if (a != b) return false;
    }
    return true;
}

static bool text_is_inter_element_whitespace(arbor_span text)
{
    if (text.length != 0u && text.data == NULL) return false;
    for (uint64_t i = 0u; i < text.length; ++i) {
        const uint8_t b = text.data[i];
        if (b != UINT8_C(0x09) && b != UINT8_C(0x0a) && b != UINT8_C(0x0c) &&
            b != UINT8_C(0x0d) && b != UINT8_C(0x20)) return false;
    }
    return true;
}

static bool prior_owner_at_offset(const g04_r1a_context *context, uint64_t offset)
{
    if (context == NULL || offset == ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE) return false;
    for (uint64_t i = 0u; i < context->prior_error_anchor_count; ++i) {
        if ((uint64_t)context->prior_error_anchors[i].byte_offset == offset) return true;
    }
    return false;
}

static uint64_t base_expected_category_mask(uint64_t parent_id)
{
    switch (parent_id) {
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
    case ARBOR_VIEW0_NATIVE_ELEMENT_DIV:
    case ARBOR_VIEW0_NATIVE_ELEMENT_CAPTION:
    case ARBOR_VIEW0_NATIVE_ELEMENT_TD:
    case ARBOR_VIEW0_NATIVE_ELEMENT_TH:
    case ARBOR_VIEW0_NATIVE_ELEMENT_FORM:
    case ARBOR_VIEW0_NATIVE_ELEMENT_FIELDSET:
    case ARBOR_VIEW0_NATIVE_ELEMENT_DETAILS:
    case ARBOR_VIEW0_NATIVE_ELEMENT_DIALOG:
        return G04_CAT_FLOW;
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
    case ARBOR_VIEW0_NATIVE_ELEMENT_TIME:
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
    case ARBOR_VIEW0_NATIVE_ELEMENT_DATALIST:
    case ARBOR_VIEW0_NATIVE_ELEMENT_OUTPUT:
    case ARBOR_VIEW0_NATIVE_ELEMENT_PROGRESS:
    case ARBOR_VIEW0_NATIVE_ELEMENT_METER:
    case ARBOR_VIEW0_NATIVE_ELEMENT_LEGEND:
    case ARBOR_VIEW0_NATIVE_ELEMENT_SUMMARY:
        return G04_CAT_PHRASING;
    default:
        return 0u;
    }
}

static bool frame_matches_current(
    const g04_r1a_context *context,
    uint64_t depth,
    uint64_t source_offset)
{
    if (context == NULL || depth == 0u || depth > G04_R1A_FRAME_COUNT ||
        source_offset == ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE) return false;
    const g04_r1a_frame *frame = &context->frames[depth - 1u];
    return (frame->flags & G04_R1A_FRAME_VALID) != 0u &&
        frame->source_offset == source_offset;
}

static bool select_family_ancestor(const g04_r1a_context *context, uint64_t depth)
{
    if (context == NULL || depth > G04_R1A_FRAME_COUNT) return false;
    for (uint64_t i = 0u; i < depth; ++i) {
        if ((context->frames[i].flags & G04_R1A_FRAME_VALID) == 0u) continue;
        const uint64_t id = context->frames[i].standard_element_id;
        if (id == ARBOR_VIEW0_NATIVE_ELEMENT_SELECT ||
            id == ARBOR_VIEW0_NATIVE_ELEMENT_OPTGROUP ||
            id == ARBOR_VIEW0_NATIVE_ELEMENT_OPTION) return true;
    }
    return false;
}

static bool datalist_ancestor(const g04_r1a_context *context, uint64_t depth)
{
    if (context == NULL || depth > G04_R1A_FRAME_COUNT) return false;
    for (uint64_t i = 0u; i < depth; ++i) {
        if ((context->frames[i].flags & G04_R1A_FRAME_VALID) != 0u &&
            context->frames[i].standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_DATALIST)
            return true;
    }
    return false;
}

static bool element_is_transparent(
    g04_r1a_context *context,
    const arbor_view0_native_source_repair_context *record)
{
    switch (record->standard_element_id) {
    case ARBOR_VIEW0_NATIVE_ELEMENT_A:
    case ARBOR_VIEW0_NATIVE_ELEMENT_INS:
    case ARBOR_VIEW0_NATIVE_ELEMENT_DEL:
    case ARBOR_VIEW0_NATIVE_ELEMENT_OBJECT:
    case ARBOR_VIEW0_NATIVE_ELEMENT_VIDEO:
    case ARBOR_VIEW0_NATIVE_ELEMENT_AUDIO:
    case ARBOR_VIEW0_NATIVE_ELEMENT_MAP:
    case ARBOR_VIEW0_NATIVE_ELEMENT_SLOT:
    case ARBOR_VIEW0_NATIVE_ELEMENT_CANVAS:
        return true;
    case ARBOR_VIEW0_NATIVE_ELEMENT_DIV:
        return record->initial_current_standard_element_id != ARBOR_VIEW0_NATIVE_ELEMENT_DL &&
            select_family_ancestor(context, record->initial_open_elements_depth);
    case ARBOR_VIEW0_NATIVE_ELEMENT_NOSCRIPT:
        /* V1N1 G04 R1C explicitly freezes this checker/parser path to
         * scripting disabled. Outside head, the pinned element definition is
         * therefore transparent. In head it has the separate link/style/meta
         * model and is not a G04 R1 transparent subject. */
        if (record->initial_current_standard_element_id != ARBOR_VIEW0_NATIVE_ELEMENT_HEAD) {
            if (context->noscript_resolved_count != UINT64_MAX)
                context->noscript_resolved_count += 1u;
            return true;
        }
        return false;
    default:
        return false;
    }
}

static g04_r1a_model containing_model(
    g04_r1a_context *context,
    const arbor_view0_native_source_repair_context *record)
{
    const uint64_t depth = record->initial_open_elements_depth;
    if (depth == 0u) return G04_R1A_MODEL_UNKNOWN; /* R2 owns parentless. */
    if (frame_matches_current(context, depth, record->initial_current_source_offset)) {
        const g04_r1a_frame *parent = &context->frames[depth - 1u];
        if ((parent->flags & G04_R1A_FRAME_TRANSPARENT) != 0u) {
            if ((parent->flags & G04_R1A_FRAME_BLOCKED) != 0u)
                return G04_R1A_MODEL_UNKNOWN;
            return (g04_r1a_model)parent->effective_model;
        }
    }
    if (record->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_NOSCRIPT) {
        if (record->initial_current_standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_SELECT)
            return G04_R1A_MODEL_SELECT_TAIL;
        if (record->initial_current_standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_OPTGROUP)
            return G04_R1A_MODEL_OPTGROUP_TAIL;
        if (record->initial_current_standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_OPTION &&
            frame_matches_current(context, depth, record->initial_current_source_offset)) {
            const g04_r1a_frame *parent = &context->frames[depth - 1u];
            const bool has_label = (parent->flags & G04_R1A_FRAME_OPTION_LABEL) != 0u;
            const bool in_datalist = (parent->flags & G04_R1A_FRAME_OPTION_DATALIST) != 0u;
            if (!has_label && !in_datalist) return G04_R1A_MODEL_PHRASING;
            return G04_R1A_MODEL_UNKNOWN; /* G03 owns non-admitted option branches. */
        }
    }
    if (record->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_DIV) {
        if (record->initial_current_standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_SELECT)
            return G04_R1A_MODEL_SELECT_TAIL;
        if (record->initial_current_standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_OPTGROUP)
            return G04_R1A_MODEL_OPTGROUP_TAIL;
        if (record->initial_current_standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_OPTION) {
            if (frame_matches_current(context, depth, record->initial_current_source_offset)) {
                const g04_r1a_frame *parent = &context->frames[depth - 1u];
                const bool has_label = (parent->flags & G04_R1A_FRAME_OPTION_LABEL) != 0u;
                const bool in_datalist = (parent->flags & G04_R1A_FRAME_OPTION_DATALIST) != 0u;
                if (arbor_view0_native_g04_select_transparent_div_is_r7_subject(
                        ARBOR_VIEW0_NATIVE_ELEMENT_OPTION, has_label, in_datalist)) {
                    if (context->option_branch_resolved_count == UINT64_MAX)
                        return G04_R1A_MODEL_UNKNOWN;
                    context->option_branch_resolved_count += 1u;
                    return G04_R1A_MODEL_OPTION_TAIL;
                }
            }
            /* The remaining option branches do not admit div at all; G03 owns
             * that parent-content/context error, so there is no G04 deferral. */
            return G04_R1A_MODEL_UNKNOWN;
        }
    }
    const uint64_t expected = base_expected_category_mask(
        record->initial_current_standard_element_id);
    if (record->standard_element_id > ARBOR_VIEW0_NATIVE_STANDARD_ELEMENT_COUNT)
        return G04_R1A_MODEL_UNKNOWN;
    const uint64_t placement = expected & g04_element_categories[record->standard_element_id];
    if ((placement & G04_CAT_PHRASING) != 0u) return G04_R1A_MODEL_PHRASING;
    if ((placement & G04_CAT_FLOW) != 0u) return G04_R1A_MODEL_FLOW;
    return G04_R1A_MODEL_UNKNOWN;
}

bool arbor_view0_native_g04_select_transparent_div_is_r7_subject(
    uint64_t containing_standard_element_id,
    bool option_has_label,
    bool option_in_datalist)
{
    return containing_standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_OPTION &&
        !option_has_label && !option_in_datalist;
}

static bool model_allows_element(g04_r1a_model model, uint64_t child_id)
{
    if (child_id == ARBOR_VIEW0_NATIVE_ELEMENT_NONE ||
        child_id > ARBOR_VIEW0_NATIVE_STANDARD_ELEMENT_COUNT) return true;
    const uint64_t categories = g04_element_categories[child_id];
    switch (model) {
    case G04_R1A_MODEL_FLOW:
        return (categories & G04_CAT_FLOW) != 0u;
    case G04_R1A_MODEL_PHRASING:
        return (categories & G04_CAT_PHRASING) != 0u;
    case G04_R1A_MODEL_SELECT_TAIL:
        return child_id == ARBOR_VIEW0_NATIVE_ELEMENT_OPTION ||
            child_id == ARBOR_VIEW0_NATIVE_ELEMENT_OPTGROUP ||
            child_id == ARBOR_VIEW0_NATIVE_ELEMENT_HR ||
            child_id == ARBOR_VIEW0_NATIVE_ELEMENT_NOSCRIPT ||
            child_id == ARBOR_VIEW0_NATIVE_ELEMENT_DIV ||
            (categories & G04_CAT_SCRIPT_SUPPORTING) != 0u;
    case G04_R1A_MODEL_OPTGROUP_TAIL:
        return child_id == ARBOR_VIEW0_NATIVE_ELEMENT_OPTION ||
            child_id == ARBOR_VIEW0_NATIVE_ELEMENT_NOSCRIPT ||
            child_id == ARBOR_VIEW0_NATIVE_ELEMENT_DIV ||
            (categories & G04_CAT_SCRIPT_SUPPORTING) != 0u;
    case G04_R1A_MODEL_OPTION_TAIL:
        return child_id == ARBOR_VIEW0_NATIVE_ELEMENT_DIV ||
            (categories & G04_CAT_PHRASING) != 0u;
    default:
        return true;
    }
}

static void fill_diagnostic(
    arbor_view0_native_diagnostic *diagnostic,
    uint64_t source_offset,
    uint64_t source_length,
    uint64_t discovery_sequence)
{
    static const char symbolic[] = "ARBOR_VIEW_V1_G04_TRANSPARENT_PARENT_MODEL";
    static const char message[] =
        "Transparent element content does not satisfy the containing parent content model";
    (void)memset(diagnostic, 0, sizeof(*diagnostic));
    diagnostic->rule_id = ARBOR_VIEW_V1_G04_TRANSPARENT_PARENT_MODEL;
    diagnostic->byte_offset = source_offset;
    diagnostic->source_length = source_length;
    diagnostic->discovery_sequence = discovery_sequence;
    diagnostic->severity = (uint32_t)ARBOR_VIEW0_NATIVE_SEVERITY_ERROR;
    diagnostic->origin = (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_AUTHORING;
    (void)memcpy(diagnostic->symbolic_name, symbolic, sizeof(symbolic));
    (void)memcpy(diagnostic->message, message, sizeof(message));
}

void arbor_view0_native_g04_r1a_materialize_anchor(
    const arbor_view0_native_source_anchor *anchor,
    uint64_t discovery_sequence,
    arbor_view0_native_diagnostic *diagnostic)
{
    fill_diagnostic(diagnostic, anchor->byte_offset, anchor->source_length, discovery_sequence);
}

static arbor_status report_invalid_anchor(
    g04_r1a_context *context, uint64_t source_offset, uint64_t source_length)
{
    if (context == NULL || source_offset == ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE ||
        source_length == 0u) return status_from_errno_value(EINVAL);
    if (context->diagnostic_count == UINT64_MAX) return status_from_errno_value(EOVERFLOW);
    if (context->publish || context->collect_anchors) {
        if (context->diagnostic_count >= context->output_capacity)
            return status_from_errno_value(ENOBUFS);
        if (source_offset > UINT32_MAX || source_length > UINT32_MAX)
            return status_from_errno_value(E2BIG);
        if (context->publish) {
            if (context->output.diagnostics == NULL) return status_from_errno_value(EINVAL);
            if (context->discovery_sequence_base > UINT64_MAX - context->diagnostic_count)
                return status_from_errno_value(EOVERFLOW);
            fill_diagnostic(context->output.diagnostics + context->diagnostic_count,
                source_offset, source_length,
                context->discovery_sequence_base + context->diagnostic_count);
        } else {
            if (context->output.anchors == NULL) return status_from_errno_value(EINVAL);
            context->output.anchors[context->diagnostic_count] =
                (arbor_view0_native_source_anchor){
                    .byte_offset = (uint32_t)source_offset,
                    .source_length = (uint32_t)source_length
                };
        }
    }
    context->diagnostic_count += 1u;
    return ok_status();
}

static arbor_status report_invalid(
    g04_r1a_context *context,
    const arbor_view0_native_source_repair_context *record)
{
    if (context == NULL || record == NULL) return status_from_errno_value(EINVAL);
    return report_invalid_anchor(context, record->source_offset, record->source_length);
}

static void clear_from(g04_r1a_context *context, uint64_t from)
{
    if (context == NULL || from >= G04_R1A_FRAME_COUNT) return;
    uint64_t to = context->known_depth;
    if (to > G04_R1A_FRAME_COUNT) to = G04_R1A_FRAME_COUNT;
    for (uint64_t i = from; i < to; ++i) context->frames[i] = (g04_r1a_frame){0};
    if (context->known_depth > from) context->known_depth = from;
}

static arbor_status source_attribute(
    void *context_void,
    const arbor_view0_native_source_attribute_observation *observation)
{
    g04_r1a_context *context = (g04_r1a_context *)context_void;
    if (context == NULL || observation == NULL) return status_from_errno_value(EINVAL);
    if (observation->owner_standard_element_id != ARBOR_VIEW0_NATIVE_ELEMENT_OPTION)
        return ok_status();
    if (context->pending_option_source_offset != observation->owner_source_offset) {
        context->pending_option_source_offset = observation->owner_source_offset;
        context->pending_option_has_label = false;
        context->pending_option_has_value = false;
    }
    if (span_ascii_equals(observation->local_name, "label"))
        context->pending_option_has_label = true;
    else if (span_ascii_equals(observation->local_name, "value"))
        context->pending_option_has_value = true;
    return ok_status();
}

static arbor_status source_text(
    void *context_void,
    const arbor_view0_native_source_text_observation *observation)
{
    g04_r1a_context *context = (g04_r1a_context *)context_void;
    if (context == NULL || observation == NULL) return status_from_errno_value(EINVAL);
    if (text_is_inter_element_whitespace(observation->text)) return ok_status();
    const uint64_t depth = observation->initial_open_elements_depth;
    if (!frame_matches_current(context, depth, observation->initial_current_source_offset))
        return ok_status();
    g04_r1a_frame *current = &context->frames[depth - 1u];
    if ((current->flags & (G04_R1A_FRAME_TRANSPARENT | G04_R1A_FRAME_BLOCKED)) !=
        G04_R1A_FRAME_TRANSPARENT) return ok_status();
    const g04_r1a_model model = (g04_r1a_model)current->effective_model;
    if (model != G04_R1A_MODEL_SELECT_TAIL && model != G04_R1A_MODEL_OPTGROUP_TAIL)
        return ok_status();
    if ((current->flags & G04_R1A_FRAME_TEXT_REPORTED) != 0u) return ok_status();
    arbor_status status = report_invalid_anchor(context, current->source_offset, current->source_length);
    if (status.native != 0) return status;
    if (context->select_text_violation_count == UINT64_MAX)
        return status_from_errno_value(EOVERFLOW);
    context->select_text_violation_count += 1u;
    current->flags |= G04_R1A_FRAME_TEXT_REPORTED;
    return ok_status();
}

static arbor_status source_repair(
    void *context_void,
    const arbor_view0_native_source_repair_context *record)
{
    g04_r1a_context *context = (g04_r1a_context *)context_void;
    if (context == NULL || record == NULL) return status_from_errno_value(EINVAL);
    if ((record->insertion_flags & ~ARBOR_VIEW0_NATIVE_SOURCE_REPAIR_FLAG_FOSTER_PARENTING) != 0u ||
        record->initial_open_elements_depth >= G04_R1A_FRAME_COUNT ||
        record->insertion_open_elements_depth >= G04_R1A_FRAME_COUNT)
        return status_from_errno_value(EIO);

    /* Reconcile our bounded authored-frame mirror to the parser stack before this token. */
    if (context->known_depth > record->initial_open_elements_depth)
        clear_from(context, record->initial_open_elements_depth);
    else if (context->known_depth < record->initial_open_elements_depth)
        context->known_depth = record->initial_open_elements_depth;

    bool parent_transparent = false;
    g04_r1a_model parent_model = G04_R1A_MODEL_UNKNOWN;
    uint64_t parent_id = record->initial_current_standard_element_id;
    if (record->initial_open_elements_depth != 0u &&
        frame_matches_current(context, record->initial_open_elements_depth,
                              record->initial_current_source_offset)) {
        const g04_r1a_frame *parent =
            &context->frames[record->initial_open_elements_depth - 1u];
        parent_id = parent->standard_element_id;
        if ((parent->flags & G04_R1A_FRAME_TRANSPARENT) != 0u) {
            parent_transparent = true;
            if ((parent->flags & G04_R1A_FRAME_BLOCKED) == 0u)
                parent_model = (g04_r1a_model)parent->effective_model;
        }
    }

    if (parent_transparent && parent_model != G04_R1A_MODEL_UNKNOWN) {
        const bool media_prefix =
            (parent_id == ARBOR_VIEW0_NATIVE_ELEMENT_VIDEO ||
             parent_id == ARBOR_VIEW0_NATIVE_ELEMENT_AUDIO) &&
            (record->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_SOURCE ||
             record->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_TRACK);
        if (!media_prefix && !model_allows_element(parent_model, record->standard_element_id)) {
            if (prior_owner_at_offset(context, record->source_offset)) {
                if (context->prior_owner_suppression_count == UINT64_MAX)
                    return status_from_errno_value(EOVERFLOW);
                context->prior_owner_suppression_count += 1u;
            } else {
                arbor_status status = report_invalid(context, record);
                if (status.native != 0) return status;
            }
        }
    }

    const bool transparent = element_is_transparent(context, record);
    const bool blocked = prior_owner_at_offset(context, record->source_offset);
    const g04_r1a_model model = transparent && !blocked
        ? containing_model(context, record)
        : G04_R1A_MODEL_UNKNOWN;

    if (record->insertion_seen == 1u) {
        clear_from(context, record->insertion_open_elements_depth);
        const uint64_t slot = record->insertion_open_elements_depth;
        uint32_t option_flags = 0u;
        if (record->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_OPTION) {
            if (context->pending_option_source_offset == record->source_offset) {
                if (context->pending_option_has_label) option_flags |= G04_R1A_FRAME_OPTION_LABEL;
                if (context->pending_option_has_value) option_flags |= G04_R1A_FRAME_OPTION_VALUE;
            }
            if (datalist_ancestor(context, record->initial_open_elements_depth))
                option_flags |= G04_R1A_FRAME_OPTION_DATALIST;
        }
        context->frames[slot] = (g04_r1a_frame){
            .standard_element_id = record->standard_element_id,
            .source_offset = record->source_offset,
            .source_length = record->source_length,
            .effective_model = (uint32_t)model,
            .flags = G04_R1A_FRAME_VALID | option_flags |
                (transparent ? G04_R1A_FRAME_TRANSPARENT : 0u) |
                (blocked || (transparent && model == G04_R1A_MODEL_UNKNOWN)
                    ? G04_R1A_FRAME_BLOCKED : 0u)
        };
        context->known_depth = slot + 1u;
    }
    return ok_status();
}

static arbor_status element_begin(
    void *context_void,
    const arbor_view0_native_element_observation *observation)
{
    g04_r1a_context *context = (g04_r1a_context *)context_void;
    if (context == NULL || observation == NULL) return status_from_errno_value(EINVAL);
    if (observation->namespace_id == ARBOR_VIEW0_NATIVE_NAMESPACE_HTML &&
        observation->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_NONE &&
        (observation->flags & ARBOR_VIEW0_NATIVE_ELEMENT_FLAG_SYNTHETIC) == 0u &&
        span_has_ascii_hyphen(observation->local_name)) {
        if (context->g13_custom_deferred_count == UINT64_MAX)
            return status_from_errno_value(EOVERFLOW);
        context->g13_custom_deferred_count += 1u;
        context->deferred_flags |= ARBOR_VIEW0_NATIVE_RESULT_FLAG_G04_R1_DEFERRED_G13_CUSTOM;
    }
    return ok_status();
}

static G04_R1A_NOINLINE arbor_status collect_prior_error_anchors(
    arbor_span input,
    arbor_view0_native_source_anchor *anchors,
    uint64_t *count_out)
{
    if (anchors == NULL || count_out == NULL) return status_from_errno_value(EINVAL);
    uint64_t total = 0u;

    arbor_view0_native_g03_r1a_evaluation r1m = {0};
    arbor_status status = arbor_view0_native_g03_r1a_measure(input, &r1m);
    if (status.native != 0) return status;
    if (r1m.diagnostic_count > G04_R1A_PRIOR_ANCHOR_CAPACITY - total)
        return status_from_errno_value(E2BIG);
    arbor_view0_native_g03_r1a_evaluation r1c = {0};
    status = arbor_view0_native_g03_r1a_collect_anchors(
        input, anchors + total, r1m.diagnostic_count, &r1c);
    if (status.native != 0) return status;
    if (r1c.diagnostic_count != r1m.diagnostic_count ||
        r1c.deferred_main_form_count != r1m.deferred_main_form_count)
        return status_from_errno_value(EIO);
    total += r1c.diagnostic_count;

#define COLLECT_ANCHORS_WITH_FLAGS(RULE, TYPE) do { \
    TYPE m = {0}; \
    arbor_status s = arbor_view0_native_g03_##RULE##_measure(input, &m); \
    if (s.native != 0) return s; \
    if (m.diagnostic_count > G04_R1A_PRIOR_ANCHOR_CAPACITY - total) \
        return status_from_errno_value(E2BIG); \
    TYPE c = {0}; \
    s = arbor_view0_native_g03_##RULE##_collect_anchors( \
        input, anchors + total, m.diagnostic_count, &c); \
    if (s.native != 0) return s; \
    if (c.diagnostic_count != m.diagnostic_count || \
        c.deferred_flags != m.deferred_flags) return status_from_errno_value(EIO); \
    total += c.diagnostic_count; \
} while (0)
    COLLECT_ANCHORS_WITH_FLAGS(r2a, arbor_view0_native_g03_r2a_evaluation);
    COLLECT_ANCHORS_WITH_FLAGS(r3a, arbor_view0_native_g03_r3a_evaluation);
    COLLECT_ANCHORS_WITH_FLAGS(r4a, arbor_view0_native_g03_r4a_evaluation);
#undef COLLECT_ANCHORS_WITH_FLAGS

    arbor_view0_native_g03_r5a_evaluation r5m = {0};
    status = arbor_view0_native_g03_r5a_measure(input, &r5m);
    if (status.native != 0) return status;
    if (r5m.diagnostic_count > G04_R1A_PRIOR_ANCHOR_CAPACITY - total)
        return status_from_errno_value(E2BIG);
    arbor_view0_native_g03_r5a_evaluation r5c = {0};
    status = arbor_view0_native_g03_r5a_collect_anchors(
        input, anchors + total, r5m.diagnostic_count, &r5c);
    if (status.native != 0) return status;
    if (r5c.diagnostic_count != r5m.diagnostic_count ||
        r5c.prior_owner_suppression_count != r5m.prior_owner_suppression_count)
        return status_from_errno_value(EIO);
    total += r5c.diagnostic_count;
    *count_out = total;
    return ok_status();
}

static G04_R1A_NOINLINE arbor_status evaluate_with_prior_anchors(
    arbor_span input,
    const arbor_view0_native_source_anchor *prior_anchors,
    uint64_t prior_anchor_count,
    arbor_view0_native_diagnostic *diagnostics,
    arbor_view0_native_source_anchor *anchors,
    uint64_t output_capacity,
    uint64_t discovery_sequence_base,
    bool publish,
    bool collect_anchors,
    arbor_view0_native_g04_r1a_evaluation *evaluation_out)
{
    if (evaluation_out == NULL || (publish && collect_anchors) ||
        (prior_anchor_count != 0u && prior_anchors == NULL) ||
        prior_anchor_count > G04_R1A_PRIOR_ANCHOR_CAPACITY ||
        (publish && output_capacity != 0u && diagnostics == NULL) ||
        (collect_anchors && output_capacity != 0u && anchors == NULL))
        return status_from_errno_value(EINVAL);

    g04_r1a_context context = {
        .output = {.diagnostics = diagnostics},
        .output_capacity = output_capacity,
        .discovery_sequence_base = discovery_sequence_base,
        .prior_error_anchors = prior_anchors,
        .prior_error_anchor_count = prior_anchor_count,
        .publish = publish,
        .collect_anchors = collect_anchors
    };
    if (collect_anchors) context.output.anchors = anchors;
    const arbor_view0_native_semantic_observer observer = {
        .context = &context,
        .element_begin = element_begin,
        .source_repair = source_repair,
        .source_attribute = source_attribute,
        .source_text = source_text
    };
    arbor_view0_native_parse_counts parse_counts = {0};
    arbor_view0_native_document_facts facts = {0};
    arbor_view0_native_observation_counts observations = {0};
    arbor_status status = arbor_view0_native_lexbor_observe(
        input, &observer, &parse_counts, &facts, &observations);
    if (status.native != 0) return status;
    if ((publish || collect_anchors) && context.diagnostic_count != output_capacity)
        return status_from_errno_value(EIO);

    *evaluation_out = (arbor_view0_native_g04_r1a_evaluation){
        .diagnostic_count = context.diagnostic_count,
        .prior_owner_suppression_count = context.prior_owner_suppression_count,
        .deferred_flags = context.deferred_flags,
        .noscript_deferred_count = context.noscript_deferred_count,
        .noscript_resolved_count = context.noscript_resolved_count,
        .option_branch_deferred_count = context.option_branch_deferred_count,
        .option_branch_resolved_count = context.option_branch_resolved_count,
        .select_text_violation_count = context.select_text_violation_count,
        .g13_custom_deferred_count = context.g13_custom_deferred_count
    };
    return ok_status();
}

static G04_R1A_NOINLINE arbor_status evaluate(
    arbor_span input,
    arbor_view0_native_diagnostic *diagnostics,
    arbor_view0_native_source_anchor *anchors,
    uint64_t output_capacity,
    uint64_t discovery_sequence_base,
    bool publish,
    bool collect_anchors,
    arbor_view0_native_g04_r1a_evaluation *evaluation_out)
{
    if (evaluation_out == NULL || (publish && collect_anchors) ||
        (publish && output_capacity != 0u && diagnostics == NULL) ||
        (collect_anchors && output_capacity != 0u && anchors == NULL))
        return status_from_errno_value(EINVAL);

    arbor_view0_native_source_anchor prior_anchors[ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS] = {{0}};
    uint64_t prior_count = 0u;
    arbor_status status = collect_prior_error_anchors(input, prior_anchors, &prior_count);
    if (status.native != 0) return status;

    return evaluate_with_prior_anchors(
        input, prior_anchors, prior_count, diagnostics, anchors, output_capacity,
        discovery_sequence_base, publish, collect_anchors, evaluation_out);
}

arbor_status arbor_view0_native_g04_r1a_measure(
    arbor_span input, arbor_view0_native_g04_r1a_evaluation *evaluation_out)
{
    return evaluate(input, NULL, NULL, 0u, 0u, false, false, evaluation_out);
}

arbor_status arbor_view0_native_g04_r1a_collect(
    arbor_span input,
    arbor_view0_native_diagnostic *diagnostics,
    uint64_t diagnostic_capacity,
    uint64_t discovery_sequence_base,
    arbor_view0_native_g04_r1a_evaluation *evaluation_out)
{
    return evaluate(input, diagnostics, NULL, diagnostic_capacity,
                    discovery_sequence_base, true, false, evaluation_out);
}

arbor_status arbor_view0_native_g04_r1a_collect_anchors(
    arbor_span input,
    arbor_view0_native_source_anchor *anchors,
    uint64_t anchor_capacity,
    arbor_view0_native_g04_r1a_evaluation *evaluation_out)
{
    return evaluate(input, NULL, anchors, anchor_capacity, 0u,
                    false, true, evaluation_out);
}
