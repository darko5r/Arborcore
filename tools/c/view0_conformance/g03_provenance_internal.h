#ifndef ARBORCORE_VIEW0_CONFORMANCE_G03_PROVENANCE_INTERNAL_H
#define ARBORCORE_VIEW0_CONFORMANCE_G03_PROVENANCE_INTERNAL_H

#include <arborcore/view0_conformance/native.h>

#include <lexbor/html/token.h>
#include <lexbor/html/tree.h>

#include <stdbool.h>
#include <stdint.h>

typedef struct arbor_view0_native_g03_provenance_context {
    const uint8_t *input_data;
    uint64_t input_length;
    const lxb_char_t *current_begin;
    lxb_tag_id_t current_tag;
    bool current_start;
    bool current_assigned;
    bool wrapper_seen;
    bool insertion_wrapper_seen;
    bool failed;

    lxb_html_tree_t *source_repair_tree;
    const lxb_html_token_t *source_repair_token;
    arbor_view0_native_source_repair_context source_repair_record;
    bool source_repair_active;
} arbor_view0_native_g03_provenance_context;

uint64_t arbor_view0_native_g03_standard_element_id_from_lexbor(
    uintptr_t local_name,
    uintptr_t ns);

#endif
