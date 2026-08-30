#include "g09.h"

#include <lexbor/core/mraw.h>

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef enum g09_attr_code {
    G09_ATTR_NONE = 0,
    G09_ATTR_ID,
    G09_ATTR_HEADERS,
    G09_ATTR_SCOPE,
    G09_ATTR_SPAN,
    G09_ATTR_COLSPAN,
    G09_ATTR_ROWSPAN,
    G09_ATTR_ABBR,
    G09_ATTR__COUNT
} g09_attr_code;

typedef struct g09_value { arbor_span span; bool present; } g09_value;
typedef struct g09_source_attr g09_source_attr;
typedef struct g09_table g09_table;
typedef struct g09_group g09_group;
typedef struct g09_row g09_row;
typedef struct g09_cell g09_cell;
typedef struct g09_col g09_col;
typedef struct g09_edge g09_edge;
typedef struct g09_id_entry g09_id_entry;

struct g09_source_attr {
    g09_source_attr *next;
    uint64_t owner;
    uint32_t offset;
    uint32_t length;
    uint16_t code;
};

struct g09_group {
    g09_group *next;
    g09_table *table;
    uint64_t source_offset;
    uint64_t source_length;
    uint64_t first_y;
    uint64_t end_y;
    uint8_t kind;
};

struct g09_row {
    g09_row *next;
    g09_table *table;
    g09_group *group;
    g09_cell *cells;
    g09_cell *cells_tail;
    uint64_t source_offset;
    uint64_t source_length;
    uint64_t y;
};

struct g09_cell {
    g09_cell *next;
    g09_table *table;
    g09_row *row;
    arbor_span id;
    arbor_span headers;
    arbor_span scope;
    arbor_span abbr;
    uint64_t source_offset;
    uint64_t source_length;
    uint64_t x0, x1, y0, y1;
    uint64_t colspan, rowspan;
    uint64_t association_mark;
    uint64_t block_mark;
    uint64_t pending_mark;
    g09_edge *edges;
    g09_edge *edges_tail;
    bool is_header;
    bool headers_present;
    bool span_lexically_valid;
    bool colspan_range_invalid;
    bool rowspan_range_invalid;
    bool empty;
    uint8_t graph_color;
};

struct g09_edge {
    g09_edge *next;
    g09_cell *target;
    arbor_span token;
};

struct g09_id_entry {
    g09_id_entry *next;
    arbor_span id;
    g09_table *table;
    g09_cell *cell;
};

struct g09_table {
    g09_table *next;
    g09_row *rows;
    g09_row *rows_tail;
    g09_group *groups;
    g09_group *groups_tail;
    g09_cell *placed;
    uint64_t source_offset;
    uint64_t source_length;
    uint64_t width;
    uint64_t height;
    uint32_t direct_caption_count;
};

typedef struct g09_colgroup {
    struct g09_colgroup *next;
    g09_table *table;
    uint64_t source_offset;
    uint64_t source_length;
    uint64_t span;
    uint64_t x0;
    uint64_t x1;
    uint64_t child_span_total;
    uint32_t direct_col_count;
    bool span_present;
    bool span_valid;
    bool span_lexically_valid;
} g09_colgroup;

struct g09_col {
    g09_col *next;
    g09_colgroup *group;
    uint64_t source_offset;
    uint64_t source_length;
    uint64_t span;
    bool span_range_invalid;
    bool span_lexically_valid;
};

typedef struct g09_frame {
    uint64_t id, source_offset, source_length, depth;
    g09_table *table;
    g09_group *group;
    g09_row *row;
    g09_cell *cell;
    g09_colgroup *colgroup;
    g09_col *col;
} g09_frame;

typedef struct g09_current {
    uint64_t id, parent_id, source_offset, source_length;
    g09_value values[G09_ATTR__COUNT];
} g09_current;

typedef struct g09_context {
    lexbor_mraw_t *arena;
    arbor_view0_native_v1n2_g09_anchor *anchors;
    uint64_t anchor_capacity;
    bool collect;
    arbor_view0_native_v1n2_g09_evaluation evaluation;
    g09_table *tables;
    g09_table *tables_tail;
    g09_colgroup *colgroups;
    g09_colgroup *colgroups_tail;
    g09_col *cols;
    g09_col *cols_tail;
    g09_id_entry *ids;
    g09_id_entry *ids_tail;
    g09_source_attr *source_attrs;
    g09_source_attr *source_attrs_tail;
    g09_frame frames[ARBOR_VIEW0_NATIVE_V1N2_G09_MAX_DEPTH];
    uint64_t frame_count;
    g09_current current;
    uint64_t generation;
} g09_context;

arbor_status arbor_view0_native_v1n2_g09_test_checked_add(
    uint64_t left, uint64_t right, uint64_t *out);
arbor_status arbor_view0_native_v1n2_g09_test_checked_multiply(
    uint64_t count, size_t item_size, size_t *out);

static arbor_status ok_status(void) { return arbor_status_from_native(0); }
static arbor_status err_status(int value) { return arbor_status_from_native(-(int64_t)value); }

static void *support_calloc(g09_context *c, size_t size) {
    return arbor_view0_native_v1n2_g09_support_calloc(c->arena, size);
}

static uint8_t ascii_lower(uint8_t b) {
    return b >= (uint8_t)'A' && b <= (uint8_t)'Z'
        ? (uint8_t)(b + ((uint8_t)'a' - (uint8_t)'A')) : b;
}

static bool ascii_space(uint8_t b) {
    return b == UINT8_C(0x09) || b == UINT8_C(0x0a) || b == UINT8_C(0x0c) ||
           b == UINT8_C(0x0d) || b == UINT8_C(0x20);
}

static bool span_eq_ci(arbor_span span, const char *literal) {
    const size_t n = strlen(literal);
    if (span.data == NULL || span.length != (uint64_t)n) return false;
    for (size_t i = 0u; i < n; ++i)
        if (ascii_lower(span.data[i]) != ascii_lower((uint8_t)literal[i])) return false;
    return true;
}

static bool span_eq(arbor_span a, arbor_span b) {
    return a.length == b.length && (a.length == 0u ||
        (a.data != NULL && b.data != NULL && memcmp(a.data, b.data, (size_t)a.length) == 0));
}

