#include <arborcore/view0_conformance/native.h>
#include "g04_r2a.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static arbor_span span_from_cstr(const char *s)
{
    return (arbor_span){(const uint8_t *)s, (uint64_t)strlen(s)};
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

static int expect_fragment(
    const char *name,
    const char *html,
    uint64_t expected_r2,
    int expect_g13)
{
    arbor_view0_native_diagnostic diagnostics[64] = {{0}};
    arbor_view0_native_result result = {0};
    const arbor_status status = arbor_view0_native_check_fragment_model(
        span_from_cstr(html), diagnostics, 64u, &result);
    if (status.native != 0) {
        (void)fprintf(stderr, "FAIL %s mechanism=%" PRId64 "\n", name, status.native);
        return 1;
    }
    if (count_rule(
            diagnostics,
            result.diagnostic_count,
            ARBOR_VIEW_V1_G04_TRANSPARENT_PARENTLESS_FLOW) != expected_r2) {
        (void)fprintf(stderr, "FAIL %s R2 count diagnostics=%" PRIu64 "\n",
                      name, result.diagnostic_count);
        return 2;
    }
    const int g13 =
        (result.flags & ARBOR_VIEW0_NATIVE_RESULT_FLAG_G04_R2_DEFERRED_G13_CUSTOM) != 0u;
    if (g13 != expect_g13) {
        (void)fprintf(stderr, "FAIL %s G13 flag=0x%" PRIx64 "\n", name, result.flags);
        return 3;
    }
    return 0;
}

int main(void)
{
    int rc = expect_fragment("matrix-positive", "<a><div>x</div></a>", 0u, 0);
    if (rc != 0) return 10 + rc;

    arbor_view0_native_diagnostic negative[16] = {{0}};
    arbor_view0_native_result negative_result = {0};
    arbor_status status = arbor_view0_native_check_fragment_model(
        span_from_cstr("<a><html></html></a>"), negative, 16u, &negative_result);
    if (status.native != 0 ||
        count_rule(negative, negative_result.diagnostic_count,
                   ARBOR_VIEW_V1_G04_TRANSPARENT_PARENTLESS_FLOW) != 1u)
        return 20;
    const arbor_view0_native_diagnostic *r2 = NULL;
    for (uint64_t i = 0u; i < negative_result.diagnostic_count; ++i) {
        if (negative[i].rule_id == ARBOR_VIEW_V1_G04_TRANSPARENT_PARENTLESS_FLOW) {
            r2 = &negative[i];
            break;
        }
    }
    if (r2 == NULL || r2->severity != (uint32_t)ARBOR_VIEW0_NATIVE_SEVERITY_ERROR ||
        r2->origin != (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_AUTHORING ||
        r2->byte_offset != 4u || r2->source_length != 4u ||
        r2->line != 1u || r2->column != 5u ||
        strcmp(r2->symbolic_name, "ARBOR_VIEW_V1_G04_TRANSPARENT_PARENTLESS_FLOW") != 0)
        return 21;

    const struct {
        const char *name;
        const char *html;
        uint64_t expected_r2;
        int g13;
    } cases[] = {
        {"section-flow", "<a><section>x</section></a>", 0u, 0},
        {"head-not-flow", "<a><head></head></a>", 1u, 0},
        {"iterative", "<a><ins><div>x</div></ins></a>", 0u, 0},
        {"non-subject", "<p><a><div>x</div></a></p>", 0u, 0},
        {"video-prefix", "<video><source><div>x</div></video>", 0u, 0},
        {"audio-prefix", "<audio><track><section>x</section></audio>", 0u, 0},
        {"noscript-disabled", "<noscript><div>x</div></noscript>", 0u, 0},
        {"custom-root", "<x-r2><html></html></x-r2>", 0u, 1},
        {"custom-nested", "<a><x-r2></x-r2></a>", 0u, 1}
    };
    for (size_t i = 0u; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        rc = expect_fragment(cases[i].name, cases[i].html, cases[i].expected_r2, cases[i].g13);
        if (rc != 0) return (int)(30u + i * 4u + (size_t)rc);
    }

    arbor_view0_native_g04_r2a_evaluation measured = {0};
    status = arbor_view0_native_g04_r2a_measure_fragment_model(
        span_from_cstr("<a><html></html></a>"), &measured);
    if (status.native != 0 || measured.diagnostic_count != 1u ||
        measured.parse_counts.tree_error_count == 0u)
        return 80;
    arbor_view0_native_source_anchor anchors[2] = {{0}};
    arbor_view0_native_g04_r2a_evaluation collected = {0};
    status = arbor_view0_native_g04_r2a_collect_fragment_anchors(
        span_from_cstr("<a><html></html></a>"), anchors, 1u, &collected);
    if (status.native != 0 || memcmp(&measured, &collected, sizeof(measured)) != 0 ||
        anchors[0].byte_offset != 4u || anchors[0].source_length != 4u)
        return 81;

    (void)puts("VIEW0_V1N1_G04_R2_FROZEN_MATRIX_FIXTURES=2_OF_2");
    (void)puts("VIEW0_V1N1_G04_R2_EXPLICIT_FRAGMENT_MODEL=BODY_CONTEXT_PASS");
    (void)puts("VIEW0_V1N1_G04_R2_SYNTHETIC_WRAPPER_NOT_AUTHORED_PARENT=PASS");
    (void)puts("VIEW0_V1N1_G04_R2_PARENTLESS_FLOW_FALLBACK=PASS");
    (void)puts("VIEW0_V1N1_G04_R2_ITERATIVE_TRANSPARENT_FLOW=PASS");
    (void)puts("VIEW0_V1N1_G04_R2_MEDIA_PREFIX=PASS");
    (void)puts("VIEW0_V1N1_G04_R2_NOSCRIPT_SCRIPTING_DISABLED=PASS");
    (void)puts("VIEW0_V1N1_G04_R2_G13_EXTERNAL_DEPENDENCY=PASS");
    (void)puts("PASS: VIEW0 V1N1 G04 R2 parentless transparent flow semantics");
    return 0;
}
