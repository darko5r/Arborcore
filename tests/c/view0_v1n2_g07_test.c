#include <arborcore/view0_conformance/native.h>

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void need(bool condition, const char *message) {
    if (!condition) { (void)fprintf(stderr, "FAIL: %s\n", message); exit(1); }
}

static uint64_t count_rule(const char *body, uint64_t rule) {
    static const char prefix[] = "<!doctype html><html><head><title>x</title></head><body>";
    static const char suffix[] = "</body></html>";
    char input[4096];
    arbor_view0_native_diagnostic diagnostics[128];
    arbor_view0_native_result result = {0};
    const int written = snprintf(input, sizeof(input), "%s%s%s", prefix, body, suffix);
    need(written > 0 && (size_t)written < sizeof(input), "fixture construction");
    const arbor_status status = arbor_view0_native_check(
        (arbor_span){(const uint8_t *)input, (uint64_t)written}, diagnostics, 128u, &result);
    if (status.native != 0) {
        (void)fprintf(stderr, "FAIL: checker status=%" PRId64 " fixture=%s\n", status.native, body);
        exit(1);
    }
    uint64_t count = 0u;
    for (uint64_t i = 0u; i < result.diagnostic_count; ++i)
        if (diagnostics[i].rule_id == rule) ++count;
    return count;
}

int main(void) {
    static const struct { const char *fixture; uint64_t rule; const char *name; } negatives[] = {
        {"<a href='http://' target=_self>x</a>", ARBOR_VIEW_V1_G07_HYPERLINK_ELEMENT_SEMANTICS, "R1"},
        {"<a download=report.txt>x</a>", ARBOR_VIEW_V1_G07_DOWNLOAD_SEMANTICS, "R2"},
        {"<a href=/next ping='https://audit.test/p ftp://bad.test/p'>x</a>", ARBOR_VIEW_V1_G07_HYPERLINK_AUDITING, "R3"},
        {"<a href=/x rel=canonical>x</a>", ARBOR_VIEW_V1_G07_LINK_TYPE_APPLICABILITY, "R4"},
        {"<link rel='alternate stylesheet' href=/theme.css>", ARBOR_VIEW_V1_G07_LINK_RELATION_SEMANTICS, "R5"}
    };
    for (size_t i = 0u; i < sizeof(negatives) / sizeof(negatives[0]); ++i) {
        need(count_rule(negatives[i].fixture, negatives[i].rule) == 1u, negatives[i].name);
        (void)printf("PASS G07-%s\n", negatives[i].name);
    }
    need(count_rule("<a href=/x target=_blank download>ok</a>", ARBOR_VIEW_V1_G07_HYPERLINK_ELEMENT_SEMANTICS) == 0u,
         "valid hyperlink declaration");
    need(count_rule("<a href=/x ping='https://audit.test/p http://audit.test/q'>ok</a>", ARBOR_VIEW_V1_G07_HYPERLINK_AUDITING) == 0u,
         "valid ping list");
    need(count_rule("<link rel=canonical href=/x>", ARBOR_VIEW_V1_G07_LINK_TYPE_APPLICABILITY) == 0u,
         "canonical link applicability");
    need(count_rule("<form rel='help external'></form>", ARBOR_VIEW_V1_G07_LINK_TYPE_APPLICABILITY) == 0u,
         "form hyperlink relations");
    need(count_rule("<link rel='alternate stylesheet' href=/x title='High contrast'>", ARBOR_VIEW_V1_G07_LINK_RELATION_SEMANTICS) == 0u,
         "alternate stylesheet title companion");
    need(count_rule("<link rel='shortcut icon' href=/favicon.ico>", ARBOR_VIEW_V1_G07_LINK_RELATION_SEMANTICS) == 0u,
         "exact historical shortcut icon spelling");
    need(count_rule("<link rel='shortcut  icon' href=/favicon.ico>", ARBOR_VIEW_V1_G07_LINK_RELATION_SEMANTICS) == 1u,
         "shortcut icon requires exactly one space");
    need(count_rule("<a href=/x rel='future-registry-token'>x</a>", ARBOR_VIEW_V1_G07_LINK_TYPE_APPLICABILITY) == 0u,
         "unfrozen extension registry remains non-rejecting");
    puts("VIEW0_V1N2_G07_RULES=5_OF_5");
    puts("PASS: VIEW0 V1N2 G07 exact static link-semantics rules");
    return 0;
}
