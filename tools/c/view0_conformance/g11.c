#include "g11.h"

#include <lexbor/core/mraw.h>

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef enum g11_attr_code {
    G11_ATTR_NONE = 0,
    G11_ATTR_NAME,
    G11_ATTR_OPEN,
    G11_ATTR_CLOSEDBY,
    G11_ATTR_POPOVER,
    G11_ATTR_TABINDEX,
    G11_ATTR_AUTOFOCUS,
    G11_ATTR__COUNT
} g11_attr_code;

typedef struct g11_value {
    arbor_span span;
    bool present;
} g11_value;

typedef struct g11_node g11_node;
typedef struct g11_source_attr g11_source_attr;

struct g11_node {
    g11_node *next;
    g11_node *parent_details;
    uint64_t element_id;
    uint64_t source_offset;
    uint64_t source_length;
    uint64_t depth;
    uint64_t flags;
    g11_value values[G11_ATTR__COUNT];
};

struct g11_source_attr {
    g11_source_attr *next;
    uint64_t owner_source_offset;
    uint32_t source_offset;
    uint32_t source_length;
    uint16_t code;
};

typedef struct g11_frame {
    g11_node *node;
    g11_node *prior_details;
    uint64_t source_offset;
    uint64_t depth;
} g11_frame;

typedef struct g11_context {
    lexbor_mraw_t *arena;
    arbor_view0_native_v1n2_g11_anchor *anchors;
    uint64_t anchor_capacity;
    bool collect;
    arbor_view0_native_v1n2_g11_evaluation evaluation;
    g11_node *nodes;
    g11_node *nodes_tail;
    g11_node *current;
    g11_node *current_details;
    g11_source_attr *source_attrs;
    g11_source_attr *source_attrs_tail;
    g11_frame frames[ARBOR_VIEW0_NATIVE_V1N2_G11_MAX_DEPTH];
    uint64_t frame_count;
} g11_context;

static arbor_status ok_status(void) { return arbor_status_from_native(0); }
static arbor_status err_status(int value) { return arbor_status_from_native(-(int64_t)value); }

static void *support_calloc(g11_context *context, size_t size) {
    return arbor_view0_native_v1n2_g11_support_calloc(context->arena, size);
}

static uint8_t ascii_lower(uint8_t byte) {
    return byte >= (uint8_t)'A' && byte <= (uint8_t)'Z'
        ? (uint8_t)(byte + ((uint8_t)'a' - (uint8_t)'A')) : byte;
}

static bool span_eq_literal_ci(arbor_span span, const char *literal) {
    const size_t length = strlen(literal);
    if (span.data == NULL || span.length != (uint64_t)length) return false;
    for (size_t index = 0u; index < length; ++index)
        if (ascii_lower(span.data[index]) != ascii_lower((uint8_t)literal[index])) return false;
    return true;
}

static bool span_eq(arbor_span left, arbor_span right) {
    return left.length == right.length && left.data != NULL && right.data != NULL &&
        (left.length == 0u || memcmp(left.data, right.data, (size_t)left.length) == 0);
}

static g11_attr_code attr_code(arbor_span name) {
    if (span_eq_literal_ci(name, "name")) return G11_ATTR_NAME;
    if (span_eq_literal_ci(name, "open")) return G11_ATTR_OPEN;
    if (span_eq_literal_ci(name, "closedby")) return G11_ATTR_CLOSEDBY;
    if (span_eq_literal_ci(name, "popover")) return G11_ATTR_POPOVER;
    if (span_eq_literal_ci(name, "tabindex")) return G11_ATTR_TABINDEX;
    if (span_eq_literal_ci(name, "autofocus")) return G11_ATTR_AUTOFOCUS;
    return G11_ATTR_NONE;
}

static bool target_element(uint64_t element_id) {
    return element_id == ARBOR_VIEW0_NATIVE_ELEMENT_DETAILS ||
        element_id == ARBOR_VIEW0_NATIVE_ELEMENT_SUMMARY ||
        element_id == ARBOR_VIEW0_NATIVE_ELEMENT_DIALOG;
}

static const g11_source_attr *source_attr_for(
    const g11_context *context, uint64_t owner_offset, g11_attr_code code) {
    for (const g11_source_attr *entry = context->source_attrs;
         entry != NULL; entry = entry->next)
        if (entry->owner_source_offset == owner_offset && entry->code == (uint16_t)code)
            return entry;
    return NULL;
}

