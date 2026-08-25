#include <arborcore/view0_conformance/native.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct observed_element {
    uint64_t id;
    uint64_t ns;
    uint64_t source;
    uint64_t parent;
    uint64_t grandparent;
    uint64_t depth;
    uint64_t flags;
    uint64_t ancestors[2];
    char name[32];
} observed_element;

typedef struct observation_capture {
    observed_element elements[64];
    uint64_t element_count;
    uint64_t complete_count;
    uint64_t attribute_count;
    uint64_t child_count;
    uint64_t p_child_count;
    uint64_t p_child_kinds[8];
    uint64_t p_child_flags[8];
    uint64_t p_child_ids[8];
    char attribute_name[32];
    char attribute_value[32];
} observation_capture;

static arbor_status ok_status(void)
{
    return arbor_status_from_native(0);
}

static arbor_span span_from_cstr(const char *text)
{
    return (arbor_span){(const uint8_t *)text, (uint64_t)strlen(text)};
}

static void copy_span(char *dst, size_t capacity, arbor_span span)
{
    size_t length = (size_t)span.length;
    if (length >= capacity) {
        length = capacity - 1u;
    }
    if (length != 0u) {
        (void)memcpy(dst, span.data, length);
    }
    dst[length] = '\0';
}

static arbor_status capture_element_begin(
    void *context,
    const arbor_view0_native_element_observation *observation)
{
    observation_capture *capture = (observation_capture *)context;
    if (capture == NULL || observation == NULL || capture->element_count >= 64u) {
        return arbor_status_from_native(-1);
    }

    observed_element *out = &capture->elements[capture->element_count];
    out->id = observation->standard_element_id;
    out->ns = observation->namespace_id;
    out->source = observation->source_offset;
    out->parent = observation->parent_standard_element_id;
    out->grandparent = observation->grandparent_standard_element_id;
    out->depth = observation->depth;
    out->flags = observation->flags;
    out->ancestors[0] = observation->ancestor_bits[0];
    out->ancestors[1] = observation->ancestor_bits[1];
    copy_span(out->name, sizeof(out->name), observation->local_name);
    capture->element_count += 1u;
    return ok_status();
}

static arbor_status capture_attribute(
    void *context,
    const arbor_view0_native_attribute_observation *observation)
{
    observation_capture *capture = (observation_capture *)context;
    if (capture == NULL || observation == NULL) {
        return arbor_status_from_native(-1);
    }
    capture->attribute_count += 1u;
    if (capture->attribute_name[0] == '\0') {
        copy_span(capture->attribute_name, sizeof(capture->attribute_name), observation->local_name);
        copy_span(capture->attribute_value, sizeof(capture->attribute_value), observation->value);
    }
    return ok_status();
}

static arbor_status capture_direct_child(
    void *context,
    const arbor_view0_native_direct_child_observation *observation)
{
    observation_capture *capture = (observation_capture *)context;
    if (capture == NULL || observation == NULL) {
        return arbor_status_from_native(-1);
    }
    capture->child_count += 1u;
    if (observation->parent_standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_P &&
        capture->p_child_count < 8u) {
        uint64_t i = capture->p_child_count;
        capture->p_child_kinds[i] = observation->kind;
        capture->p_child_flags[i] = observation->flags;
        capture->p_child_ids[i] = observation->standard_element_id;
        capture->p_child_count += 1u;
    }
    return ok_status();
}

static arbor_status capture_element_complete(
    void *context,
    const arbor_view0_native_element_observation *observation)
{
    observation_capture *capture = (observation_capture *)context;
    if (capture == NULL || observation == NULL) {
        return arbor_status_from_native(-1);
    }
    capture->complete_count += 1u;
    return ok_status();
}

static int observe(
    const char *html,
    observation_capture *capture,
    arbor_view0_native_observation_counts *observation_counts,
    arbor_view0_native_parse_counts *parse_counts,
    arbor_view0_native_document_facts *facts)
{
    const arbor_view0_native_semantic_observer observer = {
        .context = capture,
        .element_begin = capture_element_begin,
        .attribute = capture_attribute,
        .direct_child = capture_direct_child,
        .element_complete = capture_element_complete
    };
    arbor_status status = arbor_view0_native_lexbor_observe(
        span_from_cstr(html),
        &observer,
        parse_counts,
        facts,
        observation_counts);
    return status.native == 0 ? 0 : 1;
}

