#include "g06_c0.h"

#include <limits.h>
#include <stddef.h>
#include <string.h>

static const arbor_view0_native_g06_c0_rule_meta g_rules[] = {
    {UINT64_C(0x0000000030060001), "ARBOR_VIEW_V1_G06_BOOLEAN", "8c097e05e21e9f5ec45dd6e981143d8ef79bd7d10b0a96167df2eb077ea43b3b"},
    {UINT64_C(0x0000000030060002), "ARBOR_VIEW_V1_G06_ENUMERATED", "c37f56777534026f5d2ca920c90df661297f604dc8fb915e061a5a10a65093a1"},
    {UINT64_C(0x0000000030060003), "ARBOR_VIEW_V1_G06_SIGNED_INTEGER", "b17dc0c1628baf6aa772836853488af73d24e20bc8d526cc561bb245bfb2a076"},
    {UINT64_C(0x0000000030060004), "ARBOR_VIEW_V1_G06_NONNEGATIVE_INTEGER", "b4ba8015769f2a0baba6a67a3d4c941912b5f4036ff56f3b7606b9b1efaf39ae"},
    {UINT64_C(0x0000000030060005), "ARBOR_VIEW_V1_G06_FLOATING_POINT", "55a99984a01d7f5a7f2a82e5765cd16c94fd17089f6613bf3cf7224accb3ada8"},
    {UINT64_C(0x0000000030060006), "ARBOR_VIEW_V1_G06_MONTH", "637e702c0cea4e36ebd837d5253b0d30d057d6d26304f259df3314e8f2f25771"},
    {UINT64_C(0x0000000030060007), "ARBOR_VIEW_V1_G06_DATE", "637e702c0cea4e36ebd837d5253b0d30d057d6d26304f259df3314e8f2f25771"},
    {UINT64_C(0x0000000030060008), "ARBOR_VIEW_V1_G06_YEARLESS_DATE", "637e702c0cea4e36ebd837d5253b0d30d057d6d26304f259df3314e8f2f25771"},
    {UINT64_C(0x0000000030060009), "ARBOR_VIEW_V1_G06_TIME", "637e702c0cea4e36ebd837d5253b0d30d057d6d26304f259df3314e8f2f25771"},
    {UINT64_C(0x000000003006000a), "ARBOR_VIEW_V1_G06_LOCAL_DATETIME", "637e702c0cea4e36ebd837d5253b0d30d057d6d26304f259df3314e8f2f25771"},
    {UINT64_C(0x000000003006000b), "ARBOR_VIEW_V1_G06_TIMEZONE", "637e702c0cea4e36ebd837d5253b0d30d057d6d26304f259df3314e8f2f25771"},
    {UINT64_C(0x000000003006000c), "ARBOR_VIEW_V1_G06_GLOBAL_DATETIME", "637e702c0cea4e36ebd837d5253b0d30d057d6d26304f259df3314e8f2f25771"},
    {UINT64_C(0x000000003006000d), "ARBOR_VIEW_V1_G06_WEEK", "637e702c0cea4e36ebd837d5253b0d30d057d6d26304f259df3314e8f2f25771"},
    {UINT64_C(0x000000003006000e), "ARBOR_VIEW_V1_G06_DURATION", "637e702c0cea4e36ebd837d5253b0d30d057d6d26304f259df3314e8f2f25771"},
    {UINT64_C(0x000000003006000f), "ARBOR_VIEW_V1_G06_DATE_OPTIONAL_TIME", "637e702c0cea4e36ebd837d5253b0d30d057d6d26304f259df3314e8f2f25771"},
    {UINT64_C(0x0000000030060010), "ARBOR_VIEW_V1_G06_SPACE_SEPARATED_TOKENS", "14ebf8c398a3205fd25e1e96ccfb1719a1dc0e5e5bc076120050260f7a972341"},
    {UINT64_C(0x0000000030060011), "ARBOR_VIEW_V1_G06_COMMA_SEPARATED_TOKENS", "646559dcc293899fe7fa6137196dc4d8bb906a6a5328e0e04d107bb0b61125bd"},
};

typedef struct g06_year {
    uint16_t mod400;
    bool nonzero;
} g06_year;

static bool span_ok(arbor_span value) {
    return value.data != NULL || value.length == 0u;
}

