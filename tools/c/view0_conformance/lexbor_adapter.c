#include <arborcore/view0_conformance/native.h>

#include "g03_provenance_internal.h"

#include <lexbor/core/array_obj.h>
#include <lexbor/dom/interface.h>
#include <lexbor/dom/interfaces/attr.h>
#include <lexbor/dom/interfaces/document.h>
#include <lexbor/dom/interfaces/element.h>
#include <lexbor/dom/interfaces/text.h>
#include <lexbor/html/interfaces/document.h>
#include <lexbor/html/parser.h>
#include <lexbor/html/token.h>
#include <lexbor/html/tokenizer.h>
#include <lexbor/html/tokenizer/error.h>
#include <lexbor/html/tree/error.h>
#include <lexbor/html/tree/insertion_mode.h>
#include <lexbor/ns/const.h>
#include <lexbor/tag/const.h>
#include <lexbor/tag/tag.h>

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

_Static_assert(sizeof(uintptr_t) == sizeof(uint64_t), "V1N0 requires 64-bit uintptr_t");
_Static_assert(sizeof(size_t) <= sizeof(uint64_t), "V1N0 size_t must fit uint64_t");
_Static_assert(sizeof(arbor_view0_native_document_facts) == 184u,
               "V1N1 C0 document-facts layout drift");
_Static_assert(sizeof(arbor_view0_native_source_repair_context) == 104u,
               "V1N1 C0-SR1 source-repair record layout drift");

static arbor_status status_from_errno_value(int value)
{
    return arbor_status_from_native(-(int64_t)value);
}

static arbor_status ok_status(void)
{
    return arbor_status_from_native(0);
}

static bool adapter_range_representable(const void *data, uint64_t length)
{
    if (length == 0u) {
        return true;
    }
    if (data == NULL) {
        return false;
    }

    arbor_asm_result_u64 end = range_end_checked((uint64_t)(uintptr_t)data, length);
    return end.status == 0;
}

static bool adapter_ranges_overlap(
    const void *left,
    uint64_t left_length,
    const void *right,
    uint64_t right_length)
{
    if (left_length == 0u || right_length == 0u || left == NULL || right == NULL) {
        return false;
    }

    arbor_asm_result_u64 overlap = range_overlaps(
        (uint64_t)(uintptr_t)left,
        left_length,
        (uint64_t)(uintptr_t)right,
        right_length);
    return overlap.status == 0 && overlap.value != 0u;
}

static arbor_status validate_adapter_regions(
    arbor_span input,
    arbor_view0_native_diagnostic *diagnostics,
    uint64_t diagnostic_capacity,
    arbor_view0_native_parse_counts *counts_out,
    arbor_view0_native_document_facts *facts_out)
{
    if (counts_out == NULL || facts_out == NULL ||
        diagnostic_capacity > ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS ||
        (diagnostic_capacity != 0u && diagnostics == NULL) ||
        !adapter_range_representable(input.data, input.length) ||
        !adapter_range_representable(counts_out, sizeof(*counts_out)) ||
        !adapter_range_representable(facts_out, sizeof(*facts_out))) {
        return status_from_errno_value(EINVAL);
    }

    arbor_asm_result_u64 diagnostic_bytes = u64_mul_checked(
        diagnostic_capacity,
        (uint64_t)sizeof(*diagnostics));
    if (diagnostic_bytes.status != 0) {
        return arbor_status_from_native(diagnostic_bytes.status);
    }
    if (!adapter_range_representable(diagnostics, diagnostic_bytes.value)) {
        return status_from_errno_value(EINVAL);
    }

    if (adapter_ranges_overlap(input.data, input.length, counts_out, sizeof(*counts_out)) ||
        adapter_ranges_overlap(input.data, input.length, facts_out, sizeof(*facts_out)) ||
        adapter_ranges_overlap(input.data, input.length, diagnostics, diagnostic_bytes.value) ||
        adapter_ranges_overlap(counts_out, sizeof(*counts_out), facts_out, sizeof(*facts_out)) ||
        adapter_ranges_overlap(counts_out, sizeof(*counts_out), diagnostics, diagnostic_bytes.value) ||
        adapter_ranges_overlap(facts_out, sizeof(*facts_out), diagnostics, diagnostic_bytes.value)) {
        return status_from_errno_value(EINVAL);
    }

    return ok_status();
}

static bool pointer_offset(
    arbor_span input,
    const lxb_char_t *pointer,
    uint64_t *offset_out)
{
    if (pointer == NULL || offset_out == NULL) {
        return false;
    }

    if (pointer == lxb_html_tokenizer_eof) {
        *offset_out = input.length;
        return true;
    }

    const uintptr_t base = (uintptr_t)input.data;
    arbor_asm_result_u64 end_result = range_end_checked(
        (uint64_t)base,
        input.length);
    if (end_result.status != 0) {
        return false;
    }

    const uintptr_t end = (uintptr_t)end_result.value;
    const uintptr_t value = (uintptr_t)pointer;
    if (value < base || value > end) {
        return false;
    }

    *offset_out = (uint64_t)(value - base);
    return true;
}

static bool tree_range(
    arbor_span input,
    const lxb_html_tree_error_t *error,
    uint64_t *offset_out,
    uint64_t *length_out)
{
    if (error == NULL || offset_out == NULL || length_out == NULL) {
        return false;
    }

    uint64_t begin = 0u;
    uint64_t end = 0u;
    if (!pointer_offset(input, error->begin, &begin) ||
        !pointer_offset(input, error->end, &end) || end < begin) {
        return false;
    }

    *offset_out = begin;
    *length_out = end - begin;
    return true;
}

static bool text_fits(size_t prefix_length, size_t text_length, uint64_t capacity)
{
    if (prefix_length > SIZE_MAX - 1u ||
        text_length > SIZE_MAX - prefix_length - 1u) {
        return false;
    }
    return (uint64_t)(prefix_length + text_length + 1u) <= capacity;
}

static void copy_prefixed_text(
    char *destination,
    const char *prefix,
    size_t prefix_length,
    const lxb_char_t *text,
    size_t text_length)
{
    (void)memcpy(destination, prefix, prefix_length);
    (void)memcpy(destination + prefix_length, text, text_length);
    destination[prefix_length + text_length] = '\0';
}

static void copy_prefixed_symbol(
    char *destination,
    const char *prefix,
    size_t prefix_length,
    const lxb_char_t *text,
    size_t text_length)
{
    (void)memcpy(destination, prefix, prefix_length);
    for (size_t i = 0u; i < text_length; ++i) {
        destination[prefix_length + i] = text[i] == (lxb_char_t)' '
            ? '-'
            : (char)text[i];
    }
    destination[prefix_length + text_length] = '\0';
}

static arbor_status preflight_tokenizer_errors(
    arbor_span input,
    const lexbor_array_obj_t *errors)
{
    if (errors == NULL) {
        return ok_status();
    }

    const size_t count = errors->length;
    for (size_t i = 0u; i < count; ++i) {
        const lxb_html_tokenizer_error_t *error =
            (const lxb_html_tokenizer_error_t *)lexbor_array_obj_get(errors, i);
        if (error == NULL) {
            return status_from_errno_value(EIO);
        }

        uint64_t offset = 0u;
        if (!pointer_offset(input, error->pos, &offset) || offset > input.length) {
            return status_from_errno_value(EIO);
        }

        size_t name_length = 0u;
        const lxb_char_t *name = lxb_html_tokenizer_error_to_string(
            error->id,
            &name_length);
        if (name == NULL ||
            !text_fits(strlen("html.parse.tokenizer."), name_length,
                       ARBOR_VIEW0_NATIVE_RULE_SYMBOL_CAP) ||
            !text_fits(strlen("HTML tokenizer parse error: "), name_length,
                       ARBOR_VIEW0_NATIVE_MESSAGE_CAP)) {
            return status_from_errno_value(EOVERFLOW);
        }
    }

    return ok_status();
}

static arbor_status preflight_tree_errors(
    arbor_span input,
    const lexbor_array_obj_t *errors)
{
    if (errors == NULL) {
        return ok_status();
    }

    const size_t count = errors->length;
    for (size_t i = 0u; i < count; ++i) {
        const lxb_html_tree_error_t *error =
            (const lxb_html_tree_error_t *)lexbor_array_obj_get(errors, i);
        if (error == NULL) {
            return status_from_errno_value(EIO);
        }

        uint64_t offset = 0u;
        uint64_t length = 0u;
        if (!tree_range(input, error, &offset, &length) ||
            offset > input.length || length > input.length - offset) {
            return status_from_errno_value(EIO);
        }

        size_t name_length = 0u;
        const lxb_char_t *name = lxb_html_tree_error_to_string(
            error->id,
            &name_length);
        if (name == NULL ||
            !text_fits(strlen("html.parse.tree."), name_length,
                       ARBOR_VIEW0_NATIVE_RULE_SYMBOL_CAP) ||
            !text_fits(strlen("HTML tree-construction parse error: "), name_length,
                       ARBOR_VIEW0_NATIVE_MESSAGE_CAP)) {
            return status_from_errno_value(EOVERFLOW);
        }
    }

    return ok_status();
}

