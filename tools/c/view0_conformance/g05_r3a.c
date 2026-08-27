#include "g05_r3a.h"
#include "g05_c0.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define G05_R3A_MAX_DEPTH UINT64_C(4097)
#define G05_R3A_DEPTH_SLOTS UINT64_C(4098)
#define G05_R3_ATTR_ACCEPT (UINT64_C(1) << 0)
#define G05_R3_ATTR_ALPHA (UINT64_C(1) << 1)
#define G05_R3_ATTR_ALT (UINT64_C(1) << 2)
#define G05_R3_ATTR_AS (UINT64_C(1) << 3)
#define G05_R3_ATTR_AUTOCOMPLETE (UINT64_C(1) << 4)
#define G05_R3_ATTR_CHARSET (UINT64_C(1) << 5)
#define G05_R3_ATTR_CHECKED (UINT64_C(1) << 6)
#define G05_R3_ATTR_COLOR (UINT64_C(1) << 7)
#define G05_R3_ATTR_COLORSPACE (UINT64_C(1) << 8)
#define G05_R3_ATTR_COLS (UINT64_C(1) << 9)
#define G05_R3_ATTR_CONTROLS (UINT64_C(1) << 10)
#define G05_R3_ATTR_DIRNAME (UINT64_C(1) << 11)
#define G05_R3_ATTR_DISABLED (UINT64_C(1) << 12)
#define G05_R3_ATTR_DOWNLOAD (UINT64_C(1) << 13)
#define G05_R3_ATTR_FORMACTION (UINT64_C(1) << 14)
#define G05_R3_ATTR_FORMENCTYPE (UINT64_C(1) << 15)
#define G05_R3_ATTR_FORMMETHOD (UINT64_C(1) << 16)
#define G05_R3_ATTR_FORMNOVALIDATE (UINT64_C(1) << 17)
#define G05_R3_ATTR_FORMTARGET (UINT64_C(1) << 18)
#define G05_R3_ATTR_HEIGHT (UINT64_C(1) << 19)
#define G05_R3_ATTR_HREF (UINT64_C(1) << 20)
#define G05_R3_ATTR_HREFLANG (UINT64_C(1) << 21)
#define G05_R3_ATTR_HTTP_EQUIV (UINT64_C(1) << 22)
#define G05_R3_ATTR_IMAGESIZES (UINT64_C(1) << 23)
#define G05_R3_ATTR_IMAGESRCSET (UINT64_C(1) << 24)
#define G05_R3_ATTR_INTEGRITY (UINT64_C(1) << 25)
#define G05_R3_ATTR_ISMAP (UINT64_C(1) << 26)
#define G05_R3_ATTR_ITEMPROP (UINT64_C(1) << 27)
#define G05_R3_ATTR_LABEL (UINT64_C(1) << 28)
#define G05_R3_ATTR_LIST (UINT64_C(1) << 29)
#define G05_R3_ATTR_MAX (UINT64_C(1) << 30)
#define G05_R3_ATTR_MAXLENGTH (UINT64_C(1) << 31)
#define G05_R3_ATTR_MIN (UINT64_C(1) << 32)
#define G05_R3_ATTR_MINLENGTH (UINT64_C(1) << 33)
#define G05_R3_ATTR_MULTIPLE (UINT64_C(1) << 34)
#define G05_R3_ATTR_NAME (UINT64_C(1) << 35)
#define G05_R3_ATTR_PATTERN (UINT64_C(1) << 36)
#define G05_R3_ATTR_PING (UINT64_C(1) << 37)
#define G05_R3_ATTR_PLACEHOLDER (UINT64_C(1) << 38)
#define G05_R3_ATTR_POPOVERTARGET (UINT64_C(1) << 39)
#define G05_R3_ATTR_POPOVERTARGETACTION (UINT64_C(1) << 40)
#define G05_R3_ATTR_READONLY (UINT64_C(1) << 41)
#define G05_R3_ATTR_REFERRERPOLICY (UINT64_C(1) << 42)
#define G05_R3_ATTR_REL (UINT64_C(1) << 43)
#define G05_R3_ATTR_REQUIRED (UINT64_C(1) << 44)
#define G05_R3_ATTR_SIZE (UINT64_C(1) << 45)
#define G05_R3_ATTR_SIZES (UINT64_C(1) << 46)
#define G05_R3_ATTR_SRC (UINT64_C(1) << 47)
#define G05_R3_ATTR_STEP (UINT64_C(1) << 48)
#define G05_R3_ATTR_TABINDEX (UINT64_C(1) << 49)
#define G05_R3_ATTR_TARGET (UINT64_C(1) << 50)
#define G05_R3_ATTR_TYPE (UINT64_C(1) << 51)
#define G05_R3_ATTR_VALUE (UINT64_C(1) << 52)
#define G05_R3_ATTR_WIDTH (UINT64_C(1) << 53)
#define G05_R3_ATTR_WRAP (UINT64_C(1) << 54)

#define G05_R3_REL_STYLESHEET UINT64_C(0x01)
#define G05_R3_REL_PRELOAD UINT64_C(0x02)
#define G05_R3_REL_MODULEPRELOAD UINT64_C(0x04)
#define G05_R3_REL_ICON UINT64_C(0x08)
#define G05_R3_REL_APPLE_TOUCH_ICON UINT64_C(0x10)
#define G05_R3_REL_MASK_ICON UINT64_C(0x20)

