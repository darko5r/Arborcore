#include <arborcore/view0_conformance/native.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static arbor_span span_from_cstr(const char *text)
{
    return (arbor_span){(const uint8_t *)text, (uint64_t)strlen(text)};
}

static uint64_t count_rule(
    const arbor_view0_native_diagnostic *diagnostics,
    uint64_t diagnostic_count,
    uint64_t rule_id)
{
    uint64_t count = 0u;
    for (uint64_t i = 0u; i < diagnostic_count; ++i) {
        if (diagnostics[i].rule_id == rule_id) {
            count += 1u;
        }
    }
    return count;
}

static const arbor_view0_native_diagnostic *find_rule(
    const arbor_view0_native_diagnostic *diagnostics,
    uint64_t diagnostic_count,
    uint64_t rule_id)
{
    for (uint64_t i = 0u; i < diagnostic_count; ++i) {
        if (diagnostics[i].rule_id == rule_id) {
            return diagnostics + i;
        }
    }
    return NULL;
}

static uint64_t source_name_offset(const char *html, const char *needle)
{
    const char *position = strstr(html, needle);
    return position == NULL ? UINT64_MAX : (uint64_t)(position - html) + 1u;
}

static int expect_foster_r5(
    const char *html,
    const char *start_tag,
    uint64_t expected_name_length)
{
    arbor_view0_native_diagnostic diagnostics[64] = {{0}};
    arbor_view0_native_result result = {0};
    arbor_status status = arbor_view0_native_check(
        span_from_cstr(html), diagnostics, 64u, &result);
    if (status.native != 0) {
        return 1;
    }
    if (result.tokenizer_error_count != 0u || result.tree_error_count != 0u ||
        count_rule(diagnostics, result.diagnostic_count, ARBOR_VIEW_V1_G03_ELEMENT_CONTEXT) != 0u ||
        count_rule(diagnostics, result.diagnostic_count, ARBOR_VIEW_V1_G03_CONTENT_MODEL) != 0u ||
        count_rule(diagnostics, result.diagnostic_count, ARBOR_VIEW_V1_G03_DESCENDANT_EXCLUSIONS) != 0u ||
        count_rule(diagnostics, result.diagnostic_count, ARBOR_VIEW_V1_G03_NOTHING_MODEL) != 0u) {
        return 2;
    }
    if ((result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R5_PARTIAL) == 0u) {
        return 3;
    }
    if (count_rule(
            diagnostics, result.diagnostic_count,
            ARBOR_VIEW_V1_G03_EXPLICIT_HTML_ELEMENT_ALLOWANCE) != 1u) {
        return 4;
    }
    const arbor_view0_native_diagnostic *diagnostic = find_rule(
        diagnostics, result.diagnostic_count,
        ARBOR_VIEW_V1_G03_EXPLICIT_HTML_ELEMENT_ALLOWANCE);
    if (diagnostic == NULL) {
        return 5;
    }
    const uint64_t expected_offset = source_name_offset(html, start_tag);
    if (expected_offset == UINT64_MAX || diagnostic->byte_offset != expected_offset ||
        diagnostic->source_length != expected_name_length) {
        return 6;
    }
    if (diagnostic->severity != (uint32_t)ARBOR_VIEW0_NATIVE_SEVERITY_ERROR ||
        diagnostic->origin != (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_AUTHORING) {
        return 7;
    }
    if (strcmp(
            diagnostic->symbolic_name,
            "ARBOR_VIEW_V1_G03_EXPLICIT_HTML_ELEMENT_ALLOWANCE") != 0) {
        return 8;
    }
    return 0;
}

static int expect_no_r5(const char *html)
{
    arbor_view0_native_diagnostic diagnostics[64] = {{0}};
    arbor_view0_native_result result = {0};
    arbor_status status = arbor_view0_native_check(
        span_from_cstr(html), diagnostics, 64u, &result);
    if (status.native != 0) {
        return 1;
    }
    if ((result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R5_PARTIAL) == 0u) {
        return 2;
    }
    if (count_rule(
            diagnostics, result.diagnostic_count,
            ARBOR_VIEW_V1_G03_EXPLICIT_HTML_ELEMENT_ALLOWANCE) != 0u) {
        return 3;
    }
    if (result.diagnostic_count != 0u || result.tokenizer_error_count != 0u ||
        result.tree_error_count != 0u) {
        return 4;
    }
    return 0;
}

int main(void)
{
    static const char table_p[] =
        "<!doctype html><title>x</title><body><table><p>x</p></table></body>";
    static const char table_div[] =
        "<!doctype html><title>x</title><body><table><div>x</div></table></body>";
    static const char tbody_p[] =
        "<!doctype html><title>x</title><body><table><tbody><p>x</p></tbody></table></body>";
    static const char tr_div[] =
        "<!doctype html><title>x</title><body><table><tbody><tr><div>x</div></tr></tbody></table></body>";
    static const char colgroup_div[] =
        "<!doctype html><title>x</title><body><table><colgroup><div>x</div></table></body>";

    if (expect_foster_r5(table_p, "<p>", 1u) != 0) return 1;
    if (expect_foster_r5(table_div, "<div>", 3u) != 0) return 2;
    if (expect_foster_r5(tbody_p, "<p>", 1u) != 0) return 3;
    if (expect_foster_r5(tr_div, "<div>", 3u) != 0) return 4;
    if (expect_foster_r5(colgroup_div, "<div>", 3u) != 0) return 5;

    static const char table_li[] =
        "<!doctype html><title>x</title><body><table><li>x</li></table></body>";
    arbor_view0_native_diagnostic suppression[64] = {{0}};
    arbor_view0_native_result suppression_result = {0};
    arbor_status status = arbor_view0_native_check(
        span_from_cstr(table_li), suppression, 64u, &suppression_result);
    if (status.native != 0 ||
        count_rule(
            suppression, suppression_result.diagnostic_count,
            ARBOR_VIEW_V1_G03_ELEMENT_CONTEXT) != 1u ||
        count_rule(
            suppression, suppression_result.diagnostic_count,
            ARBOR_VIEW_V1_G03_EXPLICIT_HTML_ELEMENT_ALLOWANCE) != 0u ||
        suppression_result.diagnostic_count != 1u ||
        suppression_result.tokenizer_error_count != 0u ||
        suppression_result.tree_error_count != 0u) {
        return 6;
    }

    static const char table_tr[] =
        "<!doctype html><title>x</title><body><table><tr><td>x</td></tr></table></body>";
    static const char body_p[] =
        "<!doctype html><title>x</title><body><p>x</p></body>";
    static const char td_div[] =
        "<!doctype html><title>x</title><body><table><tr><td><div>x</div></td></tr></table></body>";
    static const char foreignobject_p[] =
        "<!doctype html><title>x</title><body><table><svg><foreignObject><p>x</p></foreignObject></svg></table></body>";
    if (expect_no_r5(table_tr) != 0) return 7;
    if (expect_no_r5(body_p) != 0) return 8;
    if (expect_no_r5(td_div) != 0) return 9;
    if (expect_no_r5(foreignobject_p) != 0) return 10;

    (void)puts(
        "PASS: VIEW0 V1N1 G03 R5A actual foster residuals, exact source anchors, prior-owner suppression and non-table foster-state controls");
    return 0;
}
