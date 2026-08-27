#include <arborcore/view0_conformance/native.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct ctx { uint64_t n; arbor_view0_native_source_attribute_observation a[8]; char names[8][32]; } ctx;
static void need(int ok,const char*m){if(!ok){fprintf(stderr,"FAIL: %s\
",m);exit(1);}}
static arbor_status obs(void *v,const arbor_view0_native_source_attribute_observation *o){ctx*c=v;if(c->n>=8u){arbor_status s={.native=-1};return s;}c->a[c->n]=*o;size_t n=(size_t)o->local_name.length;if(n>=32u)n=31u;memcpy(c->names[c->n],o->local_name.data,n);c->names[c->n][n]=0;c->n++;arbor_status s={0};return s;}
int main(void){
 const char *html="<!doctype html><title>x</title><body><p  ID =\\\"x\\\" data-z='q' bogus=1>ok</p></body>";
 ctx c={0}; arbor_view0_native_semantic_observer o={0};o.context=&c;o.source_attribute=obs; arbor_view0_native_parse_counts pc={0};arbor_view0_native_observation_counts oc={0};
 arbor_span in={(const uint8_t*)html,(uint64_t)strlen(html)}; arbor_view0_native_document_facts facts={0}; arbor_status s=arbor_view0_native_lexbor_observe(in,&o,&pc,&facts,&oc);need(s.native==0,"collect");need(c.n==3u,"attr count");
 const char *want[3]={"id","data-z","bogus"}; const char *src[3]={"ID","data-z","bogus"};
 for(uint64_t i=0;i<3u;i++){need(strcmp(c.names[i],want[i])==0,"normalized attr name");need(c.a[i].source_length==strlen(src[i]),"source length");need(c.a[i].source_offset<in.length,"source offset bound");need(memcmp(in.data+c.a[i].source_offset,src[i],strlen(src[i]))==0,"source anchor bytes");need(c.a[i].owner_standard_element_id==ARBOR_VIEW0_NATIVE_ELEMENT_P,"owner p");}
 puts("VIEW0_V1N1_G05_C0_SOURCE_ATTRIBUTE_NAME_ANCHORS=3_OF_3");
 puts("PASS: VIEW0 V1N1 G05 C0 authored attribute-name source anchoring");return 0;
}
