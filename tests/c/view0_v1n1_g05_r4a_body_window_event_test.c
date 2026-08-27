#include <arborcore/view0_conformance/native.h>
#include "g05_c0.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int count_rule(const char *html, uint64_t rule, uint64_t *off, uint64_t *len)
{
    arbor_view0_native_diagnostic d[128] = {{0}};
    arbor_view0_native_result r = {0};
    const arbor_span in = {(const uint8_t *)html, (uint64_t)strlen(html)};
    const arbor_status st = arbor_view0_native_check(in, d, UINT64_C(128), &r);
    if (st.native != 0) return -1;
    uint64_t n = 0u;
    for (uint64_t i = 0u; i < r.diagnostic_count; ++i) {
        if (d[i].rule_id == rule) {
            if (n == 0u && off != NULL) { *off = d[i].byte_offset; *len = d[i].source_length; }
            n += 1u;
        }
    }
    return (int)n;
}

int main(void)
{
    static const char pos[] = "<!doctype html><title>x</title><body ononline=\"x()\"></body>";
    static const char neg[] = "<!doctype html><title>x</title><body><p onpageshow=\"x()\"></p></body>";
    uint64_t off = 0u, len = 0u;
    if (count_rule(pos, ARBOR_VIEW_V1_G05_BODY_WINDOW_EVENT_ATTRIBUTE_APPLICABILITY, NULL, NULL) != 0) return 10;
    if (count_rule(neg, ARBOR_VIEW_V1_G05_BODY_WINDOW_EVENT_ATTRIBUTE_APPLICABILITY, &off, &len) != 1) return 11;
    const char *q = strstr(neg, "onpageshow");
    if (q == NULL || off != (uint64_t)(q - neg) || len != UINT64_C(10)) return 12;
    if (arbor_view0_native_g05_c0_body_window_event_count() != UINT64_C(18)) return 13;

    static const char *names[] = {
        "onafterprint","onbeforeprint","onbeforeunload","onhashchange","onlanguagechange",
        "onmessage","onmessageerror","onoffline","ononline","onpageswap","onpagehide",
        "onpagereveal","onpageshow","onpopstate","onrejectionhandled","onstorage",
        "onunhandledrejection","onunload"
    };
    if (sizeof(names) / sizeof(names[0]) != 18u) return 14;
    for (size_t i = 0u; i < sizeof(names) / sizeof(names[0]); ++i) {
        char body[256]; char para[256];
        const int nb = snprintf(body, sizeof(body), "<!doctype html><title>x</title><body %s=\"not javascript validated here\"></body>", names[i]);
        const int np = snprintf(para, sizeof(para), "<!doctype html><title>x</title><body><p %s=\"x()\"></p></body>", names[i]);
        if (nb < 0 || np < 0 || (size_t)nb >= sizeof(body) || (size_t)np >= sizeof(para)) return 15;
        if (count_rule(body, ARBOR_VIEW_V1_G05_BODY_WINDOW_EVENT_ATTRIBUTE_APPLICABILITY, NULL, NULL) != 0) return 16;
        if (count_rule(para, ARBOR_VIEW_V1_G05_BODY_WINDOW_EVENT_ATTRIBUTE_APPLICABILITY, NULL, NULL) != 1) return 17;
    }

    if (count_rule("<!doctype html><title>x</title><body><p onclick=\"x()\"></p></body>", ARBOR_VIEW_V1_G05_BODY_WINDOW_EVENT_ATTRIBUTE_APPLICABILITY, NULL, NULL) != 0) return 18;
    if (count_rule("<!doctype html><title>x</title><body onpageshow=\"{ definitely not parsed as JS here }\"></body>", ARBOR_VIEW_V1_G05_BODY_WINDOW_EVENT_ATTRIBUTE_APPLICABILITY, NULL, NULL) != 0) return 19;

    (void)puts("VIEW0_V1N1_G05_R4A_FROZEN_MATRIX_FIXTURES=2_OF_2");
    (void)puts("VIEW0_V1N1_G05_R4A_NEGATIVE_ATTRIBUTE_ANCHOR=PASS");
    (void)puts("VIEW0_V1N1_G05_R4A_BODY_WINDOW_EVENT_CATALOG=18_OF_18");
    (void)puts("VIEW0_V1N1_G05_R4A_BODY_ONLY_PLACEMENT=PASS");
    (void)puts("VIEW0_V1N1_G05_R4A_VALUE_SEMANTICS=DEFER_G16_PASS");
    return 0;
}
