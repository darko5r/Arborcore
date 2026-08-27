#include <arborcore/view0_conformance/native.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int count_rule(const char *html, uint64_t rule, uint64_t *offset_out, uint64_t *length_out)
{
    arbor_view0_native_diagnostic d[64] = {{0}};
    arbor_view0_native_result r = {0};
    arbor_span in = {(const uint8_t *)html, (uint64_t)strlen(html)};
    arbor_status st = arbor_view0_native_check(in, d, 64u, &r);
    if (st.native != 0) return -1;
    uint64_t n = 0u;
    for (uint64_t i = 0u; i < r.diagnostic_count; ++i) {
        if (d[i].rule_id == rule) {
            if (n == 0u && offset_out != NULL) {
                *offset_out = d[i].byte_offset;
                *length_out = d[i].source_length;
            }
            n += 1u;
        }
    }
    return (int)n;
}

int main(void)
{
    const char *pos = "<!doctype html><title>x</title><body><a href=\"/\">x</a></body>";
    const char *neg = "<!doctype html><title>x</title><body><p href=\"/\">x</p></body>";
    uint64_t off = 0u, len = 0u;
    if (count_rule(pos, ARBOR_VIEW_V1_G05_ELEMENT_ATTRIBUTE_APPLICABILITY, NULL, NULL) != 0) return 10;
    if (count_rule(neg, ARBOR_VIEW_V1_G05_ELEMENT_ATTRIBUTE_APPLICABILITY, &off, &len) != 1) return 11;
    const char *q = strstr(neg, "href");
    if (q == NULL || off != (uint64_t)(q - neg) || len != 4u) return 12;
    if (count_rule("<!doctype html><title>x</title><body><p id=\"x\">x</p></body>", ARBOR_VIEW_V1_G05_ELEMENT_ATTRIBUTE_APPLICABILITY, NULL, NULL) != 0) return 13;
    if (count_rule("<!doctype html><title>x</title><body><p bogus=\"x\">x</p></body>", ARBOR_VIEW_V1_G05_ELEMENT_ATTRIBUTE_APPLICABILITY, NULL, NULL) != 0) return 14;
    if (count_rule("<!doctype html><title>x</title><body><a target=\"_blank\">x</a></body>", ARBOR_VIEW_V1_G05_ELEMENT_ATTRIBUTE_APPLICABILITY, NULL, NULL) != 0) return 15;
    if (count_rule("<!doctype html><title>x</title><body><p onpageshow=\"x()\">x</p></body>", ARBOR_VIEW_V1_G05_ELEMENT_ATTRIBUTE_APPLICABILITY, NULL, NULL) != 0) return 16;
    if (count_rule("<!doctype html><title>x</title><body><x-foo href=\"/\">x</x-foo></body>", ARBOR_VIEW_V1_G05_ELEMENT_ATTRIBUTE_APPLICABILITY, NULL, NULL) != 0) return 17;
    const char *pair = "<!doctype html><title>x</title><body><img src=\"x\" alt=\"x\"><p src=\"x\">y</p></body>";
    if (count_rule(pair, ARBOR_VIEW_V1_G05_ELEMENT_ATTRIBUTE_APPLICABILITY, &off, &len) != 1) return 18;
    q = strstr(pair, "src=\"x\">y");
    if (q == NULL || off != (uint64_t)(q - pair) || len != 3u) return 19;
    printf("VIEW0_V1N1_G05_R2A_FROZEN_MATRIX_FIXTURES=2_OF_2\n");
    printf("VIEW0_V1N1_G05_R2A_NEGATIVE_ATTRIBUTE_ANCHOR=PASS\n");
    printf("VIEW0_V1N1_G05_R2A_STATIC_ELEMENT_PAIR_CATALOG=PASS\n");
    printf("VIEW0_V1N1_G05_R2A_R1_R3_R4_HANDOFF=PASS\n");
    printf("VIEW0_V1N1_G05_R2A_NONSTANDARD_OWNER_SCOPE=PASS\n");
    return 0;
}