static bool ascii_digit(uint8_t c) { return c >= (uint8_t)'0' && c <= (uint8_t)'9'; }
static bool ascii_space(uint8_t c) {
    return c == UINT8_C(0x09) || c == UINT8_C(0x0a) || c == UINT8_C(0x0c) ||
           c == UINT8_C(0x0d) || c == UINT8_C(0x20);
}
static uint8_t ascii_lower(uint8_t c) {
    return c >= (uint8_t)'A' && c <= (uint8_t)'Z' ?
               (uint8_t)(c + ((uint8_t)'a' - (uint8_t)'A')) : c;
}
static bool span_equal_ascii_ci(arbor_span a, arbor_span b) {
    uint64_t i;
    if (!span_ok(a) || !span_ok(b) || a.length != b.length) return false;
    for (i = 0u; i < a.length; ++i) {
        if (ascii_lower(a.data[i]) != ascii_lower(b.data[i])) return false;
    }
    return true;
}
static bool span_equal_cstr_ci(arbor_span a, const char *b) {
    arbor_span bs;
    if (b == NULL) return false;
    bs.data = (const uint8_t *)b;
    bs.length = (uint64_t)strlen(b);
    return span_equal_ascii_ci(a, bs);
}
static bool two_digits(arbor_span value, uint64_t pos, uint8_t *out) {
    if (!span_ok(value) || pos > value.length || value.length - pos < 2u ||
        !ascii_digit(value.data[pos]) || !ascii_digit(value.data[pos + 1u])) return false;
    if (out != NULL) {
        *out = (uint8_t)((value.data[pos] - (uint8_t)'0') * UINT8_C(10) +
                         (value.data[pos + 1u] - (uint8_t)'0'));
    }
    return true;
}
static bool parse_year(arbor_span value, uint64_t *pos, g06_year *year) {
    uint64_t p;
    uint64_t digits = 0u;
    uint16_t mod400 = 0u;
    bool nonzero = false;
    if (pos == NULL || year == NULL || !span_ok(value)) return false;
    p = *pos;
    while (p < value.length && ascii_digit(value.data[p])) {
        uint8_t digit = (uint8_t)(value.data[p] - (uint8_t)'0');
        mod400 = (uint16_t)(((uint16_t)(mod400 * UINT16_C(10)) + (uint16_t)digit) % UINT16_C(400));
        nonzero = nonzero || digit != 0u;
        ++p;
        ++digits;
    }
    if (digits < 4u || !nonzero) return false;
    *pos = p;
    year->mod400 = mod400;
    year->nonzero = true;
    return true;
}
static bool leap_year(g06_year year) {
    return year.mod400 == 0u || (year.mod400 % UINT16_C(4) == 0u &&
                                 year.mod400 % UINT16_C(100) != 0u);
}
static uint8_t days_in_month(uint8_t month, bool leap) {
    static const uint8_t days[] = {31u, 28u, 31u, 30u, 31u, 30u,
                                    31u, 31u, 30u, 31u, 30u, 31u};
    if (month == 0u || month > 12u) return 0u;
    return month == 2u && leap ? 29u : days[month - 1u];
}
static bool parse_month_component(arbor_span value, uint64_t *pos,
                                  g06_year *year, uint8_t *month) {
    uint64_t p;
    uint8_t m;
    if (pos == NULL || year == NULL || month == NULL) return false;
    p = *pos;
    if (!parse_year(value, &p, year) || p >= value.length || value.data[p] != (uint8_t)'-') return false;
    ++p;
    if (!two_digits(value, p, &m) || m == 0u || m > 12u) return false;
    p += 2u;
    *pos = p;
    *month = m;
    return true;
}
static bool parse_date_component(arbor_span value, uint64_t *pos) {
    uint64_t p;
    g06_year year;
    uint8_t month;
    uint8_t day;
    if (pos == NULL) return false;
    p = *pos;
    if (!parse_month_component(value, &p, &year, &month) ||
        p >= value.length || value.data[p] != (uint8_t)'-') return false;
    ++p;
    if (!two_digits(value, p, &day) || day == 0u ||
        day > days_in_month(month, leap_year(year))) return false;
    p += 2u;
    *pos = p;
    return true;
}
static bool parse_time_component(arbor_span value, uint64_t *pos) {
    uint64_t p;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    if (pos == NULL || !span_ok(value)) return false;
    p = *pos;
    if (!two_digits(value, p, &hour) || hour > 23u) return false;
    p += 2u;
    if (p >= value.length || value.data[p] != (uint8_t)':') return false;
    ++p;
    if (!two_digits(value, p, &minute) || minute > 59u) return false;
    p += 2u;
    if (p < value.length && value.data[p] == (uint8_t)':') {
        uint64_t fraction = 0u;
        ++p;
        if (!two_digits(value, p, &second) || second > 59u) return false;
        p += 2u;
        if (p < value.length && value.data[p] == (uint8_t)'.') {
            ++p;
            while (p < value.length && ascii_digit(value.data[p]) && fraction < 3u) {
                ++p;
                ++fraction;
            }
            if (fraction == 0u || (p < value.length && ascii_digit(value.data[p]))) return false;
        }
    }
    *pos = p;
    return true;
}
static bool parse_timezone_component(arbor_span value, uint64_t *pos) {
    uint64_t p;
    uint8_t hour;
    uint8_t minute;
    bool negative;
    if (pos == NULL || !span_ok(value)) return false;
    p = *pos;
    if (p < value.length && value.data[p] == (uint8_t)'Z') {
        *pos = p + 1u;
        return true;
    }
    if (p >= value.length || (value.data[p] != (uint8_t)'+' && value.data[p] != (uint8_t)'-')) return false;
    negative = value.data[p] == (uint8_t)'-';
    ++p;
    if (!two_digits(value, p, &hour)) return false;
    p += 2u;
    if (p < value.length && value.data[p] == (uint8_t)':') ++p;
    if (!two_digits(value, p, &minute)) return false;
    p += 2u;
    if (hour > 23u || minute > 59u || (negative && hour == 0u && minute == 0u)) return false;
    *pos = p;
    return true;
}
static bool parse_ascii_digits(arbor_span value, uint64_t *pos) {
    uint64_t p;
    uint64_t start;
    if (pos == NULL || !span_ok(value)) return false;
    p = *pos;
    start = p;
    while (p < value.length && ascii_digit(value.data[p])) ++p;
    if (p == start) return false;
    *pos = p;
    return true;
}