static g09_attr_code attr_code(arbor_span name) {
    if (span_eq_ci(name, "id")) return G09_ATTR_ID;
    if (span_eq_ci(name, "headers")) return G09_ATTR_HEADERS;
    if (span_eq_ci(name, "scope")) return G09_ATTR_SCOPE;
    if (span_eq_ci(name, "span")) return G09_ATTR_SPAN;
    if (span_eq_ci(name, "colspan")) return G09_ATTR_COLSPAN;
    if (span_eq_ci(name, "rowspan")) return G09_ATTR_ROWSPAN;
    if (span_eq_ci(name, "abbr")) return G09_ATTR_ABBR;
    return G09_ATTR_NONE;
}

static bool parse_decimal(arbor_span s, uint64_t *out) {
    if (out == NULL || s.data == NULL || s.length == 0u) return false;
    uint64_t value = 0u;
    for (uint64_t i = 0u; i < s.length; ++i) {
        if (s.data[i] < (uint8_t)'0' || s.data[i] > (uint8_t)'9') return false;
        const uint64_t digit = (uint64_t)(s.data[i] - (uint8_t)'0');
        if (value > (UINT64_MAX - digit) / UINT64_C(10)) return false;
        value = value * UINT64_C(10) + digit;
    }
    *out = value;
    return true;
}

static bool token_next(arbor_span s, uint64_t *cursor, arbor_span *token) {
    uint64_t i = *cursor;
    while (i < s.length && ascii_space(s.data[i])) ++i;
    if (i == s.length) { *cursor = i; return false; }
    const uint64_t begin = i;
    while (i < s.length && !ascii_space(s.data[i])) ++i;
    *cursor = i;
    *token = (arbor_span){s.data + begin, i - begin};
    return true;
}

static const g09_source_attr *source_attr_for(
    const g09_context *c, uint64_t owner, g09_attr_code code) {
    for (const g09_source_attr *a = c->source_attrs; a != NULL; a = a->next)
        if (a->owner == owner && a->code == (uint16_t)code) return a;
    return NULL;
}

static arbor_status emit_at(
    g09_context *c, uint16_t rule, uint64_t owner, uint64_t length, g09_attr_code code) {
    if (rule == 0u || rule > ARBOR_VIEW0_NATIVE_V1N2_G09_RULE_COUNT) return err_status(EIO);
    if (c->evaluation.diagnostic_count == ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS)
        return ok_status();
    if (c->evaluation.diagnostic_count == UINT64_MAX ||
        c->evaluation.rule_violation_count[rule - 1u] == UINT64_MAX) return err_status(EOVERFLOW);
    if (c->collect) {
        if (c->evaluation.diagnostic_count >= c->anchor_capacity) return err_status(ENOSPC);
        uint64_t offset = owner, source_length = length;
        const g09_source_attr *a = source_attr_for(c, owner, code);
        if (a != NULL) { offset = a->offset; source_length = a->length; }
        if (offset > UINT32_MAX || source_length > UINT32_MAX) return err_status(EIO);
        c->anchors[c->evaluation.diagnostic_count].shared =
            (arbor_view0_native_v1n2_anchor){
                .byte_offset = offset, .source_length = source_length,
                .discovery_sequence = c->evaluation.diagnostic_count,
                .subject_index = owner, .group_ordinal = UINT16_C(3),
                .rule_ordinal = rule,
                .kind = a == NULL ? ARBOR_VIEW0_NATIVE_V1N2_ANCHOR_ELEMENT
                                  : ARBOR_VIEW0_NATIVE_V1N2_ANCHOR_ATTRIBUTE_NAME
            };
    }
    c->evaluation.diagnostic_count += 1u;
    c->evaluation.rule_violation_count[rule - 1u] += 1u;
    return ok_status();
}

static arbor_status append_table(g09_context *c, const arbor_view0_native_element_observation *o,
                                 g09_table **out) {
    g09_table *t = support_calloc(c, sizeof(*t));
    if (t == NULL) return err_status(ENOMEM);
    t->source_offset = o->source_offset; t->source_length = o->source_length;
    if (c->tables_tail == NULL) c->tables = t; else c->tables_tail->next = t;
    c->tables_tail = t; c->evaluation.table_count += 1u; *out = t;
    return ok_status();
}

static arbor_status append_group(g09_context *c, g09_table *t,
                                 const arbor_view0_native_element_observation *o,
                                 g09_group **out) {
    if (t == NULL) { *out = NULL; return ok_status(); }
    if ((o->flags & ARBOR_VIEW0_NATIVE_ELEMENT_FLAG_SYNTHETIC) != 0u) {
        *out = NULL;
        return ok_status();
    }
    g09_group *g = support_calloc(c, sizeof(*g));
    if (g == NULL) return err_status(ENOMEM);
    g->table = t; g->source_offset = o->source_offset; g->source_length = o->source_length;
    g->kind = o->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_THEAD ? UINT8_C(1) :
        (o->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_TFOOT ? UINT8_C(3) : UINT8_C(2));
    if (t->groups_tail == NULL) t->groups = g; else t->groups_tail->next = g;
    t->groups_tail = g; c->evaluation.row_group_count += 1u; *out = g;
    return ok_status();
}

static arbor_status append_row(g09_context *c, g09_table *t, g09_group *g,
                               const arbor_view0_native_element_observation *o, g09_row **out) {
    if (t == NULL) { *out = NULL; return ok_status(); }
    g09_row *r = support_calloc(c, sizeof(*r));
    if (r == NULL) return err_status(ENOMEM);
    r->table = t; r->group = g; r->source_offset = o->source_offset; r->source_length = o->source_length;
    if (t->rows_tail == NULL) t->rows = r; else t->rows_tail->next = r;
    t->rows_tail = r; c->evaluation.row_count += 1u; *out = r;
    return ok_status();
}

static arbor_status append_cell(g09_context *c, g09_row *r,
                                const arbor_view0_native_element_observation *o, g09_cell **out) {
    if (r == NULL) { *out = NULL; return ok_status(); }
    g09_cell *cell = support_calloc(c, sizeof(*cell));
    if (cell == NULL) return err_status(ENOMEM);
    cell->table = r->table; cell->row = r; cell->source_offset = o->source_offset;
    cell->source_length = o->source_length; cell->colspan = 1u; cell->rowspan = 1u;
    cell->span_lexically_valid = true;
    cell->empty = true;
    cell->is_header = o->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_TH;
    if (r->cells_tail == NULL) r->cells = cell; else r->cells_tail->next = cell;
    r->cells_tail = cell; c->evaluation.cell_count += 1u; *out = cell;
    return ok_status();
}