typedef struct g05_r3_name_bit {
    const char *name;
    uint64_t bit;
} g05_r3_name_bit;

static const g05_r3_name_bit g_attr_names[] = {
    {"accept", G05_R3_ATTR_ACCEPT},
    {"alpha", G05_R3_ATTR_ALPHA},
    {"alt", G05_R3_ATTR_ALT},
    {"as", G05_R3_ATTR_AS},
    {"autocomplete", G05_R3_ATTR_AUTOCOMPLETE},
    {"charset", G05_R3_ATTR_CHARSET},
    {"checked", G05_R3_ATTR_CHECKED},
    {"color", G05_R3_ATTR_COLOR},
    {"colorspace", G05_R3_ATTR_COLORSPACE},
    {"cols", G05_R3_ATTR_COLS},
    {"controls", G05_R3_ATTR_CONTROLS},
    {"dirname", G05_R3_ATTR_DIRNAME},
    {"disabled", G05_R3_ATTR_DISABLED},
    {"download", G05_R3_ATTR_DOWNLOAD},
    {"formaction", G05_R3_ATTR_FORMACTION},
    {"formenctype", G05_R3_ATTR_FORMENCTYPE},
    {"formmethod", G05_R3_ATTR_FORMMETHOD},
    {"formnovalidate", G05_R3_ATTR_FORMNOVALIDATE},
    {"formtarget", G05_R3_ATTR_FORMTARGET},
    {"height", G05_R3_ATTR_HEIGHT},
    {"href", G05_R3_ATTR_HREF},
    {"hreflang", G05_R3_ATTR_HREFLANG},
    {"http-equiv", G05_R3_ATTR_HTTP_EQUIV},
    {"imagesizes", G05_R3_ATTR_IMAGESIZES},
    {"imagesrcset", G05_R3_ATTR_IMAGESRCSET},
    {"integrity", G05_R3_ATTR_INTEGRITY},
    {"ismap", G05_R3_ATTR_ISMAP},
    {"itemprop", G05_R3_ATTR_ITEMPROP},
    {"label", G05_R3_ATTR_LABEL},
    {"list", G05_R3_ATTR_LIST},
    {"max", G05_R3_ATTR_MAX},
    {"maxlength", G05_R3_ATTR_MAXLENGTH},
    {"min", G05_R3_ATTR_MIN},
    {"minlength", G05_R3_ATTR_MINLENGTH},
    {"multiple", G05_R3_ATTR_MULTIPLE},
    {"name", G05_R3_ATTR_NAME},
    {"pattern", G05_R3_ATTR_PATTERN},
    {"ping", G05_R3_ATTR_PING},
    {"placeholder", G05_R3_ATTR_PLACEHOLDER},
    {"popovertarget", G05_R3_ATTR_POPOVERTARGET},
    {"popovertargetaction", G05_R3_ATTR_POPOVERTARGETACTION},
    {"readonly", G05_R3_ATTR_READONLY},
    {"referrerpolicy", G05_R3_ATTR_REFERRERPOLICY},
    {"rel", G05_R3_ATTR_REL},
    {"required", G05_R3_ATTR_REQUIRED},
    {"size", G05_R3_ATTR_SIZE},
    {"sizes", G05_R3_ATTR_SIZES},
    {"src", G05_R3_ATTR_SRC},
    {"step", G05_R3_ATTR_STEP},
    {"tabindex", G05_R3_ATTR_TABINDEX},
    {"target", G05_R3_ATTR_TARGET},
    {"type", G05_R3_ATTR_TYPE},
    {"value", G05_R3_ATTR_VALUE},
    {"width", G05_R3_ATTR_WIDTH},
    {"wrap", G05_R3_ATTR_WRAP},
};

typedef struct g05_r3_input_forbidden {
    uint64_t state_mask;
    uint64_t forbidden_attr_mask;
    uint64_t clause_ordinal;
} g05_r3_input_forbidden;

static const g05_r3_input_forbidden g_input_forbidden[] = {
    {ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_HIDDEN, UINT64_C(0x0021b3d7e00fc147), UINT64_C(17)},
    {(ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TEXT | ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_SEARCH), UINT64_C(0x00218185400fc147), UINT64_C(18)},
    {ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TEL, UINT64_C(0x00218185400fc147), UINT64_C(19)},
    {ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_URL, UINT64_C(0x00218185400fc147), UINT64_C(20)},
    {ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_EMAIL, UINT64_C(0x00218181400fc147), UINT64_C(21)},
    {ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_PASSWORD, UINT64_C(0x00218185600fc147), UINT64_C(22)},
    {ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_DATE, UINT64_C(0x0020a1d6800fc947), UINT64_C(23)},
    {ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_MONTH, UINT64_C(0x0020a1d6800fc947), UINT64_C(24)},
    {ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_WEEK, UINT64_C(0x0020a1d6800fc947), UINT64_C(25)},
    {ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TIME, UINT64_C(0x0020a1d6800fc947), UINT64_C(26)},
    {ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_DATETIME_LOCAL, UINT64_C(0x0020a1d6800fc947), UINT64_C(27)},
    {ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_NUMBER, UINT64_C(0x0020a196800fc947), UINT64_C(28)},
    {ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_RANGE, UINT64_C(0x0020b3d6800fc947), UINT64_C(29)},
    {ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_COLOR, UINT64_C(0x0021b3d7c00fc845), UINT64_C(30)},
    {ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_CHECKBOX, UINT64_C(0x0021a3d7e00fc917), UINT64_C(31)},
    {ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_RADIO, UINT64_C(0x0021a3d7e00fc917), UINT64_C(32)},
    {ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_FILE, UINT64_C(0x0021a3d3e00fc956), UINT64_C(33)},
    {ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_SUBMIT, UINT64_C(0x0021b257e0080157), UINT64_C(35)},
    {ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_IMAGE, UINT64_C(0x00013257e0000953), UINT64_C(36)},
    {ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_RESET, UINT64_C(0x0021b257e00fc957), UINT64_C(38)},
    {ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_BUTTON, UINT64_C(0x0021b257e00fc957), UINT64_C(39)},
};