static void fill_tokenizer_errors(
    arbor_span input,
    const lexbor_array_obj_t *errors,
    arbor_view0_native_diagnostic *diagnostics,
    uint64_t *sequence_io,
    uint64_t *index_io)
{
    if (errors == NULL) {
        return;
    }

    const char symbol_prefix[] = "html.parse.tokenizer.";
    const char message_prefix[] = "HTML tokenizer parse error: ";
    const size_t symbol_prefix_length = sizeof(symbol_prefix) - 1u;
    const size_t message_prefix_length = sizeof(message_prefix) - 1u;

    for (size_t i = 0u; i < errors->length; ++i) {
        const lxb_html_tokenizer_error_t *error =
            (const lxb_html_tokenizer_error_t *)lexbor_array_obj_get(errors, i);
        uint64_t offset = 0u;
        (void)pointer_offset(input, error->pos, &offset);

        size_t name_length = 0u;
        const lxb_char_t *name = lxb_html_tokenizer_error_to_string(
            error->id,
            &name_length);

        arbor_view0_native_diagnostic *diagnostic = diagnostics + *index_io;
        (void)memset(diagnostic, 0, sizeof(*diagnostic));
        diagnostic->rule_id = ARBOR_VIEW0_NATIVE_RULE_TOKENIZER_BASE +
                              (uint64_t)(uint32_t)error->id;
        diagnostic->byte_offset = offset;
        diagnostic->source_length = offset < input.length ? 1u : 0u;
        diagnostic->discovery_sequence = *sequence_io;
        diagnostic->severity = (uint32_t)ARBOR_VIEW0_NATIVE_SEVERITY_ERROR;
        diagnostic->origin = (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_LEXBOR_TOKENIZER;
        diagnostic->external_id = (uint32_t)error->id;
        copy_prefixed_symbol(
            diagnostic->symbolic_name,
            symbol_prefix,
            symbol_prefix_length,
            name,
            name_length);
        copy_prefixed_text(
            diagnostic->message,
            message_prefix,
            message_prefix_length,
            name,
            name_length);

        *sequence_io += 1u;
        *index_io += 1u;
    }
}

static void fill_tree_errors(
    arbor_span input,
    const lexbor_array_obj_t *errors,
    arbor_view0_native_diagnostic *diagnostics,
    uint64_t *sequence_io,
    uint64_t *index_io)
{
    if (errors == NULL) {
        return;
    }

    const char symbol_prefix[] = "html.parse.tree.";
    const char message_prefix[] = "HTML tree-construction parse error: ";
    const size_t symbol_prefix_length = sizeof(symbol_prefix) - 1u;
    const size_t message_prefix_length = sizeof(message_prefix) - 1u;

    for (size_t i = 0u; i < errors->length; ++i) {
        const lxb_html_tree_error_t *error =
            (const lxb_html_tree_error_t *)lexbor_array_obj_get(errors, i);
        uint64_t offset = 0u;
        uint64_t length = 0u;
        (void)tree_range(input, error, &offset, &length);

        size_t name_length = 0u;
        const lxb_char_t *name = lxb_html_tree_error_to_string(
            error->id,
            &name_length);

        arbor_view0_native_diagnostic *diagnostic = diagnostics + *index_io;
        (void)memset(diagnostic, 0, sizeof(*diagnostic));
        diagnostic->rule_id = ARBOR_VIEW0_NATIVE_RULE_TREE_BASE +
                              (uint64_t)(uint32_t)error->id;
        diagnostic->byte_offset = offset;
        diagnostic->source_length = length;
        diagnostic->discovery_sequence = *sequence_io;
        diagnostic->severity = (uint32_t)ARBOR_VIEW0_NATIVE_SEVERITY_ERROR;
        diagnostic->origin = (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_LEXBOR_TREE;
        diagnostic->external_id = (uint32_t)error->id;
        copy_prefixed_symbol(
            diagnostic->symbolic_name,
            symbol_prefix,
            symbol_prefix_length,
            name,
            name_length);
        copy_prefixed_text(
            diagnostic->message,
            message_prefix,
            message_prefix_length,
            name,
            name_length);

        *sequence_io += 1u;
        *index_io += 1u;
    }
}

static uint64_t namespace_id_from_lexbor(uintptr_t ns)
{
    switch (ns) {
        case LXB_NS_HTML: return ARBOR_VIEW0_NATIVE_NAMESPACE_HTML;
        case LXB_NS_SVG: return ARBOR_VIEW0_NATIVE_NAMESPACE_SVG;
        case LXB_NS_MATH: return ARBOR_VIEW0_NATIVE_NAMESPACE_MATHML;
        case LXB_NS__UNDEF: return ARBOR_VIEW0_NATIVE_NAMESPACE_NONE;
        default: return ARBOR_VIEW0_NATIVE_NAMESPACE_OTHER;
    }
}

uint64_t arbor_view0_native_g03_standard_element_id_from_lexbor(
    uintptr_t local_name,
    uintptr_t ns)
{
    if (ns != LXB_NS_HTML) {
        return ARBOR_VIEW0_NATIVE_ELEMENT_NONE;
    }

    switch ((lxb_tag_id_t)local_name) {
        case LXB_TAG_HTML: return ARBOR_VIEW0_NATIVE_ELEMENT_HTML;
        case LXB_TAG_HEAD: return ARBOR_VIEW0_NATIVE_ELEMENT_HEAD;
        case LXB_TAG_TITLE: return ARBOR_VIEW0_NATIVE_ELEMENT_TITLE;
        case LXB_TAG_BASE: return ARBOR_VIEW0_NATIVE_ELEMENT_BASE;
        case LXB_TAG_LINK: return ARBOR_VIEW0_NATIVE_ELEMENT_LINK;
        case LXB_TAG_META: return ARBOR_VIEW0_NATIVE_ELEMENT_META;
        case LXB_TAG_STYLE: return ARBOR_VIEW0_NATIVE_ELEMENT_STYLE;
        case LXB_TAG_BODY: return ARBOR_VIEW0_NATIVE_ELEMENT_BODY;
        case LXB_TAG_ARTICLE: return ARBOR_VIEW0_NATIVE_ELEMENT_ARTICLE;
        case LXB_TAG_SECTION: return ARBOR_VIEW0_NATIVE_ELEMENT_SECTION;
        case LXB_TAG_NAV: return ARBOR_VIEW0_NATIVE_ELEMENT_NAV;
        case LXB_TAG_ASIDE: return ARBOR_VIEW0_NATIVE_ELEMENT_ASIDE;
        case LXB_TAG_H1: return ARBOR_VIEW0_NATIVE_ELEMENT_H1;
        case LXB_TAG_H2: return ARBOR_VIEW0_NATIVE_ELEMENT_H2;
        case LXB_TAG_H3: return ARBOR_VIEW0_NATIVE_ELEMENT_H3;
        case LXB_TAG_H4: return ARBOR_VIEW0_NATIVE_ELEMENT_H4;
        case LXB_TAG_H5: return ARBOR_VIEW0_NATIVE_ELEMENT_H5;
        case LXB_TAG_H6: return ARBOR_VIEW0_NATIVE_ELEMENT_H6;
        case LXB_TAG_HGROUP: return ARBOR_VIEW0_NATIVE_ELEMENT_HGROUP;
        case LXB_TAG_HEADER: return ARBOR_VIEW0_NATIVE_ELEMENT_HEADER;
        case LXB_TAG_FOOTER: return ARBOR_VIEW0_NATIVE_ELEMENT_FOOTER;
        case LXB_TAG_ADDRESS: return ARBOR_VIEW0_NATIVE_ELEMENT_ADDRESS;
        case LXB_TAG_P: return ARBOR_VIEW0_NATIVE_ELEMENT_P;
        case LXB_TAG_HR: return ARBOR_VIEW0_NATIVE_ELEMENT_HR;
        case LXB_TAG_PRE: return ARBOR_VIEW0_NATIVE_ELEMENT_PRE;
        case LXB_TAG_BLOCKQUOTE: return ARBOR_VIEW0_NATIVE_ELEMENT_BLOCKQUOTE;
        case LXB_TAG_OL: return ARBOR_VIEW0_NATIVE_ELEMENT_OL;
        case LXB_TAG_UL: return ARBOR_VIEW0_NATIVE_ELEMENT_UL;
        case LXB_TAG_MENU: return ARBOR_VIEW0_NATIVE_ELEMENT_MENU;
        case LXB_TAG_LI: return ARBOR_VIEW0_NATIVE_ELEMENT_LI;
        case LXB_TAG_DL: return ARBOR_VIEW0_NATIVE_ELEMENT_DL;
        case LXB_TAG_DT: return ARBOR_VIEW0_NATIVE_ELEMENT_DT;
        case LXB_TAG_DD: return ARBOR_VIEW0_NATIVE_ELEMENT_DD;
        case LXB_TAG_FIGURE: return ARBOR_VIEW0_NATIVE_ELEMENT_FIGURE;
        case LXB_TAG_FIGCAPTION: return ARBOR_VIEW0_NATIVE_ELEMENT_FIGCAPTION;
        case LXB_TAG_MAIN: return ARBOR_VIEW0_NATIVE_ELEMENT_MAIN;
        case LXB_TAG_SEARCH: return ARBOR_VIEW0_NATIVE_ELEMENT_SEARCH;
        case LXB_TAG_DIV: return ARBOR_VIEW0_NATIVE_ELEMENT_DIV;
        case LXB_TAG_A: return ARBOR_VIEW0_NATIVE_ELEMENT_A;
        case LXB_TAG_EM: return ARBOR_VIEW0_NATIVE_ELEMENT_EM;
        case LXB_TAG_STRONG: return ARBOR_VIEW0_NATIVE_ELEMENT_STRONG;
        case LXB_TAG_SMALL: return ARBOR_VIEW0_NATIVE_ELEMENT_SMALL;
        case LXB_TAG_S: return ARBOR_VIEW0_NATIVE_ELEMENT_S;
        case LXB_TAG_CITE: return ARBOR_VIEW0_NATIVE_ELEMENT_CITE;
        case LXB_TAG_Q: return ARBOR_VIEW0_NATIVE_ELEMENT_Q;
        case LXB_TAG_DFN: return ARBOR_VIEW0_NATIVE_ELEMENT_DFN;
        case LXB_TAG_ABBR: return ARBOR_VIEW0_NATIVE_ELEMENT_ABBR;
        case LXB_TAG_RUBY: return ARBOR_VIEW0_NATIVE_ELEMENT_RUBY;
        case LXB_TAG_RT: return ARBOR_VIEW0_NATIVE_ELEMENT_RT;
        case LXB_TAG_RP: return ARBOR_VIEW0_NATIVE_ELEMENT_RP;
        case LXB_TAG_DATA: return ARBOR_VIEW0_NATIVE_ELEMENT_DATA;
        case LXB_TAG_TIME: return ARBOR_VIEW0_NATIVE_ELEMENT_TIME;
        case LXB_TAG_CODE: return ARBOR_VIEW0_NATIVE_ELEMENT_CODE;
        case LXB_TAG_VAR: return ARBOR_VIEW0_NATIVE_ELEMENT_VAR;
        case LXB_TAG_SAMP: return ARBOR_VIEW0_NATIVE_ELEMENT_SAMP;
        case LXB_TAG_KBD: return ARBOR_VIEW0_NATIVE_ELEMENT_KBD;
        case LXB_TAG_SUB: return ARBOR_VIEW0_NATIVE_ELEMENT_SUB;
        case LXB_TAG_SUP: return ARBOR_VIEW0_NATIVE_ELEMENT_SUP;
        case LXB_TAG_I: return ARBOR_VIEW0_NATIVE_ELEMENT_I;
        case LXB_TAG_B: return ARBOR_VIEW0_NATIVE_ELEMENT_B;
        case LXB_TAG_U: return ARBOR_VIEW0_NATIVE_ELEMENT_U;
        case LXB_TAG_MARK: return ARBOR_VIEW0_NATIVE_ELEMENT_MARK;
        case LXB_TAG_BDI: return ARBOR_VIEW0_NATIVE_ELEMENT_BDI;
        case LXB_TAG_BDO: return ARBOR_VIEW0_NATIVE_ELEMENT_BDO;
        case LXB_TAG_SPAN: return ARBOR_VIEW0_NATIVE_ELEMENT_SPAN;
        case LXB_TAG_BR: return ARBOR_VIEW0_NATIVE_ELEMENT_BR;
        case LXB_TAG_WBR: return ARBOR_VIEW0_NATIVE_ELEMENT_WBR;
        case LXB_TAG_INS: return ARBOR_VIEW0_NATIVE_ELEMENT_INS;
        case LXB_TAG_DEL: return ARBOR_VIEW0_NATIVE_ELEMENT_DEL;
        case LXB_TAG_PICTURE: return ARBOR_VIEW0_NATIVE_ELEMENT_PICTURE;
        case LXB_TAG_SOURCE: return ARBOR_VIEW0_NATIVE_ELEMENT_SOURCE;
        case LXB_TAG_IMG: return ARBOR_VIEW0_NATIVE_ELEMENT_IMG;
        case LXB_TAG_IFRAME: return ARBOR_VIEW0_NATIVE_ELEMENT_IFRAME;
        case LXB_TAG_EMBED: return ARBOR_VIEW0_NATIVE_ELEMENT_EMBED;
        case LXB_TAG_OBJECT: return ARBOR_VIEW0_NATIVE_ELEMENT_OBJECT;
        case LXB_TAG_VIDEO: return ARBOR_VIEW0_NATIVE_ELEMENT_VIDEO;
        case LXB_TAG_AUDIO: return ARBOR_VIEW0_NATIVE_ELEMENT_AUDIO;
        case LXB_TAG_TRACK: return ARBOR_VIEW0_NATIVE_ELEMENT_TRACK;
        case LXB_TAG_MAP: return ARBOR_VIEW0_NATIVE_ELEMENT_MAP;
        case LXB_TAG_AREA: return ARBOR_VIEW0_NATIVE_ELEMENT_AREA;
        case LXB_TAG_TABLE: return ARBOR_VIEW0_NATIVE_ELEMENT_TABLE;
        case LXB_TAG_CAPTION: return ARBOR_VIEW0_NATIVE_ELEMENT_CAPTION;
        case LXB_TAG_COLGROUP: return ARBOR_VIEW0_NATIVE_ELEMENT_COLGROUP;
        case LXB_TAG_COL: return ARBOR_VIEW0_NATIVE_ELEMENT_COL;
        case LXB_TAG_TBODY: return ARBOR_VIEW0_NATIVE_ELEMENT_TBODY;
        case LXB_TAG_THEAD: return ARBOR_VIEW0_NATIVE_ELEMENT_THEAD;
        case LXB_TAG_TFOOT: return ARBOR_VIEW0_NATIVE_ELEMENT_TFOOT;
        case LXB_TAG_TR: return ARBOR_VIEW0_NATIVE_ELEMENT_TR;
        case LXB_TAG_TD: return ARBOR_VIEW0_NATIVE_ELEMENT_TD;
        case LXB_TAG_TH: return ARBOR_VIEW0_NATIVE_ELEMENT_TH;
        case LXB_TAG_FORM: return ARBOR_VIEW0_NATIVE_ELEMENT_FORM;
        case LXB_TAG_LABEL: return ARBOR_VIEW0_NATIVE_ELEMENT_LABEL;
        case LXB_TAG_INPUT: return ARBOR_VIEW0_NATIVE_ELEMENT_INPUT;
        case LXB_TAG_BUTTON: return ARBOR_VIEW0_NATIVE_ELEMENT_BUTTON;
        case LXB_TAG_SELECT: return ARBOR_VIEW0_NATIVE_ELEMENT_SELECT;
        case LXB_TAG_DATALIST: return ARBOR_VIEW0_NATIVE_ELEMENT_DATALIST;
        case LXB_TAG_OPTGROUP: return ARBOR_VIEW0_NATIVE_ELEMENT_OPTGROUP;
        case LXB_TAG_OPTION: return ARBOR_VIEW0_NATIVE_ELEMENT_OPTION;
        case LXB_TAG_TEXTAREA: return ARBOR_VIEW0_NATIVE_ELEMENT_TEXTAREA;
        case LXB_TAG_OUTPUT: return ARBOR_VIEW0_NATIVE_ELEMENT_OUTPUT;
        case LXB_TAG_PROGRESS: return ARBOR_VIEW0_NATIVE_ELEMENT_PROGRESS;
        case LXB_TAG_METER: return ARBOR_VIEW0_NATIVE_ELEMENT_METER;
        case LXB_TAG_FIELDSET: return ARBOR_VIEW0_NATIVE_ELEMENT_FIELDSET;
        case LXB_TAG_LEGEND: return ARBOR_VIEW0_NATIVE_ELEMENT_LEGEND;
        case LXB_TAG_SELECTEDCONTENT: return ARBOR_VIEW0_NATIVE_ELEMENT_SELECTEDCONTENT;
        case LXB_TAG_DETAILS: return ARBOR_VIEW0_NATIVE_ELEMENT_DETAILS;
        case LXB_TAG_SUMMARY: return ARBOR_VIEW0_NATIVE_ELEMENT_SUMMARY;
        case LXB_TAG_DIALOG: return ARBOR_VIEW0_NATIVE_ELEMENT_DIALOG;
        case LXB_TAG_SCRIPT: return ARBOR_VIEW0_NATIVE_ELEMENT_SCRIPT;
        case LXB_TAG_NOSCRIPT: return ARBOR_VIEW0_NATIVE_ELEMENT_NOSCRIPT;
        case LXB_TAG_TEMPLATE: return ARBOR_VIEW0_NATIVE_ELEMENT_TEMPLATE;
        case LXB_TAG_SLOT: return ARBOR_VIEW0_NATIVE_ELEMENT_SLOT;
        case LXB_TAG_CANVAS: return ARBOR_VIEW0_NATIVE_ELEMENT_CANVAS;
        default: return ARBOR_VIEW0_NATIVE_ELEMENT_NONE;
    }
}

static arbor_span node_local_name_span(const lxb_dom_node_t *node)
{
    arbor_span span = {NULL, 0u};
    if (node == NULL || node->type != LXB_DOM_NODE_TYPE_ELEMENT) {
        return span;
    }
    size_t length = 0u;
    const lxb_char_t *name = lxb_tag_name_by_id((lxb_tag_id_t)node->local_name, &length);
    if (name == NULL) {
        return span;
    }
    span.data = (const uint8_t *)name;
    span.length = (uint64_t)length;
    return span;
}

static arbor_view0_native_document_facts document_facts_initial(void)
{
    arbor_view0_native_document_facts facts = {0};
    facts.source_first_doctype_keyword_offset = ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE;
    facts.source_first_doctype_name_offset = ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE;
    facts.source_first_doctype_external_keyword_offset = ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE;
    facts.source_first_doctype_public_id_offset = ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE;
    facts.source_first_doctype_system_id_offset = ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE;
    facts.source_second_title_start_tag_offset = ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE;
    facts.source_second_base_start_tag_offset = ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE;
    facts.source_second_body_start_tag_offset = ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE;
    return facts;
}

typedef struct arbor_view0_native_source_capture {
    arbor_span input;
    lxb_html_tokenizer_token_f downstream;
    void *downstream_context;
    arbor_view0_native_document_facts *facts;
    const arbor_view0_native_semantic_observer *observer;
    lxb_html_tree_t *tree;
    arbor_status source_repair_failure;
    arbor_view0_native_g03_provenance_context provenance;
    bool failed;
} arbor_view0_native_source_capture;

static uint64_t source_repair_mode_id(lxb_html_tree_insertion_mode_f mode)
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

static arbor_status source_repair_current_context(
    const arbor_view0_native_source_capture *capture,
    lxb_html_tree_t *tree,
    uint64_t *element_id_out,
    uint64_t *source_offset_out,
    uint64_t *mode_id_out,
    uint64_t *depth_out)
{
    if (capture == NULL || tree == NULL || element_id_out == NULL ||
        source_offset_out == NULL || mode_id_out == NULL || depth_out == NULL ||
        tree->open_elements == NULL) return status_from_errno_value(EINVAL);
    const uint64_t mode_id = source_repair_mode_id(tree->mode);
    if (mode_id == ARBOR_VIEW0_NATIVE_INSERTION_MODE_NONE) return status_from_errno_value(EIO);
    lxb_dom_node_t *node = lxb_html_tree_current_node(tree);
    *element_id_out = node == NULL ? ARBOR_VIEW0_NATIVE_ELEMENT_NONE
        : arbor_view0_native_g03_standard_element_id_from_lexbor(node->local_name, node->ns);
    if (node == NULL || node->user == NULL) {
        *source_offset_out = ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE;
    } else {
        uint64_t offset = 0u;
        if (!pointer_offset(capture->input, (const lxb_char_t *)node->user, &offset))
            return status_from_errno_value(EIO);
        *source_offset_out = offset;
    }
    *mode_id_out = mode_id;
    *depth_out = (uint64_t)tree->open_elements->length;
    return ok_status();
}

static arbor_status source_repair_begin(
    arbor_view0_native_source_capture *capture,
    lxb_html_tree_t *tree,
    const lxb_html_token_t *token,
    uint64_t standard_element_id,
    uint64_t source_offset,
    uint64_t source_length)
{
    if (capture == NULL || tree == NULL || token == NULL ||
        standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_NONE || source_length == 0u ||
        capture->provenance.source_repair_active || capture->provenance.source_repair_tree != tree)
        return status_from_errno_value(EINVAL);
    arbor_view0_native_source_repair_context record = {0};
    record.standard_element_id = standard_element_id;
    record.source_offset = source_offset;
    record.source_length = source_length;
    record.initial_current_source_offset = ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE;
    record.insertion_current_source_offset = ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE;
    arbor_status status = source_repair_current_context(
        capture, tree, &record.initial_current_standard_element_id,
        &record.initial_current_source_offset, &record.initial_insertion_mode_id,
        &record.initial_open_elements_depth);
    if (status.native != 0) return status;
    capture->provenance.source_repair_token = token;
    capture->provenance.source_repair_record = record;
    capture->provenance.source_repair_active = true;
    return ok_status();
}

static bool source_token_name_range(
    const arbor_view0_native_source_capture *capture,
    const lxb_html_token_t *token,
    uint64_t *offset_out,
    uint64_t *length_out)
{
    uint64_t begin = 0u;
    uint64_t end = 0u;
    if (capture == NULL || token == NULL || offset_out == NULL || length_out == NULL ||
        !pointer_offset(capture->input, token->begin, &begin) ||
        !pointer_offset(capture->input, token->end, &end) || end < begin ||
        begin > capture->input.length || end > capture->input.length) {
        return false;
    }

    *offset_out = begin;
    *length_out = end - begin;
    return true;
}

static bool source_pointer_range(
    const arbor_view0_native_source_capture *capture,
    const lxb_char_t *begin_pointer,
    const lxb_char_t *end_pointer,
    uint64_t *offset_out,
    uint64_t *length_out)
{
    if (capture == NULL || offset_out == NULL || length_out == NULL) {
        return false;
    }
    if (begin_pointer == NULL && end_pointer == NULL) {
        *offset_out = ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE;
        *length_out = 0u;
        return true;
    }
    if (begin_pointer == NULL || end_pointer == NULL) {
        return false;
    }

    uint64_t begin = 0u;
    uint64_t end = 0u;
    if (!pointer_offset(capture->input, begin_pointer, &begin) ||
        !pointer_offset(capture->input, end_pointer, &end) || end < begin ||
        begin > capture->input.length || end > capture->input.length) {
        return false;
    }

    *offset_out = begin;
    *length_out = end - begin;
    return true;
}


static arbor_status source_attribute_observe(
    arbor_view0_native_source_capture *capture,
    const lxb_html_token_t *token,
    uint64_t standard_element_id,
    uint64_t owner_source_offset)
{
    if (capture == NULL || token == NULL ||
        standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_NONE ||
        capture->observer == NULL || capture->observer->source_attribute == NULL)
        return status_from_errno_value(EINVAL);

    uint64_t ordinal = 0u;
    for (lxb_html_token_attr_t *attr = token->attr_first; attr != NULL; attr = attr->next) {
        size_t name_length = 0u;
        const lxb_char_t *name = lxb_html_token_attr_name(attr, &name_length);
        if (name == NULL || name_length == 0u) return status_from_errno_value(EIO);
        if (ordinal == UINT64_MAX) return status_from_errno_value(EOVERFLOW);
        uint64_t name_offset = 0u;
        uint64_t name_source_length = 0u;
        if (!source_pointer_range(capture, attr->name_begin, attr->name_end,
                                  &name_offset, &name_source_length) ||
            name_source_length == 0u) {
            return status_from_errno_value(EIO);
        }
        arbor_view0_native_source_attribute_observation observation = {
            .owner_standard_element_id = standard_element_id,
            .owner_source_offset = owner_source_offset,
            .source_offset = name_offset,
            .source_length = name_source_length,
            .ordinal = ordinal,
            .local_name = {(const uint8_t *)name, (uint64_t)name_length},
            .value = {(const uint8_t *)attr->value, (uint64_t)attr->value_size}
        };
        if (observation.value.length != 0u && observation.value.data == NULL)
            return status_from_errno_value(EIO);
        arbor_status status = capture->observer->source_attribute(
            capture->observer->context, &observation);
        if (status.native != 0) return status;
        ordinal += 1u;
    }
    return ok_status();
}

static arbor_status source_text_observe(
    arbor_view0_native_source_capture *capture,
    const lxb_html_token_t *token)
{
    if (capture == NULL || token == NULL || capture->tree == NULL ||
        capture->observer == NULL || capture->observer->source_text == NULL ||
        token->tag_id != LXB_TAG__TEXT) return status_from_errno_value(EINVAL);

    uint64_t source_offset = 0u, source_length = 0u;
    if (!source_token_name_range(capture, token, &source_offset, &source_length))
        return status_from_errno_value(EIO);
    if (token->text_start == NULL || token->text_end == NULL ||
        token->text_end < token->text_start) return status_from_errno_value(EIO);

    arbor_view0_native_source_text_observation observation = {
        .source_offset = source_offset,
        .source_length = source_length,
        .initial_current_source_offset = ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE,
        .text = {(const uint8_t *)token->text_start,
                 (uint64_t)(token->text_end - token->text_start)}
    };
    arbor_status status = source_repair_current_context(
        capture, capture->tree, &observation.initial_current_standard_element_id,
        &observation.initial_current_source_offset, &observation.initial_insertion_mode_id,
        &observation.initial_open_elements_depth);
    if (status.native != 0) return status;
    return capture->observer->source_text(capture->observer->context, &observation);
}

static bool capture_first_doctype_source(
    arbor_view0_native_source_capture *capture,
    const lxb_html_token_t *token,
    uint64_t keyword_offset,
    uint64_t keyword_length)
{
    arbor_view0_native_document_facts *facts = capture->facts;
    facts->source_first_doctype_keyword_offset = keyword_offset;
    facts->source_first_doctype_keyword_length = keyword_length;

    const lxb_html_token_attr_t *attr = token->attr_first;
    if (attr == NULL) {
        return true;
    }
    if (!source_pointer_range(
            capture,
            attr->name_begin,
            attr->name_end,
            &facts->source_first_doctype_name_offset,
            &facts->source_first_doctype_name_length)) {
        return false;
    }

    attr = attr->next;
    if (attr == NULL) {
        return true;
    }
    if (attr->name == NULL ||
        !source_pointer_range(
            capture,
            attr->name_begin,
            attr->name_end,
            &facts->source_first_doctype_external_keyword_offset,
            &facts->source_first_doctype_external_keyword_length)) {
        return false;
    }

    if (attr->name->attr_id == LXB_DOM_ATTR_PUBLIC) {
        if (!source_pointer_range(
                capture,
                attr->value_begin,
                attr->value_end,
                &facts->source_first_doctype_public_id_offset,
                &facts->source_first_doctype_public_id_length)) {
            return false;
        }

        attr = attr->next;
        if (attr != NULL &&
            !source_pointer_range(
                capture,
                attr->value_begin,
                attr->value_end,
                &facts->source_first_doctype_system_id_offset,
                &facts->source_first_doctype_system_id_length)) {
            return false;
        }
    } else if (attr->name->attr_id == LXB_DOM_ATTR_SYSTEM) {
        if (!source_pointer_range(
                capture,
                attr->value_begin,
                attr->value_end,
                &facts->source_first_doctype_system_id_offset,
                &facts->source_first_doctype_system_id_length)) {
            return false;
        }
    }

    return true;
}

static void capture_relevant_start_tag(
    arbor_view0_native_source_capture *capture,
    lxb_tag_id_t tag_id,
    uint64_t offset)
{
    uint64_t *count = NULL;
    uint64_t *second_offset = NULL;

    if (tag_id == LXB_TAG_TITLE) {
        count = &capture->facts->source_title_start_tag_count;
        second_offset = &capture->facts->source_second_title_start_tag_offset;
    } else if (tag_id == LXB_TAG_BASE) {
        count = &capture->facts->source_base_start_tag_count;
        second_offset = &capture->facts->source_second_base_start_tag_offset;
    } else if (tag_id == LXB_TAG_BODY) {
        count = &capture->facts->source_body_start_tag_count;
        second_offset = &capture->facts->source_second_body_start_tag_offset;
    } else {
        return;
    }

    if (*count == UINT64_MAX) {
        capture->failed = true;
        return;
    }

    *count += 1u;
    if (*count == 2u) {
        *second_offset = offset;
    }
}

static lxb_html_token_t *capture_token_done(
    lxb_html_tokenizer_t *tokenizer,
    lxb_html_token_t *token,
    void *context)
{
    arbor_view0_native_source_capture *capture =
        (arbor_view0_native_source_capture *)context;

    if (capture == NULL || capture->downstream == NULL || capture->facts == NULL) {
        if (tokenizer != NULL) {
            lxb_html_tokenizer_status_set(tokenizer, LXB_STATUS_ERROR);
        }
        return NULL;
    }

    capture->provenance.current_start = token != NULL &&
        (token->type & LXB_HTML_TOKEN_TYPE_CLOSE) == 0 &&
        token->tag_id > LXB_TAG__TEXT;
    capture->provenance.current_begin =
        capture->provenance.current_start ? token->begin : NULL;
    capture->provenance.current_tag =
        capture->provenance.current_start ? token->tag_id : LXB_TAG__UNDEF;
    capture->provenance.current_assigned = false;

    capture->provenance.source_repair_active = false;
    capture->provenance.source_repair_token = NULL;
    (void)memset(&capture->provenance.source_repair_record, 0,
                 sizeof(capture->provenance.source_repair_record));

    if (capture->observer != NULL && capture->observer->source_repair != NULL &&
        capture->tree != NULL && token != NULL &&
        (token->type & LXB_HTML_TOKEN_TYPE_CLOSE) == 0 &&
        token->tag_id > LXB_TAG__TEXT) {
        const uint64_t standard_id =
            arbor_view0_native_g03_standard_element_id_from_lexbor(
                (uintptr_t)token->tag_id, (uintptr_t)LXB_NS_HTML);
        if (standard_id != ARBOR_VIEW0_NATIVE_ELEMENT_NONE) {
            uint64_t sr_offset = 0u;
            uint64_t sr_length = 0u;
            if (!source_token_name_range(capture, token, &sr_offset, &sr_length)) {
                capture->failed = true;
            } else {
                arbor_status sr_status = source_repair_begin(
                    capture, capture->tree, token, standard_id,
                    sr_offset, sr_length);
                if (sr_status.native != 0) {
                    capture->failed = true;
                    capture->source_repair_failure = sr_status;
                }
            }
        }
    }

    if (!capture->failed && capture->observer != NULL &&
        capture->observer->source_attribute != NULL && capture->tree != NULL &&
        token != NULL && (token->type & LXB_HTML_TOKEN_TYPE_CLOSE) == 0 &&
        token->tag_id > LXB_TAG__TEXT) {
        const uint64_t standard_id =
            arbor_view0_native_g03_standard_element_id_from_lexbor(
                (uintptr_t)token->tag_id, (uintptr_t)LXB_NS_HTML);
        if (standard_id != ARBOR_VIEW0_NATIVE_ELEMENT_NONE) {
            uint64_t owner_offset = 0u, owner_length = 0u;
            if (!source_token_name_range(capture, token, &owner_offset, &owner_length)) {
                capture->failed = true;
            } else {
                arbor_status attr_status = source_attribute_observe(
                    capture, token, standard_id, owner_offset);
                if (attr_status.native != 0) {
                    capture->failed = true;
                    capture->source_repair_failure = attr_status;
                }
            }
        }
    }

    if (!capture->failed && capture->observer != NULL &&
        capture->observer->source_text != NULL && capture->tree != NULL &&
        token != NULL && token->tag_id == LXB_TAG__TEXT) {
        arbor_status text_status = source_text_observe(capture, token);
        if (text_status.native != 0) {
            capture->failed = true;
            capture->source_repair_failure = text_status;
        }
    }

    if (capture->failed) {
        if (tokenizer != NULL) lxb_html_tokenizer_status_set(tokenizer, LXB_STATUS_ERROR);
        return NULL;
    }

    if (token != NULL && (token->type & LXB_HTML_TOKEN_TYPE_CLOSE) == 0) {
        const bool relevant = token->tag_id == LXB_TAG__EM_DOCTYPE ||
                              token->tag_id == LXB_TAG_TITLE ||
                              token->tag_id == LXB_TAG_BASE ||
                              token->tag_id == LXB_TAG_BODY;
        if (relevant) {
            uint64_t offset = 0u;
            uint64_t length = 0u;
            if (!source_token_name_range(capture, token, &offset, &length)) {
                capture->failed = true;
            } else if (token->tag_id == LXB_TAG__EM_DOCTYPE) {
                if (capture->facts->source_doctype_count == UINT64_MAX) {
                    capture->failed = true;
                } else {
                    capture->facts->source_doctype_count += 1u;
                    if (capture->facts->source_doctype_count == 1u &&
                        !capture_first_doctype_source(
                            capture, token, offset, length)) {
                        capture->failed = true;
                    }
                }
            } else {
                capture_relevant_start_tag(capture, token->tag_id, offset);
            }
        }
    }

    lxb_html_token_t *result = capture->downstream(
        tokenizer,
        token,
        capture->downstream_context);

    if (!capture->failed && !capture->provenance.failed &&
        capture->provenance.source_repair_active &&
        capture->observer != NULL && capture->observer->source_repair != NULL) {
        arbor_status sr_status = capture->observer->source_repair(
            capture->observer->context,
            &capture->provenance.source_repair_record);
        if (sr_status.native != 0) {
            capture->failed = true;
            capture->source_repair_failure = sr_status;
            if (tokenizer != NULL) {
                lxb_html_tokenizer_status_set(tokenizer, LXB_STATUS_ERROR);
            }
            result = NULL;
        }
    }

    capture->provenance.source_repair_active = false;
    capture->provenance.source_repair_token = NULL;
    capture->provenance.current_start = false;
    capture->provenance.current_begin = NULL;
    capture->provenance.current_tag = LXB_TAG__UNDEF;
    capture->provenance.current_assigned = false;
    if (capture->provenance.failed) {
        capture->failed = true;
    }
    return result;
}

static arbor_status count_html_children(
    lxb_dom_node_t *parent,
    lxb_tag_id_t tag_id,
    uint64_t *count_out)
{
    if (count_out == NULL) {
        return status_from_errno_value(EINVAL);
    }

    uint64_t count = 0u;
    if (parent != NULL) {
        for (lxb_dom_node_t *node = parent->first_child;
             node != NULL;
             node = node->next) {
            if (node->type == LXB_DOM_NODE_TYPE_ELEMENT &&
                node->ns == LXB_NS_HTML &&
                node->local_name == tag_id) {
                if (count == UINT64_MAX) {
                    return status_from_errno_value(EOVERFLOW);
                }
                count += 1u;
            }
        }
    }

    *count_out = count;
    return ok_status();
}

static arbor_status collect_dom_facts(
    lxb_html_document_t *document,
    arbor_view0_native_document_facts *facts)
{
    if (document == NULL || facts == NULL) {
        return status_from_errno_value(EINVAL);
    }

    lxb_dom_document_t *dom_document = lxb_dom_interface_document(document);
    if (dom_document == NULL) {
        return status_from_errno_value(EIO);
    }

    facts->dom_doctype_node_count = dom_document->doctype == NULL ? 0u : 1u;

    lxb_dom_node_t *root = dom_document->element == NULL
        ? NULL
        : lxb_dom_interface_node(dom_document->element);
    facts->dom_html_document_element_count =
        root != NULL && root->type == LXB_DOM_NODE_TYPE_ELEMENT &&
        root->ns == LXB_NS_HTML && root->local_name == LXB_TAG_HTML
            ? 1u
            : 0u;

    arbor_status status = count_html_children(
        root,
        LXB_TAG_HEAD,
        &facts->dom_html_head_element_count);
    if (status.native != 0) {
        return status;
    }
    status = count_html_children(
        root,
        LXB_TAG_BODY,
        &facts->dom_html_body_element_count);
    if (status.native != 0) {
        return status;
    }

    lxb_dom_node_t *head = document->head == NULL
        ? NULL
        : lxb_dom_interface_node(document->head);
    status = count_html_children(
        head,
        LXB_TAG_TITLE,
        &facts->dom_head_title_child_count);
    if (status.native != 0) {
        return status;
    }
    return count_html_children(
        head,
        LXB_TAG_BASE,
        &facts->dom_head_base_child_count);
}

static arbor_status element_source_provenance(
    arbor_span input,
    const lxb_dom_node_t *node,
    uint64_t *offset_out,
    uint64_t *length_out,
    uint64_t *flags_out)
{
    if (node == NULL || offset_out == NULL || length_out == NULL || flags_out == NULL) {
        return status_from_errno_value(EINVAL);
    }

    arbor_span local_name = node_local_name_span(node);
    if (local_name.data == NULL || local_name.length == 0u) {
        return status_from_errno_value(EIO);
    }

    if (node->user == NULL) {
        *offset_out = ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE;
        *length_out = 0u;
        *flags_out = ARBOR_VIEW0_NATIVE_ELEMENT_FLAG_SYNTHETIC;
        return ok_status();
    }

    const uint8_t *pointer = (const uint8_t *)node->user;
    const uintptr_t base = (uintptr_t)input.data;
    const uintptr_t value = (uintptr_t)pointer;
    arbor_asm_result_u64 end = range_end_checked(base, input.length);
    if (end.status != 0 || value < base || value > (uintptr_t)end.value) {
        return status_from_errno_value(EIO);
    }

    const uint64_t offset = (uint64_t)(value - base);
    if (offset > input.length || local_name.length > input.length - offset) {
        return status_from_errno_value(EIO);
    }

    *offset_out = offset;
    *length_out = local_name.length;
    *flags_out = 0u;
    return ok_status();
}

static uint64_t count_element_attributes(lxb_dom_element_t *element)
{
    uint64_t count = 0u;
    for (lxb_dom_attr_t *attr = lxb_dom_element_first_attribute(element);
         attr != NULL;
         attr = lxb_dom_element_next_attribute(attr)) {
        if (count == UINT64_MAX) {
            return UINT64_MAX;
        }
        count += 1u;
    }
    return count;
}

static bool is_direct_observation_node(const lxb_dom_node_t *node)
{
    return node != NULL &&
        (node->type == LXB_DOM_NODE_TYPE_ELEMENT ||
         node->type == LXB_DOM_NODE_TYPE_TEXT);
}

static arbor_status count_direct_observation_children(
    lxb_dom_node_t *parent,
    uint64_t *direct_count_out,
    uint64_t *element_count_out)
{
    if (parent == NULL || direct_count_out == NULL || element_count_out == NULL) {
        return status_from_errno_value(EINVAL);
    }

    uint64_t direct_count = 0u;
    uint64_t element_count = 0u;
    for (lxb_dom_node_t *child = parent->first_child;
         child != NULL;
         child = child->next) {
        if (!is_direct_observation_node(child)) {
            continue;
        }
        if (direct_count == UINT64_MAX) {
            return status_from_errno_value(EOVERFLOW);
        }
        direct_count += 1u;
        if (child->type == LXB_DOM_NODE_TYPE_ELEMENT) {
            if (element_count == UINT64_MAX) {
                return status_from_errno_value(EOVERFLOW);
            }
            element_count += 1u;
        }
    }

    *direct_count_out = direct_count;
    *element_count_out = element_count;
    return ok_status();
}

static bool text_is_inter_element_whitespace(arbor_span text)
{
    for (uint64_t i = 0u; i < text.length; ++i) {
        const uint8_t byte = text.data[i];
        if (byte != (uint8_t)' ' && byte != (uint8_t)'\t' &&
            byte != (uint8_t)'\n' && byte != (uint8_t)'\f' &&
            byte != (uint8_t)'\r') {
            return false;
        }
    }
    return true;
}

static arbor_status emit_attribute_observations(
    const arbor_view0_native_semantic_observer *observer,
    const arbor_view0_native_element_observation *element_observation,
    lxb_dom_element_t *element,
    arbor_view0_native_observation_counts *counts)
{
    uint64_t ordinal = 0u;
    for (lxb_dom_attr_t *attr = lxb_dom_element_first_attribute(element);
         attr != NULL;
         attr = lxb_dom_element_next_attribute(attr)) {
        size_t name_length = 0u;
        size_t value_length = 0u;
        const lxb_char_t *name = lxb_dom_attr_local_name(attr, &name_length);
        const lxb_char_t *value = lxb_dom_attr_value(attr, &value_length);
        if (name == NULL || name_length == 0u) {
            return status_from_errno_value(EIO);
        }

        const arbor_view0_native_attribute_observation attribute = {
            .owner_standard_element_id = element_observation->standard_element_id,
            .namespace_id = namespace_id_from_lexbor(lxb_dom_interface_node(attr)->ns),
            .ordinal = ordinal,
            .local_name = {(const uint8_t *)name, (uint64_t)name_length},
            .value = {(const uint8_t *)value, (uint64_t)value_length}
        };

        if (observer->attribute != NULL) {
            arbor_status status = observer->attribute(observer->context, &attribute);
            if (status.native != 0) {
                return status;
            }
        }
        if (counts->attribute_count == UINT64_MAX || ordinal == UINT64_MAX) {
            return status_from_errno_value(EOVERFLOW);
        }
        counts->attribute_count += 1u;
        ordinal += 1u;
    }
    return ok_status();
}

static arbor_status direct_child_source(
    arbor_span input,
    const lxb_dom_node_t *child,
    uint64_t *source_offset_out,
    uint64_t *source_length_out,
    uint64_t *flags_out)
{
    uint64_t element_flags = 0u;
    arbor_status status = element_source_provenance(
        input,
        child,
        source_offset_out,
        source_length_out,
        &element_flags);
    if (status.native != 0) {
        return status;
    }
    *flags_out = (element_flags & ARBOR_VIEW0_NATIVE_ELEMENT_FLAG_SYNTHETIC) != 0u
        ? ARBOR_VIEW0_NATIVE_CHILD_FLAG_SYNTHETIC
        : 0u;
    return ok_status();
}

static arbor_status emit_direct_child_observations(
    arbor_span input,
    const arbor_view0_native_semantic_observer *observer,
    const arbor_view0_native_element_observation *element_observation,
    lxb_dom_node_t *parent,
    uint64_t element_count,
    arbor_view0_native_observation_counts *counts)
{
    uint64_t sequence_index = 0u;
    uint64_t element_index = 0u;

    for (lxb_dom_node_t *child = parent->first_child;
         child != NULL;
         child = child->next) {
        if (!is_direct_observation_node(child)) {
            continue;
        }

        arbor_view0_native_direct_child_observation observation = {
            .parent_standard_element_id = element_observation->standard_element_id,
            .kind = child->type == LXB_DOM_NODE_TYPE_ELEMENT
                ? ARBOR_VIEW0_NATIVE_DIRECT_CHILD_ELEMENT
                : ARBOR_VIEW0_NATIVE_DIRECT_CHILD_TEXT,
            .sequence_index = sequence_index,
            .element_index = ARBOR_VIEW0_NATIVE_OBSERVATION_INDEX_NONE,
            .element_count = element_count,
            .standard_element_id = ARBOR_VIEW0_NATIVE_ELEMENT_NONE,
            .namespace_id = ARBOR_VIEW0_NATIVE_NAMESPACE_NONE,
            .source_offset = ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE,
            .source_length = 0u,
            .flags = 0u,
            .local_name = {NULL, 0u},
            .text = {NULL, 0u}
        };

        if (child->type == LXB_DOM_NODE_TYPE_ELEMENT) {
            observation.element_index = element_index;
            observation.standard_element_id = arbor_view0_native_g03_standard_element_id_from_lexbor(
                child->local_name,
                child->ns);
            observation.namespace_id = namespace_id_from_lexbor(child->ns);
            observation.local_name = node_local_name_span(child);
            if (observation.local_name.data == NULL) {
                return status_from_errno_value(EIO);
            }
            arbor_status status = direct_child_source(
                input,
                child,
                &observation.source_offset,
                &observation.source_length,
                &observation.flags);
            if (status.native != 0) {
                return status;
            }
            if (element_index == 0u) {
                observation.flags |= ARBOR_VIEW0_NATIVE_CHILD_FLAG_FIRST_ELEMENT;
            }
            if (element_index + 1u == element_count) {
                observation.flags |= ARBOR_VIEW0_NATIVE_CHILD_FLAG_LAST_ELEMENT;
            }
            if (element_index == UINT64_MAX) {
                return status_from_errno_value(EOVERFLOW);
            }
            element_index += 1u;
        } else {
            lxb_dom_text_t *text = lxb_dom_interface_text(child);
            if (text == NULL) {
                return status_from_errno_value(EIO);
            }
            observation.text.data = (const uint8_t *)text->char_data.data.data;
            observation.text.length = (uint64_t)text->char_data.data.length;
            if (observation.text.length != 0u && observation.text.data == NULL) {
                return status_from_errno_value(EIO);
            }
            if (text_is_inter_element_whitespace(observation.text)) {
                observation.flags |=
                    ARBOR_VIEW0_NATIVE_CHILD_FLAG_TEXT_INTER_ELEMENT_WHITESPACE;
            }
        }

        if (observer->direct_child != NULL) {
            arbor_status status = observer->direct_child(
                observer->context,
                &observation);
            if (status.native != 0) {
                return status;
            }
        }
        if (counts->direct_child_count == UINT64_MAX || sequence_index == UINT64_MAX) {
            return status_from_errno_value(EOVERFLOW);
        }
        counts->direct_child_count += 1u;
        sequence_index += 1u;
    }

    if (element_index != element_count) {
        return status_from_errno_value(EIO);
    }
    return ok_status();
}

static lxb_dom_node_t *first_element_child(lxb_dom_node_t *parent)
{
    if (parent == NULL) {
        return NULL;
    }
    for (lxb_dom_node_t *node = parent->first_child; node != NULL; node = node->next) {
        if (node->type == LXB_DOM_NODE_TYPE_ELEMENT) {
            return node;
        }
    }
    return NULL;
}

static lxb_dom_node_t *next_element_sibling(lxb_dom_node_t *node)
{
    if (node == NULL) {
        return NULL;
    }
    for (lxb_dom_node_t *next = node->next; next != NULL; next = next->next) {
        if (next->type == LXB_DOM_NODE_TYPE_ELEMENT) {
            return next;
        }
    }
    return NULL;
}

static arbor_status ancestor_counter_add(
    uint64_t counters[114],
    uint64_t standard_id)
{
    if (standard_id == ARBOR_VIEW0_NATIVE_ELEMENT_NONE) {
        return ok_status();
    }
    if (standard_id > ARBOR_VIEW0_NATIVE_STANDARD_ELEMENT_COUNT ||
        counters[standard_id] == UINT64_MAX) {
        return status_from_errno_value(EOVERFLOW);
    }
    counters[standard_id] += 1u;
    return ok_status();
}

static arbor_status ancestor_counter_remove(
    uint64_t counters[114],
    uint64_t standard_id)
{
    if (standard_id == ARBOR_VIEW0_NATIVE_ELEMENT_NONE) {
        return ok_status();
    }
    if (standard_id > ARBOR_VIEW0_NATIVE_STANDARD_ELEMENT_COUNT ||
        counters[standard_id] == 0u) {
        return status_from_errno_value(EIO);
    }
    counters[standard_id] -= 1u;
    return ok_status();
}

static void ancestor_bits_from_counters(
    const uint64_t counters[114],
    uint64_t bits[2])
{
    bits[0] = 0u;
    bits[1] = 0u;
    for (uint64_t id = 1u; id <= ARBOR_VIEW0_NATIVE_STANDARD_ELEMENT_COUNT; ++id) {
        if (counters[id] == 0u) {
            continue;
        }
        const uint64_t bit = id - 1u;
        bits[bit / 64u] |= UINT64_C(1) << (bit % 64u);
    }
}

static arbor_status build_element_observation(
    arbor_span input,
    lxb_dom_node_t *node,
    uint64_t depth,
    const uint64_t ancestor_counters[114],
    arbor_view0_native_element_observation *observation_out,
    lxb_dom_element_t **element_out)
{
    if (node == NULL || ancestor_counters == NULL || observation_out == NULL ||
        element_out == NULL || node->type != LXB_DOM_NODE_TYPE_ELEMENT) {
        return status_from_errno_value(EINVAL);
    }

    uint64_t source_offset = ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE;
    uint64_t source_length = 0u;
    uint64_t flags = 0u;
    arbor_status status = element_source_provenance(
        input,
        node,
        &source_offset,
        &source_length,
        &flags);
    if (status.native != 0) {
        return status;
    }

    uint64_t direct_child_count = 0u;
    uint64_t element_child_count = 0u;
    status = count_direct_observation_children(
        node,
        &direct_child_count,
        &element_child_count);
    if (status.native != 0) {
        return status;
    }

    lxb_dom_element_t *element = lxb_dom_interface_element(node);
    if (element == NULL) {
        return status_from_errno_value(EIO);
    }
    const uint64_t attribute_count = count_element_attributes(element);
    if (attribute_count == UINT64_MAX) {
        return status_from_errno_value(EOVERFLOW);
    }

    lxb_dom_node_t *parent = node->parent;
    while (parent != NULL && parent->type != LXB_DOM_NODE_TYPE_ELEMENT) {
        parent = parent->parent;
    }
    lxb_dom_node_t *grandparent = parent == NULL ? NULL : parent->parent;
    while (grandparent != NULL && grandparent->type != LXB_DOM_NODE_TYPE_ELEMENT) {
        grandparent = grandparent->parent;
    }

    arbor_view0_native_element_observation observation = {
        .standard_element_id = arbor_view0_native_g03_standard_element_id_from_lexbor(
            node->local_name,
            node->ns),
        .namespace_id = namespace_id_from_lexbor(node->ns),
        .source_offset = source_offset,
        .source_length = source_length,
        .parent_standard_element_id = parent == NULL
            ? ARBOR_VIEW0_NATIVE_ELEMENT_NONE
            : arbor_view0_native_g03_standard_element_id_from_lexbor(parent->local_name, parent->ns),
        .grandparent_standard_element_id = grandparent == NULL
            ? ARBOR_VIEW0_NATIVE_ELEMENT_NONE
            : arbor_view0_native_g03_standard_element_id_from_lexbor(
                grandparent->local_name,
                grandparent->ns),
        .depth = depth,
        .attribute_count = attribute_count,
        .direct_child_count = direct_child_count,
        .element_child_count = element_child_count,
        .flags = flags,
        .ancestor_bits = {0u, 0u},
        .local_name = node_local_name_span(node)
    };
    if (observation.local_name.data == NULL) {
        return status_from_errno_value(EIO);
    }
    ancestor_bits_from_counters(ancestor_counters, observation.ancestor_bits);

    *observation_out = observation;
    *element_out = element;
    return ok_status();
}

static arbor_status observe_one_element(
    arbor_span input,
    const arbor_view0_native_semantic_observer *observer,
    lxb_dom_node_t *node,
    uint64_t depth,
    const uint64_t ancestor_counters[114],
    arbor_view0_native_observation_counts *counts)
{
    if (observer == NULL || node == NULL || counts == NULL ||
        node->type != LXB_DOM_NODE_TYPE_ELEMENT) {
        return status_from_errno_value(EINVAL);
    }

    arbor_view0_native_element_observation observation;
    lxb_dom_element_t *element = NULL;
    arbor_status status = build_element_observation(
        input,
        node,
        depth,
        ancestor_counters,
        &observation,
        &element);
    if (status.native != 0) {
        return status;
    }

    if (observer->traversal_enter != NULL) {
        status = observer->traversal_enter(observer->context, &observation);
        if (status.native != 0) {
            return status;
        }
    }
    if (observer->element_begin != NULL) {
        status = observer->element_begin(observer->context, &observation);
        if (status.native != 0) {
            return status;
        }
    }
    status = emit_attribute_observations(observer, &observation, element, counts);
    if (status.native != 0) {
        return status;
    }
    status = emit_direct_child_observations(
        input,
        observer,
        &observation,
        node,
        observation.element_child_count,
        counts);
    if (status.native != 0) {
        return status;
    }
    if (observer->element_complete != NULL) {
        status = observer->element_complete(observer->context, &observation);
        if (status.native != 0) {
            return status;
        }
    }

    if (counts->element_count == UINT64_MAX) {
        return status_from_errno_value(EOVERFLOW);
    }
    counts->element_count += 1u;
    if ((observation.flags & ARBOR_VIEW0_NATIVE_ELEMENT_FLAG_SYNTHETIC) != 0u) {
        if (counts->synthetic_element_count == UINT64_MAX) {
            return status_from_errno_value(EOVERFLOW);
        }
        counts->synthetic_element_count += 1u;
    } else {
        if (counts->authored_element_count == UINT64_MAX) {
            return status_from_errno_value(EOVERFLOW);
        }
        counts->authored_element_count += 1u;
    }
    if (depth > counts->max_depth) {
        counts->max_depth = depth;
    }

    return ok_status();
}

static arbor_status observe_element_leave(
    arbor_span input,
    const arbor_view0_native_semantic_observer *observer,
    lxb_dom_node_t *node,
    uint64_t depth,
    const uint64_t ancestor_counters[114])
{
    if (observer == NULL || node == NULL || ancestor_counters == NULL ||
        node->type != LXB_DOM_NODE_TYPE_ELEMENT) {
        return status_from_errno_value(EINVAL);
    }
    if (observer->traversal_leave == NULL) {
        return ok_status();
    }

    arbor_view0_native_element_observation observation;
    lxb_dom_element_t *element = NULL;
    arbor_status status = build_element_observation(
        input,
        node,
        depth,
        ancestor_counters,
        &observation,
        &element);
    if (status.native != 0) {
        return status;
    }
    (void)element;
    return observer->traversal_leave(observer->context, &observation);
}

static arbor_status observe_document(
    arbor_span input,
    lxb_html_document_t *document,
    const arbor_view0_native_semantic_observer *observer,
    arbor_view0_native_observation_counts *counts_out)
{
    if (document == NULL || observer == NULL || counts_out == NULL) {
        return status_from_errno_value(EINVAL);
    }

    lxb_dom_document_t *dom_document = lxb_dom_interface_document(document);
    if (dom_document == NULL || dom_document->element == NULL) {
        return status_from_errno_value(EIO);
    }

    lxb_dom_node_t *root = lxb_dom_interface_node(dom_document->element);
    if (root == NULL || root->type != LXB_DOM_NODE_TYPE_ELEMENT) {
        return status_from_errno_value(EIO);
    }

    uint64_t ancestor_counters[114] = {0u};
    arbor_view0_native_observation_counts counts = {0u};
    lxb_dom_node_t *node = root;
    uint64_t depth = 0u;

    while (node != NULL) {
        arbor_status status = observe_one_element(
            input,
            observer,
            node,
            depth,
            ancestor_counters,
            &counts);
        if (status.native != 0) {
            return status;
        }

        const uint64_t current_id = arbor_view0_native_g03_standard_element_id_from_lexbor(
            node->local_name,
            node->ns);
        status = ancestor_counter_add(ancestor_counters, current_id);
        if (status.native != 0) {
            return status;
        }

        lxb_dom_node_t *child = first_element_child(node);
        if (child != NULL) {
            if (depth == UINT64_MAX) {
                return status_from_errno_value(EOVERFLOW);
            }
            depth += 1u;
            node = child;
            continue;
        }

        for (;;) {
            const uint64_t exiting_id = arbor_view0_native_g03_standard_element_id_from_lexbor(
                node->local_name,
                node->ns);
            status = ancestor_counter_remove(ancestor_counters, exiting_id);
            if (status.native != 0) {
                return status;
            }
            status = observe_element_leave(
                input,
                observer,
                node,
                depth,
                ancestor_counters);
            if (status.native != 0) {
                return status;
            }

            lxb_dom_node_t *sibling = next_element_sibling(node);
            if (sibling != NULL) {
                node = sibling;
                break;
            }

            lxb_dom_node_t *parent = node->parent;
            while (parent != NULL && parent->type != LXB_DOM_NODE_TYPE_ELEMENT) {
                parent = parent->parent;
            }
            if (parent == NULL) {
                node = NULL;
                break;
            }
            node = parent;
            if (depth == 0u) {
                return status_from_errno_value(EIO);
            }
            depth -= 1u;
        }
    }

    *counts_out = counts;
    return ok_status();
}

static arbor_status validate_observation_regions(
    arbor_span input,
    const arbor_view0_native_parse_counts *parse_counts_out,
    const arbor_view0_native_document_facts *facts_out,
    const arbor_view0_native_observation_counts *observation_counts_out)
{
    if (parse_counts_out == NULL || facts_out == NULL ||
        observation_counts_out == NULL ||
        !adapter_range_representable(parse_counts_out, sizeof(*parse_counts_out)) ||
        !adapter_range_representable(facts_out, sizeof(*facts_out)) ||
        !adapter_range_representable(
            observation_counts_out,
            sizeof(*observation_counts_out))) {
        return status_from_errno_value(EINVAL);
    }

    if (adapter_ranges_overlap(
            input.data,
            input.length,
            observation_counts_out,
            sizeof(*observation_counts_out)) ||
        adapter_ranges_overlap(
            parse_counts_out,
            sizeof(*parse_counts_out),
            observation_counts_out,
            sizeof(*observation_counts_out)) ||
        adapter_ranges_overlap(
            facts_out,
            sizeof(*facts_out),
            observation_counts_out,
            sizeof(*observation_counts_out))) {
        return status_from_errno_value(EINVAL);
    }
    return ok_status();
}

static arbor_status lexbor_process(
    arbor_span input,
    arbor_view0_native_diagnostic *diagnostics,
    uint64_t diagnostic_capacity,
    arbor_view0_native_parse_counts *counts_out,
    arbor_view0_native_document_facts *facts_out,
    bool publish_diagnostics,
    const arbor_view0_native_semantic_observer *observer,
    arbor_view0_native_observation_counts *observation_counts_out,
    const arbor_view0_native_parse_counts *expected_counts,
    const arbor_view0_native_document_facts *expected_facts)
{
    if ((expected_counts == NULL) != (expected_facts == NULL)) {
        return status_from_errno_value(EINVAL);
    }
    arbor_status region_status = validate_adapter_regions(
        input,
        diagnostics,
        diagnostic_capacity,
        counts_out,
        facts_out);
    if (region_status.native != 0) {
        return region_status;
    }

    lxb_html_parser_t *parser = lxb_html_parser_create();
    if (parser == NULL) {
        return status_from_errno_value(ENOMEM);
    }

    const lxb_status_t init_status = lxb_html_parser_init(parser);
    if (init_status != LXB_STATUS_OK) {
        (void)lxb_html_parser_destroy(parser);
        return status_from_errno_value(EIO);
    }

    /* VIEW0 V1N1 G04 R1C freezes the current development checker to the
     * scripting-disabled HTML parser semantics. Make the Lexbor mode explicit
     * instead of relying on its initialization default. A future enabled-mode
     * checker expansion must be reviewed across every semantic evaluator. */
    lxb_html_parser_scripting_set(parser, false);
    if (lxb_html_parser_scripting(parser)) {
        (void)lxb_html_parser_destroy(parser);
        return status_from_errno_value(EIO);
    }

    lxb_html_tokenizer_t *tokenizer = lxb_html_parser_tokenizer(parser);
    if (tokenizer == NULL) {
        (void)lxb_html_parser_destroy(parser);
        return status_from_errno_value(EIO);
    }

    lxb_html_tokenizer_keep_duplicate_set(tokenizer, false);
    lxb_html_tokenizer_input_validation_set(tokenizer, true);

    arbor_view0_native_document_facts facts = document_facts_initial();
    arbor_view0_native_source_capture capture = {
        .input = input,
        .downstream = NULL,
        .downstream_context = NULL,
        .facts = &facts,
        .observer = observer,
        .tree = NULL,
        .source_repair_failure = {0},
        .provenance = {
            .input_data = input.data,
            .input_length = input.length,
            .current_begin = NULL,
            .current_tag = LXB_TAG__UNDEF,
            .current_start = false,
            .current_assigned = false,
            .wrapper_seen = false,
            .insertion_wrapper_seen = false,
            .failed = false,
            .source_repair_tree = NULL,
            .source_repair_token = NULL,
            .source_repair_record = {0},
            .source_repair_active = false
        },
        .failed = false
    };

    lxb_html_document_t *document = NULL;
    if (observer == NULL) {
        if (tokenizer->callback_token_done == NULL) {
            (void)lxb_html_parser_destroy(parser);
            return status_from_errno_value(EIO);
        }
        capture.downstream = tokenizer->callback_token_done;
        capture.downstream_context =
            lxb_html_tokenizer_callback_token_done_ctx(tokenizer);
        lxb_html_tokenizer_callback_token_done_set(
            tokenizer,
            capture_token_done,
            &capture);

        document = lxb_html_parse(
            parser,
            (const lxb_char_t *)input.data,
            (size_t)input.length);
    } else {
        document = lxb_html_parse_chunk_begin(parser);
        if (document != NULL) {
            tokenizer = lxb_html_parser_tokenizer(parser);
            if (tokenizer == NULL || tokenizer->callback_token_done == NULL) {
                (void)lxb_html_document_destroy(document);
                (void)lxb_html_parser_destroy(parser);
                return status_from_errno_value(EIO);
            }
            capture.downstream = tokenizer->callback_token_done;
            capture.downstream_context =
                lxb_html_tokenizer_callback_token_done_ctx(tokenizer);
            capture.tree = lxb_html_parser_tree(parser);
            if (capture.tree == NULL ||
                capture.downstream_context != (void *)capture.tree) {
                (void)lxb_html_document_destroy(document);
                (void)lxb_html_parser_destroy(parser);
                return status_from_errno_value(EIO);
            }
            capture.provenance.source_repair_tree = capture.tree;
            document->dom_document.user = &capture.provenance;
            lxb_html_tokenizer_callback_token_done_set(
                tokenizer,
                capture_token_done,
                &capture);

            lxb_status_t parse_status = lxb_html_parse_chunk_process(
                parser,
                (const lxb_char_t *)input.data,
                (size_t)input.length);
            if (parse_status == LXB_STATUS_OK) {
                parse_status = lxb_html_parse_chunk_end(parser);
            }
            if (parse_status != LXB_STATUS_OK) {
                (void)lxb_html_document_destroy(document);
                document = NULL;
            }
        }
    }
    if (document == NULL) {
        arbor_status failure = capture.source_repair_failure.native != 0
            ? capture.source_repair_failure
            : status_from_errno_value(EIO);
        (void)lxb_html_parser_destroy(parser);
        return failure;
    }

    lxb_html_tree_t *tree = lxb_html_parser_tree(parser);
    const bool sr_requested = observer != NULL && observer->source_repair != NULL;
    if (tree == NULL || capture.failed || capture.provenance.failed ||
        (observer != NULL && !capture.provenance.wrapper_seen) ||
        (sr_requested && !capture.provenance.insertion_wrapper_seen)) {
        arbor_status failure = capture.source_repair_failure.native != 0
            ? capture.source_repair_failure
            : status_from_errno_value(EIO);
        (void)lxb_html_document_destroy(document);
        (void)lxb_html_parser_destroy(parser);
        return failure;
    }

    arbor_status status = collect_dom_facts(document, &facts);
    if (status.native != 0) {
        (void)lxb_html_document_destroy(document);
        (void)lxb_html_parser_destroy(parser);
        return status;
    }

    arbor_view0_native_observation_counts observation_counts = {0u};
    if (observer != NULL) {
        status = observe_document(input, document, observer, &observation_counts);
        if (status.native != 0) {
            document->dom_document.user = NULL;
            (void)lxb_html_document_destroy(document);
            (void)lxb_html_parser_destroy(parser);
            return status;
        }
    }

    const uint64_t tokenizer_count = tokenizer->parse_errors == NULL
        ? 0u
        : (uint64_t)tokenizer->parse_errors->length;
    const uint64_t tree_count = tree->parse_errors == NULL
        ? 0u
        : (uint64_t)tree->parse_errors->length;

    arbor_asm_result_u64 total = u64_add_checked(tokenizer_count, tree_count);
    if (total.status != 0) {
        (void)lxb_html_document_destroy(document);
        (void)lxb_html_parser_destroy(parser);
        return arbor_status_from_native(total.status);
    }

    const arbor_view0_native_parse_counts counts = {
        .tokenizer_error_count = tokenizer_count,
        .tree_error_count = tree_count
    };
    if (expected_counts != NULL &&
        (counts.tokenizer_error_count != expected_counts->tokenizer_error_count ||
         counts.tree_error_count != expected_counts->tree_error_count ||
         memcmp(&facts, expected_facts, sizeof(facts)) != 0)) {
        (void)lxb_html_document_destroy(document);
        (void)lxb_html_parser_destroy(parser);
        return status_from_errno_value(EIO);
    }

    if (publish_diagnostics) {
        if (total.value > diagnostic_capacity) {
            (void)lxb_html_document_destroy(document);
            (void)lxb_html_parser_destroy(parser);
            return status_from_errno_value(ENOSPC);
        }

        status = preflight_tokenizer_errors(input, tokenizer->parse_errors);
        if (status.native == 0) {
            status = preflight_tree_errors(input, tree->parse_errors);
        }
        if (status.native != 0) {
            (void)lxb_html_document_destroy(document);
            (void)lxb_html_parser_destroy(parser);
            return status;
        }

        uint64_t sequence = 0u;
        uint64_t index = 0u;
        fill_tokenizer_errors(
            input,
            tokenizer->parse_errors,
            diagnostics,
            &sequence,
            &index);
        fill_tree_errors(
            input,
            tree->parse_errors,
            diagnostics,
            &sequence,
            &index);
    }

    *counts_out = counts;
    *facts_out = facts;
    if (observer != NULL) {
        *observation_counts_out = observation_counts;
    }

    document->dom_document.user = NULL;
    (void)lxb_html_document_destroy(document);
    (void)lxb_html_parser_destroy(parser);
    return ok_status();
}


arbor_status arbor_view0_native_lexbor_measure(
    arbor_span input,
    arbor_view0_native_parse_counts *counts_out,
    arbor_view0_native_document_facts *facts_out)
{
    return lexbor_process(
        input, NULL, 0u, counts_out, facts_out, false, NULL, NULL, NULL, NULL);
}

arbor_status arbor_view0_native_lexbor_collect(
    arbor_span input,
    arbor_view0_native_diagnostic *diagnostics,
    uint64_t diagnostic_capacity,
    arbor_view0_native_parse_counts *counts_out,
    arbor_view0_native_document_facts *facts_out)
{
    return lexbor_process(
        input,
        diagnostics,
        diagnostic_capacity,
        counts_out,
        facts_out,
        true,
        NULL,
        NULL,
        NULL,
        NULL);
}

arbor_status arbor_view0_native_lexbor_collect_exact(
    arbor_span input,
    arbor_view0_native_diagnostic *diagnostics,
    uint64_t diagnostic_capacity,
    const arbor_view0_native_parse_counts *expected_counts,
    const arbor_view0_native_document_facts *expected_facts)
{
    if (expected_counts == NULL || expected_facts == NULL) {
        return status_from_errno_value(EINVAL);
    }
    arbor_view0_native_parse_counts counts = {0};
    arbor_view0_native_document_facts facts = {0};
    return lexbor_process(
        input,
        diagnostics,
        diagnostic_capacity,
        &counts,
        &facts,
        true,
        NULL,
        NULL,
        expected_counts,
        expected_facts);
}

arbor_status arbor_view0_native_lexbor_observe(
    arbor_span input,
    const arbor_view0_native_semantic_observer *observer,
    arbor_view0_native_parse_counts *parse_counts_out,
    arbor_view0_native_document_facts *facts_out,
    arbor_view0_native_observation_counts *observation_counts_out)
{
    if (observer == NULL) {
        return status_from_errno_value(EINVAL);
    }
    if (input.length > ARBOR_VIEW0_NATIVE_MAX_INPUT_BYTES) {
        return status_from_errno_value(E2BIG);
    }
    arbor_status status = validate_observation_regions(
        input,
        parse_counts_out,
        facts_out,
        observation_counts_out);
    if (status.native != 0) {
        return status;
    }

    return lexbor_process(
        input,
        NULL,
        0u,
        parse_counts_out,
        facts_out,
        false,
        observer,
        observation_counts_out,
        NULL,
        NULL);
}

static arbor_status validate_fragment_adapter_regions(
    arbor_span input,
    arbor_view0_native_diagnostic *diagnostics,
    uint64_t diagnostic_capacity,
    arbor_view0_native_parse_counts *counts_out,
    arbor_view0_native_observation_counts *observation_counts_out)
{
    if (counts_out == NULL ||
        diagnostic_capacity > ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS ||
        (diagnostic_capacity != 0u && diagnostics == NULL) ||
        !adapter_range_representable(input.data, input.length) ||
        !adapter_range_representable(counts_out, sizeof(*counts_out)) ||
        (observation_counts_out != NULL &&
         !adapter_range_representable(observation_counts_out, sizeof(*observation_counts_out)))) {
        return status_from_errno_value(EINVAL);
    }
    arbor_asm_result_u64 diagnostic_bytes = u64_mul_checked(
        diagnostic_capacity, (uint64_t)sizeof(*diagnostics));
    if (diagnostic_bytes.status != 0) return arbor_status_from_native(diagnostic_bytes.status);
    if (!adapter_range_representable(diagnostics, diagnostic_bytes.value))
        return status_from_errno_value(EINVAL);
    if (adapter_ranges_overlap(input.data, input.length, counts_out, sizeof(*counts_out)) ||
        adapter_ranges_overlap(input.data, input.length, diagnostics, diagnostic_bytes.value) ||
        adapter_ranges_overlap(counts_out, sizeof(*counts_out), diagnostics, diagnostic_bytes.value) ||
        (observation_counts_out != NULL &&
         (adapter_ranges_overlap(input.data, input.length, observation_counts_out, sizeof(*observation_counts_out)) ||
          adapter_ranges_overlap(counts_out, sizeof(*counts_out), observation_counts_out, sizeof(*observation_counts_out)) ||
          adapter_ranges_overlap(diagnostics, diagnostic_bytes.value, observation_counts_out, sizeof(*observation_counts_out))))) {
        return status_from_errno_value(EINVAL);
    }
    return ok_status();
}

static arbor_status lexbor_fragment_process(
    arbor_span input,
    arbor_view0_native_diagnostic *diagnostics,
    uint64_t diagnostic_capacity,
    arbor_view0_native_parse_counts *counts_out,
    bool publish_diagnostics,
    const arbor_view0_native_semantic_observer *observer,
    arbor_view0_native_observation_counts *observation_counts_out,
    const arbor_view0_native_parse_counts *expected_counts)
{
    arbor_status region_status = validate_fragment_adapter_regions(
        input, diagnostics, diagnostic_capacity, counts_out, observation_counts_out);
    if (region_status.native != 0) return region_status;
    if ((observer == NULL) != (observation_counts_out == NULL))
        return status_from_errno_value(EINVAL);

    lxb_html_parser_t *parser = lxb_html_parser_create();
    if (parser == NULL) return status_from_errno_value(ENOMEM);
    if (lxb_html_parser_init(parser) != LXB_STATUS_OK) {
        (void)lxb_html_parser_destroy(parser);
        return status_from_errno_value(EIO);
    }
    lxb_html_parser_scripting_set(parser, false);
    if (lxb_html_parser_scripting(parser)) {
        (void)lxb_html_parser_destroy(parser);
        return status_from_errno_value(EIO);
    }

    lxb_html_tokenizer_t *tokenizer = lxb_html_parser_tokenizer(parser);
    if (tokenizer == NULL) {
        (void)lxb_html_parser_destroy(parser);
        return status_from_errno_value(EIO);
    }
    lxb_html_tokenizer_keep_duplicate_set(tokenizer, false);
    lxb_html_tokenizer_input_validation_set(tokenizer, true);

    arbor_view0_native_document_facts dummy_facts = document_facts_initial();
    arbor_view0_native_source_capture capture = {
        .input = input,
        .downstream = NULL,
        .downstream_context = NULL,
        .facts = &dummy_facts,
        .observer = observer,
        .tree = NULL,
        .source_repair_failure = {0},
        .provenance = {
            .input_data = input.data,
            .input_length = input.length,
            .current_begin = NULL,
            .current_tag = LXB_TAG__UNDEF,
            .current_start = false,
            .current_assigned = false,
            .wrapper_seen = false,
            .insertion_wrapper_seen = false,
            .failed = false,
            .source_repair_tree = NULL,
            .source_repair_token = NULL,
            .source_repair_record = {0},
            .source_repair_active = false
        },
        .failed = false
    };

    lxb_status_t parse_status = lxb_html_parse_fragment_chunk_begin(
        parser, NULL, LXB_TAG_BODY, LXB_NS_HTML);
    if (parse_status != LXB_STATUS_OK) {
        (void)lxb_html_parser_destroy(parser);
        return status_from_errno_value(EIO);
    }

    tokenizer = lxb_html_parser_tokenizer(parser);
    capture.tree = lxb_html_parser_tree(parser);
    if (tokenizer == NULL || capture.tree == NULL ||
        tokenizer->callback_token_done == NULL || capture.tree->document == NULL) {
        (void)lxb_html_parser_destroy(parser);
        return status_from_errno_value(EIO);
    }
    capture.downstream = tokenizer->callback_token_done;
    capture.downstream_context = lxb_html_tokenizer_callback_token_done_ctx(tokenizer);
    if (capture.downstream_context != (void *)capture.tree) {
        (void)lxb_html_parser_destroy(parser);
        return status_from_errno_value(EIO);
    }
    capture.provenance.source_repair_tree = capture.tree;
    capture.tree->document->dom_document.user = &capture.provenance;
    lxb_html_tokenizer_callback_token_done_set(tokenizer, capture_token_done, &capture);

    parse_status = lxb_html_parse_fragment_chunk_process(
        parser, (const lxb_char_t *)input.data, (size_t)input.length);
    lxb_dom_node_t *root = NULL;
    if (parse_status == LXB_STATUS_OK)
        root = lxb_html_parse_fragment_chunk_end(parser);
    if (root == NULL || capture.failed || capture.provenance.failed) {
        arbor_status failure = capture.source_repair_failure.native != 0
            ? capture.source_repair_failure
            : status_from_errno_value(EIO);
        (void)lxb_html_parser_destroy(parser);
        return failure;
    }

    lxb_html_tree_t *tree = lxb_html_parser_tree(parser);
    if (tree == NULL) {
        lxb_html_document_destroy(lxb_html_interface_document(root->owner_document));
        (void)lxb_html_parser_destroy(parser);
        return status_from_errno_value(EIO);
    }

    arbor_view0_native_observation_counts observation_counts = {0};
    if (observer != NULL) {
        lxb_html_document_t *document = lxb_html_interface_document(root->owner_document);
        if (document == NULL) {
            (void)lxb_html_parser_destroy(parser);
            return status_from_errno_value(EIO);
        }
        arbor_status status = observe_document(input, document, observer, &observation_counts);
        if (status.native != 0) {
            document->dom_document.user = NULL;
            lxb_html_document_destroy(document);
            (void)lxb_html_parser_destroy(parser);
            return status;
        }
    }

    const uint64_t tokenizer_count = tokenizer->parse_errors == NULL
        ? 0u : (uint64_t)tokenizer->parse_errors->length;
    const uint64_t tree_count = tree->parse_errors == NULL
        ? 0u : (uint64_t)tree->parse_errors->length;
    const arbor_view0_native_parse_counts counts = {
        .tokenizer_error_count = tokenizer_count,
        .tree_error_count = tree_count
    };
    if (expected_counts != NULL &&
        (counts.tokenizer_error_count != expected_counts->tokenizer_error_count ||
         counts.tree_error_count != expected_counts->tree_error_count)) {
        lxb_html_document_t *document = lxb_html_interface_document(root->owner_document);
        if (document != NULL) document->dom_document.user = NULL;
        lxb_html_document_destroy(document);
        (void)lxb_html_parser_destroy(parser);
        return status_from_errno_value(EIO);
    }

    arbor_asm_result_u64 total = u64_add_checked(tokenizer_count, tree_count);
    if (total.status != 0 || (publish_diagnostics && total.value > diagnostic_capacity)) {
        lxb_html_document_t *document = lxb_html_interface_document(root->owner_document);
        if (document != NULL) document->dom_document.user = NULL;
        lxb_html_document_destroy(document);
        (void)lxb_html_parser_destroy(parser);
        return total.status != 0 ? arbor_status_from_native(total.status)
                                 : status_from_errno_value(ENOSPC);
    }
    if (publish_diagnostics) {
        arbor_status status = preflight_tokenizer_errors(input, tokenizer->parse_errors);
        if (status.native == 0) status = preflight_tree_errors(input, tree->parse_errors);
        if (status.native != 0) {
            lxb_html_document_t *document = lxb_html_interface_document(root->owner_document);
            if (document != NULL) document->dom_document.user = NULL;
            lxb_html_document_destroy(document);
            (void)lxb_html_parser_destroy(parser);
            return status;
        }
        uint64_t sequence = 0u;
        uint64_t index = 0u;
        fill_tokenizer_errors(input, tokenizer->parse_errors, diagnostics, &sequence, &index);
        fill_tree_errors(input, tree->parse_errors, diagnostics, &sequence, &index);
    }

    *counts_out = counts;
    if (observer != NULL) *observation_counts_out = observation_counts;
    lxb_html_document_t *document = lxb_html_interface_document(root->owner_document);
    if (document != NULL) document->dom_document.user = NULL;
    lxb_html_document_destroy(document);
    (void)lxb_html_parser_destroy(parser);
    return ok_status();
}

arbor_status arbor_view0_native_lexbor_observe_fragment_model(
    arbor_span input,
    const arbor_view0_native_semantic_observer *observer,
    arbor_view0_native_parse_counts *parse_counts_out,
    arbor_view0_native_observation_counts *observation_counts_out)
{
    if (observer == NULL || input.length > ARBOR_VIEW0_NATIVE_MAX_INPUT_BYTES)
        return status_from_errno_value(observer == NULL ? EINVAL : E2BIG);
    return lexbor_fragment_process(
        input, NULL, 0u, parse_counts_out, false, observer,
        observation_counts_out, NULL);
}

arbor_status arbor_view0_native_lexbor_fragment_collect_exact(
    arbor_span input,
    arbor_view0_native_diagnostic *diagnostics,
    uint64_t diagnostic_capacity,
    const arbor_view0_native_parse_counts *expected_counts)
{
    if (expected_counts == NULL) return status_from_errno_value(EINVAL);
    arbor_view0_native_parse_counts counts = {0};
    return lexbor_fragment_process(
        input, diagnostics, diagnostic_capacity, &counts, true, NULL, NULL,
        expected_counts);
}