static arbor_status append_col(g09_context *c, g09_colgroup *group,
                               const arbor_view0_native_element_observation *o,
                               g09_col **out) {
    if (group == NULL) { *out = NULL; return ok_status(); }
    g09_col *col = support_calloc(c, sizeof(*col));
    if (col == NULL) return err_status(ENOMEM);
    col->group = group;
    col->source_offset = o->source_offset;
    col->source_length = o->source_length;
    col->span = 1u;
    col->span_lexically_valid = true;
    if (c->cols_tail == NULL) c->cols = col; else c->cols_tail->next = col;
    c->cols_tail = col;
    group->direct_col_count += 1u;
    *out = col;
    return ok_status();
}

static arbor_status traversal_enter(void *opaque,
                                    const arbor_view0_native_element_observation *o) {
    if (opaque == NULL || o == NULL) return err_status(EINVAL);
    g09_context *c = opaque;
    if (o->depth != c->frame_count || c->frame_count >= ARBOR_VIEW0_NATIVE_V1N2_G09_MAX_DEPTH)
        return err_status(ENOSPC);
    g09_frame f = {.id = o->standard_element_id, .source_offset = o->source_offset,
        .source_length = o->source_length, .depth = o->depth};
    if (c->frame_count != 0u) {
        const g09_frame *p = c->frames + c->frame_count - 1u;
        f.table = p->table; f.group = p->group; f.row = p->row; f.colgroup = p->colgroup;
    }
    arbor_status status = ok_status();
    if (o->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_TABLE)
        status = append_table(c, o, &f.table), f.group = NULL, f.row = NULL, f.colgroup = NULL;
    else if (o->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_THEAD ||
             o->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_TBODY ||
             o->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_TFOOT)
        status = append_group(c, f.table, o, &f.group), f.row = NULL;
    else if (o->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_TR)
        status = append_row(c, f.table, f.group, o, &f.row);
    else if (o->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_TD ||
             o->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_TH)
        status = append_cell(c, f.row, o, &f.cell);
    else if (o->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_COLGROUP && f.table != NULL) {
        g09_colgroup *cg = support_calloc(c, sizeof(*cg));
        if (cg == NULL) status = err_status(ENOMEM);
        else {
            cg->table = f.table; cg->source_offset = o->source_offset; cg->source_length = o->source_length;
            cg->span = 1u; cg->span_valid = true; cg->span_lexically_valid = true;
            if (c->colgroups_tail == NULL) c->colgroups = cg; else c->colgroups_tail->next = cg;
            c->colgroups_tail = cg; f.colgroup = cg; c->evaluation.column_group_count += 1u;
        }
    } else if (o->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_COL && f.colgroup != NULL)
        status = append_col(c, f.colgroup, o, &f.col);
    if (status.native != 0) return status;
    c->frames[c->frame_count++] = f;
    return ok_status();
}

static arbor_status element_begin(void *opaque,
                                  const arbor_view0_native_element_observation *o) {
    if (opaque == NULL || o == NULL) return err_status(EINVAL);
    g09_context *c = opaque;
    (void)memset(&c->current, 0, sizeof(c->current));
    c->current.id = o->standard_element_id; c->current.parent_id = o->parent_standard_element_id;
    c->current.source_offset = o->source_offset; c->current.source_length = o->source_length;
    return ok_status();
}

static arbor_status attribute(void *opaque,
                              const arbor_view0_native_attribute_observation *o) {
    if (opaque == NULL || o == NULL) return err_status(EINVAL);
    g09_context *c = opaque;
    const g09_attr_code code = attr_code(o->local_name);
    if (code != G09_ATTR_NONE && !c->current.values[code].present)
        c->current.values[code] = (g09_value){o->value, true};
    return ok_status();
}

static arbor_status direct_child(void *opaque,
                                 const arbor_view0_native_direct_child_observation *o) {
    if (opaque == NULL || o == NULL) return err_status(EINVAL);
    g09_context *c = opaque;
    if (c->frame_count == 0u) return err_status(EIO);
    g09_frame *f = c->frames + c->frame_count - 1u;
    if (f->cell == NULL || (c->current.id != ARBOR_VIEW0_NATIVE_ELEMENT_TD &&
                            c->current.id != ARBOR_VIEW0_NATIVE_ELEMENT_TH)) return ok_status();
    if (o->kind == ARBOR_VIEW0_NATIVE_DIRECT_CHILD_ELEMENT ||
        (o->kind == ARBOR_VIEW0_NATIVE_DIRECT_CHILD_TEXT &&
         (o->flags & ARBOR_VIEW0_NATIVE_CHILD_FLAG_TEXT_INTER_ELEMENT_WHITESPACE) == 0u))
        f->cell->empty = false;
    return ok_status();
}

static arbor_status source_attribute(void *opaque,
    const arbor_view0_native_source_attribute_observation *o) {
    if (opaque == NULL || o == NULL) return err_status(EINVAL);
    g09_context *c = opaque;
    const g09_attr_code code = attr_code(o->local_name);
    if (code == G09_ATTR_NONE) return ok_status();
    if (o->owner_source_offset == ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE ||
        o->source_offset == ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE ||
        o->source_offset > UINT32_MAX || o->source_length > UINT32_MAX) return err_status(EIO);
    if (source_attr_for(c, o->owner_source_offset, code) != NULL) return ok_status();
    g09_source_attr *a = support_calloc(c, sizeof(*a));
    if (a == NULL) return err_status(ENOMEM);
    a->owner = o->owner_source_offset; a->offset = (uint32_t)o->source_offset;
    a->length = (uint32_t)o->source_length; a->code = (uint16_t)code;
    if (c->source_attrs_tail == NULL) c->source_attrs = a; else c->source_attrs_tail->next = a;
    c->source_attrs_tail = a;
    return ok_status();
}

static const g09_id_entry *find_id(const g09_context *c, arbor_span id) {
    for (const g09_id_entry *entry = c->ids; entry != NULL; entry = entry->next)
        if (span_eq(entry->id, id)) return entry;
    return NULL;
}

static arbor_status append_first_id(g09_context *c, arbor_span id, g09_table *table,
                                    g09_cell *cell) {
    if (id.data == NULL || id.length == 0u || find_id(c, id) != NULL) return ok_status();
    g09_id_entry *entry = support_calloc(c, sizeof(*entry));
    if (entry == NULL) return err_status(ENOMEM);
    entry->id = id;
    entry->table = table;
    entry->cell = cell;
    if (c->ids_tail == NULL) c->ids = entry; else c->ids_tail->next = entry;
    c->ids_tail = entry;
    return ok_status();
}