typedef struct g05_r3_source_anchor_record {
    uint64_t owner_source_offset;
    uint64_t attr_bit;
    uint32_t source_offset;
    uint32_t source_length;
} g05_r3_source_anchor_record;

typedef struct g05_r3_current_element {
    uint64_t standard_element_id;
    uint64_t source_offset;
    uint64_t source_length;
    uint64_t depth;
    uint64_t attr_bits;
    uint64_t rel_tokens;
    uint64_t input_state;
    bool as_image;
    bool name_charset;
    bool alt_nonempty;
    bool wrap_hard;
    bool has_child_legend;
    bool authored;
} g05_r3_current_element;

typedef struct g05_r3_context {
    arbor_view0_native_source_anchor *anchors;
    uint64_t anchor_capacity;
    uint64_t diagnostic_count;
    uint64_t predicate_evaluation_count;
    uint64_t input_element_count;
    uint64_t missing_required_count;
    uint64_t forbidden_present_count;
    uint64_t clause_violation_count[43];
    bool collect_anchors;

    g05_r3_source_anchor_record source_attrs[ARBOR_VIEW0_NATIVE_G05_R3A_MAX_TRACKED_SOURCE_ATTRIBUTES];
    uint64_t source_attr_count;

    g05_r3_current_element current;
    bool a_href_stack[G05_R3A_DEPTH_SLOTS];
    uint64_t a_href_ancestor_count;
} g05_r3_context;

_Static_assert(sizeof("ARBOR_VIEW_V1_G05_CONDITIONAL_ATTRIBUTE_APPLICABILITY") <=
                   ARBOR_VIEW0_NATIVE_RULE_SYMBOL_CAP,
               "G05 R3 symbolic name exceeds diagnostic capacity");
_Static_assert(sizeof("Attribute conditional applicability requirement is not satisfied by the frozen predicate table") <=
                   ARBOR_VIEW0_NATIVE_MESSAGE_CAP,
               "G05 R3 message exceeds diagnostic capacity");
_Static_assert(sizeof(g_attr_names) / sizeof(g_attr_names[0]) == 55u,
               "G05 R3 conditional attribute-name catalog drift");
_Static_assert(sizeof(g_input_forbidden) / sizeof(g_input_forbidden[0]) == 21u,
               "G05 R3 input forbidden-state table drift");
_Static_assert(G05_R3A_DEPTH_SLOTS == G05_R3A_MAX_DEPTH + UINT64_C(1),
               "G05 R3 depth workspace drift");

static arbor_status err_status(int e) { return arbor_status_from_native(-(int64_t)e); }
static arbor_status ok_status(void) { return arbor_status_from_native(0); }

static uint8_t ascii_lower(uint8_t c)
{
    return c >= (uint8_t)'A' && c <= (uint8_t)'Z'
        ? (uint8_t)(c + ((uint8_t)'a' - (uint8_t)'A'))
        : c;
}

static bool ascii_space(uint8_t c)
{
    return c == UINT8_C(0x09) || c == UINT8_C(0x0a) || c == UINT8_C(0x0c) ||
           c == UINT8_C(0x0d) || c == UINT8_C(0x20);
}

static bool span_eq_literal(arbor_span span, const char *literal)
{
    const size_t n = strlen(literal);
    return span.data != NULL && span.length == (uint64_t)n &&
           memcmp(span.data, literal, n) == 0;
}

static bool span_eq_ci_literal(arbor_span span, const char *literal)
{
    const size_t n = strlen(literal);
    if (span.data == NULL || span.length != (uint64_t)n) return false;
    for (size_t i = 0u; i < n; ++i) {
        if (ascii_lower(span.data[i]) != ascii_lower((uint8_t)literal[i])) return false;
    }
    return true;
}

static uint64_t attr_bit(arbor_span name)
{
    for (size_t i = 0u; i < sizeof(g_attr_names) / sizeof(g_attr_names[0]); ++i) {
        if (span_eq_literal(name, g_attr_names[i].name)) return g_attr_names[i].bit;
    }
    return UINT64_C(0);
}

static bool token_ci_equal(const uint8_t *data, uint64_t length, const char *literal)
{
    arbor_span span = {data, length};
    return span_eq_ci_literal(span, literal);
}

static uint64_t rel_tokens(arbor_span value)
{
    uint64_t result = 0u;
    uint64_t i = 0u;
    while (i < value.length) {
        while (i < value.length && ascii_space(value.data[i])) ++i;
        const uint64_t start = i;
        while (i < value.length && !ascii_space(value.data[i])) ++i;
        const uint64_t length = i - start;
        if (length == 0u) continue;
        const uint8_t *data = value.data + start;
        if (token_ci_equal(data, length, "stylesheet")) result |= G05_R3_REL_STYLESHEET;
        else if (token_ci_equal(data, length, "preload")) result |= G05_R3_REL_PRELOAD;
        else if (token_ci_equal(data, length, "modulepreload")) result |= G05_R3_REL_MODULEPRELOAD;
        else if (token_ci_equal(data, length, "icon")) result |= G05_R3_REL_ICON;
        else if (token_ci_equal(data, length, "apple-touch-icon")) result |= G05_R3_REL_APPLE_TOUCH_ICON;
        else if (token_ci_equal(data, length, "mask-icon")) result |= G05_R3_REL_MASK_ICON;
    }
    return result;
}

