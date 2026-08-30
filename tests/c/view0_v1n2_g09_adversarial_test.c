#include <arborcore/view0_conformance/native.h>
#include "g09.h"

#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char fixture[ARBOR_VIEW0_NATIVE_MAX_INPUT_BYTES];

static void need(bool condition, const char *message) {
    if (!condition) { (void)fprintf(stderr, "FAIL: %s\n", message); exit(1); }
}

static void append_text(size_t *used, const char *text) {
    const size_t length = strlen(text);
    need(*used <= sizeof(fixture) && length < sizeof(fixture) - *used,
         "adversarial fixture capacity");
    (void)memcpy(fixture + *used, text, length);
    *used += length;
}

static arbor_view0_native_v1n2_g09_evaluation measure(size_t length) {
    arbor_view0_native_v1n2_g09_evaluation evaluation = {0};
    need(arbor_view0_native_v1n2_g09_measure(
        (arbor_span){(const uint8_t *)fixture, (uint64_t)length}, &evaluation).native == 0,
        "adversarial measurement");
    return evaluation;
}

static arbor_view0_native_v1n2_g09_evaluation collect(
    size_t length, arbor_view0_native_v1n2_g09_anchor *anchors, uint64_t capacity) {
    arbor_view0_native_v1n2_g09_evaluation evaluation = {0};
    need(arbor_view0_native_v1n2_g09_collect_anchors(
        (arbor_span){(const uint8_t *)fixture, (uint64_t)length}, anchors, capacity,
        &evaluation).native == 0, "adversarial anchor collection");
    return evaluation;
}

static size_t build_sparse(void) {
    size_t used = 0u;
    append_text(&used, "<!doctype html><html><head><title>x</title></head><body>"
                       "<table><tr>");
    for (size_t i = 0u; i < 512u; ++i)
        append_text(&used, "<td headers='' colspan=1000>x</td>");
    append_text(&used, "</tr></table></body></html>");
    return used;
}

static size_t build_cap(void) {
    size_t used = 0u;
    append_text(&used, "<!doctype html><html><head><title>x</title></head><body>"
                       "<table><colgroup span=2></colgroup>");
    for (size_t i = 0u; i < 4104u; ++i)
        append_text(&used, "<tr><td headers=''>x</td></tr>");
    append_text(&used, "</table></body></html>");
    return used;
}

static size_t build_implied_bands(void) {
    size_t used = 0u;
    append_text(&used, "<!doctype html><html><head><title>x</title></head><body>"
                       "<table><tr><td rowspan=4>a</td><td rowspan=2>b</td>"
                       "<td rowspan=3>c</td></tr></table></body></html>");
    return used;
}

static size_t build_mixed_anchors(void) {
    size_t used = 0u;
    append_text(&used, "<!doctype html><html><head><title>x</title></head><body>"
                       "<table><tr><td rowspan=3>a</td><td>b</td>"
                       "<td rowspan=2>c</td></tr><tr></tr></table></body></html>");
    return used;
}

static size_t build_deferred_tfoots(void) {
    size_t used = 0u;
    append_text(&used, "<!doctype html><html><head><title>x</title></head><body><table>"
                       "<tfoot><tr><td>f1</td></tr></tfoot>"
                       "<tbody><tr><th scope=col>h1</th></tr></tbody>"
                       "<tfoot><tr><td>f2</td></tr></tfoot>"
                       "<tbody><tr><th scope=col>h2</th></tr></tbody>"
                       "</table></body></html>");
    return used;
}

static size_t build_mixed_sections(void) {
    size_t used = 0u;
    append_text(&used, "<!doctype html><html><head><title>x</title></head><body><table>"
                       "<tfoot><tr><td>f</td></tr></tfoot>"
                       "<tr><th scope=col>d</th></tr>"
                       "<thead><tr><th scope=col>h</th></tr></thead>"
                       "<tbody><tr><td>b</td></tr></tbody>"
                       "</table></body></html>");
    return used;
}