uint64_t arbor_view0_native_g06_c0_rule_count(void) {
    return (uint64_t)(sizeof g_rules / sizeof g_rules[0]);
}

const arbor_view0_native_g06_c0_rule_meta *
arbor_view0_native_g06_c0_rule_at(uint64_t index) {
    return index < arbor_view0_native_g06_c0_rule_count() ? &g_rules[index] : NULL;
}

bool arbor_view0_native_g06_c0_boolean(arbor_span canonical_name,
                                        arbor_span raw_value) {
    return span_ok(canonical_name) && span_ok(raw_value) &&
           (raw_value.length == 0u || span_equal_ascii_ci(canonical_name, raw_value));
}

bool arbor_view0_native_g06_c0_enumerated(
    arbor_span raw_value, const char *const *keywords, uint64_t keyword_count,
    bool empty_value_allowed) {
    uint64_t i;
    if (!span_ok(raw_value) || (keyword_count != 0u && keywords == NULL)) return false;
    if (raw_value.length == 0u && empty_value_allowed) return true;
    for (i = 0u; i < keyword_count; ++i) {
        if (span_equal_cstr_ci(raw_value, keywords[i])) return true;
    }
    return false;
}

arbor_view0_native_g06_c0_number_result
arbor_view0_native_g06_c0_signed_integer(arbor_span value, int64_t *out) {
    uint64_t p = 0u;
    uint64_t digits_start;
    uint64_t magnitude = 0u;
    uint64_t limit;
    bool negative = false;
    if (!span_ok(value) || value.length == 0u) return ARBOR_VIEW0_NATIVE_G06_C0_NUMBER_SYNTAX;
    if (value.data[p] == (uint8_t)'-') {
        negative = true;
        ++p;
    }
    if (p == value.length) return ARBOR_VIEW0_NATIVE_G06_C0_NUMBER_SYNTAX;
    digits_start = p;
    for (; p < value.length; ++p) {
        if (!ascii_digit(value.data[p])) return ARBOR_VIEW0_NATIVE_G06_C0_NUMBER_SYNTAX;
    }
    limit = negative ? (uint64_t)INT64_MAX + UINT64_C(1) : (uint64_t)INT64_MAX;
    for (p = digits_start; p < value.length; ++p) {
        uint8_t digit;
        digit = (uint8_t)(value.data[p] - (uint8_t)'0');
        if (magnitude > (limit - (uint64_t)digit) / UINT64_C(10)) return ARBOR_VIEW0_NATIVE_G06_C0_NUMBER_RANGE;
        magnitude = magnitude * UINT64_C(10) + (uint64_t)digit;
    }
    if (out != NULL) {
        if (negative && magnitude == (uint64_t)INT64_MAX + UINT64_C(1)) *out = INT64_MIN;
        else if (negative) *out = -(int64_t)magnitude;
        else *out = (int64_t)magnitude;
    }
    return ARBOR_VIEW0_NATIVE_G06_C0_NUMBER_VALID;
}

