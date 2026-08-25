#include <arborcore/view0_conformance/native.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct sr_capture {
    arbor_view0_native_source_repair_context records[64];
    uint64_t count;
} sr_capture;

static arbor_span span_from_cstr(const char *text)
{
    return (arbor_span){(const uint8_t *)text, (uint64_t)strlen(text)};
}

static arbor_status capture_sr(
    void *context,
    const arbor_view0_native_source_repair_context *record)
{
    sr_capture *capture = (sr_capture *)context;
    if (capture == NULL || record == NULL || capture->count >= 64u) {
        return arbor_status_from_native(-1);
    }
    capture->records[capture->count] = *record;
    capture->count += 1u;
    return arbor_status_from_native(0);
}

static int observe(const char *html, sr_capture *capture)
{
    arbor_view0_native_parse_counts parse_counts = {0};
    arbor_view0_native_document_facts facts = {0};
    arbor_view0_native_observation_counts observation_counts = {0};
    const arbor_view0_native_semantic_observer observer = {
        .context = capture,
        .source_repair = capture_sr
    };
    arbor_status status = arbor_view0_native_lexbor_observe(
        span_from_cstr(html), &observer, &parse_counts, &facts, &observation_counts);
    return status.native == 0 ? 0 : 1;
}

static const arbor_view0_native_source_repair_context *find_record(
    const sr_capture *capture,
    uint64_t element_id,
    uint64_t source_offset)
{
    if (capture == NULL) return NULL;
    for (uint64_t i = 0u; i < capture->count; ++i) {
        if (capture->records[i].standard_element_id == element_id &&
            capture->records[i].source_offset == source_offset) {
            return &capture->records[i];
        }
    }
    return NULL;
}

static int check_body_p(void)
{
    static const char html[] = "<!doctype html><title>x</title><body><p>x</p></body>";
    sr_capture capture = {0};
    if (observe(html, &capture) != 0) return 1;
    const arbor_view0_native_source_repair_context *r =
        find_record(&capture, ARBOR_VIEW0_NATIVE_ELEMENT_P, 38u);
    if (r == NULL || r->source_length != 1u ||
        r->initial_current_standard_element_id != ARBOR_VIEW0_NATIVE_ELEMENT_BODY ||
        r->initial_current_source_offset != 32u ||
        r->initial_insertion_mode_id != ARBOR_VIEW0_NATIVE_INSERTION_MODE_IN_BODY ||
        r->initial_open_elements_depth != 2u ||
        r->insertion_seen != 1u ||
        r->insertion_current_standard_element_id != ARBOR_VIEW0_NATIVE_ELEMENT_BODY ||
        r->insertion_current_source_offset != 32u ||
        r->insertion_mode_id != ARBOR_VIEW0_NATIVE_INSERTION_MODE_IN_BODY ||
        r->insertion_open_elements_depth != 2u || r->insertion_flags != 0u) return 2;
    return 0;
}

static int check_omitted_body_p(void)
{
    static const char html[] = "<!doctype html><title>x</title><p>x</p>";
    sr_capture capture = {0};
    if (observe(html, &capture) != 0) return 1;
    const arbor_view0_native_source_repair_context *r =
        find_record(&capture, ARBOR_VIEW0_NATIVE_ELEMENT_P, 32u);
    if (r == NULL ||
        r->initial_current_standard_element_id != ARBOR_VIEW0_NATIVE_ELEMENT_HEAD ||
        r->initial_current_source_offset != ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE ||
        r->initial_insertion_mode_id != ARBOR_VIEW0_NATIVE_INSERTION_MODE_IN_HEAD ||
        r->initial_open_elements_depth != 2u ||
        r->insertion_seen != 1u ||
        r->insertion_current_standard_element_id != ARBOR_VIEW0_NATIVE_ELEMENT_BODY ||
        r->insertion_current_source_offset != ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE ||
        r->insertion_mode_id != ARBOR_VIEW0_NATIVE_INSERTION_MODE_IN_BODY ||
        r->insertion_open_elements_depth != 2u || r->insertion_flags != 0u) return 2;
    return 0;
}

