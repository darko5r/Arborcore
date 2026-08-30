#include "g10.h"

#include "g05_c0.h"
#include "g06_c0.h"

#include <lexbor/core/mraw.h>

#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef enum g10_attr_code {
    G10_ATTR_NONE = 0,
    G10_ATTR_ID,
    G10_ATTR_FORM,
    G10_ATTR_FOR,
    G10_ATTR_TYPE,
    G10_ATTR_NAME,
    G10_ATTR_ACTION,
    G10_ATTR_METHOD,
    G10_ATTR_ENCTYPE,
    G10_ATTR_TARGET,
    G10_ATTR_ACCEPT_CHARSET,
    G10_ATTR_FORMACTION,
    G10_ATTR_FORMMETHOD,
    G10_ATTR_FORMENCTYPE,
    G10_ATTR_FORMTARGET,
    G10_ATTR_FORMNOVALIDATE,
    G10_ATTR_NOVALIDATE,
    G10_ATTR_COMMANDFOR,
    G10_ATTR_VALUE,
    G10_ATTR_MIN,
    G10_ATTR_MAX,
    G10_ATTR_STEP,
    G10_ATTR_CHECKED,
    G10_ATTR_LIST,
    G10_ATTR_PATTERN,
    G10_ATTR_PLACEHOLDER,
    G10_ATTR_MAXLENGTH,
    G10_ATTR_MINLENGTH,
    G10_ATTR_SIZE,
    G10_ATTR_MULTIPLE,
    G10_ATTR_REQUIRED,
    G10_ATTR_SRC,
    G10_ATTR_ALT,
    G10_ATTR_ACCEPT,
    G10_ATTR_SELECTED,
    G10_ATTR_LABEL,
    G10_ATTR_ROWS,
    G10_ATTR_COLS,
    G10_ATTR_WRAP,
    G10_ATTR_DIRNAME,
    G10_ATTR_LOW,
    G10_ATTR_HIGH,
    G10_ATTR_OPTIMUM,
    G10_ATTR_DISABLED,
    G10_ATTR_READONLY,
    G10_ATTR_AUTOFOCUS,
    G10_ATTR_POPOVER,
    G10_ATTR_AUTOCOMPLETE,
    G10_ATTR__COUNT
} g10_attr_code;

typedef struct g10_value {
    arbor_span span;
    bool present;
} g10_value;

typedef struct g10_node g10_node;
typedef struct g10_source_attr g10_source_attr;
typedef struct g10_selectedcontent_source g10_selectedcontent_source;

struct g10_node {
    g10_node *next;
    g10_node *parent;
    g10_node *first_child;
    g10_node *last_child;
    g10_node *next_sibling;
    uint64_t element_id;
    uint64_t source_offset;
    uint64_t source_length;
    uint64_t depth;
    uint64_t input_state;
    bool autofocus_seen;
    g10_value values[G10_ATTR__COUNT];
};

struct g10_source_attr {
    g10_source_attr *next;
    uint64_t owner_source_offset;
    uint32_t source_offset;
    uint32_t source_length;
    uint16_t code;
};

struct g10_selectedcontent_source {
    g10_selectedcontent_source *next;
    uint64_t source_offset;
};

typedef struct g10_frame {
    g10_node *node;
    uint64_t source_offset;
    uint64_t depth;
} g10_frame;

typedef struct g10_context {
    lexbor_mraw_t *arena;
    arbor_view0_native_v1n2_g10_anchor *anchors;
    uint64_t anchor_capacity;
    bool collect;
    arbor_view0_native_v1n2_g10_evaluation evaluation;
    g10_node *nodes;
    g10_node *nodes_tail;
    g10_node *current;
    g10_source_attr *source_attrs;
    g10_source_attr *source_attrs_tail;
    g10_selectedcontent_source *selectedcontent_sources;
    g10_selectedcontent_source *selectedcontent_sources_tail;
    g10_frame frames[ARBOR_VIEW0_NATIVE_V1N2_G10_MAX_DEPTH];
    uint64_t frame_count;
} g10_context;

static arbor_status ok_status(void) { return arbor_status_from_native(0); }
static arbor_status err_status(int value) { return arbor_status_from_native(-(int64_t)value); }

static void *support_calloc(g10_context *context, size_t size) {
    return arbor_view0_native_v1n2_g10_support_calloc(context->arena, size);
}

static uint8_t ascii_lower(uint8_t byte) {
    return byte >= (uint8_t)'A' && byte <= (uint8_t)'Z'
        ? (uint8_t)(byte + ((uint8_t)'a' - (uint8_t)'A')) : byte;
}

static bool ascii_space(uint8_t byte) {
    return byte == UINT8_C(0x09) || byte == UINT8_C(0x0a) ||
           byte == UINT8_C(0x0c) || byte == UINT8_C(0x0d) ||
           byte == UINT8_C(0x20);
}

static bool span_eq(arbor_span left, arbor_span right) {
    return left.length == right.length &&
        (left.length == 0u || (left.data != NULL && right.data != NULL &&
         memcmp(left.data, right.data, (size_t)left.length) == 0));
}

static bool span_eq_ci(arbor_span span, const char *literal) {
    const size_t length = strlen(literal);
    if (span.data == NULL || span.length != (uint64_t)length) return false;
    for (size_t index = 0u; index < length; ++index)
        if (ascii_lower(span.data[index]) != ascii_lower((uint8_t)literal[index])) return false;
    return true;
}

static bool span_eq_ascii_ci(arbor_span left, arbor_span right) {
    if (left.length != right.length || left.data == NULL || right.data == NULL) return false;
    for (uint64_t index = 0u; index < left.length; ++index)
        if (ascii_lower(left.data[index]) != ascii_lower(right.data[index])) return false;
    return true;
}

static bool span_starts_ci(arbor_span span, const char *literal) {
    const size_t length = strlen(literal);
    if (span.data == NULL || span.length < (uint64_t)length) return false;
    for (size_t index = 0u; index < length; ++index)
        if (ascii_lower(span.data[index]) != ascii_lower((uint8_t)literal[index])) return false;
    return true;
}

static bool span_empty(arbor_span span) { return span.length == 0u; }

static bool span_has_cr_lf(arbor_span span) {
    if (span.data == NULL) return false;
    for (uint64_t index = 0u; index < span.length; ++index)
        if (span.data[index] == (uint8_t)'\r' || span.data[index] == (uint8_t)'\n') return true;
    return false;
}

static g10_attr_code attr_code(arbor_span name) {
    static const struct { const char *name; g10_attr_code code; } entries[] = {
        {"id",G10_ATTR_ID},{"form",G10_ATTR_FORM},{"for",G10_ATTR_FOR},
        {"type",G10_ATTR_TYPE},{"name",G10_ATTR_NAME},{"action",G10_ATTR_ACTION},
        {"method",G10_ATTR_METHOD},{"enctype",G10_ATTR_ENCTYPE},{"target",G10_ATTR_TARGET},
        {"accept-charset",G10_ATTR_ACCEPT_CHARSET},{"formaction",G10_ATTR_FORMACTION},
        {"formmethod",G10_ATTR_FORMMETHOD},{"formenctype",G10_ATTR_FORMENCTYPE},
        {"formtarget",G10_ATTR_FORMTARGET},{"formnovalidate",G10_ATTR_FORMNOVALIDATE},
        {"novalidate",G10_ATTR_NOVALIDATE},{"commandfor",G10_ATTR_COMMANDFOR},
        {"value",G10_ATTR_VALUE},{"min",G10_ATTR_MIN},{"max",G10_ATTR_MAX},
        {"step",G10_ATTR_STEP},{"checked",G10_ATTR_CHECKED},{"list",G10_ATTR_LIST},
        {"pattern",G10_ATTR_PATTERN},{"placeholder",G10_ATTR_PLACEHOLDER},
        {"maxlength",G10_ATTR_MAXLENGTH},{"minlength",G10_ATTR_MINLENGTH},
        {"size",G10_ATTR_SIZE},{"multiple",G10_ATTR_MULTIPLE},{"required",G10_ATTR_REQUIRED},
        {"src",G10_ATTR_SRC},{"alt",G10_ATTR_ALT},{"accept",G10_ATTR_ACCEPT},
        {"selected",G10_ATTR_SELECTED},{"label",G10_ATTR_LABEL},{"rows",G10_ATTR_ROWS},
        {"cols",G10_ATTR_COLS},{"wrap",G10_ATTR_WRAP},{"dirname",G10_ATTR_DIRNAME},
        {"low",G10_ATTR_LOW},{"high",G10_ATTR_HIGH},{"optimum",G10_ATTR_OPTIMUM},
        {"disabled",G10_ATTR_DISABLED},{"readonly",G10_ATTR_READONLY},
        {"autofocus",G10_ATTR_AUTOFOCUS},{"popover",G10_ATTR_POPOVER},
        {"autocomplete",G10_ATTR_AUTOCOMPLETE}
    };
    for (size_t index = 0u; index < sizeof(entries) / sizeof(entries[0]); ++index)
        if (span_eq_ci(name, entries[index].name)) return entries[index].code;
    return G10_ATTR_NONE;
}

static bool parse_u64(arbor_span span, uint64_t *out) {
    if (out == NULL || span.data == NULL || span.length == 0u) return false;
    uint64_t value = 0u;
    for (uint64_t index = 0u; index < span.length; ++index) {
        const uint8_t byte = span.data[index];
        if (byte < (uint8_t)'0' || byte > (uint8_t)'9') return false;
        const uint64_t digit = (uint64_t)(byte - (uint8_t)'0');
        if (value > (UINT64_MAX - digit) / UINT64_C(10)) return false;
        value = value * UINT64_C(10) + digit;
    }
    *out = value;
    return true;
}

static bool parse_number(arbor_span span, long double *out) {
    if (span.data == NULL || span.length == 0u || out == NULL) return false;
    uint64_t index = 0u;
    bool negative = false;
    if (span.data[index] == (uint8_t)'+' || span.data[index] == (uint8_t)'-') {
        negative = span.data[index] == (uint8_t)'-';
        if (++index == span.length) return false;
    }
    bool digit_seen = false;
    long double value = 0.0L;
    while (index < span.length && span.data[index] >= (uint8_t)'0' &&
           span.data[index] <= (uint8_t)'9') {
        digit_seen = true;
        value = value * 10.0L + (long double)(span.data[index] - (uint8_t)'0');
        ++index;
    }
    if (index < span.length && span.data[index] == (uint8_t)'.') {
        ++index;
        long double scale = 0.1L;
        while (index < span.length && span.data[index] >= (uint8_t)'0' &&
               span.data[index] <= (uint8_t)'9') {
            digit_seen = true;
            value += (long double)(span.data[index] - (uint8_t)'0') * scale;
            scale *= 0.1L;
            ++index;
        }
    }
    if (!digit_seen) return false;
    int exponent = 0;
    bool exponent_negative = false;
    if (index < span.length && (span.data[index] == (uint8_t)'e' ||
                                span.data[index] == (uint8_t)'E')) {
        ++index;
        if (index < span.length && (span.data[index] == (uint8_t)'+' ||
                                    span.data[index] == (uint8_t)'-')) {
            exponent_negative = span.data[index] == (uint8_t)'-';
            ++index;
        }
        const uint64_t exponent_begin = index;
        while (index < span.length && span.data[index] >= (uint8_t)'0' &&
               span.data[index] <= (uint8_t)'9') {
            if (exponent < 1000) exponent = exponent * 10 + (int)(span.data[index] - (uint8_t)'0');
            ++index;
        }
        if (index == exponent_begin) return false;
    }
    if (index != span.length || exponent > 4930) return false;
    while (exponent-- > 0) value = exponent_negative ? value / 10.0L : value * 10.0L;
    *out = negative ? -value : value;
    return true;
}

static arbor_span trim_ascii(arbor_span span) {
    uint64_t begin = 0u;
    uint64_t end = span.length;
    while (begin < end && ascii_space(span.data[begin])) ++begin;
    while (end > begin && ascii_space(span.data[end - 1u])) --end;
    return (arbor_span){span.data == NULL ? NULL : span.data + begin, end - begin};
}

static bool ascii_digit(uint8_t byte) {
    return byte >= (uint8_t)'0' && byte <= (uint8_t)'9';
}

static bool mime_token_char(uint8_t byte) {
    if (byte <= UINT8_C(0x20) || byte >= UINT8_C(0x7f)) return false;
    switch (byte) {
        case '(': case ')': case '<': case '>': case '@': case ',': case ';':
        case ':': case '\\': case '"': case '/': case '[': case ']': case '?':
        case '=': case '{': case '}':
            return false;
        default:
            return true;
    }
}

