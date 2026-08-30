#include "g07.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef enum g07_attr_code {
    G07_ATTR_NONE = 0,
    G07_ATTR_BLOCKING,
    G07_ATTR_DOWNLOAD,
    G07_ATTR_HREF,
    G07_ATTR_PING,
    G07_ATTR_REL,
    G07_ATTR_TARGET,
    G07_ATTR_TITLE,
    G07_ATTR__COUNT
} g07_attr_code;

typedef struct g07_value {
    arbor_span span;
    bool present;
} g07_value;

typedef struct g07_source_attr {
    uint64_t owner_source_offset;
    uint32_t source_offset;
    uint32_t source_length;
    uint16_t code;
    uint16_t reserved;
} g07_source_attr;

typedef struct g07_current {
    uint64_t standard_element_id;
    uint64_t source_offset;
    uint64_t source_length;
    g07_value values[G07_ATTR__COUNT];
} g07_current;

typedef struct g07_context {
    arbor_view0_native_v1n2_g07_anchor *anchors;
    uint64_t anchor_capacity;
    bool collect;
    arbor_view0_native_v1n2_g07_evaluation evaluation;
    g07_source_attr source_attrs[ARBOR_VIEW0_NATIVE_V1N2_G07_MAX_SOURCE_ATTRIBUTES];
    uint64_t source_attr_count;
    g07_current current;
} g07_context;

typedef struct g07_relation {
    const char *name;
    uint8_t element_mask;
} g07_relation;

#define G07_REL_LINK UINT8_C(0x01)
#define G07_REL_HYPERLINK UINT8_C(0x02)
#define G07_REL_FORM UINT8_C(0x04)

static const g07_relation g07_relations[] = {
    {"alternate", G07_REL_LINK | G07_REL_HYPERLINK},
    {"author", G07_REL_LINK | G07_REL_HYPERLINK},
    {"bookmark", G07_REL_HYPERLINK},
    {"canonical", G07_REL_LINK},
    {"dns-prefetch", G07_REL_LINK},
    {"expect", G07_REL_LINK},
    {"external", G07_REL_HYPERLINK | G07_REL_FORM},
    {"help", G07_REL_LINK | G07_REL_HYPERLINK | G07_REL_FORM},
    {"icon", G07_REL_LINK},
    {"license", G07_REL_LINK | G07_REL_HYPERLINK | G07_REL_FORM},
    {"manifest", G07_REL_LINK},
    {"modulepreload", G07_REL_LINK},
    {"next", G07_REL_LINK | G07_REL_HYPERLINK | G07_REL_FORM},
    {"nofollow", G07_REL_HYPERLINK | G07_REL_FORM},
    {"noopener", G07_REL_HYPERLINK | G07_REL_FORM},
    {"noreferrer", G07_REL_HYPERLINK | G07_REL_FORM},
    {"opener", G07_REL_HYPERLINK | G07_REL_FORM},
    {"pingback", G07_REL_LINK},
    {"preconnect", G07_REL_LINK},
    {"prefetch", G07_REL_LINK},
    {"preload", G07_REL_LINK},
    {"prev", G07_REL_LINK | G07_REL_HYPERLINK | G07_REL_FORM},
    {"privacy-policy", G07_REL_LINK | G07_REL_HYPERLINK},
    {"search", G07_REL_LINK | G07_REL_HYPERLINK | G07_REL_FORM},
    {"stylesheet", G07_REL_LINK},
    {"tag", G07_REL_HYPERLINK},
    {"terms-of-service", G07_REL_LINK | G07_REL_HYPERLINK}
};

static arbor_status err_status(int value) {
    return arbor_status_from_native(-(int64_t)value);
}

static arbor_status ok_status(void) {
    return arbor_status_from_native(0);
}

static uint8_t ascii_lower(uint8_t byte) {
    return byte >= (uint8_t)'A' && byte <= (uint8_t)'Z'
        ? (uint8_t)(byte + ((uint8_t)'a' - (uint8_t)'A')) : byte;
}

static bool ascii_space(uint8_t byte) {
    return byte == UINT8_C(0x09) || byte == UINT8_C(0x0a) ||
           byte == UINT8_C(0x0c) || byte == UINT8_C(0x0d) ||
           byte == UINT8_C(0x20);
}

static bool ascii_hex(uint8_t byte) {
    const uint8_t lower = ascii_lower(byte);
    return (byte >= (uint8_t)'0' && byte <= (uint8_t)'9') ||
           (lower >= (uint8_t)'a' && lower <= (uint8_t)'f');
}

