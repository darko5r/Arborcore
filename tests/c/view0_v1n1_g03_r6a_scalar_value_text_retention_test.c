#include <arborcore/view0_conformance/native.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define R6_RULE_ID UINT64_C(0x0000000030030006)

static arbor_span span_from_bytes(const uint8_t *data, size_t size)
{
    return (arbor_span){data, (uint64_t)size};
}

static arbor_span span_from_cstr(const char *text)
{
    return span_from_bytes((const uint8_t *)text, strlen(text));
}

static uint64_t count_rule(
    const arbor_view0_native_diagnostic *diagnostics,
    uint64_t count,
    uint64_t rule_id)
{
    uint64_t found = 0u;
    for (uint64_t i = 0u; i < count; ++i) {
        if (diagnostics[i].rule_id == rule_id) found += 1u;
    }
    return found;
}

static uint64_t count_symbol(
    const arbor_view0_native_diagnostic *diagnostics,
    uint64_t count,
    const char *symbol)
{
    uint64_t found = 0u;
    for (uint64_t i = 0u; i < count; ++i) {
        if (strcmp(diagnostics[i].symbolic_name, symbol) == 0) found += 1u;
    }
    return found;
}

static int expect_clean(const char *html)
{
    arbor_view0_native_diagnostic diagnostics[16] = {{0}};
    arbor_view0_native_result result = {0};
    const arbor_status status = arbor_view0_native_check(
        span_from_cstr(html), diagnostics, 16u, &result);
    if (status.native != 0 || result.diagnostic_count != 0u ||
        result.tokenizer_error_count != 0u || result.tree_error_count != 0u) {
        return 1;
    }
    if (count_rule(diagnostics, result.diagnostic_count, R6_RULE_ID) != 0u) return 2;
    return 0;
}