static bool valid_accept_token(arbor_span token) {
    if (token.data == NULL || token.length == 0u) return false;
    if (token.data[0] == (uint8_t)'.') {
        if (token.length == 1u) return false;
        for (uint64_t index = 1u; index < token.length; ++index)
            if (token.data[index] == (uint8_t)'.' || ascii_space(token.data[index])) return false;
        return true;
    }
    if (span_eq_ci(token, "audio/*") || span_eq_ci(token, "video/*") ||
        span_eq_ci(token, "image/*")) return true;
    uint64_t slash = UINT64_MAX;
    for (uint64_t index = 0u; index < token.length; ++index) {
        if (token.data[index] == (uint8_t)'/') {
            if (slash != UINT64_MAX) return false;
            slash = index;
        } else if (!mime_token_char(token.data[index])) return false;
    }
    return slash != UINT64_MAX && slash != 0u && slash + 1u < token.length;
}

static bool valid_accept(arbor_span value) {
    if (value.data == NULL || value.length == 0u) return false;
    uint64_t cursor = 0u;
    while (cursor <= value.length) {
        const uint64_t begin = cursor;
        while (cursor < value.length && value.data[cursor] != (uint8_t)',') ++cursor;
        const arbor_span token = trim_ascii(
            (arbor_span){value.data + begin, cursor - begin});
        if (!valid_accept_token(token)) return false;
        uint64_t prior_cursor = 0u;
        while (prior_cursor < begin) {
            const uint64_t prior_begin = prior_cursor;
            while (prior_cursor < begin && value.data[prior_cursor] != (uint8_t)',')
                ++prior_cursor;
            const arbor_span prior = trim_ascii(
                (arbor_span){value.data + prior_begin, prior_cursor - prior_begin});
            if (span_eq_ascii_ci(token, prior)) return false;
            ++prior_cursor;
        }
        if (cursor == value.length) return true;
        ++cursor;
    }
    return false;
}

static bool parse_digits(arbor_span span, uint64_t begin, uint64_t count, uint64_t *out) {
    if (out == NULL || span.data == NULL || begin > span.length ||
        count > span.length - begin || count == 0u) return false;
    uint64_t value = 0u;
    for (uint64_t index = 0u; index < count; ++index) {
        const uint8_t byte = span.data[begin + index];
        if (!ascii_digit(byte)) return false;
        value = value * UINT64_C(10) + (uint64_t)(byte - (uint8_t)'0');
    }
    *out = value;
    return true;
}

static bool leap_year(uint64_t year) {
    return year % UINT64_C(4) == 0u &&
        (year % UINT64_C(100) != 0u || year % UINT64_C(400) == 0u);
}

static uint64_t month_days(uint64_t year, uint64_t month) {
    static const uint8_t days[] = {31u,28u,31u,30u,31u,30u,31u,31u,30u,31u,30u,31u};
    if (month == 0u || month > 12u) return 0u;
    if (month == 2u && leap_year(year)) return 29u;
    return (uint64_t)days[month - 1u];
}

typedef struct g10_decimal {
    uint64_t coefficient;
    int64_t exponent10;
    bool negative;
} g10_decimal;

static arbor_status checked_add_u64(uint64_t left, uint64_t right, uint64_t *out) {
    if (out == NULL) return err_status(EINVAL);
    if (left > UINT64_MAX - right) return err_status(EOVERFLOW);
    *out = left + right;
    return ok_status();
}

static arbor_status checked_mul_u64(uint64_t left, uint64_t right, uint64_t *out) {
    if (out == NULL) return err_status(EINVAL);
    if (left != 0u && right > UINT64_MAX / left) return err_status(EOVERFLOW);
    *out = left * right;
    return ok_status();
}

static void decimal_normalize(g10_decimal *value) {
    if (value->coefficient == 0u) {
        value->negative = false;
        value->exponent10 = 0;
        return;
    }
    while (value->coefficient % UINT64_C(10) == 0u && value->exponent10 < INT64_MAX) {
        value->coefficient /= UINT64_C(10);
        value->exponent10 += INT64_C(1);
    }
}

static arbor_status decimal_parse(arbor_span span, g10_decimal *out, bool *valid) {
    if (out == NULL || valid == NULL) return err_status(EINVAL);
    *valid = false;
    if (!arbor_view0_native_g06_c0_floating_point(span)) return ok_status();
    uint64_t begin = 0u;
    bool negative = false;
    if (span.data[begin] == (uint8_t)'-') { negative = true; ++begin; }
    uint64_t exponent_at = span.length;
    uint64_t decimal_at = span.length;
    for (uint64_t index = begin; index < span.length; ++index) {
        if (span.data[index] == (uint8_t)'.') decimal_at = index;
        if (span.data[index] == (uint8_t)'e' || span.data[index] == (uint8_t)'E') {
            exponent_at = index;
            break;
        }
    }
    const uint64_t mantissa_end = exponent_at;
    uint64_t first_nonzero = mantissa_end;
    uint64_t last_nonzero = mantissa_end;
    uint64_t digit_count = 0u;
    uint64_t fraction_digits = 0u;
    for (uint64_t index = begin; index < mantissa_end; ++index) {
        if (span.data[index] == (uint8_t)'.') continue;
        if (span.data[index] != (uint8_t)'0') {
            if (first_nonzero == mantissa_end) first_nonzero = index;
            last_nonzero = index;
        }
        digit_count += 1u;
        if (decimal_at != span.length && index > decimal_at) fraction_digits += 1u;
    }
    int64_t explicit_exponent = 0;
    if (exponent_at != span.length) {
        uint64_t index = exponent_at + 1u;
        bool exponent_negative = false;
        if (span.data[index] == (uint8_t)'+' || span.data[index] == (uint8_t)'-') {
            exponent_negative = span.data[index] == (uint8_t)'-';
            ++index;
        }
        uint64_t magnitude = 0u;
        for (; index < span.length; ++index) {
            const uint64_t digit = (uint64_t)(span.data[index] - (uint8_t)'0');
            if (magnitude > (UINT64_MAX - digit) / UINT64_C(10))
                return err_status(EOVERFLOW);
            magnitude = magnitude * UINT64_C(10) + digit;
        }
        if (magnitude > (uint64_t)INT64_MAX) return err_status(EOVERFLOW);
        explicit_exponent = exponent_negative ? -(int64_t)magnitude : (int64_t)magnitude;
    }
    if (first_nonzero == mantissa_end) {
        *out = (g10_decimal){0};
        *valid = true;
        return ok_status();
    }
    uint64_t trailing_zeros = 0u;
    bool after_last = false;
    for (uint64_t index = begin; index < mantissa_end; ++index) {
        if (span.data[index] == (uint8_t)'.') continue;
        if (after_last) trailing_zeros += 1u;
        if (index == last_nonzero) after_last = true;
    }
    uint64_t coefficient = 0u;
    for (uint64_t index = first_nonzero; index <= last_nonzero; ++index) {
        if (span.data[index] == (uint8_t)'.') continue;
        const uint64_t digit = (uint64_t)(span.data[index] - (uint8_t)'0');
        if (coefficient > (UINT64_MAX - digit) / UINT64_C(10))
            return err_status(EOVERFLOW);
        coefficient = coefficient * UINT64_C(10) + digit;
    }
    (void)digit_count;
    if (fraction_digits > (uint64_t)INT64_MAX || trailing_zeros > (uint64_t)INT64_MAX)
        return err_status(EOVERFLOW);
    const int64_t adjustment = (int64_t)trailing_zeros - (int64_t)fraction_digits;
    if ((adjustment > 0 && explicit_exponent > INT64_MAX - adjustment) ||
        (adjustment < 0 && explicit_exponent < INT64_MIN - adjustment))
        return err_status(EOVERFLOW);
    *out = (g10_decimal){coefficient, explicit_exponent + adjustment, negative};
    decimal_normalize(out);
    *valid = true;
    return ok_status();
}

static arbor_status decimal_scaled_coefficient(
    const g10_decimal *value, int64_t exponent10, uint64_t *out) {
    if (value == NULL || out == NULL || exponent10 > value->exponent10)
        return err_status(EINVAL);
    if (value->coefficient == 0u) { *out = 0u; return ok_status(); }
    const uint64_t distance =
        (uint64_t)value->exponent10 - (uint64_t)exponent10;
    if (distance > UINT64_C(19)) return err_status(EOVERFLOW);
    uint64_t result = value->coefficient;
    for (uint64_t index = 0u; index < distance; ++index) {
        arbor_status status = checked_mul_u64(result, UINT64_C(10), &result);
        if (status.native != 0) return status;
    }
    *out = result;
    return ok_status();
}

static arbor_status decimal_compare(
    const g10_decimal *left, const g10_decimal *right, int *comparison) {
    if (left == NULL || right == NULL || comparison == NULL) return err_status(EINVAL);
    if (left->negative != right->negative) {
        *comparison = left->negative ? -1 : 1;
        if (left->coefficient == 0u && right->coefficient == 0u) *comparison = 0;
        return ok_status();
    }
    const int64_t exponent10 = left->exponent10 < right->exponent10
        ? left->exponent10 : right->exponent10;
    uint64_t left_scaled = 0u, right_scaled = 0u;
    arbor_status status = decimal_scaled_coefficient(left, exponent10, &left_scaled);
    if (status.native != 0) return status;
    status = decimal_scaled_coefficient(right, exponent10, &right_scaled);
    if (status.native != 0) return status;
    int result = left_scaled < right_scaled ? -1 : left_scaled > right_scaled ? 1 : 0;
    *comparison = left->negative ? -result : result;
    return ok_status();
}

static arbor_status decimal_add_unsigned(
    const g10_decimal *left, const g10_decimal *right, g10_decimal *out) {
    if (left == NULL || right == NULL || out == NULL || left->negative || right->negative)
        return err_status(EINVAL);
    const int64_t exponent10 = left->exponent10 < right->exponent10
        ? left->exponent10 : right->exponent10;
    uint64_t left_scaled = 0u, right_scaled = 0u, sum = 0u;
    arbor_status status = decimal_scaled_coefficient(left, exponent10, &left_scaled);
    if (status.native != 0) return status;
    status = decimal_scaled_coefficient(right, exponent10, &right_scaled);
    if (status.native != 0) return status;
    status = checked_add_u64(left_scaled, right_scaled, &sum);
    if (status.native != 0) return status;
    *out = (g10_decimal){sum, exponent10, false};
    decimal_normalize(out);
    return ok_status();
}

static arbor_status decimal_abs_difference(
    const g10_decimal *left, const g10_decimal *right, g10_decimal *out) {
    if (left == NULL || right == NULL || out == NULL) return err_status(EINVAL);
    const int64_t exponent10 = left->exponent10 < right->exponent10
        ? left->exponent10 : right->exponent10;
    uint64_t left_scaled = 0u, right_scaled = 0u;
    arbor_status status = decimal_scaled_coefficient(left, exponent10, &left_scaled);
    if (status.native != 0) return status;
    status = decimal_scaled_coefficient(right, exponent10, &right_scaled);
    if (status.native != 0) return status;
    uint64_t difference = 0u;
    if (left->negative == right->negative)
        difference = left_scaled > right_scaled
            ? left_scaled - right_scaled : right_scaled - left_scaled;
    else {
        status = checked_add_u64(left_scaled, right_scaled, &difference);
        if (status.native != 0) return status;
    }
    *out = (g10_decimal){difference, exponent10, false};
    decimal_normalize(out);
    return ok_status();
}

static arbor_status decimal_multiply_u64(
    const g10_decimal *value, uint64_t factor, g10_decimal *out) {
    if (value == NULL || out == NULL) return err_status(EINVAL);
    uint64_t coefficient = 0u;
    arbor_status status = checked_mul_u64(value->coefficient, factor, &coefficient);
    if (status.native != 0) return status;
    *out = (g10_decimal){coefficient, value->exponent10, value->negative};
    decimal_normalize(out);
    return ok_status();
}

static arbor_status decimal_modulo_nonzero(
    const g10_decimal *difference, const g10_decimal *step, bool *mismatch) {
    if (difference == NULL || step == NULL || mismatch == NULL ||
        difference->negative || step->negative || step->coefficient == 0u)
        return err_status(EINVAL);
    const int64_t exponent10 = difference->exponent10 < step->exponent10
        ? difference->exponent10 : step->exponent10;
    uint64_t difference_scaled = 0u, step_scaled = 0u;
    arbor_status status = decimal_scaled_coefficient(
        difference, exponent10, &difference_scaled);
    if (status.native != 0) return status;
    status = decimal_scaled_coefficient(step, exponent10, &step_scaled);
    if (status.native != 0) return status;
    *mismatch = difference_scaled % step_scaled != 0u;
    return ok_status();
}