static bool span_eq_literal_ci(arbor_span span, const char *literal) {
    const size_t length = strlen(literal);
    if (span.data == NULL || span.length != (uint64_t)length) return false;
    for (size_t i = 0u; i < length; ++i) {
        if (ascii_lower(span.data[i]) != ascii_lower((uint8_t)literal[i])) return false;
    }
    return true;
}

static g07_attr_code attr_code(arbor_span name) {
    if (span_eq_literal_ci(name, "blocking")) return G07_ATTR_BLOCKING;
    if (span_eq_literal_ci(name, "download")) return G07_ATTR_DOWNLOAD;
    if (span_eq_literal_ci(name, "href")) return G07_ATTR_HREF;
    if (span_eq_literal_ci(name, "ping")) return G07_ATTR_PING;
    if (span_eq_literal_ci(name, "rel")) return G07_ATTR_REL;
    if (span_eq_literal_ci(name, "target")) return G07_ATTR_TARGET;
    if (span_eq_literal_ci(name, "title")) return G07_ATTR_TITLE;
    return G07_ATTR_NONE;
}

static arbor_span trimmed(arbor_span value) {
    uint64_t begin = 0u;
    uint64_t end = value.length;
    while (begin < end && ascii_space(value.data[begin])) ++begin;
    while (end > begin && ascii_space(value.data[end - 1u])) --end;
    return (arbor_span){value.data == NULL ? NULL : value.data + begin, end - begin};
}

static bool valid_url_string(arbor_span raw) {
    const arbor_span value = trimmed(raw);
    if (value.data == NULL || value.length == 0u) return false;
    for (uint64_t i = 0u; i < value.length; ++i) {
        const uint8_t byte = value.data[i];
        if (byte <= UINT8_C(0x20) || byte == UINT8_C(0x7f)) return false;
        if (byte == (uint8_t)'%' &&
            (i + 2u >= value.length || !ascii_hex(value.data[i + 1u]) ||
             !ascii_hex(value.data[i + 2u]))) return false;
    }
    if (value.length >= 7u &&
        (span_eq_literal_ci((arbor_span){value.data, 7u}, "http://") ||
         (value.length >= 8u && span_eq_literal_ci((arbor_span){value.data, 8u}, "https://")))) {
        const uint64_t start = ascii_lower(value.data[4u]) == (uint8_t)'s' ? 8u : 7u;
        if (value.length == start || value.data[start] == (uint8_t)'/' ||
            value.data[start] == (uint8_t)'?' || value.data[start] == (uint8_t)'#') return false;
    }
    return true;
}

static bool valid_http_url(arbor_span raw) {
    const arbor_span value = trimmed(raw);
    const bool scheme = value.length >= 7u &&
        (span_eq_literal_ci((arbor_span){value.data, 7u}, "http://") ||
         (value.length >= 8u && span_eq_literal_ci((arbor_span){value.data, 8u}, "https://")));
    return scheme && valid_url_string(value);
}

static bool valid_target(arbor_span value) {
    if (value.data == NULL || value.length == 0u) return false;
    for (uint64_t i = 0u; i < value.length; ++i) {
        if (value.data[i] == UINT8_C(0x09) || value.data[i] == UINT8_C(0x0a) ||
            value.data[i] == UINT8_C(0x0c) || value.data[i] == UINT8_C(0x0d)) return false;
    }
    if (value.data[0] != (uint8_t)'_') return true;
    return span_eq_literal_ci(value, "_blank") || span_eq_literal_ci(value, "_self") ||
           span_eq_literal_ci(value, "_parent") || span_eq_literal_ci(value, "_top") ||
           span_eq_literal_ci(value, "_unfencedtop");
}

static bool token_next(arbor_span value, uint64_t *cursor, arbor_span *token) {
    uint64_t i = *cursor;
    while (i < value.length && ascii_space(value.data[i])) ++i;
    if (i == value.length) { *cursor = i; return false; }
    const uint64_t start = i;
    while (i < value.length && !ascii_space(value.data[i])) ++i;
    *cursor = i;
    *token = (arbor_span){value.data + start, i - start};
    return true;
}

static bool token_contains(arbor_span value, const char *literal) {
    uint64_t cursor = 0u;
    arbor_span token = {0};
    while (token_next(value, &cursor, &token)) {
        if (span_eq_literal_ci(token, literal)) return true;
    }
    return false;
}

