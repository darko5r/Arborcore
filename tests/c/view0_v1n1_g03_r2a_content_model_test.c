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
    uint64_t count,
    uint64_t rule_id)
{
    uint64_t found = 0u;
    for (uint64_t i = 0u; i < count; ++i) {
        if (diagnostics[i].rule_id == rule_id) found += 1u;
    }
    return found;
}

static int expect_counts(
    const char *html,
    uint64_t r1_count,
    uint64_t r2_count,
    uint64_t required_flags)
{
    arbor_view0_native_diagnostic diagnostics[64] = {{0}};
    arbor_view0_native_result result = {0};
    arbor_status status = arbor_view0_native_check(
        span_from_cstr(html), diagnostics, 64u, &result);
    if (status.native != 0) return 1;
    if (count_rule(diagnostics, result.diagnostic_count,
                   ARBOR_VIEW_V1_G03_ELEMENT_CONTEXT) != r1_count) return 2;
    if (count_rule(diagnostics, result.diagnostic_count,
                   ARBOR_VIEW_V1_G03_CONTENT_MODEL) != r2_count) return 3;
    if ((result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_PARTIAL) == 0u) return 4;
    if ((result.flags & required_flags) != required_flags) return 5;
    return 0;
}


static int expect_parser_owned(const char *html)
{
    arbor_view0_native_diagnostic diagnostics[64] = {{0}};
    arbor_view0_native_result result = {0};
    arbor_status status = arbor_view0_native_check(
        span_from_cstr(html), diagnostics, 64u, &result);
    if (status.native != 0) return 1;
    if (result.tree_error_count == 0u) return 2;
    if (count_rule(diagnostics, result.diagnostic_count,
                   ARBOR_VIEW_V1_G03_CONTENT_MODEL) != 0u) return 3;
    return 0;
}

