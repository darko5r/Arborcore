#ifndef ARBORCORE_VIEW0_CONFORMANCE_V1N3_C0_H
#define ARBORCORE_VIEW0_CONFORMANCE_V1N3_C0_H

#include <arborcore/view0_conformance/native.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ARBOR_VIEW0_NATIVE_V1N3_RULE_COUNT UINT64_C(30)
#define ARBOR_VIEW0_NATIVE_V1N3_MAX_HTML_DEPTH UINT64_C(4096)

typedef struct arbor_view0_native_v1n3_attr arbor_view0_native_v1n3_attr;
typedef struct arbor_view0_native_v1n3_element arbor_view0_native_v1n3_element;

struct arbor_view0_native_v1n3_attr {
    arbor_view0_native_v1n3_attr *next;
    arbor_span name;
    arbor_span value;
    uint64_t name_offset;
    uint64_t value_offset;
    uint64_t source_length;
    bool has_value;
};

struct arbor_view0_native_v1n3_element {
    arbor_view0_native_v1n3_element *next;
    arbor_view0_native_v1n3_element *parent;
    arbor_view0_native_v1n3_attr *attributes;
    arbor_span name;
    arbor_span text;
    uint64_t source_offset;
    uint64_t source_length;
    uint64_t depth;
    bool template_opaque;
};

typedef struct arbor_view0_native_v1n3_anchor {
    uint64_t rule_id;
    uint64_t byte_offset;
    uint64_t source_length;
    uint32_t external_id;
    uint16_t group;
    uint16_t ordinal;
} arbor_view0_native_v1n3_anchor;

typedef struct arbor_view0_native_v1n3_evaluation {
    uint64_t diagnostic_count;
    uint64_t element_count;
    uint64_t attribute_count;
    uint64_t frontend_parse_count;
    uint64_t deferred_runtime_count;
    uint64_t rule_violation_count[30];
} arbor_view0_native_v1n3_evaluation;

typedef struct arbor_view0_native_v1n3_context {
    arbor_span input;
    const arbor_view0_native_v1n3_options *options;
    void *arena;
    uint16_t group;
    arbor_view0_native_v1n3_anchor *anchors;
    uint64_t anchor_capacity;
    arbor_view0_native_v1n3_evaluation evaluation;
    arbor_view0_native_v1n3_element *elements;
    arbor_view0_native_v1n3_element *elements_tail;
} arbor_view0_native_v1n3_context;

void *arbor_view0_native_v1n3_support_calloc(void *arena, size_t size);

bool arbor_view0_native_v1n3_ascii_equal(arbor_span value, const char *literal);
bool arbor_view0_native_v1n3_ascii_case_equal(arbor_span value, const char *literal);
bool arbor_view0_native_v1n3_ascii_contains(arbor_span value, const char *literal);
bool arbor_view0_native_v1n3_token_contains(arbor_span value, const char *literal);
bool arbor_view0_native_v1n3_absolute_url(arbor_span value);
const arbor_view0_native_v1n3_attr *arbor_view0_native_v1n3_attr_find(
    const arbor_view0_native_v1n3_element *element, const char *name);
const arbor_view0_native_v1n3_element *arbor_view0_native_v1n3_id_find(
    const arbor_view0_native_v1n3_context *context, arbor_span id);
bool arbor_view0_native_v1n3_is_descendant(
    const arbor_view0_native_v1n3_element *element,
    const arbor_view0_native_v1n3_element *ancestor);

arbor_status arbor_view0_native_v1n3_emit(
    arbor_view0_native_v1n3_context *context,
    uint16_t ordinal,
    uint64_t byte_offset,
    uint64_t source_length,
    uint32_t external_id);

arbor_status arbor_view0_native_v1n3_run_group(
    uint16_t group,
    arbor_span input,
    const arbor_view0_native_v1n3_options *options,
    void *arena,
    arbor_view0_native_v1n3_anchor *anchors,
    uint64_t anchor_capacity,
    arbor_view0_native_v1n3_evaluation *evaluation_out);

void arbor_view0_native_v1n3_materialize(
    const arbor_view0_native_v1n3_anchor *anchor,
    uint64_t discovery_sequence,
    arbor_view0_native_diagnostic *diagnostic);

bool arbor_view0_native_v1n3_c0_validate(void);

#endif
