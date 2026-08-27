#include "g04_r1a.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define R1_RULE_ID UINT64_C(0x0000000030040001)

static arbor_span span_from_cstr(const char *s)
{
    return (arbor_span){(const uint8_t *)s, (uint64_t)strlen(s)};
}

static int evaluate_case(
    const char *name,
    const char *html,
    uint64_t diagnostics,
    uint64_t suppressions,
    uint64_t option_resolved,
    uint64_t text_violations,
    uint64_t flags)
{
    arbor_view0_native_g04_r1a_evaluation measured = {0};
    arbor_status status = arbor_view0_native_g04_r1a_measure(span_from_cstr(html), &measured);
    if (status.native != 0 || measured.diagnostic_count != diagnostics ||
        measured.prior_owner_suppression_count != suppressions ||
        measured.option_branch_resolved_count != option_resolved ||
        measured.select_text_violation_count != text_violations ||
        measured.deferred_flags != flags) {
        (void)fprintf(stderr,
            "FAIL %s status=%lld d=%llu s=%llu option_resolved=%llu text=%llu flags=0x%llx\n",
            name, (long long)status.native,
            (unsigned long long)measured.diagnostic_count,
            (unsigned long long)measured.prior_owner_suppression_count,
            (unsigned long long)measured.option_branch_resolved_count,
            (unsigned long long)measured.select_text_violation_count,
            (unsigned long long)measured.deferred_flags);
        return 1;
    }

    arbor_view0_native_diagnostic out[8] = {{0}};
    arbor_view0_native_g04_r1a_evaluation collected = {0};
    status = arbor_view0_native_g04_r1a_collect(
        span_from_cstr(html), out, diagnostics, 73u, &collected);
    if (status.native != 0 || memcmp(&measured, &collected, sizeof(measured)) != 0) {
        (void)fprintf(stderr, "FAIL %s collect status=%lld\n", name, (long long)status.native);
        return 2;
    }
    for (uint64_t i = 0u; i < diagnostics; ++i) {
        if (out[i].rule_id != R1_RULE_ID ||
            out[i].severity != (uint32_t)ARBOR_VIEW0_NATIVE_SEVERITY_ERROR ||
            out[i].origin != (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_AUTHORING ||
            out[i].discovery_sequence != 73u + i || out[i].source_length == 0u) {
            (void)fprintf(stderr, "FAIL %s diagnostic identity\n", name);
            return 3;
        }
    }
    return 0;
}

static int exact_anchor(
    const char *html, const char *needle, uint64_t expected_diagnostics)
{
    const char *p = strstr(html, needle);
    if (p == NULL || needle[0] != '<') return 1;
    const char *name = p + 1;
    const char *name_end = name;
    while (*name_end != '\0' && *name_end != '>' && *name_end != ' ' && *name_end != '\t')
        ++name_end;
    const uint64_t expected_offset = (uint64_t)(name - html);
    const uint64_t expected_length = (uint64_t)(name_end - name);

    arbor_view0_native_g04_r1a_evaluation measured = {0};
    arbor_status status = arbor_view0_native_g04_r1a_measure(span_from_cstr(html), &measured);
    if (status.native != 0 || measured.diagnostic_count != expected_diagnostics) return 2;
    if (expected_diagnostics == 0u) return 0;
    arbor_view0_native_source_anchor anchors[4] = {{0}};
    arbor_view0_native_g04_r1a_evaluation collected = {0};
    status = arbor_view0_native_g04_r1a_collect_anchors(
        span_from_cstr(html), anchors, expected_diagnostics, &collected);
    if (status.native != 0 || memcmp(&measured, &collected, sizeof(measured)) != 0) return 3;
    if (anchors[0].byte_offset != (uint32_t)expected_offset ||
        anchors[0].source_length != (uint32_t)expected_length) return 4;
    return 0;
}

int main(void)
{
    const uint64_t custom = ARBOR_VIEW0_NATIVE_RESULT_FLAG_G04_R1_DEFERRED_G13_CUSTOM;

    struct {
        const char *name;
        const char *html;
        uint64_t d, s, o, t, flags;
    } cases[] = {
        {"matrix-positive", "<!doctype html><title>x</title><body><p><a href=\"/\"><em>x</em></a></p></body>", 0,0,0,0,0},
        {"matrix-negative", "<!doctype html><title>x</title><body><p><a href=\"/\"><div>x</div></a></p></body>", 1,0,0,0,0},
        {"iterative-positive", "<!doctype html><title>x</title><body><p><object><ins><map><a href=\"/\">Apples</a></map></ins></object></p></body>", 0,0,0,0,0},
        {"flow-context", "<!doctype html><title>x</title><body><ins><p>x</p></ins></body>", 0,0,0,0,0},
        {"select-element-valid", "<!doctype html><title>x</title><body><select><div><option>x</option></div></select></body>", 0,0,0,0,0},
        {"select-text-invalid", "<!doctype html><title>x</title><body><select><div>bad</div></select></body>", 1,0,0,1,0},
        {"select-whitespace-valid", "<!doctype html><title>x</title><body><select><div> \n\t\r\f </div></select></body>", 0,0,0,0,0},
        {"select-charref-space-valid", "<!doctype html><title>x</title><body><select><div>&#32;</div></select></body>", 0,0,0,0,0},
        {"select-charref-amp-invalid", "<!doctype html><title>x</title><body><select><div>&amp;</div></select></body>", 1,0,0,1,0},
        {"optgroup-text-invalid", "<!doctype html><title>x</title><body><select><optgroup label=x><div>bad</div></optgroup></select></body>", 1,0,0,1,0},
        {"option-no-label-resolved", "<!doctype html><title>x</title><body><select><option><div><span>x</span></div></option></select></body>", 0,0,1,0,0},
        {"option-value-no-label-resolved", "<!doctype html><title>x</title><body><select><option value=v><div>text</div></option></select></body>", 0,0,1,0,0},
        {"option-nested-div-resolved", "<!doctype html><title>x</title><body><select><option><div><div><em>x</em></div></div></option></select></body>", 0,0,1,0,0},
        {"option-label-g03-owned", "<!doctype html><title>x</title><body><select><option label=x><div><span>x</span></div></option></select></body>", 0,0,0,0,0},
        {"option-datalist-g03-owned", "<!doctype html><title>x</title><body><datalist><option><div><span>x</span></div></option></datalist></body>", 0,0,0,0,0},
        {"noscript-resolved-under-r1c", "<!doctype html><title>x</title><body><noscript><div>x</div></noscript></body>", 0,0,0,0,0},
        {"custom-still-deferred", "<!doctype html><title>x</title><body><x-r1><div>x</div></x-r1></body>", 0,0,0,0,custom}
    };

    for (size_t i = 0u; i < sizeof(cases)/sizeof(cases[0]); ++i) {
        int rc = evaluate_case(cases[i].name, cases[i].html, cases[i].d, cases[i].s,
                               cases[i].o, cases[i].t, cases[i].flags);
        if (rc != 0) return (int)(10u + i * 4u + (size_t)rc);
    }

    static const char matrix_negative[] =
        "<!doctype html><title>x</title><body><p><a href=\"/\"><div>x</div></a></p></body>";
    if (exact_anchor(matrix_negative, "<div>", 1u) != 0) return 100;
    static const char select_text[] =
        "<!doctype html><title>x</title><body><select><div>bad</div></select></body>";
    if (exact_anchor(select_text, "<div>", 1u) != 0) return 101;

    (void)puts("VIEW0_V1N1_G04_R1B_FROZEN_MATRIX_FIXTURES=2_OF_2");
    (void)puts("VIEW0_V1N1_G04_R1B_OPTION_BRANCH_SOURCE_ATTRIBUTES=PASS");
    (void)puts("VIEW0_V1N1_G04_R1B_SELECT_FAMILY_SOURCE_TEXT=PASS");
    (void)puts("VIEW0_V1N1_G04_R1B_CHARACTER_REFERENCE_TEXT=PASS");
    (void)puts("VIEW0_V1N1_G04_R1B_EXACT_ELEMENT_ANCHORS=PASS");
    (void)puts("VIEW0_V1N1_G04_R1B_RETAINED_UNDER_R1C=PASS_G13_ONLY");
    (void)puts("PASS: VIEW0 V1N1 G04 R1B transparent-parent closure increment");
    return 0;
}
