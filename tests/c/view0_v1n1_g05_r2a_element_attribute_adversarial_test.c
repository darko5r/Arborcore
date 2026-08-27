#include <arborcore/view0_conformance/native.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int equal_diag(const arbor_view0_native_diagnostic *a, const arbor_view0_native_diagnostic *b)
{
    return memcmp(a, b, sizeof(*a)) == 0;
}

int main(void)
{
    const char *html = "<!doctype html><title>x</title><body><p href=\"/\">x</p></body>";
    arbor_span in = {(const uint8_t *)html, (uint64_t)strlen(html)};
    arbor_view0_native_diagnostic d1[64] = {{0}}, d2[64] = {{0}};
    arbor_view0_native_result r1 = {0}, r2 = {0};
    arbor_status s1 = arbor_view0_native_check(in, d1, 64u, &r1);
    arbor_status s2 = arbor_view0_native_check(in, d2, 64u, &r2);
    if (s1.native != 0 || s2.native != 0 || memcmp(&r1, &r2, sizeof(r1)) != 0) return 20;
    for (uint64_t i = 0u; i < r1.diagnostic_count; ++i) if (!equal_diag(&d1[i], &d2[i])) return 21;
    uint64_t r2_count = 0u, r1_count = 0u;
    for (uint64_t i = 0u; i < r1.diagnostic_count; ++i) {
        if (d1[i].rule_id == ARBOR_VIEW_V1_G05_ELEMENT_ATTRIBUTE_APPLICABILITY) r2_count += 1u;
        if (d1[i].rule_id == ARBOR_VIEW_V1_G05_GLOBAL_ATTRIBUTE_APPLICABILITY) r1_count += 1u;
    }
    if (r2_count != 1u || r1_count != 0u) return 22;

    arbor_view0_native_diagnostic sentinel; (void)memset(&sentinel, 0xA5, sizeof(sentinel));
    arbor_view0_native_result rr; (void)memset(&rr, 0x5A, sizeof(rr));
    arbor_view0_native_diagnostic before = sentinel; arbor_view0_native_result rbefore = rr;
    arbor_status cap = arbor_view0_native_check(in, &sentinel, 0u, &rr);
    if (cap.native != -ENOSPC || memcmp(&sentinel, &before, sizeof(sentinel)) != 0 || memcmp(&rr, &rbefore, sizeof(rr)) != 0) return 23;

    const char *unknown = "<!doctype html><title>x</title><body><p bogus=\"x\">x</p></body>";
    arbor_view0_native_diagnostic u[64] = {{0}}; arbor_view0_native_result ur = {0};
    arbor_span ui = {(const uint8_t *)unknown, (uint64_t)strlen(unknown)};
    if (arbor_view0_native_check(ui, u, 64u, &ur).native != 0) return 24;
    r2_count = 0u; r1_count = 0u;
    for (uint64_t i = 0u; i < ur.diagnostic_count; ++i) {
        if (u[i].rule_id == ARBOR_VIEW_V1_G05_ELEMENT_ATTRIBUTE_APPLICABILITY) r2_count += 1u;
        if (u[i].rule_id == ARBOR_VIEW_V1_G05_GLOBAL_ATTRIBUTE_APPLICABILITY) r1_count += 1u;
    }
    if (r1_count != 1u || r2_count != 0u) return 25;

    const uint8_t bad[] = {0xff}; arbor_span bin = {bad, 1u};
    arbor_view0_native_diagnostic ud[4] = {{0}}; arbor_view0_native_result ubr = {0};
    arbor_status us = arbor_view0_native_check(bin, ud, 4u, &ubr);
    if (us.native != 0 || ubr.diagnostic_count != 1u || ud[0].rule_id != ARBOR_VIEW0_NATIVE_RULE_UTF8_INVALID) return 26;

    printf("VIEW0_V1N1_G05_R2A_DETERMINISM=PASS\n");
    printf("VIEW0_V1N1_G05_R2A_CAPACITY_FAILURE_ATOMICITY=PASS\n");
    printf("VIEW0_V1N1_G05_R2A_R1_PRIOR_OWNER_SUPPRESSION=PASS\n");
    printf("VIEW0_V1N1_G05_R2A_UTF8_PRECEDENCE=PASS\n");
    return 0;
}