static arbor_status parse_year_prefix_checked(
    arbor_span span, uint64_t separator, uint64_t *year_out) {
    if (span.data == NULL || year_out == NULL || separator < 4u ||
        separator >= span.length || span.data[separator] != (uint8_t)'-')
        return err_status(EINVAL);
    uint64_t year = 0u;
    for (uint64_t index = 0u; index < separator; ++index) {
        const uint64_t digit = (uint64_t)(span.data[index] - (uint8_t)'0');
        if (year > (UINT64_MAX - digit) / UINT64_C(10)) return err_status(EOVERFLOW);
        year = year * UINT64_C(10) + digit;
    }
    *year_out = year;
    return ok_status();
}

static arbor_status date_ordinal_parts(
    uint64_t year, uint64_t month, uint64_t day, uint64_t *out) {
    if (year == 0u || month == 0u || month > 12u || day == 0u || out == NULL)
        return err_status(EINVAL);
    const uint64_t prior_year = year - 1u;
    uint64_t ordinal = 0u;
    arbor_status status = checked_mul_u64(prior_year, UINT64_C(365), &ordinal);
    if (status.native != 0) return status;
    status = checked_add_u64(ordinal, prior_year / UINT64_C(4), &ordinal);
    if (status.native != 0) return status;
    ordinal -= prior_year / UINT64_C(100);
    status = checked_add_u64(ordinal, prior_year / UINT64_C(400), &ordinal);
    if (status.native != 0) return status;
    for (uint64_t current = 1u; current < month; ++current) {
        status = checked_add_u64(ordinal, month_days(year, current), &ordinal);
        if (status.native != 0) return status;
    }
    status = checked_add_u64(ordinal, day - 1u, &ordinal);
    if (status.native != 0) return status;
    *out = ordinal;
    return ok_status();
}

static arbor_status date_ordinal_checked(arbor_span span, uint64_t *out, bool *valid) {
    if (out == NULL || valid == NULL) return err_status(EINVAL);
    *valid = false;
    if (!arbor_view0_native_g06_c0_date(span)) return ok_status();
    uint64_t separator = 0u;
    while (separator < span.length && span.data[separator] != (uint8_t)'-') ++separator;
    uint64_t year = 0u, month = 0u, day = 0u;
    arbor_status status = parse_year_prefix_checked(span, separator, &year);
    if (status.native != 0) return status;
    if (!parse_digits(span, separator + 1u, 2u, &month) ||
        !parse_digits(span, separator + 4u, 2u, &day)) return err_status(EIO);
    status = date_ordinal_parts(year, month, day, out);
    if (status.native != 0) return status;
    *valid = true;
    return ok_status();
}

static arbor_status month_ordinal_checked(arbor_span span, uint64_t *out, bool *valid) {
    if (out == NULL || valid == NULL) return err_status(EINVAL);
    *valid = false;
    if (!arbor_view0_native_g06_c0_month(span)) return ok_status();
    uint64_t separator = 0u;
    while (separator < span.length && span.data[separator] != (uint8_t)'-') ++separator;
    uint64_t year = 0u, month = 0u, ordinal = 0u;
    arbor_status status = parse_year_prefix_checked(span, separator, &year);
    if (status.native != 0) return status;
    if (!parse_digits(span, separator + 1u, 2u, &month)) return err_status(EIO);
    status = checked_mul_u64(year - 1u, UINT64_C(12), &ordinal);
    if (status.native != 0) return status;
    status = checked_add_u64(ordinal, month - 1u, &ordinal);
    if (status.native != 0) return status;
    *out = ordinal;
    *valid = true;
    return ok_status();
}

static arbor_status week_ordinal_checked(arbor_span span, uint64_t *out, bool *valid) {
    if (out == NULL || valid == NULL) return err_status(EINVAL);
    *valid = false;
    if (!arbor_view0_native_g06_c0_week(span)) return ok_status();
    uint64_t separator = 0u;
    while (separator < span.length && span.data[separator] != (uint8_t)'-') ++separator;
    uint64_t year = 0u, week = 0u, january_four = 0u, offset = 0u, result = 0u;
    arbor_status status = parse_year_prefix_checked(span, separator, &year);
    if (status.native != 0) return status;
    if (!parse_digits(span, separator + 2u, 2u, &week)) return err_status(EIO);
    status = date_ordinal_parts(year, UINT64_C(1), UINT64_C(4), &january_four);
    if (status.native != 0) return status;
    const uint64_t weekday = january_four % UINT64_C(7);
    status = checked_mul_u64(week - 1u, UINT64_C(7), &offset);
    if (status.native != 0) return status;
    status = checked_add_u64(january_four - weekday, offset, &result);
    if (status.native != 0) return status;
    *out = result;
    *valid = true;
    return ok_status();
}

static arbor_status time_decimal(arbor_span span, g10_decimal *out, bool *valid) {
    if (out == NULL || valid == NULL) return err_status(EINVAL);
    *valid = false;
    if (!arbor_view0_native_g06_c0_time(span)) return ok_status();
    uint64_t hour = 0u, minute = 0u;
    if (!parse_digits(span, 0u, 2u, &hour) || !parse_digits(span, 3u, 2u, &minute))
        return err_status(EIO);
    g10_decimal prefix = {hour * UINT64_C(3600) + minute * UINT64_C(60), 0, false};
    g10_decimal seconds = {0};
    bool seconds_valid = true;
    if (span.length > 5u) {
        const arbor_span seconds_span = {span.data + 6u, span.length - 6u};
        arbor_status status = decimal_parse(seconds_span, &seconds, &seconds_valid);
        if (status.native != 0) return status;
        if (!seconds_valid) return err_status(EIO);
    }
    arbor_status status = decimal_add_unsigned(&prefix, &seconds, out);
    if (status.native != 0) return status;
    *valid = true;
    return ok_status();
}

static arbor_status domain_decimal(
    uint64_t state, arbor_span span, g10_decimal *out, bool *valid) {
    if (out == NULL || valid == NULL) return err_status(EINVAL);
    uint64_t ordinal = 0u;
    arbor_status status = ok_status();
    if (state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_DATE) {
        status = date_ordinal_checked(span, &ordinal, valid);
        if (status.native == 0 && *valid) *out = (g10_decimal){ordinal, 0, false};
        return status;
    }
    if (state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_MONTH) {
        status = month_ordinal_checked(span, &ordinal, valid);
        if (status.native == 0 && *valid) *out = (g10_decimal){ordinal, 0, false};
        return status;
    }
    if (state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_WEEK) {
        status = week_ordinal_checked(span, &ordinal, valid);
        if (status.native == 0 && *valid) *out = (g10_decimal){ordinal, 0, false};
        return status;
    }
    if (state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TIME)
        return time_decimal(span, out, valid);
    if (state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_DATETIME_LOCAL) {
        *valid = false;
        if (!arbor_view0_native_g06_c0_local_datetime(span)) return ok_status();
        uint64_t split = 0u;
        while (split < span.length && span.data[split] != (uint8_t)'T' &&
               span.data[split] != (uint8_t)' ') ++split;
        const arbor_span date = {span.data, split};
        const arbor_span time = {span.data + split + 1u, span.length - split - 1u};
        bool date_valid = false, time_valid = false;
        status = date_ordinal_checked(date, &ordinal, &date_valid);
        if (status.native != 0) return status;
        g10_decimal day_seconds = {ordinal, 0, false};
        status = decimal_multiply_u64(&day_seconds, UINT64_C(86400), &day_seconds);
        if (status.native != 0) return status;
        g10_decimal clock = {0};
        status = time_decimal(time, &clock, &time_valid);
        if (status.native != 0) return status;
        if (!date_valid || !time_valid) return err_status(EIO);
        status = decimal_add_unsigned(&day_seconds, &clock, out);
        if (status.native != 0) return status;
        *valid = true;
        return ok_status();
    }
    if (state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_NUMBER ||
        state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_RANGE)
        return decimal_parse(span, out, valid);
    *valid = false;
    return ok_status();
}

static bool input_range_state(uint64_t state) {
    return state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_DATE ||
        state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_MONTH ||
        state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_WEEK ||
        state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TIME ||
        state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_DATETIME_LOCAL ||
        state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_NUMBER ||
        state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_RANGE;
}

static bool token_next(arbor_span span, uint64_t *cursor, arbor_span *token) {
    uint64_t index = *cursor;
    while (index < span.length && ascii_space(span.data[index])) ++index;
    if (index == span.length) { *cursor = index; return false; }
    const uint64_t begin = index;
    while (index < span.length && !ascii_space(span.data[index])) ++index;
    *cursor = index;
    *token = (arbor_span){span.data + begin, index - begin};
    return true;
}

static bool valid_url_reference(arbor_span span, bool absolute_required) {
    if (span.data == NULL || span.length == 0u) return false;
    bool colon = false;
    for (uint64_t index = 0u; index < span.length; ++index) {
        const uint8_t byte = span.data[index];
        if (byte <= UINT8_C(0x20) || byte == UINT8_C(0x7f)) return false;
        if (byte == (uint8_t)'%') {
            if (index + 2u >= span.length) return false;
            for (uint64_t part = 1u; part <= 2u; ++part) {
                const uint8_t hex = span.data[index + part];
                if (!((hex >= (uint8_t)'0' && hex <= (uint8_t)'9') ||
                      (ascii_lower(hex) >= (uint8_t)'a' && ascii_lower(hex) <= (uint8_t)'f')))
                    return false;
            }
            index += 2u;
        }
        if (byte == (uint8_t)':') colon = true;
    }
    if (!absolute_required) return true;
    if (!((span.data[0] >= (uint8_t)'A' && span.data[0] <= (uint8_t)'Z') ||
          (span.data[0] >= (uint8_t)'a' && span.data[0] <= (uint8_t)'z'))) return false;
    return colon;
}

static bool email_atom_char(uint8_t byte) {
    if (ascii_digit(byte) || (ascii_lower(byte) >= (uint8_t)'a' &&
        ascii_lower(byte) <= (uint8_t)'z')) return true;
    return strchr(".!#$%&'*+/=?^_`{|}~-", (int)byte) != NULL;
}

static bool valid_single_email(arbor_span span) {
    if (span.data == NULL || span.length == 0u || span.length > UINT64_C(254)) return false;
    uint64_t at = UINT64_MAX;
    for (uint64_t index = 0u; index < span.length; ++index) {
        if (span.data[index] == (uint8_t)'@') {
            if (at != UINT64_MAX) return false;
            at = index;
        }
    }
    if (at == UINT64_MAX || at == 0u || at > UINT64_C(64) || at + 1u == span.length ||
        span.data[0] == (uint8_t)'.' || span.data[at - 1u] == (uint8_t)'.') return false;
    for (uint64_t index = 0u; index < at; ++index) {
        if (!email_atom_char(span.data[index]) ||
            (span.data[index] == (uint8_t)'.' && index + 1u < at &&
             span.data[index + 1u] == (uint8_t)'.')) return false;
    }
    uint64_t label_begin = at + 1u;
    for (uint64_t index = label_begin; index <= span.length; ++index) {
        if (index != span.length && span.data[index] != (uint8_t)'.') continue;
        const uint64_t length = index - label_begin;
        if (length == 0u || length > UINT64_C(63) ||
            span.data[label_begin] == (uint8_t)'-' || span.data[index - 1u] == (uint8_t)'-')
            return false;
        for (uint64_t part = label_begin; part < index; ++part) {
            const uint8_t byte = ascii_lower(span.data[part]);
            if (!ascii_digit(byte) && !(byte >= (uint8_t)'a' && byte <= (uint8_t)'z') &&
                byte != (uint8_t)'-') return false;
        }
        label_begin = index + 1u;
    }
    return true;
}

static bool valid_email(arbor_span span, bool multiple) {
    if (span.data == NULL || span.length == 0u) return false;
    uint64_t begin = 0u;
    for (;;) {
        uint64_t end = begin;
        while (end < span.length && span.data[end] != (uint8_t)',') ++end;
        const arbor_span address = trim_ascii((arbor_span){span.data + begin, end - begin});
        if (!valid_single_email(address)) return false;
        if (end == span.length) return true;
        if (!multiple) return false;
        begin = end + 1u;
        if (begin == span.length) return false;
    }
}