static const g07_relation *relation_for(arbor_span token) {
    for (size_t i = 0u; i < sizeof(g07_relations) / sizeof(g07_relations[0]); ++i) {
        if (span_eq_literal_ci(token, g07_relations[i].name)) return &g07_relations[i];
    }
    return NULL;
}

static uint8_t current_element_mask(const g07_current *current) {
    if (current->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_LINK) return G07_REL_LINK;
    if (current->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_A ||
        current->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_AREA) return G07_REL_HYPERLINK;
    if (current->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_FORM) return G07_REL_FORM;
    return UINT8_C(0);
}

static const g07_source_attr *source_attr_for(
    const g07_context *context, uint64_t owner_offset, g07_attr_code code) {
    for (uint64_t i = 0u; i < context->source_attr_count; ++i) {
        if (context->source_attrs[i].owner_source_offset == owner_offset &&
            context->source_attrs[i].code == (uint16_t)code) return &context->source_attrs[i];
    }
    return NULL;
}

static arbor_status emit(g07_context *context, uint16_t rule_ordinal, g07_attr_code code) {
    if (rule_ordinal == 0u || rule_ordinal > ARBOR_VIEW0_NATIVE_V1N2_G07_RULE_COUNT)
        return err_status(EIO);
    if (context->evaluation.diagnostic_count == UINT64_MAX ||
        context->evaluation.rule_violation_count[rule_ordinal - 1u] == UINT64_MAX)
        return err_status(EOVERFLOW);
    if (context->collect) {
        if (context->evaluation.diagnostic_count >= context->anchor_capacity)
            return err_status(ENOSPC);
        uint64_t offset = context->current.source_offset;
        uint64_t length = context->current.source_length;
        const g07_source_attr *attribute = source_attr_for(
            context, context->current.source_offset, code);
        if (attribute != NULL) {
            offset = attribute->source_offset;
            length = attribute->source_length;
        }
        if (offset > UINT32_MAX || length > UINT32_MAX) return err_status(EIO);
        context->anchors[context->evaluation.diagnostic_count].shared =
            (arbor_view0_native_v1n2_anchor){
                .byte_offset = offset,
                .source_length = length,
                .discovery_sequence = context->evaluation.diagnostic_count,
                .subject_index = context->current.source_offset,
                .group_ordinal = UINT16_C(1),
                .rule_ordinal = rule_ordinal,
                .kind = attribute == NULL
                    ? ARBOR_VIEW0_NATIVE_V1N2_ANCHOR_ELEMENT
                    : ARBOR_VIEW0_NATIVE_V1N2_ANCHOR_ATTRIBUTE_NAME
            };
    }
    context->evaluation.diagnostic_count += 1u;
    context->evaluation.rule_violation_count[rule_ordinal - 1u] += 1u;
    return ok_status();
}

static arbor_status source_attribute(
    void *opaque, const arbor_view0_native_source_attribute_observation *observation) {
    if (opaque == NULL || observation == NULL) return err_status(EINVAL);
    g07_context *context = (g07_context *)opaque;
    const g07_attr_code code = attr_code(observation->local_name);
    if (code == G07_ATTR_NONE) return ok_status();
    if (observation->owner_source_offset == ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE ||
        observation->source_offset == ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE ||
        observation->source_length == 0u || observation->source_offset > UINT32_MAX ||
        observation->source_length > UINT32_MAX) return err_status(EIO);
    if (source_attr_for(context, observation->owner_source_offset, code) != NULL)
        return ok_status();
    if (context->source_attr_count >= ARBOR_VIEW0_NATIVE_V1N2_G07_MAX_SOURCE_ATTRIBUTES)
        return err_status(ENOSPC);
    context->source_attrs[context->source_attr_count++] = (g07_source_attr){
        .owner_source_offset = observation->owner_source_offset,
        .source_offset = (uint32_t)observation->source_offset,
        .source_length = (uint32_t)observation->source_length,
        .code = (uint16_t)code
    };
    return ok_status();
}

static arbor_status element_begin(
    void *opaque, const arbor_view0_native_element_observation *observation) {
    if (opaque == NULL || observation == NULL) return err_status(EINVAL);
    g07_context *context = (g07_context *)opaque;
    (void)memset(&context->current, 0, sizeof(context->current));
    context->current.standard_element_id = observation->standard_element_id;
    context->current.source_offset = observation->source_offset;
    context->current.source_length = observation->source_length;
    return ok_status();
}

