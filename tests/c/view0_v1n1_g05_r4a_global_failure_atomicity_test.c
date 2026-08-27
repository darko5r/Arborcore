#include <arborcore/view0_conformance/native.h>
#include "g05_r4a.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <lexbor/html/parser.h>

typedef enum injection_mode { INJECTION_NONE=0, INJECTION_R4_ANCHOR_FAILURE=1, INJECTION_FINAL_LEXBOR_PARSER_FAILURE=2 } injection_mode;
static injection_mode current_mode=INJECTION_NONE;
static bool inside_exact=false;

arbor_status __real_arbor_view0_native_g05_r4a_collect_anchors(arbor_span,arbor_view0_native_source_anchor*,uint64_t,arbor_view0_native_g05_r4a_evaluation*);
arbor_status __wrap_arbor_view0_native_g05_r4a_collect_anchors(arbor_span,arbor_view0_native_source_anchor*,uint64_t,arbor_view0_native_g05_r4a_evaluation*);
arbor_status __wrap_arbor_view0_native_g05_r4a_collect_anchors(arbor_span input,arbor_view0_native_source_anchor *anchors,uint64_t cap,arbor_view0_native_g05_r4a_evaluation *out)
{
    if (current_mode==INJECTION_R4_ANCHOR_FAILURE) return arbor_status_from_native(-(int64_t)ENOMEM);
    return __real_arbor_view0_native_g05_r4a_collect_anchors(input,anchors,cap,out);
}

arbor_status __real_arbor_view0_native_lexbor_collect_exact(arbor_span,arbor_view0_native_diagnostic*,uint64_t,const arbor_view0_native_parse_counts*,const arbor_view0_native_document_facts*);
arbor_status __wrap_arbor_view0_native_lexbor_collect_exact(arbor_span,arbor_view0_native_diagnostic*,uint64_t,const arbor_view0_native_parse_counts*,const arbor_view0_native_document_facts*);
arbor_status __wrap_arbor_view0_native_lexbor_collect_exact(arbor_span input,arbor_view0_native_diagnostic *d,uint64_t cap,const arbor_view0_native_parse_counts *counts,const arbor_view0_native_document_facts *facts)
{
    const bool prev=inside_exact; inside_exact=true;
    const arbor_status st=__real_arbor_view0_native_lexbor_collect_exact(input,d,cap,counts,facts);
    inside_exact=prev; return st;
}
lxb_html_parser_t *__real_lxb_html_parser_create(void);
lxb_html_parser_t *__wrap_lxb_html_parser_create(void);
lxb_html_parser_t *__wrap_lxb_html_parser_create(void)
{
    if (current_mode==INJECTION_FINAL_LEXBOR_PARSER_FAILURE && inside_exact) return NULL;
    return __real_lxb_html_parser_create();
}

static int unchanged(injection_mode mode)
{
    static const char html[]="<!doctype html><title>x</title><body><p onpageshow=\"x()\"></p></body>";
    arbor_view0_native_diagnostic d[64], before[64]; arbor_view0_native_result r, rb;
    (void)memset(d,0x6a,sizeof(d)); (void)memcpy(before,d,sizeof(d)); (void)memset(&r,0xa6,sizeof(r)); rb=r;
    current_mode=mode;
    const arbor_status st=arbor_view0_native_check((arbor_span){(const uint8_t*)html,(uint64_t)(sizeof(html)-1u)},d,64u,&r);
    current_mode=INJECTION_NONE;
    if (st.native!=-(int64_t)ENOMEM) return 1;
    if (memcmp(d,before,sizeof(d))!=0 || memcmp(&r,&rb,sizeof(r))!=0) return 2;
    return 0;
}

static int equivalence(void)
{
    static const char html[]="<!doctype html><title>x</title><body><p onpageshow=\"x()\"></p></body>";
    const arbor_span in={(const uint8_t*)html,(uint64_t)(sizeof(html)-1u)};
    arbor_view0_native_g05_r4a_evaluation m={0},c={0}; arbor_view0_native_source_anchor a={0};
    if (arbor_view0_native_g05_r4a_measure(in,&m).native!=0 || m.diagnostic_count!=1u) return 1;
    if (arbor_view0_native_g05_r4a_collect_anchors(in,&a,1u,&c).native!=0 || memcmp(&m,&c,sizeof(m))!=0) return 2;
    const char *q=strstr(html,"onpageshow"); if(q==NULL || a.byte_offset!=(uint32_t)(q-html) || a.source_length!=10u) return 3;
    arbor_view0_native_diagnostic d={0}; arbor_view0_native_g05_r4a_materialize_anchor(&a,91u,&d);
    if(d.rule_id!=ARBOR_VIEW_V1_G05_BODY_WINDOW_EVENT_ATTRIBUTE_APPLICABILITY || d.byte_offset!=(uint64_t)(q-html) || d.source_length!=10u || d.discovery_sequence!=91u) return 4;
    return 0;
}

int main(void)
{
    int rc=unchanged(INJECTION_R4_ANCHOR_FAILURE); if(rc!=0) return 10+rc;
    rc=unchanged(INJECTION_FINAL_LEXBOR_PARSER_FAILURE); if(rc!=0) return 20+rc;
    rc=equivalence(); if(rc!=0) return 30+rc;
    (void)puts("VIEW0_V1N1_G05_R4A_ANCHOR_FAILURE_ATOMICITY=PASS");
    (void)puts("VIEW0_V1N1_G05_R4A_FINAL_LEXBOR_FAILURE_ATOMICITY=PASS");
    (void)puts("VIEW0_V1N1_G05_R4A_ANCHOR_EQUIVALENCE=PASS");
    (void)puts("PASS: G05 R4A global mechanism failure atomicity");
    return 0;
}