static const g10_source_attr *source_attr_for(
    const g10_context *context, uint64_t owner, g10_attr_code code) {
    for (const g10_source_attr *entry = context->source_attrs; entry != NULL; entry = entry->next)
        if (entry->owner_source_offset == owner && entry->code == (uint16_t)code) return entry;
    return NULL;
}

static arbor_status emit(g10_context *context, uint16_t rule, const g10_node *node,
                         g10_attr_code code) {
    if (rule == 0u || rule > ARBOR_VIEW0_NATIVE_V1N2_G10_RULE_COUNT || node == NULL)
        return err_status(EINVAL);
    if (context->evaluation.diagnostic_count == ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS)
        return ok_status();
    const uint64_t slot = context->evaluation.diagnostic_count;
    uint64_t offset = node->source_offset;
    uint64_t length = node->source_length;
    uint16_t kind = (uint16_t)ARBOR_VIEW0_NATIVE_V1N2_ANCHOR_ELEMENT;
    if (code != G10_ATTR_NONE) {
        const g10_source_attr *attribute = source_attr_for(context, node->source_offset, code);
        if (attribute != NULL) {
            offset = attribute->source_offset;
            length = attribute->source_length;
            kind = (uint16_t)ARBOR_VIEW0_NATIVE_V1N2_ANCHOR_ATTRIBUTE_NAME;
        }
    }
    if (offset == ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE || offset > UINT32_MAX ||
        length == 0u || length > UINT32_MAX) return err_status(EIO);
    if (context->collect) {
        if (slot >= context->anchor_capacity) return err_status(ENOSPC);
        context->anchors[slot].shared = (arbor_view0_native_v1n2_anchor){
            .byte_offset = offset, .source_length = length, .subject_index = node->source_offset,
            .group_ordinal = UINT16_C(10), .rule_ordinal = rule, .kind = kind
        };
    }
    context->evaluation.diagnostic_count += 1u;
    context->evaluation.rule_violation_count[rule - 1u] += 1u;
    return ok_status();
}

static g10_node *find_first_id(const g10_context *context, arbor_span id) {
    if (id.data == NULL || id.length == 0u) return NULL;
    for (g10_node *node = context->nodes; node != NULL; node = node->next)
        if (node->values[G10_ATTR_ID].present && span_eq(node->values[G10_ATTR_ID].span, id))
            return node;
    return NULL;
}

static g10_node *nearest_ancestor(const g10_node *node, uint64_t element_id) {
    for (g10_node *parent = node == NULL ? NULL : node->parent;
         parent != NULL; parent = parent->parent)
        if (parent->element_id == element_id) return parent;
    return NULL;
}

static bool is_autofocus_scoping_root(const g10_node *node) {
    return node != NULL &&
        (node->element_id == ARBOR_VIEW0_NATIVE_ELEMENT_DIALOG ||
         node->values[G10_ATTR_POPOVER].present);
}

static g10_node *nearest_autofocus_scoping_root(g10_node *node) {
    g10_node *tree_root = NULL;
    for (g10_node *candidate = node; candidate != NULL;
         candidate = candidate->parent) {
        tree_root = candidate;
        if (is_autofocus_scoping_root(candidate)) return candidate;
    }
    return tree_root;
}

static bool descendant_of(const g10_node *node, const g10_node *ancestor) {
    for (const g10_node *parent = node == NULL ? NULL : node->parent;
         parent != NULL; parent = parent->parent)
        if (parent == ancestor) return true;
    return false;
}

static g10_node *first_direct_child(const g10_node *node, uint64_t element_id) {
    for (g10_node *child = node == NULL ? NULL : node->first_child;
         child != NULL; child = child->next_sibling)
        if (child->element_id == element_id) return child;
    return NULL;
}

static bool disabled_by_fieldset(const g10_node *node) {
    for (const g10_node *ancestor = node == NULL ? NULL : node->parent;
         ancestor != NULL; ancestor = ancestor->parent) {
        if (ancestor->element_id != ARBOR_VIEW0_NATIVE_ELEMENT_FIELDSET ||
            !ancestor->values[G10_ATTR_DISABLED].present) continue;
        const g10_node *legend = first_direct_child(
            ancestor, ARBOR_VIEW0_NATIVE_ELEMENT_LEGEND);
        if (legend == NULL || (node != legend && !descendant_of(node, legend))) return true;
    }
    return false;
}

static bool disabled_control(const g10_node *node) {
    return node != NULL && (node->values[G10_ATTR_DISABLED].present ||
        disabled_by_fieldset(node));
}

static bool datalist_descendant(const g10_node *node) {
    return nearest_ancestor(node, ARBOR_VIEW0_NATIVE_ELEMENT_DATALIST) != NULL;
}

static g10_node *form_owner(const g10_context *context, const g10_node *node) {
    if (node->values[G10_ATTR_FORM].present) {
        g10_node *target = find_first_id(context, node->values[G10_ATTR_FORM].span);
        return target != NULL && target->element_id == ARBOR_VIEW0_NATIVE_ELEMENT_FORM ? target : NULL;
    }
    return nearest_ancestor(node, ARBOR_VIEW0_NATIVE_ELEMENT_FORM);
}

static bool is_labelable(const g10_node *node) {
    if (node == NULL) return false;
    switch (node->element_id) {
        case ARBOR_VIEW0_NATIVE_ELEMENT_BUTTON:
        case ARBOR_VIEW0_NATIVE_ELEMENT_SELECT:
        case ARBOR_VIEW0_NATIVE_ELEMENT_TEXTAREA:
        case ARBOR_VIEW0_NATIVE_ELEMENT_OUTPUT:
        case ARBOR_VIEW0_NATIVE_ELEMENT_PROGRESS:
        case ARBOR_VIEW0_NATIVE_ELEMENT_METER: return true;
        case ARBOR_VIEW0_NATIVE_ELEMENT_INPUT:
            return node->input_state != ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_HIDDEN;
        default: return false;
    }
}

static g10_node *first_descendant_labelable(const g10_node *node) {
    for (g10_node *candidate = node->first_child; candidate != NULL; candidate = candidate->next_sibling) {
        if (is_labelable(candidate)) return candidate;
        g10_node *nested = first_descendant_labelable(candidate);
        if (nested != NULL) return nested;
    }
    return NULL;
}

static bool has_descendant_labelable_other_than(
    const g10_node *node, const g10_node *labeled_control) {
    for (g10_node *candidate = node->first_child; candidate != NULL;
         candidate = candidate->next_sibling) {
        if (is_labelable(candidate) && candidate != labeled_control) return true;
        if (has_descendant_labelable_other_than(candidate, labeled_control)) return true;
    }
    return false;
}

static bool is_form_control(uint64_t element_id) {
    return element_id == ARBOR_VIEW0_NATIVE_ELEMENT_INPUT ||
           element_id == ARBOR_VIEW0_NATIVE_ELEMENT_BUTTON ||
           element_id == ARBOR_VIEW0_NATIVE_ELEMENT_SELECT ||
           element_id == ARBOR_VIEW0_NATIVE_ELEMENT_TEXTAREA ||
           element_id == ARBOR_VIEW0_NATIVE_ELEMENT_OUTPUT ||
           element_id == ARBOR_VIEW0_NATIVE_ELEMENT_FIELDSET;
}

static bool is_submit_input(const g10_node *node) {
    return node->element_id == ARBOR_VIEW0_NATIVE_ELEMENT_INPUT &&
        (node->input_state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_SUBMIT ||
         node->input_state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_IMAGE);
}

static bool button_is_submit(const g10_node *node) {
    if (node == NULL || node->element_id != ARBOR_VIEW0_NATIVE_ELEMENT_BUTTON) return false;
    if (!node->values[G10_ATTR_TYPE].present || span_empty(node->values[G10_ATTR_TYPE].span))
        return true;
    if (span_eq_ci(node->values[G10_ATTR_TYPE].span, "reset") ||
        span_eq_ci(node->values[G10_ATTR_TYPE].span, "button")) return false;
    return true;
}

static bool readonly_applicable(uint64_t state) {
    return state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TEXT ||
        state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_SEARCH ||
        state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TEL ||
        state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_URL ||
        state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_EMAIL ||
        state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_PASSWORD ||
        state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_DATE ||
        state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_MONTH ||
        state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_WEEK ||
        state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TIME ||
        state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_DATETIME_LOCAL ||
        state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_NUMBER;
}

static bool barred_from_validation(const g10_node *node) {
    if (node == NULL || disabled_control(node) || datalist_descendant(node)) return true;
    if (node->element_id != ARBOR_VIEW0_NATIVE_ELEMENT_INPUT) return false;
    const uint64_t state = node->input_state;
    if (node->values[G10_ATTR_READONLY].present && readonly_applicable(state)) return true;
    return state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_HIDDEN ||
        state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_SUBMIT ||
        state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_IMAGE ||
        state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_RESET ||
        state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_BUTTON;
}

static bool has_submission_override(const g10_node *node) {
    return node->values[G10_ATTR_FORMACTION].present || node->values[G10_ATTR_FORMMETHOD].present ||
           node->values[G10_ATTR_FORMENCTYPE].present || node->values[G10_ATTR_FORMTARGET].present ||
           node->values[G10_ATTR_FORMNOVALIDATE].present;
}

static g10_attr_code first_submission_override(const g10_node *node) {
    const g10_attr_code codes[] = {G10_ATTR_FORMACTION,G10_ATTR_FORMMETHOD,G10_ATTR_FORMENCTYPE,
                                  G10_ATTR_FORMTARGET,G10_ATTR_FORMNOVALIDATE};
    for (size_t index = 0u; index < sizeof(codes)/sizeof(codes[0]); ++index)
        if (node->values[codes[index]].present) return codes[index];
    return G10_ATTR_NONE;
}

static bool same_radio_group(const g10_context *context, const g10_node *left,
                             const g10_node *right) {
    if (left->input_state != ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_RADIO ||
        right->input_state != ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_RADIO ||
        !left->values[G10_ATTR_NAME].present || !right->values[G10_ATTR_NAME].present ||
        span_empty(left->values[G10_ATTR_NAME].span) ||
        !span_eq(left->values[G10_ATTR_NAME].span, right->values[G10_ATTR_NAME].span)) return false;
    return form_owner(context, left) == form_owner(context, right);
}

static bool nearest_select_is(const g10_node *option, const g10_node *select) {
    return nearest_ancestor(option, ARBOR_VIEW0_NATIVE_ELEMENT_SELECT) == select;
}

static g10_node *first_owned_option(const g10_context *context, const g10_node *select) {
    for (g10_node *node = context->nodes; node != NULL; node = node->next)
        if (node->element_id == ARBOR_VIEW0_NATIVE_ELEMENT_OPTION &&
            nearest_select_is(node, select)) return node;
    return NULL;
}

static uint64_t select_display_size(const g10_node *select) {
    uint64_t parsed = 0u;
    if (select->values[G10_ATTR_SIZE].present &&
        parse_u64(select->values[G10_ATTR_SIZE].span, &parsed) && parsed != 0u)
        return parsed;
    return select->values[G10_ATTR_MULTIPLE].present ? UINT64_C(4) : UINT64_C(1);
}

static g10_node *placeholder_label_option(
    const g10_context *context, const g10_node *select) {
    if (!select->values[G10_ATTR_REQUIRED].present || select_display_size(select) != 1u)
        return NULL;
    g10_node *first = first_owned_option(context, select);
    if (first == NULL || first->parent != select ||
        !first->values[G10_ATTR_VALUE].present ||
        !span_empty(first->values[G10_ATTR_VALUE].span)) return NULL;
    return first;
}

typedef enum g10_autocomplete_group {
    G10_AUTOCOMPLETE_NONE = 0,
    G10_AUTOCOMPLETE_TEXT,
    G10_AUTOCOMPLETE_MULTILINE,
    G10_AUTOCOMPLETE_PASSWORD,
    G10_AUTOCOMPLETE_URL,
    G10_AUTOCOMPLETE_USERNAME,
    G10_AUTOCOMPLETE_TEL,
    G10_AUTOCOMPLETE_NUMERIC,
    G10_AUTOCOMPLETE_MONTH,
    G10_AUTOCOMPLETE_DATE
} g10_autocomplete_group;

static bool span_in_literals(arbor_span span, const char *const *literals, size_t count) {
    for (size_t index = 0u; index < count; ++index)
        if (span_eq_ci(span, literals[index])) return true;
    return false;
}