static bool relevant_owner(uint64_t id)
{
    return id == ARBOR_VIEW0_NATIVE_ELEMENT_BASE ||
           id == ARBOR_VIEW0_NATIVE_ELEMENT_LINK ||
           id == ARBOR_VIEW0_NATIVE_ELEMENT_META ||
           id == ARBOR_VIEW0_NATIVE_ELEMENT_A ||
           id == ARBOR_VIEW0_NATIVE_ELEMENT_IMG ||
           id == ARBOR_VIEW0_NATIVE_ELEMENT_AREA ||
           id == ARBOR_VIEW0_NATIVE_ELEMENT_INPUT ||
           id == ARBOR_VIEW0_NATIVE_ELEMENT_OPTGROUP ||
           id == ARBOR_VIEW0_NATIVE_ELEMENT_TEXTAREA ||
           id == ARBOR_VIEW0_NATIVE_ELEMENT_METER ||
           id == ARBOR_VIEW0_NATIVE_ELEMENT_DIALOG;
}

static arbor_status source_attribute(
    void *opaque,
    const arbor_view0_native_source_attribute_observation *observation)
{
    if (opaque == NULL || observation == NULL) return err_status(EINVAL);
    g05_r3_context *context = (g05_r3_context *)opaque;
    if (!relevant_owner(observation->owner_standard_element_id)) return ok_status();

    const uint64_t bit = attr_bit(observation->local_name);
    if (bit == 0u) return ok_status();
    if (observation->source_offset == ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE ||
        observation->source_length == 0u ||
        observation->source_offset > UINT32_MAX ||
        observation->source_length > UINT32_MAX)
        return err_status(EIO);
    if (context->source_attr_count >= ARBOR_VIEW0_NATIVE_G05_R3A_MAX_TRACKED_SOURCE_ATTRIBUTES)
        return err_status(E2BIG);
    if (context->source_attr_count != 0u &&
        observation->owner_source_offset <
            context->source_attrs[context->source_attr_count - 1u].owner_source_offset)
        return err_status(EIO);

    context->source_attrs[context->source_attr_count++] = (g05_r3_source_anchor_record){
        .owner_source_offset = observation->owner_source_offset,
        .attr_bit = bit,
        .source_offset = (uint32_t)observation->source_offset,
        .source_length = (uint32_t)observation->source_length
    };
    return ok_status();
}

static bool source_anchor_lookup(
    const g05_r3_context *context,
    uint64_t owner_source_offset,
    uint64_t bit,
    arbor_view0_native_source_anchor *anchor_out)
{
    if (context == NULL || anchor_out == NULL || bit == 0u) return false;
    uint64_t lo = 0u, hi = context->source_attr_count;
    while (lo < hi) {
        const uint64_t mid = lo + (hi - lo) / 2u;
        if (context->source_attrs[mid].owner_source_offset < owner_source_offset)
            lo = mid + 1u;
        else
            hi = mid;
    }
    for (uint64_t i = lo;
         i < context->source_attr_count &&
         context->source_attrs[i].owner_source_offset == owner_source_offset;
         ++i) {
        if (context->source_attrs[i].attr_bit == bit) {
            *anchor_out = (arbor_view0_native_source_anchor){
                .byte_offset = context->source_attrs[i].source_offset,
                .source_length = context->source_attrs[i].source_length
            };
            return true;
        }
    }
    return false;
}

static bool latest_source_anchor(
    const g05_r3_context *context,
    uint64_t owner_source_offset,
    uint64_t bits,
    arbor_view0_native_source_anchor *anchor_out)
{
    if (context == NULL || anchor_out == NULL || bits == 0u) return false;
    bool found = false;
    arbor_view0_native_source_anchor best = {0};
    uint64_t lo = 0u, hi = context->source_attr_count;
    while (lo < hi) {
        const uint64_t mid = lo + (hi - lo) / 2u;
        if (context->source_attrs[mid].owner_source_offset < owner_source_offset)
            lo = mid + 1u;
        else
            hi = mid;
    }
    for (uint64_t i = lo;
         i < context->source_attr_count &&
         context->source_attrs[i].owner_source_offset == owner_source_offset;
         ++i) {
        if ((context->source_attrs[i].attr_bit & bits) != 0u &&
            (!found || context->source_attrs[i].source_offset > best.byte_offset)) {
            best.byte_offset = context->source_attrs[i].source_offset;
            best.source_length = context->source_attrs[i].source_length;
            found = true;
        }
    }
    if (found) *anchor_out = best;
    return found;
}