static arbor_status emit(
    g11_context *context, const g11_node *node, uint16_t rule_ordinal, g11_attr_code code) {
    if (node == NULL || rule_ordinal == 0u ||
        rule_ordinal > ARBOR_VIEW0_NATIVE_V1N2_G11_RULE_COUNT) return err_status(EIO);
    if (context->evaluation.diagnostic_count == UINT64_MAX ||
        context->evaluation.rule_violation_count[rule_ordinal - 1u] == UINT64_MAX)
        return err_status(EOVERFLOW);
    if (context->collect) {
        if (context->evaluation.diagnostic_count >= context->anchor_capacity)
            return err_status(ENOSPC);
        uint64_t offset = node->source_offset;
        uint64_t length = node->source_length;
        const g11_source_attr *attribute = source_attr_for(context, node->source_offset, code);
        if (attribute != NULL) {
            offset = attribute->source_offset;
            length = attribute->source_length;
        }
        if (offset == ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE ||
            offset > UINT32_MAX || length > UINT32_MAX) return err_status(EIO);
        context->anchors[context->evaluation.diagnostic_count].shared =
            (arbor_view0_native_v1n2_anchor){
                .byte_offset = offset,
                .source_length = length,
                .discovery_sequence = context->evaluation.diagnostic_count,
                .subject_index = node->source_offset,
                .group_ordinal = UINT16_C(5),
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

static arbor_status evaluate_details(g11_context *context, g11_node *node) {
    if (context->evaluation.details_count == UINT64_MAX) return err_status(EOVERFLOW);
    context->evaluation.details_count += 1u;
    if (!node->values[G11_ATTR_NAME].present) return ok_status();
    const arbor_span name = node->values[G11_ATTR_NAME].span;
    if (name.length == 0u) return emit(context, node, UINT16_C(1), G11_ATTR_NAME);

    bool earlier_open = false;
    for (const g11_node *other = context->nodes; other != node; other = other->next) {
        if (other->element_id != ARBOR_VIEW0_NATIVE_ELEMENT_DETAILS ||
            !other->values[G11_ATTR_NAME].present ||
            other->values[G11_ATTR_NAME].span.length == 0u ||
            !span_eq(name, other->values[G11_ATTR_NAME].span)) continue;
        if (context->evaluation.name_group_relation_count == UINT64_MAX)
            return err_status(EOVERFLOW);
        context->evaluation.name_group_relation_count += 1u;
        if (node->values[G11_ATTR_OPEN].present && other->values[G11_ATTR_OPEN].present)
            earlier_open = true;
    }
    arbor_status status = ok_status();
    if (earlier_open) {
        status = emit(context, node, UINT16_C(1), G11_ATTR_OPEN);
        if (status.native != 0) return status;
    }
    for (const g11_node *ancestor = node->parent_details;
         ancestor != NULL; ancestor = ancestor->parent_details) {
        if (ancestor->values[G11_ATTR_NAME].present &&
            ancestor->values[G11_ATTR_NAME].span.length != 0u &&
            span_eq(name, ancestor->values[G11_ATTR_NAME].span))
            return emit(context, node, UINT16_C(1), G11_ATTR_NAME);
    }
    return status;
}

static arbor_status evaluate_dialog(g11_context *context, g11_node *node) {
    if (context->evaluation.dialog_count == UINT64_MAX) return err_status(EOVERFLOW);
    context->evaluation.dialog_count += 1u;
    if (node->values[G11_ATTR_TABINDEX].present) {
        if (context->evaluation.prior_owner_suppression_count == UINT64_MAX)
            return err_status(EOVERFLOW);
        context->evaluation.prior_owner_suppression_count += 1u;
    }
    if (node->values[G11_ATTR_CLOSEDBY].present) {
        if (context->evaluation.prior_owner_suppression_count == UINT64_MAX)
            return err_status(EOVERFLOW);
        context->evaluation.prior_owner_suppression_count += 1u;
    }
    /* The pinned source permits dialog to carry the global popover attribute,
     * including alongside authored open. Showing-state conflicts exist only
     * inside runtime algorithms and are not inferred from those attributes.
     * G05 owns the dialog tabindex prohibition and G06 owns open/closedby
     * microsyntax, so R2 publishes no duplicate diagnostic here. */
    if (node->values[G11_ATTR_OPEN].present ||
        node->values[G11_ATTR_POPOVER].present ||
        node->values[G11_ATTR_AUTOFOCUS].present) {
        if (context->evaluation.deferred_external_semantics_count == UINT64_MAX)
            return err_status(EOVERFLOW);
        context->evaluation.deferred_external_semantics_count += 1u;
    }
    return ok_status();
}

static arbor_status evaluate_all(g11_context *context) {
    for (g11_node *node = context->nodes; node != NULL; node = node->next) {
        if ((node->flags & ARBOR_VIEW0_NATIVE_ELEMENT_FLAG_SYNTHETIC) != 0u) continue;
        arbor_status status = ok_status();
        if (node->element_id == ARBOR_VIEW0_NATIVE_ELEMENT_DETAILS)
            status = evaluate_details(context, node);
        else if (node->element_id == ARBOR_VIEW0_NATIVE_ELEMENT_DIALOG)
            status = evaluate_dialog(context, node);
        else if (node->element_id == ARBOR_VIEW0_NATIVE_ELEMENT_SUMMARY) {
            if (context->evaluation.prior_owner_suppression_count == UINT64_MAX)
                return err_status(EOVERFLOW);
            context->evaluation.prior_owner_suppression_count += 1u;
        }
        if (status.native != 0) return status;
    }
    return ok_status();
}

static arbor_status traversal_enter(
    void *opaque, const arbor_view0_native_element_observation *observation) {
    if (opaque == NULL || observation == NULL) return err_status(EINVAL);
    g11_context *context = opaque;
    if (context->frame_count >= ARBOR_VIEW0_NATIVE_V1N2_G11_MAX_DEPTH)
        return err_status(ENOSPC);
    g11_node *node = NULL;
    if (target_element(observation->standard_element_id)) {
        node = support_calloc(context, sizeof(*node));
        if (node == NULL) return err_status(ENOMEM);
        node->element_id = observation->standard_element_id;
        node->source_offset = observation->source_offset;
        node->source_length = observation->source_length;
        node->depth = observation->depth;
        node->flags = observation->flags;
        node->parent_details = context->current_details;
        if (context->nodes_tail == NULL) context->nodes = node;
        else context->nodes_tail->next = node;
        context->nodes_tail = node;
    }
    context->frames[context->frame_count++] = (g11_frame){
        .node = node,
        .prior_details = context->current_details,
        .source_offset = observation->source_offset,
        .depth = observation->depth
    };
    if (node != NULL && node->element_id == ARBOR_VIEW0_NATIVE_ELEMENT_DETAILS)
        context->current_details = node;
    return ok_status();
}

static arbor_status element_begin(
    void *opaque, const arbor_view0_native_element_observation *observation) {
    if (opaque == NULL || observation == NULL) return err_status(EINVAL);
    g11_context *context = opaque;
    if (context->frame_count == 0u) return err_status(EIO);
    const g11_frame *frame = context->frames + context->frame_count - 1u;
    if (frame->source_offset != observation->source_offset) return err_status(EIO);
    context->current = frame->node;
    return ok_status();
}

static arbor_status attribute(
    void *opaque, const arbor_view0_native_attribute_observation *observation) {
    if (opaque == NULL || observation == NULL) return err_status(EINVAL);
    g11_context *context = opaque;
    if (context->current == NULL) return ok_status();
    const g11_attr_code code = attr_code(observation->local_name);
    if (code != G11_ATTR_NONE && !context->current->values[code].present)
        context->current->values[code] = (g11_value){observation->value, true};
    return ok_status();
}

static arbor_status element_complete(
    void *opaque, const arbor_view0_native_element_observation *observation) {
    if (opaque == NULL || observation == NULL) return err_status(EINVAL);
    g11_context *context = opaque;
    if (context->frame_count == 0u) return err_status(EIO);
    const g11_frame *frame = context->frames + context->frame_count - 1u;
    if (frame->source_offset != observation->source_offset) return err_status(EIO);
    return ok_status();
}

static arbor_status source_attribute(
    void *opaque, const arbor_view0_native_source_attribute_observation *observation) {
    if (opaque == NULL || observation == NULL) return err_status(EINVAL);
    g11_context *context = opaque;
    const g11_attr_code code = attr_code(observation->local_name);
    if (code == G11_ATTR_NONE) return ok_status();
    if (observation->owner_source_offset == ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE ||
        observation->source_offset == ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE ||
        observation->source_offset > UINT32_MAX || observation->source_length > UINT32_MAX)
        return err_status(EIO);
    if (source_attr_for(context, observation->owner_source_offset, code) != NULL)
        return ok_status();
    g11_source_attr *entry = support_calloc(context, sizeof(*entry));
    if (entry == NULL) return err_status(ENOMEM);
    entry->owner_source_offset = observation->owner_source_offset;
    entry->source_offset = (uint32_t)observation->source_offset;
    entry->source_length = (uint32_t)observation->source_length;
    entry->code = (uint16_t)code;
    if (context->source_attrs_tail == NULL) context->source_attrs = entry;
    else context->source_attrs_tail->next = entry;
    context->source_attrs_tail = entry;
    return ok_status();
}

static arbor_status traversal_leave(
    void *opaque, const arbor_view0_native_element_observation *observation) {
    if (opaque == NULL || observation == NULL) return err_status(EINVAL);
    g11_context *context = opaque;
    if (context->frame_count == 0u) return err_status(EIO);
    const g11_frame *frame = context->frames + context->frame_count - 1u;
    if (frame->source_offset != observation->source_offset || frame->depth != observation->depth)
        return err_status(EIO);
    if (observation->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_HTML &&
        observation->depth == 0u) {
        arbor_status status = evaluate_all(context);
        if (status.native != 0) return status;
    }
    context->current_details = frame->prior_details;
    context->frame_count -= 1u;
    context->current = NULL;
    return ok_status();
}

static arbor_status evaluate(
    arbor_span input, arbor_view0_native_v1n2_g11_anchor *anchors,
    uint64_t anchor_capacity, bool collect,
    arbor_view0_native_v1n2_g11_evaluation *evaluation_out) {
    if (evaluation_out == NULL || (collect && anchor_capacity != 0u && anchors == NULL))
        return err_status(EINVAL);
    lexbor_mraw_t *arena = lexbor_mraw_create();
    if (arena == NULL) return err_status(ENOMEM);
    if (lexbor_mraw_init(arena, 4096u) != LXB_STATUS_OK) {
        (void)lexbor_mraw_destroy(arena, true);
        return err_status(ENOMEM);
    }
    g11_context context = {
        .arena = arena, .anchors = anchors,
        .anchor_capacity = anchor_capacity, .collect = collect
    };
    context.evaluation.deferred_external_semantics_count = UINT64_C(4);
    const arbor_view0_native_semantic_observer observer = {
        .context = &context,
        .element_begin = element_begin,
        .attribute = attribute,
        .element_complete = element_complete,
        .traversal_enter = traversal_enter,
        .traversal_leave = traversal_leave,
        .source_attribute = source_attribute
    };
    arbor_view0_native_parse_counts parse_counts = {0};
    arbor_view0_native_document_facts facts = {0};
    arbor_view0_native_observation_counts counts = {0};
    arbor_status status = arbor_view0_native_lexbor_observe(
        input, &observer, &parse_counts, &facts, &counts);
    if (status.native == 0 && context.frame_count != 0u) status = err_status(EIO);
    if (status.native == 0 && collect && context.evaluation.diagnostic_count != anchor_capacity)
        status = err_status(EIO);
    if (status.native == 0) *evaluation_out = context.evaluation;
    (void)lexbor_mraw_destroy(arena, true);
    return status;
}

arbor_status arbor_view0_native_v1n2_g11_measure(
    arbor_span input, arbor_view0_native_v1n2_g11_evaluation *evaluation_out) {
    return evaluate(input, NULL, 0u, false, evaluation_out);
}

arbor_status arbor_view0_native_v1n2_g11_collect_anchors(
    arbor_span input, arbor_view0_native_v1n2_g11_anchor *anchors,
    uint64_t anchor_capacity, arbor_view0_native_v1n2_g11_evaluation *evaluation_out) {
    return evaluate(input, anchors, anchor_capacity, true, evaluation_out);
}

void arbor_view0_native_v1n2_g11_materialize_anchor(
    const arbor_view0_native_v1n2_g11_anchor *anchor, uint64_t discovery_sequence,
    arbor_view0_native_diagnostic *diagnostic) {
    static const char *const messages[] = {
        "Details name-group or open-state relationship violates the frozen static boundary",
        "Dialog authored-state relationship violates the frozen static boundary"
    };
    if (anchor == NULL || diagnostic == NULL || anchor->shared.rule_ordinal == 0u ||
        anchor->shared.rule_ordinal > ARBOR_VIEW0_NATIVE_V1N2_G11_RULE_COUNT) return;
    const arbor_view0_native_v1n2_rule_meta *meta = arbor_view0_native_v1n2_c0_rule_at(
        UINT64_C(36) + (uint64_t)anchor->shared.rule_ordinal - 1u);
    if (meta == NULL || meta->group != ARBOR_VIEW0_NATIVE_V1N2_GROUP_G11) return;
    (void)memset(diagnostic, 0, sizeof(*diagnostic));
    diagnostic->rule_id = meta->rule_id;
    diagnostic->byte_offset = anchor->shared.byte_offset;
    diagnostic->source_length = anchor->shared.source_length;
    diagnostic->discovery_sequence = discovery_sequence;
    diagnostic->severity = (uint32_t)ARBOR_VIEW0_NATIVE_SEVERITY_ERROR;
    diagnostic->origin = (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_AUTHORING;
    (void)memcpy(diagnostic->symbolic_name, meta->rule_symbol, strlen(meta->rule_symbol) + 1u);
    const char *message = messages[anchor->shared.rule_ordinal - 1u];
    (void)memcpy(diagnostic->message, message, strlen(message) + 1u);
}