static g10_autocomplete_group autocomplete_group(arbor_span field) {
    static const char *const text_fields[] = {
        "name", "honorific-prefix", "given-name", "additional-name", "family-name",
        "honorific-suffix", "nickname", "organization-title", "organization",
        "address-line1", "address-line2", "address-line3", "address-level4",
        "address-level3", "address-level2", "address-level1", "country",
        "country-name", "postal-code", "cc-name", "cc-given-name",
        "cc-additional-name", "cc-family-name", "cc-number", "cc-csc", "cc-type",
        "transaction-currency", "language", "sex", "tel-country-code",
        "tel-national", "tel-area-code", "tel-local", "tel-local-prefix",
        "tel-local-suffix", "tel-extension"
    };
    static const char *const password_fields[] = {
        "new-password", "current-password", "one-time-code"
    };
    static const char *const url_fields[] = {"url", "photo", "impp"};
    static const char *const username_fields[] = {"username", "email"};
    static const char *const numeric_fields[] = {
        "cc-exp-month", "cc-exp-year", "transaction-amount",
        "bday-day", "bday-month", "bday-year"
    };
    if (span_in_literals(field, text_fields,
            sizeof(text_fields) / sizeof(text_fields[0]))) return G10_AUTOCOMPLETE_TEXT;
    if (span_eq_ci(field, "street-address")) return G10_AUTOCOMPLETE_MULTILINE;
    if (span_in_literals(field, password_fields,
            sizeof(password_fields) / sizeof(password_fields[0]))) return G10_AUTOCOMPLETE_PASSWORD;
    if (span_in_literals(field, url_fields,
            sizeof(url_fields) / sizeof(url_fields[0]))) return G10_AUTOCOMPLETE_URL;
    if (span_in_literals(field, username_fields,
            sizeof(username_fields) / sizeof(username_fields[0]))) return G10_AUTOCOMPLETE_USERNAME;
    if (span_eq_ci(field, "tel")) return G10_AUTOCOMPLETE_TEL;
    if (span_in_literals(field, numeric_fields,
            sizeof(numeric_fields) / sizeof(numeric_fields[0]))) return G10_AUTOCOMPLETE_NUMERIC;
    if (span_eq_ci(field, "cc-exp")) return G10_AUTOCOMPLETE_MONTH;
    if (span_eq_ci(field, "bday")) return G10_AUTOCOMPLETE_DATE;
    return G10_AUTOCOMPLETE_NONE;
}

static bool autocomplete_contact_field(arbor_span field) {
    static const char *const fields[] = {
        "tel", "tel-country-code", "tel-national", "tel-area-code", "tel-local",
        "tel-local-prefix", "tel-local-suffix", "tel-extension", "email", "impp"
    };
    return span_in_literals(field, fields, sizeof(fields) / sizeof(fields[0]));
}

static bool autocomplete_control_in_group(
    const g10_node *node, g10_autocomplete_group group) {
    if (node->element_id == ARBOR_VIEW0_NATIVE_ELEMENT_TEXTAREA ||
        node->element_id == ARBOR_VIEW0_NATIVE_ELEMENT_SELECT) return true;
    if (node->element_id != ARBOR_VIEW0_NATIVE_ELEMENT_INPUT) return false;
    const uint64_t state = node->input_state;
    if (state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_HIDDEN) return true;
    if (state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TEXT ||
        state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_SEARCH)
        return group != G10_AUTOCOMPLETE_MULTILINE;
    return (group == G10_AUTOCOMPLETE_PASSWORD &&
            state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_PASSWORD) ||
        (group == G10_AUTOCOMPLETE_URL &&
            state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_URL) ||
        (group == G10_AUTOCOMPLETE_USERNAME &&
            state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_EMAIL) ||
        (group == G10_AUTOCOMPLETE_TEL &&
            state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TEL) ||
        (group == G10_AUTOCOMPLETE_NUMERIC &&
            state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_NUMBER) ||
        (group == G10_AUTOCOMPLETE_MONTH &&
            state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_MONTH) ||
        (group == G10_AUTOCOMPLETE_DATE &&
            state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_DATE);
}

static bool valid_autocomplete(const g10_node *node) {
    arbor_span tokens[5] = {{0}};
    uint64_t cursor = 0u;
    size_t count = 0u;
    arbor_span token = {0};
    while (token_next(node->values[G10_ATTR_AUTOCOMPLETE].span, &cursor, &token)) {
        if (count == sizeof(tokens) / sizeof(tokens[0])) return false;
        tokens[count++] = token;
    }
    if (count == 0u) return false;
    const bool anchor_mantle = node->element_id == ARBOR_VIEW0_NATIVE_ELEMENT_INPUT &&
        node->input_state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_HIDDEN;
    if (count == 1u && (span_eq_ci(tokens[0], "on") || span_eq_ci(tokens[0], "off")))
        return !anchor_mantle;

    size_t index = 0u;
    if (span_starts_ci(tokens[index], "section-")) ++index;
    if (index < count &&
        (span_eq_ci(tokens[index], "shipping") || span_eq_ci(tokens[index], "billing")))
        ++index;
    bool contact_hint = false;
    if (index < count &&
        (span_eq_ci(tokens[index], "home") || span_eq_ci(tokens[index], "work") ||
         span_eq_ci(tokens[index], "mobile") || span_eq_ci(tokens[index], "fax") ||
         span_eq_ci(tokens[index], "pager"))) {
        contact_hint = true;
        ++index;
    }
    if (index >= count) return false;
    const arbor_span field = tokens[index++];
    const g10_autocomplete_group group = autocomplete_group(field);
    if (group == G10_AUTOCOMPLETE_NONE ||
        (contact_hint && !autocomplete_contact_field(field)) ||
        !autocomplete_control_in_group(node, group)) return false;
    if (index < count && span_eq_ci(tokens[index], "webauthn")) {
        if (node->element_id != ARBOR_VIEW0_NATIVE_ELEMENT_INPUT &&
            node->element_id != ARBOR_VIEW0_NATIVE_ELEMENT_TEXTAREA) return false;
        ++index;
    }
    return index == count;
}

static bool valid_method(arbor_span value) {
    return span_eq_ci(value, "get") || span_eq_ci(value, "post") ||
        span_eq_ci(value, "dialog");
}

static bool valid_enctype(arbor_span value) {
    return span_eq_ci(value, "application/x-www-form-urlencoded") ||
        span_eq_ci(value, "multipart/form-data") || span_eq_ci(value, "text/plain");
}

static bool valid_target(arbor_span value) {
    if (span_empty(value)) return false;
    if (value.data[0] != (uint8_t)'_') return true;
    return span_eq_ci(value, "_blank") || span_eq_ci(value, "_self") ||
        span_eq_ci(value, "_parent") || span_eq_ci(value, "_top") ||
        span_eq_ci(value, "_unfencedTop");
}

static bool name_is_isindex(arbor_span value) {
    static const uint8_t literal[] = "isindex";
    return value.length == sizeof(literal) - 1u && value.data != NULL &&
        memcmp(value.data, literal, sizeof(literal) - 1u) == 0;
}

static arbor_status evaluate_form(g10_context *context, g10_node *node) {
    arbor_status status = ok_status();
    if (node->values[G10_ATTR_ACCEPT_CHARSET].present &&
        !span_eq_ci(node->values[G10_ATTR_ACCEPT_CHARSET].span, "utf-8")) {
        status = emit(context, UINT16_C(1), node, G10_ATTR_ACCEPT_CHARSET);
        if (status.native != 0) return status;
    }
    if (node->values[G10_ATTR_ACTION].present &&
        !valid_url_reference(node->values[G10_ATTR_ACTION].span, false)) {
        status = emit(context, UINT16_C(1), node, G10_ATTR_ACTION);
        if (status.native != 0) return status;
    }
    if (node->values[G10_ATTR_NAME].present) {
        if (span_empty(node->values[G10_ATTR_NAME].span)) {
            status = emit(context, UINT16_C(1), node, G10_ATTR_NAME);
            if (status.native != 0) return status;
        } else {
            for (g10_node *candidate = context->nodes; candidate != node;
                 candidate = candidate->next) {
                if (candidate->element_id == ARBOR_VIEW0_NATIVE_ELEMENT_FORM &&
                    candidate->values[G10_ATTR_NAME].present &&
                    span_eq(candidate->values[G10_ATTR_NAME].span,
                        node->values[G10_ATTR_NAME].span)) {
                    status = emit(context, UINT16_C(1), node, G10_ATTR_NAME);
                    if (status.native != 0) return status;
                    break;
                }
            }
        }
    }
    if (node->values[G10_ATTR_METHOD].present &&
        !valid_method(node->values[G10_ATTR_METHOD].span)) {
        status = emit(context, UINT16_C(1), node, G10_ATTR_METHOD);
        if (status.native != 0) return status;
    }
    if (node->values[G10_ATTR_ENCTYPE].present &&
        !valid_enctype(node->values[G10_ATTR_ENCTYPE].span)) {
        status = emit(context, UINT16_C(1), node, G10_ATTR_ENCTYPE);
        if (status.native != 0) return status;
    }
    if (node->values[G10_ATTR_TARGET].present &&
        !valid_target(node->values[G10_ATTR_TARGET].span)) {
        status = emit(context, UINT16_C(1), node, G10_ATTR_TARGET);
        if (status.native != 0) return status;
    }
    return status;
}

static arbor_status evaluate_owner(g10_context *context, g10_node *node) {
    if (!node->values[G10_ATTR_FORM].present) return ok_status();
    g10_node *target = find_first_id(context, node->values[G10_ATTR_FORM].span);
    if (target == NULL || target->element_id != ARBOR_VIEW0_NATIVE_ELEMENT_FORM)
        return emit(context, UINT16_C(1), node, G10_ATTR_FORM);
    return ok_status();
}

static arbor_status evaluate_label(g10_context *context, g10_node *node) {
    context->evaluation.label_count += 1u;
    g10_node *labeled_control = NULL;
    if (node->values[G10_ATTR_FOR].present) {
        context->evaluation.idref_token_count += 1u;
        labeled_control = find_first_id(context, node->values[G10_ATTR_FOR].span);
        if (!is_labelable(labeled_control))
            return emit(context, UINT16_C(2), node, G10_ATTR_FOR);
    } else {
        labeled_control = first_descendant_labelable(node);
    }
    if (has_descendant_labelable_other_than(node, labeled_control))
        return emit(context, UINT16_C(2), node, G10_ATTR_NONE);
    return ok_status();
}

static bool text_like_state(uint64_t state);