static arbor_status element_complete(void *opaque,
                                     const arbor_view0_native_element_observation *o) {
    if (opaque == NULL || o == NULL) return err_status(EINVAL);
    g09_context *c = opaque;
    if (c->frame_count == 0u || c->current.source_offset != o->source_offset ||
        c->current.id != o->standard_element_id) return err_status(EIO);
    g09_frame *f = c->frames + c->frame_count - 1u;
    const g09_value *v = c->current.values;
    if (v[G09_ATTR_ID].present) {
        arbor_status id_status = append_first_id(c, v[G09_ATTR_ID].span, f->table,
            (o->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_TD ||
             o->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_TH) ? f->cell : NULL);
        if (id_status.native != 0) return id_status;
    }
    if (f->cell != NULL && (o->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_TD ||
                            o->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_TH)) {
        f->cell->id = v[G09_ATTR_ID].present ? v[G09_ATTR_ID].span : (arbor_span){0};
        f->cell->headers = v[G09_ATTR_HEADERS].present ? v[G09_ATTR_HEADERS].span : (arbor_span){0};
        f->cell->headers_present = v[G09_ATTR_HEADERS].present;
        f->cell->scope = v[G09_ATTR_SCOPE].present ? v[G09_ATTR_SCOPE].span : (arbor_span){0};
        f->cell->abbr = v[G09_ATTR_ABBR].present ? v[G09_ATTR_ABBR].span : (arbor_span){0};
        uint64_t n = 0u;
        if (v[G09_ATTR_COLSPAN].present) {
            if (!parse_decimal(v[G09_ATTR_COLSPAN].span, &n)) {
                f->cell->span_lexically_valid = false;
                c->evaluation.prior_owner_suppression_count += 1u;
            } else if (n == 0u || n > 1000u) f->cell->colspan_range_invalid = true;
            else f->cell->colspan = n;
        }
        if (v[G09_ATTR_ROWSPAN].present) {
            if (!parse_decimal(v[G09_ATTR_ROWSPAN].span, &n)) {
                f->cell->span_lexically_valid = false;
                c->evaluation.prior_owner_suppression_count += 1u;
            } else if (n > 65534u) f->cell->rowspan_range_invalid = true;
            else f->cell->rowspan = n;
        }
    }
    if (o->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_CAPTION && f->table != NULL &&
        c->current.parent_id == ARBOR_VIEW0_NATIVE_ELEMENT_TABLE) f->table->direct_caption_count += 1u;
    if (o->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_COLGROUP && f->colgroup != NULL) {
        f->colgroup->span_present = v[G09_ATTR_SPAN].present;
        if (v[G09_ATTR_SPAN].present) {
            uint64_t n = 0u;
            f->colgroup->span_lexically_valid = parse_decimal(v[G09_ATTR_SPAN].span, &n);
            f->colgroup->span_valid = f->colgroup->span_lexically_valid && n >= 1u && n <= 1000u;
            if (f->colgroup->span_valid) f->colgroup->span = n;
            else if (!f->colgroup->span_lexically_valid)
                c->evaluation.prior_owner_suppression_count += 1u;
        }
    }
    if (o->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_COL && f->col != NULL &&
        v[G09_ATTR_SPAN].present) {
        uint64_t n = 0u;
        f->col->span_lexically_valid = parse_decimal(v[G09_ATTR_SPAN].span, &n);
        if (!f->col->span_lexically_valid)
            c->evaluation.prior_owner_suppression_count += 1u;
        else if (n == 0u || n > 1000u) f->col->span_range_invalid = true;
        else f->col->span = n;
    }
    return ok_status();
}

static bool intersects(const g09_cell *a, uint64_t x0, uint64_t x1, uint64_t y0, uint64_t y1) {
    return a->x0 < x1 && x0 < a->x1 && a->y0 < y1 && y0 < a->y1;
}

static bool checked_add_u64(uint64_t a, uint64_t b, uint64_t *out) {
    if (out == NULL || b > UINT64_MAX - a) return false;
    *out = a + b;
    return true;
}

arbor_status arbor_view0_native_v1n2_g09_test_checked_add(
    uint64_t left, uint64_t right, uint64_t *out) {
    uint64_t value = 0u;
    if (out == NULL) return err_status(EINVAL);
    if (!checked_add_u64(left, right, &value)) return err_status(EOVERFLOW);
    *out = value;
    return ok_status();
}

arbor_status arbor_view0_native_v1n2_g09_test_checked_multiply(
    uint64_t count, size_t item_size, size_t *out) {
    if (out == NULL) return err_status(EINVAL);
    if (item_size != 0u && count > (uint64_t)(SIZE_MAX / item_size))
        return err_status(EOVERFLOW);
    *out = (size_t)count * item_size;
    return ok_status();
}

static arbor_status next_generation(g09_context *c, uint64_t *out) {
    if (c->generation == UINT64_MAX) return err_status(EOVERFLOW);
    c->generation += 1u;
    *out = c->generation;
    return ok_status();
}

static g09_colgroup *column_group_at(g09_context *c, g09_table *t, uint64_t x) {
    for (g09_colgroup *cg = c->colgroups; cg != NULL; cg = cg->next)
        if (cg->table == t && cg->x0 <= x && x < cg->x1) return cg;
    return NULL;
}

static arbor_status build_column_groups(g09_context *c, g09_table *t) {
    uint64_t x = 0u;
    for (g09_colgroup *cg = c->colgroups; cg != NULL; cg = cg->next) {
        if (cg->table != t) continue;
        uint64_t width = cg->span;
        bool group_violation = false;
        if (cg->direct_col_count != 0u) {
            width = 0u;
            if (cg->span_present) group_violation = true;
            for (g09_col *col = c->cols; col != NULL; col = col->next) {
                if (col->group != cg) continue;
                if (col->span_range_invalid) {
                    arbor_status s = emit_at(c, UINT16_C(3), col->source_offset,
                        col->source_length, G09_ATTR_SPAN);
                    if (s.native != 0) return s;
                }
                if (!checked_add_u64(width, col->span, &width)) return err_status(EOVERFLOW);
            }
            cg->child_span_total = width;
        }
        if (cg->span_lexically_valid && !cg->span_valid) group_violation = true;
        if (group_violation) {
            arbor_status s = emit_at(c, UINT16_C(3), cg->source_offset,
                cg->source_length, G09_ATTR_SPAN);
            if (s.native != 0) return s;
        }
        cg->x0 = x;
        if (!checked_add_u64(x, width, &cg->x1)) return err_status(EOVERFLOW);
        x = cg->x1;
    }
    if (x > t->width) t->width = x;
    return ok_status();
}