static arbor_status publish_violation(
    g05_r3_context *context,
    uint64_t clause_ordinal,
    arbor_view0_native_source_anchor anchor,
    bool missing_required)
{
    if (context == NULL || clause_ordinal == 0u ||
        clause_ordinal > ARBOR_VIEW0_NATIVE_G05_R3A_CLAUSE_COUNT ||
        anchor.source_length == 0u)
        return err_status(EINVAL);
    if (context->diagnostic_count == UINT64_MAX ||
        context->clause_violation_count[clause_ordinal - 1u] == UINT64_MAX)
        return err_status(EOVERFLOW);
    if (context->collect_anchors) {
        if (context->diagnostic_count >= context->anchor_capacity || context->anchors == NULL)
            return err_status(ENOSPC);
        context->anchors[context->diagnostic_count] = anchor;
    }
    context->diagnostic_count += 1u;
    context->clause_violation_count[clause_ordinal - 1u] += 1u;
    if (missing_required) {
        if (context->missing_required_count == UINT64_MAX) return err_status(EOVERFLOW);
        context->missing_required_count += 1u;
    } else {
        if (context->forbidden_present_count == UINT64_MAX) return err_status(EOVERFLOW);
        context->forbidden_present_count += 1u;
    }
    return ok_status();
}

static arbor_status emit_owner(g05_r3_context *context, uint64_t clause_ordinal)
{
    if (context->current.source_offset == ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE ||
        context->current.source_length == 0u ||
        context->current.source_offset > UINT32_MAX ||
        context->current.source_length > UINT32_MAX)
        return err_status(EIO);
    const arbor_view0_native_source_anchor anchor = {
        .byte_offset = (uint32_t)context->current.source_offset,
        .source_length = (uint32_t)context->current.source_length
    };
    return publish_violation(context, clause_ordinal, anchor, true);
}

static arbor_status emit_attr(g05_r3_context *context, uint64_t clause_ordinal, uint64_t bit)
{
    arbor_view0_native_source_anchor anchor = {0};
    if (!source_anchor_lookup(context, context->current.source_offset, bit, &anchor))
        return err_status(EIO);
    return publish_violation(context, clause_ordinal, anchor, false);
}

static arbor_status emit_latest(
    g05_r3_context *context,
    uint64_t clause_ordinal,
    uint64_t bits)
{
    arbor_view0_native_source_anchor anchor = {0};
    if (!latest_source_anchor(context, context->current.source_offset, bits, &anchor))
        return err_status(EIO);
    return publish_violation(context, clause_ordinal, anchor, false);
}

static arbor_status traversal_enter(
    void *opaque,
    const arbor_view0_native_element_observation *observation)
{
    if (opaque == NULL || observation == NULL || observation->depth > G05_R3A_MAX_DEPTH)
        return err_status(EINVAL);
    g05_r3_context *context = (g05_r3_context *)opaque;
    context->a_href_stack[observation->depth] = false;
    context->current = (g05_r3_current_element){
        .standard_element_id = observation->standard_element_id,
        .source_offset = observation->source_offset,
        .source_length = observation->source_length,
        .depth = observation->depth,
        .input_state = ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TEXT,
        .authored = (observation->flags & ARBOR_VIEW0_NATIVE_ELEMENT_FLAG_SYNTHETIC) == 0u
    };
    return ok_status();
}

static arbor_status attribute(
    void *opaque,
    const arbor_view0_native_attribute_observation *observation)
{
    if (opaque == NULL || observation == NULL) return err_status(EINVAL);
    g05_r3_context *context = (g05_r3_context *)opaque;
    if (observation->owner_standard_element_id != context->current.standard_element_id)
        return err_status(EIO);
    const uint64_t bit = attr_bit(observation->local_name);
    context->current.attr_bits |= bit;
    if (bit == G05_R3_ATTR_REL) context->current.rel_tokens = rel_tokens(observation->value);
    if (bit == G05_R3_ATTR_AS) context->current.as_image = span_eq_ci_literal(observation->value, "image");
    if (bit == G05_R3_ATTR_NAME && context->current.standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_INPUT)
        context->current.name_charset = span_eq_ci_literal(observation->value, "_charset_");
    if (bit == G05_R3_ATTR_ALT && context->current.standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_IMG)
        context->current.alt_nonempty = observation->value.length != 0u;
    if (bit == G05_R3_ATTR_TYPE && context->current.standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_INPUT)
        context->current.input_state =
            arbor_view0_native_g05_c0_input_state_from_type(observation->value);
    if (bit == G05_R3_ATTR_WRAP && context->current.standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_TEXTAREA)
        context->current.wrap_hard = span_eq_ci_literal(observation->value, "hard");
    return ok_status();
}

static arbor_status direct_child(
    void *opaque,
    const arbor_view0_native_direct_child_observation *observation)
{
    if (opaque == NULL || observation == NULL) return err_status(EINVAL);
    g05_r3_context *context = (g05_r3_context *)opaque;
    if (context->current.standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_OPTGROUP &&
        observation->kind == ARBOR_VIEW0_NATIVE_DIRECT_CHILD_ELEMENT &&
        observation->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_LEGEND) {
        context->current.has_child_legend = true;
    }
    return ok_status();
}

static arbor_status count_predicates(g05_r3_context *context, uint64_t n)
{
    if (context->predicate_evaluation_count > UINT64_MAX - n) return err_status(EOVERFLOW);
    context->predicate_evaluation_count += n;
    return ok_status();
}

