#include "g06.h"
#include "g05_c0.h"
#include "g06_c0.h"

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef enum g06_attr_code {
    G06_ATTR_NONE = 0,
    G06_ATTR_ACCEPT,
    G06_ATTR_ALPHA,
    G06_ATTR_ALT,
    G06_ATTR_AS,
    G06_ATTR_ASYNC,
    G06_ATTR_CHECKED,
    G06_ATTR_CLASS,
    G06_ATTR_CLOSEDBY,
    G06_ATTR_COLORSPACE,
    G06_ATTR_COLS,
    G06_ATTR_COMMAND,
    G06_ATTR_CONTENT,
    G06_ATTR_CONTROLS,
    G06_ATTR_DATETIME,
    G06_ATTR_DEFAULT,
    G06_ATTR_DEFER,
    G06_ATTR_DIR,
    G06_ATTR_DISABLED,
    G06_ATTR_FOR,
    G06_ATTR_HEIGHT,
    G06_ATTR_HREF,
    G06_ATTR_HTTP_EQUIV,
    G06_ATTR_ISMAP,
    G06_ATTR_KIND,
    G06_ATTR_LOW,
    G06_ATTR_HIGH,
    G06_ATTR_MAX,
    G06_ATTR_MIN,
    G06_ATTR_MULTIPLE,
    G06_ATTR_NAME,
    G06_ATTR_NOMODULE,
    G06_ATTR_OPEN,
    G06_ATTR_OPTIMUM,
    G06_ATTR_READONLY,
    G06_ATTR_REL,
    G06_ATTR_REQUIRED,
    G06_ATTR_REVERSED,
    G06_ATTR_ROWS,
    G06_ATTR_SCOPE,
    G06_ATTR_SELECTED,
    G06_ATTR_SHAPE,
    G06_ATTR_SHADOWROOTCLONABLE,
    G06_ATTR_SHADOWROOTCUSTOMELEMENTREGISTRY,
    G06_ATTR_SHADOWROOTDELEGATESFOCUS,
    G06_ATTR_SHADOWROOTMODE,
    G06_ATTR_SHADOWROOTSERIALIZABLE,
    G06_ATTR_SHADOWROOTSLOTASSIGNMENT,
    G06_ATTR_SIZE,
    G06_ATTR_SIZES,
    G06_ATTR_SPAN,
    G06_ATTR_START,
    G06_ATTR_STEP,
    G06_ATTR_TRANSLATE,
    G06_ATTR_TYPE,
    G06_ATTR_VALUE,
    G06_ATTR_WIDTH,
    G06_ATTR_WRAP,
    G06_ATTR__COUNT
} g06_attr_code;

typedef struct g06_attr_name {
    const char *name;
    g06_attr_code code;
} g06_attr_name;

static const g06_attr_name g_attr_names[] = {
    {"accept", G06_ATTR_ACCEPT}, {"alpha", G06_ATTR_ALPHA},
    {"alt", G06_ATTR_ALT}, {"as", G06_ATTR_AS},
    {"async", G06_ATTR_ASYNC}, {"checked", G06_ATTR_CHECKED},
    {"class", G06_ATTR_CLASS}, {"closedby", G06_ATTR_CLOSEDBY},
    {"colorspace", G06_ATTR_COLORSPACE}, {"cols", G06_ATTR_COLS},
    {"command", G06_ATTR_COMMAND}, {"content", G06_ATTR_CONTENT},
    {"controls", G06_ATTR_CONTROLS}, {"datetime", G06_ATTR_DATETIME},
    {"default", G06_ATTR_DEFAULT}, {"defer", G06_ATTR_DEFER},
    {"dir", G06_ATTR_DIR}, {"disabled", G06_ATTR_DISABLED},
    {"for", G06_ATTR_FOR}, {"height", G06_ATTR_HEIGHT},
    {"href", G06_ATTR_HREF}, {"http-equiv", G06_ATTR_HTTP_EQUIV},
    {"ismap", G06_ATTR_ISMAP}, {"kind", G06_ATTR_KIND},
    {"low", G06_ATTR_LOW}, {"high", G06_ATTR_HIGH},
    {"max", G06_ATTR_MAX}, {"min", G06_ATTR_MIN},
    {"multiple", G06_ATTR_MULTIPLE}, {"name", G06_ATTR_NAME},
    {"nomodule", G06_ATTR_NOMODULE}, {"open", G06_ATTR_OPEN},
    {"optimum", G06_ATTR_OPTIMUM}, {"readonly", G06_ATTR_READONLY},
    {"rel", G06_ATTR_REL}, {"required", G06_ATTR_REQUIRED},
    {"reversed", G06_ATTR_REVERSED}, {"rows", G06_ATTR_ROWS},
    {"scope", G06_ATTR_SCOPE}, {"selected", G06_ATTR_SELECTED},
    {"shape", G06_ATTR_SHAPE},
    {"shadowrootclonable", G06_ATTR_SHADOWROOTCLONABLE},
    {"shadowrootcustomelementregistry", G06_ATTR_SHADOWROOTCUSTOMELEMENTREGISTRY},
    {"shadowrootdelegatesfocus", G06_ATTR_SHADOWROOTDELEGATESFOCUS},
    {"shadowrootmode", G06_ATTR_SHADOWROOTMODE},
    {"shadowrootserializable", G06_ATTR_SHADOWROOTSERIALIZABLE},
    {"shadowrootslotassignment", G06_ATTR_SHADOWROOTSLOTASSIGNMENT},
    {"size", G06_ATTR_SIZE}, {"sizes", G06_ATTR_SIZES},
    {"span", G06_ATTR_SPAN}, {"start", G06_ATTR_START},
    {"step", G06_ATTR_STEP}, {"translate", G06_ATTR_TRANSLATE},
    {"type", G06_ATTR_TYPE}, {"value", G06_ATTR_VALUE},
    {"width", G06_ATTR_WIDTH}, {"wrap", G06_ATTR_WRAP}
};

typedef struct g06_source_attr {
    uint64_t owner_source_offset;
    uint32_t source_offset;
    uint32_t source_length;
    uint16_t code;
    uint16_t reserved16;
    uint32_t reserved32;
} g06_source_attr;

typedef struct g06_value {
    arbor_span span;
    bool present;
} g06_value;

typedef struct g06_current {
    uint64_t standard_element_id;
    uint64_t source_offset;
    uint64_t source_length;
    uint64_t depth;
    g06_value values[G06_ATTR__COUNT];
    uint8_t time_text[ARBOR_VIEW0_NATIVE_G06_TIME_TEXT_CAP];
    uint64_t time_text_length;
    bool time_has_element_child;
    bool time_text_overflow;
} g06_current;

typedef struct g06_comma_slot {
    uint64_t hash;
    uint64_t start;
    uint64_t length;
} g06_comma_slot;

typedef enum g06_comma_result {
    G06_COMMA_VALID = 0,
    G06_COMMA_INVALID = 1,
    G06_COMMA_CAPACITY = 2
} g06_comma_result;

typedef struct g06_context {
    arbor_view0_native_g06_anchor *anchors;
    uint64_t anchor_capacity;
    bool collect_anchors;
    arbor_view0_native_g06_evaluation evaluation;
    g06_source_attr source_attrs[ARBOR_VIEW0_NATIVE_G06_MAX_SOURCE_ATTRIBUTES];
    uint64_t source_attr_count;
    g06_current current;
    bool a_href_stack[4098];
    uint64_t a_href_ancestor_count;
} g06_context;

typedef struct g06_decimal {
    arbor_span value;
    uint64_t first_significant;
    uint64_t last_significant;
    uint64_t significant_digits;
    uint64_t fraction_digits;
    uint64_t trailing_zeros;
    uint64_t exponent_start;
    uint64_t exponent_length;
    bool negative;
    bool exponent_negative;
    bool zero;
} g06_decimal;

static arbor_status err_status(int e) { return arbor_status_from_native(-(int64_t)e); }
static arbor_status ok_status(void) { return arbor_status_from_native(0); }

static uint8_t ascii_lower(uint8_t c)
{
    return c >= (uint8_t)'A' && c <= (uint8_t)'Z'
        ? (uint8_t)(c + ((uint8_t)'a' - (uint8_t)'A')) : c;
}

static bool ascii_space(uint8_t c)
{
    return c == UINT8_C(0x09) || c == UINT8_C(0x0a) || c == UINT8_C(0x0c) ||
           c == UINT8_C(0x0d) || c == UINT8_C(0x20);
}

