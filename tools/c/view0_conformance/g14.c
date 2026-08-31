#include "g14.h"

#include <errno.h>
#include <string.h>

static arbor_status ea(arbor_view0_native_v1n3_context *c, uint16_t r, const arbor_view0_native_v1n3_attr *a)
{ return arbor_view0_native_v1n3_emit(c, r, a->name_offset, a->source_length, 0u); }
static arbor_status ee(arbor_view0_native_v1n3_context *c, uint16_t r, const arbor_view0_native_v1n3_element *e)
{ return arbor_view0_native_v1n3_emit(c, r, e->source_offset, e->source_length, 0u); }

static bool duplicate_tokens(arbor_span value)
{
    uint64_t i = 0u; while (i < value.length) {
        while (i < value.length && value.data[i] == (uint8_t)' ') i += 1u;
        const uint64_t a = i; while (i < value.length && value.data[i] != (uint8_t)' ') i += 1u;
        uint64_t j = i; while (j < value.length) {
            while (j < value.length && value.data[j] == (uint8_t)' ') j += 1u;
            const uint64_t b = j; while (j < value.length && value.data[j] != (uint8_t)' ') j += 1u;
            if (i - a == j - b && i != a && memcmp(value.data + a, value.data + b, (size_t)(i - a)) == 0) return true;
        }
    }
    return false;
}

static bool all_absolute(arbor_span value)
{
    uint64_t i = 0u; bool any = false;
    while (i < value.length) {
        while (i < value.length && value.data[i] == (uint8_t)' ') i += 1u;
        const uint64_t start = i; while (i < value.length && value.data[i] != (uint8_t)' ') i += 1u;
        if (i != start) { any = true; const arbor_span token = {value.data + start, i - start}; if (!arbor_view0_native_v1n3_absolute_url(token)) return false; }
    }
    return any;
}

arbor_status arbor_view0_native_v1n3_g14_validate_element(
    arbor_view0_native_v1n3_context *c, const arbor_view0_native_v1n3_element *e)
{
    if (c == NULL || e == NULL) return arbor_status_from_native(-(int64_t)EINVAL);
    if (e->template_opaque) return arbor_status_from_native(0);
    const arbor_view0_native_v1n3_attr *scope = arbor_view0_native_v1n3_attr_find(e, "itemscope");
    const arbor_view0_native_v1n3_attr *type = arbor_view0_native_v1n3_attr_find(e, "itemtype");
    const arbor_view0_native_v1n3_attr *id = arbor_view0_native_v1n3_attr_find(e, "itemid");
    const arbor_view0_native_v1n3_attr *ref = arbor_view0_native_v1n3_attr_find(e, "itemref");
    const arbor_view0_native_v1n3_attr *prop = arbor_view0_native_v1n3_attr_find(e, "itemprop");
    if (type != NULL && (scope == NULL || !all_absolute(type->value) || duplicate_tokens(type->value))) { arbor_status s = ea(c, 1u, type); if (s.native != 0) return s; }
    if (id != NULL && (scope == NULL || type == NULL || !arbor_view0_native_v1n3_absolute_url(id->value))) { arbor_status s = ea(c, 2u, id); if (s.native != 0) return s; }
    if (ref != NULL && (scope == NULL || duplicate_tokens(ref->value))) { arbor_status s = ea(c, 3u, ref); if (s.native != 0) return s; }
    if (prop != NULL && (prop->value.length == 0u || duplicate_tokens(prop->value))) { arbor_status s = ea(c, 4u, prop); if (s.native != 0) return s; }
    if (prop != NULL) {
        const char *required = NULL;
        if (arbor_view0_native_v1n3_ascii_case_equal(e->name, "meta")) required = "content";
        else if (arbor_view0_native_v1n3_ascii_case_equal(e->name, "audio") || arbor_view0_native_v1n3_ascii_case_equal(e->name, "embed") ||
                 arbor_view0_native_v1n3_ascii_case_equal(e->name, "iframe") || arbor_view0_native_v1n3_ascii_case_equal(e->name, "img") ||
                 arbor_view0_native_v1n3_ascii_case_equal(e->name, "source") || arbor_view0_native_v1n3_ascii_case_equal(e->name, "track") ||
                 arbor_view0_native_v1n3_ascii_case_equal(e->name, "video")) required = "src";
        else if (arbor_view0_native_v1n3_ascii_case_equal(e->name, "link")) required = "href";
        else if (arbor_view0_native_v1n3_ascii_case_equal(e->name, "object")) required = "data";
        else if (arbor_view0_native_v1n3_ascii_case_equal(e->name, "data") || arbor_view0_native_v1n3_ascii_case_equal(e->name, "meter")) required = "value";
        else if (arbor_view0_native_v1n3_ascii_case_equal(e->name, "time")) required = "datetime";
        if (required != NULL && arbor_view0_native_v1n3_attr_find(e, required) == NULL) { arbor_status s = ee(c, 5u, e); if (s.native != 0) return s; }
    }
    return arbor_status_from_native(0);
}

arbor_status arbor_view0_native_v1n3_g14_finalize(arbor_view0_native_v1n3_context *c)
{
    for (const arbor_view0_native_v1n3_element *e = c->elements; e != NULL; e = e->next) {
        if (e->template_opaque) continue;
        const arbor_view0_native_v1n3_attr *ref = arbor_view0_native_v1n3_attr_find(e, "itemref");
        if (ref != NULL) {
            uint64_t i = 0u; while (i < ref->value.length) {
                while (i < ref->value.length && ref->value.data[i] == (uint8_t)' ') i += 1u;
                const uint64_t start = i; while (i < ref->value.length && ref->value.data[i] != (uint8_t)' ') i += 1u;
                if (i != start) {
                    const arbor_span token = {ref->value.data + start, i - start};
                    const arbor_view0_native_v1n3_element *target = arbor_view0_native_v1n3_id_find(c, token);
                    if (target == NULL || arbor_view0_native_v1n3_is_descendant(target, e)) { arbor_status s = ea(c, 3u, ref); if (s.native != 0) return s; break; }
                }
            }
        }
        const arbor_view0_native_v1n3_attr *prop = arbor_view0_native_v1n3_attr_find(e, "itemprop");
        if (prop != NULL) {
            bool owned = false;
            for (const arbor_view0_native_v1n3_element *p = e; p != NULL; p = p->parent)
                if (arbor_view0_native_v1n3_attr_find(p, "itemscope") != NULL) { owned = true; break; }
            if (!owned) { arbor_status s = ea(c, 6u, prop); if (s.native != 0) return s; }
        }
    }
    return arbor_status_from_native(0);
}

arbor_status arbor_view0_native_v1n3_g14_measure(arbor_span i, const arbor_view0_native_v1n3_options *o, void *a, arbor_view0_native_v1n3_g14_evaluation *e)
{ return arbor_view0_native_v1n3_run_group(14u, i, o, a, NULL, 0u, e); }
arbor_status arbor_view0_native_v1n3_g14_collect_anchors(arbor_span i, const arbor_view0_native_v1n3_options *o, void *a, arbor_view0_native_v1n3_g14_anchor *n, uint64_t c, arbor_view0_native_v1n3_g14_evaluation *e)
{ return arbor_view0_native_v1n3_run_group(14u, i, o, a, n, c, e); }
void arbor_view0_native_v1n3_g14_materialize_anchor(const arbor_view0_native_v1n3_g14_anchor *a, uint64_t s, arbor_view0_native_diagnostic *d)
{ arbor_view0_native_v1n3_materialize(a, s, d); }