static const observed_element *find_element(
    const observation_capture *capture,
    const char *name,
    uint64_t ordinal)
{
    uint64_t seen = 0u;
    for (uint64_t i = 0u; i < capture->element_count; ++i) {
        if (strcmp(capture->elements[i].name, name) == 0) {
            seen += 1u;
            if (seen == ordinal) {
                return &capture->elements[i];
            }
        }
    }
    return NULL;
}

static int ancestor_present(const observed_element *element, uint64_t id)
{
    if (element == NULL || id == 0u || id > 113u) {
        return 0;
    }
    const uint64_t bit = id - 1u;
    return (element->ancestors[bit / 64u] & (UINT64_C(1) << (bit % 64u))) != 0u;
}

int main(void)
{
    static const char basic[] =
        "<!doctype html><title>x</title><p id=\"a\"> \n<em>x</em>y</p>";
    observation_capture capture = {0};
    arbor_view0_native_observation_counts observation_counts = {0};
    arbor_view0_native_parse_counts parse_counts = {0};
    arbor_view0_native_document_facts facts = {0};
    if (observe(basic, &capture, &observation_counts, &parse_counts, &facts) != 0 ||
        parse_counts.tokenizer_error_count != 0u || parse_counts.tree_error_count != 0u ||
        observation_counts.element_count != 6u ||
        observation_counts.authored_element_count != 3u ||
        observation_counts.synthetic_element_count != 3u ||
        observation_counts.attribute_count != 1u ||
        observation_counts.direct_child_count != 9u ||
        observation_counts.max_depth != 3u ||
        capture.element_count != 6u || capture.complete_count != 6u ||
        capture.attribute_count != 1u || strcmp(capture.attribute_name, "id") != 0 ||
        strcmp(capture.attribute_value, "a") != 0 ||
        capture.p_child_count != 3u ||
        capture.p_child_kinds[0] != ARBOR_VIEW0_NATIVE_DIRECT_CHILD_TEXT ||
        (capture.p_child_flags[0] & ARBOR_VIEW0_NATIVE_CHILD_FLAG_TEXT_INTER_ELEMENT_WHITESPACE) == 0u ||
        capture.p_child_kinds[1] != ARBOR_VIEW0_NATIVE_DIRECT_CHILD_ELEMENT ||
        capture.p_child_ids[1] != ARBOR_VIEW0_NATIVE_ELEMENT_EM ||
        capture.p_child_kinds[2] != ARBOR_VIEW0_NATIVE_DIRECT_CHILD_TEXT ||
        (capture.p_child_flags[2] & ARBOR_VIEW0_NATIVE_CHILD_FLAG_TEXT_INTER_ELEMENT_WHITESPACE) != 0u) {
        return 1;
    }

    const observed_element *html = find_element(&capture, "html", 1u);
    const observed_element *head = find_element(&capture, "head", 1u);
    const observed_element *body = find_element(&capture, "body", 1u);
    const observed_element *title = find_element(&capture, "title", 1u);
    const observed_element *p = find_element(&capture, "p", 1u);
    const observed_element *em = find_element(&capture, "em", 1u);
    if (html == NULL || head == NULL || body == NULL || title == NULL || p == NULL || em == NULL ||
        html->id != ARBOR_VIEW0_NATIVE_ELEMENT_HTML ||
        head->id != ARBOR_VIEW0_NATIVE_ELEMENT_HEAD ||
        body->id != ARBOR_VIEW0_NATIVE_ELEMENT_BODY ||
        (html->flags & ARBOR_VIEW0_NATIVE_ELEMENT_FLAG_SYNTHETIC) == 0u ||
        (head->flags & ARBOR_VIEW0_NATIVE_ELEMENT_FLAG_SYNTHETIC) == 0u ||
        (body->flags & ARBOR_VIEW0_NATIVE_ELEMENT_FLAG_SYNTHETIC) == 0u ||
        title->source != 16u || p->source == ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE ||
        em->parent != ARBOR_VIEW0_NATIVE_ELEMENT_P ||
        em->grandparent != ARBOR_VIEW0_NATIVE_ELEMENT_BODY ||
        !ancestor_present(em, ARBOR_VIEW0_NATIVE_ELEMENT_BODY) ||
        !ancestor_present(em, ARBOR_VIEW0_NATIVE_ELEMENT_P) ||
        ancestor_present(em, ARBOR_VIEW0_NATIVE_ELEMENT_EM)) {
        return 2;
    }

    static const char foster[] =
        "<!doctype html><title>x</title><body><table><p>x</p></table></body>";
    capture = (observation_capture){0};
    observation_counts = (arbor_view0_native_observation_counts){0};
    parse_counts = (arbor_view0_native_parse_counts){0};
    facts = (arbor_view0_native_document_facts){0};
    if (observe(foster, &capture, &observation_counts, &parse_counts, &facts) != 0) {
        return 3;
    }
    const observed_element *table = find_element(&capture, "table", 1u);
    p = find_element(&capture, "p", 1u);
    if (table == NULL || p == NULL ||
        table->parent != ARBOR_VIEW0_NATIVE_ELEMENT_BODY ||
        p->parent != ARBOR_VIEW0_NATIVE_ELEMENT_BODY ||
        table->source != 38u || p->source != 45u) {
        return 4;
    }

    static const char implied_tbody[] =
        "<!doctype html><title>x</title><body><table><tr><td>x</td></tr></table></body>";
    capture = (observation_capture){0};
    observation_counts = (arbor_view0_native_observation_counts){0};
    parse_counts = (arbor_view0_native_parse_counts){0};
    facts = (arbor_view0_native_document_facts){0};
    if (observe(implied_tbody, &capture, &observation_counts, &parse_counts, &facts) != 0) {
        return 5;
    }
    const observed_element *tbody = find_element(&capture, "tbody", 1u);
    const observed_element *tr = find_element(&capture, "tr", 1u);
    if (tbody == NULL || tr == NULL ||
        (tbody->flags & ARBOR_VIEW0_NATIVE_ELEMENT_FLAG_SYNTHETIC) == 0u ||
        tr->source != 45u || tr->parent != ARBOR_VIEW0_NATIVE_ELEMENT_TBODY) {
        return 6;
    }

    static const char adoption[] =
        "<!doctype html><title>x</title><p><b><i>x</b>y</i></p>";
    capture = (observation_capture){0};
    observation_counts = (arbor_view0_native_observation_counts){0};
    parse_counts = (arbor_view0_native_parse_counts){0};
    facts = (arbor_view0_native_document_facts){0};
    if (observe(adoption, &capture, &observation_counts, &parse_counts, &facts) != 0) {
        return 7;
    }
    const observed_element *i1 = find_element(&capture, "i", 1u);
    const observed_element *i2 = find_element(&capture, "i", 2u);
    if (i1 == NULL || i2 == NULL || i1->source != 38u ||
        (i2->flags & ARBOR_VIEW0_NATIVE_ELEMENT_FLAG_SYNTHETIC) == 0u) {
        return 8;
    }

    static const char template_case[] =
        "<!doctype html><title>x</title><body><template><p>inside</p></template><p>outside</p></body>";
    capture = (observation_capture){0};
    observation_counts = (arbor_view0_native_observation_counts){0};
    parse_counts = (arbor_view0_native_parse_counts){0};
    facts = (arbor_view0_native_document_facts){0};
    if (observe(template_case, &capture, &observation_counts, &parse_counts, &facts) != 0 ||
        find_element(&capture, "template", 1u) == NULL ||
        find_element(&capture, "p", 1u) == NULL ||
        find_element(&capture, "p", 2u) != NULL) {
        return 9;
    }

    static const char foreign_custom[] =
        "<!doctype html><title>x</title><body><x-thing><svg><g></g></svg></x-thing></body>";
    capture = (observation_capture){0};
    observation_counts = (arbor_view0_native_observation_counts){0};
    parse_counts = (arbor_view0_native_parse_counts){0};
    facts = (arbor_view0_native_document_facts){0};
    if (observe(foreign_custom, &capture, &observation_counts, &parse_counts, &facts) != 0) {
        return 10;
    }
    const observed_element *custom = find_element(&capture, "x-thing", 1u);
    const observed_element *svg = find_element(&capture, "svg", 1u);
    const observed_element *g = find_element(&capture, "g", 1u);
    if (custom == NULL || svg == NULL || g == NULL ||
        custom->id != ARBOR_VIEW0_NATIVE_ELEMENT_NONE ||
        custom->ns != ARBOR_VIEW0_NATIVE_NAMESPACE_HTML ||
        svg->id != ARBOR_VIEW0_NATIVE_ELEMENT_NONE || svg->ns != ARBOR_VIEW0_NATIVE_NAMESPACE_SVG ||
        g->id != ARBOR_VIEW0_NATIVE_ELEMENT_NONE || g->ns != ARBOR_VIEW0_NATIVE_NAMESPACE_SVG) {
        return 11;
    }

    puts("PASS: VIEW0 V1N1 G03 C0 neutral observation, stable IDs, parser-repair provenance, template exclusion and direct-child streams");
    return 0;
}