static bool ascii_digit(uint8_t c)
{
    return c >= (uint8_t)'0' && c <= (uint8_t)'9';
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

static g06_attr_code attr_code(arbor_span name)
{
    for (size_t i = 0u; i < sizeof(g_attr_names) / sizeof(g_attr_names[0]); ++i) {
        if (span_eq_literal(name, g_attr_names[i].name)) return g_attr_names[i].code;
    }
    return G06_ATTR_NONE;
}

static bool token_contains_ci(arbor_span value, const char *literal)
{
    uint64_t p = 0u;
    while (p < value.length) {
        while (p < value.length && ascii_space(value.data[p])) ++p;
        const uint64_t start = p;
        while (p < value.length && !ascii_space(value.data[p])) ++p;
        arbor_span token = {value.data + start, p - start};
        if (span_eq_ci_literal(token, literal)) return true;
    }
    return false;
}

static bool all_digits_nonzero_year(arbor_span value)
{
    bool nonzero = false;
    if (value.data == NULL || value.length < 4u) return false;
    for (uint64_t i = 0u; i < value.length; ++i) {
        if (!ascii_digit(value.data[i])) return false;
        nonzero = nonzero || value.data[i] != (uint8_t)'0';
    }
    return nonzero;
}

static bool nonnegative_lexical(arbor_span value, bool *zero_out)
{
    bool nonzero = false;
    if (value.data == NULL || value.length == 0u) return false;
    for (uint64_t i = 0u; i < value.length; ++i) {
        if (!ascii_digit(value.data[i])) return false;
        nonzero = nonzero || value.data[i] != (uint8_t)'0';
    }
    if (zero_out != NULL) *zero_out = !nonzero;
    return true;
}

static bool decimal_parse(arbor_span value, g06_decimal *out)
{
    uint64_t p = 0u;
    uint64_t mantissa_end;
    uint64_t digit_count = 0u;
    uint64_t dot_digit_count = 0u;
    uint64_t first = UINT64_MAX;
    uint64_t last = UINT64_MAX;
    uint64_t trailing = 0u;
    bool seen_dot = false;
    bool negative = false;
    bool exp_negative = false;

    if (out == NULL || value.data == NULL ||
        !arbor_view0_native_g06_c0_floating_point(value)) return false;
    if (value.data[p] == (uint8_t)'-') { negative = true; ++p; }
    mantissa_end = value.length;
    for (uint64_t i = p; i < value.length; ++i) {
        if (value.data[i] == (uint8_t)'e' || value.data[i] == (uint8_t)'E') {
            mantissa_end = i;
            break;
        }
    }
    for (uint64_t i = p; i < mantissa_end; ++i) {
        if (value.data[i] == (uint8_t)'.') { seen_dot = true; continue; }
        ++digit_count;
        if (seen_dot) ++dot_digit_count;
        if (value.data[i] != (uint8_t)'0') {
            if (first == UINT64_MAX) first = i;
            last = i;
            trailing = 0u;
        } else if (first != UINT64_MAX) {
            ++trailing;
        }
    }
    uint64_t exp_start = value.length;
    uint64_t exp_length = 0u;
    if (mantissa_end < value.length) {
        p = mantissa_end + 1u;
        if (value.data[p] == (uint8_t)'+' || value.data[p] == (uint8_t)'-') {
            exp_negative = value.data[p] == (uint8_t)'-';
            ++p;
        }
        while (p < value.length && value.data[p] == (uint8_t)'0') ++p;
        exp_start = p;
        exp_length = value.length - p;
    }
    *out = (g06_decimal){
        .value = value,
        .first_significant = first,
        .last_significant = last,
        .significant_digits = first == UINT64_MAX ? 0u : digit_count -
            (first - (negative ? 1u : 0u) - (seen_dot && first > mantissa_end ? 1u : 0u)) - trailing,
        .fraction_digits = dot_digit_count,
        .trailing_zeros = trailing,
        .exponent_start = exp_start,
        .exponent_length = exp_length,
        .negative = negative,
        .exponent_negative = exp_negative,
        .zero = first == UINT64_MAX
    };
    if (!out->zero) {
        uint64_t sig = 0u;
        for (uint64_t i = first; i <= last; ++i) if (ascii_digit(value.data[i])) ++sig;
        out->significant_digits = sig;
    }
    return true;
}

static int unsigned_decimal_compare(arbor_span a, uint64_t as, uint64_t al,
                                    arbor_span b, uint64_t bs, uint64_t bl)
{
    while (al != 0u && a.data[as] == (uint8_t)'0') { ++as; --al; }
    while (bl != 0u && b.data[bs] == (uint8_t)'0') { ++bs; --bl; }
    if (al != bl) return al < bl ? -1 : 1;
    for (uint64_t i = 0u; i < al; ++i) {
        if (a.data[as + i] != b.data[bs + i])
            return a.data[as + i] < b.data[bs + i] ? -1 : 1;
    }
    return 0;
}

/* Return min(|a-b|, cap+1) for non-negative decimal strings, with a >= b. */
static uint64_t unsigned_decimal_difference_capped(
    arbor_span a, uint64_t as, uint64_t al,
    arbor_span b, uint64_t bs, uint64_t bl, uint64_t cap)
{
    uint64_t result = 0u;
    uint64_t place = 1u;
    uint8_t borrow = 0u;
    uint64_t ai = al;
    uint64_t bi = bl;
    while (ai != 0u || bi != 0u) {
        int av = ai == 0u ? 0 : (int)(a.data[as + --ai] - (uint8_t)'0');
        const int bv = bi == 0u ? 0 : (int)(b.data[bs + --bi] - (uint8_t)'0');
        av -= (int)borrow;
        if (av < bv) { av += 10; borrow = 1u; } else borrow = 0u;
        const uint64_t digit = (uint64_t)(av - bv);
        if (digit != 0u) {
            if (place > cap || digit > (cap - result) / place) return cap + 1u;
            result += digit * place;
        }
        if (ai != 0u || bi != 0u) {
            if (place > cap / UINT64_C(10)) place = cap + 1u;
            else place *= UINT64_C(10);
        }
    }
    return result;
}

static int exponent_difference_compare(const g06_decimal *a,
                                       const g06_decimal *b,
                                       int64_t target)
{
    arbor_span av = a->value;
    arbor_span bv = b->value;
    const uint64_t cap = target < 0 ? (uint64_t)(-(target + 1)) + 1u : (uint64_t)target;
    if (a->exponent_length == 0u && b->exponent_length == 0u)
        return 0 < target ? -1 : (0 > target ? 1 : 0);
    if (a->exponent_negative != b->exponent_negative) {
        if (a->exponent_length == 0u) {
            return b->exponent_negative ? (0 < target ? -1 : (0 > target ? 1 : 0)) : -1;
        }
        if (b->exponent_length == 0u) return a->exponent_negative ? -1 : 1;
        return a->exponent_negative ? -1 : 1;
    }
    const int cmp = unsigned_decimal_compare(
        av, a->exponent_start, a->exponent_length,
        bv, b->exponent_start, b->exponent_length);
    if (cmp == 0) return 0 < target ? -1 : (0 > target ? 1 : 0);
    const g06_decimal *hi = cmp > 0 ? a : b;
    const g06_decimal *lo = cmp > 0 ? b : a;
    uint64_t diff = unsigned_decimal_difference_capped(
        hi->value, hi->exponent_start, hi->exponent_length,
        lo->value, lo->exponent_start, lo->exponent_length, cap);
    int sign = cmp;
    if (a->exponent_negative) sign = -sign;
    if (diff > cap) return sign;
    int64_t signed_diff = sign > 0 ? (int64_t)diff : -(int64_t)diff;
    return signed_diff < target ? -1 : (signed_diff > target ? 1 : 0);
}

static uint8_t significant_digit_at(const g06_decimal *value, uint64_t index)
{
    uint64_t seen = 0u;
    if (index >= value->significant_digits) return (uint8_t)'0';
    for (uint64_t p = value->first_significant; p <= value->last_significant; ++p) {
        if (!ascii_digit(value->value.data[p])) continue;
        if (seen == index) return value->value.data[p];
        ++seen;
    }
    return (uint8_t)'0';
}

static int decimal_compare(const g06_decimal *a, const g06_decimal *b)
{
    if (a->zero && b->zero) return 0;
    if (a->negative != b->negative) return a->negative ? -1 : 1;
    if (a->zero) return b->negative ? 1 : -1;
    if (b->zero) return a->negative ? -1 : 1;
    const int64_t ak = (int64_t)a->significant_digits - (int64_t)a->fraction_digits +
                       (int64_t)a->trailing_zeros;
    const int64_t bk = (int64_t)b->significant_digits - (int64_t)b->fraction_digits +
                       (int64_t)b->trailing_zeros;
    const int exp_cmp = exponent_difference_compare(a, b, bk - ak);
    int magnitude = exp_cmp;
    if (magnitude == 0) {
        const uint64_t max = a->significant_digits > b->significant_digits
            ? a->significant_digits : b->significant_digits;
        for (uint64_t i = 0u; i < max; ++i) {
            const uint8_t ad = significant_digit_at(a, i);
            const uint8_t bd = significant_digit_at(b, i);
            if (ad != bd) { magnitude = ad < bd ? -1 : 1; break; }
        }
    }
    return a->negative ? -magnitude : magnitude;
}

static g06_decimal decimal_zero(void)
{
    static const uint8_t zero[] = {'0'};
    g06_decimal out;
    (void)decimal_parse((arbor_span){zero, 1u}, &out);
    return out;
}

static g06_decimal decimal_one(void)
{
    static const uint8_t one[] = {'1'};
    g06_decimal out;
    (void)decimal_parse((arbor_span){one, 1u}, &out);
    return out;
}

static const g06_source_attr *find_source_attr(
    const g06_context *context, uint64_t owner_source_offset, g06_attr_code code)
{
    for (uint64_t i = 0u; i < context->source_attr_count; ++i) {
        if (context->source_attrs[i].owner_source_offset == owner_source_offset &&
            context->source_attrs[i].code == (uint16_t)code)
            return &context->source_attrs[i];
    }
    return NULL;
}

static arbor_status add_violation(
    g06_context *context, uint16_t rule_ordinal,
    uint64_t source_offset, uint64_t source_length)
{
    if (context == NULL || rule_ordinal == 0u ||
        rule_ordinal > ARBOR_VIEW0_NATIVE_G06_RULE_COUNT ||
        source_offset == ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE ||
        source_length == 0u || source_offset > UINT32_MAX || source_length > UINT32_MAX)
        return err_status(EIO);
    if (context->evaluation.diagnostic_count == UINT64_MAX ||
        context->evaluation.rule_violation_count[rule_ordinal - 1u] == UINT64_MAX)
        return err_status(EOVERFLOW);
    if (context->collect_anchors) {
        if (context->anchors == NULL ||
            context->evaluation.diagnostic_count >= context->anchor_capacity)
            return err_status(ENOSPC);
        context->anchors[context->evaluation.diagnostic_count] =
            (arbor_view0_native_g06_anchor){
                .source = {
                    .byte_offset = (uint32_t)source_offset,
                    .source_length = (uint32_t)source_length
                },
                .rule_ordinal = rule_ordinal
            };
    }
    context->evaluation.diagnostic_count += 1u;
    context->evaluation.rule_violation_count[rule_ordinal - 1u] += 1u;
    return ok_status();
}

static arbor_status add_attr_violation(
    g06_context *context, uint16_t rule_ordinal, g06_attr_code code)
{
    const g06_source_attr *source = find_source_attr(
        context, context->current.source_offset, code);
    if (source == NULL) return err_status(EIO);
    return add_violation(context, rule_ordinal,
                         (uint64_t)source->source_offset,
                         (uint64_t)source->source_length);
}

static arbor_status match_consumer(g06_context *context)
{
    if (context->evaluation.matched_consumer_count == UINT64_MAX)
        return err_status(EOVERFLOW);
    context->evaluation.matched_consumer_count += 1u;
    return ok_status();
}

static arbor_status suppress_prior_owner(g06_context *context)
{
    if (context->evaluation.prior_owner_suppression_count == UINT64_MAX)
        return err_status(EOVERFLOW);
    context->evaluation.prior_owner_suppression_count += 1u;
    return ok_status();
}

static uint64_t input_state(arbor_span type)
{
    if (span_eq_ci_literal(type, "hidden")) return ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_HIDDEN;
    if (span_eq_ci_literal(type, "search")) return ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_SEARCH;
    if (span_eq_ci_literal(type, "tel")) return ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TEL;
    if (span_eq_ci_literal(type, "url")) return ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_URL;
    if (span_eq_ci_literal(type, "email")) return ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_EMAIL;
    if (span_eq_ci_literal(type, "password")) return ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_PASSWORD;
    if (span_eq_ci_literal(type, "date")) return ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_DATE;
    if (span_eq_ci_literal(type, "month")) return ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_MONTH;
    if (span_eq_ci_literal(type, "week")) return ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_WEEK;
    if (span_eq_ci_literal(type, "time")) return ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TIME;
    if (span_eq_ci_literal(type, "datetime-local")) return ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_DATETIME_LOCAL;
    if (span_eq_ci_literal(type, "number")) return ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_NUMBER;
    if (span_eq_ci_literal(type, "range")) return ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_RANGE;
    if (span_eq_ci_literal(type, "color")) return ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_COLOR;
    if (span_eq_ci_literal(type, "checkbox")) return ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_CHECKBOX;
    if (span_eq_ci_literal(type, "radio")) return ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_RADIO;
    if (span_eq_ci_literal(type, "file")) return ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_FILE;
    if (span_eq_ci_literal(type, "submit")) return ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_SUBMIT;
    if (span_eq_ci_literal(type, "image")) return ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_IMAGE;
    if (span_eq_ci_literal(type, "reset")) return ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_RESET;
    if (span_eq_ci_literal(type, "button")) return ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_BUTTON;
    return ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TEXT;
}

static bool input_state_in(uint64_t state, uint64_t mask)
{
    return (state & mask) != 0u;
}

static bool keyword_list(arbor_span value, const char *const *keywords,
                         size_t keyword_count, bool empty_allowed)
{
    return arbor_view0_native_g06_c0_enumerated(
        value, keywords, (uint64_t)keyword_count, empty_allowed);
}

static bool valid_custom_command(arbor_span value)
{
    return value.data != NULL && value.length >= 2u &&
           value.data[0] == (uint8_t)'-' && value.data[1] == (uint8_t)'-';
}

static bool positive_floating(arbor_span value)
{
    g06_decimal parsed;
    return decimal_parse(value, &parsed) && !parsed.negative && !parsed.zero;
}

static bool valid_dimension_token(arbor_span token)
{
    uint64_t x = UINT64_MAX;
    bool zero = false;
    if (span_eq_ci_literal(token, "any")) return true;
    for (uint64_t i = 0u; i < token.length; ++i) {
        if (token.data[i] == (uint8_t)'x' || token.data[i] == (uint8_t)'X') {
            if (x != UINT64_MAX) return false;
            x = i;
        }
    }
    if (x == UINT64_MAX || x == 0u || x + 1u == token.length) return false;
    arbor_span width = {token.data, x};
    arbor_span height = {token.data + x + 1u, token.length - x - 1u};
    if (!nonnegative_lexical(width, &zero) || zero) return false;
    if (!nonnegative_lexical(height, &zero) || zero) return false;
    return true;
}

static bool valid_sizes_value(arbor_span value)
{
    uint64_t p = 0u;
    while (p < value.length) {
        while (p < value.length && ascii_space(value.data[p])) ++p;
        if (p == value.length) break;
        const uint64_t start = p;
        while (p < value.length && !ascii_space(value.data[p])) ++p;
        if (!valid_dimension_token((arbor_span){value.data + start, p - start})) return false;
    }
    return true;
}

static bool valid_refresh_delay(arbor_span value)
{
    uint64_t p = 0u;
    while (p < value.length && ascii_space(value.data[p])) ++p;
    const uint64_t start = p;
    while (p < value.length && ascii_digit(value.data[p])) ++p;
    if (p == start) return false;
    while (p < value.length && ascii_space(value.data[p])) ++p;
    return p == value.length || value.data[p] == (uint8_t)';' || value.data[p] == (uint8_t)',';
}

static bool token_char(uint8_t c)
{
    if (c <= UINT8_C(0x20) || c >= UINT8_C(0x7f)) return false;
    switch (c) {
        case '(': case ')': case '<': case '>': case '@': case ',': case ';':
        case ':': case '\\': case '"': case '/': case '[': case ']': case '?':
        case '=': case '{': case '}':
            return false;
        default:
            return true;
    }
}

static bool valid_accept_token(arbor_span token)
{
    if (token.length == 0u || token.data == NULL) return false;
    if (token.data[0] == (uint8_t)'.') {
        if (token.length == 1u) return false;
        for (uint64_t i = 1u; i < token.length; ++i)
            if (ascii_space(token.data[i]) || token.data[i] == (uint8_t)'.') return false;
        return true;
    }
    uint64_t slash = UINT64_MAX;
    for (uint64_t i = 0u; i < token.length; ++i) {
        if (token.data[i] == (uint8_t)'/') {
            if (slash != UINT64_MAX) return false;
            slash = i;
        } else if (!token_char(token.data[i])) return false;
    }
    return slash != UINT64_MAX && slash != 0u && slash + 1u < token.length;
}

static bool email_atom_char(uint8_t c)
{
    if ((c >= (uint8_t)'a' && c <= (uint8_t)'z') ||
        (c >= (uint8_t)'A' && c <= (uint8_t)'Z') ||
        ascii_digit(c)) return true;
    return strchr(".!#$%&'*+/=?^_`{|}~-", (int)c) != NULL;
}

static bool valid_email(arbor_span value)
{
    uint64_t at = UINT64_MAX;
    if (value.data == NULL || value.length == 0u) return false;
    for (uint64_t i = 0u; i < value.length; ++i) {
        if (value.data[i] == (uint8_t)'@') {
            if (at != UINT64_MAX) return false;
            at = i;
        }
    }
    if (at == UINT64_MAX || at == 0u || at + 1u == value.length) return false;
    for (uint64_t i = 0u; i < at; ++i) if (!email_atom_char(value.data[i])) return false;
    uint64_t label_start = at + 1u;
    for (uint64_t i = label_start; i <= value.length; ++i) {
        if (i != value.length && value.data[i] != (uint8_t)'.') continue;
        const uint64_t length = i - label_start;
        if (length == 0u || length > 63u ||
            value.data[label_start] == (uint8_t)'-' || value.data[i - 1u] == (uint8_t)'-')
            return false;
        for (uint64_t j = label_start; j < i; ++j) {
            const uint8_t c = value.data[j];
            if (!ascii_digit(c) && !(c >= (uint8_t)'a' && c <= (uint8_t)'z') &&
                !(c >= (uint8_t)'A' && c <= (uint8_t)'Z') && c != (uint8_t)'-')
                return false;
        }
        label_start = i + 1u;
    }
    return true;
}

static arbor_span trim_ascii(arbor_span value)
{
    uint64_t start = 0u;
    uint64_t end = value.length;
    while (start < end && ascii_space(value.data[start])) ++start;
    while (end > start && ascii_space(value.data[end - 1u])) --end;
    return (arbor_span){value.data + start, end - start};
}

static uint64_t comma_token_hash(arbor_span token, bool ascii_case_insensitive)
{
    uint64_t hash = UINT64_C(1469598103934665603);
    for (uint64_t i = 0u; i < token.length; ++i) {
        const uint8_t byte = ascii_case_insensitive
            ? ascii_lower(token.data[i]) : token.data[i];
        hash ^= (uint64_t)byte;
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

static bool comma_token_equal(
    arbor_span value, arbor_span token, const g06_comma_slot *slot,
    bool ascii_case_insensitive)
{
    if (slot == NULL || slot->length != token.length ||
        slot->start > value.length || slot->length > value.length - slot->start)
        return false;
    for (uint64_t i = 0u; i < token.length; ++i) {
        const uint8_t a = ascii_case_insensitive
            ? ascii_lower(token.data[i]) : token.data[i];
        const uint8_t b = ascii_case_insensitive
            ? ascii_lower(value.data[slot->start + i]) : value.data[slot->start + i];
        if (a != b) return false;
    }
    return true;
}

static g06_comma_result valid_comma_value(
    arbor_span value, bool (*validator)(arbor_span), bool allow_empty_list,
    bool require_unique_ci)
{
    g06_comma_slot slots[ARBOR_VIEW0_NATIVE_G06_C0_TOKEN_WORKSPACE_CAP];
    uint64_t p = 0u;
    uint64_t count = 0u;
    if (value.length == 0u)
        return allow_empty_list ? G06_COMMA_VALID : G06_COMMA_INVALID;
    if (require_unique_ci) {
        for (uint64_t i = 0u; i < ARBOR_VIEW0_NATIVE_G06_C0_TOKEN_WORKSPACE_CAP; ++i) {
            slots[i].hash = 0u;
            slots[i].start = UINT64_MAX;
            slots[i].length = 0u;
        }
    }
    while (p <= value.length) {
        const uint64_t start = p;
        while (p < value.length && value.data[p] != (uint8_t)',') ++p;
        arbor_span token = trim_ascii((arbor_span){value.data + start, p - start});
        if (token.length == 0u || (validator != NULL && !validator(token)))
            return G06_COMMA_INVALID;
        if (require_unique_ci) {
            if (count == ARBOR_VIEW0_NATIVE_G06_C0_TOKEN_WORKSPACE_CAP)
                return G06_COMMA_CAPACITY;
            const uint64_t hash = comma_token_hash(token, true);
            uint64_t slot = hash % ARBOR_VIEW0_NATIVE_G06_C0_TOKEN_WORKSPACE_CAP;
            uint64_t probes = 0u;
            for (; probes < ARBOR_VIEW0_NATIVE_G06_C0_TOKEN_WORKSPACE_CAP; ++probes) {
                if (slots[slot].start == UINT64_MAX) {
                    slots[slot] = (g06_comma_slot){
                        .hash = hash,
                        .start = (uint64_t)(token.data - value.data),
                        .length = token.length
                    };
                    break;
                }
                if (slots[slot].hash == hash &&
                    comma_token_equal(value, token, slots + slot, true))
                    return G06_COMMA_INVALID;
                slot = (slot + 1u) % ARBOR_VIEW0_NATIVE_G06_C0_TOKEN_WORKSPACE_CAP;
            }
            if (probes == ARBOR_VIEW0_NATIVE_G06_C0_TOKEN_WORKSPACE_CAP)
                return G06_COMMA_CAPACITY;
        }
        ++count;
        if (p == value.length) break;
        ++p;
    }
    return count != 0u ? G06_COMMA_VALID : G06_COMMA_INVALID;
}

static bool current_has(const g06_context *context, g06_attr_code code)
{
    return context->current.values[code].present;
}

static arbor_span current_value(const g06_context *context, g06_attr_code code)
{
    return context->current.values[code].span;
}

static arbor_status validate_boolean_attr(
    g06_context *context, g06_attr_code code, bool condition)
{
    if (!current_has(context, code)) return ok_status();
    if (!condition) return suppress_prior_owner(context);
    arbor_status status = match_consumer(context);
    if (status.native != 0) return status;
    const g06_source_attr *source = find_source_attr(
        context, context->current.source_offset, code);
    if (source == NULL) return err_status(EIO);
    const char *canonical = NULL;
    for (size_t i = 0u; i < sizeof(g_attr_names) / sizeof(g_attr_names[0]); ++i) {
        if (g_attr_names[i].code == code) { canonical = g_attr_names[i].name; break; }
    }
    if (canonical == NULL) return err_status(EIO);
    arbor_span name = {(const uint8_t *)canonical, (uint64_t)strlen(canonical)};
    if (!arbor_view0_native_g06_c0_boolean(name, current_value(context, code)))
        return add_attr_violation(context, UINT16_C(1), code);
    return ok_status();
}

static arbor_status validate_enumerated_attr(
    g06_context *context, g06_attr_code code, bool condition,
    const char *const *keywords, size_t keyword_count, bool empty_allowed,
    bool custom_command_allowed)
{
    if (!current_has(context, code)) return ok_status();
    if (!condition) return suppress_prior_owner(context);
    arbor_status status = match_consumer(context);
    if (status.native != 0) return status;
    arbor_span value = current_value(context, code);
    if (!keyword_list(value, keywords, keyword_count, empty_allowed) &&
        !(custom_command_allowed && valid_custom_command(value)))
        return add_attr_violation(context, UINT16_C(2), code);
    return ok_status();
}

static arbor_status validate_signed_attr(g06_context *context, g06_attr_code code)
{
    if (!current_has(context, code)) return ok_status();
    arbor_status status = match_consumer(context);
    if (status.native != 0) return status;
    if (arbor_view0_native_g06_c0_signed_integer(current_value(context, code), NULL) ==
        ARBOR_VIEW0_NATIVE_G06_C0_NUMBER_SYNTAX)
        return add_attr_violation(context, UINT16_C(3), code);
    return ok_status();
}

static arbor_status validate_nonnegative_attr(
    g06_context *context, g06_attr_code code, bool condition,
    bool require_positive, bool upper_1000)
{
    if (!current_has(context, code)) return ok_status();
    if (!condition) return suppress_prior_owner(context);
    arbor_status status = match_consumer(context);
    if (status.native != 0) return status;
    bool zero = false;
    arbor_span value = current_value(context, code);
    if (!nonnegative_lexical(value, &zero) || (require_positive && zero))
        return add_attr_violation(context, UINT16_C(4), code);
    if (upper_1000) {
        uint64_t parsed = 0u;
        if (arbor_view0_native_g06_c0_nonnegative_integer(value, &parsed) !=
                ARBOR_VIEW0_NATIVE_G06_C0_NUMBER_VALID || parsed > UINT64_C(1000))
            return add_attr_violation(context, UINT16_C(4), code);
    }
    return ok_status();
}

static arbor_status validate_float_attr(
    g06_context *context, g06_attr_code code, bool condition,
    bool positive_or_any)
{
    if (!current_has(context, code)) return ok_status();
    if (!condition) return suppress_prior_owner(context);
    arbor_status status = match_consumer(context);
    if (status.native != 0) return status;
    arbor_span value = current_value(context, code);
    bool valid = positive_or_any && span_eq_ci_literal(value, "any");
    if (!valid) valid = positive_or_any ? positive_floating(value) :
                                         arbor_view0_native_g06_c0_floating_point(value);
    if (!valid) return add_attr_violation(context, UINT16_C(5), code);
    return ok_status();
}

static arbor_status validate_datetime_attr(
    g06_context *context, g06_attr_code code, bool condition,
    uint16_t rule_ordinal, bool (*validator)(arbor_span))
{
    if (!current_has(context, code)) return ok_status();
    if (!condition) return suppress_prior_owner(context);
    arbor_status status = match_consumer(context);
    if (status.native != 0) return status;
    if (!validator(current_value(context, code)))
        return add_attr_violation(context, rule_ordinal, code);
    return ok_status();
}

static arbor_status evaluate_r1(g06_context *context, uint64_t state, uint64_t rel)
{
    arbor_status status;
    const uint64_t id = context->current.standard_element_id;
#define G06_BOOL(ELEMENT, ATTR, CONDITION) do { \
    if (id == (ELEMENT)) { status = validate_boolean_attr(context, (ATTR), (CONDITION)); \
        if (status.native != 0) return status; } \
} while (0)
    G06_BOOL(ARBOR_VIEW0_NATIVE_ELEMENT_DETAILS, G06_ATTR_OPEN, true);
    G06_BOOL(ARBOR_VIEW0_NATIVE_ELEMENT_DIALOG, G06_ATTR_OPEN, true);
    G06_BOOL(ARBOR_VIEW0_NATIVE_ELEMENT_IMG, G06_ATTR_CONTROLS,
             current_has(context, G06_ATTR_ALT) && current_value(context, G06_ATTR_ALT).length != 0u);
    G06_BOOL(ARBOR_VIEW0_NATIVE_ELEMENT_IMG, G06_ATTR_ISMAP,
             context->a_href_ancestor_count != 0u);
    G06_BOOL(ARBOR_VIEW0_NATIVE_ELEMENT_INPUT, G06_ATTR_ALPHA,
             state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_COLOR);
    G06_BOOL(ARBOR_VIEW0_NATIVE_ELEMENT_INPUT, G06_ATTR_CHECKED,
             input_state_in(state, ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_CHECKBOX |
                                   ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_RADIO));
    G06_BOOL(ARBOR_VIEW0_NATIVE_ELEMENT_INPUT, G06_ATTR_MULTIPLE,
             input_state_in(state, ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_EMAIL |
                                   ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_FILE));
    G06_BOOL(ARBOR_VIEW0_NATIVE_ELEMENT_INPUT, G06_ATTR_READONLY,
             input_state_in(state,
                 ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TEXT |
                 ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_SEARCH |
                 ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TEL |
                 ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_URL |
                 ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_EMAIL |
                 ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_PASSWORD |
                 ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_DATE |
                 ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_MONTH |
                 ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_WEEK |
                 ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TIME |
                 ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_DATETIME_LOCAL |
                 ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_NUMBER));
    G06_BOOL(ARBOR_VIEW0_NATIVE_ELEMENT_INPUT, G06_ATTR_REQUIRED,
             input_state_in(state,
                 ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TEXT |
                 ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_SEARCH |
                 ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TEL |
                 ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_URL |
                 ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_EMAIL |
                 ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_PASSWORD |
                 ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_DATE |
                 ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_MONTH |
                 ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_WEEK |
                 ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TIME |
                 ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_DATETIME_LOCAL |
                 ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_NUMBER |
                 ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_CHECKBOX |
                 ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_RADIO |
                 ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_FILE));
    G06_BOOL(ARBOR_VIEW0_NATIVE_ELEMENT_LINK, G06_ATTR_DISABLED,
             (rel & UINT64_C(1)) != 0u);
    G06_BOOL(ARBOR_VIEW0_NATIVE_ELEMENT_OL, G06_ATTR_REVERSED, true);
    G06_BOOL(ARBOR_VIEW0_NATIVE_ELEMENT_OPTION, G06_ATTR_DISABLED, true);
    G06_BOOL(ARBOR_VIEW0_NATIVE_ELEMENT_OPTION, G06_ATTR_SELECTED, true);
    G06_BOOL(ARBOR_VIEW0_NATIVE_ELEMENT_SCRIPT, G06_ATTR_ASYNC, true);
    G06_BOOL(ARBOR_VIEW0_NATIVE_ELEMENT_SCRIPT, G06_ATTR_DEFER, true);
    G06_BOOL(ARBOR_VIEW0_NATIVE_ELEMENT_SCRIPT, G06_ATTR_NOMODULE, true);
    G06_BOOL(ARBOR_VIEW0_NATIVE_ELEMENT_SELECT, G06_ATTR_MULTIPLE, true);
    G06_BOOL(ARBOR_VIEW0_NATIVE_ELEMENT_SELECT, G06_ATTR_REQUIRED, true);
    G06_BOOL(ARBOR_VIEW0_NATIVE_ELEMENT_TEMPLATE, G06_ATTR_SHADOWROOTCLONABLE, true);
    G06_BOOL(ARBOR_VIEW0_NATIVE_ELEMENT_TEMPLATE, G06_ATTR_SHADOWROOTCUSTOMELEMENTREGISTRY, true);
    G06_BOOL(ARBOR_VIEW0_NATIVE_ELEMENT_TEMPLATE, G06_ATTR_SHADOWROOTDELEGATESFOCUS, true);
    G06_BOOL(ARBOR_VIEW0_NATIVE_ELEMENT_TEMPLATE, G06_ATTR_SHADOWROOTSERIALIZABLE, true);
    G06_BOOL(ARBOR_VIEW0_NATIVE_ELEMENT_TRACK, G06_ATTR_DEFAULT, true);
#undef G06_BOOL
    return ok_status();
}

static arbor_status evaluate_r2(g06_context *context, uint64_t state, uint64_t rel)
{
    static const char *const area_shape[] = {"circle", "default", "poly", "rect"};
    static const char *const command[] = {"toggle-popover", "show-popover", "hide-popover",
                                          "close", "request-close", "show-modal"};
    static const char *const closedby[] = {"any", "closerequest", "none"};
    static const char *const dir[] = {"ltr", "rtl", "auto"};
    static const char *const translate[] = {"yes", "no"};
    static const char *const colorspace[] = {"limited-srgb", "display-p3"};
    static const char *const preload[] = {"fetch", "font", "image", "script", "style", "track"};
    static const char *const module[] = {"json", "style", "text", "audioworklet", "paintworklet",
                                         "script", "serviceworker", "sharedworker", "worker"};
    static const char *const both[] = {"style", "script"};
    static const char *const shadow_mode[] = {"open", "closed"};
    static const char *const shadow_slot[] = {"named", "manual"};
    static const char *const wrap[] = {"soft", "hard"};
    static const char *const scope[] = {"row", "col", "rowgroup", "colgroup"};
    static const char *const kind[] = {"subtitles", "captions", "descriptions", "chapters", "metadata"};
    arbor_status status;
    const uint64_t id = context->current.standard_element_id;
#define G06_ENUM(ELEMENT, ATTR, CONDITION, KEYS, EMPTY, CUSTOM) do { \
    if (id == (ELEMENT)) { status = validate_enumerated_attr(context, (ATTR), (CONDITION), \
        (KEYS), sizeof(KEYS) / sizeof((KEYS)[0]), (EMPTY), (CUSTOM)); \
        if (status.native != 0) return status; } \
} while (0)
    G06_ENUM(ARBOR_VIEW0_NATIVE_ELEMENT_AREA, G06_ATTR_SHAPE, true, area_shape, false, false);
    G06_ENUM(ARBOR_VIEW0_NATIVE_ELEMENT_BUTTON, G06_ATTR_COMMAND, true, command, false, true);
    G06_ENUM(ARBOR_VIEW0_NATIVE_ELEMENT_DIALOG, G06_ATTR_CLOSEDBY, true, closedby, false, false);
    if (id >= ARBOR_VIEW0_NATIVE_ELEMENT_HTML && id <= ARBOR_VIEW0_NATIVE_ELEMENT_CANVAS) {
        status = validate_enumerated_attr(context, G06_ATTR_DIR, true, dir, 3u, false, false);
        if (status.native != 0) return status;
        status = validate_enumerated_attr(context, G06_ATTR_TRANSLATE, true, translate, 2u, true, false);
        if (status.native != 0) return status;
    }
    G06_ENUM(ARBOR_VIEW0_NATIVE_ELEMENT_INPUT, G06_ATTR_COLORSPACE,
             state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_COLOR, colorspace, false, false);
    if (id == ARBOR_VIEW0_NATIVE_ELEMENT_LINK && current_has(context, G06_ATTR_AS)) {
        if ((rel & UINT64_C(6)) == UINT64_C(2))
            status = validate_enumerated_attr(context, G06_ATTR_AS, true, preload, 6u, false, false);
        else if ((rel & UINT64_C(6)) == UINT64_C(4))
            status = validate_enumerated_attr(context, G06_ATTR_AS, true, module, 9u, false, false);
        else if ((rel & UINT64_C(6)) == UINT64_C(6))
            status = validate_enumerated_attr(context, G06_ATTR_AS, true, both, 2u, false, false);
        else status = suppress_prior_owner(context);
        if (status.native != 0) return status;
    }
    G06_ENUM(ARBOR_VIEW0_NATIVE_ELEMENT_TEMPLATE, G06_ATTR_SHADOWROOTMODE, true, shadow_mode, false, false);
    G06_ENUM(ARBOR_VIEW0_NATIVE_ELEMENT_TEMPLATE, G06_ATTR_SHADOWROOTSLOTASSIGNMENT, true, shadow_slot, false, false);
    G06_ENUM(ARBOR_VIEW0_NATIVE_ELEMENT_TEXTAREA, G06_ATTR_WRAP, true, wrap, false, false);
    G06_ENUM(ARBOR_VIEW0_NATIVE_ELEMENT_TH, G06_ATTR_SCOPE, true, scope, false, false);
    G06_ENUM(ARBOR_VIEW0_NATIVE_ELEMENT_TRACK, G06_ATTR_KIND, true, kind, false, false);
#undef G06_ENUM
    return ok_status();
}

static arbor_status evaluate_r3_r4(g06_context *context, uint64_t state, uint64_t rel)
{
    arbor_status status;
    const uint64_t id = context->current.standard_element_id;
    if (id == ARBOR_VIEW0_NATIVE_ELEMENT_LI) {
        status = validate_signed_attr(context, G06_ATTR_VALUE);
        if (status.native != 0) return status;
    }
    if (id == ARBOR_VIEW0_NATIVE_ELEMENT_OL) {
        status = validate_signed_attr(context, G06_ATTR_START);
        if (status.native != 0) return status;
    }
    if (id == ARBOR_VIEW0_NATIVE_ELEMENT_CANVAS) {
        status = validate_nonnegative_attr(context, G06_ATTR_WIDTH, true, false, false);
        if (status.native != 0) return status;
        status = validate_nonnegative_attr(context, G06_ATTR_HEIGHT, true, false, false);
        if (status.native != 0) return status;
    }
    if (id == ARBOR_VIEW0_NATIVE_ELEMENT_COL || id == ARBOR_VIEW0_NATIVE_ELEMENT_COLGROUP) {
        status = validate_nonnegative_attr(context, G06_ATTR_SPAN, true, true, true);
        if (status.native != 0) return status;
    }
    if (id == ARBOR_VIEW0_NATIVE_ELEMENT_INPUT) {
        const uint64_t size_states =
            ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TEXT |
            ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_SEARCH |
            ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TEL |
            ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_URL |
            ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_EMAIL |
            ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_PASSWORD;
        status = validate_nonnegative_attr(context, G06_ATTR_SIZE,
                                           input_state_in(state, size_states), true, false);
        if (status.native != 0) return status;
    }
    if (id == ARBOR_VIEW0_NATIVE_ELEMENT_LINK && current_has(context, G06_ATTR_SIZES)) {
        if ((rel & UINT64_C(24)) == 0u) {
            status = suppress_prior_owner(context);
        } else {
            status = match_consumer(context);
            if (status.native == 0 && !valid_sizes_value(current_value(context, G06_ATTR_SIZES)))
                status = add_attr_violation(context, UINT16_C(4), G06_ATTR_SIZES);
        }
        if (status.native != 0) return status;
    }
    if (id == ARBOR_VIEW0_NATIVE_ELEMENT_META && current_has(context, G06_ATTR_CONTENT)) {
        const bool refresh = current_has(context, G06_ATTR_HTTP_EQUIV) &&
                             span_eq_ci_literal(current_value(context, G06_ATTR_HTTP_EQUIV), "refresh");
        if (!refresh) status = suppress_prior_owner(context);
        else {
            status = match_consumer(context);
            if (status.native == 0 && !valid_refresh_delay(current_value(context, G06_ATTR_CONTENT)))
                status = add_attr_violation(context, UINT16_C(4), G06_ATTR_CONTENT);
        }
        if (status.native != 0) return status;
    }
    if (id == ARBOR_VIEW0_NATIVE_ELEMENT_SELECT) {
        status = validate_nonnegative_attr(context, G06_ATTR_SIZE, true, true, false);
        if (status.native != 0) return status;
    }
    if (id == ARBOR_VIEW0_NATIVE_ELEMENT_TEXTAREA) {
        status = validate_nonnegative_attr(context, G06_ATTR_COLS, true, true, false);
        if (status.native != 0) return status;
        status = validate_nonnegative_attr(context, G06_ATTR_ROWS, true, true, false);
        if (status.native != 0) return status;
    }
    return ok_status();
}

static arbor_status meter_progress_constraints(g06_context *context)
{
    const uint64_t id = context->current.standard_element_id;
    const g06_decimal zero = decimal_zero();
    const g06_decimal one = decimal_one();
    g06_decimal values[G06_ATTR__COUNT];
    bool parsed[G06_ATTR__COUNT] = {false};
    const g06_attr_code attrs[] = {
        G06_ATTR_VALUE, G06_ATTR_MIN, G06_ATTR_LOW, G06_ATTR_HIGH,
        G06_ATTR_MAX, G06_ATTR_OPTIMUM
    };
    for (size_t i = 0u; i < sizeof(attrs) / sizeof(attrs[0]); ++i) {
        const g06_attr_code code = attrs[i];
        if (current_has(context, code))
            parsed[code] = decimal_parse(current_value(context, code), &values[code]);
    }
    if (id == ARBOR_VIEW0_NATIVE_ELEMENT_PROGRESS) {
        const g06_decimal *maximum = parsed[G06_ATTR_MAX] ? &values[G06_ATTR_MAX] : &one;
        if (parsed[G06_ATTR_MAX] && decimal_compare(maximum, &zero) <= 0) {
            arbor_status status = add_attr_violation(context, UINT16_C(5), G06_ATTR_MAX);
            if (status.native != 0) return status;
        }
        if (parsed[G06_ATTR_VALUE] &&
            (decimal_compare(&values[G06_ATTR_VALUE], &zero) < 0 ||
             decimal_compare(&values[G06_ATTR_VALUE], maximum) > 0))
            return add_attr_violation(context, UINT16_C(5), G06_ATTR_VALUE);
        return ok_status();
    }
    if (id != ARBOR_VIEW0_NATIVE_ELEMENT_METER) return ok_status();
    const g06_decimal *minimum = parsed[G06_ATTR_MIN] ? &values[G06_ATTR_MIN] : &zero;
    const g06_decimal *maximum = parsed[G06_ATTR_MAX] ? &values[G06_ATTR_MAX] : &one;
    if (parsed[G06_ATTR_MAX] && decimal_compare(maximum, minimum) <= 0) {
        arbor_status status = add_attr_violation(context, UINT16_C(5), G06_ATTR_MAX);
        if (status.native != 0) return status;
    }
    if (parsed[G06_ATTR_VALUE] &&
        (decimal_compare(&values[G06_ATTR_VALUE], minimum) < 0 ||
         decimal_compare(&values[G06_ATTR_VALUE], maximum) > 0)) {
        arbor_status status = add_attr_violation(context, UINT16_C(5), G06_ATTR_VALUE);
        if (status.native != 0) return status;
    }
    if (parsed[G06_ATTR_LOW] &&
        (decimal_compare(&values[G06_ATTR_LOW], minimum) < 0 ||
         decimal_compare(&values[G06_ATTR_LOW], maximum) > 0)) {
        arbor_status status = add_attr_violation(context, UINT16_C(5), G06_ATTR_LOW);
        if (status.native != 0) return status;
    }
    if (parsed[G06_ATTR_HIGH] &&
        (decimal_compare(&values[G06_ATTR_HIGH], minimum) < 0 ||
         decimal_compare(&values[G06_ATTR_HIGH], maximum) > 0 ||
         (parsed[G06_ATTR_LOW] &&
          decimal_compare(&values[G06_ATTR_HIGH], &values[G06_ATTR_LOW]) < 0))) {
        arbor_status status = add_attr_violation(context, UINT16_C(5), G06_ATTR_HIGH);
        if (status.native != 0) return status;
    }
    if (parsed[G06_ATTR_OPTIMUM] &&
        (decimal_compare(&values[G06_ATTR_OPTIMUM], minimum) < 0 ||
         decimal_compare(&values[G06_ATTR_OPTIMUM], maximum) > 0))
        return add_attr_violation(context, UINT16_C(5), G06_ATTR_OPTIMUM);
    return ok_status();
}

static arbor_status evaluate_r5(g06_context *context, uint64_t state)
{
    arbor_status status;
    const uint64_t id = context->current.standard_element_id;
    if (id == ARBOR_VIEW0_NATIVE_ELEMENT_INPUT) {
        const bool number = state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_NUMBER;
        const bool range = state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_RANGE;
        status = validate_float_attr(context, G06_ATTR_MIN, number || range, false);
        if (status.native != 0) return status;
        status = validate_float_attr(context, G06_ATTR_MAX, number || range, false);
        if (status.native != 0) return status;
        status = validate_float_attr(context, G06_ATTR_VALUE, number || range, false);
        if (status.native != 0) return status;
        const uint64_t step_states =
            ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_DATE |
            ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_MONTH |
            ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_WEEK |
            ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TIME |
            ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_DATETIME_LOCAL |
            ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_NUMBER |
            ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_RANGE;
        status = validate_float_attr(context, G06_ATTR_STEP,
                                     input_state_in(state, step_states), true);
        if (status.native != 0) return status;
    }
    if (id == ARBOR_VIEW0_NATIVE_ELEMENT_METER) {
        const g06_attr_code attrs[] = {G06_ATTR_VALUE, G06_ATTR_MIN, G06_ATTR_LOW,
                                       G06_ATTR_HIGH, G06_ATTR_MAX, G06_ATTR_OPTIMUM};
        for (size_t i = 0u; i < sizeof(attrs) / sizeof(attrs[0]); ++i) {
            status = validate_float_attr(context, attrs[i], true, false);
            if (status.native != 0) return status;
        }
    }
    if (id == ARBOR_VIEW0_NATIVE_ELEMENT_PROGRESS) {
        status = validate_float_attr(context, G06_ATTR_VALUE, true, false);
        if (status.native != 0) return status;
        status = validate_float_attr(context, G06_ATTR_MAX, true, false);
        if (status.native != 0) return status;
    }
    return meter_progress_constraints(context);
}

static bool time_union_valid(arbor_span value, bool *year_branch)
{
    *year_branch = false;
    if (arbor_view0_native_g06_c0_month(value) ||
        arbor_view0_native_g06_c0_date(value) ||
        arbor_view0_native_g06_c0_yearless_date(value) ||
        arbor_view0_native_g06_c0_time(value) ||
        arbor_view0_native_g06_c0_local_datetime(value) ||
        arbor_view0_native_g06_c0_timezone(value) ||
        arbor_view0_native_g06_c0_global_datetime(value) ||
        arbor_view0_native_g06_c0_week(value) ||
        arbor_view0_native_g06_c0_duration(value)) return true;
    if (all_digits_nonzero_year(value)) { *year_branch = true; return true; }
    return false;
}

static uint16_t time_union_failure_rule(arbor_span value)
{
    uint64_t hyphens = 0u;
    uint64_t first_hyphen = UINT64_MAX;
    bool colon = false;
    bool separator = false;
    bool week = false;
    bool duration_hint = value.length != 0u && value.data[0] == (uint8_t)'P';
    for (uint64_t i = 0u; i < value.length; ++i) {
        const uint8_t c = value.data[i];
        if (c == (uint8_t)'-') {
            if (first_hyphen == UINT64_MAX) first_hyphen = i;
            ++hyphens;
        }
        if (c == (uint8_t)':') colon = true;
        if (c == (uint8_t)'T' || c == (uint8_t)' ') separator = true;
        if (c == (uint8_t)'W') week = true;
        if (i != 0u && (ascii_lower(c) == (uint8_t)'h' ||
                        ascii_lower(c) == (uint8_t)'d' ||
                        ascii_lower(c) == (uint8_t)'w' ||
                        ascii_lower(c) == (uint8_t)'s')) duration_hint = true;
    }
    if (duration_hint) return UINT16_C(14);
    if (week) return UINT16_C(13);
    if (separator) {
        if (value.length != 0u &&
            (value.data[value.length - 1u] == (uint8_t)'Z' || hyphens >= 3u ||
             (value.length >= 5u && value.data[value.length - 3u] == (uint8_t)':')))
            return UINT16_C(12);
        return UINT16_C(10);
    }
    if (value.length != 0u &&
        (value.data[0] == (uint8_t)'Z' || value.data[0] == (uint8_t)'+' ||
         (value.data[0] == (uint8_t)'-' && hyphens == 1u)))
        return UINT16_C(11);
    if (colon) return UINT16_C(9);
    if (hyphens >= 2u && value.length <= 7u) return UINT16_C(8);
    if (hyphens >= 2u) return UINT16_C(7);
    if (hyphens == 1u && first_hyphen <= 2u) return UINT16_C(8);
    return UINT16_C(6);
}

static arbor_status evaluate_time(g06_context *context)
{
    if (context->current.standard_element_id != ARBOR_VIEW0_NATIVE_ELEMENT_TIME)
        return ok_status();
    arbor_span value;
    const g06_source_attr *anchor = NULL;
    if (current_has(context, G06_ATTR_DATETIME)) {
        value = current_value(context, G06_ATTR_DATETIME);
        anchor = find_source_attr(context, context->current.source_offset, G06_ATTR_DATETIME);
        if (anchor == NULL) return err_status(EIO);
    } else {
        if (context->current.time_has_element_child) return suppress_prior_owner(context);
        value = (arbor_span){context->current.time_text, context->current.time_text_length};
    }
    arbor_status status = match_consumer(context);
    if (status.native != 0) return status;
    bool year = false;
    if (time_union_valid(value, &year)) {
        if (year) {
            if (context->evaluation.time_union_year_admission_count == UINT64_MAX)
                return err_status(EOVERFLOW);
            context->evaluation.time_union_year_admission_count += 1u;
        }
        return ok_status();
    }
    if (context->evaluation.time_union_fallback_count == UINT64_MAX)
        return err_status(EOVERFLOW);
    context->evaluation.time_union_fallback_count += 1u;
    const uint16_t rule = time_union_failure_rule(value);
    if (anchor != NULL)
        return add_violation(context, rule, anchor->source_offset, anchor->source_length);
    return add_violation(context, rule,
                         context->current.source_offset,
                         context->current.source_length);
}

static arbor_status evaluate_input_dates(g06_context *context, uint64_t state)
{
    if (context->current.standard_element_id != ARBOR_VIEW0_NATIVE_ELEMENT_INPUT)
        return ok_status();
    arbor_status status;
#define G06_DATE_STATE(STATE, RULE, VALIDATOR) do { \
    if (state == (STATE)) { \
        status = validate_datetime_attr(context, G06_ATTR_MIN, true, (RULE), (VALIDATOR)); \
        if (status.native != 0) return status; \
        status = validate_datetime_attr(context, G06_ATTR_MAX, true, (RULE), (VALIDATOR)); \
        if (status.native != 0) return status; \
        status = validate_datetime_attr(context, G06_ATTR_VALUE, true, (RULE), (VALIDATOR)); \
        if (status.native != 0) return status; \
    } \
} while (0)
    G06_DATE_STATE(ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_MONTH, UINT16_C(6), arbor_view0_native_g06_c0_month);
    G06_DATE_STATE(ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_DATE, UINT16_C(7), arbor_view0_native_g06_c0_date);
    G06_DATE_STATE(ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TIME, UINT16_C(9), arbor_view0_native_g06_c0_time);
    G06_DATE_STATE(ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_DATETIME_LOCAL, UINT16_C(10), arbor_view0_native_g06_c0_local_datetime);
    G06_DATE_STATE(ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_WEEK, UINT16_C(13), arbor_view0_native_g06_c0_week);
#undef G06_DATE_STATE
    return ok_status();
}