static arbor_status evaluate_input_relations(g10_context *context, g10_node *node) {
    arbor_status status = ok_status();
    uint64_t minimum_length = 0u, maximum_length = 0u, size = 0u;
    const uint64_t state = node->input_state;
    if (text_like_state(state) && node->values[G10_ATTR_MINLENGTH].present &&
        node->values[G10_ATTR_MAXLENGTH].present &&
        parse_u64(node->values[G10_ATTR_MINLENGTH].span, &minimum_length) &&
        parse_u64(node->values[G10_ATTR_MAXLENGTH].span, &maximum_length) &&
        minimum_length > maximum_length) {
        status = emit(context, UINT16_C(3), node, G10_ATTR_MINLENGTH);
        if (status.native != 0) return status;
    }
    if (text_like_state(state) && node->values[G10_ATTR_SIZE].present &&
        parse_u64(node->values[G10_ATTR_SIZE].span, &size) && size == 0u) {
        status = emit(context, UINT16_C(3), node, G10_ATTR_SIZE);
        if (status.native != 0) return status;
    }
    long double step = 0.0L;
    if (input_range_state(state) && node->values[G10_ATTR_MIN].present &&
        node->values[G10_ATTR_MAX].present) {
        g10_decimal minimum = {0}, maximum = {0};
        bool minimum_valid = false, maximum_valid = false;
        status = domain_decimal(state, node->values[G10_ATTR_MIN].span,
                                &minimum, &minimum_valid);
        if (status.native != 0) return status;
        status = domain_decimal(state, node->values[G10_ATTR_MAX].span,
                                &maximum, &maximum_valid);
        if (status.native != 0) return status;
        if (minimum_valid && maximum_valid) {
            int comparison = 0;
            status = decimal_compare(&minimum, &maximum, &comparison);
            if (status.native != 0) return status;
            if (comparison > 0 &&
                state != ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TIME) {
                status = emit(context, UINT16_C(3), node, G10_ATTR_MIN);
                if (status.native != 0) return status;
            }
        }
    }
    if (input_range_state(state) && node->values[G10_ATTR_STEP].present &&
        !span_eq_ci(node->values[G10_ATTR_STEP].span, "any") &&
        parse_number(node->values[G10_ATTR_STEP].span, &step) && step <= 0.0L) {
        status = emit(context, UINT16_C(3), node, G10_ATTR_STEP);
        if (status.native != 0) return status;
    }
    const bool list_applicable = state != ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_HIDDEN &&
        state != ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_PASSWORD &&
        state != ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_CHECKBOX &&
        state != ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_RADIO &&
        state != ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_FILE &&
        state != ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_SUBMIT &&
        state != ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_IMAGE &&
        state != ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_RESET &&
        state != ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_BUTTON;
    if (list_applicable && node->values[G10_ATTR_LIST].present &&
        !span_empty(node->values[G10_ATTR_LIST].span)) {
        context->evaluation.idref_token_count += 1u;
        g10_node *target = find_first_id(context, node->values[G10_ATTR_LIST].span);
        if (target == NULL || target->element_id != ARBOR_VIEW0_NATIVE_ELEMENT_DATALIST) {
            status = emit(context, UINT16_C(3), node, G10_ATTR_LIST);
            if (status.native != 0) return status;
        }
    }
    if ((text_like_state(state) || state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_NUMBER) &&
        node->values[G10_ATTR_PLACEHOLDER].present &&
        span_has_cr_lf(node->values[G10_ATTR_PLACEHOLDER].span)) {
        status = emit(context, UINT16_C(3), node, G10_ATTR_PLACEHOLDER);
        if (status.native != 0) return status;
    }
    if (state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_FILE &&
        node->values[G10_ATTR_ACCEPT].present && !valid_accept(node->values[G10_ATTR_ACCEPT].span)) {
        status = emit(context, UINT16_C(3), node, G10_ATTR_ACCEPT);
        if (status.native != 0) return status;
    }
    if (state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_IMAGE &&
        (!node->values[G10_ATTR_SRC].present || span_empty(node->values[G10_ATTR_SRC].span))) {
        status = emit(context, UINT16_C(3), node, G10_ATTR_SRC);
        if (status.native != 0) return status;
    }
    if (state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_IMAGE &&
        node->values[G10_ATTR_SRC].present &&
        !span_empty(node->values[G10_ATTR_SRC].span) &&
        !valid_url_reference(node->values[G10_ATTR_SRC].span, false)) {
        status = emit(context, UINT16_C(3), node, G10_ATTR_SRC);
        if (status.native != 0) return status;
    }
    if (state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_IMAGE &&
        (!node->values[G10_ATTR_ALT].present || span_empty(node->values[G10_ATTR_ALT].span))) {
        status = emit(context, UINT16_C(3), node, G10_ATTR_ALT);
        if (status.native != 0) return status;
    }
    return status;
}

static arbor_status evaluate_button(g10_context *context, g10_node *node) {
    const bool submit = button_is_submit(node);
    if (has_submission_override(node) && !submit) {
        arbor_status status = emit(context, UINT16_C(4), node, first_submission_override(node));
        if (status.native != 0) return status;
    }
    if (node->values[G10_ATTR_COMMANDFOR].present &&
        (span_empty(node->values[G10_ATTR_COMMANDFOR].span) ||
         find_first_id(context, node->values[G10_ATTR_COMMANDFOR].span) == NULL)) {
        context->evaluation.idref_token_count += 1u;
        return emit(context, UINT16_C(4), node, G10_ATTR_COMMANDFOR);
    }
    return ok_status();
}

static arbor_status evaluate_select(g10_context *context, g10_node *select) {
    uint64_t option_count = 0u, selected_count = 0u;
    for (g10_node *node = context->nodes; node != NULL; node = node->next) {
        if (node->element_id != ARBOR_VIEW0_NATIVE_ELEMENT_OPTION ||
            !nearest_select_is(node, select)) continue;
        option_count += 1u;
        if (node->values[G10_ATTR_SELECTED].present) selected_count += 1u;
    }
    (void)option_count;
    if (!select->values[G10_ATTR_MULTIPLE].present && selected_count > 1u) {
        arbor_status status = emit(context, UINT16_C(5), select, G10_ATTR_MULTIPLE);
        if (status.native != 0) return status;
    }
    if (select->values[G10_ATTR_REQUIRED].present &&
        !select->values[G10_ATTR_MULTIPLE].present && select_display_size(select) == 1u &&
        placeholder_label_option(context, select) == NULL) {
        arbor_status status = emit(context, UINT16_C(5), select, G10_ATTR_REQUIRED);
        if (status.native != 0) return status;
    }
    return ok_status();
}

static arbor_status evaluate_optgroup(g10_context *context, g10_node *node) {
    if ((!node->values[G10_ATTR_LABEL].present || span_empty(node->values[G10_ATTR_LABEL].span)) &&
        first_direct_child(node, ARBOR_VIEW0_NATIVE_ELEMENT_LEGEND) == NULL)
        return emit(context, UINT16_C(5), node, G10_ATTR_LABEL);
    return ok_status();
}

static arbor_status evaluate_option(g10_context *context, g10_node *node) {
    if (node->values[G10_ATTR_LABEL].present && span_empty(node->values[G10_ATTR_LABEL].span))
        return emit(context, UINT16_C(5), node, G10_ATTR_LABEL);
    return ok_status();
}

static arbor_status evaluate_textarea(g10_context *context, g10_node *node) {
    uint64_t minimum_length = 0u, maximum_length = 0u;
    if (node->values[G10_ATTR_MINLENGTH].present && node->values[G10_ATTR_MAXLENGTH].present &&
        parse_u64(node->values[G10_ATTR_MINLENGTH].span, &minimum_length) &&
        parse_u64(node->values[G10_ATTR_MAXLENGTH].span, &maximum_length) &&
        minimum_length > maximum_length) {
        arbor_status status = emit(context, UINT16_C(6), node, G10_ATTR_MINLENGTH);
        if (status.native != 0) return status;
    }
    if (node->values[G10_ATTR_PLACEHOLDER].present &&
        span_has_cr_lf(node->values[G10_ATTR_PLACEHOLDER].span))
        return emit(context, UINT16_C(6), node, G10_ATTR_PLACEHOLDER);
    return ok_status();
}

static arbor_status evaluate_output(g10_context *context, g10_node *node) {
    if (!node->values[G10_ATTR_FOR].present) return ok_status();
    uint64_t cursor = 0u;
    arbor_span token = {0};
    while (token_next(node->values[G10_ATTR_FOR].span, &cursor, &token)) {
        context->evaluation.idref_token_count += 1u;
        if (find_first_id(context, token) == NULL) {
            arbor_status status = emit(context, UINT16_C(7), node, G10_ATTR_FOR);
            if (status.native != 0) return status;
        }
    }
    return ok_status();
}

static arbor_status evaluate_progress_meter(g10_context *context, g10_node *node) {
    if (node->element_id == ARBOR_VIEW0_NATIVE_ELEMENT_METER &&
        !node->values[G10_ATTR_VALUE].present)
        return emit(context, UINT16_C(8), node, G10_ATTR_VALUE);
    return ok_status();
}

static arbor_status evaluate_fieldset(g10_context *context, g10_node *node) {
    if (node->values[G10_ATTR_NAME].present &&
        (span_empty(node->values[G10_ATTR_NAME].span) ||
         name_is_isindex(node->values[G10_ATTR_NAME].span)))
        return emit(context, UINT16_C(9), node, G10_ATTR_NAME);
    return ok_status();
}

static arbor_status evaluate_selectedcontent(g10_context *context, g10_node *node) {
    for (const g10_selectedcontent_source *entry = context->selectedcontent_sources;
         entry != NULL; entry = entry->next)
        if (entry->source_offset == node->source_offset)
            return emit(context, UINT16_C(10), node, G10_ATTR_NONE);
    return ok_status();
}

static arbor_status evaluate_autofocus(g10_context *context, g10_node *node) {
    if (!node->values[G10_ATTR_AUTOFOCUS].present) return ok_status();
    g10_node *root = nearest_autofocus_scoping_root(node);
    if (root == NULL) return err_status(EIO);
    if (root->autofocus_seen) {
        arbor_status status = emit(context, UINT16_C(11), node, G10_ATTR_AUTOFOCUS);
        if (status.native != 0) return status;
    }
    root->autofocus_seen = true;
    return ok_status();
}

static arbor_status evaluate_common(g10_context *context, g10_node *node) {
    if (node->element_id != ARBOR_VIEW0_NATIVE_ELEMENT_FIELDSET &&
        node->values[G10_ATTR_NAME].present &&
        (span_empty(node->values[G10_ATTR_NAME].span) ||
         name_is_isindex(node->values[G10_ATTR_NAME].span))) {
        arbor_status status = emit(context, UINT16_C(11), node, G10_ATTR_NAME);
        if (status.native != 0) return status;
    }
    if (node->values[G10_ATTR_DIRNAME].present &&
        span_empty(node->values[G10_ATTR_DIRNAME].span)) {
        arbor_status status = emit(context, UINT16_C(11), node, G10_ATTR_DIRNAME);
        if (status.native != 0) return status;
    }
    if (node->values[G10_ATTR_AUTOCOMPLETE].present && !valid_autocomplete(node)) {
        arbor_status status = emit(context, UINT16_C(11), node, G10_ATTR_AUTOCOMPLETE);
        if (status.native != 0) return status;
    }
    const bool submit_capable = is_submit_input(node) || button_is_submit(node);
    if (submit_capable && has_submission_override(node) && form_owner(context, node) == NULL) {
        arbor_status status = emit(context, UINT16_C(11), node,
                                   first_submission_override(node));
        if (status.native != 0) return status;
    }
    return ok_status();
}

static bool text_like_state(uint64_t state) {
    const uint64_t mask = ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TEXT |
        ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_SEARCH |
        ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TEL |
        ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_URL |
        ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_EMAIL |
        ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_PASSWORD;
    return (state & mask) != 0u;
}

static arbor_status input_step_mismatch(const g10_node *node, bool *mismatch) {
    if (node == NULL || mismatch == NULL) return err_status(EINVAL);
    *mismatch = false;
    if (!input_range_state(node->input_state) ||
        !node->values[G10_ATTR_VALUE].present || span_empty(node->values[G10_ATTR_VALUE].span) ||
        (node->values[G10_ATTR_STEP].present &&
         span_eq_ci(node->values[G10_ATTR_STEP].span, "any"))) return ok_status();
    g10_decimal value = {0}, base = {0}, step = {0};
    bool value_valid = false, base_valid = false, step_valid = true;
    arbor_status status = domain_decimal(
        node->input_state, node->values[G10_ATTR_VALUE].span, &value, &value_valid);
    if (status.native != 0 || !value_valid) return status;
    if (node->values[G10_ATTR_STEP].present) {
        status = decimal_parse(node->values[G10_ATTR_STEP].span, &step, &step_valid);
        if (status.native != 0) return status;
        if (!step_valid || step.negative || step.coefficient == 0u) return ok_status();
    } else {
        step = (g10_decimal){
            node->input_state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TIME ||
            node->input_state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_DATETIME_LOCAL
                ? UINT64_C(60) : UINT64_C(1), 0, false};
    }
    if (node->input_state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_WEEK) {
        g10_decimal scaled = {0};
        status = decimal_multiply_u64(&step, UINT64_C(7), &scaled);
        if (status.native != 0) return status;
        step = scaled;
    }
    if (node->values[G10_ATTR_MIN].present) {
        status = domain_decimal(
            node->input_state, node->values[G10_ATTR_MIN].span, &base, &base_valid);
        if (status.native != 0) return status;
    }
    if (!base_valid) base = value;
    g10_decimal difference = {0};
    status = decimal_abs_difference(&value, &base, &difference);
    if (status.native != 0 || difference.coefficient == 0u) return status;
    return decimal_modulo_nonzero(&difference, &step, mismatch);
}

