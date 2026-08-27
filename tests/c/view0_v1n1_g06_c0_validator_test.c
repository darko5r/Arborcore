#include "g06_c0.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void need(bool ok, const char *message) {
    if (!ok) {
        (void)fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

static arbor_span sp(const char *value) {
    arbor_span result = {(const uint8_t *)value, (uint64_t)strlen(value)};
    return result;
}

int main(void) {
    static const char *const directions[] = {"ltr", "rtl", "auto"};
    static char capacity_input[4097u * 6u];
    static const char hex[] = "0123456789abcdef";
    int64_t signed_value = 0;
    uint64_t unsigned_value = 0u;
    uint64_t tokens = UINT64_MAX;
    uint64_t i;
    size_t capacity_length = 0u;

    need(arbor_view0_native_g06_c0_rule_count() == 17u, "17 frozen rules");
    for (i = 0u; i < arbor_view0_native_g06_c0_rule_count(); ++i) {
        const arbor_view0_native_g06_c0_rule_meta *meta = arbor_view0_native_g06_c0_rule_at(i);
        need(meta != NULL, "rule metadata present");
        need(meta->rule_id == UINT64_C(0x0000000030060001) + i, "rule identity order");
        need(strlen(meta->rule_symbol) > 20u, "rule symbol present");
        need(strlen(meta->source_fingerprint_sha256) == 64u, "source fingerprint present");
    }
    need(arbor_view0_native_g06_c0_rule_at(17u) == NULL, "rule metadata bound");

    need(arbor_view0_native_g06_c0_boolean(sp("checked"), sp("")), "R1 empty boolean");
    need(arbor_view0_native_g06_c0_boolean(sp("checked"), sp("CHECKED")), "R1 canonical boolean");
    need(!arbor_view0_native_g06_c0_boolean(sp("checked"), sp("false")), "R1 false rejected");
    need(arbor_view0_native_g06_c0_enumerated(sp("RTL"), directions, 3u, false), "R2 keyword");
    need(!arbor_view0_native_g06_c0_enumerated(sp("sideways"), directions, 3u, false), "R2 invalid keyword");
    need(arbor_view0_native_g06_c0_enumerated(sp(""), directions, 3u, true), "R2 admitted empty default");

    need(arbor_view0_native_g06_c0_signed_integer(sp("-2"), &signed_value) == ARBOR_VIEW0_NATIVE_G06_C0_NUMBER_VALID && signed_value == -2, "R3 signed value");
    need(arbor_view0_native_g06_c0_signed_integer(sp("-9223372036854775808"), &signed_value) == ARBOR_VIEW0_NATIVE_G06_C0_NUMBER_VALID && signed_value == INT64_MIN, "R3 minimum");
    need(arbor_view0_native_g06_c0_signed_integer(sp("9223372036854775808"), &signed_value) == ARBOR_VIEW0_NATIVE_G06_C0_NUMBER_RANGE, "R3 overflow");
    need(arbor_view0_native_g06_c0_signed_integer(sp("92233720368547758080x"), &signed_value) == ARBOR_VIEW0_NATIVE_G06_C0_NUMBER_SYNTAX, "R3 syntax precedes overflow");
    need(arbor_view0_native_g06_c0_signed_integer(sp("+2"), &signed_value) == ARBOR_VIEW0_NATIVE_G06_C0_NUMBER_SYNTAX, "R3 author plus rejected");
    need(arbor_view0_native_g06_c0_nonnegative_integer(sp("300"), &unsigned_value) == ARBOR_VIEW0_NATIVE_G06_C0_NUMBER_VALID && unsigned_value == 300u, "R4 unsigned value");
    need(arbor_view0_native_g06_c0_nonnegative_integer(sp("18446744073709551616"), &unsigned_value) == ARBOR_VIEW0_NATIVE_G06_C0_NUMBER_RANGE, "R4 overflow");
    need(arbor_view0_native_g06_c0_nonnegative_integer(sp("184467440737095516160x"), &unsigned_value) == ARBOR_VIEW0_NATIVE_G06_C0_NUMBER_SYNTAX, "R4 syntax precedes overflow");
    need(arbor_view0_native_g06_c0_nonnegative_integer(sp("-1"), &unsigned_value) == ARBOR_VIEW0_NATIVE_G06_C0_NUMBER_SYNTAX, "R4 negative rejected");

    need(arbor_view0_native_g06_c0_floating_point(sp("1.5")), "R5 decimal");
    need(arbor_view0_native_g06_c0_floating_point(sp("-.5E+2")), "R5 exponent");
    need(!arbor_view0_native_g06_c0_floating_point(sp("NaN")), "R5 NaN rejected");
    need(!arbor_view0_native_g06_c0_floating_point(sp("1.")), "R5 empty fraction rejected");

    need(arbor_view0_native_g06_c0_month(sp("2026-08")), "R6 month");
    need(!arbor_view0_native_g06_c0_month(sp("2026-13")), "R6 month range");
    need(arbor_view0_native_g06_c0_month(sp("10000-01")), "R6 extended year");
    need(!arbor_view0_native_g06_c0_month(sp("0000-01")), "R6 zero year");
    need(arbor_view0_native_g06_c0_date(sp("2024-02-29")), "R7 leap date");
    need(!arbor_view0_native_g06_c0_date(sp("2026-02-30")), "R7 day range");
    need(arbor_view0_native_g06_c0_yearless_date(sp("08-19")), "R8 short yearless");
    need(arbor_view0_native_g06_c0_yearless_date(sp("--02-29")), "R8 optional prefix and leap day");
    need(!arbor_view0_native_g06_c0_yearless_date(sp("02-30")), "R8 day range");
    need(arbor_view0_native_g06_c0_time(sp("17:30")), "R9 minute time");
    need(arbor_view0_native_g06_c0_time(sp("23:59:59.999")), "R9 fractional second");
    need(!arbor_view0_native_g06_c0_time(sp("25:00")), "R9 hour range");
    need(!arbor_view0_native_g06_c0_time(sp("23:59:60")), "R9 leap second rejected");
    need(arbor_view0_native_g06_c0_local_datetime(sp("2026-08-19T17:30")), "R10 T local datetime");
    need(arbor_view0_native_g06_c0_local_datetime(sp("2026-08-19 17:30")), "R10 space local datetime");
    need(!arbor_view0_native_g06_c0_local_datetime(sp("2026-08-19T25:00")), "R10 bad time");
    need(arbor_view0_native_g06_c0_timezone(sp("Z")), "R11 Z timezone");
    need(arbor_view0_native_g06_c0_timezone(sp("+03:00")), "R11 colon timezone");
    need(arbor_view0_native_g06_c0_timezone(sp("-0800")), "R11 compact timezone");
    need(!arbor_view0_native_g06_c0_timezone(sp("+24:00")), "R11 timezone range");
    need(!arbor_view0_native_g06_c0_timezone(sp("-00:00")), "R11 negative zero rejected");
    need(arbor_view0_native_g06_c0_global_datetime(sp("2026-08-19T17:30:00Z")), "R12 global datetime");
    need(!arbor_view0_native_g06_c0_global_datetime(sp("2026-08-19T17:30")), "R12 timezone required");
    need(arbor_view0_native_g06_c0_week(sp("2026-W34")), "R13 week");
    need(arbor_view0_native_g06_c0_week(sp("2020-W53")), "R13 valid week 53");
    need(!arbor_view0_native_g06_c0_week(sp("2021-W53")), "R13 invalid week 53");
    need(!arbor_view0_native_g06_c0_week(sp("2026-W99")), "R13 week range");
    need(arbor_view0_native_g06_c0_duration(sp("PT1H30M")), "R14 ISO duration");
    need(arbor_view0_native_g06_c0_duration(sp("P2DT3H4M5.25S")), "R14 ISO compound duration");
    need(arbor_view0_native_g06_c0_duration(sp("1h 30m")), "R14 human duration");
    need(arbor_view0_native_g06_c0_duration(sp("1.25s 2H")), "R14 fractional seconds human duration");
    need(!arbor_view0_native_g06_c0_duration(sp("P")), "R14 empty ISO rejected");
    need(!arbor_view0_native_g06_c0_duration(sp("P4W")), "R14 ISO week rejected");
    need(!arbor_view0_native_g06_c0_duration(sp("1h 2H")), "R14 duplicate scale rejected");
    need(!arbor_view0_native_g06_c0_duration(sp("1.5h")), "R14 nonsecond fraction rejected");
    need(arbor_view0_native_g06_c0_date_optional_time(sp("2026-08-19")), "R15 date");
    need(arbor_view0_native_g06_c0_date_optional_time(sp("2026-08-19T17:30Z")), "R15 global datetime");
    need(!arbor_view0_native_g06_c0_date_optional_time(sp("not-a-date")), "R15 invalid");

    need(arbor_view0_native_g06_c0_space_tokens(sp(" a\tb c "), false, false, &tokens) == ARBOR_VIEW0_NATIVE_G06_C0_TOKEN_VALID && tokens == 3u, "R16 ASCII whitespace tokens");
    need(arbor_view0_native_g06_c0_space_tokens(sp(""), true, false, &tokens) == ARBOR_VIEW0_NATIVE_G06_C0_TOKEN_VALID && tokens == 0u, "R16 empty set");
    need(arbor_view0_native_g06_c0_space_tokens(sp("style STYLE"), true, true, &tokens) == ARBOR_VIEW0_NATIVE_G06_C0_TOKEN_DUPLICATE, "R16 case-insensitive duplicate");
    need(arbor_view0_native_g06_c0_space_tokens(sp("style STYLE"), true, false, &tokens) == ARBOR_VIEW0_NATIVE_G06_C0_TOKEN_VALID && tokens == 2u, "R16 case-sensitive distinct");
    for (i = 0u; i < 4097u; ++i) {
        capacity_input[capacity_length++] = 'x';
        capacity_input[capacity_length++] = hex[(i >> 12u) & 15u];
        capacity_input[capacity_length++] = hex[(i >> 8u) & 15u];
        capacity_input[capacity_length++] = hex[(i >> 4u) & 15u];
        capacity_input[capacity_length++] = hex[i & 15u];
        capacity_input[capacity_length++] = i == 4096u ? '\0' : ' ';
    }
    need(arbor_view0_native_g06_c0_space_tokens(sp(capacity_input), true, false, &tokens) == ARBOR_VIEW0_NATIVE_G06_C0_TOKEN_CAPACITY, "R16 bounded workspace capacity result");
    need(arbor_view0_native_g06_c0_comma_tokens(sp("one,two"), &tokens) && tokens == 2u, "R17 two tokens");
    need(arbor_view0_native_g06_c0_comma_tokens(sp(" a ,b,,d d "), &tokens) && tokens == 4u, "R17 empty token retained");
    need(arbor_view0_native_g06_c0_comma_tokens(sp(""), &tokens) && tokens == 0u, "R17 empty set");

    puts("VIEW0_V1N1_G06_C0_RULE_VALIDATORS=17_OF_17");
    puts("VIEW0_V1N1_G06_C0_R15_AUTHOR_FACING_CONSUMERS=ZERO");
    puts("PASS: VIEW0 V1N1 G06 C0 bounded common-microsyntax foundation");
    return 0;
}
