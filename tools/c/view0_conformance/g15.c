#include "g15.h"

#include "ecma_unicode.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

static arbor_status ea(arbor_view0_native_v1n3_context *c, uint16_t r, const arbor_view0_native_v1n3_attr *a)
{ return arbor_view0_native_v1n3_emit(c, r, a->name_offset, a->source_length, 0u); }
static arbor_status ee(arbor_view0_native_v1n3_context *c, uint16_t r, const arbor_view0_native_v1n3_element *e)
{ return arbor_view0_native_v1n3_emit(c, r, e->source_offset, e->source_length, 0u); }

static bool interactive(const arbor_view0_native_v1n3_element *e)
{
    return arbor_view0_native_v1n3_ascii_case_equal(e->name, "button") || arbor_view0_native_v1n3_ascii_case_equal(e->name, "input") ||
        arbor_view0_native_v1n3_ascii_case_equal(e->name, "select") || arbor_view0_native_v1n3_ascii_case_equal(e->name, "textarea") ||
        (arbor_view0_native_v1n3_ascii_case_equal(e->name, "a") && arbor_view0_native_v1n3_attr_find(e, "href") != NULL);
}

static bool integer_value(arbor_span value, int64_t *out)
{
    if (value.length == 0u || value.length > 31u) return false;
    char buffer[32]; memcpy(buffer, value.data, (size_t)value.length); buffer[value.length] = '\0';
    char *end = NULL; errno = 0; const long long parsed = strtoll(buffer, &end, 10);
    if (errno != 0 || end == buffer || *end != '\0') return false;
    *out = (int64_t)parsed;
    return true;
}

arbor_status arbor_view0_native_v1n3_g15_validate_element(
    arbor_view0_native_v1n3_context *c, const arbor_view0_native_v1n3_element *e)
{
    if (c == NULL || e == NULL) return arbor_status_from_native(-(int64_t)EINVAL);
    if (e->template_opaque) return arbor_status_from_native(0);
    for (const arbor_view0_native_v1n3_element *p = e->parent; p != NULL; p = p->parent) {
        if (arbor_view0_native_v1n3_attr_find(p, "inert") != NULL &&
            (interactive(e) || arbor_view0_native_v1n3_attr_find(e, "tabindex") != NULL)) {
            arbor_status s = ee(c, 2u, e); if (s.native != 0) return s; break;
        }
    }
    const arbor_view0_native_v1n3_attr *tab = arbor_view0_native_v1n3_attr_find(e, "tabindex");
    if (tab != NULL) {
        int64_t value = 0; if (integer_value(tab->value, &value) && value > 0) { arbor_status s = ea(c, 3u, tab); if (s.native != 0) return s; }
    }
    const arbor_view0_native_v1n3_attr *access = arbor_view0_native_v1n3_attr_find(e, "accesskey");
    if (access != NULL) {
        uint64_t i = 0u; while (i < access->value.length) {
            while (i < access->value.length && access->value.data[i] == (uint8_t)' ') i += 1u;
            uint32_t scalar = 0u; uint64_t width = 0u; const arbor_span tail = {access->value.data + i, access->value.length - i};
            if (i < access->value.length && (!arbor_view0_native_v1n3_utf8_scalar(tail, 0u, &scalar, &width) ||
                (i + width < access->value.length && access->value.data[i + width] != (uint8_t)' '))) {
                (void)scalar; arbor_status s = ea(c, 4u, access); if (s.native != 0) return s; break;
            }
            i += width;
        }
    }
    const arbor_view0_native_v1n3_attr *inputmode = arbor_view0_native_v1n3_attr_find(e, "inputmode");
    const arbor_view0_native_v1n3_attr *editable = arbor_view0_native_v1n3_attr_find(e, "contenteditable");
    if (inputmode != NULL && editable == NULL && !arbor_view0_native_v1n3_ascii_case_equal(e->name, "input") &&
        !arbor_view0_native_v1n3_ascii_case_equal(e->name, "textarea")) { arbor_status s = ea(c, 5u, inputmode); if (s.native != 0) return s; }
    const arbor_view0_native_v1n3_attr *drag = arbor_view0_native_v1n3_attr_find(e, "draggable");
    if (drag != NULL && arbor_view0_native_v1n3_ascii_case_equal(drag->value, "true")) {
        const arbor_view0_native_v1n3_attr *title = arbor_view0_native_v1n3_attr_find(e, "title");
        if (title == NULL || title->value.length == 0u) { arbor_status s = ea(c, 6u, drag); if (s.native != 0) return s; }
    }
    const arbor_view0_native_v1n3_attr *popover = arbor_view0_native_v1n3_attr_find(e, "popover");
    if (popover != NULL && popover->value.length != 0u && !arbor_view0_native_v1n3_ascii_case_equal(popover->value, "auto") &&
        !arbor_view0_native_v1n3_ascii_case_equal(popover->value, "manual") && !arbor_view0_native_v1n3_ascii_case_equal(popover->value, "hint")) {
        arbor_status s = ea(c, 7u, popover); if (s.native != 0) return s;
    }
    return arbor_status_from_native(0);
}