static arbor_status evaluate_r16(g06_context *context, uint64_t rel)
{
    const uint64_t id = context->current.standard_element_id;
    arbor_status status;
    struct token_policy { uint64_t element; g06_attr_code code; bool ci; bool conditional; };
    static const struct token_policy policies[] = {
        {ARBOR_VIEW0_NATIVE_ELEMENT_FORM, G06_ATTR_REL, true, false},
        {UINT64_C(0), G06_ATTR_CLASS, false, false},
        {ARBOR_VIEW0_NATIVE_ELEMENT_LINK, G06_ATTR_REL, true, false},
        {ARBOR_VIEW0_NATIVE_ELEMENT_LINK, G06_ATTR_SIZES, true, true},
        {ARBOR_VIEW0_NATIVE_ELEMENT_OUTPUT, G06_ATTR_FOR, false, false}
    };
    for (size_t i = 0u; i < sizeof(policies) / sizeof(policies[0]); ++i) {
        const bool element_match = policies[i].element == 0u
            ? id >= ARBOR_VIEW0_NATIVE_ELEMENT_HTML && id <= ARBOR_VIEW0_NATIVE_ELEMENT_CANVAS
            : id == policies[i].element;
        if (!element_match || !current_has(context, policies[i].code)) continue;
        if (policies[i].conditional && (rel & UINT64_C(24)) == 0u) {
            status = suppress_prior_owner(context);
        } else {
            status = match_consumer(context);
            if (status.native == 0 &&
                arbor_view0_native_g06_c0_space_tokens(
                    current_value(context, policies[i].code), true, policies[i].ci, NULL) !=
                    ARBOR_VIEW0_NATIVE_G06_C0_TOKEN_VALID)
                status = add_attr_violation(context, UINT16_C(16), policies[i].code);
        }
        if (status.native != 0) return status;
    }
    return ok_status();
}

