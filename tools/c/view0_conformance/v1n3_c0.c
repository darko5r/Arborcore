#include "v1n3_c0.h"

#include "g12.h"
#include "g13.h"
#include "g14.h"
#include "g15.h"
#include "g16.h"

#include <errno.h>
#include <string.h>

static arbor_status status_value(int value) { return arbor_status_from_native(-(int64_t)value); }
static arbor_status ok(void) { return arbor_status_from_native(0); }
static uint8_t lower(uint8_t b) { return b >= (uint8_t)'A' && b <= (uint8_t)'Z' ? (uint8_t)(b + 32u) : b; }
static bool ws(uint8_t b) { return b == 9u || b == 10u || b == 12u || b == 13u || b == 32u; }
static bool name_byte(uint8_t b) {
    return (b >= (uint8_t)'A' && b <= (uint8_t)'Z') || (b >= (uint8_t)'a' && b <= (uint8_t)'z') ||
        (b >= (uint8_t)'0' && b <= (uint8_t)'9') || b == (uint8_t)'-' || b == (uint8_t)'_' || b == (uint8_t)':';
}

bool arbor_view0_native_v1n3_ascii_equal(arbor_span value, const char *literal)
{
    if (literal == NULL) return false;
    const size_t n = strlen(literal);
    return (uint64_t)n == value.length && (n == 0u || memcmp(value.data, literal, n) == 0);
}

bool arbor_view0_native_v1n3_ascii_case_equal(arbor_span value, const char *literal)
{
    if (literal == NULL) return false;
    const size_t n = strlen(literal);
    if ((uint64_t)n != value.length) return false;
    for (uint64_t i = 0u; i < value.length; ++i) if (lower(value.data[i]) != lower((uint8_t)literal[i])) return false;
    return true;
}

bool arbor_view0_native_v1n3_ascii_contains(arbor_span value, const char *literal)
{
    if (literal == NULL) return false;
    const size_t n = strlen(literal);
    if (n == 0u || (uint64_t)n > value.length) return false;
    for (uint64_t i = 0u; i <= value.length - (uint64_t)n; ++i) {
        bool equal = true; for (uint64_t j = 0u; j < (uint64_t)n; ++j) if (lower(value.data[i + j]) != lower((uint8_t)literal[j])) { equal = false; break; }
        if (equal) return true;
    }
    return false;
}

bool arbor_view0_native_v1n3_token_contains(arbor_span value, const char *literal)
{
    uint64_t i = 0u; while (i < value.length) {
        while (i < value.length && ws(value.data[i])) i += 1u;
        const uint64_t start = i; while (i < value.length && !ws(value.data[i])) i += 1u;
        const arbor_span token = {value.data + start, i - start};
        if (arbor_view0_native_v1n3_ascii_case_equal(token, literal)) return true;
    }
    return false;
}

bool arbor_view0_native_v1n3_absolute_url(arbor_span value)
{
    if (value.length < 3u) return false;
    if (!((value.data[0] >= (uint8_t)'A' && value.data[0] <= (uint8_t)'Z') ||
          (value.data[0] >= (uint8_t)'a' && value.data[0] <= (uint8_t)'z'))) return false;
    for (uint64_t i = 1u; i < value.length; ++i) {
        if (value.data[i] == (uint8_t)':') return true;
        if (!((value.data[i] >= (uint8_t)'A' && value.data[i] <= (uint8_t)'Z') ||
              (value.data[i] >= (uint8_t)'a' && value.data[i] <= (uint8_t)'z') ||
              (value.data[i] >= (uint8_t)'0' && value.data[i] <= (uint8_t)'9') ||
              value.data[i] == (uint8_t)'+' || value.data[i] == (uint8_t)'-' || value.data[i] == (uint8_t)'.')) return false;
    }
    return false;
}

const arbor_view0_native_v1n3_attr *arbor_view0_native_v1n3_attr_find(
    const arbor_view0_native_v1n3_element *element, const char *name)
{
    if (element == NULL) return NULL;
    for (const arbor_view0_native_v1n3_attr *a = element->attributes; a != NULL; a = a->next)
        if (arbor_view0_native_v1n3_ascii_case_equal(a->name, name)) return a;
    return NULL;
}

