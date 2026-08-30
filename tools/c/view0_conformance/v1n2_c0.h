#ifndef ARBORCORE_VIEW0_CONFORMANCE_V1N2_C0_H
#define ARBORCORE_VIEW0_CONFORMANCE_V1N2_C0_H

#include <stdbool.h>
#include <stdint.h>

#define ARBOR_VIEW0_NATIVE_V1N2_C0_RULE_COUNT UINT64_C(38)
#define ARBOR_VIEW0_NATIVE_V1N2_C0_GROUP_COUNT UINT64_C(5)
#define ARBOR_VIEW0_NATIVE_V1N2_C0_AUTHORITY_COUNT UINT64_C(12)
#define ARBOR_VIEW0_NATIVE_V1N2_C0_SHARED_ANCHOR_CAP UINT64_C(4096)

#define ARBOR_VIEW0_NATIVE_V1N2_GROUP_MASK_G07 UINT16_C(0x0001)
#define ARBOR_VIEW0_NATIVE_V1N2_GROUP_MASK_G08 UINT16_C(0x0002)
#define ARBOR_VIEW0_NATIVE_V1N2_GROUP_MASK_G09 UINT16_C(0x0004)
#define ARBOR_VIEW0_NATIVE_V1N2_GROUP_MASK_G10 UINT16_C(0x0008)
#define ARBOR_VIEW0_NATIVE_V1N2_GROUP_MASK_G11 UINT16_C(0x0010)

typedef enum arbor_view0_native_v1n2_group {
    ARBOR_VIEW0_NATIVE_V1N2_GROUP_G07 = 7,
    ARBOR_VIEW0_NATIVE_V1N2_GROUP_G08 = 8,
    ARBOR_VIEW0_NATIVE_V1N2_GROUP_G09 = 9,
    ARBOR_VIEW0_NATIVE_V1N2_GROUP_G10 = 10,
    ARBOR_VIEW0_NATIVE_V1N2_GROUP_G11 = 11
} arbor_view0_native_v1n2_group;

typedef enum arbor_view0_native_v1n2_admission {
    ARBOR_VIEW0_NATIVE_V1N2_ADMISSION_STATIC_DOCUMENT_ONLY = 1,
    ARBOR_VIEW0_NATIVE_V1N2_ADMISSION_STATIC_HTML_INTEGRATION_ONLY = 2,
    ARBOR_VIEW0_NATIVE_V1N2_ADMISSION_STATIC_DETERMINISTIC_SUBSET_ONLY = 3
} arbor_view0_native_v1n2_admission;

typedef enum arbor_view0_native_v1n2_authority_disposition {
    ARBOR_VIEW0_NATIVE_V1N2_AUTHORITY_FROZEN_AT_HTML_COMMIT_TIME = 1,
    ARBOR_VIEW0_NATIVE_V1N2_AUTHORITY_FROZEN_RETAINED_G06_A2 = 2,
    ARBOR_VIEW0_NATIVE_V1N2_AUTHORITY_FROZEN_WITH_CSS_SYNTAX_SUPPORT = 3,
    ARBOR_VIEW0_NATIVE_V1N2_AUTHORITY_EXPLICITLY_DEFERRED = 4,
    ARBOR_VIEW0_NATIVE_V1N2_AUTHORITY_HTML_INTEGRATION_ONLY = 5,
    ARBOR_VIEW0_NATIVE_V1N2_AUTHORITY_EXPLICITLY_DEFERRED_TO_G16 = 6
} arbor_view0_native_v1n2_authority_disposition;

typedef enum arbor_view0_native_v1n2_anchor_kind {
    ARBOR_VIEW0_NATIVE_V1N2_ANCHOR_ELEMENT = 1,
    ARBOR_VIEW0_NATIVE_V1N2_ANCHOR_ATTRIBUTE_NAME = 2,
    ARBOR_VIEW0_NATIVE_V1N2_ANCHOR_ATTRIBUTE_VALUE = 3,
    ARBOR_VIEW0_NATIVE_V1N2_ANCHOR_TEXT = 4,
    ARBOR_VIEW0_NATIVE_V1N2_ANCHOR_IDREF_TOKEN = 5
} arbor_view0_native_v1n2_anchor_kind;

typedef struct arbor_view0_native_v1n2_rule_meta {
    uint64_t rule_id;
    const char *rule_symbol;
    uint16_t group;
    uint16_t group_ordinal;
    uint16_t admission;
    uint16_t reserved;
    const char *source_slice_sha256;
} arbor_view0_native_v1n2_rule_meta;

typedef struct arbor_view0_native_v1n2_authority_meta {
    const char *dependency;
    uint16_t consuming_group_mask;
    uint16_t disposition;
    const char *selected_key;
    const char *selected_commit;
    const char *source_sha256;
    const char *support_source_sha256;
} arbor_view0_native_v1n2_authority_meta;

/* One combined G07-G11 staging record. Five maximum-sized group arrays are forbidden. */
typedef struct arbor_view0_native_v1n2_anchor {
    uint64_t byte_offset;
    uint64_t source_length;
    uint64_t discovery_sequence;
    uint64_t subject_index;
    uint16_t group_ordinal;
    uint16_t rule_ordinal;
    uint16_t kind;
    uint16_t reserved;
} arbor_view0_native_v1n2_anchor;

_Static_assert(sizeof(arbor_view0_native_v1n2_anchor) == 40u,
               "V1N2 shared anchor layout drift");

uint64_t arbor_view0_native_v1n2_c0_rule_count(void);
const arbor_view0_native_v1n2_rule_meta *
arbor_view0_native_v1n2_c0_rule_at(uint64_t index);
uint64_t arbor_view0_native_v1n2_c0_group_rule_count(uint16_t group);
uint64_t arbor_view0_native_v1n2_c0_authority_count(void);
const arbor_view0_native_v1n2_authority_meta *
arbor_view0_native_v1n2_c0_authority_at(uint64_t index);
bool arbor_view0_native_v1n2_c0_validate(void);

#endif