static arbor_status evaluate_r17(g06_context *context, uint64_t state)
{
    const uint64_t id = context->current.standard_element_id;
    arbor_status status;
    if (id == ARBOR_VIEW0_NATIVE_ELEMENT_INPUT && current_has(context, G06_ATTR_ACCEPT)) {
        if (state != ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_FILE)
            status = suppress_prior_owner(context);
        else {
            status = match_consumer(context);
            if (status.native == 0) {
                const g06_comma_result result = valid_comma_value(
                    current_value(context, G06_ATTR_ACCEPT), valid_accept_token,
                    true, true);
                if (result == G06_COMMA_CAPACITY) status = err_status(ENOSPC);
                else if (result == G06_COMMA_INVALID)
                    status = add_attr_violation(context, UINT16_C(17), G06_ATTR_ACCEPT);
            }
        }
        if (status.native != 0) return status;
    }
    if (id == ARBOR_VIEW0_NATIVE_ELEMENT_INPUT && current_has(context, G06_ATTR_VALUE) &&
        state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_EMAIL &&
        current_has(context, G06_ATTR_MULTIPLE)) {
        status = match_consumer(context);
        if (status.native == 0 &&
            valid_comma_value(current_value(context, G06_ATTR_VALUE), valid_email,
                              true, false) != G06_COMMA_VALID)
            status = add_attr_violation(context, UINT16_C(17), G06_ATTR_VALUE);
        if (status.native != 0) return status;
    }
    if (id == ARBOR_VIEW0_NATIVE_ELEMENT_META && current_has(context, G06_ATTR_CONTENT)) {
        const bool keywords = current_has(context, G06_ATTR_NAME) &&
                              span_eq_ci_literal(current_value(context, G06_ATTR_NAME), "keywords");
        if (keywords) {
            status = match_consumer(context);
            if (status.native == 0 &&
                valid_comma_value(current_value(context, G06_ATTR_CONTENT), NULL,
                                  true, false) != G06_COMMA_VALID)
                status = add_attr_violation(context, UINT16_C(17), G06_ATTR_CONTENT);
            if (status.native != 0) return status;
        }
    }
    return ok_status();
}

