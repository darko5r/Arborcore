#include <arborcore/view0_conformance/native.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static arbor_span s(const char *x){return (arbor_span){(const uint8_t*)x,(uint64_t)strlen(x)};}
static uint64_t count(const arbor_view0_native_diagnostic*d,uint64_t n,uint64_t id){uint64_t c=0;for(uint64_t i=0;i<n;i++)if(d[i].rule_id==id)c++;return c;}
static int expect(const char *html,uint64_t r1,uint64_t r4,uint64_t flags){
 arbor_view0_native_diagnostic d[64]={{0}}; arbor_view0_native_result r={0};
 arbor_status st=arbor_view0_native_check(s(html),d,64,&r); if(st.native!=0)return 1;
 if(count(d,r.diagnostic_count,ARBOR_VIEW_V1_G03_ELEMENT_CONTEXT)!=r1)return 2;
 if(count(d,r.diagnostic_count,ARBOR_VIEW_V1_G03_NOTHING_MODEL)!=r4)return 3;
 if((r.flags&ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R4_PARTIAL)==0)return 4;
 if((r.flags&flags)!=flags)return 5;
 return 0;
}
int main(void){
 if(expect("<!doctype html><title>x</title><body><iframe></iframe></body>",0,0,0))return 1;
 if(expect("<!doctype html><title>x</title><body><iframe> \n </iframe></body>",0,0,0))return 2;
 if(expect("<!doctype html><title>x</title><body><iframe>text</iframe></body>",0,1,0))return 3;
 if(expect("<!doctype html><title>x</title><body><select><option label=x value=y></option></select></body>",0,0,0))return 4;
 if(expect("<!doctype html><title>x</title><body><select><option label=x value=y>text</option></select></body>",0,1,0))return 5;
 if(expect("<!doctype html><title>x</title><body><select><option label=x>text</option></select></body>",0,0,0))return 6;
 if(expect("<!doctype html><title>x</title><body><table><colgroup span=2></colgroup></table></body>",0,0,0))return 7;
 if(expect("<!doctype html><title>x</title><body><table><colgroup span=2><col></colgroup></table></body>",1,0,0))return 8;
 if(expect("<!doctype html><title>x</title><body><br>text</br></body>",0,0,0))return 9;
 if(expect("<!doctype html><title>x</title><body><template><span>x</span></template></body>",0,0,0))return 10;
 if(expect("<!doctype html><title>x</title><body><select><button><selectedcontent></selectedcontent></button><option>x</option></select></body>",0,0,ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R4_DEFERRED_SELECTEDCONTENT_PROVENANCE))return 11;
 puts("PASS: VIEW0 V1N1 G03 R4A stable Nothing branches, vacuous subjects, R1 suppression and selectedcontent deferral");
 return 0;
}
