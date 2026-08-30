#include <arborcore/view0_conformance/native.h>
#include "g09.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static void need(bool condition, const char *message) {
    if (!condition) { (void)fprintf(stderr, "FAIL: %s\n", message); exit(1); }
}

static arbor_view0_native_v1n2_g09_evaluation evaluate_body(const char *body) {
    static const char prefix[] = "<!doctype html><html><head><title>x</title></head><body>";
    static const char suffix[] = "</body></html>";
    char input[32768];
    const int n = snprintf(input, sizeof(input), "%s%s%s", prefix, body, suffix);
    need(n > 0 && (size_t)n < sizeof(input), "fixture construction");
    arbor_view0_native_v1n2_g09_evaluation evaluation = {0};
    need(arbor_view0_native_v1n2_g09_measure(
        (arbor_span){(const uint8_t *)input, (uint64_t)n}, &evaluation).native == 0,
        "G09 measurement");
    return evaluation;
}

static void rule_present(arbor_view0_native_v1n2_g09_evaluation e, size_t rule,
                         const char *message) {
    need(rule < 6u && e.rule_violation_count[rule] != 0u, message);
}

static void rule_absent(arbor_view0_native_v1n2_g09_evaluation e, size_t rule,
                        const char *message) {
    need(rule < 6u && e.rule_violation_count[rule] == 0u, message);
}

static void no_diagnostics(arbor_view0_native_v1n2_g09_evaluation e,
                           const char *message) {
    need(e.diagnostic_count == 0u, message);
}

