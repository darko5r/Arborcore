#include "g12.h"

#include <errno.h>
#include <string.h>

static arbor_status emit_element(arbor_view0_native_v1n3_context *c, uint16_t r,
                                 const arbor_view0_native_v1n3_element *e)
{ return arbor_view0_native_v1n3_emit(c, r, e->source_offset, e->source_length, 0u); }
static arbor_status emit_attr(arbor_view0_native_v1n3_context *c, uint16_t r,
                              const arbor_view0_native_v1n3_attr *a)
{ return arbor_view0_native_v1n3_emit(c, r, a->name_offset, a->source_length, 0u); }

arbor_status arbor_view0_native_v1n3_g12_validate_element(
    arbor_view0_native_v1n3_context *c, const arbor_view0_native_v1n3_element *e)
{
    if (c == NULL || e == NULL) return arbor_status_from_native(-(int64_t)EINVAL);
    if (e->template_opaque) return arbor_status_from_native(0);
    if (arbor_view0_native_v1n3_ascii_case_equal(e->name, "script")) {
        const arbor_view0_native_v1n3_attr *type = arbor_view0_native_v1n3_attr_find(e, "type");
        const arbor_view0_native_v1n3_attr *language = arbor_view0_native_v1n3_attr_find(e, "language");
        const arbor_view0_native_v1n3_attr *src = arbor_view0_native_v1n3_attr_find(e, "src");
        const arbor_view0_native_v1n3_attr *nomodule = arbor_view0_native_v1n3_attr_find(e, "nomodule");
        const arbor_view0_native_v1n3_attr *async = arbor_view0_native_v1n3_attr_find(e, "async");
        const arbor_view0_native_v1n3_attr *defer = arbor_view0_native_v1n3_attr_find(e, "defer");
        const arbor_view0_native_v1n3_attr *integrity = arbor_view0_native_v1n3_attr_find(e, "integrity");
        const bool module = type != NULL && arbor_view0_native_v1n3_ascii_case_equal(type->value, "module");
        const bool data = type != NULL && (arbor_view0_native_v1n3_ascii_case_equal(type->value, "importmap") ||
            arbor_view0_native_v1n3_ascii_case_equal(type->value, "speculationrules"));
        if ((language != NULL && type != NULL) || (data && src != NULL)) {
            arbor_status s = emit_attr(c, 1u, language != NULL && type != NULL ? language : src); if (s.native != 0) return s;
        }
        if ((module && nomodule != NULL) || (data && (nomodule != NULL || async != NULL || defer != NULL)) ||
            (integrity != NULL && src == NULL)) {
            const arbor_view0_native_v1n3_attr *a = module && nomodule != NULL ? nomodule :
                data && async != NULL ? async : data && defer != NULL ? defer : data && nomodule != NULL ? nomodule : integrity;
            arbor_status s = emit_attr(c, 2u, a); if (s.native != 0) return s;
        }
        if (arbor_view0_native_v1n3_ascii_contains(e->text, "<!--") ||
            arbor_view0_native_v1n3_ascii_contains(e->text, "<script")) {
            arbor_status s = emit_element(c, 3u, e); if (s.native != 0) return s;
        }
    }
    if (arbor_view0_native_v1n3_ascii_case_equal(e->name, "noscript") &&
        e->parent != NULL && arbor_view0_native_v1n3_ascii_case_equal(e->parent->name, "noscript")) {
        arbor_status s = emit_element(c, 4u, e); if (s.native != 0) return s;
    }
    if (arbor_view0_native_v1n3_ascii_case_equal(e->name, "template")) {
        const arbor_view0_native_v1n3_attr *mode = arbor_view0_native_v1n3_attr_find(e, "shadowrootmode");
        const arbor_view0_native_v1n3_attr *focus = arbor_view0_native_v1n3_attr_find(e, "shadowrootdelegatesfocus");
        const arbor_view0_native_v1n3_attr *clone = arbor_view0_native_v1n3_attr_find(e, "shadowrootclonable");
        const arbor_view0_native_v1n3_attr *serial = arbor_view0_native_v1n3_attr_find(e, "shadowrootserializable");
        const arbor_view0_native_v1n3_attr *registry = arbor_view0_native_v1n3_attr_find(e, "shadowrootcustomelementregistry");
        if ((mode != NULL && !arbor_view0_native_v1n3_ascii_case_equal(mode->value, "open") &&
            !arbor_view0_native_v1n3_ascii_case_equal(mode->value, "closed")) ||
            (mode == NULL && (focus != NULL || clone != NULL || serial != NULL || registry != NULL))) {
            const arbor_view0_native_v1n3_attr *a = mode != NULL ? mode : focus != NULL ? focus : clone != NULL ? clone : serial != NULL ? serial : registry;
            arbor_status s = emit_attr(c, 5u, a); if (s.native != 0) return s;
        }
    }
    return arbor_status_from_native(0);
}