static arbor_status source_attribute(
    void *opaque,
    const arbor_view0_native_source_attribute_observation *observation)
{
    if (opaque == NULL || observation == NULL) return err_status(EINVAL);
    g06_context *context = (g06_context *)opaque;
    const g06_attr_code code = attr_code(observation->local_name);
    if (code == G06_ATTR_NONE) return ok_status();
    if (observation->owner_source_offset == ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE ||
        observation->source_offset == ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE ||
        observation->source_length == 0u ||
        observation->source_offset > UINT32_MAX || observation->source_length > UINT32_MAX)
        return err_status(EIO);
    if (find_source_attr(context, observation->owner_source_offset, code) != NULL)
        return ok_status();
    if (context->source_attr_count >= ARBOR_VIEW0_NATIVE_G06_MAX_SOURCE_ATTRIBUTES)
        return err_status(ENOSPC);
    context->source_attrs[context->source_attr_count++] = (g06_source_attr){
        .owner_source_offset = observation->owner_source_offset,
        .source_offset = (uint32_t)observation->source_offset,
        .source_length = (uint32_t)observation->source_length,
        .code = (uint16_t)code
    };
    return ok_status();
}

static arbor_status traversal_enter(
    void *opaque, const arbor_view0_native_element_observation *observation)
{
    if (opaque == NULL || observation == NULL) return err_status(EINVAL);
    g06_context *context = (g06_context *)opaque;
    if (observation->depth >= UINT64_C(4098)) return err_status(ENOSPC);
    context->a_href_stack[observation->depth] = false;
    return ok_status();
}

