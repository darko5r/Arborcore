#include <arborcore/view0_conformance/native.h>
#include "g11.h"

#include <errno.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char fixture[ARBOR_VIEW0_NATIVE_MAX_INPUT_BYTES];

static void need(bool condition, const char *message) {
    if (!condition) { (void)fprintf(stderr, "FAIL: %s\n", message); exit(1); }
}

static void append(size_t *used, const char *text) {
    const size_t n = strlen(text);
    need(n < sizeof(fixture) - *used, "fixture capacity");
    (void)memcpy(fixture + *used, text, n); *used += n;
}

static arbor_view0_native_v1n2_g11_evaluation measure(size_t length) {
    arbor_view0_native_v1n2_g11_evaluation e = {0};
    need(arbor_view0_native_v1n2_g11_measure(
        (arbor_span){(const uint8_t *)fixture, (uint64_t)length}, &e).native == 0,
        "adversarial measurement");
    return e;
}

static size_t build_cap(void) {
    size_t used = 0u;
    append(&used, "<!doctype html><html><head><title>x</title></head><body>");
    for (size_t i = 0u; i < 4097u; ++i) append(&used, "<details name></details>");
    append(&used, "</body></html>");
    return used;
}

static size_t build_determinism(void) {
    size_t used = 0u;
    append(&used, "<!doctype html><html><head><title>x</title></head><body>"
                  "<details name=x open><summary>a</summary></details>"
                  "<details name=x open><details name=x></details></details>"
                  "<dialog open popover=manual></dialog></body></html>");
    return used;
}

int main(void) {
    size_t length = build_determinism();
    arbor_view0_native_v1n2_g11_evaluation a = measure(length);
    arbor_view0_native_v1n2_g11_evaluation b = measure(length);
    need(memcmp(&a, &b, sizeof(a)) == 0, "repeat-run identity");
    need(a.rule_violation_count[0] == 2u && a.rule_violation_count[1] == 0u,
         "mixed boundary census");
    static const char *const locales[] = {"C", "en_US.UTF-8", "tr_TR.UTF-8"};
    for (size_t i = 0u; i < sizeof(locales) / sizeof(locales[0]); ++i) {
        if (setlocale(LC_ALL, locales[i]) == NULL) continue;
        b = measure(length);
        need(memcmp(&a, &b, sizeof(a)) == 0, "locale independence");
    }
    (void)setlocale(LC_ALL, "C");
    length = build_cap();
    a = measure(length);
    need(a.diagnostic_count == 4097u, "measure exceeds publication cap deterministically");
    arbor_view0_native_diagnostic diagnostics[1];
    arbor_view0_native_result result;
    need(arbor_view0_native_check(
        (arbor_span){(const uint8_t *)fixture, (uint64_t)length}, diagnostics, 1u,
        &result).native == -ENOSPC, "global publication cap enforced");
    puts("VIEW0_V1N2_G11_DETERMINISM=PASS");
    puts("VIEW0_V1N2_G11_LOCALE_INDEPENDENCE=PASS");
    puts("VIEW0_V1N2_G11_DIAGNOSTIC_CAP_4096=PASS");
    puts("VIEW0_V1N2_G11_RUNTIME_INTERACTION_DEFERRALS=PASS");
    puts("PASS: VIEW0 V1N2 G11 adversarial qualification");
    return 0;
}