static arbor_status emit_cell_rule(g09_context *c, const g09_cell *cell,
                                   uint16_t rule, g09_attr_code code) {
    return emit_at(c, rule, cell->source_offset, cell->source_length, code);
}

static bool row_is_deferred_tfoot(const g09_row *row) {
    return row->group != NULL && row->group->kind == UINT8_C(3);
}

static arbor_status place_cells(g09_context *c, g09_table *t) {
    uint64_t y = 0u;
    for (uint8_t pass = 0u; pass < UINT8_C(2); ++pass) {
        const bool deferred = pass != 0u;
        for (g09_row *r = t->rows; r != NULL; r = r->next) {
            if (row_is_deferred_tfoot(r) != deferred) continue;
            r->y = y;
            if (!checked_add_u64(y, 1u, &y)) return err_status(EOVERFLOW);
            if (r->group != NULL) {
                if (r->group->end_y == 0u) r->group->first_y = r->y;
                r->group->end_y = y;
            }
        }
    }
    t->height = y;

    for (uint8_t pass = 0u; pass < UINT8_C(2); ++pass) {
        const bool deferred = pass != 0u;
        for (g09_row *r = t->rows; r != NULL; r = r->next) {
            if (row_is_deferred_tfoot(r) != deferred) continue;
            uint64_t x = 0u;
            for (g09_cell *cell = r->cells; cell != NULL; cell = cell->next) {
                if (cell->colspan_range_invalid) {
                    arbor_status s = emit_cell_rule(c, cell, UINT16_C(6), G09_ATTR_COLSPAN);
                    if (s.native != 0) return s;
                }
                if (cell->rowspan_range_invalid) {
                    arbor_status s = emit_cell_rule(c, cell, UINT16_C(6), G09_ATTR_ROWSPAN);
                    if (s.native != 0) return s;
                }
                for (;;) {
                    uint64_t advanced = x;
                    for (g09_row *prior_row = t->rows; prior_row != NULL;
                         prior_row = prior_row->next)
                        for (g09_cell *prior = prior_row->cells; prior != NULL;
                             prior = prior->next)
                            if (prior != cell && prior->x1 != 0u && prior->x0 <= x &&
                                x < prior->x1 && prior->y0 <= r->y && r->y < prior->y1 &&
                                prior->x1 > advanced) advanced = prior->x1;
                    if (advanced == x) break;
                    x = advanced;
                }
                uint64_t x1 = 0u;
                uint64_t y1 = 0u;
                if (!checked_add_u64(x, cell->colspan, &x1)) return err_status(EOVERFLOW);
                if (cell->rowspan == 0u)
                    y1 = r->group != NULL ? r->group->end_y : t->height;
                else if (!checked_add_u64(r->y, cell->rowspan, &y1))
                    return err_status(EOVERFLOW);
                cell->x0 = x;
                cell->x1 = x1;
                cell->y0 = r->y;
                cell->y1 = y1;

                bool overlap = false;
                for (g09_row *prior_row = t->rows; prior_row != NULL;
                     prior_row = prior_row->next)
                    for (g09_cell *prior = prior_row->cells; prior != NULL;
                         prior = prior->next)
                        if (prior != cell && prior->x1 != 0u &&
                            intersects(prior, cell->x0, cell->x1, cell->y0, cell->y1))
                            overlap = true;
                if (overlap) {
                    arbor_status s = emit_cell_rule(c, cell, UINT16_C(1), G09_ATTR_ROWSPAN);
                    if (s.native != 0) return s;
                    s = emit_cell_rule(c, cell, UINT16_C(6), G09_ATTR_ROWSPAN);
                    if (s.native != 0) return s;
                }
                if (r->group == NULL && y1 > t->height) t->height = y1;
                if (r->group != NULL && y1 > r->group->end_y) {
                    arbor_status s = emit_cell_rule(c, cell, UINT16_C(1), G09_ATTR_ROWSPAN);
                    if (s.native != 0) return s;
                    s = emit_cell_rule(c, cell, UINT16_C(6), G09_ATTR_ROWSPAN);
                    if (s.native != 0) return s;
                    s = emit_cell_rule(c, cell, UINT16_C(4), G09_ATTR_ROWSPAN);
                    if (s.native != 0) return s;
                }
                if (x1 > t->width) t->width = x1;
                x = x1;
            }
        }
    }
    return ok_status();
}

static int compare_u64(const void *left, const void *right) {
    const uint64_t a = *(const uint64_t *)left;
    const uint64_t b = *(const uint64_t *)right;
    return a < b ? -1 : (a > b ? 1 : 0);
}

static uint64_t coverage_cell_count(const g09_table *t) {
    uint64_t count = 0u;
    for (const g09_row *row = t->rows; row != NULL; row = row->next)
        for (const g09_cell *cell = row->cells; cell != NULL; cell = cell->next) {
            if (count == UINT64_MAX) return UINT64_MAX;
            count += 1u;
        }
    return count;
}