static arbor_status evaluate_link(g05_r3_context *context)
{
    arbor_status status = count_predicates(context, UINT64_C(8));
    if (status.native != 0) return status;
    const uint64_t b = context->current.attr_bits;
    const bool href = (b & G05_R3_ATTR_HREF) != 0u;
    const bool imagesrcset = (b & G05_R3_ATTR_IMAGESRCSET) != 0u;
    const bool rel = (b & G05_R3_ATTR_REL) != 0u;
    const bool itemprop = (b & G05_R3_ATTR_ITEMPROP) != 0u;
    if (!href && !imagesrcset) {
        status = emit_owner(context, UINT64_C(2)); if (status.native != 0) return status;
    }
    if (rel == itemprop) {
        if (rel) status = emit_latest(context, UINT64_C(3), G05_R3_ATTR_REL | G05_R3_ATTR_ITEMPROP);
        else status = emit_owner(context, UINT64_C(3));
        if (status.native != 0) return status;
    }
    if ((b & G05_R3_ATTR_INTEGRITY) != 0u &&
        (context->current.rel_tokens & (G05_R3_REL_STYLESHEET | G05_R3_REL_PRELOAD | G05_R3_REL_MODULEPRELOAD)) == 0u) {
        status = emit_attr(context, UINT64_C(4), G05_R3_ATTR_INTEGRITY); if (status.native != 0) return status;
    }
    const bool image_preload = (context->current.rel_tokens & G05_R3_REL_PRELOAD) != 0u &&
                               context->current.as_image;
    if (!image_preload && (b & G05_R3_ATTR_IMAGESRCSET) != 0u) {
        status = emit_attr(context, UINT64_C(5), G05_R3_ATTR_IMAGESRCSET); if (status.native != 0) return status;
    }
    if (!image_preload && (b & G05_R3_ATTR_IMAGESIZES) != 0u) {
        status = emit_attr(context, UINT64_C(5), G05_R3_ATTR_IMAGESIZES); if (status.native != 0) return status;
    }
    if ((b & G05_R3_ATTR_SIZES) != 0u &&
        (context->current.rel_tokens & (G05_R3_REL_ICON | G05_R3_REL_APPLE_TOUCH_ICON)) == 0u) {
        status = emit_attr(context, UINT64_C(6), G05_R3_ATTR_SIZES); if (status.native != 0) return status;
    }
    const bool preload = (context->current.rel_tokens & G05_R3_REL_PRELOAD) != 0u;
    const bool modulepreload = (context->current.rel_tokens & G05_R3_REL_MODULEPRELOAD) != 0u;
    const bool as_present = (b & G05_R3_ATTR_AS) != 0u;
    if (preload && !as_present) {
        if (rel) status = emit_attr(context, UINT64_C(7), G05_R3_ATTR_REL);
        else status = emit_owner(context, UINT64_C(7));
        if (status.native != 0) return status;
    } else if (!preload && !modulepreload && as_present) {
        status = emit_attr(context, UINT64_C(7), G05_R3_ATTR_AS); if (status.native != 0) return status;
    }
    if ((b & G05_R3_ATTR_COLOR) != 0u &&
        (context->current.rel_tokens & G05_R3_REL_MASK_ICON) == 0u) {
        status = emit_attr(context, UINT64_C(8), G05_R3_ATTR_COLOR); if (status.native != 0) return status;
    }
    if ((b & G05_R3_ATTR_DISABLED) != 0u &&
        (context->current.rel_tokens & G05_R3_REL_STYLESHEET) == 0u) {
        status = emit_attr(context, UINT64_C(9), G05_R3_ATTR_DISABLED); if (status.native != 0) return status;
    }
    return ok_status();
}

static arbor_status evaluate_input(g05_r3_context *context)
{
    if (context->input_element_count == UINT64_MAX) return err_status(EOVERFLOW);
    context->input_element_count += 1u;
    const uint64_t b = context->current.attr_bits;
    arbor_status status;
    if (context->current.input_state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_HIDDEN) {
        status = count_predicates(context, UINT64_C(2)); if (status.native != 0) return status;
        if (context->current.name_charset && (b & G05_R3_ATTR_VALUE) != 0u) {
            status = emit_attr(context, UINT64_C(16), G05_R3_ATTR_VALUE); if (status.native != 0) return status;
        }
    } else {
        status = count_predicates(context, UINT64_C(1)); if (status.native != 0) return status;
    }
    for (size_t i = 0u; i < sizeof(g_input_forbidden) / sizeof(g_input_forbidden[0]); ++i) {
        if ((g_input_forbidden[i].state_mask & context->current.input_state) == 0u) continue;
        uint64_t forbidden = b & g_input_forbidden[i].forbidden_attr_mask;
        while (forbidden != 0u) {
            const uint64_t bit = forbidden & (~forbidden + UINT64_C(1));
            status = emit_attr(context, g_input_forbidden[i].clause_ordinal, bit);
            if (status.native != 0) return status;
            forbidden &= ~bit;
        }
        break;
    }
    if (context->current.input_state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_FILE &&
        (b & G05_R3_ATTR_VALUE) != 0u) {
        status = count_predicates(context, UINT64_C(1)); if (status.native != 0) return status;
        status = emit_attr(context, UINT64_C(34), G05_R3_ATTR_VALUE); if (status.native != 0) return status;
    } else if (context->current.input_state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_IMAGE &&
               (b & G05_R3_ATTR_VALUE) != 0u) {
        status = count_predicates(context, UINT64_C(1)); if (status.native != 0) return status;
        status = emit_attr(context, UINT64_C(37), G05_R3_ATTR_VALUE); if (status.native != 0) return status;
    } else if (context->current.input_state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_FILE ||
               context->current.input_state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_IMAGE) {
        status = count_predicates(context, UINT64_C(1)); if (status.native != 0) return status;
    }
    return ok_status();
}

