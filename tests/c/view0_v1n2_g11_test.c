#include <arborcore/view0_conformance/native.h>
#include "g11.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static void need(bool condition, const char *message) {
    if (!condition) { (void)fprintf(stderr, "FAIL: %s\n", message); exit(1); }
}

static arbor_view0_native_v1n2_g11_evaluation evaluate_body(const char *body) {
    static const char prefix[] = "<!doctype html><html><head><title>x</title></head><body>";
    static const char suffix[] = "</body></html>";
    char input[32768];
    const int n = snprintf(input, sizeof(input), "%s%s%s", prefix, body, suffix);
    need(n > 0 && (size_t)n < sizeof(input), "fixture construction");
    arbor_view0_native_v1n2_g11_evaluation evaluation = {0};
    need(arbor_view0_native_v1n2_g11_measure(
        (arbor_span){(const uint8_t *)input, (uint64_t)n}, &evaluation).native == 0,
        "G11 measurement");
    return evaluation;
}

static void none(arbor_view0_native_v1n2_g11_evaluation e, const char *message) {
    need(e.diagnostic_count == 0u, message);
}

int main(void) {
    arbor_view0_native_v1n2_g11_evaluation e;
    e = evaluate_body("<details><summary>A</summary>x</details>");
    none(e, "T01 unnamed details");
    e = evaluate_body("<details name=x><summary>A</summary>x</details>");
    none(e, "T02 named closed details");
    e = evaluate_body("<details name=x open><summary>A</summary>x</details>");
    none(e, "T03 single open group member");
    e = evaluate_body("<details name><summary>A</summary>x</details>");
    need(e.rule_violation_count[0] == 1u, "T04 empty name");
    e = evaluate_body("<details name=x open></details><details name=x open></details>");
    need(e.rule_violation_count[0] == 1u, "T05 second open group member");
    e = evaluate_body("<details name=x open></details><details name=X open></details>");
    none(e, "T06 names compare case-sensitively");
    e = evaluate_body("<details name=x open></details><details name=y open></details>");
    none(e, "T07 distinct names");
    e = evaluate_body("<details name=x><details name=x></details></details>");
    need(e.rule_violation_count[0] == 1u, "T08 nested same group");
    e = evaluate_body("<details name=x><section><details name=x></details></section></details>");
    need(e.rule_violation_count[0] == 1u, "T09 deep nested same group");
    e = evaluate_body("<details name=x><details name=y></details></details>");
    none(e, "T10 nested distinct group");
    e = evaluate_body("<details name=x open><details name=x open></details></details>");
    need(e.rule_violation_count[0] == 2u, "T11 independent open and nesting violations");
    e = evaluate_body("<summary>orphan</summary>");
    none(e, "T12 summary ownership remains G03");
    e = evaluate_body("<dialog></dialog>");
    none(e, "T13 closed dialog");
    e = evaluate_body("<dialog open></dialog>");
    none(e, "T14 open dialog");
    e = evaluate_body("<dialog closedby=any></dialog>");
    none(e, "T15 closedby may be preset");
    e = evaluate_body("<dialog open closedby=any></dialog>");
    none(e, "T16 open light-dismiss dialog");
    e = evaluate_body("<dialog popover=manual></dialog>");
    none(e, "T17 dialog popover declaration");
    e = evaluate_body("<dialog open popover=manual></dialog>");
    none(e, "T18 dialog and popover authored states are not runtime showing states");
    e = evaluate_body("<dialog tabindex=0></dialog>");
    none(e, "T19 tabindex remains prior owner");
    e = evaluate_body("<dialog closedby=invalid></dialog>");
    none(e, "T20 closedby syntax remains G06");
    need(e.prior_owner_suppression_count == 1u, "T20 prior-owner accounting");
    need(e.rule_violation_count[1] == 0u, "R2 publishes no prior-owner or runtime duplicate");

    puts("PASS G11-R1");
    puts("PASS G11-R2");
    puts("VIEW0_V1N2_G11_FUNCTIONAL_CASES=T01_T20_EXECUTED");
    puts("VIEW0_V1N2_G11_RULES=2_OF_2");
    puts("PASS: VIEW0 V1N2 G11 exact static interactive-element semantics");
    return 0;
}
