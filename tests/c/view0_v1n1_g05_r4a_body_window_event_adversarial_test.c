#include <arborcore/view0_conformance/native.h>

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint64_t count(const arbor_view0_native_diagnostic *d, const arbor_view0_native_result *r, uint64_t rule)
{
    uint64_t n = 0u;
    for (uint64_t i = 0u; i < r->diagnostic_count; ++i) if (d[i].rule_id == rule) n += 1u;
    return n;
}

static int run(const char *html, arbor_view0_native_diagnostic *d, arbor_view0_native_result *r)
{
    return (int)arbor_view0_native_check(
        (arbor_span){(const uint8_t *)html, (uint64_t)strlen(html)}, d, UINT64_C(128), r).native;
}

int main(void)
{
    static const char html[] = "<!doctype html><title>x</title><body><p onpageshow=\"x()\"></p></body>";
    arbor_view0_native_diagnostic d1[128]={{0}}, d2[128]={{0}};
    arbor_view0_native_result r1={0}, r2={0};
    if (run(html,d1,&r1)!=0 || run(html,d2,&r2)!=0) return 20;
    if (memcmp(&r1,&r2,sizeof(r1))!=0 || memcmp(d1,d2,sizeof(d1))!=0) return 21;
    if (count(d1,&r1,ARBOR_VIEW_V1_G05_BODY_WINDOW_EVENT_ATTRIBUTE_APPLICABILITY)!=1u) return 22;
    if (count(d1,&r1,ARBOR_VIEW_V1_G05_GLOBAL_ATTRIBUTE_APPLICABILITY)!=0u ||
        count(d1,&r1,ARBOR_VIEW_V1_G05_ELEMENT_ATTRIBUTE_APPLICABILITY)!=0u ||
        count(d1,&r1,ARBOR_VIEW_V1_G05_CONDITIONAL_ATTRIBUTE_APPLICABILITY)!=0u) return 23;

    arbor_view0_native_diagnostic sentinel; arbor_view0_native_result rr;
    (void)memset(&sentinel,0xa5,sizeof(sentinel)); (void)memset(&rr,0x5a,sizeof(rr));
    const arbor_view0_native_diagnostic before=sentinel; const arbor_view0_native_result rbefore=rr;
    const arbor_status cap=arbor_view0_native_check(
        (arbor_span){(const uint8_t *)html,(uint64_t)(sizeof(html)-1u)},&sentinel,0u,&rr);
    if (cap.native != -(int64_t)ENOSPC || memcmp(&sentinel,&before,sizeof(sentinel))!=0 || memcmp(&rr,&rbefore,sizeof(rr))!=0) return 24;

    struct { const char *html; uint64_t owner; uint64_t not_owner; } cases[] = {
        {"<!doctype html><title>x</title><body><p bogus=\"x\"></p></body>", ARBOR_VIEW_V1_G05_GLOBAL_ATTRIBUTE_APPLICABILITY, ARBOR_VIEW_V1_G05_BODY_WINDOW_EVENT_ATTRIBUTE_APPLICABILITY},
        {"<!doctype html><title>x</title><body><p href=\"/\"></p></body>", ARBOR_VIEW_V1_G05_ELEMENT_ATTRIBUTE_APPLICABILITY, ARBOR_VIEW_V1_G05_BODY_WINDOW_EVENT_ATTRIBUTE_APPLICABILITY},
        {"<!doctype html><title>x</title><body><a target=\"_blank\">x</a></body>", ARBOR_VIEW_V1_G05_CONDITIONAL_ATTRIBUTE_APPLICABILITY, ARBOR_VIEW_V1_G05_BODY_WINDOW_EVENT_ATTRIBUTE_APPLICABILITY}
    };
    for (size_t i=0u;i<sizeof(cases)/sizeof(cases[0]);++i) {
        arbor_view0_native_diagnostic d[128]={{0}}; arbor_view0_native_result r={0};
        if (run(cases[i].html,d,&r)!=0 || count(d,&r,cases[i].owner)!=1u || count(d,&r,cases[i].not_owner)!=0u) return 25+(int)i;
    }

    const uint8_t bad[]={0xff}; arbor_view0_native_diagnostic ud[4]={{0}}; arbor_view0_native_result ur={0};
    const arbor_status us=arbor_view0_native_check((arbor_span){bad,1u},ud,4u,&ur);
    if (us.native!=0 || ur.diagnostic_count!=1u || ud[0].rule_id!=ARBOR_VIEW0_NATIVE_RULE_UTF8_INVALID) return 30;

    (void)puts("VIEW0_V1N1_G05_R4A_DETERMINISM=PASS");
    (void)puts("VIEW0_V1N1_G05_R4A_CAPACITY_FAILURE_ATOMICITY=PASS");
    (void)puts("VIEW0_V1N1_G05_R4A_R1_R2_R3_PRIOR_OWNER_NON_DUPLICATION=PASS");
    (void)puts("VIEW0_V1N1_G05_R4A_UTF8_PRECEDENCE=PASS");
    return 0;
}