arbor_view0_native_g06_c0_number_result
arbor_view0_native_g06_c0_nonnegative_integer(arbor_span value, uint64_t *out) {
    uint64_t p;
    uint64_t result = 0u;
    if (!span_ok(value) || value.length == 0u) return ARBOR_VIEW0_NATIVE_G06_C0_NUMBER_SYNTAX;
    for (p = 0u; p < value.length; ++p) {
        if (!ascii_digit(value.data[p])) return ARBOR_VIEW0_NATIVE_G06_C0_NUMBER_SYNTAX;
    }
    for (p = 0u; p < value.length; ++p) {
        uint8_t digit;
        digit = (uint8_t)(value.data[p] - (uint8_t)'0');
        if (result > (UINT64_MAX - (uint64_t)digit) / UINT64_C(10)) return ARBOR_VIEW0_NATIVE_G06_C0_NUMBER_RANGE;
        result = result * UINT64_C(10) + (uint64_t)digit;
    }
    if (out != NULL) *out = result;
    return ARBOR_VIEW0_NATIVE_G06_C0_NUMBER_VALID;
}

bool arbor_view0_native_g06_c0_floating_point(arbor_span value) {
    uint64_t p = 0u;
    bool before = false;
    bool after = false;
    if (!span_ok(value) || value.length == 0u) return false;
    if (value.data[p] == (uint8_t)'-') ++p;
    while (p < value.length && ascii_digit(value.data[p])) { before = true; ++p; }
    if (p < value.length && value.data[p] == (uint8_t)'.') {
        ++p;
        while (p < value.length && ascii_digit(value.data[p])) { after = true; ++p; }
        if (!after) return false;
    }
    if (!before && !after) return false;
    if (p < value.length && (value.data[p] == (uint8_t)'e' || value.data[p] == (uint8_t)'E')) {
        uint64_t exponent_start;
        ++p;
        if (p < value.length && (value.data[p] == (uint8_t)'+' || value.data[p] == (uint8_t)'-')) ++p;
        exponent_start = p;
        while (p < value.length && ascii_digit(value.data[p])) ++p;
        if (p == exponent_start) return false;
    }
    return p == value.length;
}

bool arbor_view0_native_g06_c0_month(arbor_span value) {
    uint64_t p = 0u;
    g06_year year;
    uint8_t month;
    return parse_month_component(value, &p, &year, &month) && p == value.length;
}

bool arbor_view0_native_g06_c0_date(arbor_span value) {
    uint64_t p = 0u;
    return parse_date_component(value, &p) && p == value.length;
}

bool arbor_view0_native_g06_c0_yearless_date(arbor_span value) {
    uint64_t p = 0u;
    uint8_t month;
    uint8_t day;
    if (!span_ok(value)) return false;
    if (value.length >= 2u && value.data[0] == (uint8_t)'-' && value.data[1] == (uint8_t)'-') p = 2u;
    if (!two_digits(value, p, &month) || month == 0u || month > 12u) return false;
    p += 2u;
    if (p >= value.length || value.data[p] != (uint8_t)'-') return false;
    ++p;
    if (!two_digits(value, p, &day) || day == 0u || day > days_in_month(month, true)) return false;
    return p + 2u == value.length;
}

bool arbor_view0_native_g06_c0_time(arbor_span value) {
    uint64_t p = 0u;
    return parse_time_component(value, &p) && p == value.length;
}

bool arbor_view0_native_g06_c0_local_datetime(arbor_span value) {
    uint64_t p = 0u;
    if (!parse_date_component(value, &p) || p >= value.length ||
        (value.data[p] != (uint8_t)'T' && value.data[p] != (uint8_t)' ')) return false;
    ++p;
    return parse_time_component(value, &p) && p == value.length;
}

bool arbor_view0_native_g06_c0_timezone(arbor_span value) {
    uint64_t p = 0u;
    return parse_timezone_component(value, &p) && p == value.length;
}

bool arbor_view0_native_g06_c0_global_datetime(arbor_span value) {
    uint64_t p = 0u;
    if (!parse_date_component(value, &p) || p >= value.length ||
        (value.data[p] != (uint8_t)'T' && value.data[p] != (uint8_t)' ')) return false;
    ++p;
    return parse_time_component(value, &p) && parse_timezone_component(value, &p) &&
           p == value.length;
}