const arbor_view0_native_v1n3_element *arbor_view0_native_v1n3_id_find(
    const arbor_view0_native_v1n3_context *context, arbor_span id)
{
    if (context == NULL) return NULL;
    for (const arbor_view0_native_v1n3_element *e = context->elements; e != NULL; e = e->next) {
        const arbor_view0_native_v1n3_attr *a = arbor_view0_native_v1n3_attr_find(e, "id");
        if (a != NULL && a->value.length == id.length &&
            (id.length == 0u || memcmp(a->value.data, id.data, (size_t)id.length) == 0)) return e;
    }
    return NULL;
}

bool arbor_view0_native_v1n3_is_descendant(
    const arbor_view0_native_v1n3_element *element,
    const arbor_view0_native_v1n3_element *ancestor)
{
    if (element == NULL || ancestor == NULL) return false;
    for (const arbor_view0_native_v1n3_element *p = element->parent; p != NULL; p = p->parent) if (p == ancestor) return true;
    return false;
}

arbor_status arbor_view0_native_v1n3_emit(
    arbor_view0_native_v1n3_context *context,
    uint16_t ordinal,
    uint64_t byte_offset,
    uint64_t source_length,
    uint32_t external_id)
{
    if (context == NULL || ordinal == 0u) return status_value(EINVAL);
    const uint64_t global = context->group == 12u ? (uint64_t)ordinal - 1u :
        context->group == 13u ? 8u + (uint64_t)ordinal - 1u :
        context->group == 14u ? 14u + (uint64_t)ordinal - 1u :
        context->group == 15u ? 20u + (uint64_t)ordinal - 1u :
        context->group == 16u ? 28u + (uint64_t)ordinal - 1u : UINT64_MAX;
    if (global >= ARBOR_VIEW0_NATIVE_V1N3_RULE_COUNT) return status_value(EINVAL);
    const uint64_t index = context->evaluation.diagnostic_count;
    if (context->anchors != NULL) {
        if (index >= context->anchor_capacity) return status_value(ENOSPC);
        context->anchors[index] = (arbor_view0_native_v1n3_anchor){
            UINT64_C(0x0000000030000000) | ((uint64_t)context->group << 16u) | (uint64_t)ordinal,
            byte_offset, source_length, external_id, context->group, ordinal};
    }
    context->evaluation.diagnostic_count += 1u;
    context->evaluation.rule_violation_count[global] += 1u;
    return ok();
}

static bool void_element(arbor_span name)
{
    static const char *const names[] = {"area","base","br","col","embed","hr","img","input","link","meta","source","track","wbr"};
    for (uint64_t i = 0u; i < sizeof(names) / sizeof(names[0]); ++i)
        if (arbor_view0_native_v1n3_ascii_case_equal(name, names[i])) return true;
    return false;
}

static arbor_status dispatch_element(arbor_view0_native_v1n3_context *context,
                                     const arbor_view0_native_v1n3_element *element)
{
    switch (context->group) {
        case 12u: return arbor_view0_native_v1n3_g12_validate_element(context, element);
        case 13u: return arbor_view0_native_v1n3_g13_validate_element(context, element);
        case 14u: return arbor_view0_native_v1n3_g14_validate_element(context, element);
        case 15u: return arbor_view0_native_v1n3_g15_validate_element(context, element);
        case 16u: return arbor_view0_native_v1n3_g16_validate_element(context, element);
        default: return status_value(EINVAL);
    }
}

static arbor_status dispatch_final(arbor_view0_native_v1n3_context *context)
{
    switch (context->group) {
        case 12u: return arbor_view0_native_v1n3_g12_finalize(context);
        case 13u: return arbor_view0_native_v1n3_g13_finalize(context);
        case 14u: return arbor_view0_native_v1n3_g14_finalize(context);
        case 15u: return arbor_view0_native_v1n3_g15_finalize(context);
        case 16u: return arbor_view0_native_v1n3_g16_finalize(context);
        default: return status_value(EINVAL);
    }
}