static arbor_status element_begin(
    void *opaque, const arbor_view0_native_element_observation *observation)
{
    if (opaque == NULL || observation == NULL) return err_status(EINVAL);
    g06_context *context = (g06_context *)opaque;
    (void)memset(&context->current, 0, sizeof(context->current));
    context->current.standard_element_id = observation->standard_element_id;
    context->current.source_offset = observation->source_offset;
    context->current.source_length = observation->source_length;
    context->current.depth = observation->depth;
    return ok_status();
}

static arbor_status attribute(
    void *opaque, const arbor_view0_native_attribute_observation *observation)
{
    if (opaque == NULL || observation == NULL) return err_status(EINVAL);
    g06_context *context = (g06_context *)opaque;
    const g06_attr_code code = attr_code(observation->local_name);
    if (code == G06_ATTR_NONE || context->current.values[code].present) return ok_status();
    context->current.values[code] = (g06_value){
        .span = observation->value,
        .present = true
    };
    return ok_status();
}

static arbor_status direct_child(
    void *opaque, const arbor_view0_native_direct_child_observation *observation)
{
    if (opaque == NULL || observation == NULL) return err_status(EINVAL);
    g06_context *context = (g06_context *)opaque;
    if (context->current.standard_element_id != ARBOR_VIEW0_NATIVE_ELEMENT_TIME)
        return ok_status();
    if (current_has(context, G06_ATTR_DATETIME)) return ok_status();
    if (observation->kind == ARBOR_VIEW0_NATIVE_DIRECT_CHILD_ELEMENT) {
        context->current.time_has_element_child = true;
        return ok_status();
    }
    if (observation->kind != ARBOR_VIEW0_NATIVE_DIRECT_CHILD_TEXT)
        return ok_status();
    if (observation->text.length > ARBOR_VIEW0_NATIVE_G06_TIME_TEXT_CAP -
                                      context->current.time_text_length) {
        context->current.time_text_overflow = true;
        return ok_status();
    }
    if (observation->text.length != 0u && observation->text.data == NULL)
        return err_status(EIO);
    if (observation->text.length != 0u) {
        (void)memcpy(context->current.time_text + context->current.time_text_length,
                     observation->text.data, (size_t)observation->text.length);
        context->current.time_text_length += observation->text.length;
    }
    return ok_status();
}