static bool has_child(const arbor_view0_native_v1n3_context *c,
                      const arbor_view0_native_v1n3_element *parent)
{ for (const arbor_view0_native_v1n3_element *e = c->elements; e != NULL; e = e->next) if (e->parent == parent) return true; return false; }

arbor_status arbor_view0_native_v1n3_g12_finalize(arbor_view0_native_v1n3_context *c)
{
    for (const arbor_view0_native_v1n3_element *e = c->elements; e != NULL; e = e->next) {
        if (e->template_opaque) continue;
        if (arbor_view0_native_v1n3_ascii_case_equal(e->name, "slot")) {
            const arbor_view0_native_v1n3_attr *name = arbor_view0_native_v1n3_attr_find(e, "name");
            if (name != NULL) for (const arbor_view0_native_v1n3_element *o = c->elements; o != e; o = o->next) {
                const arbor_view0_native_v1n3_attr *other = arbor_view0_native_v1n3_attr_find(o, "name");
                if (o->parent == e->parent && arbor_view0_native_v1n3_ascii_case_equal(o->name, "slot") && other != NULL &&
                    other->value.length == name->value.length && memcmp(other->value.data, name->value.data, (size_t)name->value.length) == 0) {
                    arbor_status s = emit_attr(c, 6u, name); if (s.native != 0) return s; break;
                }
            }
        }
        if (arbor_view0_native_v1n3_ascii_case_equal(e->name, "canvas") && !has_child(c, e)) {
            arbor_status s = emit_element(c, 7u, e); if (s.native != 0) return s;
        }
        if (e->parent != NULL && arbor_view0_native_v1n3_ascii_case_equal(e->parent->name, "canvas") &&
            (arbor_view0_native_v1n3_ascii_case_equal(e->name, "a") || arbor_view0_native_v1n3_ascii_case_equal(e->name, "button") ||
             arbor_view0_native_v1n3_ascii_case_equal(e->name, "input")) &&
            arbor_view0_native_v1n3_attr_find(e, "aria-label") == NULL && arbor_view0_native_v1n3_attr_find(e, "title") == NULL) {
            arbor_status s = emit_element(c, 8u, e); if (s.native != 0) return s;
        }
    }
    return arbor_status_from_native(0);
}

arbor_status arbor_view0_native_v1n3_g12_measure(arbor_span i, const arbor_view0_native_v1n3_options *o, void *a, arbor_view0_native_v1n3_g12_evaluation *e)
{ return arbor_view0_native_v1n3_run_group(12u, i, o, a, NULL, 0u, e); }
arbor_status arbor_view0_native_v1n3_g12_collect_anchors(arbor_span i, const arbor_view0_native_v1n3_options *o, void *a, arbor_view0_native_v1n3_g12_anchor *n, uint64_t c, arbor_view0_native_v1n3_g12_evaluation *e)
{ return arbor_view0_native_v1n3_run_group(12u, i, o, a, n, c, e); }
void arbor_view0_native_v1n3_g12_materialize_anchor(const arbor_view0_native_v1n3_g12_anchor *a, uint64_t s, arbor_view0_native_diagnostic *d)
{ arbor_view0_native_v1n3_materialize(a, s, d); }