static arbor_status parse_document(arbor_view0_native_v1n3_context *context)
{
    arbor_view0_native_v1n3_element *stack[ARBOR_VIEW0_NATIVE_V1N3_MAX_HTML_DEPTH];
    uint64_t depth = 0u; uint64_t i = 0u;
    while (i < context->input.length) {
        if (context->input.data[i] != (uint8_t)'<') { i += 1u; continue; }
        if (context->input.length - i >= 4u && memcmp(context->input.data + i, "<!--", 4u) == 0) {
            i += 4u; while (i + 2u < context->input.length && memcmp(context->input.data + i, "-->", 3u) != 0) i += 1u;
            i = i + 2u < context->input.length ? i + 3u : context->input.length; continue;
        }
        uint64_t p = i + 1u; bool closing = false;
        if (p < context->input.length && context->input.data[p] == (uint8_t)'/') { closing = true; p += 1u; }
        if (p >= context->input.length || !name_byte(context->input.data[p])) { i += 1u; continue; }
        const uint64_t name_start = p; while (p < context->input.length && name_byte(context->input.data[p])) p += 1u;
        const arbor_span name = {context->input.data + name_start, p - name_start};
        if (closing) {
            while (p < context->input.length && context->input.data[p] != (uint8_t)'>') p += 1u;
            if (depth != 0u) depth -= 1u;
            i = p < context->input.length ? p + 1u : p; continue;
        }
        arbor_view0_native_v1n3_element *element = arbor_view0_native_v1n3_support_calloc(context->arena, sizeof(*element));
        if (element == NULL) return status_value(ENOMEM);
        element->name = name; element->source_offset = i; element->source_length = name.length + 2u;
        element->depth = depth; element->parent = depth == 0u ? NULL : stack[depth - 1u];
        element->template_opaque = element->parent != NULL && (element->parent->template_opaque || arbor_view0_native_v1n3_ascii_case_equal(element->parent->name, "template"));
        arbor_view0_native_v1n3_attr *tail = NULL; bool self_closing = false;
        while (p < context->input.length) {
            while (p < context->input.length && ws(context->input.data[p])) p += 1u;
            if (p >= context->input.length) break;
            if (context->input.data[p] == (uint8_t)'>') { p += 1u; break; }
            if (context->input.data[p] == (uint8_t)'/' && p + 1u < context->input.length && context->input.data[p + 1u] == (uint8_t)'>') { self_closing = true; p += 2u; break; }
            const uint64_t an = p; while (p < context->input.length && name_byte(context->input.data[p])) p += 1u;
            if (p == an) { p += 1u; continue; }
            arbor_view0_native_v1n3_attr *attr = arbor_view0_native_v1n3_support_calloc(context->arena, sizeof(*attr));
            if (attr == NULL) return status_value(ENOMEM);
            attr->name = (arbor_span){context->input.data + an, p - an}; attr->name_offset = an;
            while (p < context->input.length && ws(context->input.data[p])) p += 1u;
            if (p < context->input.length && context->input.data[p] == (uint8_t)'=') {
                attr->has_value = true; p += 1u; while (p < context->input.length && ws(context->input.data[p])) p += 1u;
                const uint64_t value_source = p; uint64_t av = p;
                if (p < context->input.length && (context->input.data[p] == (uint8_t)'\'' || context->input.data[p] == (uint8_t)'"')) {
                    const uint8_t quote = context->input.data[p++]; av = p; while (p < context->input.length && context->input.data[p] != quote) p += 1u;
                    attr->value = (arbor_span){context->input.data + av, p - av}; attr->value_offset = av;
                    if (p < context->input.length) p += 1u;
                } else {
                    av = p; while (p < context->input.length && !ws(context->input.data[p]) && context->input.data[p] != (uint8_t)'>') p += 1u;
                    attr->value = (arbor_span){context->input.data + av, p - av}; attr->value_offset = av;
                }
                attr->source_length = p - an; (void)value_source;
            } else { attr->source_length = p - an; attr->value_offset = p; }
            if (tail == NULL) element->attributes = attr; else tail->next = attr; tail = attr;
            context->evaluation.attribute_count += 1u;
        }
        element->source_length = p > i ? p - i : 1u;
        uint64_t raw_text_end = p;
        const bool script_element = arbor_view0_native_v1n3_ascii_case_equal(name, "script");
        if (script_element) {
            while (raw_text_end + 8u < context->input.length) {
                if (context->input.data[raw_text_end] == (uint8_t)'<' && context->input.data[raw_text_end + 1u] == (uint8_t)'/' &&
                    lower(context->input.data[raw_text_end + 2u]) == (uint8_t)'s' && lower(context->input.data[raw_text_end + 3u]) == (uint8_t)'c' &&
                    lower(context->input.data[raw_text_end + 4u]) == (uint8_t)'r' && lower(context->input.data[raw_text_end + 5u]) == (uint8_t)'i' &&
                    lower(context->input.data[raw_text_end + 6u]) == (uint8_t)'p' && lower(context->input.data[raw_text_end + 7u]) == (uint8_t)'t') break;
                raw_text_end += 1u;
            }
            element->text = (arbor_span){context->input.data + p, raw_text_end - p};
        }
        if (context->elements_tail == NULL) context->elements = element; else context->elements_tail->next = element;
        context->elements_tail = element; context->evaluation.element_count += 1u;
        arbor_status status = dispatch_element(context, element); if (status.native != 0) return status;
        if (!self_closing && !void_element(name)) {
            if (depth == ARBOR_VIEW0_NATIVE_V1N3_MAX_HTML_DEPTH) return status_value(ENOSPC);
            stack[depth++] = element;
        }
        i = script_element ? raw_text_end : p;
    }
    return dispatch_final(context);
}