static int check_table_foster(const char *html, uint64_t element_id, uint64_t length)
{
    sr_capture capture = {0};
    if (observe(html, &capture) != 0) return 1;
    const arbor_view0_native_source_repair_context *r =
        find_record(&capture, element_id, 45u);
    if (r == NULL || r->source_length != length ||
        r->initial_current_standard_element_id != ARBOR_VIEW0_NATIVE_ELEMENT_TABLE ||
        r->initial_current_source_offset != 38u ||
        r->initial_insertion_mode_id != ARBOR_VIEW0_NATIVE_INSERTION_MODE_IN_TABLE ||
        r->initial_open_elements_depth != 3u ||
        r->insertion_seen != 1u ||
        r->insertion_current_standard_element_id != ARBOR_VIEW0_NATIVE_ELEMENT_TABLE ||
        r->insertion_current_source_offset != 38u ||
        r->insertion_mode_id != ARBOR_VIEW0_NATIVE_INSERTION_MODE_IN_TABLE ||
        r->insertion_open_elements_depth != 3u ||
        r->insertion_flags != ARBOR_VIEW0_NATIVE_SOURCE_REPAIR_FLAG_FOSTER_PARENTING) return 2;
    return 0;
}

static int check_table_tr(void)
{
    static const char html[] = "<!doctype html><title>x</title><body><table><tr><td>x</td></tr></table></body>";
    sr_capture capture = {0};
    if (observe(html, &capture) != 0) return 1;
    const arbor_view0_native_source_repair_context *r =
        find_record(&capture, ARBOR_VIEW0_NATIVE_ELEMENT_TR, 45u);
    if (r == NULL || r->source_length != 2u ||
        r->initial_current_standard_element_id != ARBOR_VIEW0_NATIVE_ELEMENT_TABLE ||
        r->initial_current_source_offset != 38u ||
        r->initial_insertion_mode_id != ARBOR_VIEW0_NATIVE_INSERTION_MODE_IN_TABLE ||
        r->initial_open_elements_depth != 3u ||
        r->insertion_seen != 1u ||
        r->insertion_current_standard_element_id != ARBOR_VIEW0_NATIVE_ELEMENT_TBODY ||
        r->insertion_current_source_offset != ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE ||
        r->insertion_mode_id != ARBOR_VIEW0_NATIVE_INSERTION_MODE_IN_TABLE_BODY ||
        r->insertion_open_elements_depth != 4u || r->insertion_flags != 0u) return 2;
    return 0;
}

static int check_duplicate_body_no_insertion(void)
{
    static const char html[] = "<!doctype html><title>x</title><body><p>x</p><body></body></body>";
    const char *second = strstr(strstr(html, "<body") + 1, "<body");
    if (second == NULL) return 1;
    const uint64_t second_offset = (uint64_t)((second + 1) - html);
    sr_capture capture = {0};
    if (observe(html, &capture) != 0) return 2;
    const arbor_view0_native_source_repair_context *r =
        find_record(&capture, ARBOR_VIEW0_NATIVE_ELEMENT_BODY, second_offset);
    if (r == NULL || r->insertion_seen != 0u ||
        r->insertion_current_standard_element_id != ARBOR_VIEW0_NATIVE_ELEMENT_NONE ||
        r->insertion_current_source_offset != ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE ||
        r->insertion_mode_id != ARBOR_VIEW0_NATIVE_INSERTION_MODE_NONE ||
        r->insertion_open_elements_depth != 0u || r->insertion_flags != 0u) return 3;
    return 0;
}

int main(void)
{
    if (sizeof(arbor_view0_native_source_repair_context) != 104u) return 1;
    if (check_body_p() != 0) return 2;
    if (check_omitted_body_p() != 0) return 3;
    static const char table_p[] = "<!doctype html><title>x</title><body><table><p>x</p></table></body>";
    if (check_table_foster(table_p, ARBOR_VIEW0_NATIVE_ELEMENT_P, 1u) != 0) return 4;
    static const char table_div[] = "<!doctype html><title>x</title><body><table><div>x</div></table></body>";
    if (check_table_foster(table_div, ARBOR_VIEW0_NATIVE_ELEMENT_DIV, 3u) != 0) return 5;
    if (check_table_tr() != 0) return 6;
    if (check_duplicate_body_no_insertion() != 0) return 7;
    puts("PASS: VIEW0 V1N1 G03 C0-SR1 single-parser source/repair context controls");
    return 0;
}