static arbor_status verify_sparse_coverage(g09_context *c, g09_table *t) {
    if (t->width == 0u || t->height == 0u) return ok_status();

    const uint64_t cell_count = coverage_cell_count(t);
    uint64_t event_capacity = 0u;
    if (cell_count == UINT64_MAX ||
        !checked_add_u64(cell_count, cell_count, &event_capacity) ||
        !checked_add_u64(event_capacity, UINT64_C(2), &event_capacity) ||
        event_capacity > (uint64_t)(SIZE_MAX / sizeof(uint64_t)))
        return err_status(EOVERFLOW);
    uint64_t *events = support_calloc(c, (size_t)event_capacity * sizeof(*events));
    if (events == NULL) return err_status(ENOMEM);
    uint64_t event_count = 0u;
    events[event_count++] = 0u;
    events[event_count++] = t->height;
    for (const g09_row *row = t->rows; row != NULL; row = row->next)
        for (const g09_cell *cell = row->cells; cell != NULL; cell = cell->next) {
            if (cell->y0 <= t->height) events[event_count++] = cell->y0;
            if (cell->y1 <= t->height) events[event_count++] = cell->y1;
        }
    qsort(events, (size_t)event_count, sizeof(*events), compare_u64);
    uint64_t unique_count = 0u;
    for (uint64_t i = 0u; i < event_count; ++i)
        if (unique_count == 0u || events[i] != events[unique_count - 1u])
            events[unique_count++] = events[i];

    for (uint64_t i = 0u; i + 1u < unique_count; ++i) {
        const uint64_t y0 = events[i];
        const uint64_t y1 = events[i + 1u];
        if (y0 == y1) continue;
        uint64_t covered = 0u;
        for (;;) {
            uint64_t extension = covered;
            for (g09_row *rr = t->rows; rr != NULL; rr = rr->next)
                for (g09_cell *cell = rr->cells; cell != NULL; cell = cell->next)
                    if (cell->y0 <= y0 && y1 <= cell->y1 &&
                        cell->x0 <= covered && covered < cell->x1 && cell->x1 > extension)
                        extension = cell->x1;
            if (extension == covered) break;
            covered = extension;
        }
        if (covered < t->width) {
            const g09_row *anchor = NULL;
            for (const g09_row *row = t->rows; row != NULL; row = row->next)
                if (row->y == y0) { anchor = row; break; }
            const uint64_t offset = anchor == NULL ? t->source_offset : anchor->source_offset;
            const uint64_t length = anchor == NULL ? t->source_length : anchor->source_length;
            arbor_status s = emit_at(c, UINT16_C(1), offset, length, G09_ATTR_NONE);
            if (s.native != 0) return s;
        }
    }
    return ok_status();
}

static arbor_status append_edge(g09_context *c, g09_cell *cell, arbor_span token,
                                g09_cell *target) {
    g09_edge *edge = support_calloc(c, sizeof(*edge));
    if (edge == NULL) return err_status(ENOMEM);
    edge->token = token;
    edge->target = target;
    if (cell->edges_tail == NULL) cell->edges = edge; else cell->edges_tail->next = edge;
    cell->edges_tail = edge;
    return ok_status();
}

static arbor_status build_explicit_edges(g09_context *c, g09_table *t) {
    for (g09_row *r = t->rows; r != NULL; r = r->next)
        for (g09_cell *cell = r->cells; cell != NULL; cell = cell->next) {
            if (!cell->headers_present) continue;
            uint64_t cursor = 0u;
            arbor_span token = {0};
            while (token_next(cell->headers, &cursor, &token)) {
                if (c->evaluation.header_token_count == UINT64_MAX) return err_status(EOVERFLOW);
                c->evaluation.header_token_count += 1u;
                bool duplicate = false;
                for (g09_edge *prior = cell->edges; prior != NULL; prior = prior->next)
                    if (span_eq(prior->token, token)) duplicate = true;
                const g09_id_entry *entry = find_id(c, token);
                g09_cell *target = entry == NULL ? NULL : entry->cell;
                const bool valid = !duplicate && entry != NULL && target != NULL &&
                    target->is_header && entry->table == t && target != cell;
                arbor_status s = append_edge(c, cell, token, valid ? target : NULL);
                if (s.native != 0) return s;
                if (!valid) {
                    s = emit_cell_rule(c, cell, UINT16_C(6), G09_ATTR_HEADERS);
                    if (s.native != 0) return s;
                }
            }
        }
    return ok_status();
}

static uint64_t table_cell_count(const g09_table *t) {
    uint64_t count = 0u;
    for (const g09_row *r = t->rows; r != NULL; r = r->next)
        for (const g09_cell *cell = r->cells; cell != NULL; cell = cell->next) {
            if (count == UINT64_MAX) return UINT64_MAX;
            count += 1u;
        }
    return count;
}

static arbor_status detect_explicit_cycles(g09_context *c, g09_table *t) {
    const uint64_t count = table_cell_count(t);
    if (count == UINT64_MAX || count > (uint64_t)(SIZE_MAX / sizeof(g09_cell *)) ||
        count > (uint64_t)(SIZE_MAX / sizeof(g09_edge *))) return err_status(EOVERFLOW);
    if (count == 0u) return ok_status();
    g09_cell **stack = support_calloc(c, (size_t)count * sizeof(*stack));
    g09_edge **next = support_calloc(c, (size_t)count * sizeof(*next));
    if (stack == NULL || next == NULL) return err_status(ENOMEM);

    for (g09_row *r = t->rows; r != NULL; r = r->next)
        for (g09_cell *root = r->cells; root != NULL; root = root->next) {
            if (root->graph_color != 0u || root->edges == NULL) continue;
            uint64_t depth = 0u;
            bool cycle = false;
            stack[0] = root;
            next[0] = root->edges;
            root->graph_color = UINT8_C(1);
            for (;;) {
                g09_edge *edge = next[depth];
                if (edge == NULL) {
                    stack[depth]->graph_color = UINT8_C(2);
                    if (depth == 0u) break;
                    depth -= 1u;
                    continue;
                }
                next[depth] = edge->next;
                if (edge->target == NULL) continue;
                if (edge->target->graph_color == UINT8_C(1)) {
                    cycle = true;
                    continue;
                }
                if (edge->target->graph_color == UINT8_C(2)) continue;
                if (depth + 1u >= count) return err_status(EOVERFLOW);
                depth += 1u;
                stack[depth] = edge->target;
                next[depth] = edge->target->edges;
                edge->target->graph_color = UINT8_C(1);
            }
            if (cycle) {
                arbor_status s = emit_cell_rule(c, root, UINT16_C(6), G09_ATTR_HEADERS);
                if (s.native != 0) return s;
            }
        }
    return ok_status();
}

typedef enum g09_scope_kind {
    G09_SCOPE_AUTO = 0,
    G09_SCOPE_ROW,
    G09_SCOPE_COL,
    G09_SCOPE_ROWGROUP,
    G09_SCOPE_COLGROUP
} g09_scope_kind;

static g09_scope_kind scope_kind(const g09_cell *cell) {
    if (span_eq_ci(cell->scope, "row")) return G09_SCOPE_ROW;
    if (span_eq_ci(cell->scope, "col")) return G09_SCOPE_COL;
    if (span_eq_ci(cell->scope, "rowgroup")) return G09_SCOPE_ROWGROUP;
    if (span_eq_ci(cell->scope, "colgroup")) return G09_SCOPE_COLGROUP;
    return G09_SCOPE_AUTO;
}