arbor_status arbor_view0_native_v1n3_run_group(
    uint16_t group, arbor_span input, const arbor_view0_native_v1n3_options *options,
    void *arena, arbor_view0_native_v1n3_anchor *anchors, uint64_t anchor_capacity,
    arbor_view0_native_v1n3_evaluation *evaluation_out)
{
    if (evaluation_out == NULL || options == NULL || arena == NULL || group < 12u || group > 16u) return status_value(EINVAL);
    arbor_view0_native_v1n3_context context = {0}; context.input = input; context.options = options;
    context.arena = arena; context.group = group; context.anchors = anchors; context.anchor_capacity = anchor_capacity;
    arbor_status status = parse_document(&context); if (status.native != 0) return status;
    *evaluation_out = context.evaluation; return ok();
}

static const char *const symbols[30] = {
    "ARBOR_VIEW_V1_G12_SCRIPT_ELEMENT_DECLARATION","ARBOR_VIEW_V1_G12_SCRIPT_RESOURCE_ATTRIBUTES","ARBOR_VIEW_V1_G12_SCRIPT_TEXT_AUTHORING","ARBOR_VIEW_V1_G12_NOSCRIPT_AUTHORING","ARBOR_VIEW_V1_G12_TEMPLATE_SHADOW_AUTHORING","ARBOR_VIEW_V1_G12_SLOT_AUTHORING","ARBOR_VIEW_V1_G12_CANVAS_FALLBACK","ARBOR_VIEW_V1_G12_CANVAS_INTERACTIVE_ACCESSIBILITY",
    "ARBOR_VIEW_V1_G13_CUSTOM_ELEMENT_CONSTRUCTOR_AUTHORING","ARBOR_VIEW_V1_G13_VALID_CUSTOM_ELEMENT_NAME","ARBOR_VIEW_V1_G13_AUTONOMOUS_IDENTITY_AND_CATEGORY","ARBOR_VIEW_V1_G13_CUSTOMIZED_BUILTIN_IS","ARBOR_VIEW_V1_G13_AUTONOMOUS_ATTRIBUTE_BOUNDARY","ARBOR_VIEW_V1_G13_FORM_ASSOCIATED_CUSTOM_ELEMENT",
    "ARBOR_VIEW_V1_G14_ITEM_TYPE_SEMANTICS","ARBOR_VIEW_V1_G14_ITEM_IDENTIFIER_SEMANTICS","ARBOR_VIEW_V1_G14_ITEMREF_ASSOCIATION","ARBOR_VIEW_V1_G14_PROPERTY_NAME_SEMANTICS","ARBOR_VIEW_V1_G14_PROPERTY_VALUE_SEMANTICS","ARBOR_VIEW_V1_G14_PROPERTY_GRAPH_INTEGRITY",
    "ARBOR_VIEW_V1_G15_HIDDEN_REFERENCE_SEMANTICS","ARBOR_VIEW_V1_G15_INERT_AUTHORING_SEMANTICS","ARBOR_VIEW_V1_G15_TABINDEX_AUTHORING","ARBOR_VIEW_V1_G15_ACCESSKEY_AUTHORING","ARBOR_VIEW_V1_G15_EDITABLE_TEXT_HINTS","ARBOR_VIEW_V1_G15_DRAGGABLE_AUTHORING","ARBOR_VIEW_V1_G15_POPOVER_ELEMENT_AUTHORING","ARBOR_VIEW_V1_G15_POPOVER_TARGET_ASSOCIATION",
    "ARBOR_VIEW_V1_G16_EVENT_HANDLER_JAVASCRIPT_VALUE","ARBOR_VIEW_V1_G16_INPUT_PATTERN_ECMASCRIPT_REGEXP"};
