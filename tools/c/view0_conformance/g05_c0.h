#ifndef ARBORCORE_VIEW0_CONFORMANCE_G05_C0_H
#define ARBORCORE_VIEW0_CONFORMANCE_G05_C0_H

#include <arborcore/view0_conformance/native.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum arbor_view0_native_g05_c0_global_kind {
    ARBOR_VIEW0_NATIVE_G05_C0_GLOBAL_NONE = 0,
    ARBOR_VIEW0_NATIVE_G05_C0_GLOBAL_EXACT = 1,
    ARBOR_VIEW0_NATIVE_G05_C0_GLOBAL_DATA_FAMILY = 2,
    ARBOR_VIEW0_NATIVE_G05_C0_GLOBAL_ARIA_FAMILY = 3
} arbor_view0_native_g05_c0_global_kind;

#define ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_NONE UINT64_C(0)
#define ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_HIDDEN (UINT64_C(1) << 0)
#define ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TEXT (UINT64_C(1) << 1)
#define ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_SEARCH (UINT64_C(1) << 2)
#define ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TEL (UINT64_C(1) << 3)
#define ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_URL (UINT64_C(1) << 4)
#define ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_EMAIL (UINT64_C(1) << 5)
#define ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_PASSWORD (UINT64_C(1) << 6)
#define ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_DATE (UINT64_C(1) << 7)
#define ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_MONTH (UINT64_C(1) << 8)
#define ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_WEEK (UINT64_C(1) << 9)
#define ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TIME (UINT64_C(1) << 10)
#define ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_DATETIME_LOCAL (UINT64_C(1) << 11)
#define ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_NUMBER (UINT64_C(1) << 12)
#define ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_RANGE (UINT64_C(1) << 13)
#define ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_COLOR (UINT64_C(1) << 14)
#define ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_CHECKBOX (UINT64_C(1) << 15)
#define ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_RADIO (UINT64_C(1) << 16)
#define ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_FILE (UINT64_C(1) << 17)
#define ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_SUBMIT (UINT64_C(1) << 18)
#define ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_IMAGE (UINT64_C(1) << 19)
#define ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_RESET (UINT64_C(1) << 20)
#define ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_BUTTON (UINT64_C(1) << 21)
#define ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_ALL_MASK ((UINT64_C(1) << 22) - UINT64_C(1))

typedef struct arbor_view0_native_g05_c0_conditional_meta {
    uint16_t standard_element_id;
    uint16_t predicate_family_id;
    uint64_t input_state_mask;
    const char *input_state_suffix;
    const char *clause_sha256;
} arbor_view0_native_g05_c0_conditional_meta;

uint64_t arbor_view0_native_g05_c0_global_catalog_count(void);
uint64_t arbor_view0_native_g05_c0_global_exact_count(void);
uint64_t arbor_view0_native_g05_c0_global_prefix_count(void);
uint64_t arbor_view0_native_g05_c0_element_attribute_count(void);
uint64_t arbor_view0_native_g05_c0_body_window_event_count(void);
uint64_t arbor_view0_native_g05_c0_conditional_count(void);

bool arbor_view0_native_g05_c0_global_attribute_classify(
    arbor_span local_name, arbor_view0_native_g05_c0_global_kind *kind_out);
bool arbor_view0_native_g05_c0_element_attribute_listed(
    uint64_t standard_element_id, arbor_span local_name);
bool arbor_view0_native_g05_c0_body_window_event_listed(arbor_span local_name);
bool arbor_view0_native_g05_c0_known_non_global_attribute_name(arbor_span local_name);
const arbor_view0_native_g05_c0_conditional_meta *
arbor_view0_native_g05_c0_conditional_at(uint64_t index);
uint64_t arbor_view0_native_g05_c0_input_state_from_type(arbor_span value);

#endif
