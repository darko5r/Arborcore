#include "g05_c0.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void need(int ok, const char *m){if(!ok){fprintf(stderr,"FAIL: %s\
",m);exit(1);}}
static arbor_span sp(const char *s){arbor_span x={(const uint8_t*)s,(uint64_t)strlen(s)};return x;}
int main(void){
 arbor_view0_native_g05_c0_global_kind k=0;
 need(arbor_view0_native_g05_c0_global_catalog_count()==106u,"global catalog count");
 need(arbor_view0_native_g05_c0_global_exact_count()==104u,"global exact count");
 need(arbor_view0_native_g05_c0_global_prefix_count()==2u,"global prefix count");
 need(arbor_view0_native_g05_c0_element_attribute_count()==261u,"element catalog count");
 need(arbor_view0_native_g05_c0_body_window_event_count()==18u,"body event count");
 need(arbor_view0_native_g05_c0_conditional_count()==43u,"conditional count");
 need(arbor_view0_native_g05_c0_global_attribute_classify(sp("id"),&k) && k==ARBOR_VIEW0_NATIVE_G05_C0_GLOBAL_EXACT,"id global");
 need(arbor_view0_native_g05_c0_global_attribute_classify(sp("onclick"),&k) && k==ARBOR_VIEW0_NATIVE_G05_C0_GLOBAL_EXACT,"onclick global");
 need(arbor_view0_native_g05_c0_global_attribute_classify(sp("data-x"),&k) && k==ARBOR_VIEW0_NATIVE_G05_C0_GLOBAL_DATA_FAMILY,"data family");
 need(!arbor_view0_native_g05_c0_global_attribute_classify(sp("data-"),&k),"data empty suffix rejected by family lookup");
 need(arbor_view0_native_g05_c0_global_attribute_classify(sp("aria-label"),&k) && k==ARBOR_VIEW0_NATIVE_G05_C0_GLOBAL_ARIA_FAMILY,"aria family");
 need(!arbor_view0_native_g05_c0_global_attribute_classify(sp("bogus"),&k),"bogus not global");
 need(arbor_view0_native_g05_c0_element_attribute_listed(ARBOR_VIEW0_NATIVE_ELEMENT_A,sp("href")),"a href listed");
 need(!arbor_view0_native_g05_c0_element_attribute_listed(ARBOR_VIEW0_NATIVE_ELEMENT_P,sp("href")),"p href not listed");
 need(arbor_view0_native_g05_c0_body_window_event_listed(sp("ononline")),"body ononline listed");
 need(!arbor_view0_native_g05_c0_body_window_event_listed(sp("onclick")),"generic onclick not body-extra catalog");
 need(arbor_view0_native_g05_c0_input_state_from_type(sp("HIDDEN"))==ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_HIDDEN,"shared hidden input state");
 need(arbor_view0_native_g05_c0_input_state_from_type(sp("checkbox"))==ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_CHECKBOX,"shared checkbox input state");
 need(arbor_view0_native_g05_c0_input_state_from_type(sp("invalid"))==ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TEXT,"shared invalid input default");
 for(uint64_t i=0;i<43u;i++){const arbor_view0_native_g05_c0_conditional_meta *m=arbor_view0_native_g05_c0_conditional_at(i); need(m!=NULL,"conditional entry"); need(strlen(m->clause_sha256)==64u,"conditional hash");}
 need(arbor_view0_native_g05_c0_conditional_at(43u)==NULL,"conditional oob");
 puts("VIEW0_V1N1_G05_C0_GLOBAL_CATALOG=106");
 puts("VIEW0_V1N1_G05_C0_ELEMENT_ATTRIBUTE_CATALOG=261");
 puts("VIEW0_V1N1_G05_C0_BODY_WINDOW_EVENT_CATALOG=18");
 puts("VIEW0_V1N1_G05_C0_CONDITIONAL_PREDICATES=43");
 puts("VIEW0_V1N1_G05_C0_SHARED_INPUT_STATE_CLASSIFIER=22_STATES_TEXT_DEFAULT");
 puts("PASS: VIEW0 V1N1 G05 C0 authority-generated applicability foundation catalogs");
 return 0;
}