bool arbor_view0_native_g06_c0_week(arbor_span value) {
    uint64_t p = 0u;
    g06_year year;
    uint8_t week;
    uint16_t previous;
    uint16_t days_before_mod7;
    uint8_t jan1;
    uint8_t maxweek;
    if (!parse_year(value, &p, &year) || p + 4u != value.length ||
        value.data[p] != (uint8_t)'-' || value.data[p + 1u] != (uint8_t)'W' ||
        !two_digits(value, p + 2u, &week)) return false;
    previous = (uint16_t)((year.mod400 + UINT16_C(399)) % UINT16_C(400));
    days_before_mod7 = (uint16_t)((previous + previous / UINT16_C(4) -
                                   previous / UINT16_C(100) + previous / UINT16_C(400)) % UINT16_C(7));
    jan1 = (uint8_t)(days_before_mod7 + UINT16_C(1));
    maxweek = (jan1 == 4u || (jan1 == 3u && leap_year(year))) ? 53u : 52u;
    return week != 0u && week <= maxweek;
}

static bool duration_iso(arbor_span value) {
    uint64_t p = 1u;
    bool any = false;
    bool time_any = false;
    if (value.length < 2u || value.data[0] != (uint8_t)'P') return false;
    {
        uint64_t save = p;
        if (parse_ascii_digits(value, &p) && p < value.length && value.data[p] == (uint8_t)'D') {
            ++p;
            any = true;
        } else p = save;
    }
    if (p < value.length && value.data[p] == (uint8_t)'T') {
        ++p;
        {
            uint64_t save = p;
            if (parse_ascii_digits(value, &p) && p < value.length && value.data[p] == (uint8_t)'H') {
                ++p; any = true; time_any = true;
            } else p = save;
        }
        {
            uint64_t save = p;
            if (parse_ascii_digits(value, &p) && p < value.length && value.data[p] == (uint8_t)'M') {
                ++p; any = true; time_any = true;
            } else p = save;
        }
        {
            uint64_t save = p;
            uint64_t fraction = 0u;
            if (parse_ascii_digits(value, &p)) {
                if (p < value.length && value.data[p] == (uint8_t)'.') {
                    ++p;
                    while (p < value.length && ascii_digit(value.data[p]) && fraction < 3u) { ++p; ++fraction; }
                    if (fraction == 0u || (p < value.length && ascii_digit(value.data[p]))) p = save;
                }
                if (p != save && p < value.length && value.data[p] == (uint8_t)'S') {
                    ++p; any = true; time_any = true;
                } else p = save;
            }
        }
        if (!time_any) return false;
    }
    return any && p == value.length;
}

static bool duration_human(arbor_span value) {
    uint64_t p = 0u;
    uint8_t seen = 0u;
    bool any = false;
    while (p < value.length && ascii_space(value.data[p])) ++p;
    while (p < value.length) {
        uint8_t unit;
        uint8_t bit;
        uint64_t fraction = 0u;
        if (!parse_ascii_digits(value, &p)) return false;
        if (p < value.length && value.data[p] == (uint8_t)'.') {
            ++p;
            while (p < value.length && ascii_digit(value.data[p]) && fraction < 3u) { ++p; ++fraction; }
            if (fraction == 0u || (p < value.length && ascii_digit(value.data[p]))) return false;
        }
        while (p < value.length && ascii_space(value.data[p])) ++p;
        if (p == value.length) return false;
        unit = ascii_lower(value.data[p]);
        ++p;
        if (unit == (uint8_t)'w') bit = UINT8_C(1);
        else if (unit == (uint8_t)'d') bit = UINT8_C(2);
        else if (unit == (uint8_t)'h') bit = UINT8_C(4);
        else if (unit == (uint8_t)'m') bit = UINT8_C(8);
        else if (unit == (uint8_t)'s') bit = UINT8_C(16);
        else return false;
        if (fraction != 0u && unit != (uint8_t)'s') return false;
        if ((seen & bit) != 0u) return false;
        seen = (uint8_t)(seen | bit);
        any = true;
        while (p < value.length && ascii_space(value.data[p])) ++p;
    }
    return any;
}

bool arbor_view0_native_g06_c0_duration(arbor_span value) {
    if (!span_ok(value) || value.length == 0u) return false;
    return value.data[0] == (uint8_t)'P' ? duration_iso(value) : duration_human(value);
}