static int expect_single_owner(
    arbor_span input,
    uint32_t expected_origin,
    const char *expected_symbol)
{
    arbor_view0_native_diagnostic diagnostics[16] = {{0}};
    arbor_view0_native_result result = {0};
    const arbor_status status = arbor_view0_native_check(
        input, diagnostics, 16u, &result);
    if (status.native != 0 || result.diagnostic_count != 1u) return 1;
    if (diagnostics[0].severity != (uint32_t)ARBOR_VIEW0_NATIVE_SEVERITY_ERROR ||
        diagnostics[0].origin != expected_origin) return 2;
    if (count_symbol(diagnostics, result.diagnostic_count, expected_symbol) != 1u) return 3;
    if (count_rule(diagnostics, result.diagnostic_count, R6_RULE_ID) != 0u) return 4;
    if (expected_origin == (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_UTF8) {
        if (result.tokenizer_error_count != 0u || result.tree_error_count != 0u) return 5;
    } else {
        if (result.tokenizer_error_count != 1u || result.tree_error_count != 0u) return 6;
    }
    return 0;
}

int main(void)
{
    static const char valid_scalar[] =
        "<!doctype html><html><head><title>x</title></head><body>"
        "<p title=\"\xc3\xa9\">\xc3\xa9</p></body></html>";
    static const char allowed_ws[] =
        "<!doctype html><html><head><title>x</title></head><body>"
        "<p title=\"\t\n\f\r \" >x\t\n\f\r </p></body></html>";
    if (expect_clean(valid_scalar) != 0) return 1;
    if (expect_clean(allowed_ws) != 0) return 2;

    static const uint8_t malformed_utf8[] =
        "<!doctype html><html><head><title>x</title></head><body><p>"
        "\xf0\x80\x80\x80"
        "</p></body></html>";
    if (expect_single_owner(
            span_from_bytes(malformed_utf8, sizeof(malformed_utf8) - 1u),
            (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_UTF8,
            "html.utf8.invalid") != 0) return 3;

    static const char nonchar_literal_text[] =
        "<!doctype html><html><head><title>x</title></head><body><p>"
        "\xef\xb7\x90"
        "</p></body></html>";
    static const char nonchar_literal_attr[] =
        "<!doctype html><html><head><title>x</title></head><body><p title=\""
        "\xef\xb7\x90"
        "\">x</p></body></html>";
    if (expect_single_owner(
            span_from_cstr(nonchar_literal_text),
            (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_LEXBOR_TOKENIZER,
            "html.parse.tokenizer.noncharacter-in-input-stream") != 0) return 4;
    if (expect_single_owner(
            span_from_cstr(nonchar_literal_attr),
            (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_LEXBOR_TOKENIZER,
            "html.parse.tokenizer.noncharacter-in-input-stream") != 0) return 5;

    static const char nonchar_ref_text[] =
        "<!doctype html><html><head><title>x</title></head><body><p>&#xFDD0;</p></body></html>";
    static const char nonchar_ref_attr[] =
        "<!doctype html><html><head><title>x</title></head><body><p title=\"&#xFDD0;\">x</p></body></html>";
    if (expect_single_owner(
            span_from_cstr(nonchar_ref_text),
            (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_LEXBOR_TOKENIZER,
            "html.parse.tokenizer.noncharacter-character-reference") != 0) return 6;
    if (expect_single_owner(
            span_from_cstr(nonchar_ref_attr),
            (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_LEXBOR_TOKENIZER,
            "html.parse.tokenizer.noncharacter-character-reference") != 0) return 7;

    static const char control_literal_text[] =
        "<!doctype html><html><head><title>x</title></head><body><p>\v</p></body></html>";
    static const char control_literal_attr[] =
        "<!doctype html><html><head><title>x</title></head><body><p title=\"\v\">x</p></body></html>";
    if (expect_single_owner(
            span_from_cstr(control_literal_text),
            (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_LEXBOR_TOKENIZER,
            "html.parse.tokenizer.control-character-in-input-stream") != 0) return 8;
    if (expect_single_owner(
            span_from_cstr(control_literal_attr),
            (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_LEXBOR_TOKENIZER,
            "html.parse.tokenizer.control-character-in-input-stream") != 0) return 9;

    static const char control_ref_text[] =
        "<!doctype html><html><head><title>x</title></head><body><p>&#x0B;</p></body></html>";
    static const char control_ref_attr[] =
        "<!doctype html><html><head><title>x</title></head><body><p title=\"&#x0B;\">x</p></body></html>";
    if (expect_single_owner(
            span_from_cstr(control_ref_text),
            (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_LEXBOR_TOKENIZER,
            "html.parse.tokenizer.control-character-reference") != 0) return 10;
    if (expect_single_owner(
            span_from_cstr(control_ref_attr),
            (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_LEXBOR_TOKENIZER,
            "html.parse.tokenizer.control-character-reference") != 0) return 11;

    static const char surrogate_ref[] =
        "<!doctype html><html><head><title>x</title></head><body><p>&#xD800;</p></body></html>";
    static const char outside_ref[] =
        "<!doctype html><html><head><title>x</title></head><body><p>&#x110000;</p></body></html>";
    if (expect_single_owner(
            span_from_cstr(surrogate_ref),
            (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_LEXBOR_TOKENIZER,
            "html.parse.tokenizer.surrogate-character-reference") != 0) return 12;
    if (expect_single_owner(
            span_from_cstr(outside_ref),
            (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_LEXBOR_TOKENIZER,
            "html.parse.tokenizer.character-reference-outside-unicode-range") != 0) return 13;

    (void)puts("VIEW0_V1N1_G03_R6A_OWNERSHIP_LEDGER=7_OF_7");
    (void)puts("VIEW0_V1N1_G03_R6A_FIXTURE_PLAN=6_OF_6");
    (void)puts("VIEW0_V1N1_G03_R6A_DEDICATED_R6_DIAGNOSTICS=ZERO");
    (void)puts("PASS: VIEW0 V1N1 G03 R6A retained M1/tokenizer ownership covers scalar/noncharacter/control obligations");
    return 0;
}
