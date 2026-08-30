#include <arborcore/view0_conformance/native.h>
#include "g08.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static void need(bool condition, const char *message) {
    if (!condition) { (void)fprintf(stderr, "FAIL: %s\n", message); exit(1); }
}

static uint64_t count_rule(const char *body, uint64_t rule) {
    static const char prefix[] = "<!doctype html><html><head><title>x</title></head><body>";
    static const char suffix[] = "</body></html>";
    char input[8192];
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
        {"<picture><source><img src=/x alt=x></picture>", ARBOR_VIEW_V1_G08_RESPONSIVE_IMAGE_SOURCE_SET, "R1"},
        {"<img alt=x>", ARBOR_VIEW_V1_G08_IMAGE_RESOURCE_DECLARATION, "R2"},
        {"<img src=/x>", ARBOR_VIEW_V1_G08_IMAGE_TEXT_ALTERNATIVES, "R3"},
        {"<iframe sandbox='allow-top-navigation allow-top-navigation-by-user-activation'></iframe>", ARBOR_VIEW_V1_G08_IFRAME_AUTHORING, "R4"},
        {"<embed itemprop=image>", ARBOR_VIEW_V1_G08_EMBED_AUTHORING, "R5"},
        {"<object></object>", ARBOR_VIEW_V1_G08_OBJECT_AUTHORING, "R6"},
        {"<audio itemprop=audio></audio>", ARBOR_VIEW_V1_G08_MEDIA_ELEMENT_DECLARATION, "R7"},
        {"<video><track kind=metadata></video>", ARBOR_VIEW_V1_G08_TEXT_TRACK_AUTHORING, "R8"},
        {"<audio><source type=audio/ogg></audio>", ARBOR_VIEW_V1_G08_MEDIA_CROSS_RESOURCE_SEMANTICS, "R9"},
        {"<map></map>", ARBOR_VIEW_V1_G08_IMAGE_MAP_AUTHORING, "R10"},
        {"<img src=/x alt=x width=0 height=1>", ARBOR_VIEW_V1_G08_DIMENSION_ATTRIBUTE_SEMANTICS, "R12"}
    };
    for (size_t i = 0u; i < sizeof(negatives) / sizeof(negatives[0]); ++i) {
        need(count_rule(negatives[i].fixture, negatives[i].rule) == 1u, negatives[i].name);
        (void)printf("PASS G08-%s\n", negatives[i].name);
    }

    static const char foreign[] =
        "<!doctype html><html><head><title>x</title></head><body>"
        "<svg><foreignObject><p>x</p></foreignObject></svg><math><mi>x</mi></math>"
        "</body></html>";
    arbor_view0_native_v1n2_g08_evaluation evaluation = {0};
    need(arbor_view0_native_v1n2_g08_measure(
        (arbor_span){(const uint8_t *)foreign, sizeof(foreign) - 1u}, &evaluation).native == 0,
        "foreign integration measurement");
    need(evaluation.foreign_integration_count >= 2u, "HTML foreign integration observed");
    need(evaluation.rule_violation_count[10] == 0u, "full SVG/MathML language deferred");
    puts("PASS G08-R11 HTML_INTEGRATION_ONLY");

    need(count_rule("<picture><source srcset='x.png 1x'><img src=x.png alt=x></picture>",
         ARBOR_VIEW_V1_G08_RESPONSIVE_IMAGE_SOURCE_SET) == 0u, "valid responsive image");
    need(count_rule("<figure><img src=/x><figcaption>caption</figcaption></figure>",
         ARBOR_VIEW_V1_G08_IMAGE_TEXT_ALTERNATIVES) == 0u, "figcaption alt omission");
    need(count_rule("<video><track src=/x.vtt kind=subtitles srclang=en label=English default></video>",
         ARBOR_VIEW_V1_G08_TEXT_TRACK_AUTHORING) == 0u, "valid text track");
    need(count_rule("<map name=m><area href=/x alt=x shape=rect coords='0,0,10,10'></map><img src=/x alt=x usemap=#m>",
         ARBOR_VIEW_V1_G08_IMAGE_MAP_AUTHORING) == 0u, "valid image map");
    need(count_rule("<picture><source srcset=a.png><img srcset=b.png alt=x></picture>",
         ARBOR_VIEW_V1_G08_RESPONSIVE_IMAGE_SOURCE_SET) == 1u,
         "responsive source companion needs media or type");
    need(count_rule("<picture><source srcset=a.png type=image/png><img srcset=b.png alt=x></picture>",
         ARBOR_VIEW_V1_G08_RESPONSIVE_IMAGE_SOURCE_SET) == 0u,
         "responsive source companion type qualifier");
    need(count_rule("<iframe sandbox='allow-popups allow-top-navigation-to-custom-protocols'></iframe>",
         ARBOR_VIEW_V1_G08_IFRAME_AUTHORING) == 1u,
         "sandbox popup custom-protocol redundancy");
    need(count_rule("<div width=0 height=1>x</div>",
         ARBOR_VIEW_V1_G08_DIMENSION_ATTRIBUTE_SEMANTICS) == 0u,
         "dimension rule element applicability");
    need(count_rule("<img src=/x title=' '>",
         ARBOR_VIEW_V1_G08_IMAGE_TEXT_ALTERNATIVES) == 0u,
         "nonempty title is exact not trimmed");
    need(count_rule("<video><track src=a.vtt kind=metadata label=English>"
                    "<track src=b.vtt kind=metadata label=english></video>",
         ARBOR_VIEW_V1_G08_TEXT_TRACK_AUTHORING) == 0u,
         "track label equality is case-sensitive");
    need(count_rule("<video><track src=a.vtt kind=metadata label=' '></video>",
         ARBOR_VIEW_V1_G08_TEXT_TRACK_AUTHORING) == 0u,
         "track label empty-string rule is exact");
    need(count_rule("<map name=m><area alt=x coords='0,0,1,1'></map>",
         ARBOR_VIEW_V1_G08_IMAGE_MAP_AUTHORING) == 1u,
         "area alt omitted without href");
    need(count_rule("<map name=m><area href=x alt='' coords='0,0,1,1'>"
                    "<area href=x alt=x coords='0,0,1,1'></map>",
         ARBOR_VIEW_V1_G08_IMAGE_MAP_AUTHORING) == 0u,
         "blank area alt has same-resource replacement");
    need(count_rule("<map name=m><area coords='0,0,1'></map>",
         ARBOR_VIEW_V1_G08_IMAGE_MAP_AUTHORING) == 1u,
         "missing area shape defaults to rect");
    need(count_rule("<map name=m><area shape=rect coords='10,10,1,1'></map>",
         ARBOR_VIEW_V1_G08_IMAGE_MAP_AUTHORING) == 1u,
         "rectangle coordinate ordering");
    need(count_rule("<map name='a b'></map>",
         ARBOR_VIEW_V1_G08_IMAGE_MAP_AUTHORING) == 1u,
         "map name ASCII whitespace");
    need(count_rule("<map name=a id=b></map>",
         ARBOR_VIEW_V1_G08_IMAGE_MAP_AUTHORING) == 1u,
         "map name id equality");
    need(count_rule("<map name=A></map><map name=a></map>",
         ARBOR_VIEW_V1_G08_IMAGE_MAP_AUTHORING) == 0u,
         "map name equality is case-sensitive");
    static const struct {
        const char *fixture;
        uint64_t rule;
        uint64_t expected;
        const char *name;
    } sr2_cases[] = {
        {"<picture><source srcset='a.png 1x' media='?x'><img src=a.png alt=x></picture>",
         ARBOR_VIEW_V1_G08_RESPONSIVE_IMAGE_SOURCE_SET, 1u, "media query invalid"},
        {"<picture><source srcset='a.png 1x 2x'><img src=a.png alt=x></picture>",
         ARBOR_VIEW_V1_G08_RESPONSIVE_IMAGE_SOURCE_SET, 1u, "srcset extra descriptor"},
        {"<picture><source srcset='a.png 1x, b.png 1.0x'><img src=a.png alt=x></picture>",
         ARBOR_VIEW_V1_G08_RESPONSIVE_IMAGE_SOURCE_SET, 1u, "srcset duplicate density"},
        {"<picture><source srcset='a.png 400w'><img src=a.png alt=x></picture>",
         ARBOR_VIEW_V1_G08_RESPONSIVE_IMAGE_SOURCE_SET, 1u, "source width descriptor needs sizes"},
        {"<picture><source srcset='a.png 400w' sizes='(max-width: 600px) 400px, 800px' media='screen and (min-width: 1px)'><img src=a.png alt=x></picture>",
         ARBOR_VIEW_V1_G08_RESPONSIVE_IMAGE_SOURCE_SET, 0u, "responsive grammar valid"},
        {"<img srcset='a.png 1x 2x' alt=x>",
         ARBOR_VIEW_V1_G08_IMAGE_RESOURCE_DECLARATION, 1u, "img srcset invalid"},
        {"<img srcset='a.png 400w' alt=x>",
         ARBOR_VIEW_V1_G08_IMAGE_RESOURCE_DECLARATION, 1u, "img width descriptor needs sizes"},
        {"<img srcset='a.png 400w' sizes='100vw,,50vw' alt=x>",
         ARBOR_VIEW_V1_G08_IMAGE_RESOURCE_DECLARATION, 1u, "sizes empty component"},
        {"<img src=a.png alt=x loading=soon>",
         ARBOR_VIEW_V1_G08_IMAGE_RESOURCE_DECLARATION, 1u, "img loading invalid"},
        {"<img src=a.png alt=x decoding=maybe>",
         ARBOR_VIEW_V1_G08_IMAGE_RESOURCE_DECLARATION, 1u, "img decoding invalid"},
        {"<img src=a.png alt=x referrerpolicy=private>",
         ARBOR_VIEW_V1_G08_IMAGE_RESOURCE_DECLARATION, 1u, "img referrer policy invalid"},
        {"<img srcset='a.png 400w' sizes=auto loading=lazy alt=x>",
         ARBOR_VIEW_V1_G08_IMAGE_RESOURCE_DECLARATION, 0u, "auto sizes lazy valid"},
        {"<img srcset='a.png 400w' sizes=auto loading=eager alt=x>",
         ARBOR_VIEW_V1_G08_IMAGE_RESOURCE_DECLARATION, 1u, "auto sizes eager invalid"},
        {"<iframe itemprop=x></iframe>",
         ARBOR_VIEW_V1_G08_IFRAME_AUTHORING, 1u, "iframe itemprop requires src"},
        {"<iframe sandbox='allow-scripts unknown'></iframe>",
         ARBOR_VIEW_V1_G08_IFRAME_AUTHORING, 1u, "sandbox unknown token"},
        {"<iframe sandbox='allow-scripts ALLOW-SCRIPTS'></iframe>",
         ARBOR_VIEW_V1_G08_IFRAME_AUTHORING, 1u, "sandbox duplicate token"},
        {"<iframe allow='geolocation, camera'></iframe>",
         ARBOR_VIEW_V1_G08_IFRAME_AUTHORING, 1u, "permissions policy comma invalid"},
        {"<iframe allow=\"geolocation 'self'; camera *\"></iframe>",
         ARBOR_VIEW_V1_G08_IFRAME_AUTHORING, 0u, "permissions policy valid"},
        {"<iframe allowfullscreen=true></iframe>",
         ARBOR_VIEW_V1_G08_IFRAME_AUTHORING, 1u, "allowfullscreen lexical invalid"},
        {"<iframe loading=soon></iframe>",
         ARBOR_VIEW_V1_G08_IFRAME_AUTHORING, 1u, "iframe loading invalid"},
        {"<iframe referrerpolicy=private></iframe>",
         ARBOR_VIEW_V1_G08_IFRAME_AUTHORING, 1u, "iframe referrer policy invalid"},
        {"<video preload=everything></video>",
         ARBOR_VIEW_V1_G08_MEDIA_ELEMENT_DECLARATION, 1u, "media preload invalid"},
        {"<video loading=soon></video>",
         ARBOR_VIEW_V1_G08_MEDIA_ELEMENT_DECLARATION, 1u, "media loading invalid"},
        {"<video crossorigin=credentialed></video>",
         ARBOR_VIEW_V1_G08_MEDIA_ELEMENT_DECLARATION, 1u, "media crossorigin invalid"},
        {"<video controls=true></video>",
         ARBOR_VIEW_V1_G08_MEDIA_ELEMENT_DECLARATION, 1u, "media boolean invalid"},
        {"<video autoplay loop muted controls playsinline preload=metadata loading=lazy crossorigin=anonymous></video>",
         ARBOR_VIEW_V1_G08_MEDIA_ELEMENT_DECLARATION, 0u, "media declaration grammar valid"},
        {"<map name=m><area itemprop=x shape=default></map>",
         ARBOR_VIEW_V1_G08_IMAGE_MAP_AUTHORING, 1u, "area itemprop requires href"},
        {"<img src=a.png alt=x width=1px>",
         ARBOR_VIEW_V1_G08_DIMENSION_ATTRIBUTE_SEMANTICS, 1u, "dimension lexical invalid"},
        {"<img src=a.png alt=x width=' 1'>",
         ARBOR_VIEW_V1_G08_DIMENSION_ATTRIBUTE_SEMANTICS, 1u, "dimension whitespace invalid"},
        {"<iframe width=-1></iframe>",
         ARBOR_VIEW_V1_G08_DIMENSION_ATTRIBUTE_SEMANTICS, 1u, "iframe dimension negative"},
        {"<picture><source srcset='a.png 1x' width=auto><img src=a.png alt=x></picture>",
         ARBOR_VIEW_V1_G08_DIMENSION_ATTRIBUTE_SEMANTICS, 1u, "source dimension lexical invalid"}
    };
    for (size_t i = 0u; i < sizeof(sr2_cases) / sizeof(sr2_cases[0]); ++i)
        need(count_rule(sr2_cases[i].fixture, sr2_cases[i].rule) == sr2_cases[i].expected,
             sr2_cases[i].name);

    static const struct {
        const char *fixture;
        uint64_t rule;
        uint64_t expected;
        const char *name;
    } sr3_cases[] = {
        {"<img src=a alt=x loading=lazy sizes=auto>",
         ARBOR_VIEW_V1_G08_IMAGE_RESOURCE_DECLARATION, 0u, "SR3 T01 auto sizes"},
        {"<img src=a alt=x loading=LAZY sizes='AUTO, 100vw'>",
         ARBOR_VIEW_V1_G08_IMAGE_RESOURCE_DECLARATION, 0u, "SR3 T02 auto fallback"},
        {"<img src=a alt=x loading=lazy sizes=100vw>",
         ARBOR_VIEW_V1_G08_IMAGE_RESOURCE_DECLARATION, 1u, "SR3 T03 non-auto without srcset"},
        {"<img src=a alt=x loading=eager sizes=auto>",
         ARBOR_VIEW_V1_G08_IMAGE_RESOURCE_DECLARATION, 1u, "SR3 T04 eager auto"},
        {"<picture><source srcset='a.png 400w'><img src=a alt=x loading=lazy sizes=auto></picture>",
         ARBOR_VIEW_V1_G08_RESPONSIVE_IMAGE_SOURCE_SET, 0u, "SR3 T05 deferred source auto"},
        {"<picture><source srcset='a.png 400w'><img src=a alt=x></picture>",
         ARBOR_VIEW_V1_G08_RESPONSIVE_IMAGE_SOURCE_SET, 1u, "SR3 T06 unresolved source auto"},
        {"<picture><source srcset='a.png 400w' sizes=100vw><img src=a alt=x></picture>",
         ARBOR_VIEW_V1_G08_RESPONSIVE_IMAGE_SOURCE_SET, 0u, "SR3 T07 explicit source sizes"},
        {"<iframe allow='geolocation https://example.test'></iframe>",
         ARBOR_VIEW_V1_G08_IFRAME_AUTHORING, 0u, "SR3 T08 unquoted origin"},
        {"<iframe allow=\"camera *; geolocation 'self'\"></iframe>",
         ARBOR_VIEW_V1_G08_IFRAME_AUTHORING, 0u, "SR3 T09 policy keyword"},
        {"<iframe allow=fullscreen></iframe>",
         ARBOR_VIEW_V1_G08_IFRAME_AUTHORING, 0u, "SR3 T10 empty iframe allowlist"},
        {"<iframe allow='geolocation \"https://example.test\"'></iframe>",
         ARBOR_VIEW_V1_G08_IFRAME_AUTHORING, 1u, "SR3 T11 quoted origin"},
        {"<iframe allow='geolocation https://example.test:'></iframe>",
         ARBOR_VIEW_V1_G08_IFRAME_AUTHORING, 1u, "SR3 T12 malformed origin"},
        {"<iframe allow=\"camera *;; geolocation 'self'\"></iframe>",
         ARBOR_VIEW_V1_G08_IFRAME_AUTHORING, 1u, "SR3 T13 empty directive"},
        {"<audio><source src=/x media=''></audio>",
         ARBOR_VIEW_V1_G08_MEDIA_CROSS_RESOURCE_SEMANTICS, 0u, "SR3 T14 empty media list"},
        {"<audio><source src=/x media='screen and (color)'></audio>",
         ARBOR_VIEW_V1_G08_MEDIA_CROSS_RESOURCE_SEMANTICS, 0u, "SR3 T15 media type condition"},
        {"<audio><source src=/x media='not screen and (width >= 10px)'></audio>",
         ARBOR_VIEW_V1_G08_MEDIA_CROSS_RESOURCE_SEMANTICS, 0u, "SR3 T16 not media range"},
        {"<audio><source src=/x media='(width < 100px) or (orientation: landscape)'></audio>",
         ARBOR_VIEW_V1_G08_MEDIA_CROSS_RESOURCE_SEMANTICS, 0u, "SR3 T17 media or"},
        {"<audio><source src=/x media='(10px < width <= 100px)'></audio>",
         ARBOR_VIEW_V1_G08_MEDIA_CROSS_RESOURCE_SEMANTICS, 0u, "SR3 T18 two-sided range"},
        {"<audio><source src=/x media='screen garbage'></audio>",
         ARBOR_VIEW_V1_G08_MEDIA_CROSS_RESOURCE_SEMANTICS, 1u, "SR3 T19 trailing media token"},
        {"<audio><source src=/x media='screen and'></audio>",
         ARBOR_VIEW_V1_G08_MEDIA_CROSS_RESOURCE_SEMANTICS, 1u, "SR3 T20 missing condition"},
        {"<audio><source src=/x media='(color) and (width > 1px) or (height > 1px)'></audio>",
         ARBOR_VIEW_V1_G08_MEDIA_CROSS_RESOURCE_SEMANTICS, 1u, "SR3 T21 mixed connectors"},
        {"<audio><source src=/x media=only></audio>",
         ARBOR_VIEW_V1_G08_MEDIA_CROSS_RESOURCE_SEMANTICS, 1u, "SR3 T22 reserved media type"},
        {"<audio><source src=/x media='unknown-fn(foo)'></audio>",
         ARBOR_VIEW_V1_G08_MEDIA_CROSS_RESOURCE_SEMANTICS, 0u, "SR3 T23 general enclosed"},
        {"<audio><source src=/x media='url(\"unterminated)'></audio>",
         ARBOR_VIEW_V1_G08_MEDIA_CROSS_RESOURCE_SEMANTICS, 1u, "SR3 T24 bad string token"}
    };
    for (size_t i = 0u; i < sizeof(sr3_cases) / sizeof(sr3_cases[0]); ++i)
        need(count_rule(sr3_cases[i].fixture, sr3_cases[i].rule) == sr3_cases[i].expected,
             sr3_cases[i].name);
    need(count_rule("<img src=a alt=x loading=lazy sizes='auto,,100vw'>",
         ARBOR_VIEW_V1_G08_IMAGE_RESOURCE_DECLARATION) == 1u,
         "SR3 malformed auto fallback");
    need(count_rule("<audio><source src=/x media='   '></audio>",
         ARBOR_VIEW_V1_G08_MEDIA_CROSS_RESOURCE_SEMANTICS) == 0u,
         "SR3 whitespace empty media list");
    puts("VIEW0_V1N2_G08_SR1_AUTHORITY_EDGE_CASES=14_OF_14");
    puts("VIEW0_V1N2_G08_SR2_STATIC_CLAUSE_CASES=31_OF_31");
    puts("VIEW0_V1N2_G08_SR3_PARSER_AND_AUTO_SIZES_CASES=24_OF_24");
    puts("VIEW0_V1N2_G08_RULE_IDENTITIES=12_OF_12");
    puts("VIEW0_V1N2_G08_DIAGNOSTIC_RULES=11_STATIC_PLUS_R11_HTML_INTEGRATION_ONLY");
    puts("PASS: VIEW0 V1N2 G08 exact admitted embedded-content boundary");
    return 0;
}