static bool data_intersects_y(const g09_table *t, uint64_t y0, uint64_t y1) {
    for (const g09_row *r = t->rows; r != NULL; r = r->next)
        for (const g09_cell *cell = r->cells; cell != NULL; cell = cell->next)
            if (!cell->is_header && cell->y0 < y1 && y0 < cell->y1) return true;
    return false;
}

static bool data_intersects_x(const g09_table *t, uint64_t x0, uint64_t x1) {
    for (const g09_row *r = t->rows; r != NULL; r = r->next)
        for (const g09_cell *cell = r->cells; cell != NULL; cell = cell->next)
            if (!cell->is_header && cell->x0 < x1 && x0 < cell->x1) return true;
    return false;
}

static bool is_column_header(const g09_table *t, const g09_cell *cell) {
    const g09_scope_kind scope = scope_kind(cell);
    return scope == G09_SCOPE_COL ||
        (scope == G09_SCOPE_AUTO && !data_intersects_y(t, cell->y0, cell->y1));
}

static bool is_row_header(const g09_table *t, const g09_cell *cell) {
    const g09_scope_kind scope = scope_kind(cell);
    return scope == G09_SCOPE_ROW || (scope == G09_SCOPE_AUTO &&
        !is_column_header(t, cell) && !data_intersects_x(t, cell->x0, cell->x1));
}

static g09_cell *nearest_horizontal(g09_table *t, uint64_t boundary, uint64_t y) {
    g09_cell *best = NULL;
    uint64_t best_end = 0u;
    for (g09_row *r = t->rows; r != NULL; r = r->next)
        for (g09_cell *cell = r->cells; cell != NULL; cell = cell->next)
            if (cell->x1 <= boundary && cell->y0 <= y && y < cell->y1 &&
                (best == NULL || cell->x1 > best_end)) {
                best = cell;
                best_end = cell->x1;
            }
    return best;
}

static g09_cell *nearest_vertical(g09_table *t, uint64_t x, uint64_t boundary) {
    g09_cell *best = NULL;
    uint64_t best_end = 0u;
    for (g09_row *r = t->rows; r != NULL; r = r->next)
        for (g09_cell *cell = r->cells; cell != NULL; cell = cell->next)
            if (cell->y1 <= boundary && cell->x0 <= x && x < cell->x1 &&
                (best == NULL || cell->y1 > best_end)) {
                best = cell;
                best_end = cell->y1;
            }
    return best;
}

static bool blocked_by_opaque(const g09_table *t, const g09_cell *cell,
                              uint64_t scan, bool vertical) {
    for (const g09_row *r = t->rows; r != NULL; r = r->next)
        for (const g09_cell *opaque = r->cells; opaque != NULL; opaque = opaque->next)
            if (opaque->block_mark == scan &&
                ((vertical && opaque->x0 == cell->x0 &&
                  opaque->x1 - opaque->x0 == cell->x1 - cell->x0) ||
                 (!vertical && opaque->y0 == cell->y0 &&
                  opaque->y1 - opaque->y0 == cell->y1 - cell->y0))) return true;
    return false;
}

static void close_header_block(g09_table *t, uint64_t scan) {
    for (g09_row *r = t->rows; r != NULL; r = r->next)
        for (g09_cell *cell = r->cells; cell != NULL; cell = cell->next)
            if (cell->pending_mark == scan) {
                cell->block_mark = scan;
                cell->pending_mark = 0u;
            }
}

static arbor_status scan_headers(g09_context *c, g09_table *t, g09_cell *principal,
                                 uint64_t association, uint64_t coordinate,
                                 bool vertical) {
    uint64_t scan = 0u;
    arbor_status status = next_generation(c, &scan);
    if (status.native != 0) return status;
    bool in_header_block = principal->is_header;
    if (in_header_block) principal->pending_mark = scan;
    uint64_t boundary = vertical ? principal->y0 : principal->x0;
    while (boundary != 0u) {
        g09_cell *current = vertical ? nearest_vertical(t, coordinate, boundary) :
            nearest_horizontal(t, boundary, coordinate);
        if (current == NULL) break;
        boundary = vertical ? current->y0 : current->x0;
        if (current->is_header) {
            in_header_block = true;
            current->pending_mark = scan;
            const bool blocked = blocked_by_opaque(t, current, scan, vertical) ||
                (vertical ? !is_column_header(t, current) : !is_row_header(t, current));
            if (!blocked) current->association_mark = association;
        } else if (in_header_block) {
            in_header_block = false;
            close_header_block(t, scan);
        }
    }
    return ok_status();
}

static arbor_status implicit_associations(g09_context *c, g09_table *t,
                                          g09_cell *principal) {
    uint64_t association = 0u;
    arbor_status status = next_generation(c, &association);
    if (status.native != 0) return status;
    for (uint64_t y = principal->y0; y < principal->y1; ++y) {
        status = scan_headers(c, t, principal, association, y, false);
        if (status.native != 0) return status;
    }
    for (uint64_t x = principal->x0; x < principal->x1; ++x) {
        status = scan_headers(c, t, principal, association, x, true);
        if (status.native != 0) return status;
    }
    g09_colgroup *principal_colgroup = column_group_at(c, t, principal->x0);
    for (g09_row *r = t->rows; r != NULL; r = r->next)
        for (g09_cell *header = r->cells; header != NULL; header = header->next) {
            if (!header->is_header) continue;
            const g09_scope_kind scope = scope_kind(header);
            if (scope == G09_SCOPE_ROWGROUP && principal->row->group != NULL &&
                header->row->group == principal->row->group && header->x0 < principal->x1 &&
                header->y0 < principal->y1) header->association_mark = association;
            if (scope == G09_SCOPE_COLGROUP && principal_colgroup != NULL &&
                column_group_at(c, t, header->x0) == principal_colgroup &&
                header->x0 < principal->x1 && header->y0 < principal->y1)
                header->association_mark = association;
        }
    for (g09_row *r = t->rows; r != NULL; r = r->next)
        for (g09_cell *header = r->cells; header != NULL; header = header->next)
            if (header->association_mark == association && header != principal &&
                !header->empty) {
                if (c->evaluation.implicit_header_association_count == UINT64_MAX)
                    return err_status(EOVERFLOW);
                c->evaluation.implicit_header_association_count += 1u;
            }
    return ok_status();
}