static arbor_status element_complete(
    void *opaque,
    const arbor_view0_native_element_observation *observation)
{
    if (opaque == NULL || observation == NULL) return err_status(EINVAL);
    g05_r3_context *context = (g05_r3_context *)opaque;
    if (observation->standard_element_id != context->current.standard_element_id ||
        observation->depth != context->current.depth)
        return err_status(EIO);

    arbor_status status = ok_status();
    if (context->current.authored && relevant_owner(context->current.standard_element_id)) {
        const uint64_t b = context->current.attr_bits;
        switch (context->current.standard_element_id) {
            case ARBOR_VIEW0_NATIVE_ELEMENT_BASE:
                status = count_predicates(context, UINT64_C(1));
                if (status.native == 0 && (b & (G05_R3_ATTR_HREF | G05_R3_ATTR_TARGET)) == 0u)
                    status = emit_owner(context, UINT64_C(1));
                break;
            case ARBOR_VIEW0_NATIVE_ELEMENT_LINK:
                status = evaluate_link(context);
                break;
            case ARBOR_VIEW0_NATIVE_ELEMENT_META: {
                status = count_predicates(context, UINT64_C(1));
                if (status.native != 0) break;
                const uint64_t exactly = b & (G05_R3_ATTR_NAME | G05_R3_ATTR_HTTP_EQUIV |
                                              G05_R3_ATTR_CHARSET | G05_R3_ATTR_ITEMPROP);
                unsigned count = 0u;
                for (uint64_t x = exactly; x != 0u; x &= x - UINT64_C(1)) ++count;
                if (count == 0u) status = emit_owner(context, UINT64_C(10));
                else if (count != 1u) status = emit_latest(context, UINT64_C(10), exactly);
                break;
            }
            case ARBOR_VIEW0_NATIVE_ELEMENT_A:
                status = count_predicates(context, UINT64_C(1));
                if (status.native == 0 && (b & G05_R3_ATTR_HREF) == 0u) {
                    uint64_t x = b & (G05_R3_ATTR_TARGET | G05_R3_ATTR_DOWNLOAD | G05_R3_ATTR_PING |
                                      G05_R3_ATTR_REL | G05_R3_ATTR_HREFLANG | G05_R3_ATTR_TYPE |
                                      G05_R3_ATTR_REFERRERPOLICY);
                    while (status.native == 0 && x != 0u) {
                        const uint64_t bit = x & (~x + UINT64_C(1));
                        status = emit_attr(context, UINT64_C(11), bit);
                        x &= ~bit;
                    }
                }
                break;
            case ARBOR_VIEW0_NATIVE_ELEMENT_IMG:
                status = count_predicates(context, UINT64_C(2));
                if (status.native == 0 && (b & G05_R3_ATTR_ISMAP) != 0u &&
                    context->a_href_ancestor_count == 0u)
                    status = emit_attr(context, UINT64_C(12), G05_R3_ATTR_ISMAP);
                if (status.native == 0 && (b & G05_R3_ATTR_CONTROLS) != 0u &&
                    (((b & G05_R3_ATTR_ALT) == 0u) || !context->current.alt_nonempty))
                    status = emit_attr(context, UINT64_C(13), G05_R3_ATTR_CONTROLS);
                break;
            case ARBOR_VIEW0_NATIVE_ELEMENT_AREA:
                status = count_predicates(context, UINT64_C(2));
                if (status.native == 0 && (b & G05_R3_ATTR_HREF) == 0u &&
                    (b & G05_R3_ATTR_ALT) != 0u)
                    status = emit_attr(context, UINT64_C(14), G05_R3_ATTR_ALT);
                if (status.native == 0 && (b & G05_R3_ATTR_HREF) == 0u) {
                    uint64_t x = b & (G05_R3_ATTR_TARGET | G05_R3_ATTR_DOWNLOAD | G05_R3_ATTR_PING |
                                      G05_R3_ATTR_REL | G05_R3_ATTR_REFERRERPOLICY |
                                      G05_R3_ATTR_HREFLANG | G05_R3_ATTR_TYPE);
                    while (status.native == 0 && x != 0u) {
                        const uint64_t bit = x & (~x + UINT64_C(1));
                        status = emit_attr(context, UINT64_C(15), bit);
                        x &= ~bit;
                    }
                }
                break;
            case ARBOR_VIEW0_NATIVE_ELEMENT_INPUT:
                status = evaluate_input(context);
                break;
            case ARBOR_VIEW0_NATIVE_ELEMENT_OPTGROUP:
                status = count_predicates(context, UINT64_C(1));
                if (status.native == 0 && (b & G05_R3_ATTR_LABEL) == 0u &&
                    !context->current.has_child_legend)
                    status = emit_owner(context, UINT64_C(40));
                break;
            case ARBOR_VIEW0_NATIVE_ELEMENT_TEXTAREA:
                status = count_predicates(context, UINT64_C(1));
                if (status.native == 0 && context->current.wrap_hard &&
                    (b & G05_R3_ATTR_COLS) == 0u)
                    status = emit_attr(context, UINT64_C(41), G05_R3_ATTR_WRAP);
                break;
            case ARBOR_VIEW0_NATIVE_ELEMENT_METER:
                status = count_predicates(context, UINT64_C(1));
                if (status.native == 0 && (b & G05_R3_ATTR_VALUE) == 0u)
                    status = emit_owner(context, UINT64_C(42));
                break;
            case ARBOR_VIEW0_NATIVE_ELEMENT_DIALOG:
                status = count_predicates(context, UINT64_C(1));
                if (status.native == 0 && (b & G05_R3_ATTR_TABINDEX) != 0u)
                    status = emit_attr(context, UINT64_C(43), G05_R3_ATTR_TABINDEX);
                break;
            default:
                break;
        }
    }
    if (status.native != 0) return status;

    if (context->current.standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_A &&
        (context->current.attr_bits & G05_R3_ATTR_HREF) != 0u) {
        if (context->current.depth > G05_R3A_MAX_DEPTH ||
            context->a_href_ancestor_count == UINT64_MAX)
            return err_status(EOVERFLOW);
        context->a_href_stack[context->current.depth] = true;
        context->a_href_ancestor_count += 1u;
    }
    return ok_status();
}

