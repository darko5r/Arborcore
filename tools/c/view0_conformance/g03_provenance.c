#include "g03_provenance_internal.h"

#include <lexbor/dom/interfaces/node.h>
#include <lexbor/html/interface.h>
#include <lexbor/html/interfaces/document.h>
#include <lexbor/html/tree/insertion_mode.h>

#include <errno.h>
#include <stdint.h>

lxb_dom_interface_t *__real_lxb_html_interface_create(
    lxb_html_document_t *document,
    lxb_tag_id_t tag_id,
    lxb_ns_id_t ns);

lxb_dom_interface_t *__wrap_lxb_html_interface_create(
    lxb_html_document_t *document,
    lxb_tag_id_t tag_id,
    lxb_ns_id_t ns);

lxb_html_element_t *__real_lxb_html_tree_insert_foreign_element(
    lxb_html_tree_t *tree,
    lxb_html_token_t *token,
    lxb_ns_id_t ns,
    bool only_add_stack);

lxb_html_element_t *__wrap_lxb_html_tree_insert_foreign_element(
    lxb_html_tree_t *tree,
    lxb_html_token_t *token,
    lxb_ns_id_t ns,
    bool only_add_stack);

static arbor_status sr_status_errno(int value)
{
    return arbor_status_from_native(-(int64_t)value);
}

static bool sr_pointer_offset(
    const arbor_view0_native_g03_provenance_context *context,
    const lxb_char_t *pointer,
    uint64_t *offset_out)
{
    if (context == NULL || pointer == NULL || offset_out == NULL ||
        context->input_data == NULL) {
        return false;
    }

    const uintptr_t base = (uintptr_t)context->input_data;
    const uintptr_t value = (uintptr_t)pointer;
    if (context->input_length > (uint64_t)(UINTPTR_MAX - base)) {
        return false;
    }
    const uintptr_t end = base + (uintptr_t)context->input_length;
    if (value < base || value > end) {
        return false;
    }

    *offset_out = (uint64_t)(value - base);
    return true;
}

static arbor_status sr_node_source_offset(
    const arbor_view0_native_g03_provenance_context *context,
    const lxb_dom_node_t *node,
    uint64_t *offset_out)
{
    if (context == NULL || offset_out == NULL) {
        return sr_status_errno(EINVAL);
    }
    if (node == NULL || node->user == NULL) {
        *offset_out = ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE;
        return arbor_status_from_native(0);
    }

    uint64_t offset = 0u;
    if (!sr_pointer_offset(context, (const lxb_char_t *)node->user, &offset)) {
        return sr_status_errno(EIO);
    }
    *offset_out = offset;
    return arbor_status_from_native(0);
}

static uint64_t sr_mode_id(lxb_html_tree_insertion_mode_f mode)
{
    if (mode == lxb_html_tree_insertion_mode_initial) return ARBOR_VIEW0_NATIVE_INSERTION_MODE_INITIAL;
    if (mode == lxb_html_tree_insertion_mode_before_html) return ARBOR_VIEW0_NATIVE_INSERTION_MODE_BEFORE_HTML;
    if (mode == lxb_html_tree_insertion_mode_before_head) return ARBOR_VIEW0_NATIVE_INSERTION_MODE_BEFORE_HEAD;
    if (mode == lxb_html_tree_insertion_mode_in_head) return ARBOR_VIEW0_NATIVE_INSERTION_MODE_IN_HEAD;
    if (mode == lxb_html_tree_insertion_mode_in_head_noscript) return ARBOR_VIEW0_NATIVE_INSERTION_MODE_IN_HEAD_NOSCRIPT;
    if (mode == lxb_html_tree_insertion_mode_after_head) return ARBOR_VIEW0_NATIVE_INSERTION_MODE_AFTER_HEAD;
    if (mode == lxb_html_tree_insertion_mode_in_body) return ARBOR_VIEW0_NATIVE_INSERTION_MODE_IN_BODY;
    if (mode == lxb_html_tree_insertion_mode_in_body_skip_new_line) return ARBOR_VIEW0_NATIVE_INSERTION_MODE_IN_BODY_SKIP_NEW_LINE;
    if (mode == lxb_html_tree_insertion_mode_in_body_skip_new_line_textarea) return ARBOR_VIEW0_NATIVE_INSERTION_MODE_IN_BODY_SKIP_NEW_LINE_TEXTAREA;
    if (mode == lxb_html_tree_insertion_mode_text) return ARBOR_VIEW0_NATIVE_INSERTION_MODE_TEXT;
    if (mode == lxb_html_tree_insertion_mode_in_table) return ARBOR_VIEW0_NATIVE_INSERTION_MODE_IN_TABLE;
    if (mode == lxb_html_tree_insertion_mode_in_table_text) return ARBOR_VIEW0_NATIVE_INSERTION_MODE_IN_TABLE_TEXT;
    if (mode == lxb_html_tree_insertion_mode_in_caption) return ARBOR_VIEW0_NATIVE_INSERTION_MODE_IN_CAPTION;
    if (mode == lxb_html_tree_insertion_mode_in_column_group) return ARBOR_VIEW0_NATIVE_INSERTION_MODE_IN_COLUMN_GROUP;
    if (mode == lxb_html_tree_insertion_mode_in_table_body) return ARBOR_VIEW0_NATIVE_INSERTION_MODE_IN_TABLE_BODY;
    if (mode == lxb_html_tree_insertion_mode_in_row) return ARBOR_VIEW0_NATIVE_INSERTION_MODE_IN_ROW;
    if (mode == lxb_html_tree_insertion_mode_in_cell) return ARBOR_VIEW0_NATIVE_INSERTION_MODE_IN_CELL;
    if (mode == lxb_html_tree_insertion_mode_in_template) return ARBOR_VIEW0_NATIVE_INSERTION_MODE_IN_TEMPLATE;
    if (mode == lxb_html_tree_insertion_mode_after_body) return ARBOR_VIEW0_NATIVE_INSERTION_MODE_AFTER_BODY;
    if (mode == lxb_html_tree_insertion_mode_in_frameset) return ARBOR_VIEW0_NATIVE_INSERTION_MODE_IN_FRAMESET;
    if (mode == lxb_html_tree_insertion_mode_after_frameset) return ARBOR_VIEW0_NATIVE_INSERTION_MODE_AFTER_FRAMESET;
    if (mode == lxb_html_tree_insertion_mode_after_after_body) return ARBOR_VIEW0_NATIVE_INSERTION_MODE_AFTER_AFTER_BODY;
    if (mode == lxb_html_tree_insertion_mode_after_after_frameset) return ARBOR_VIEW0_NATIVE_INSERTION_MODE_AFTER_AFTER_FRAMESET;
    return ARBOR_VIEW0_NATIVE_INSERTION_MODE_NONE;
}

