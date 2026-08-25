#include "g03_r7a.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define R7_RULE_ID UINT64_C(0x0000000030030007)

typedef struct r7_case {
    const char *name;
    const char *html;
    uint64_t warnings;
    uint64_t expected_deferred_flags;
} r7_case;

static arbor_span span_from_cstr(const char *s)
{
    return (arbor_span){(const uint8_t *)s, (uint64_t)strlen(s)};
}

static int run_case(const r7_case *tc)
{
    arbor_view0_native_g03_r7a_evaluation measured = {0};
    arbor_status st = arbor_view0_native_g03_r7a_measure(span_from_cstr(tc->html), &measured);
    if (st.native != 0 || measured.diagnostic_count != tc->warnings ||
        measured.deferred_flags != tc->expected_deferred_flags) {
        return 1;
    }
    arbor_view0_native_diagnostic out[8] = {{0}};
    arbor_view0_native_g03_r7a_evaluation collected = {0};
    st = arbor_view0_native_g03_r7a_collect(
        span_from_cstr(tc->html), out, tc->warnings, 7u, &collected);
    if (st.native != 0 || memcmp(&measured, &collected, sizeof(measured)) != 0) return 2;
    for (uint64_t i = 0; i < tc->warnings; ++i) {
        if (out[i].rule_id != R7_RULE_ID ||
            out[i].severity != (uint32_t)ARBOR_VIEW0_NATIVE_SEVERITY_WARNING ||
            out[i].origin != (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_AUTHORING ||
            strcmp(out[i].symbolic_name, "ARBOR_VIEW_V1_G03_PALPABLE_PHRASING_NONEMPTY") != 0 ||
            out[i].byte_offset == ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE ||
            out[i].source_length == 0u || out[i].discovery_sequence != 7u + i) return 3;
    }
    return 0;
}

int main(void)
{
    static const r7_case cases[] = {
        {"R7F001", "<!doctype html><title>x</title><body><p>x</p></body>", 0u, 0u},
        {"R7F002", "<!doctype html><title>x</title><body><p></p></body>", 1u, 0u},
        {"R7F003", "<!doctype html><title>x</title><body><div>   </div></body>", 1u, 0u},
        {"R7F004", "<!doctype html><title>x</title><body><div><span>x</span></div></body>", 0u, 0u},
        {"R7F005", "<!doctype html><title>x</title><body><div><span hidden>x</span></div></body>", 1u, 0u},
        {"R7F006", "<!doctype html><title>x</title><body><div><style>x</style></div></body>", 1u, 0u},
        {"R7F007", "<!doctype html><title>x</title><body><div><input></div></body>", 0u, 0u},
        {"R7F008", "<!doctype html><title>x</title><body><div><input type=hidden></div></body>", 1u, 0u},
        {"R7F009", "<!doctype html><title>x</title><body><div><ul><li>x</li></ul></div></body>", 0u, 0u},
        {"R7F010", "<!doctype html><title>x</title><body><div><ul></ul></div></body>", 1u, 0u},
        {"R7F011", "<!doctype html><title>x</title><body><div><svg></svg></div></body>", 0u, 0u},
        {"R7F012", "<!doctype html><title>x</title><body><div><x-r7></x-r7></div></body>", 0u,
            ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R7_DEFERRED_G13_CUSTOM},
        {"R7F013", "<!doctype html><title>x</title><body><time datetime=2026-01-01></time></body>", 1u, 0u},
        {"R7F014", "<!doctype html><title>x</title><body><time></time></body>", 0u, 0u},
        {"R7F015", "<!doctype html><title>x</title><body><select><option></option></select></body>", 1u, 0u},
        {"R7F016", "<!doctype html><title>x</title><body><select><option label=x value=y></option></select></body>", 0u, 0u},
        {"R7F017", "<!doctype html><title>x</title><body><dl><div><dt>x</dt><dd>y</dd></div></dl></body>", 0u, 0u},
        {"R7F018", "<!doctype html><title>x</title><body><select><option><div></div></option></select></body>", 0u,
            ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R7_DEFERRED_G04_TRANSPARENT},
        {"audio-controls", "<!doctype html><title>x</title><body><div><audio controls></audio></div></body>", 0u, 0u},
        {"audio-no-controls", "<!doctype html><title>x</title><body><div><audio></audio></div></body>", 1u, 0u},
        {"dl-direct-group", "<!doctype html><title>x</title><body><div><dl><dt>x</dt><dd>y</dd></dl></div></body>", 0u, 0u},
        {"dl-no-group", "<!doctype html><title>x</title><body><div><dl></dl></div></body>", 1u, 0u},
        {"menu-li", "<!doctype html><title>x</title><body><div><menu><li>x</li></menu></div></body>", 0u, 0u},
        {"ol-no-li", "<!doctype html><title>x</title><body><div><ol></ol></div></body>", 1u, 0u},
        {"mathml", "<!doctype html><title>x</title><body><div><math></math></div></body>", 0u, 0u},
        {"svg-hidden", "<!doctype html><title>x</title><body><div><svg hidden></svg></div></body>", 1u, 0u},
        {"math-hidden", "<!doctype html><title>x</title><body><div><math hidden></math></div></body>", 1u, 0u},
        {"hidden-custom", "<!doctype html><title>x</title><body><div><x-r7 hidden></x-r7></div></body>", 1u,
            ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R7_DEFERRED_G13_CUSTOM}
    };
    for (size_t i = 0; i < sizeof(cases)/sizeof(cases[0]); ++i) {
        const int rc = run_case(&cases[i]);
        if (rc != 0) {
            (void)fprintf(stderr, "FAIL %s rc=%d\n", cases[i].name, rc);
            return (int)(10u + i);
        }
    }
    (void)puts("VIEW0_V1N1_G03_R7A_F1_R13_FIXTURES=18_OF_18");
    (void)puts("VIEW0_V1N1_G03_R7A_CONDITIONAL_PALPABILITY_CONTROLS=10_OF_10");
    (void)puts("PASS: VIEW0 V1N1 G03 R7A direct-child palpable warning semantics");
    return 0;
}
