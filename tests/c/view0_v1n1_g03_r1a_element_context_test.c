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

static int expect_r1(
    const char *html,
    uint64_t expected_r1_count,
    int expect_deferred)
{
    arbor_view0_native_diagnostic diagnostics[64] = {{0}};
    arbor_view0_native_result result = {0};
    arbor_status status = arbor_view0_native_check(
        span_from_cstr(html), diagnostics, 64u, &result);
    if (status.native != 0 ||
        count_rule(diagnostics, result.diagnostic_count,
                   ARBOR_VIEW_V1_G03_ELEMENT_CONTEXT) != expected_r1_count ||
        (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R1_PARTIAL) == 0u) {
        return 1;
    }
    const int deferred =
        (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R1_DEFERRED_MAIN_FORM) != 0u;
    return deferred == expect_deferred ? 0 : 1;
}

int main(void)
{
    static const char canonical[] =
        "<!doctype html><html><head><title>x</title></head>"
        "<body><p>x</p></body></html>";
    if (expect_r1(canonical, 0u, 0) != 0) return 1;

    /* G03 context alternatives 1-4, 36-37: dl/dt/dd/div structural contexts. */
    static const char dl_valid[] =
        "<!doctype html><title>x</title><dl><dt>a</dt><dd>b</dd>"
        "<div><dt>c</dt><dd>d</dd></div></dl>";
    static const char dd_first[] =
        "<!doctype html><title>x</title><dl><dd>x</dd></dl>";
    static const char dt_trailing[] =
        "<!doctype html><title>x</title><dl><dt>x</dt></dl>";
    static const char dt_script_dd[] =
        "<!doctype html><title>x</title><dl><dt>x</dt><script></script><dd>y</dd></dl>";
    if (expect_r1(dl_valid, 0u, 0) != 0) return 2;
    if (expect_r1(dd_first, 1u, 0) != 0) return 3;
    if (expect_r1(dt_trailing, 1u, 0) != 0) return 34;
    if (expect_r1(dt_script_dd, 0u, 0) != 0) return 35;

    /* 5-8: media/picture prefix constraints, including conditional flow. */
    static const char media_valid[] =
        "<!doctype html><title>x</title><video><source src=x><track src=x>"
        "<p>x</p></video><picture><source srcset=x><img src=x alt=x></picture>";
    static const char media_link_before_source[] =
        "<!doctype html><title>x</title><video><link rel=stylesheet href=x>"
        "<source src=x></video>";
    static const char media_bad_link_before_source[] =
        "<!doctype html><title>x</title><video><link rel=alternate href=x>"
        "<source src=x></video>";
    static const char picture_bad[] =
        "<!doctype html><title>x</title><picture><img src=x alt=x>"
        "<source srcset=x></picture>";
    if (expect_r1(media_valid, 0u, 0) != 0) return 4;
    if (expect_r1(media_link_before_source, 1u, 0) != 0) return 5;
    /* The invalid link itself is one R1 error; source remains correctly prefix-valid. */
    if (expect_r1(media_bad_link_before_source, 1u, 0) != 0) return 6;
    if (expect_r1(picture_bad, 1u, 0) != 0) return 7;

    /* 9-10: ruby rt/rp contexts. */
    static const char ruby_valid[] =
        "<!doctype html><title>x</title><ruby>x<rt>a</rt><rp>(</rp>"
        "<rt>b</rt><rp>)</rp></ruby>";
    static const char ruby_bad[] =
        "<!doctype html><title>x</title><ruby><rp>(</rp><span>x</span></ruby>";
    if (expect_r1(ruby_valid, 0u, 0) != 0) return 8;
    if (expect_r1(ruby_bad, 1u, 0) != 0) return 9;

    /* 11-19,32: table family ordering and direct contexts. */
    static const char table_valid[] =
        "<!doctype html><title>x</title><table><caption>x</caption>"
        "<colgroup><col></colgroup><thead><tr><th>x</th></tr></thead>"
        "<tbody><tr><td>x</td></tr></tbody><tfoot><tr><td>x</td></tr></tfoot>"
        "</table>";
    static const char table_bad_caption[] =
        "<!doctype html><title>x</title><table><colgroup></colgroup>"
        "<caption>x</caption></table>";
    static const char col_bad_span[] =
        "<!doctype html><title>x</title><table><colgroup span=2><col>"
        "</colgroup></table>";
    static const char table_early_tfoot[] =
        "<!doctype html><title>x</title><table><tfoot><tr><td>x</td></tr></tfoot>"
        "<tbody><tr><td>y</td></tr></tbody></table>";
    static const char table_final_tfoot[] =
        "<!doctype html><title>x</title><table><tbody><tr><td>x</td></tr></tbody>"
        "<tfoot><tr><td>y</td></tr></tfoot></table>";
    if (expect_r1(table_valid, 0u, 0) != 0) return 10;
    if (expect_r1(table_bad_caption, 1u, 0) != 0) return 11;
    if (expect_r1(col_bad_span, 1u, 0) != 0) return 12;
    if (expect_r1(table_early_tfoot, 1u, 0) != 0) return 36;
    if (expect_r1(table_final_tfoot, 0u, 0) != 0) return 37;

    /* 20: hgroup child contexts. */
    static const char hgroup_valid[] =
        "<!doctype html><title>x</title><hgroup><h1>x</h1><p>sub</p></hgroup>";
    static const char hgroup_bad[] =
        "<!doctype html><title>x</title><hgroup><div>x</div></hgroup>";
    if (expect_r1(hgroup_valid, 0u, 0) != 0) return 13;
    if (expect_r1(hgroup_bad, 1u, 0) != 0) return 14;

    /* 21-26,30-31: select/datalist/option/optgroup ordered ancestry. */
    static const char select_valid[] =
        "<!doctype html><title>x</title><select>\n<button><span>"
        "<selectedcontent></selectedcontent></span></button>"
        "<optgroup><legend>x</legend><option><div><div>x</div></div></option>"
        "</optgroup><hr></select><datalist><option>text</option></datalist>";
    static const char selectedcontent_bad[] =
        "<!doctype html><title>x</title><button>"
        "<selectedcontent></selectedcontent></button>";
    static const char option_bad[] =
        "<!doctype html><title>x</title><div><option>x</option></div>";
    if (expect_r1(select_valid, 0u, 0) != 0) return 15;
    if (expect_r1(selectedcontent_bad, 1u, 0) != 0) return 16;
    if (expect_r1(option_bad, 1u, 0) != 0) return 17;

    /* 28-29,34: first/last meaningful child semantics ignore inter-element whitespace. */
    static const char first_last_valid[] =
        "<!doctype html><title>x</title><details> \n<summary>x</summary><p>x</p>"
        "</details><fieldset> \n<legend>x</legend><p>x</p></fieldset>"
        "<figure>text<figcaption>x</figcaption></figure>";
    static const char summary_bad[] =
        "<!doctype html><title>x</title><details>text<summary>x</summary></details>";
    static const char figcaption_bad[] =
        "<!doctype html><title>x</title><figure>text<figcaption>x</figcaption>"
        "more</figure>";
    if (expect_r1(first_last_valid, 0u, 0) != 0) return 18;
    if (expect_r1(summary_bad, 1u, 0) != 0) return 19;
    if (expect_r1(figcaption_bad, 1u, 0) != 0) return 20;

    /* 38-48: meta/link/title/base/noscript attribute-conditioned contexts. */
    static const char metadata_valid[] =
        "<!doctype html><html><head><title>x</title><base href=/><meta charset=utf-8>"
        "<meta name=description content=x><link rel=stylesheet href=x>"
        "<noscript><link rel=stylesheet href=x><style>x{}</style>"
        "<meta http-equiv=refresh content=x></noscript></head><body>"
        "<p><link rel=preload href=x><meta itemprop=x content=y>x</p></body></html>";
    static const char link_bad_body[] =
        "<!doctype html><title>x</title><p><link rel=alternate href=x>x</p>";
    static const char meta_bad_body[] =
        "<!doctype html><title>x</title><p><meta charset=utf-8>x</p>";
    if (expect_r1(metadata_valid, 0u, 0) != 0) return 21;
    if (expect_r1(link_bad_body, 1u, 0) != 0) return 22;
    if (expect_r1(meta_bad_body, 1u, 0) != 0) return 23;

    /* 49-51,58: list and map-ancestor contexts. */
    static const char list_map_valid[] =
        "<!doctype html><title>x</title><ul><li>x</li></ul><ol><li>x</li></ol>"
        "<menu><li>x</li></menu><map name=m><area href=x alt=x></map>";
    static const char li_bad[] =
        "<!doctype html><title>x</title><div><li>x</li></div>";
    static const char area_bad[] =
        "<!doctype html><title>x</title><p><area href=x alt=x></p>";
    if (expect_r1(list_map_valid, 0u, 0) != 0) return 24;
    if (expect_r1(li_bad, 1u, 0) != 0) return 25;
    if (expect_r1(area_bad, 1u, 0) != 0) return 26;

    /* 52,54-61 category-based contexts and transparent inheritance. */
    static const char category_valid[] =
        "<!doctype html><title>x</title><article><section><nav><aside>"
        "<header><h2>x</h2></header><p><a href=x><em>x</em><span>x</span>"
        "<img src=x alt=x></a><script></script><template></template></p>"
        "</aside></nav></section></article>";
    static const char custom_transparent_valid[] =
        "<!doctype html><title>x</title><p><x-shell><span>x</span></x-shell></p>";
    if (expect_r1(category_valid, 0u, 0) != 0) return 27;
    if (expect_r1(custom_transparent_valid, 0u, 0) != 0) return 28;

    /* 53: main hierarchy + AN-P0 tri-state accessible-name deferral. */
    static const char main_plain_form[] =
        "<!doctype html><title>x</title><form><div><main><p>x</p></main></div></form>";
    static const char main_named_form[] =
        "<!doctype html><title>x</title><form aria-label=n><div><main><p>x</p>"
        "</main></div></form>";
    static const char main_custom_ancestor[] =
        "<!doctype html><title>x</title><x-shell><div><main><p>x</p></main>"
        "</div></x-shell>";
    static const char main_bad_hierarchy[] =
        "<!doctype html><title>x</title><section><main><p>x</p></main></section>";
    if (expect_r1(main_plain_form, 0u, 0) != 0) return 29;
    if (expect_r1(main_named_form, 0u, 1) != 0) return 30;
    if (expect_r1(main_custom_ancestor, 0u, 0) != 0) return 31;
    if (expect_r1(main_bad_hierarchy, 1u, 0) != 0) return 32;

    /* Stable diagnostic identity and source anchor. */
    arbor_view0_native_diagnostic diagnostics[8] = {{0}};
    arbor_view0_native_result result = {0};
    static const char anchored[] =
        "<!doctype html><title>x</title><div><li>x</li></div>";
    arbor_status status = arbor_view0_native_check(
        span_from_cstr(anchored), diagnostics, 8u, &result);
    const arbor_view0_native_diagnostic *r1 = find_rule(
        diagnostics, result.diagnostic_count, ARBOR_VIEW_V1_G03_ELEMENT_CONTEXT);
    const char *li = strstr(anchored, "<li>");
    if (status.native != 0 || r1 == NULL || li == NULL ||
        r1->severity != (uint32_t)ARBOR_VIEW0_NATIVE_SEVERITY_ERROR ||
        r1->origin != (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_AUTHORING ||
        r1->byte_offset != (uint64_t)((li + 1) - anchored) ||
        r1->source_length != 2u ||
        strcmp(r1->symbolic_name, "ARBOR_VIEW_V1_G03_ELEMENT_CONTEXT") != 0 ||
        strcmp(r1->message,
               "HTML element is not permitted in this structural context") != 0) {
        return 33;
    }

    puts("PASS: VIEW0 V1N1 G03 R1A definite structural contexts, source-order rules, category inheritance and explicit main/form deferral");
    return 0;
}