static arbor_status sr_current_context(
    arbor_view0_native_g03_provenance_context *context,
    lxb_html_tree_t *tree,
    uint64_t *element_id_out,
    uint64_t *source_offset_out,
    uint64_t *mode_id_out,
    uint64_t *depth_out)
{
    if (context == NULL || tree == NULL || element_id_out == NULL ||
        source_offset_out == NULL || mode_id_out == NULL || depth_out == NULL ||
        tree->open_elements == NULL) {
        return sr_status_errno(EINVAL);
    }

    const uint64_t mode_id = sr_mode_id(tree->mode);
    if (mode_id == ARBOR_VIEW0_NATIVE_INSERTION_MODE_NONE) {
        return sr_status_errno(EIO);
    }

    lxb_dom_node_t *node = lxb_html_tree_current_node(tree);
    *element_id_out = node == NULL
        ? ARBOR_VIEW0_NATIVE_ELEMENT_NONE
        : arbor_view0_native_g03_standard_element_id_from_lexbor(
            node->local_name, node->ns);
    arbor_status status = sr_node_source_offset(context, node, source_offset_out);
    if (status.native != 0) {
        return status;
    }
    if (tree->open_elements->length > UINT64_MAX) {
        return sr_status_errno(EOVERFLOW);
    }
    *mode_id_out = mode_id;
    *depth_out = (uint64_t)tree->open_elements->length;
    return arbor_status_from_native(0);
}

lxb_dom_interface_t *__wrap_lxb_html_interface_create(
    lxb_html_document_t *document,
    lxb_tag_id_t tag_id,
    lxb_ns_id_t ns)
{
    lxb_dom_interface_t *interface =
        __real_lxb_html_interface_create(document, tag_id, ns);
    if (interface == NULL || document == NULL) {
        return interface;
    }

    arbor_view0_native_g03_provenance_context *context =
        (arbor_view0_native_g03_provenance_context *)document->dom_document.user;
    if (context == NULL) {
        return interface;
    }

    context->wrapper_seen = true;
    if (context->current_start && !context->current_assigned &&
        tag_id == context->current_tag) {
        lxb_dom_node_t *node = lxb_dom_interface_node(interface);
        if (node == NULL || context->current_begin == NULL) {
            context->failed = true;
            return interface;
        }
        node->user = (void *)context->current_begin;
        context->current_assigned = true;
    }

    return interface;
}

lxb_html_element_t *__wrap_lxb_html_tree_insert_foreign_element(
    lxb_html_tree_t *tree,
    lxb_html_token_t *token,
    lxb_ns_id_t ns,
    bool only_add_stack)
{
    arbor_view0_native_g03_provenance_context *context = NULL;
    if (tree != NULL && tree->document != NULL) {
        context = (arbor_view0_native_g03_provenance_context *)
            tree->document->dom_document.user;
    }
    if (context != NULL) {
        context->insertion_wrapper_seen = true;
    }

    bool correlated = false;
    uint64_t current_id = 0u;
    uint64_t current_offset = ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE;
    uint64_t mode_id = 0u;
    uint64_t depth = 0u;
    uint64_t flags = 0u;

    if (context != NULL && context->source_repair_active &&
        context->source_repair_tree == tree &&
        context->source_repair_token == token) {
        uint64_t begin = 0u;
        uint64_t end = 0u;
        if (token == NULL ||
            !sr_pointer_offset(context, token->begin, &begin) ||
            !sr_pointer_offset(context, token->end, &end) || end < begin ||
            begin != context->source_repair_record.source_offset ||
            end - begin != context->source_repair_record.source_length ||
            context->source_repair_record.insertion_seen != 0u) {
            context->failed = true;
        } else {
            arbor_status status = sr_current_context(
                context, tree, &current_id, &current_offset, &mode_id, &depth);
            if (status.native != 0) {
                context->failed = true;
            } else {
                flags = tree->foster_parenting
                    ? ARBOR_VIEW0_NATIVE_SOURCE_REPAIR_FLAG_FOSTER_PARENTING
                    : 0u;
                correlated = true;
            }
        }
    }

    lxb_html_element_t *element = __real_lxb_html_tree_insert_foreign_element(
        tree, token, ns, only_add_stack);

    if (correlated && context != NULL && !context->failed && element != NULL) {
        context->source_repair_record.insertion_seen = 1u;
        context->source_repair_record.insertion_current_standard_element_id = current_id;
        context->source_repair_record.insertion_current_source_offset = current_offset;
        context->source_repair_record.insertion_mode_id = mode_id;
        context->source_repair_record.insertion_open_elements_depth = depth;
        context->source_repair_record.insertion_flags = flags;
    }

    return element;
}