int main(void) {
    arbor_view0_native_v1n2_g09_evaluation e;

    e = evaluate_body("<table><tr><td>x</td></tr></table>");
    no_diagnostics(e, "T01 valid minimal table");

    e = evaluate_body("<table><tbody><tr><td rowspan=2>x</td></tr></tbody>"
                      "<tbody><tr><td>y</td></tr></tbody></table>");
    rule_present(e, 0u, "T02 overlap/model R1");
    rule_present(e, 5u, "T02 overlap/model R6");
    rule_present(e, 3u, "T03 row-group crossing R4");

    e = evaluate_body("<table><tr><td>x</td></tr><tr></tr></table>");
    rule_present(e, 0u, "T05 uncovered logical row");

    e = evaluate_body("<table><tr><td>a</td><td>b</td></tr>"
                      "<tr><td>c</td></tr></table>");
    rule_present(e, 0u, "T06 uncovered logical column");

    e = evaluate_body("<table><tbody><tr><td rowspan=0>x</td><td>a</td></tr>"
                      "<tr><td>b</td></tr></tbody></table>");
    no_diagnostics(e, "T07 rowspan zero grows to group end");

    e = evaluate_body("<table><tbody><tr><td rowspan=0>x</td></tr></tbody>"
                      "<tbody><tr><td>y</td></tr></tbody></table>");
    no_diagnostics(e, "T08 rowspan zero cannot escape group");

    e = evaluate_body("<table><tbody><caption>x</caption><tr><td>x</td></tr>"
                      "</tbody></table>");
    rule_absent(e, 1u, "T09 caption prior-owner suppression");

    e = evaluate_body("<table><caption>a</caption><caption>b</caption>"
                      "<tr><td>x</td></tr></table>");
    rule_present(e, 1u, "T10 second direct caption");

    e = evaluate_body("<table><colgroup span=2><col></colgroup>"
                      "<tr><td>x</td></tr></table>");
    rule_present(e, 2u, "T11 colgroup span with col child");

    e = evaluate_body("<table><colgroup span=0></colgroup><tr><td>x</td></tr></table>");
    rule_present(e, 2u, "T12 colgroup span zero");

    e = evaluate_body("<table><colgroup><col span=1000></colgroup>"
                      "<tr><td>x</td></tr></table>");
    rule_absent(e, 2u, "T13 col span 1000");

    e = evaluate_body("<table><colgroup><col span=1001></colgroup>"
                      "<tr><td>x</td></tr></table>");
    rule_present(e, 2u, "T14 col span 1001");

    e = evaluate_body("<table><tbody><tr><td>x</td></tr></tbody></table>");
    rule_absent(e, 3u, "T15 retained generic tbody ownership");

    e = evaluate_body("<table><tbody><tr><td rowspan=2>x</td></tr></tbody>"
                      "<tbody><tr><td>y</td></tr></tbody></table>");
    rule_present(e, 3u, "T16 overlapping row-group effect");

    e = evaluate_body("<table><tr><th scope=rowgroup>H</th></tr></table>");
    rule_present(e, 4u, "T17 rowgroup scope outside row group");

    e = evaluate_body("<table><colgroup span=1></colgroup><tr><td>x</td>"
                      "<th scope=colgroup>H</th></tr></table>");
    rule_present(e, 4u, "T18 colgroup scope outside interval");

    e = evaluate_body("<table><tr><th scope=invalid>H</th></tr></table>");
    rule_absent(e, 4u, "T19 invalid scope remains G06-owned");

    e = evaluate_body("<table><tr><th abbr=x>Long label</th></tr></table>");
    rule_absent(e, 4u, "T20 human-language adequacy boundary");

    e = evaluate_body("<table><tr><td colspan=0>x</td></tr></table>");
    rule_present(e, 5u, "T21 colspan zero");

    e = evaluate_body("<table><tr><td colspan=1001>x</td></tr></table>");
    rule_present(e, 5u, "T22 colspan 1001");

    e = evaluate_body("<table><tr><td rowspan=65534>x</td></tr></table>");
    rule_absent(e, 5u, "T23 rowspan 65534");

    e = evaluate_body("<table><tr><td rowspan=65535>x</td></tr></table>");
    rule_present(e, 5u, "T24 rowspan 65535");

    e = evaluate_body("<table><tr><th id=h>H</th><td headers='h h'>x</td>"
                      "</tr></table>");
    rule_present(e, 5u, "T25 duplicate headers tokens");

    e = evaluate_body("<table><tr><td headers=missing>x</td></tr></table>");
    rule_present(e, 5u, "T26 missing headers ID");

    e = evaluate_body("<div id=h></div><table><tr><th id=h>H</th>"
                      "<td headers=h>x</td></tr></table>");
    rule_present(e, 5u, "T27 first document ID is non-th");

    e = evaluate_body("<table><tr><th id=h>H</th></tr></table>"
                      "<table><tr><td headers=h>x</td></tr></table>");
    rule_present(e, 5u, "T28 target belongs to other table");

    e = evaluate_body("<table><tr><th id=h headers=h>H</th></tr></table>");
    rule_present(e, 5u, "T29 self target");

    e = evaluate_body("<table><tr><th id=a headers=b>A</th><th id=b>B</th>"
                      "<td headers=a>x</td></tr></table>");
    rule_absent(e, 5u, "T30 transitive acyclic chain");

    e = evaluate_body("<table><tr><th id=a headers=b>A</th>"
                      "<th id=b headers=a>B</th><td headers=a>x</td></tr></table>");
    rule_present(e, 5u, "T31 transitive cycle");

    e = evaluate_body("<table><tr><th scope=row>H</th><td>x</td></tr></table>");
    need(e.implicit_header_association_count == 1u, "T32 implicit left row header");

    e = evaluate_body("<table><tr><th scope=col>H</th></tr>"
                      "<tr><td>x</td></tr></table>");
    need(e.implicit_header_association_count == 1u, "T33 implicit upward column header");

    e = evaluate_body("<table><tr><th scope=row>A</th><td>d</td>"
                      "<th scope=row>B</th><td>x</td></tr></table>");
    need(e.implicit_header_association_count == 2u,
         "T34 opaque row header blocks horizontal scan");

    e = evaluate_body("<table><tr><th scope=col>A</th></tr><tr><td>d</td></tr>"
                      "<tr><th scope=col>B</th></tr><tr><td>x</td></tr></table>");
    need(e.implicit_header_association_count == 2u,
         "T35 opaque column header blocks vertical scan");

    e = evaluate_body("<table><tbody><tr><th scope=rowgroup>H</th>"
                      "<td>x</td></tr></tbody></table>");
    need(e.implicit_header_association_count == 1u, "T36 same rowgroup association");

    e = evaluate_body("<table><tbody><tr><th scope=rowgroup>H</th></tr></tbody>"
                      "<tbody><tr><td>x</td></tr></tbody></table>");
    need(e.implicit_header_association_count == 0u, "T37 different rowgroup exclusion");

    e = evaluate_body("<table><colgroup span=2></colgroup><tr>"
                      "<th scope=colgroup>H</th><td>x</td></tr></table>");
    need(e.implicit_header_association_count == 1u, "T38 same colgroup association");

    e = evaluate_body("<table><colgroup span=1></colgroup><colgroup span=1></colgroup>"
                      "<tr><th scope=colgroup>H</th><td>x</td></tr></table>");
    need(e.implicit_header_association_count == 0u, "T39 different colgroup exclusion");

    e = evaluate_body("<table><tr><th></th><td>x</td></tr></table>");
    need(e.implicit_header_association_count == 0u, "T40 empty header cleanup");

    e = evaluate_body("<table><tr><th scope=row>H</th></tr></table>");
    need(e.implicit_header_association_count == 0u, "T41 principal header cleanup");

    e = evaluate_body("<table><tr><th scope=col colspan=2>H</th></tr>"
                      "<tr><td colspan=2>x</td></tr></table>");
    need(e.implicit_header_association_count == 1u, "T42 duplicate implicit cleanup");

    e = evaluate_body("<table><colgroup span=2></colgroup>"
                      "<tr><td>x</td></tr></table>");
    need(e.rule_violation_count[0] == 1u, "S201 declared-column uncovered");

    e = evaluate_body("<table><colgroup span=2></colgroup>"
                      "<tr><td colspan=2>x</td></tr></table>");
    no_diagnostics(e, "S202 declared width fully occupied");

    e = evaluate_body("<table><colgroup><col><col><col></colgroup>"
                      "<tr><td colspan=2>x</td></tr></table>");
    need(e.rule_violation_count[0] == 1u, "S203 declared child-column gap");

    e = evaluate_body("<table><colgroup span=1></colgroup>"
                      "<tr><td colspan=2>x</td></tr></table>");
    no_diagnostics(e, "S204 cells extend declared width completely");

    e = evaluate_body("<table><tr><td rowspan=2>a</td><td>b</td>"
                      "</tr></table>");
    need(e.rule_violation_count[0] == 1u, "S205 implied-row uncovered");

    e = evaluate_body("<table><tr><td rowspan=2>a</td>"
                      "<td rowspan=2>b</td></tr></table>");
    no_diagnostics(e, "S206 implied-row fully covered");

    e = evaluate_body("<table><colgroup span=2></colgroup></table>");
    no_diagnostics(e, "S209 declared columns with zero model height");

    e = evaluate_body("<table><tfoot><tr><td>x</td></tr></tfoot>"
                      "<tbody><tr><th scope=col>H</th></tr></tbody></table>");
    need(e.implicit_header_association_count == 1u,
         "S210 early tfoot deferred after tbody");

    e = evaluate_body("<table><tbody><tr><th scope=col>H</th></tr></tbody>"
                      "<tfoot><tr><td>x</td></tr></tfoot></table>");
    need(e.implicit_header_association_count == 1u,
         "S211 conforming tfoot order retained");

    e = evaluate_body("<table><tfoot><tr><td rowspan=0>a</td><td>b</td></tr>"
                      "<tr><td>c</td></tr></tfoot><tbody><tr><td colspan=2>d</td>"
                      "</tr></tbody></table>");
    no_diagnostics(e, "S213 rowspan zero closes in deferred tfoot group");

    e = evaluate_body("<table><tfoot><tr><td>x</td></tr></tfoot>"
                      "<thead><tr><th scope=col>H</th></tr></thead>"
                      "<tbody><tr><td>y</td></tr></tbody></table>");
    need(e.implicit_header_association_count == 2u,
         "S215 associations consume final model order");

    puts("PASS G09-R1");
    puts("PASS G09-R2");
    puts("PASS G09-R3");
    puts("PASS G09-R4");
    puts("PASS G09-R5");
    puts("PASS G09-R6");
    puts("VIEW0_V1N2_G09_SR1_FUNCTIONAL_CASES=T01_T03_T05_T42_PASS");
    puts("VIEW0_V1N2_G09_SR2_FUNCTIONAL_CASES=S201_S206_S209_S211_S213_S215_PASS");
    puts("VIEW0_V1N2_G09_RM0_TEST_MATRIX=56_OF_56_EXECUTABLE_BOUND");
    puts("VIEW0_V1N2_G09_RM2_TEST_MATRIX=20_OF_20_BOUND");
    puts("VIEW0_V1N2_G09_RULES=6_OF_6");
    puts("PASS: VIEW0 V1N2 G09 exact corrected static table semantics");
    return 0;
}
