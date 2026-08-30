#include "v1n2_c0.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void need(bool ok, const char *message) {
    if (!ok) {
        (void)fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

int main(void) {
    static const uint16_t expected_groups[] = {7u, 8u, 9u, 10u, 11u};
    static const uint16_t expected_counts[] = {5u, 12u, 6u, 13u, 2u};
    uint64_t group_counts[5] = {0};
    uint64_t admission_counts[3] = {0};
    uint64_t disposition_counts[6] = {0};
    uint64_t frozen_sources = 0u;
    uint64_t i;

    need(arbor_view0_native_v1n2_c0_validate(), "frozen C0 tables validate");
    need(arbor_view0_native_v1n2_c0_rule_count() == 38u, "38 frozen rules");
    need(arbor_view0_native_v1n2_c0_authority_count() == 12u,
         "12 authority closure rows");
    need(ARBOR_VIEW0_NATIVE_V1N2_C0_SHARED_ANCHOR_CAP == 4096u,
         "one retained diagnostic-cap anchor arena");
    need(sizeof(arbor_view0_native_v1n2_anchor) == 40u,
         "shared anchor layout");

    for (i = 0u; i < arbor_view0_native_v1n2_c0_rule_count(); ++i) {
        const arbor_view0_native_v1n2_rule_meta *rule =
            arbor_view0_native_v1n2_c0_rule_at(i);
        need(rule != NULL, "rule metadata present");
        const uint64_t group_index = (uint64_t)(rule->group - 7u);
        const uint64_t expected_id = UINT64_C(0x0000000030000000) |
            ((uint64_t)rule->group << 16u) | (uint64_t)rule->group_ordinal;
        need(group_index < 5u, "rule group bounded");
        need(rule->rule_id == expected_id, "rule identity series");
        need(strlen(rule->rule_symbol) > 20u, "rule symbol present");
        need(strlen(rule->source_slice_sha256) == 64u,
             "rule source fingerprint present");
        need(rule->admission >= 1u && rule->admission <= 3u,
             "rule admission bounded");
        ++group_counts[group_index];
        ++admission_counts[rule->admission - 1u];
    }
    need(arbor_view0_native_v1n2_c0_rule_at(38u) == NULL,
         "rule table upper bound");
    for (i = 0u; i < 5u; ++i) {
        need(group_counts[i] == expected_counts[i], "exact group rule count");
        need(arbor_view0_native_v1n2_c0_group_rule_count(expected_groups[i]) ==
                 expected_counts[i],
             "group count query");
    }
    need(arbor_view0_native_v1n2_c0_group_rule_count(6u) == 0u,
         "non-V1N2 group rejected");
    need(admission_counts[0] == 36u && admission_counts[1] == 1u &&
             admission_counts[2] == 1u,
         "exact 36/1/1 admission partition");

    for (i = 0u; i < arbor_view0_native_v1n2_c0_authority_count(); ++i) {
        const arbor_view0_native_v1n2_authority_meta *authority =
            arbor_view0_native_v1n2_c0_authority_at(i);
        need(authority != NULL && authority->dependency != NULL,
             "authority metadata present");
        need(authority->consuming_group_mask != 0u &&
                 (authority->consuming_group_mask & UINT16_C(0xffe0)) == 0u,
             "authority consumer mask bounded");
        need(authority->disposition >= 1u && authority->disposition <= 6u,
             "authority disposition bounded");
        ++disposition_counts[authority->disposition - 1u];
        if (authority->source_sha256 != NULL) {
            need(authority->selected_key != NULL &&
                     strlen(authority->selected_commit) == 40u &&
                     strlen(authority->source_sha256) == 64u,
                 "frozen authority source identity");
            ++frozen_sources;
        }
    }
    need(arbor_view0_native_v1n2_c0_authority_at(12u) == NULL,
         "authority table upper bound");
    need(frozen_sources == 8u, "eight direct frozen authority rows");
    need(disposition_counts[0] == 6u && disposition_counts[1] == 1u &&
             disposition_counts[2] == 1u && disposition_counts[3] == 2u &&
             disposition_counts[4] == 1u && disposition_counts[5] == 1u,
         "exact authority disposition partition");

    puts("VIEW0_V1N2_C0_RULE_IDENTITIES=38_OF_38");
    puts("VIEW0_V1N2_C0_GROUP_COUNTS=G07_5_G08_12_G09_6_G10_13_G11_2");
    puts("VIEW0_V1N2_C0_ADMISSIONS=STATIC_36_HTML_INTEGRATION_1_DETERMINISTIC_SUBSET_1");
    puts("VIEW0_V1N2_C0_AUTHORITY_ROWS=12_OF_12");
    puts("VIEW0_V1N2_C0_SHARED_ANCHOR_ARENAS=1");
    puts("VIEW0_V1N2_C0_DIAGNOSTIC_RULES_IMPLEMENTED=ZERO");
    puts("PASS: VIEW0 V1N2 C0 zero-diagnostic implementation foundation");
    return 0;
}