static arbor_status traversal_leave(
    void *opaque,
    const arbor_view0_native_element_observation *observation)
{
    if (opaque == NULL || observation == NULL || observation->depth > G05_R3A_MAX_DEPTH)
        return err_status(EINVAL);
    g05_r3_context *context = (g05_r3_context *)opaque;
    if (context->a_href_stack[observation->depth]) {
        if (context->a_href_ancestor_count == 0u) return err_status(EIO);
        context->a_href_ancestor_count -= 1u;
        context->a_href_stack[observation->depth] = false;
    }
    return ok_status();
}

static arbor_status evaluate(
    arbor_span input,
    arbor_view0_native_source_anchor *anchors,
    uint64_t anchor_capacity,
    bool collect_anchors,
    arbor_view0_native_g05_r3a_evaluation *evaluation_out)
{
    if (evaluation_out == NULL ||
        (collect_anchors && anchor_capacity != 0u && anchors == NULL))
        return err_status(EINVAL);

    g05_r3_context context = {
        .anchors = anchors,
        .anchor_capacity = anchor_capacity,
        .collect_anchors = collect_anchors
    };
    const arbor_view0_native_semantic_observer observer = {
        .context = &context,
        .traversal_enter = traversal_enter,
        .attribute = attribute,
        .direct_child = direct_child,
        .element_complete = element_complete,
        .traversal_leave = traversal_leave,
        .source_attribute = source_attribute
    };
    arbor_view0_native_parse_counts parse_counts = {0};
    arbor_view0_native_document_facts facts = {0};
    arbor_view0_native_observation_counts observation_counts = {0};
    arbor_status status = arbor_view0_native_lexbor_observe(
        input, &observer, &parse_counts, &facts, &observation_counts);
    if (status.native != 0) return status;
    if (observation_counts.max_depth > G05_R3A_MAX_DEPTH ||
        context.a_href_ancestor_count != 0u)
        return err_status(EIO);
    if (collect_anchors && context.diagnostic_count != anchor_capacity)
        return err_status(EIO);

    *evaluation_out = (arbor_view0_native_g05_r3a_evaluation){
        .diagnostic_count = context.diagnostic_count,
        .predicate_evaluation_count = context.predicate_evaluation_count,
        .input_element_count = context.input_element_count,
        .missing_required_count = context.missing_required_count,
        .forbidden_present_count = context.forbidden_present_count,
        .tracked_source_attribute_count = context.source_attr_count
    };
    (void)memcpy(evaluation_out->clause_violation_count,
                 context.clause_violation_count,
                 sizeof(context.clause_violation_count));
    return ok_status();
}

arbor_status arbor_view0_native_g05_r3a_measure(
    arbor_span input,
    arbor_view0_native_g05_r3a_evaluation *evaluation_out)
{
    return evaluate(input, NULL, 0u, false, evaluation_out);
}

arbor_status arbor_view0_native_g05_r3a_collect_anchors(
    arbor_span input,
    arbor_view0_native_source_anchor *anchors,
    uint64_t anchor_capacity,
    arbor_view0_native_g05_r3a_evaluation *evaluation_out)
{
    return evaluate(input, anchors, anchor_capacity, true, evaluation_out);
}

void arbor_view0_native_g05_r3a_materialize_anchor(
    const arbor_view0_native_source_anchor *anchor,
    uint64_t discovery_sequence,
    arbor_view0_native_diagnostic *diagnostic)
{
    static const char symbolic[] = "ARBOR_VIEW_V1_G05_CONDITIONAL_ATTRIBUTE_APPLICABILITY";
    static const char message[] =
        "Attribute conditional applicability requirement is not satisfied by the frozen predicate table";
    if (anchor == NULL || diagnostic == NULL) return;
    (void)memset(diagnostic, 0, sizeof(*diagnostic));
    diagnostic->rule_id = ARBOR_VIEW_V1_G05_CONDITIONAL_ATTRIBUTE_APPLICABILITY;
    diagnostic->byte_offset = (uint64_t)anchor->byte_offset;
    diagnostic->source_length = (uint64_t)anchor->source_length;
    diagnostic->discovery_sequence = discovery_sequence;
    diagnostic->severity = (uint32_t)ARBOR_VIEW0_NATIVE_SEVERITY_ERROR;
    diagnostic->origin = (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_AUTHORING;
    (void)memcpy(diagnostic->symbolic_name, symbolic, sizeof(symbolic));
    (void)memcpy(diagnostic->message, message, sizeof(message));
}