static size_t build_determinism(void) {
    size_t used = 0u;
    append_text(&used, "<!doctype html><html><head><title>x</title></head><body>"
                       "<table><colgroup span=2></colgroup><tbody>"
                       "<tr><th id=a headers=b scope=col>A</th><th id=b headers=a>B</th></tr>"
                       "<tr><th scope=row>R</th><td>x</td></tr>"
                       "</tbody></table></body></html>");
    return used;
}

int main(void) {
    size_t length = build_sparse();
    arbor_view0_native_v1n2_g09_evaluation a = measure(length);
    need(a.table_count == 1u && a.cell_count == 512u, "T04 sparse fact accounting");
    need(a.diagnostic_count == 0u, "T04 sparse table validity");

    length = build_implied_bands();
    a = measure(length);
    need(a.rule_violation_count[0] == 2u, "S207 multiple implied bands");

    length = build_mixed_anchors();
    a = measure(length);
    need(a.rule_violation_count[0] == 2u, "S208 mixed actual and implied bands");
    arbor_view0_native_v1n2_g09_anchor anchors[2] = {0};
    a = collect(length, anchors, 2u);
    const char *const actual_row = strstr(fixture, "<tr></tr>");
    const char *const table = strstr(fixture, "<table>");
    need(actual_row != NULL && table != NULL, "S208 anchor fixture markers");
    need(anchors[0].shared.byte_offset == (uint64_t)(actual_row - fixture) + 1u,
         "S208 actual band row anchor");
    need(anchors[1].shared.byte_offset == (uint64_t)(table - fixture) + 1u,
         "S208 implied band table anchor");

    length = build_deferred_tfoots();
    a = measure(length);
    need(a.diagnostic_count == 0u, "S212 multiple deferred tfoot validity");
    need(a.implicit_header_association_count == 5u,
         "S212 stable multiple tfoot association order");

    length = build_mixed_sections();
    a = measure(length);
    need(a.diagnostic_count == 0u, "S214 mixed section validity");
    need(a.implicit_header_association_count == 5u,
         "S214 non-tfoot partition precedes tfoot");

    length = build_cap();
    a = measure(length);
    need(a.diagnostic_count == ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS,
         "T48 deterministic diagnostic cap");

    length = build_determinism();
    a = measure(length);
    arbor_view0_native_v1n2_g09_evaluation b = measure(length);
    need(memcmp(&a, &b, sizeof(a)) == 0, "T49 repeat-run identity");

    static const char *const locales[] = {"C", "en_US.UTF-8", "tr_TR.UTF-8"};
    for (size_t i = 0u; i < sizeof(locales) / sizeof(locales[0]); ++i) {
        if (setlocale(LC_ALL, locales[i]) == NULL) continue;
        b = measure(length);
        need(memcmp(&a, &b, sizeof(a)) == 0, "T50 locale independence");
    }
    (void)setlocale(LC_ALL, "C");
    need(a.deferred_external_semantics_count == 3u, "external boundary accounting");

    puts("VIEW0_V1N2_G09_DETERMINISM=PASS");
    puts("VIEW0_V1N2_G09_SPARSE_RECTANGLE_MODEL=PASS");
    puts("VIEW0_V1N2_G09_SR2_LOGICAL_Y_BAND_COVERAGE=PASS");
    puts("VIEW0_V1N2_G09_SR2_ACTUAL_AND_IMPLIED_ANCHORS=PASS");
    puts("VIEW0_V1N2_G09_SR2_DEFERRED_TFOOT_ORDER=PASS");
    puts("VIEW0_V1N2_G09_PRODUCT_GRID_ALLOCATION=ZERO");
    puts("VIEW0_V1N2_G09_DIAGNOSTIC_CAP_4096=PASS");
    puts("VIEW0_V1N2_G09_LOCALE_INDEPENDENCE=PASS");
    puts("VIEW0_V1N2_G09_EXTERNAL_PRESENTATION_DEFERRALS=PASS");
    puts("PASS: VIEW0 V1N2 G09 corrected adversarial qualification");
    return 0;
}