static arbor_status evaluate_constraint(g10_context *context, g10_node *node) {
    if (barred_from_validation(node)) return ok_status();
    if (node->element_id == ARBOR_VIEW0_NATIVE_ELEMENT_INPUT) {
        const uint64_t state = node->input_state;
        if (state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_RADIO) {
            bool required = node->values[G10_ATTR_REQUIRED].present;
            bool checked = node->values[G10_ATTR_CHECKED].present;
            for (g10_node *candidate = context->nodes; candidate != NULL;
                 candidate = candidate->next) {
                if (candidate != node && same_radio_group(context, node, candidate)) {
                    if (candidate->values[G10_ATTR_REQUIRED].present) required = true;
                    if (candidate->values[G10_ATTR_CHECKED].present) checked = true;
                }
            }
            if (required && !checked) {
                const g10_attr_code anchor = node->values[G10_ATTR_REQUIRED].present
                    ? G10_ATTR_REQUIRED : G10_ATTR_NAME;
                arbor_status status = emit(context, UINT16_C(12), node, anchor);
                if (status.native != 0) return status;
            }
        } else if (node->values[G10_ATTR_REQUIRED].present) {
            if ((state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_CHECKBOX &&
                 !node->values[G10_ATTR_CHECKED].present) ||
                (text_like_state(state) && (!node->values[G10_ATTR_VALUE].present ||
                 span_empty(node->values[G10_ATTR_VALUE].span)))) {
                arbor_status status = emit(context, UINT16_C(12), node, G10_ATTR_REQUIRED);
                if (status.native != 0) return status;
            }
        }
        if (node->values[G10_ATTR_VALUE].present && !span_empty(node->values[G10_ATTR_VALUE].span)) {
            if (state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_URL &&
                !valid_url_reference(node->values[G10_ATTR_VALUE].span, true)) {
                arbor_status status = emit(context, UINT16_C(12), node, G10_ATTR_VALUE);
                if (status.native != 0) return status;
            }
            if (state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_EMAIL &&
                !valid_email(node->values[G10_ATTR_VALUE].span,
                    node->values[G10_ATTR_MULTIPLE].present)) {
                arbor_status status = emit(context, UINT16_C(12), node, G10_ATTR_VALUE);
                if (status.native != 0) return status;
            }
            if (input_range_state(state)) {
                g10_decimal value = {0};
                bool value_valid = false;
                arbor_status status = domain_decimal(
                    state, node->values[G10_ATTR_VALUE].span, &value, &value_valid);
                if (status.native != 0) return status;
                bool reversed_range = false;
                g10_decimal reversed_minimum = {0}, reversed_maximum = {0};
                bool reversed_minimum_valid = false, reversed_maximum_valid = false;
                if (value_valid && state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TIME &&
                    node->values[G10_ATTR_MIN].present && node->values[G10_ATTR_MAX].present) {
                    status = domain_decimal(state, node->values[G10_ATTR_MIN].span,
                                            &reversed_minimum, &reversed_minimum_valid);
                    if (status.native != 0) return status;
                    status = domain_decimal(state, node->values[G10_ATTR_MAX].span,
                                            &reversed_maximum, &reversed_maximum_valid);
                    if (status.native != 0) return status;
                    if (reversed_minimum_valid && reversed_maximum_valid) {
                        int comparison = 0;
                        status = decimal_compare(
                            &reversed_minimum, &reversed_maximum, &comparison);
                        if (status.native != 0) return status;
                        reversed_range = comparison > 0;
                    }
                }
                if (reversed_range) {
                    int versus_minimum = 0, versus_maximum = 0;
                    status = decimal_compare(&value, &reversed_minimum, &versus_minimum);
                    if (status.native != 0) return status;
                    status = decimal_compare(&value, &reversed_maximum, &versus_maximum);
                    if (status.native != 0) return status;
                    if (versus_maximum > 0 && versus_minimum < 0) {
                        status = emit(context, UINT16_C(12), node, G10_ATTR_VALUE);
                        if (status.native != 0) return status;
                    }
                }
                if (!reversed_range && value_valid && node->values[G10_ATTR_MIN].present) {
                    g10_decimal minimum = {0};
                    bool minimum_valid = false;
                    status = domain_decimal(state, node->values[G10_ATTR_MIN].span,
                                            &minimum, &minimum_valid);
                    if (status.native != 0) return status;
                    if (minimum_valid) {
                        int comparison = 0;
                        status = decimal_compare(&value, &minimum, &comparison);
                        if (status.native != 0) return status;
                        if (comparison < 0) {
                            status = emit(context, UINT16_C(12), node, G10_ATTR_VALUE);
                            if (status.native != 0) return status;
                        }
                    }
                }
                if (!reversed_range && value_valid && node->values[G10_ATTR_MAX].present) {
                    g10_decimal maximum = {0};
                    bool maximum_valid = false;
                    status = domain_decimal(state, node->values[G10_ATTR_MAX].span,
                                            &maximum, &maximum_valid);
                    if (status.native != 0) return status;
                    if (maximum_valid) {
                        int comparison = 0;
                        status = decimal_compare(&value, &maximum, &comparison);
                        if (status.native != 0) return status;
                        if (comparison > 0) {
                            status = emit(context, UINT16_C(12), node, G10_ATTR_VALUE);
                            if (status.native != 0) return status;
                        }
                    }
                }
            }
            bool step_mismatch = false;
            arbor_status step_status = input_step_mismatch(node, &step_mismatch);
            if (step_status.native != 0) return step_status;
            if (step_mismatch) {
                arbor_status status = emit(context, UINT16_C(12), node, G10_ATTR_STEP);
                if (status.native != 0) return status;
            }
        }
    }
    if (node->element_id == ARBOR_VIEW0_NATIVE_ELEMENT_SELECT &&
        node->values[G10_ATTR_REQUIRED].present) {
        g10_node *placeholder = placeholder_label_option(context, node);
        uint64_t selected_count = 0u;
        g10_node *selected = NULL;
        for (g10_node *option = context->nodes; option != NULL; option = option->next) {
            if (option->element_id != ARBOR_VIEW0_NATIVE_ELEMENT_OPTION ||
                !nearest_select_is(option, node) ||
                !option->values[G10_ATTR_SELECTED].present) continue;
            selected_count += 1u;
            selected = option;
        }
        if (selected_count == 1u && selected == placeholder)
            return emit(context, UINT16_C(12), node, G10_ATTR_REQUIRED);
    }
    return ok_status();
}

static arbor_status evaluate_submission(g10_context *context, g10_node *node) {
    if (node->values[G10_ATTR_DIRNAME].present &&
        (!node->values[G10_ATTR_NAME].present || span_empty(node->values[G10_ATTR_NAME].span)))
        return emit(context, UINT16_C(13), node, G10_ATTR_DIRNAME);
    if (node->element_id == ARBOR_VIEW0_NATIVE_ELEMENT_INPUT &&
        node->input_state == ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_IMAGE &&
        (!node->values[G10_ATTR_NAME].present || span_empty(node->values[G10_ATTR_NAME].span)))
        return emit(context, UINT16_C(13), node, G10_ATTR_NAME);
    bool successful_name_required = false;
    if (form_owner(context, node) != NULL && !disabled_control(node)) {
        if (node->element_id == ARBOR_VIEW0_NATIVE_ELEMENT_INPUT) {
            successful_name_required = node->input_state != ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_SUBMIT &&
                node->input_state != ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_IMAGE &&
                node->input_state != ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_RESET &&
                node->input_state != ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_BUTTON;
        } else {
            successful_name_required = node->element_id == ARBOR_VIEW0_NATIVE_ELEMENT_SELECT ||
                node->element_id == ARBOR_VIEW0_NATIVE_ELEMENT_TEXTAREA;
        }
    }
    if (successful_name_required &&
        (!node->values[G10_ATTR_NAME].present || span_empty(node->values[G10_ATTR_NAME].span)))
        return emit(context, UINT16_C(13), node, G10_ATTR_NAME);
    const bool submit_capable = is_submit_input(node) || button_is_submit(node);
    if (submit_capable && node->values[G10_ATTR_FORMACTION].present &&
        !valid_url_reference(node->values[G10_ATTR_FORMACTION].span, false))
        return emit(context, UINT16_C(13), node, G10_ATTR_FORMACTION);
    if (submit_capable && node->values[G10_ATTR_FORMMETHOD].present &&
        !valid_method(node->values[G10_ATTR_FORMMETHOD].span))
        return emit(context, UINT16_C(13), node, G10_ATTR_FORMMETHOD);
    if (submit_capable && node->values[G10_ATTR_FORMENCTYPE].present &&
        !valid_enctype(node->values[G10_ATTR_FORMENCTYPE].span))
        return emit(context, UINT16_C(13), node, G10_ATTR_FORMENCTYPE);
    if (submit_capable && node->values[G10_ATTR_FORMTARGET].present &&
        !valid_target(node->values[G10_ATTR_FORMTARGET].span))
        return emit(context, UINT16_C(13), node, G10_ATTR_FORMTARGET);
    if (submit_capable && node->values[G10_ATTR_FORMENCTYPE].present &&
        !span_eq_ci(node->values[G10_ATTR_FORMENCTYPE].span,
            "application/x-www-form-urlencoded")) {
        bool post = false;
        if (node->values[G10_ATTR_FORMMETHOD].present)
            post = span_eq_ci(node->values[G10_ATTR_FORMMETHOD].span, "post");
        else {
            g10_node *owner = form_owner(context, node);
            post = owner != NULL && owner->values[G10_ATTR_METHOD].present &&
                span_eq_ci(owner->values[G10_ATTR_METHOD].span, "post");
        }
        if (!post) return emit(context, UINT16_C(13), node, G10_ATTR_FORMENCTYPE);
    }
    if (node->element_id == ARBOR_VIEW0_NATIVE_ELEMENT_FORM &&
        node->values[G10_ATTR_ENCTYPE].present &&
        (!node->values[G10_ATTR_METHOD].present ||
         !span_eq_ci(node->values[G10_ATTR_METHOD].span, "post")) &&
        !span_eq_ci(node->values[G10_ATTR_ENCTYPE].span, "application/x-www-form-urlencoded"))
        return emit(context, UINT16_C(13), node, G10_ATTR_ENCTYPE);
    return ok_status();
}

static arbor_status evaluate_all(g10_context *context) {
    for (g10_node *node = context->nodes; node != NULL; node = node->next) {
        arbor_status status = ok_status();
        if (node->element_id == ARBOR_VIEW0_NATIVE_ELEMENT_FORM) status = evaluate_form(context, node);
        if (status.native == 0 && is_form_control(node->element_id)) status = evaluate_owner(context, node);
        if (status.native == 0 && node->element_id == ARBOR_VIEW0_NATIVE_ELEMENT_LABEL)
            status = evaluate_label(context, node);
        if (status.native == 0 && node->element_id == ARBOR_VIEW0_NATIVE_ELEMENT_INPUT)
            status = evaluate_input_relations(context, node);
        if (status.native == 0 && node->element_id == ARBOR_VIEW0_NATIVE_ELEMENT_BUTTON)
            status = evaluate_button(context, node);
        if (status.native == 0 && node->element_id == ARBOR_VIEW0_NATIVE_ELEMENT_SELECT)
            status = evaluate_select(context, node);
        if (status.native == 0 && node->element_id == ARBOR_VIEW0_NATIVE_ELEMENT_OPTGROUP)
            status = evaluate_optgroup(context, node);
        if (status.native == 0 && node->element_id == ARBOR_VIEW0_NATIVE_ELEMENT_OPTION)
            status = evaluate_option(context, node);
        if (status.native == 0 && node->element_id == ARBOR_VIEW0_NATIVE_ELEMENT_TEXTAREA)
            status = evaluate_textarea(context, node);
        if (status.native == 0 && node->element_id == ARBOR_VIEW0_NATIVE_ELEMENT_OUTPUT)
            status = evaluate_output(context, node);
        if (status.native == 0 && (node->element_id == ARBOR_VIEW0_NATIVE_ELEMENT_PROGRESS ||
                                  node->element_id == ARBOR_VIEW0_NATIVE_ELEMENT_METER))
            status = evaluate_progress_meter(context, node);
        if (status.native == 0 && node->element_id == ARBOR_VIEW0_NATIVE_ELEMENT_FIELDSET)
            status = evaluate_fieldset(context, node);
        if (status.native == 0 && node->element_id == ARBOR_VIEW0_NATIVE_ELEMENT_SELECTEDCONTENT)
            status = evaluate_selectedcontent(context, node);
        if (status.native == 0 && node->element_id != ARBOR_VIEW0_NATIVE_ELEMENT_NONE)
            status = evaluate_autofocus(context, node);
        if (status.native == 0 && is_form_control(node->element_id))
            status = evaluate_common(context, node);
        if (status.native == 0 && (node->element_id == ARBOR_VIEW0_NATIVE_ELEMENT_INPUT ||
                                  node->element_id == ARBOR_VIEW0_NATIVE_ELEMENT_SELECT))
            status = evaluate_constraint(context, node);
        if (status.native == 0 && (node->element_id == ARBOR_VIEW0_NATIVE_ELEMENT_FORM ||
                                  is_form_control(node->element_id)))
            status = evaluate_submission(context, node);
        if (status.native != 0) return status;
    }
    for (g10_node *left = context->nodes; left != NULL; left = left->next) {
        if (left->element_id != ARBOR_VIEW0_NATIVE_ELEMENT_INPUT ||
            left->input_state != ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_RADIO ||
            !left->values[G10_ATTR_CHECKED].present) continue;
        for (g10_node *right = left->next; right != NULL; right = right->next)
            if (right->element_id == ARBOR_VIEW0_NATIVE_ELEMENT_INPUT &&
                right->values[G10_ATTR_CHECKED].present &&
                same_radio_group(context, left, right)) {
                arbor_status status = emit(context, UINT16_C(3), right, G10_ATTR_CHECKED);
                if (status.native != 0) return status;
            }
    }
    return ok_status();
}

