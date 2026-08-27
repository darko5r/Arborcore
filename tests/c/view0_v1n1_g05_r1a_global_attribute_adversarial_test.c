#include <arborcore/view0_conformance/native.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int equal_diag(const arbor_view0_native_diagnostic *a,const arbor_view0_native_diagnostic *b){return memcmp(a,b,sizeof(*a))==0;}

int main(void)
{
    const char *html="<!doctype html><title>x</title><body><p bogus=\"x\">ok</p></body>";
    arbor_span in={(const uint8_t*)html,(uint64_t)strlen(html)};
    arbor_view0_native_diagnostic d1[32]={{0}},d2[32]={{0}};
    arbor_view0_native_result r1={0},r2={0};
    arbor_status s1=arbor_view0_native_check(in,d1,32,&r1);
    arbor_status s2=arbor_view0_native_check(in,d2,32,&r2);
    if(s1.native!=0||s2.native!=0||memcmp(&r1,&r2,sizeof(r1))!=0) return 20;
    for(uint64_t i=0;i<r1.diagnostic_count;i++) if(!equal_diag(&d1[i],&d2[i])) return 21;
    arbor_view0_native_diagnostic sentinel; memset(&sentinel,0xA5,sizeof(sentinel));
    arbor_view0_native_result rr; memset(&rr,0x5A,sizeof(rr));
    arbor_view0_native_diagnostic before=sentinel; arbor_view0_native_result rbefore=rr;
    arbor_status cap=arbor_view0_native_check(in,&sentinel,0,&rr);
    if(cap.native!=-ENOSPC || memcmp(&sentinel,&before,sizeof(sentinel))!=0 || memcmp(&rr,&rbefore,sizeof(rr))!=0) return 22;
    const uint8_t bad[]={0xff}; arbor_span bin={bad,1};
    arbor_view0_native_diagnostic ud[4]={{0}}; arbor_view0_native_result ur={0};
    arbor_status us=arbor_view0_native_check(bin,ud,4,&ur);
    if(us.native!=0||ur.diagnostic_count!=1||ud[0].rule_id!=ARBOR_VIEW0_NATIVE_RULE_UTF8_INVALID) return 23;
    printf("VIEW0_V1N1_G05_R1A_DETERMINISM=PASS\n");
    printf("VIEW0_V1N1_G05_R1A_CAPACITY_FAILURE_ATOMICITY=PASS\n");
    printf("VIEW0_V1N1_G05_R1A_UTF8_PRECEDENCE=PASS\n");
    return 0;
}