static bool hidden(const arbor_view0_native_v1n3_element *e)
{ return e != NULL && arbor_view0_native_v1n3_attr_find(e, "hidden") != NULL; }

arbor_status arbor_view0_native_v1n3_g15_finalize(arbor_view0_native_v1n3_context *c)
{
    for (const arbor_view0_native_v1n3_element *e = c->elements; e != NULL; e = e->next) {
        if (e->template_opaque) continue;
        const arbor_view0_native_v1n3_attr *reference = NULL;
        if (arbor_view0_native_v1n3_ascii_case_equal(e->name, "a")) reference = arbor_view0_native_v1n3_attr_find(e, "href");
        else if (arbor_view0_native_v1n3_ascii_case_equal(e->name, "label")) reference = arbor_view0_native_v1n3_attr_find(e, "for");
        else if (arbor_view0_native_v1n3_ascii_case_equal(e->name, "output")) reference = arbor_view0_native_v1n3_attr_find(e, "for");
        if (reference != NULL && reference->value.length > 1u && reference->value.data[0] == (uint8_t)'#') {
            const arbor_span id = {reference->value.data + 1u, reference->value.length - 1u};
            if (hidden(arbor_view0_native_v1n3_id_find(c, id))) { arbor_status s = ea(c, 1u, reference); if (s.native != 0) return s; }
        }
        const arbor_view0_native_v1n3_attr *access = arbor_view0_native_v1n3_attr_find(e, "accesskey");
        if (access != NULL) for (const arbor_view0_native_v1n3_element *o = c->elements; o != e; o = o->next) {
            const arbor_view0_native_v1n3_attr *other = arbor_view0_native_v1n3_attr_find(o, "accesskey");
            if (other != NULL && other->value.length == access->value.length && memcmp(other->value.data, access->value.data, (size_t)access->value.length) == 0) {
                arbor_status s = ea(c, 4u, access); if (s.native != 0) return s; break;
            }
        }
        const arbor_view0_native_v1n3_attr *target = arbor_view0_native_v1n3_attr_find(e, "popovertarget");
        const arbor_view0_native_v1n3_attr *action = arbor_view0_native_v1n3_attr_find(e, "popovertargetaction");
        if (target != NULL) {
            const arbor_view0_native_v1n3_element *t = arbor_view0_native_v1n3_id_find(c, target->value);
            if (t == NULL || arbor_view0_native_v1n3_attr_find(t, "popover") == NULL ||
                (action != NULL && !arbor_view0_native_v1n3_ascii_case_equal(action->value, "toggle") &&
                 !arbor_view0_native_v1n3_ascii_case_equal(action->value, "show") &&
                 !arbor_view0_native_v1n3_ascii_case_equal(action->value, "hide"))) {
                arbor_status s = ea(c, 8u, target); if (s.native != 0) return s;
            }
        } else if (action != NULL) { arbor_status s = ea(c, 8u, action); if (s.native != 0) return s; }
    }
    return arbor_status_from_native(0);
}

arbor_status arbor_view0_native_v1n3_g15_measure(arbor_span i, const arbor_view0_native_v1n3_options *o, void *a, arbor_view0_native_v1n3_g15_evaluation *e)
{ return arbor_view0_native_v1n3_run_group(15u, i, o, a, NULL, 0u, e); }
arbor_status arbor_view0_native_v1n3_g15_collect_anchors(arbor_span i, const arbor_view0_native_v1n3_options *o, void *a, arbor_view0_native_v1n3_g15_anchor *n, uint64_t c, arbor_view0_native_v1n3_g15_evaluation *e)
{ return arbor_view0_native_v1n3_run_group(15u, i, o, a, n, c, e); }
void arbor_view0_native_v1n3_g15_materialize_anchor(const arbor_view0_native_v1n3_g15_anchor *a, uint64_t s, arbor_view0_native_diagnostic *d)
{ arbor_view0_native_v1n3_materialize(a, s, d); }
