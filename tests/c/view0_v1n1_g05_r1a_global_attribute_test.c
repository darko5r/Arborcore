#include <arborcore/view0_conformance/native.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int check_case(const char *html, uint64_t expected_r1, uint64_t *offset_out, uint64_t *length_out)
{
    arbor_view0_native_diagnostic d[32] = {{0}};
    arbor_view0_native_result r = {0};
    arbor_span in = {(const uint8_t *)html, (uint64_t)strlen(html)};
    arbor_status st = arbor_view0_native_check(in, d, 32u, &r);
    if (st.native != 0) return 1;
    uint64_t n=0;
    for(uint64_t i=0;i<r.diagnostic_count;i++) if(d[i].rule_id==ARBOR_VIEW_V1_G05_GLOBAL_ATTRIBUTE_APPLICABILITY){
        if(n==0 && offset_out){*offset_out=d[i].byte_offset;*length_out=d[i].source_length;}
        n++;
    }
    return n==expected_r1 ? 0 : 2;
}

int main(void)
{
    const char *pos="<!doctype html><title>x</title><body><p id=\"x\">ok</p></body>";
    const char *neg="<!doctype html><title>x</title><body><p bogus=\"x\">ok</p></body>";
    uint64_t off=0,len=0;
    if(check_case(pos,0,NULL,NULL)!=0) return 10;
    if(check_case(neg,1,&off,&len)!=0) return 11;
    const char *q=strstr(neg,"bogus");
    if(q==NULL || off!=(uint64_t)(q-neg) || len!=5u) return 12;
    if(check_case("<!doctype html><title>x</title><body><p onclick=\"x()\" data-z=\"q\" aria-label=\"x\">ok</p></body>",0,NULL,NULL)!=0) return 13;
    if(check_case("<!doctype html><title>x</title><body><p href=\"/\">ok</p></body>",0,NULL,NULL)!=0) return 14;
    if(check_case("<!doctype html><title>x</title><body><p onpageshow=\"x()\">ok</p></body>",0,NULL,NULL)!=0) return 15;
    if(check_case("<!doctype html><title>x</title><body><p data-=\"x\">ok</p></body>",1,NULL,NULL)!=0) return 16;
    if(check_case("<!doctype html><title>x</title><body><x-foo bogus=\"x\">ok</x-foo></body>",0,NULL,NULL)!=0) return 17;
    printf("VIEW0_V1N1_G05_R1A_FROZEN_MATRIX_FIXTURES=2_OF_2\n");
    printf("VIEW0_V1N1_G05_R1A_NEGATIVE_ATTRIBUTE_ANCHOR=PASS\n");
    printf("VIEW0_V1N1_G05_R1A_GLOBAL_FAMILIES=PASS\n");
    printf("VIEW0_V1N1_G05_R1A_LATER_G05_OWNERSHIP=R2_R4_PASS\n");
    printf("VIEW0_V1N1_G05_R1A_NONSTANDARD_OWNER_SCOPE=PASS\n");
    return 0;
}