static arbor_status traversal_enter(void *opaque,
                                    const arbor_view0_native_element_observation *observation) {
    if (opaque == NULL || observation == NULL) return err_status(EINVAL);
    g10_context *context = opaque;
    if (observation->depth != context->frame_count ||
        context->frame_count >= ARBOR_VIEW0_NATIVE_V1N2_G10_MAX_DEPTH) return err_status(ENOSPC);
    g10_node *node = support_calloc(context, sizeof(*node));
    if (node == NULL) return err_status(ENOMEM);
    node->element_id = observation->standard_element_id;
    node->source_offset = observation->source_offset;
    node->source_length = observation->source_length;
    node->depth = observation->depth;
    if (context->frame_count != 0u) {
        node->parent = context->frames[context->frame_count - 1u].node;
        if (node->parent->last_child == NULL) node->parent->first_child = node;
        else node->parent->last_child->next_sibling = node;
        node->parent->last_child = node;
    }
    if (context->nodes_tail == NULL) context->nodes = node;
    else context->nodes_tail->next = node;
    context->nodes_tail = node;
    context->frames[context->frame_count++] = (g10_frame){node, observation->source_offset,
                                                          observation->depth};
    if (node->element_id == ARBOR_VIEW0_NATIVE_ELEMENT_FORM) context->evaluation.form_count += 1u;
    if (node->element_id == ARBOR_VIEW0_NATIVE_ELEMENT_INPUT) context->evaluation.input_count += 1u;
    if (node->element_id == ARBOR_VIEW0_NATIVE_ELEMENT_OPTION) context->evaluation.option_count += 1u;
    if (is_form_control(node->element_id)) context->evaluation.control_count += 1u;
    return ok_status();
}

static arbor_status element_begin(void *opaque,
                                  const arbor_view0_native_element_observation *observation) {
    if (opaque == NULL || observation == NULL) return err_status(EINVAL);
    g10_context *context = opaque;
    if (context->frame_count == 0u) return err_status(EIO);
    context->current = context->frames[context->frame_count - 1u].node;
    if (context->current->source_offset != observation->source_offset) return err_status(EIO);
    return ok_status();
}

static arbor_status attribute(void *opaque,
                              const arbor_view0_native_attribute_observation *observation) {
    if (opaque == NULL || observation == NULL) return err_status(EINVAL);
    g10_context *context = opaque;
    if (context->current == NULL) return err_status(EIO);
    const g10_attr_code code = attr_code(observation->local_name);
    if (code != G10_ATTR_NONE && !context->current->values[code].present)
        context->current->values[code] = (g10_value){observation->value, true};
    return ok_status();
}

static arbor_status element_complete(void *opaque,
                                     const arbor_view0_native_element_observation *observation) {
    if (opaque == NULL || observation == NULL) return err_status(EINVAL);
    g10_context *context = opaque;
    if (context->current == NULL || context->current->source_offset != observation->source_offset)
        return err_status(EIO);
    if (context->current->element_id == ARBOR_VIEW0_NATIVE_ELEMENT_INPUT) {
        arbor_span type = context->current->values[G10_ATTR_TYPE].present
            ? context->current->values[G10_ATTR_TYPE].span : (arbor_span){0};
        context->current->input_state = arbor_view0_native_g05_c0_input_state_from_type(type);
    }
    return ok_status();
}

static arbor_status source_attribute(void *opaque,
    const arbor_view0_native_source_attribute_observation *observation) {
    if (opaque == NULL || observation == NULL) return err_status(EINVAL);
    g10_context *context = opaque;
    const g10_attr_code code = attr_code(observation->local_name);
    if (code == G10_ATTR_NONE) return ok_status();
    if (observation->owner_source_offset == ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE ||
        observation->source_offset == ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE ||
        observation->source_offset > UINT32_MAX || observation->source_length > UINT32_MAX)
        return err_status(EIO);
    if (source_attr_for(context, observation->owner_source_offset, code) != NULL) return ok_status();
    g10_source_attr *entry = support_calloc(context, sizeof(*entry));
    if (entry == NULL) return err_status(ENOMEM);
    entry->owner_source_offset = observation->owner_source_offset;
    entry->source_offset = (uint32_t)observation->source_offset;
    entry->source_length = (uint32_t)observation->source_length;
    entry->code = (uint16_t)code;
    if (context->source_attrs_tail == NULL) context->source_attrs = entry;
    else context->source_attrs_tail->next = entry;
    context->source_attrs_tail = entry;
    return ok_status();
}

static arbor_status record_selectedcontent_source(
    g10_context *context, uint64_t source_offset) {
    if (context == NULL || source_offset == ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE)
        return err_status(EIO);
    for (const g10_selectedcontent_source *entry = context->selectedcontent_sources;
         entry != NULL; entry = entry->next)
        if (entry->source_offset == source_offset) return ok_status();
    g10_selectedcontent_source *entry = support_calloc(context, sizeof(*entry));
    if (entry == NULL) return err_status(ENOMEM);
    entry->source_offset = source_offset;
    if (context->selectedcontent_sources_tail == NULL)
        context->selectedcontent_sources = entry;
    else
        context->selectedcontent_sources_tail->next = entry;
    context->selectedcontent_sources_tail = entry;
    return ok_status();
}

static arbor_status source_repair(void *opaque,
    const arbor_view0_native_source_repair_context *observation) {
    if (opaque == NULL || observation == NULL) return err_status(EINVAL);
    if (observation->initial_current_standard_element_id !=
        ARBOR_VIEW0_NATIVE_ELEMENT_SELECTEDCONTENT) return ok_status();
    return record_selectedcontent_source(
        opaque, observation->initial_current_source_offset);
}

static arbor_status source_text(void *opaque,
    const arbor_view0_native_source_text_observation *observation) {
    if (opaque == NULL || observation == NULL) return err_status(EINVAL);
    if (observation->initial_current_standard_element_id !=
        ARBOR_VIEW0_NATIVE_ELEMENT_SELECTEDCONTENT) return ok_status();
    if (observation->text.length != 0u && observation->text.data == NULL)
        return err_status(EIO);
    for (uint64_t index = 0u; index < observation->text.length; ++index)
        if (!ascii_space(observation->text.data[index]))
            return record_selectedcontent_source(
                opaque, observation->initial_current_source_offset);
    return ok_status();
}

static arbor_status traversal_leave(void *opaque,
                                    const arbor_view0_native_element_observation *observation) {
    if (opaque == NULL || observation == NULL) return err_status(EINVAL);
    g10_context *context = opaque;
    if (context->frame_count == 0u) return err_status(EIO);
    const g10_frame *frame = context->frames + context->frame_count - 1u;
    if (frame->source_offset != observation->source_offset || frame->depth != observation->depth)
        return err_status(EIO);
    if (observation->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_HTML &&
        observation->depth == 0u) {
        arbor_status status = evaluate_all(context);
        if (status.native != 0) return status;
    }
    context->frame_count -= 1u;
    context->current = NULL;
    return ok_status();
}

static arbor_status evaluate(arbor_span input,
    arbor_view0_native_v1n2_g10_anchor *anchors, uint64_t anchor_capacity,
    bool collect, arbor_view0_native_v1n2_g10_evaluation *evaluation_out) {
    if (evaluation_out == NULL || (collect && anchor_capacity != 0u && anchors == NULL))
        return err_status(EINVAL);
    lexbor_mraw_t *arena = lexbor_mraw_create();
    if (arena == NULL) return err_status(ENOMEM);
    if (lexbor_mraw_init(arena, 4096u) != LXB_STATUS_OK) {
        (void)lexbor_mraw_destroy(arena, true);
        return err_status(ENOMEM);
    }
    g10_context context = {.arena = arena, .anchors = anchors,
        .anchor_capacity = anchor_capacity, .collect = collect};
    context.evaluation.deferred_external_semantics_count = UINT64_C(6);
    const arbor_view0_native_semantic_observer observer = {
        .context = &context, .element_begin = element_begin, .attribute = attribute,
        .element_complete = element_complete, .traversal_enter = traversal_enter,
        .traversal_leave = traversal_leave, .source_repair = source_repair,
        .source_attribute = source_attribute, .source_text = source_text
    };
    arbor_view0_native_parse_counts parse_counts = {0};
    arbor_view0_native_document_facts facts = {0};
    arbor_view0_native_observation_counts counts = {0};
    arbor_status status = arbor_view0_native_lexbor_observe(
        input, &observer, &parse_counts, &facts, &counts);
    if (status.native == 0 && context.frame_count != 0u) status = err_status(EIO);
    if (status.native == 0 && collect && context.evaluation.diagnostic_count != anchor_capacity)
        status = err_status(EIO);
    if (status.native == 0) *evaluation_out = context.evaluation;
    (void)lexbor_mraw_destroy(arena, true);
    return status;
}

arbor_status arbor_view0_native_v1n2_g10_measure(
    arbor_span input, arbor_view0_native_v1n2_g10_evaluation *evaluation_out) {
    return evaluate(input, NULL, 0u, false, evaluation_out);
}

arbor_status arbor_view0_native_v1n2_g10_collect_anchors(
    arbor_span input, arbor_view0_native_v1n2_g10_anchor *anchors,
    uint64_t anchor_capacity, arbor_view0_native_v1n2_g10_evaluation *evaluation_out) {
    return evaluate(input, anchors, anchor_capacity, true, evaluation_out);
}

void arbor_view0_native_v1n2_g10_materialize_anchor(
    const arbor_view0_native_v1n2_g10_anchor *anchor, uint64_t discovery_sequence,
    arbor_view0_native_diagnostic *diagnostic) {
    static const char *const messages[] = {
        "Form owner or form declaration violates the frozen static boundary",
        "Label association violates the frozen static boundary",
        "Input state relationship violates the frozen static boundary",
        "Button command or submission declaration violates the frozen static boundary",
        "Select or option relationship violates the frozen static boundary",
        "Textarea relationship violates the frozen static boundary",
        "Output ID-reference association violates the frozen static boundary",
        "Progress or meter numeric relationship violates the frozen static boundary",
        "Fieldset or legend relationship violates the frozen static boundary",
        "Selectedcontent relationship violates the frozen static boundary",
        "Common form-control relationship violates the frozen static boundary",
        "Deterministic constraint-validation predicate is false",
        "Form-submission authoring declaration violates the frozen static boundary"
    };
    if (anchor == NULL || diagnostic == NULL || anchor->shared.rule_ordinal == 0u ||
        anchor->shared.rule_ordinal > ARBOR_VIEW0_NATIVE_V1N2_G10_RULE_COUNT) return;
    const arbor_view0_native_v1n2_rule_meta *meta = arbor_view0_native_v1n2_c0_rule_at(
        UINT64_C(23) + (uint64_t)anchor->shared.rule_ordinal - 1u);
    if (meta == NULL || meta->group != ARBOR_VIEW0_NATIVE_V1N2_GROUP_G10) return;
    (void)memset(diagnostic, 0, sizeof(*diagnostic));
    diagnostic->rule_id = meta->rule_id;
    diagnostic->byte_offset = anchor->shared.byte_offset;
    diagnostic->source_length = anchor->shared.source_length;
    diagnostic->discovery_sequence = discovery_sequence;
    diagnostic->severity = (uint32_t)ARBOR_VIEW0_NATIVE_SEVERITY_ERROR;
    diagnostic->origin = (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_AUTHORING;
    (void)memcpy(diagnostic->symbolic_name, meta->rule_symbol, strlen(meta->rule_symbol) + 1u);
    const char *message = messages[anchor->shared.rule_ordinal - 1u];
    (void)memcpy(diagnostic->message, message, strlen(message) + 1u);
}