bool arbor_view0_native_g06_c0_date_optional_time(arbor_span value) {
    return arbor_view0_native_g06_c0_date(value) || arbor_view0_native_g06_c0_global_datetime(value);
}

typedef struct g06_token_slot {
    uint64_t hash;
    uint64_t start;
    uint64_t length;
} g06_token_slot;

static uint64_t token_hash(arbor_span value, uint64_t start, uint64_t end,
                           bool ci) {
    uint64_t hash = UINT64_C(1469598103934665603);
    uint64_t i;
    for (i = start; i < end; ++i) {
        uint8_t byte = ci ? ascii_lower(value.data[i]) : value.data[i];
        hash ^= (uint64_t)byte;
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

static bool token_equal(arbor_span value, uint64_t a_start, uint64_t a_end,
                        uint64_t b_start, uint64_t b_length, bool ci) {
    uint64_t i;
    if (a_end - a_start != b_length) return false;
    for (i = 0u; i < a_end - a_start; ++i) {
        uint8_t a = value.data[a_start + i];
        uint8_t b = value.data[b_start + i];
        if (ci) { a = ascii_lower(a); b = ascii_lower(b); }
        if (a != b) return false;
    }
    return true;
}

arbor_view0_native_g06_c0_token_result
arbor_view0_native_g06_c0_space_tokens(arbor_span value, bool require_unique,
                                        bool ascii_case_insensitive,
                                        uint64_t *token_count_out) {
    g06_token_slot slots[ARBOR_VIEW0_NATIVE_G06_C0_TOKEN_WORKSPACE_CAP];
    uint64_t p = 0u;
    uint64_t count = 0u;
    uint64_t i;
    if (!span_ok(value)) return ARBOR_VIEW0_NATIVE_G06_C0_TOKEN_ARGUMENT;
    if (require_unique) {
        for (i = 0u; i < ARBOR_VIEW0_NATIVE_G06_C0_TOKEN_WORKSPACE_CAP; ++i) {
            slots[i].hash = 0u;
            slots[i].start = UINT64_MAX;
            slots[i].length = 0u;
        }
    }
    while (p < value.length) {
        uint64_t start;
        uint64_t end;
        while (p < value.length && ascii_space(value.data[p])) ++p;
        if (p == value.length) break;
        start = p;
        while (p < value.length && !ascii_space(value.data[p])) ++p;
        end = p;
        if (require_unique) {
            uint64_t hash;
            uint64_t slot;
            uint64_t probes;
            if (count == ARBOR_VIEW0_NATIVE_G06_C0_TOKEN_WORKSPACE_CAP)
                return ARBOR_VIEW0_NATIVE_G06_C0_TOKEN_CAPACITY;
            hash = token_hash(value, start, end, ascii_case_insensitive);
            slot = hash % ARBOR_VIEW0_NATIVE_G06_C0_TOKEN_WORKSPACE_CAP;
            for (probes = 0u; probes < ARBOR_VIEW0_NATIVE_G06_C0_TOKEN_WORKSPACE_CAP; ++probes) {
                if (slots[slot].start == UINT64_MAX) {
                    slots[slot].hash = hash;
                    slots[slot].start = start;
                    slots[slot].length = end - start;
                    break;
                }
                if (slots[slot].hash == hash &&
                    token_equal(value, start, end, slots[slot].start,
                                slots[slot].length, ascii_case_insensitive))
                    return ARBOR_VIEW0_NATIVE_G06_C0_TOKEN_DUPLICATE;
                slot = (slot + 1u) % ARBOR_VIEW0_NATIVE_G06_C0_TOKEN_WORKSPACE_CAP;
            }
            if (probes == ARBOR_VIEW0_NATIVE_G06_C0_TOKEN_WORKSPACE_CAP)
                return ARBOR_VIEW0_NATIVE_G06_C0_TOKEN_CAPACITY;
        }
        ++count;
    }
    if (token_count_out != NULL) *token_count_out = count;
    return ARBOR_VIEW0_NATIVE_G06_C0_TOKEN_VALID;
}

bool arbor_view0_native_g06_c0_comma_tokens(arbor_span value,
                                             uint64_t *token_count_out) {
    uint64_t i;
    uint64_t count;
    if (!span_ok(value)) return false;
    count = value.length == 0u ? 0u : 1u;
    for (i = 0u; i < value.length; ++i) if (value.data[i] == (uint8_t)',') ++count;
    if (token_count_out != NULL) *token_count_out = count;
    return true;
}
