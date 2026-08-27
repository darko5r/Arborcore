#include "g04_r1a.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define R1_RULE_ID UINT64_C(0x0000000030040001)

typedef struct r1_case {
    const char *name;
    const char *html;
    uint64_t diagnostics;
    uint64_t suppressions;
    uint64_t flags;
} r1_case;

static arbor_span span_from_cstr(const char *s)
{
    return (arbor_span){(const uint8_t *)s, (uint64_t)strlen(s)};
}

static int run_case(const r1_case *tc)
{
    arbor_view0_native_g04_r1a_evaluation measured = {0};
    arbor_status status = arbor_view0_native_g04_r1a_measure(
        span_from_cstr(tc->html), &measured);
    if (status.native != 0 ||
        measured.diagnostic_count != tc->diagnostics ||
        measured.prior_owner_suppression_count != tc->suppressions ||
        measured.deferred_flags != tc->flags) {
        (void)fprintf(
            stderr,
            "FAIL %s measure status=%lld diagnostics=%llu suppressions=%llu flags=0x%llx\n",
            tc->name,
            (long long)status.native,
            (unsigned long long)measured.diagnostic_count,
            (unsigned long long)measured.prior_owner_suppression_count,
            (unsigned long long)measured.deferred_flags);
        return 1;
    }

    arbor_view0_native_diagnostic diagnostics[8] = {{0}};
    arbor_view0_native_g04_r1a_evaluation collected = {0};
    status = arbor_view0_native_g04_r1a_collect(
        span_from_cstr(tc->html), diagnostics, tc->diagnostics, 19u, &collected);
    if (status.native != 0 || memcmp(&measured, &collected, sizeof(measured)) != 0) {
        (void)fprintf(stderr, "FAIL %s collect status=%lld\n",
                      tc->name, (long long)status.native);
        return 2;
    }

    for (uint64_t i = 0u; i < tc->diagnostics; ++i) {
        if (diagnostics[i].rule_id != R1_RULE_ID ||
            diagnostics[i].severity != (uint32_t)ARBOR_VIEW0_NATIVE_SEVERITY_ERROR ||
            diagnostics[i].origin != (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_AUTHORING ||
            strcmp(diagnostics[i].symbolic_name,
                   "ARBOR_VIEW_V1_G04_TRANSPARENT_PARENT_MODEL") != 0 ||
            diagnostics[i].source_length == 0u ||
            diagnostics[i].byte_offset == ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE ||
            diagnostics[i].discovery_sequence != 19u + i) {
            (void)fprintf(stderr, "FAIL %s diagnostic identity\n", tc->name);
            return 3;
        }
    }
    return 0;
}

static int exact_negative_anchor(void)
{
    static const char html[] =
        "<!doctype html><title>x</title><body><p><a href=\"/\"><div>x</div></a></p></body>";
    const char *tag = strstr(html, "<div>");
    if (tag == NULL) return 1;

    arbor_view0_native_g04_r1a_evaluation measured = {0};
    arbor_status status = arbor_view0_native_g04_r1a_measure(
        span_from_cstr(html), &measured);
    if (status.native != 0 || measured.diagnostic_count != 1u) return 2;

    arbor_view0_native_source_anchor anchor = {0};
    arbor_view0_native_g04_r1a_evaluation anchored = {0};
    status = arbor_view0_native_g04_r1a_collect_anchors(
        span_from_cstr(html), &anchor, 1u, &anchored);
    if (status.native != 0 || memcmp(&measured, &anchored, sizeof(measured)) != 0)
        return 3;
    if (anchor.byte_offset != (uint32_t)((tag - html) + 1) || anchor.source_length != 3u)
        return 4;

    arbor_view0_native_diagnostic diagnostic = {0};
    arbor_view0_native_g04_r1a_materialize_anchor(&anchor, 29u, &diagnostic);
    if (diagnostic.rule_id != R1_RULE_ID ||
        diagnostic.byte_offset != (uint64_t)((tag - html) + 1) ||
        diagnostic.source_length != 3u || diagnostic.discovery_sequence != 29u)
        return 5;
    return 0;
}

int main(void)
{
    const uint64_t custom =
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G04_R1_DEFERRED_G13_CUSTOM;

    static const r1_case cases[] = {
        {
            "matrix-positive",
            "<!doctype html><title>x</title><body><p><a href=\"/\"><em>x</em></a></p></body>",
            0u, 0u, 0u
        },
        {
            "matrix-negative",
            "<!doctype html><title>x</title><body><p><a href=\"/\"><div>x</div></a></p></body>",
            1u, 0u, 0u
        },
        {
            "whatwg-iterative-positive",
            "<!doctype html><title>x</title><body><p><object><ins><map><a href=\"/\">Apples</a></map></ins></object></p></body>",
            0u, 0u, 0u
        },
        {
            "nested-phrasing-prior-owner-suppressed",
            "<!doctype html><title>x</title><body><p><object><ins><map><div>x</div></map></ins></object></p></body>",
            0u, 1u, 0u
        },
        {
            "flow-context-transparent",
            "<!doctype html><title>x</title><body><ins><p>x</p></ins></body>",
            0u, 0u, 0u
        },
        {
            "phrasing-context-transparent",
            "<!doctype html><title>x</title><body><p><ins><span>x</span></ins></p></body>",
            0u, 0u, 0u
        },
        {
            "media-prefix-then-transparent-negative",
            "<!doctype html><title>x</title><body><p><video><source><div>x</div></video></p></body>",
            1u, 0u, 0u
        },
        {
            "dl-div-not-transparent",
            "<!doctype html><title>x</title><body><dl><div><dt>x</dt><dd>y</dd></div></dl></body>",
            0u, 0u, 0u
        },
        {
            "select-div-tail-valid",
            "<!doctype html><title>x</title><body><select><div><option>x</option></div></select></body>",
            0u, 0u, 0u
        },
        {
            "select-div-prior-owner-suppressed",
            "<!doctype html><title>x</title><body><select><div><span>x</span></div></select></body>",
            0u, 1u, 0u
        },
        {
            "option-branch-resolved-by-r1b",
            "<!doctype html><title>x</title><body><select><option><div><span>x</span></div></option></select></body>",
            0u, 0u, 0u
        },
        {
            "noscript-resolved-by-r1c",
            "<!doctype html><title>x</title><body><noscript><div>x</div></noscript></body>",
            0u, 0u, 0u
        },
        {
            "autonomous-custom-g13-deferred",
            "<!doctype html><title>x</title><body><x-r1><div>x</div></x-r1></body>",
            0u, 0u, custom
        }
    };

    for (size_t i = 0u; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        const int rc = run_case(&cases[i]);
        if (rc != 0) return (int)(10u + (i * 4u) + (size_t)rc);
    }

    const int anchor_rc = exact_negative_anchor();
    if (anchor_rc != 0) {
        (void)fprintf(stderr, "FAIL exact-negative-anchor rc=%d\n", anchor_rc);
        return 90 + anchor_rc;
    }

    (void)puts("VIEW0_V1N1_G04_R1A_FROZEN_MATRIX_FIXTURES=2_OF_2");
    (void)puts("VIEW0_V1N1_G04_R1A_ITERATIVE_TRANSPARENT_CHAIN=PASS");
    (void)puts("VIEW0_V1N1_G04_R1A_SOURCE_REPAIR_NEGATIVE_ANCHOR=PASS");
    (void)puts("VIEW0_V1N1_G04_R1A_PRIOR_OWNER_SUPPRESSION=PASS");
    (void)puts("VIEW0_V1N1_G04_R1A_RETAINED_UNDER_R1B=PASS");
    (void)puts("PASS: VIEW0 V1N1 G04 R1A retained core under R1C extension");
    return 0;
}