static const char *const messages[30] = {
    "Script declaration violates the frozen static authoring relationship","Script resource attributes are inconsistent with the declared script kind","Script text violates the HTML-owned raw-text authoring restriction","Noscript use is inconsistent with the configured scripting mode","Declarative shadow-root attributes require a valid shadowrootmode relationship","Slot name or assignment relationship is not statically valid","Canvas requires meaningful authored fallback content","Interactive canvas fallback lacks a deterministic accessibility relationship",
    "Configured custom-element constructor is outside the admitted parse-only subset","Custom-element name is not a valid non-reserved name","Autonomous custom element lacks a matching immutable definition","Customized built-in is value does not match its immutable definition","Autonomous custom element violates its specialized attribute boundary","Form-associated custom-element attribute lacks a form-associated definition",
    "Microdata itemtype requires itemscope and absolute unique vocabulary tokens","Microdata itemid lacks its required itemscope itemtype or absolute URL","Microdata itemref does not resolve to unique same-tree targets","Microdata itemprop contains an invalid property-name relationship","Microdata property element lacks its required authored value source","Microdata property is orphaned or introduces an invalid item graph relationship",
    "A non-hidden authored relation targets hidden content","Inert content contains a prohibited authored interaction relationship","Tabindex is used outside the frozen specialized authoring boundary","Accesskey tokens must be unique single code points within the document","Editable-text hint attributes have an inconsistent static relationship","Draggable true requires a non-empty advisory title","Popover declaration violates a static nesting or accessibility relationship","Popover target does not resolve to a same-tree popover with a valid action",
    "Event-handler attribute value is not a valid ECMAScript FunctionBody","Pattern attribute value is not a valid ECMAScript Pattern in UnicodeSets mode"};

void arbor_view0_native_v1n3_materialize(
    const arbor_view0_native_v1n3_anchor *anchor, uint64_t discovery_sequence,
    arbor_view0_native_diagnostic *diagnostic)
{
    if (anchor == NULL || diagnostic == NULL) return;
    const uint64_t index = anchor->group == 12u ? (uint64_t)anchor->ordinal - 1u :
        anchor->group == 13u ? 8u + (uint64_t)anchor->ordinal - 1u :
        anchor->group == 14u ? 14u + (uint64_t)anchor->ordinal - 1u :
        anchor->group == 15u ? 20u + (uint64_t)anchor->ordinal - 1u : 28u + (uint64_t)anchor->ordinal - 1u;
    arbor_view0_native_diagnostic out = {0}; out.rule_id = anchor->rule_id; out.byte_offset = anchor->byte_offset;
    out.source_length = anchor->source_length; out.discovery_sequence = discovery_sequence;
    out.severity = (uint32_t)ARBOR_VIEW0_NATIVE_SEVERITY_ERROR; out.origin = (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_AUTHORING;
    out.external_id = anchor->external_id;
    if (index < 30u) {
        (void)strncpy(out.symbolic_name, symbols[index], sizeof(out.symbolic_name) - 1u);
        (void)strncpy(out.message, messages[index], sizeof(out.message) - 1u);
    }
    *diagnostic = out;
}

bool arbor_view0_native_v1n3_c0_validate(void)
{
    return ARBOR_VIEW0_NATIVE_V1N3_RULE_COUNT == 30u &&
        ARBOR_VIEW0_NATIVE_V1N3_TRANSIENT_SUPPORT_ARENAS == 1u &&
        ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS == 4096u;
}
