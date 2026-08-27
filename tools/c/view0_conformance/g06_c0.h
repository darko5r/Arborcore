#ifndef ARBORCORE_VIEW0_CONFORMANCE_G06_C0_H
#define ARBORCORE_VIEW0_CONFORMANCE_G06_C0_H

#include <arborcore/view0_conformance/native.h>
#include <stdbool.h>
#include <stdint.h>

#define ARBOR_VIEW0_NATIVE_G06_C0_RULE_COUNT UINT64_C(17)
#define ARBOR_VIEW0_NATIVE_G06_C0_TOKEN_WORKSPACE_CAP UINT64_C(4096)

typedef enum arbor_view0_native_g06_c0_number_result {
    ARBOR_VIEW0_NATIVE_G06_C0_NUMBER_VALID = 0,
    ARBOR_VIEW0_NATIVE_G06_C0_NUMBER_SYNTAX = 1,
    ARBOR_VIEW0_NATIVE_G06_C0_NUMBER_RANGE = 2
} arbor_view0_native_g06_c0_number_result;

typedef enum arbor_view0_native_g06_c0_token_result {
    ARBOR_VIEW0_NATIVE_G06_C0_TOKEN_VALID = 0,
    ARBOR_VIEW0_NATIVE_G06_C0_TOKEN_DUPLICATE = 1,
    ARBOR_VIEW0_NATIVE_G06_C0_TOKEN_CAPACITY = 2,
    ARBOR_VIEW0_NATIVE_G06_C0_TOKEN_ARGUMENT = 3
} arbor_view0_native_g06_c0_token_result;

typedef struct arbor_view0_native_g06_c0_rule_meta {
    uint64_t rule_id;
    const char *rule_symbol;
    const char *source_fingerprint_sha256;
} arbor_view0_native_g06_c0_rule_meta;

uint64_t arbor_view0_native_g06_c0_rule_count(void);
const arbor_view0_native_g06_c0_rule_meta *
arbor_view0_native_g06_c0_rule_at(uint64_t index);

bool arbor_view0_native_g06_c0_boolean(arbor_span canonical_name,
                                        arbor_span raw_value);
bool arbor_view0_native_g06_c0_enumerated(
    arbor_span raw_value, const char *const *keywords, uint64_t keyword_count,
    bool empty_value_allowed);
arbor_view0_native_g06_c0_number_result
arbor_view0_native_g06_c0_signed_integer(arbor_span value, int64_t *out);
arbor_view0_native_g06_c0_number_result
arbor_view0_native_g06_c0_nonnegative_integer(arbor_span value, uint64_t *out);
bool arbor_view0_native_g06_c0_floating_point(arbor_span value);
bool arbor_view0_native_g06_c0_month(arbor_span value);
bool arbor_view0_native_g06_c0_date(arbor_span value);
bool arbor_view0_native_g06_c0_yearless_date(arbor_span value);
bool arbor_view0_native_g06_c0_time(arbor_span value);
bool arbor_view0_native_g06_c0_local_datetime(arbor_span value);
bool arbor_view0_native_g06_c0_timezone(arbor_span value);
bool arbor_view0_native_g06_c0_global_datetime(arbor_span value);
bool arbor_view0_native_g06_c0_week(arbor_span value);
bool arbor_view0_native_g06_c0_duration(arbor_span value);
bool arbor_view0_native_g06_c0_date_optional_time(arbor_span value);
arbor_view0_native_g06_c0_token_result
arbor_view0_native_g06_c0_space_tokens(arbor_span value, bool require_unique,
                                        bool ascii_case_insensitive,
                                        uint64_t *token_count_out);
bool arbor_view0_native_g06_c0_comma_tokens(arbor_span value,
                                             uint64_t *token_count_out);

#endif
