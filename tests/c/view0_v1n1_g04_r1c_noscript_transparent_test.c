#include "g04_r1a.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static arbor_span span_from_cstr(const char *s)
{
    return (arbor_span){(const uint8_t *)s, (uint64_t)strlen(s)};
}

static int check_case(
    const char *name,
    const char *html,
    uint64_t expected_resolved,
    uint64_t expected_diagnostics,
    uint64_t expected_suppressions,
    uint64_t expected_flags)
{
    arbor_view0_native_g04_r1a_evaluation measured = {0};
    arbor_status status = arbor_view0_native_g04_r1a_measure(span_from_cstr(html), &measured);
    if (status.native != 0 ||
        measured.noscript_deferred_count != 0u ||
        measured.noscript_resolved_count != expected_resolved ||
        measured.diagnostic_count != expected_diagnostics ||
        measured.prior_owner_suppression_count != expected_suppressions ||
        measured.deferred_flags != expected_flags) {
        (void)fprintf(stderr,
            "FAIL %s status=%lld resolved=%llu deferred=%llu diagnostics=%llu suppressions=%llu flags=0x%llx\n",
            name,
            (long long)status.native,
            (unsigned long long)measured.noscript_resolved_count,
            (unsigned long long)measured.noscript_deferred_count,
            (unsigned long long)measured.diagnostic_count,
            (unsigned long long)measured.prior_owner_suppression_count,
            (unsigned long long)measured.deferred_flags);
        return 1;
    }

    arbor_view0_native_source_anchor anchors[8] = {{0}};
    arbor_view0_native_g04_r1a_evaluation collected = {0};
    status = arbor_view0_native_g04_r1a_collect_anchors(
        span_from_cstr(html), anchors, expected_diagnostics, &collected);
    if (status.native != 0 || memcmp(&measured, &collected, sizeof(measured)) != 0) {
        (void)fprintf(stderr, "FAIL %s collect status=%lld\n", name, (long long)status.native);
        return 2;
    }
    return 0;
}

int main(void)
{
    const uint64_t g13 = ARBOR_VIEW0_NATIVE_RESULT_FLAG_G04_R1_DEFERRED_G13_CUSTOM;
    struct {
        const char *name;
        const char *html;
        uint64_t resolved;
        uint64_t diagnostics;
        uint64_t suppressions;
        uint64_t flags;
    } cases[] = {
        {"body-flow", "<!doctype html><title>x</title><body><noscript><div>x</div></noscript></body>", 1u, 0u, 0u, 0u},
        {"phrasing-em", "<!doctype html><title>x</title><body><p><noscript><em>x</em></noscript></p></body>", 1u, 0u, 0u, 0u},
        {"phrasing-prior-owner", "<!doctype html><title>x</title><body><span><noscript><section>x</section></noscript></span></body>", 1u, 0u, 1u, 0u},
        {"select-tail", "<!doctype html><title>x</title><body><select><noscript><div><option>x</option></div></noscript></select></body>", 1u, 0u, 0u, 0u},
        {"optgroup-tail", "<!doctype html><title>x</title><body><select><optgroup label=x><noscript><div><option>x</option></div></noscript></optgroup></select></body>", 1u, 0u, 0u, 0u},
        {"option-phrasing", "<!doctype html><title>x</title><body><select><option><noscript><em>x</em></noscript></option></select></body>", 1u, 0u, 0u, 0u},
        {"head-control", "<!doctype html><html><head><title>x</title><noscript><meta name=x content=y></noscript></head><body></body></html>", 0u, 0u, 0u, 0u},
        {"custom-external", "<!doctype html><title>x</title><body><x-r1><span>x</span></x-r1></body>", 0u, 0u, 0u, g13}
    };

    for (size_t i = 0u; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        const int rc = check_case(
            cases[i].name, cases[i].html, cases[i].resolved, cases[i].diagnostics,
            cases[i].suppressions, cases[i].flags);
        if (rc != 0) return (int)(10u + i * 3u + (size_t)rc);
    }

    (void)puts("VIEW0_V1N1_G04_R1C_CHECKER_SCRIPTING_MODE=DISABLED");
    (void)puts("VIEW0_V1N1_G04_R1C_NOSCRIPT_OUTSIDE_HEAD_TRANSPARENT=PASS");
    (void)puts("VIEW0_V1N1_G04_R1C_NOSCRIPT_SELECT_FAMILY_CONTAINING_MODEL=PASS");
    (void)puts("VIEW0_V1N1_G04_R1C_NOSCRIPT_OPTION_PHRASING_PART=PASS");
    (void)puts("VIEW0_V1N1_G04_R1C_HEAD_NOSCRIPT_NONTRANSPARENT_CONTROL=PASS");
    (void)puts("VIEW0_V1N1_G04_R1C_PRIOR_OWNER_SUPPRESSION=PASS");
    (void)puts("VIEW0_V1N1_G04_R1C_REMAINING_R1_DEPENDENCY=G13_ONLY");
    (void)puts("PASS: VIEW0 V1N1 G04 R1C scripting-disabled noscript transparent closure");
    return 0;
}
