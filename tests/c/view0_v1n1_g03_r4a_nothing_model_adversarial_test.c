#include <arborcore/view0_conformance/native.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static arbor_span s(const char*x){return (arbor_span){(const uint8_t*)x,(uint64_t)strlen(x)};}
static uint64_t count(const arbor_view0_native_diagnostic*d,uint64_t n,uint64_t id){uint64_t c=0;for(uint64_t i=0;i<n;i++)if(d[i].rule_id==id)c++;return c;}
int main(void){
 static const char html[]="<!doctype html><title>x</title><body><iframe>a</iframe><iframe>b</iframe></body>";
 arbor_view0_native_diagnostic ample[32]={{0}}; arbor_view0_native_result ar={0}; arbor_status st=arbor_view0_native_check(s(html),ample,32,&ar);
 if(st.native!=0||count(ample,ar.diagnostic_count,ARBOR_VIEW_V1_G03_NOTHING_MODEL)!=2)return 1;
 arbor_view0_native_diagnostic small[32],before[32]; memset(small,0x5a,sizeof small); memcpy(before,small,sizeof small);
 arbor_view0_native_result rr={1,2,3,4},rb=rr; st=arbor_view0_native_check(s(html),small,ar.diagnostic_count-1,&rr);
 if(st.native!=-(int64_t)ENOSPC||memcmp(small,before,sizeof small)||memcmp(&rr,&rb,sizeof rr))return 2;
 arbor_view0_native_diagnostic exact[32]={{0}}; arbor_view0_native_result er={0}; st=arbor_view0_native_check(s(html),exact,ar.diagnostic_count,&er);
 if(st.native!=0||memcmp(&ar,&er,sizeof ar)||memcmp(ample,exact,(size_t)(ar.diagnostic_count*sizeof exact[0])))return 3;
 static const char sup[]="<!doctype html><title>x</title><body><table><colgroup span=2><col></colgroup></table></body>";
 arbor_view0_native_diagnostic sd[16]={{0}}; arbor_view0_native_result sr={0}; st=arbor_view0_native_check(s(sup),sd,16,&sr);
 if(st.native!=0||count(sd,sr.diagnostic_count,ARBOR_VIEW_V1_G03_ELEMENT_CONTEXT)!=1||count(sd,sr.diagnostic_count,ARBOR_VIEW_V1_G03_NOTHING_MODEL)!=0)return 4;
 static const char def[]="<!doctype html><title>x</title><body><select><button><selectedcontent>authored</selectedcontent></button><option>x</option></select></body>";
 arbor_view0_native_diagnostic dd[32]={{0}}; arbor_view0_native_result dr={0}; st=arbor_view0_native_check(s(def),dd,32,&dr);
 if(st.native!=0||(dr.flags&ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R4_DEFERRED_SELECTEDCONTENT_PROVENANCE)==0||count(dd,dr.diagnostic_count,ARBOR_VIEW_V1_G03_NOTHING_MODEL)!=0)return 5;
 static const uint8_t bad[]={ '<','!','d','o','c','t','y','p','e',' ','h','t','m','l','>','<','i','f','r','a','m','e','>',0xf0,0x80,0x80,0x80};
 arbor_view0_native_diagnostic ud[8]={{0}}; arbor_view0_native_result ur={0}; st=arbor_view0_native_check((arbor_span){bad,sizeof bad},ud,8,&ur);
 if(st.native!=0||ur.diagnostic_count!=1||ud[0].origin!=ARBOR_VIEW0_NATIVE_ORIGIN_UTF8||count(ud,ur.diagnostic_count,ARBOR_VIEW_V1_G03_NOTHING_MODEL)!=0)return 6;
 puts("PASS: VIEW0 V1N1 G03 R4A capacity atomicity, determinism, R1 suppression, deferral and UTF-8 precedence"); return 0;
}
