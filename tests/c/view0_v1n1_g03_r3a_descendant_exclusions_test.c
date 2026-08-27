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

static int expect(
    const char *html,
    uint64_t r1_count,
    uint64_t r2_count,
    uint64_t r3_count,
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
    if (count_rule(diagnostics, result.diagnostic_count,
                   ARBOR_VIEW_V1_G03_DESCENDANT_EXCLUSIONS) != r3_count) return 4;
    if ((result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_PARTIAL) == 0u) return 5;
    if ((result.flags & required_flags) != required_flags) return 6;
    return 0;
}

static int expect_parser_repair_owned(const char *html)
{
    arbor_view0_native_diagnostic diagnostics[64] = {{0}};
    arbor_view0_native_result result = {0};
    arbor_status status = arbor_view0_native_check(
        span_from_cstr(html), diagnostics, 64u, &result);
    if (status.native != 0) return 1;
    if (result.tree_error_count == 0u) return 2;
    if (count_rule(diagnostics, result.diagnostic_count,
                   ARBOR_VIEW_V1_G03_DESCENDANT_EXCLUSIONS) != 0u) return 3;
    return 0;
}

int main(void)
{
    if (expect("<!doctype html><title>x</title><p>x</p>", 0u, 0u, 0u, 0u) != 0) return 1;

    if (expect("<!doctype html><title>x</title><table><caption><div><table><tr><td>x</td></tr></table></div></caption></table>",
               0u, 0u, 1u, 0u) != 0) return 2;
    if (expect_parser_repair_owned(
        "<!doctype html><title>x</title><form><form></form></form>") != 0) return 3;
    if (expect("<!doctype html><title>x</title><dl><dt><header>x</header></dt><dd>y</dd></dl>",
               0u, 0u, 1u, 0u) != 0) return 4;
    if (expect("<!doctype html><title>x</title><table><tr><th><section>x</section></th></tr></table>",
               0u, 0u, 1u, 0u) != 0) return 5;
    if (expect("<!doctype html><title>x</title><header><div><footer>x</footer></div></header>",
               0u, 0u, 1u, 0u) != 0) return 6;
    if (expect("<!doctype html><title>x</title><footer><div><header>x</header></div></footer>",
               0u, 0u, 1u, 0u) != 0) return 7;
    if (expect("<!doctype html><title>x</title><address><h1>x</h1></address>",
               0u, 0u, 1u, 0u) != 0) return 8;

    if (expect("<!doctype html><title>x</title><select><option><a href=/>x</a></option></select>",
               0u, 0u, 1u, 0u) != 0) return 9;
    if (expect("<!doctype html><title>x</title><select><option><span><object></object></span></option></select>",
               0u, 0u, 1u, 0u) != 0) return 10;
    if (expect("<!doctype html><title>x</title><select><option><span tabindex=0>x</span></option></select>",
               0u, 0u, 1u, 0u) != 0) return 11;
    if (expect("<!doctype html><title>x</title><select><option label=x><span tabindex=0>x</span></option></select>",
               1u, 0u, 0u, 0u) != 0) return 12;

    if (expect("<!doctype html><title>x</title><video><div><audio></audio></div></video>",
               0u, 0u, 1u, 0u) != 0) return 13;
    if (expect("<!doctype html><title>x</title><audio><span><video></video></span></audio>",
               0u, 0u, 1u, 0u) != 0) return 14;
    if (expect("<!doctype html><title>x</title><select><optgroup><legend><a href=/>x</a></legend><option>x</option></optgroup></select>",
               0u, 0u, 1u, 0u) != 0) return 15;
    if (expect("<!doctype html><title>x</title><p><dfn><span><dfn>x</dfn></span></dfn></p>",
               0u, 0u, 1u, 0u) != 0) return 16;
    if (expect("<!doctype html><title>x</title><button><span><a href=/>x</a></span></button>",
               0u, 0u, 1u, 0u) != 0) return 17;
    if (expect("<!doctype html><title>x</title><p><meter><span><meter></meter></span></meter></p>",
               0u, 0u, 1u, 0u) != 0) return 18;
    if (expect("<!doctype html><title>x</title><p><progress><span><progress></progress></span></progress></p>",
               0u, 0u, 1u, 0u) != 0) return 19;

    if (expect("<!doctype html><title>x</title><label><span><label>x</label></span></label>",
               0u, 0u, 1u, 0u) != 0) return 20;
    if (expect("<!doctype html><title>x</title><label><input></label>",
               0u, 0u, 0u,
               ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_DEFERRED_LABELED_CONTROL) != 0) return 21;

    if (expect("<!doctype html><title>x</title><ruby><ruby>a<rt>x</rt></ruby><rt>y</rt></ruby>",
               0u, 0u, 0u, 0u) != 0) return 22;
    if (expect("<!doctype html><title>x</title><ruby><span><ruby>a<rt>x</rt></ruby></span><rt>y</rt></ruby>",
               0u, 0u, 1u, 0u) != 0) return 23;
    if (expect("<!doctype html><title>x</title><ruby><ruby><ruby>a<rt>x</rt></ruby><rt>y</rt></ruby><rt>z</rt></ruby>",
               0u, 0u, 1u, 0u) != 0) return 24;

    if (expect("<!doctype html><title>x</title><a href=/><span><button>x</button></span></a>",
               0u, 0u, 1u, 0u) != 0) return 25;
    if (expect_parser_repair_owned(
        "<!doctype html><title>x</title><a href=/><a href=/></a></a>") != 0) return 26;
    if (expect("<!doctype html><title>x</title><a href=/><input></a>",
               0u, 0u, 1u, 0u) != 0) return 27;
    if (expect("<!doctype html><title>x</title><a href=/><input type=hidden></a>",
               0u, 0u, 0u, 0u) != 0) return 28;

    if (expect("<!doctype html><title>x</title><canvas><textarea>x</textarea></canvas>",
               0u, 0u, 1u, 0u) != 0) return 29;
    if (expect("<!doctype html><title>x</title><canvas><button>x</button></canvas>",
               0u, 0u, 0u, 0u) != 0) return 30;
    if (expect("<!doctype html><title>x</title><canvas><img usemap=#m></canvas>",
               0u, 0u, 0u, 0u) != 0) return 31;
    if (expect("<!doctype html><title>x</title><canvas><select><option>x</option></select></canvas>",
               0u, 0u, 1u, 0u) != 0) return 32;
    if (expect("<!doctype html><title>x</title><canvas><select multiple><option>x</option></select></canvas>",
               0u, 0u, 0u, 0u) != 0) return 33;
    if (expect("<!doctype html><title>x</title><canvas><input type=checkbox></canvas>",
               0u, 0u, 0u, 0u) != 0) return 34;
    if (expect("<!doctype html><title>x</title><canvas><select size=2><option>x</option></select></canvas>",
               0u, 0u, 0u, 0u) != 0) return 35;

    if (expect("<!doctype html><title>x</title><body><noscript><span>x</span></noscript></body>",
               0u, 0u, 0u, 0u) != 0) return 36;

    /* Same element is invalid under R1 and a distant R3 ancestor: R1 wins. */
    if (expect("<!doctype html><title>x</title><address><span><h1>x</h1></span></address>",
               1u, 0u, 0u, 0u) != 0) return 37;

    puts("PASS: VIEW0 V1N1 G03 R3A reconciled input/canvas/select/noscript descendant exclusions, label boundary and R1 suppression");
    return 0;
}
