#include <arborcore/view0_conformance/native.h>

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static arbor_span span_from_bytes(const uint8_t *bytes, uint64_t length)
{
    return (arbor_span){bytes, length};
}

static arbor_span span_from_cstr(const char *text)
{
    return span_from_bytes((const uint8_t *)text, (uint64_t)strlen(text));
}

static int expect_parse_clean(const char *html)
{
    arbor_view0_native_result result = {0};
    arbor_status status = arbor_view0_native_check(
        span_from_cstr(html),
        NULL,
        0u,
        &result);
    if (status.native != 0 || result.diagnostic_count != 0u ||
        (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_PARSE_CLEAN) == 0u) {
        return 1;
    }
    return 0;
}

static int expect_origin_error(
    const uint8_t *html,
    uint64_t length,
    arbor_view0_native_origin origin,
    const char *symbol_fragment)
{
    arbor_view0_native_diagnostic diagnostics[16] = {{0}};
    arbor_view0_native_result result = {0};
    arbor_status status = arbor_view0_native_check(
        span_from_bytes(html, length),
        diagnostics,
        16u,
        &result);
    if (status.native != 0 || result.diagnostic_count == 0u) {
        return 1;
    }

    for (uint64_t i = 0u; i < result.diagnostic_count; ++i) {
        if (diagnostics[i].origin == (uint32_t)origin &&
            strstr(diagnostics[i].symbolic_name, symbol_fragment) != NULL &&
            diagnostics[i].line != 0u && diagnostics[i].column != 0u &&
            diagnostics[i].byte_offset <= length) {
            return 0;
        }
    }

    return 1;
}

int main(void)
{
    static const char valid[] =
        "<!doctype html>\n"
        "<html lang=\"en\">\n"
        "<head><meta charset=\"utf-8\"><title>V1N0</title></head>\n"
        "<body><main><p>Hello</p></main></body>\n"
        "</html>\n";
    if (expect_parse_clean(valid) != 0) {
        return 1;
    }

    /* Higher admitted authoring-rule layers may add document diagnostics to
     * empty input. V1N0 retains authority only over parser cleanliness here. */
    arbor_view0_native_diagnostic empty_diagnostic[16] = {{0}};
    arbor_view0_native_result empty_result = {0};
    arbor_status empty_status = arbor_view0_native_check(
        (arbor_span){NULL, 0u}, empty_diagnostic, 16u, &empty_result);
    if (empty_status.native != 0 || empty_result.tokenizer_error_count != 0u ||
        empty_result.tree_error_count != 0u ||
        (empty_result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_PARSE_CLEAN) == 0u) {
        return 2;
    }

    static const char tokenizer_error[] =
        "<!doctype html>\n<html><head><title>x</title></head>\n<body>\n"
        "<?bad>\n</body></html>\n";
    if (expect_origin_error(
            (const uint8_t *)tokenizer_error,
            (uint64_t)(sizeof(tokenizer_error) - 1u),
            ARBOR_VIEW0_NATIVE_ORIGIN_LEXBOR_TOKENIZER,
            "unexpected-question-mark-instead-of-tag-name") != 0) {
        return 3;
    }

    static const char duplicate_attribute[] =
        "<!doctype html><html><head><title>x</title></head>"
        "<body><div a=1 a=2></div></body></html>";
    if (expect_origin_error(
            (const uint8_t *)duplicate_attribute,
            (uint64_t)(sizeof(duplicate_attribute) - 1u),
            ARBOR_VIEW0_NATIVE_ORIGIN_LEXBOR_TOKENIZER,
            "duplicate-attribute") != 0) {
        return 4;
    }

    static const uint8_t input_control[] = {
        '<','!','d','o','c','t','y','p','e',' ','h','t','m','l','>',
        '<','h','t','m','l','>','<','h','e','a','d','>',
        '<','t','i','t','l','e','>','x','<','/','t','i','t','l','e','>',
        '<','/','h','e','a','d','>','<','b','o','d','y','>',
        0x01,
        '<','/','b','o','d','y','>','<','/','h','t','m','l','>'
    };
    if (expect_origin_error(
            input_control,
            (uint64_t)sizeof(input_control),
            ARBOR_VIEW0_NATIVE_ORIGIN_LEXBOR_TOKENIZER,
            "control-character-in-input-stream") != 0) {
        return 5;
    }

    static const char tree_error[] =
        "<!doctype html>\n<html><head><title>x</title></head>\n<body>\n"
        "<!doctype html>\n<p>x</p></body></html>\n";
    if (expect_origin_error(
            (const uint8_t *)tree_error,
            (uint64_t)(sizeof(tree_error) - 1u),
            ARBOR_VIEW0_NATIVE_ORIGIN_LEXBOR_TREE,
            "doctype-token-in-body-mode") != 0) {
        return 6;
    }

    static const uint8_t invalid_utf8[] = {
        '<','p','>', UINT8_C(0xf0), UINT8_C(0x80), UINT8_C(0x80), UINT8_C(0x80),
        '<','/','p','>'
    };
    arbor_view0_native_diagnostic utf8_diag[1] = {{0}};
    arbor_view0_native_result utf8_result = {0};
    arbor_status utf8_status = arbor_view0_native_check(
        span_from_bytes(invalid_utf8, (uint64_t)sizeof(invalid_utf8)),
        utf8_diag,
        1u,
        &utf8_result);
    if (utf8_status.native != 0 || utf8_result.diagnostic_count != 1u ||
        utf8_diag[0].origin != (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_UTF8 ||
        utf8_diag[0].byte_offset != 4u || utf8_diag[0].line != 1u ||
        utf8_diag[0].column != 5u) {
        return 7;
    }

    puts("PASS: VIEW0 V1N0 parse-clean, tokenizer/tree diagnostics, conformance tokenizer options and UTF-8-first boundary");
    return 0;
}