static arbor_status evaluate_scopes_and_associations(g09_context *c, g09_table *t) {
    for (g09_row *r = t->rows; r != NULL; r = r->next)
        for (g09_cell *cell = r->cells; cell != NULL; cell = cell->next) {
            if (cell->is_header && cell->scope.data != NULL) {
                const bool known = span_eq_ci(cell->scope, "row") ||
                    span_eq_ci(cell->scope, "col") || span_eq_ci(cell->scope, "rowgroup") ||
                    span_eq_ci(cell->scope, "colgroup") || span_eq_ci(cell->scope, "auto");
                if (!known) c->evaluation.prior_owner_suppression_count += 1u;
                const g09_scope_kind scope = scope_kind(cell);
                if ((scope == G09_SCOPE_ROWGROUP && r->group == NULL) ||
                    (scope == G09_SCOPE_COLGROUP && column_group_at(c, t, cell->x0) == NULL)) {
                    arbor_status s = emit_cell_rule(c, cell, UINT16_C(5), G09_ATTR_SCOPE);
                    if (s.native != 0) return s;
                }
            }
            if (!cell->headers_present) {
                arbor_status s = implicit_associations(c, t, cell);
                if (s.native != 0) return s;
            }
        }
    return ok_status();
}

static arbor_status evaluate_table(g09_context *c, g09_table *t) {
    if (t->direct_caption_count > 1u) {
        arbor_status s = emit_at(c, UINT16_C(2), t->source_offset, t->source_length,
            G09_ATTR_NONE);
        if (s.native != 0) return s;
    }
    arbor_status status = build_column_groups(c, t);
    if (status.native != 0) return status;
    status = place_cells(c, t);
    if (status.native != 0) return status;
    status = verify_sparse_coverage(c, t);
    if (status.native != 0) return status;
    status = build_explicit_edges(c, t);
    if (status.native != 0) return status;
    status = detect_explicit_cycles(c, t);
    if (status.native != 0) return status;
    return evaluate_scopes_and_associations(c, t);
}

static arbor_status traversal_leave(void *opaque,
                                    const arbor_view0_native_element_observation *o) {
    if (opaque == NULL || o == NULL) return err_status(EINVAL);
    g09_context *c = opaque;
    if (c->frame_count == 0u) return err_status(EIO);
    const g09_frame *f = c->frames + c->frame_count - 1u;
    if (f->source_offset != o->source_offset || f->depth != o->depth) return err_status(EIO);
    if (o->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_HTML && o->depth == 0u)
        for (g09_table *t = c->tables; t != NULL; t = t->next) {
            arbor_status s = evaluate_table(c, t); if (s.native != 0) return s;
        }
    c->frame_count -= 1u;
    return ok_status();
}

static arbor_status evaluate(arbor_span input, arbor_view0_native_v1n2_g09_anchor *anchors,
    uint64_t anchor_capacity, bool collect,
    arbor_view0_native_v1n2_g09_evaluation *evaluation_out) {
    if (evaluation_out == NULL || (collect && anchor_capacity != 0u && anchors == NULL))
        return err_status(EINVAL);
    lexbor_mraw_t *arena = lexbor_mraw_create();
    if (arena == NULL) return err_status(ENOMEM);
    if (lexbor_mraw_init(arena, 4096u) != LXB_STATUS_OK) {
        (void)lexbor_mraw_destroy(arena, true); return err_status(ENOMEM);
    }
    g09_context c = {.arena = arena, .anchors = anchors, .anchor_capacity = anchor_capacity,
        .collect = collect};
    c.evaluation.deferred_external_semantics_count = UINT64_C(3);
    const arbor_view0_native_semantic_observer observer = {
        .context = &c, .element_begin = element_begin, .attribute = attribute,
        .direct_child = direct_child,
        .element_complete = element_complete, .traversal_enter = traversal_enter,
        .traversal_leave = traversal_leave, .source_attribute = source_attribute
    };
    arbor_view0_native_parse_counts parse_counts = {0};
    arbor_view0_native_document_facts facts = {0};
    arbor_view0_native_observation_counts counts = {0};
    arbor_status status = arbor_view0_native_lexbor_observe(
        input, &observer, &parse_counts, &facts, &counts);
    if (status.native == 0 && c.frame_count != 0u) status = err_status(EIO);
    if (status.native == 0 && collect && c.evaluation.diagnostic_count != anchor_capacity)
        status = err_status(EIO);
    if (status.native == 0) *evaluation_out = c.evaluation;
    (void)lexbor_mraw_destroy(arena, true);
    return status;
}

arbor_status arbor_view0_native_v1n2_g09_measure(
    arbor_span input, arbor_view0_native_v1n2_g09_evaluation *evaluation_out) {
    return evaluate(input, NULL, 0u, false, evaluation_out);
}

arbor_status arbor_view0_native_v1n2_g09_collect_anchors(
    arbor_span input, arbor_view0_native_v1n2_g09_anchor *anchors, uint64_t anchor_capacity,
    arbor_view0_native_v1n2_g09_evaluation *evaluation_out) {
    return evaluate(input, anchors, anchor_capacity, true, evaluation_out);
}

void arbor_view0_native_v1n2_g09_materialize_anchor(
    const arbor_view0_native_v1n2_g09_anchor *anchor, uint64_t discovery_sequence,
    arbor_view0_native_diagnostic *diagnostic) {
    static const char *const messages[] = {
        "Table model contains an uncovered, overlapping, or row-group-crossing slot",
        "Table caption cardinality or placement violates the frozen static boundary",
        "Column or column-group span semantics violate the frozen static boundary",
        "Table row-group interval semantics violate the frozen static boundary",
        "Header scope is inapplicable to the cell's table-model relationship",
        "Table cell span or header association violates the frozen static boundary"
    };
    if (anchor == NULL || diagnostic == NULL || anchor->shared.rule_ordinal == 0u ||
        anchor->shared.rule_ordinal > ARBOR_VIEW0_NATIVE_V1N2_G09_RULE_COUNT) return;
    const arbor_view0_native_v1n2_rule_meta *meta = arbor_view0_native_v1n2_c0_rule_at(
        UINT64_C(17) + (uint64_t)anchor->shared.rule_ordinal - 1u);
    if (meta == NULL || meta->group != ARBOR_VIEW0_NATIVE_V1N2_GROUP_G09) return;
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