static uint64_t current_rel_flags(const g06_context *context)
{
    if (!current_has(context, G06_ATTR_REL)) return 0u;
    const arbor_span rel = current_value(context, G06_ATTR_REL);
    uint64_t flags = 0u;
    if (token_contains_ci(rel, "stylesheet")) flags |= UINT64_C(1);
    if (token_contains_ci(rel, "preload")) flags |= UINT64_C(2);
    if (token_contains_ci(rel, "modulepreload")) flags |= UINT64_C(4);
    if (token_contains_ci(rel, "icon")) flags |= UINT64_C(8);
    if (token_contains_ci(rel, "apple-touch-icon")) flags |= UINT64_C(16);
    return flags;
}

static arbor_status element_complete(
    void *opaque, const arbor_view0_native_element_observation *observation)
{
    if (opaque == NULL || observation == NULL) return err_status(EINVAL);
    g06_context *context = (g06_context *)opaque;
    if (observation->standard_element_id != context->current.standard_element_id ||
        observation->source_offset != context->current.source_offset ||
        observation->depth != context->current.depth)
        return err_status(EIO);
    const arbor_span type = current_has(context, G06_ATTR_TYPE)
        ? current_value(context, G06_ATTR_TYPE) : (arbor_span){NULL, 0u};
    const uint64_t state = context->current.standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_INPUT
        ? input_state(type) : ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_NONE;
    const uint64_t rel = current_rel_flags(context);
    if (context->current.standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_TIME &&
        !current_has(context, G06_ATTR_DATETIME) &&
        !context->current.time_has_element_child && context->current.time_text_overflow)
        return err_status(ENOSPC);
    arbor_status status = evaluate_r1(context, state, rel);
    if (status.native != 0) return status;
    status = evaluate_r2(context, state, rel);
    if (status.native != 0) return status;
    status = evaluate_r3_r4(context, state, rel);
    if (status.native != 0) return status;
    status = evaluate_r5(context, state);
    if (status.native != 0) return status;
    status = evaluate_input_dates(context, state);
    if (status.native != 0) return status;
    status = evaluate_time(context);
    if (status.native != 0) return status;
    status = evaluate_r16(context, rel);
    if (status.native != 0) return status;
    status = evaluate_r17(context, state);
    if (status.native != 0) return status;

    if (context->current.standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_A &&
        current_has(context, G06_ATTR_HREF)) {
        if (context->current.depth >= UINT64_C(4098)) return err_status(EIO);
        context->a_href_stack[context->current.depth] = true;
        if (context->a_href_ancestor_count == UINT64_MAX) return err_status(EOVERFLOW);
        context->a_href_ancestor_count += 1u;
    }
    return ok_status();
}