static arbor_status attribute(
    void *opaque, const arbor_view0_native_attribute_observation *observation) {
    if (opaque == NULL || observation == NULL) return err_status(EINVAL);
    g07_context *context = (g07_context *)opaque;
    const g07_attr_code code = attr_code(observation->local_name);
    if (code != G07_ATTR_NONE && !context->current.values[code].present) {
        context->current.values[code] = (g07_value){observation->value, true};
    }
    return ok_status();
}

static arbor_status evaluate_rel(g07_context *context) {
    const g07_current *current = &context->current;
    const uint8_t mask = current_element_mask(current);
    if (mask == 0u || !current->values[G07_ATTR_REL].present) return ok_status();
    if (context->evaluation.rel_consumer_count == UINT64_MAX) return err_status(EOVERFLOW);
    context->evaluation.rel_consumer_count += 1u;
    const arbor_span rel = current->values[G07_ATTR_REL].span;
    uint64_t cursor = 0u;
    arbor_span token = {0};
    bool applicability_violation = false;
    bool synonym_violation = false;
    while (token_next(rel, &cursor, &token)) {
        const g07_relation *known = relation_for(token);
        if (known != NULL) {
            if ((known->element_mask & mask) == 0u) applicability_violation = true;
        } else if (span_eq_literal_ci(token, "copyright") ||
                   span_eq_literal_ci(token, "previous")) {
            synonym_violation = true;
        } else if (span_eq_literal_ci(token, "shortcut")) {
            /* Frozen HTML authority admits only the exact historical
             * "shortcut icon" spelling, checked below. */
        } else {
            if (context->evaluation.extension_relation_deferred_count == UINT64_MAX)
                return err_status(EOVERFLOW);
            context->evaluation.extension_relation_deferred_count += 1u;
        }
    }
    arbor_status status = ok_status();
    if (applicability_violation) {
        status = emit(context, UINT16_C(4), G07_ATTR_REL);
        if (status.native != 0) return status;
    }
    const bool alt_stylesheet = token_contains(rel, "alternate") && token_contains(rel, "stylesheet");
    const bool title_missing = !current->values[G07_ATTR_TITLE].present ||
        trimmed(current->values[G07_ATTR_TITLE].span).length == 0u;
    const bool opener_conflict = token_contains(rel, "opener") &&
        (token_contains(rel, "noopener") || token_contains(rel, "noreferrer"));
    const bool shortcut_bad = token_contains(rel, "shortcut") &&
        !span_eq_literal_ci(rel, "shortcut icon");
    if (synonym_violation || (mask == G07_REL_LINK && alt_stylesheet && title_missing) ||
        opener_conflict || shortcut_bad) {
        status = emit(context, UINT16_C(5), G07_ATTR_REL);
        if (status.native != 0) return status;
    }
    return ok_status();
}

static arbor_status element_complete(
    void *opaque, const arbor_view0_native_element_observation *observation) {
    if (opaque == NULL || observation == NULL) return err_status(EINVAL);
    g07_context *context = (g07_context *)opaque;
    if (observation->standard_element_id != context->current.standard_element_id ||
        observation->source_offset != context->current.source_offset) return err_status(EIO);
    const bool hyperlink = context->current.standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_A ||
        context->current.standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_AREA;
    if (hyperlink) {
        if (context->evaluation.hyperlink_element_count == UINT64_MAX) return err_status(EOVERFLOW);
        context->evaluation.hyperlink_element_count += 1u;
        if (context->current.values[G07_ATTR_HREF].present &&
            (!valid_url_string(context->current.values[G07_ATTR_HREF].span) ||
             (context->current.values[G07_ATTR_TARGET].present &&
              !valid_target(context->current.values[G07_ATTR_TARGET].span)))) {
            const g07_attr_code code = !valid_url_string(context->current.values[G07_ATTR_HREF].span)
                ? G07_ATTR_HREF : G07_ATTR_TARGET;
            arbor_status status = emit(context, UINT16_C(1), code);
            if (status.native != 0) return status;
        }
        if (context->current.values[G07_ATTR_DOWNLOAD].present &&
            !context->current.values[G07_ATTR_HREF].present) {
            arbor_status status = emit(context, UINT16_C(2), G07_ATTR_DOWNLOAD);
            if (status.native != 0) return status;
        }
        if (context->current.values[G07_ATTR_PING].present) {
            bool invalid = !context->current.values[G07_ATTR_HREF].present;
            uint64_t cursor = 0u;
            uint64_t token_count = 0u;
            arbor_span token = {0};
            while (token_next(context->current.values[G07_ATTR_PING].span, &cursor, &token)) {
                ++token_count;
                if (!valid_http_url(token)) invalid = true;
            }
            if (token_count == 0u) invalid = true;
            if (invalid) {
                arbor_status status = emit(context, UINT16_C(3), G07_ATTR_PING);
                if (status.native != 0) return status;
            }
        }
    }
    return evaluate_rel(context);
}

