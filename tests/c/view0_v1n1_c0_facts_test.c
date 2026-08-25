#include <arborcore/view0_conformance/native.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

_Static_assert(sizeof(arbor_view0_native_document_facts) == 184u,
               "V1N1 C0 document-facts layout drift");

static arbor_span span_from_cstr(const char *text)
{
    return (arbor_span){(const uint8_t *)text, (uint64_t)strlen(text)};
}

static uint64_t nth_name_offset(const char *text, const char *needle, uint64_t ordinal)
{
    const char *cursor = text;
    for (uint64_t i = 0u; i < ordinal; ++i) {
        cursor = strstr(cursor, needle);
        if (cursor == NULL) {
            return ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE;
        }
        if (i + 1u != ordinal) {
            cursor += strlen(needle);
        }
    }

    return (uint64_t)(cursor - text) + 1u;
}

static int collect_facts(
    const char *html,
    arbor_view0_native_document_facts *facts_out,
    arbor_view0_native_parse_counts *counts_out)
{
    arbor_view0_native_diagnostic diagnostics[32] = {{0}};
    const arbor_status status = arbor_view0_native_lexbor_collect(
        span_from_cstr(html),
        diagnostics,
        32u,
        counts_out,
        facts_out);
    return status.native == 0 ? 0 : 1;
}

int main(void)
{
    static const char canonical[] =
        "<!doctype html><html><head><title>x</title><base href=\"/\"></head>"
        "<body><p>x</p></body></html>";
    arbor_view0_native_document_facts facts = {0};
    arbor_view0_native_parse_counts counts = {0};
    if (collect_facts(canonical, &facts, &counts) != 0 ||
        counts.tokenizer_error_count != 0u || counts.tree_error_count != 0u ||
        facts.source_doctype_count != 1u ||
        facts.source_first_doctype_keyword_offset != 2u ||
        facts.source_first_doctype_keyword_length != 7u ||
        facts.source_first_doctype_name_offset != 10u ||
        facts.source_first_doctype_name_length != 4u ||
        facts.source_first_doctype_external_keyword_offset != ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE ||
        facts.source_first_doctype_public_id_offset != ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE ||
        facts.source_first_doctype_system_id_offset != ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE ||
        facts.source_title_start_tag_count != 1u ||
        facts.source_base_start_tag_count != 1u ||
        facts.source_body_start_tag_count != 1u ||
        facts.source_second_title_start_tag_offset != ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE ||
        facts.source_second_base_start_tag_offset != ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE ||
        facts.source_second_body_start_tag_offset != ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE ||
        facts.dom_doctype_node_count != 1u ||
        facts.dom_html_document_element_count != 1u ||
        facts.dom_html_head_element_count != 1u ||
        facts.dom_html_body_element_count != 1u ||
        facts.dom_head_title_child_count != 1u ||
        facts.dom_head_base_child_count != 1u) {
        return 1;
    }

    static const char legacy_doctype[] =
        "<!DOCTYPE html SYSTEM \"about:legacy-compat\"><title>x</title>";
    facts = (arbor_view0_native_document_facts){0};
    counts = (arbor_view0_native_parse_counts){0};
    if (collect_facts(legacy_doctype, &facts, &counts) != 0 ||
        facts.source_first_doctype_name_offset != 10u ||
        facts.source_first_doctype_name_length != 4u ||
        facts.source_first_doctype_external_keyword_offset != 15u ||
        facts.source_first_doctype_external_keyword_length != 6u ||
        facts.source_first_doctype_public_id_offset != ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE ||
        facts.source_first_doctype_system_id_offset != 23u ||
        facts.source_first_doctype_system_id_length != 19u) {
        return 2;
    }

    static const char public_doctype[] =
        "<!DOCTYPE html PUBLIC \"foo\" \"bar\"><title>x</title>";
    facts = (arbor_view0_native_document_facts){0};
    counts = (arbor_view0_native_parse_counts){0};
    if (collect_facts(public_doctype, &facts, &counts) != 0 ||
        facts.source_first_doctype_external_keyword_offset != 15u ||
        facts.source_first_doctype_external_keyword_length != 6u ||
        facts.source_first_doctype_public_id_offset != 23u ||
        facts.source_first_doctype_public_id_length != 3u ||
        facts.source_first_doctype_system_id_offset != 29u ||
        facts.source_first_doctype_system_id_length != 3u) {
        return 3;
    }

    static const char omitted[] = "<!doctype html><title>x</title><p>ok</p>";
    facts = (arbor_view0_native_document_facts){0};
    counts = (arbor_view0_native_parse_counts){0};
    if (collect_facts(omitted, &facts, &counts) != 0 ||
        counts.tokenizer_error_count != 0u || counts.tree_error_count != 0u ||
        facts.source_body_start_tag_count != 0u ||
        facts.dom_html_document_element_count != 1u ||
        facts.dom_html_head_element_count != 1u ||
        facts.dom_html_body_element_count != 1u ||
        facts.dom_head_title_child_count != 1u) {
        return 4;
    }

    static const char duplicate_title[] =
        "<!doctype html><title>a</title><title>b</title><p>x</p>";
    facts = (arbor_view0_native_document_facts){0};
    counts = (arbor_view0_native_parse_counts){0};
    if (collect_facts(duplicate_title, &facts, &counts) != 0 ||
        facts.source_title_start_tag_count != 2u ||
        facts.source_second_title_start_tag_offset !=
            nth_name_offset(duplicate_title, "<title", 2u) ||
        facts.dom_head_title_child_count != 2u) {
        return 5;
    }

    static const char duplicate_base[] =
        "<!doctype html><title>x</title><base href=\"/\"><base href=\"/x\"><p>x</p>";
    facts = (arbor_view0_native_document_facts){0};
    counts = (arbor_view0_native_parse_counts){0};
    if (collect_facts(duplicate_base, &facts, &counts) != 0 ||
        facts.source_base_start_tag_count != 2u ||
        facts.source_second_base_start_tag_offset !=
            nth_name_offset(duplicate_base, "<base", 2u) ||
        facts.dom_head_base_child_count != 2u) {
        return 6;
    }

    static const char duplicate_body[] =
        "<!doctype html><html><head><title>x</title></head>"
        "<body></body><body></body></html>";
    facts = (arbor_view0_native_document_facts){0};
    counts = (arbor_view0_native_parse_counts){0};
    if (collect_facts(duplicate_body, &facts, &counts) != 0 ||
        facts.source_body_start_tag_count != 2u ||
        facts.source_second_body_start_tag_offset !=
            nth_name_offset(duplicate_body, "<body", 2u) ||
        facts.dom_html_body_element_count != 1u ||
        counts.tree_error_count == 0u) {
        return 7;
    }

    static const char raw_text[] =
        "<!doctype html><title>x</title>"
        "<script>const s=\"<body><base><title>\";</script><p>x</p>";
    facts = (arbor_view0_native_document_facts){0};
    counts = (arbor_view0_native_parse_counts){0};
    if (collect_facts(raw_text, &facts, &counts) != 0 ||
        facts.source_title_start_tag_count != 1u ||
        facts.source_base_start_tag_count != 0u ||
        facts.source_body_start_tag_count != 0u ||
        facts.dom_html_body_element_count != 1u) {
        return 8;
    }

    puts("PASS: VIEW0 V1N1 C0 Lexbor-independent source/DOM document facts and parser-repair evidence");
    return 0;
}