static arbor_status traversal_leave(
    void *opaque, const arbor_view0_native_element_observation *observation)
{
    if (opaque == NULL || observation == NULL) return err_status(EINVAL);
    g06_context *context = (g06_context *)opaque;
    if (observation->depth >= UINT64_C(4098)) return err_status(EIO);
    if (context->a_href_stack[observation->depth]) {
        if (observation->standard_element_id != ARBOR_VIEW0_NATIVE_ELEMENT_A ||
            context->a_href_ancestor_count == 0u) return err_status(EIO);
        context->a_href_ancestor_count -= 1u;
        context->a_href_stack[observation->depth] = false;
    }
    return ok_status();
}

static arbor_status evaluate(
    arbor_span input,
    arbor_view0_native_g06_anchor *anchors,
    uint64_t anchor_capacity,
    bool collect_anchors,
    arbor_view0_native_g06_evaluation *evaluation_out)
{
    if (evaluation_out == NULL ||
        (collect_anchors && anchor_capacity != 0u && anchors == NULL))
        return err_status(EINVAL);
    g06_context context = {
        .anchors = anchors,
        .anchor_capacity = anchor_capacity,
        .collect_anchors = collect_anchors
    };
    const arbor_view0_native_semantic_observer observer = {
        .context = &context,
        .element_begin = element_begin,
        .attribute = attribute,
        .direct_child = direct_child,
        .element_complete = element_complete,
        .traversal_enter = traversal_enter,
        .traversal_leave = traversal_leave,
        .source_attribute = source_attribute
    };
    arbor_view0_native_parse_counts parse_counts = {0};
    arbor_view0_native_document_facts facts = {0};
    arbor_view0_native_observation_counts observation_counts = {0};
    arbor_status status = arbor_view0_native_lexbor_observe(
        input, &observer, &parse_counts, &facts, &observation_counts);
    if (status.native != 0) return status;
    if (context.a_href_ancestor_count != 0u) return err_status(EIO);
    if (collect_anchors && context.evaluation.diagnostic_count != anchor_capacity)
        return err_status(EIO);
    *evaluation_out = context.evaluation;
    return ok_status();
}

arbor_status arbor_view0_native_g06_measure(
    arbor_span input,
    arbor_view0_native_g06_evaluation *evaluation_out)
{
    return evaluate(input, NULL, 0u, false, evaluation_out);
}

arbor_status arbor_view0_native_g06_collect_anchors(
    arbor_span input,
    arbor_view0_native_g06_anchor *anchors,
    uint64_t anchor_capacity,
    arbor_view0_native_g06_evaluation *evaluation_out)
{
    return evaluate(input, anchors, anchor_capacity, true, evaluation_out);
}

void arbor_view0_native_g06_materialize_anchor(
    const arbor_view0_native_g06_anchor *anchor,
    uint64_t discovery_sequence,
    arbor_view0_native_diagnostic *diagnostic)
{
    static const char *const messages[] = {
        "Boolean attribute value does not satisfy its frozen consumer contract",
        "Enumerated attribute value is not an admitted keyword for its frozen consumer",
        "Signed-integer value does not satisfy the frozen authoring microsyntax",
        "Non-negative-integer value does not satisfy its frozen consumer contract",
        "Floating-point value does not satisfy its frozen consumer contract",
        "Month or classified time-union value does not satisfy the frozen microsyntax",
        "Date value does not satisfy the frozen date microsyntax",
        "Yearless-date value does not satisfy the frozen yearless-date microsyntax",
        "Time value does not satisfy the frozen time microsyntax",
        "Local date-time value does not satisfy the frozen local date-time microsyntax",
        "Time-zone value does not satisfy the frozen time-zone microsyntax",
        "Global date-time value does not satisfy the frozen global date-time microsyntax",
        "Week value does not satisfy the frozen week microsyntax",
        "Duration value does not satisfy the frozen duration microsyntax",
        "Date-with-optional-time value does not satisfy the frozen microsyntax",
        "Space-separated token value violates its frozen consuming policy",
        "Comma-separated token value violates its frozen consuming policy"
    };
    if (anchor == NULL || diagnostic == NULL || anchor->rule_ordinal == 0u ||
        anchor->rule_ordinal > ARBOR_VIEW0_NATIVE_G06_RULE_COUNT) return;
    const arbor_view0_native_g06_c0_rule_meta *meta =
        arbor_view0_native_g06_c0_rule_at((uint64_t)anchor->rule_ordinal - 1u);
    if (meta == NULL) return;
    (void)memset(diagnostic, 0, sizeof(*diagnostic));
    diagnostic->rule_id = meta->rule_id;
    diagnostic->byte_offset = (uint64_t)anchor->source.byte_offset;
    diagnostic->source_length = (uint64_t)anchor->source.source_length;
    diagnostic->discovery_sequence = discovery_sequence;
    diagnostic->severity = (uint32_t)ARBOR_VIEW0_NATIVE_SEVERITY_ERROR;
    diagnostic->origin = (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_AUTHORING;
    (void)memcpy(diagnostic->symbolic_name, meta->rule_symbol,
                 strlen(meta->rule_symbol) + 1u);
    (void)memcpy(diagnostic->message, messages[anchor->rule_ordinal - 1u],
                 strlen(messages[anchor->rule_ordinal - 1u]) + 1u);
}