static arbor_status evaluate(
    arbor_span input, arbor_view0_native_v1n2_g07_anchor *anchors,
    uint64_t anchor_capacity, bool collect,
    arbor_view0_native_v1n2_g07_evaluation *evaluation_out) {
    if (evaluation_out == NULL || (collect && anchor_capacity != 0u && anchors == NULL))
        return err_status(EINVAL);
    g07_context context = {.anchors = anchors, .anchor_capacity = anchor_capacity, .collect = collect};
    const arbor_view0_native_semantic_observer observer = {
        .context = &context,
        .element_begin = element_begin,
        .attribute = attribute,
        .element_complete = element_complete,
        .source_attribute = source_attribute
    };
    arbor_view0_native_parse_counts parse_counts = {0};
    arbor_view0_native_document_facts facts = {0};
    arbor_view0_native_observation_counts counts = {0};
    arbor_status status = arbor_view0_native_lexbor_observe(
        input, &observer, &parse_counts, &facts, &counts);
    if (status.native != 0) return status;
    if (collect && context.evaluation.diagnostic_count != anchor_capacity)
        return err_status(EIO);
    *evaluation_out = context.evaluation;
    return ok_status();
}

arbor_status arbor_view0_native_v1n2_g07_measure(
    arbor_span input, arbor_view0_native_v1n2_g07_evaluation *evaluation_out) {
    return evaluate(input, NULL, 0u, false, evaluation_out);
}

arbor_status arbor_view0_native_v1n2_g07_collect_anchors(
    arbor_span input, arbor_view0_native_v1n2_g07_anchor *anchors,
    uint64_t anchor_capacity, arbor_view0_native_v1n2_g07_evaluation *evaluation_out) {
    return evaluate(input, anchors, anchor_capacity, true, evaluation_out);
}

void arbor_view0_native_v1n2_g07_materialize_anchor(
    const arbor_view0_native_v1n2_g07_anchor *anchor,
    uint64_t discovery_sequence, arbor_view0_native_diagnostic *diagnostic) {
    static const char *const messages[] = {
        "Hyperlink href or target does not satisfy the frozen static authoring contract",
        "Download attribute requires a hyperlink-bearing href attribute",
        "Ping attribute must contain only valid non-empty HTTP(S) URLs on a hyperlink",
        "Link relation is not admitted on this element by the frozen applicability matrix",
        "Link relation violates a frozen companion, exclusion, or synonym requirement"
    };
    if (anchor == NULL || diagnostic == NULL || anchor->shared.rule_ordinal == 0u ||
        anchor->shared.rule_ordinal > ARBOR_VIEW0_NATIVE_V1N2_G07_RULE_COUNT) return;
    const arbor_view0_native_v1n2_rule_meta *meta =
        arbor_view0_native_v1n2_c0_rule_at((uint64_t)anchor->shared.rule_ordinal - 1u);
    if (meta == NULL || meta->group != ARBOR_VIEW0_NATIVE_V1N2_GROUP_G07) return;
    (void)memset(diagnostic, 0, sizeof(*diagnostic));
    diagnostic->rule_id = meta->rule_id;
    diagnostic->byte_offset = anchor->shared.byte_offset;
    diagnostic->source_length = anchor->shared.source_length;
    diagnostic->discovery_sequence = discovery_sequence;
    diagnostic->severity = (uint32_t)ARBOR_VIEW0_NATIVE_SEVERITY_ERROR;
    diagnostic->origin = (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_AUTHORING;
    (void)memcpy(diagnostic->symbolic_name, meta->rule_symbol, strlen(meta->rule_symbol) + 1u);
    (void)memcpy(diagnostic->message, messages[anchor->shared.rule_ordinal - 1u],
                 strlen(messages[anchor->shared.rule_ordinal - 1u]) + 1u);
}