int main(void)
{
    if (expect_counts(
        "<!doctype html><html><head><title>x</title></head><body><p>x</p></body></html>",
        0u, 0u, 0u) != 0) return 1;

    if (expect_counts(
        "<!doctype html><title>x</title><dl><dt>a</dt><dd>b</dd>"
        "<div><dt>c</dt><dd>d</dd></div></dl>", 0u, 1u, 0u) != 0) return 2;
    if (expect_counts(
        "<!doctype html><title>x</title><dl><div><script></script></div></dl>",
        0u, 1u, ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_DEFERRED_SCRIPT) != 0) return 3;
    if (expect_counts(
        "<!doctype html><title>x</title><figure><figcaption>a</figcaption>"
        "<p>x</p><figcaption>b</figcaption></figure>", 0u, 1u, 0u) != 0) return 4;
    if (expect_counts(
        "<!doctype html><title>x</title><datalist><span>x</span>"
        "<option>x</option></datalist>", 0u, 1u, 0u) != 0) return 5;
    if (expect_counts(
        "<!doctype html><title>x</title><details><p>x</p></details>",
        0u, 1u, 0u) != 0) return 6;
    if (expect_counts(
        "<!doctype html><title>x</title><select multiple><button>"
        "<selectedcontent></selectedcontent></button><option>x</option></select>",
        0u, 1u, 0u) != 0) return 7;
    if (expect_counts(
        "<!doctype html><title>x</title><select><button>"
        "<selectedcontent></selectedcontent><span><selectedcontent></selectedcontent>"
        "</span></button><option>x</option></select>", 0u, 1u, 0u) != 0) return 8;
    if (expect_counts(
        "<!doctype html><html><head><title>   </title></head><body></body></html>",
        0u, 1u, 0u) != 0) return 9;
    if (expect_counts(
        "<!doctype html><title>x</title><hgroup><p>sub</p></hgroup>",
        0u, 1u, 0u) != 0) return 10;
    if (expect_counts(
        "<!doctype html><title>x</title><picture><source srcset=x></picture>",
        0u, 1u, 0u) != 0) return 11;
    /* Lexbor tree construction moves the hr out of optgroup and emits a parse
     * error; the surviving DOM therefore has no R2 residual. R5 owns the
     * source-visible misuse erased by repair. */
    if (expect_parser_owned(
        "<!doctype html><title>x</title><select><optgroup><hr></optgroup></select>") != 0) return 12;
    if (expect_counts(
        "<!doctype html><title>x</title><select><optgroup>text<option>x</option></optgroup></select>",
        0u, 1u, 0u) != 0) return 13;
    if (expect_counts(
        "<!doctype html><title>x</title><datalist><span><option>x</option></span></datalist>",
        0u, 1u, 0u) != 0) return 14;

    if (expect_counts(
        "<!doctype html><title>x</title><ruby>a<rt>b</rt></ruby>",
        0u, 0u, 0u) != 0) return 15;
    if (expect_counts(
        "<!doctype html><title>x</title><ruby>a</ruby>",
        0u, 1u, 0u) != 0) return 16;
    if (expect_counts(
        "<!doctype html><title>x</title><ruby>a<rp>(</rp><rt>b</rt><rp>)</rp></ruby>",
        0u, 0u, 0u) != 0) return 17;

    /* R1 owns a lone trailing dt; R2 must not duplicate that child relation. */
    if (expect_counts(
        "<!doctype html><title>x</title><dl><dt>x</dt></dl>",
        1u, 0u, 0u) != 0) return 18;


    if (expect_counts(
        "<!doctype html><title>x</title><select><option><div>a</div>"
        "<span>b</span></option></select>", 0u, 1u, 0u) != 0) return 19;
    if (expect_counts(
        "<!doctype html><title>x</title><select><div><span><div>x</div>"
        "</span></div><option>x</option></select>", 1u, 1u, 0u) != 0) return 20;
    if (expect_counts(
        "<!doctype html><title>x</title><ruby>a<rt>x</rt>b<rt>y</rt></ruby>",
        0u, 0u, 0u) != 0) return 21;
    if (expect_counts(
        "<!doctype html><title>x</title><ruby><rt>x</rt></ruby>",
        0u, 1u, 0u) != 0) return 22;
    if (expect_counts(
        "<!doctype html><title>x</title><select><button></button>"
        "<option>x</option></select>", 0u, 0u, 0u) != 0) return 23;

    if (expect_counts(
        "<!doctype html><title>x</title><ul>text<li>x</li></ul>",
        0u, 1u, 0u) != 0) return 24;
    if (expect_counts(
        "<!doctype html><title>x</title><select>text<option>x</option></select>",
        0u, 1u, 0u) != 0) return 25;

    /* These source violations are consumed/repaired by tree construction; R2
     * must not fabricate a parent residual from a DOM that no longer contains
     * the authored Text relation. */
    if (expect_parser_owned(
        "<!doctype html><title>x</title><table><colgroup>text</colgroup></table>") != 0) return 26;
    if (expect_parser_owned(
        "<!doctype html><title>x</title><table>text<tr><td>x</td></tr></table>") != 0) return 27;
    if (expect_parser_owned(
        "<!doctype html><title>x</title><table><tr>text<td>x</td></tr></table>") != 0) return 28;
    if (expect_parser_owned(
        "<!doctype html><title>x</title><table><tbody>text<tr><td>x</td></tr></tbody></table>") != 0) return 29;

    /* Text-only parent models that contain a standard child remain R1-owned
     * under the frozen R1>R2 precedence; R2 must not duplicate them. The
     * datetime-present time branch admits phrasing content. */
    if (expect_counts(
        "<!doctype html><title>x</title><time><span>x</span></time>",
        1u, 0u, 0u) != 0) return 30;
    if (expect_counts(
        "<!doctype html><title>x</title><time datetime=2020-01-01><span>x</span></time>",
        0u, 0u, 0u) != 0) return 31;
    if (expect_counts(
        "<!doctype html><title>x</title><ruby>a<rp><span>(</span></rp>"
        "<rt>b</rt><rp>)</rp></ruby>",
        1u, 0u, 0u) != 0) return 32;

    const uint64_t deferred =
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_DEFERRED_STYLE |
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_DEFERRED_SCRIPT |
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_DEFERRED_SELECT_PLATFORM |
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_DEFERRED_UNCLASSIFIED;
    if (expect_counts(
        "<!doctype html><title>x</title><style>x{}</style><script></script>"
        "<noscript>x</noscript><select size=1 multiple><button></button>"
        "<option>x</option></select><p><x-widget></x-widget></p>",
        0u, 0u, deferred) != 0) return 33;

    puts("PASS: VIEW0 V1N1 G03 R2A residual content models, reconciled select/noscript ownership, R1 suppression and explicit retained deferrals");
    return 0;
}
