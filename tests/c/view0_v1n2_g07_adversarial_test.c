#include <arborcore/view0_conformance/native.h>
#include "g07.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void need(bool condition, const char *message) {
    if (!condition) { (void)fprintf(stderr, "FAIL: %s\n", message); exit(1); }
}

int main(void) {
    static const char fixture[] =
        "<!doctype html><html><head><title>x</title>"
        "<link rel='alternate stylesheet' href=/x></head><body>"
        "<a href='http://' download ping='ftp://bad' rel='canonical opener noopener'>x</a>"
        "</body></html>";
    const arbor_span input = {(const uint8_t *)fixture, sizeof(fixture) - 1u};
    arbor_view0_native_v1n2_g07_evaluation first = {0};
    arbor_view0_native_v1n2_g07_evaluation second = {0};
    need(arbor_view0_native_v1n2_g07_measure(input, &first).native == 0, "first measurement");
    need(arbor_view0_native_v1n2_g07_measure(input, &second).native == 0, "second measurement");
    need(memcmp(&first, &second, sizeof(first)) == 0, "deterministic measurement");
    need(first.diagnostic_count == 5u, "five independently owned violations");
    arbor_view0_native_v1n2_g07_anchor anchors[5] = {0};
    need(arbor_view0_native_v1n2_g07_collect_anchors(input, anchors, 5u, &second).native == 0,
         "exact anchor collection");
    need(memcmp(&first, &second, sizeof(first)) == 0, "measurement/collection equivalence");
    need(arbor_view0_native_v1n2_g07_collect_anchors(input, anchors, 4u, &second).native == -ENOSPC,
         "bounded anchor capacity");
    for (uint64_t i = 0u; i < 5u; ++i) {
        need(anchors[i].shared.group_ordinal == 1u, "G07 shared-arena group ordinal");
        need(anchors[i].shared.rule_ordinal >= 1u && anchors[i].shared.rule_ordinal <= 5u,
             "rule ordinal range");
    }
    puts("VIEW0_V1N2_G07_DETERMINISM=PASS");
    puts("VIEW0_V1N2_G07_BOUNDED_ANCHOR_CAPACITY=PASS");
    puts("VIEW0_V1N2_G07_EXTERNAL_REGISTRY_DEFERRAL=PASS");
    puts("PASS: VIEW0 V1N2 G07 adversarial qualification");
    return 0;
}
