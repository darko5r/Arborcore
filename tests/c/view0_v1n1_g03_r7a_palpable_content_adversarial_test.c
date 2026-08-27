#include <arborcore/view0_conformance/native.h>

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define R7_RULE_ID UINT64_C(0x0000000030030007)

static arbor_span span_from_cstr(const char *s)
{
    return (arbor_span){(const uint8_t *)s, (uint64_t)strlen(s)};
}

static uint64_t count_rule(const arbor_view0_native_diagnostic *d, uint64_t n, uint64_t rule)
{
    uint64_t c = 0u;
    for (uint64_t i = 0u; i < n; ++i) if (d[i].rule_id == rule) c += 1u;
    return c;
}

int main(void)
{
    const uint64_t r7_deferred_mask =
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R7_DEFERRED_G13_CUSTOM;
    const uint64_t active_partial_flags =
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R1_PARTIAL |
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_PARTIAL |
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_PARTIAL |
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R4_PARTIAL |
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R5_PARTIAL |
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R7_PARTIAL;

    static const char two_warnings[] =
        "<!doctype html><title>x</title><body><p></p><p></p></body>";
    arbor_view0_native_diagnostic ample[8] = {{0}};
    arbor_view0_native_result ample_result = {0};
    arbor_status st = arbor_view0_native_check(span_from_cstr(two_warnings), ample, 8u, &ample_result);
    if (st.native != 0 || count_rule(ample, ample_result.diagnostic_count, R7_RULE_ID) != 2u) return 1;

    arbor_view0_native_diagnostic small[8], before[8];
    (void)memset(small, 0x5a, sizeof(small));
    (void)memcpy(before, small, sizeof(small));
    arbor_view0_native_result failed = {11u,22u,33u,44u};
    const arbor_view0_native_result failed_before = failed;
    st = arbor_view0_native_check(span_from_cstr(two_warnings), small, 1u, &failed);
    if (st.native != -(int64_t)ENOSPC || memcmp(small,before,sizeof(small)) != 0 ||
        memcmp(&failed,&failed_before,sizeof(failed)) != 0) return 2;

    arbor_view0_native_diagnostic exact[8] = {{0}}, repeat[8] = {{0}};
    arbor_view0_native_result exact_result = {0}, repeat_result = {0};
    st = arbor_view0_native_check(span_from_cstr(two_warnings), exact, 2u, &exact_result);
    if (st.native != 0 || memcmp(&ample_result,&exact_result,sizeof(ample_result)) != 0 ||
        memcmp(ample,exact,2u*sizeof(exact[0])) != 0) return 3;
    st = arbor_view0_native_check(span_from_cstr(two_warnings), repeat, 8u, &repeat_result);
    if (st.native != 0 || memcmp(&ample_result,&repeat_result,sizeof(ample_result)) != 0 ||
        memcmp(ample,repeat,2u*sizeof(repeat[0])) != 0) return 4;

    static const uint8_t invalid_utf8[] = {
        '<','!','d','o','c','t','y','p','e',' ','h','t','m','l','>',
        '<','t','i','t','l','e','>','x','<','/','t','i','t','l','e','>',
        '<','p','>',0xf0,0x80,0x80,0x80
    };
    arbor_view0_native_diagnostic utf8d[8] = {{0}};
    arbor_view0_native_result utf8r = {0};
    st = arbor_view0_native_check((arbor_span){invalid_utf8,sizeof(invalid_utf8)}, utf8d, 8u, &utf8r);
    if (st.native != 0 || utf8r.diagnostic_count != 1u ||
        utf8d[0].origin != (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_UTF8 ||
        count_rule(utf8d,utf8r.diagnostic_count,R7_RULE_ID) != 0u ||
        (utf8r.flags & active_partial_flags) != active_partial_flags) return 5;

    arbor_view0_native_diagnostic emptyd[8] = {{0}};
    arbor_view0_native_result emptyr = {0};
    st = arbor_view0_native_check((arbor_span){NULL,0u}, emptyd, 8u, &emptyr);
    if (st.native != 0 || count_rule(emptyd,emptyr.diagnostic_count,R7_RULE_ID) != 0u ||
        (emptyr.flags & active_partial_flags) != active_partial_flags) return 6;

    static const char custom[] =
        "<!doctype html><title>x</title><body><div><x-r7></x-r7></div></body>";
    arbor_view0_native_diagnostic customd[8] = {{0}}; arbor_view0_native_result customr = {0};
    st = arbor_view0_native_check(span_from_cstr(custom), customd, 8u, &customr);
    if (st.native != 0 || count_rule(customd,customr.diagnostic_count,R7_RULE_ID) != 0u ||
        (customr.flags & r7_deferred_mask) !=
            ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R7_DEFERRED_G13_CUSTOM) return 7;

    static const char transparent[] =
        "<!doctype html><title>x</title><body><select><option><div></div></option></select></body>";
    arbor_view0_native_diagnostic transd[8] = {{0}}; arbor_view0_native_result transr = {0};
    st = arbor_view0_native_check(span_from_cstr(transparent), transd, 8u, &transr);
    if (st.native != 0 || count_rule(transd,transr.diagnostic_count,R7_RULE_ID) != 1u ||
        (transr.flags & r7_deferred_mask) != 0u) return 8;

    static const char coexist[] = "<title>x</title><body><p></p></body>";
    arbor_view0_native_diagnostic co[8] = {{0}}; arbor_view0_native_result cor = {0};
    st = arbor_view0_native_check(span_from_cstr(coexist), co, 8u, &cor);
    if (st.native != 0 || count_rule(co,cor.diagnostic_count,R7_RULE_ID) != 1u ||
        cor.diagnostic_count <= count_rule(co,cor.diagnostic_count,R7_RULE_ID)) return 9;

    (void)puts("VIEW0_V1N1_G03_R7A_CAPACITY_FAILURE_ATOMICITY=PASS");
    (void)puts("VIEW0_V1N1_G03_R7A_DETERMINISM=PASS");
    (void)puts("VIEW0_V1N1_G03_R7A_UTF8_PRECEDENCE=PASS");
    (void)puts("VIEW0_V1N1_G03_R7A_RESULT_FLAGS=PASS");
    (void)puts("VIEW0_V1N1_G03_R7A_INDEPENDENT_WARNING_COEXISTENCE=PASS");
    (void)puts("PASS: VIEW0 V1N1 G03 R7A adversarial qualification");
    return 0;
}
