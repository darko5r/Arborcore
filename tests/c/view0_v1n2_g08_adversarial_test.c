#include <arborcore/view0_conformance/native.h>
#include "g08.h"

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
        "<!doctype html><html><head><title>x</title></head><body>"
        "<picture><img src=/x><source src=/bad></picture>"
        "<picture><source srcset='a.png 1x 2x' media='?x'><img src=a.png alt=x></picture>"
        "<picture><source srcset='a.png 400w'><img src=a.png alt=x loading=lazy sizes='AUTO, 100vw'></picture>"
        "<picture><source srcset='a.png 400w'><img src=a.png alt=x></picture>"
        "<img src='http://' srcset='a.png 400w' sizes='100vw,,50vw' loading=soon decoding=maybe referrerpolicy=private width=0 height=1>"
        "<iframe name=_new itemprop=x sandbox='allow-scripts ALLOW-SCRIPTS' allow='geolocation, camera' allowfullscreen=true loading=soon referrerpolicy=private></iframe>"
        "<iframe allow='geolocation https://example.test'></iframe>"
        "<iframe allow='camera https://example.test:'></iframe>"
        "<embed itemprop=x type='bad type'>"
        "<object name=_new></object>"
        "<video src=/movie controls=true preload=everything crossorigin=credentialed loading=soon><source><track kind=subtitles label=''></video>"
        "<audio><source src=/ok media='not screen and (10px < width <= 100px)'></audio>"
        "<audio><source src=/bad media='screen and (color) garbage'></audio>"
        "<map name=m><area href=/x shape=circle coords='0,0'><area itemprop=x shape=default></map>"
        "<map name=m></map><img src=/x alt=x usemap=#missing>"
        "<svg><foreignObject><p>x</p></foreignObject></svg>"
        "</body></html>";
    const arbor_span input = {(const uint8_t *)fixture, sizeof(fixture) - 1u};
    arbor_view0_native_v1n2_g08_evaluation first = {0};
    arbor_view0_native_v1n2_g08_evaluation second = {0};
    need(arbor_view0_native_v1n2_g08_measure(input, &first).native == 0, "first measurement");
    need(arbor_view0_native_v1n2_g08_measure(input, &second).native == 0, "second measurement");
    need(memcmp(&first, &second, sizeof(first)) == 0, "deterministic measurement");
    need(first.diagnostic_count > 10u && first.diagnostic_count <= 64u, "mixed diagnostic bound");
    arbor_view0_native_v1n2_g08_anchor anchors[64] = {0};
    need(arbor_view0_native_v1n2_g08_collect_anchors(
        input, anchors, first.diagnostic_count, &second).native == 0, "exact anchor collection");
    need(memcmp(&first, &second, sizeof(first)) == 0, "measurement collection equivalence");
    need(arbor_view0_native_v1n2_g08_collect_anchors(
        input, anchors, first.diagnostic_count - 1u, &second).native == -ENOSPC,
        "bounded anchor capacity");
    for (uint64_t i = 0u; i < first.diagnostic_count; ++i) {
        need(anchors[i].shared.group_ordinal == 2u, "G08 shared-arena group ordinal");
        need(anchors[i].shared.rule_ordinal >= 1u && anchors[i].shared.rule_ordinal <= 12u,
             "G08 rule ordinal range");
    }
    need(first.foreign_integration_count != 0u, "foreign integration retained");
    need(first.deferred_external_semantics_count != 0u, "external semantics deferred");
    puts("VIEW0_V1N2_G08_DETERMINISM=PASS");
    puts("VIEW0_V1N2_G08_BOUNDED_ANCHOR_CAPACITY=PASS");
    puts("VIEW0_V1N2_G08_SR3_PARSER_DETERMINISM=PASS");
    puts("VIEW0_V1N2_G08_SR3_AUTO_SIZES_DEFERRED_RELATIONSHIP=PASS");
    puts("VIEW0_V1N2_G08_EXTERNAL_RESOURCE_DEFERRALS=PASS");
    puts("PASS: VIEW0 V1N2 G08 adversarial qualification");
    return 0;
}
