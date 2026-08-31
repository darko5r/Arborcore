#include "g13.h"

#include "ecma_frontend.h"

#include <errno.h>
#include <string.h>

static bool span_equal(arbor_span a, arbor_span b)
{ return a.length == b.length && (a.length == 0u || memcmp(a.data, b.data, (size_t)a.length) == 0); }

static bool valid_name(arbor_span name)
{
    static const char *const reserved[] = {"annotation-xml","color-profile","font-face","font-face-src","font-face-uri","font-face-format","font-face-name","missing-glyph"};
    bool hyphen = false;
    if (name.length < 2u || name.data[0] < (uint8_t)'a' || name.data[0] > (uint8_t)'z') return false;
    for (uint64_t i = 0u; i < name.length; ++i) {
        const uint8_t b = name.data[i]; if (b == (uint8_t)'-') hyphen = true;
        if (!((b >= (uint8_t)'a' && b <= (uint8_t)'z') || (b >= (uint8_t)'0' && b <= (uint8_t)'9') ||
              b == (uint8_t)'-' || b == (uint8_t)'_' || b == (uint8_t)'.' || b >= UINT8_C(0x80))) return false;
    }
    if (!hyphen) return false;
    for (uint64_t i = 0u; i < sizeof(reserved) / sizeof(reserved[0]); ++i)
        if (arbor_view0_native_v1n3_ascii_equal(name, reserved[i])) return false;
    return true;
}

static const arbor_view0_native_v1n3_definition *definition_for(
    const arbor_view0_native_v1n3_options *options, arbor_span name, uint64_t *index_out)
{
    if (options == NULL) return NULL;
    for (uint64_t i = 0u; i < options->definition_count; ++i) if (span_equal(options->definitions[i].name, name)) {
        if (index_out != NULL) *index_out = i;
        return options->definitions + i;
    }
    return NULL;
}

static arbor_status emit_element(arbor_view0_native_v1n3_context *c, uint16_t r, const arbor_view0_native_v1n3_element *e)
{ return arbor_view0_native_v1n3_emit(c, r, e->source_offset, e->source_length, 0u); }
static arbor_status emit_attr(arbor_view0_native_v1n3_context *c, uint16_t r, const arbor_view0_native_v1n3_attr *a)
{ return arbor_view0_native_v1n3_emit(c, r, a->name_offset, a->source_length, 0u); }

arbor_status arbor_view0_native_v1n3_g13_validate_element(
    arbor_view0_native_v1n3_context *c, const arbor_view0_native_v1n3_element *e)
{
    if (c == NULL || e == NULL) return arbor_status_from_native(-(int64_t)EINVAL);
    if (e->template_opaque) return arbor_status_from_native(0);
    const bool custom_name = memchr(e->name.data, '-', (size_t)e->name.length) != NULL;
    const arbor_view0_native_v1n3_attr *is = arbor_view0_native_v1n3_attr_find(e, "is");
    if (custom_name && !valid_name(e->name)) { arbor_status s = emit_element(c, 2u, e); if (s.native != 0) return s; }
    uint64_t def_index = 0u;
    const arbor_view0_native_v1n3_definition *def = custom_name ? definition_for(c->options, e->name, &def_index) : NULL;
    if (custom_name && valid_name(e->name) && def == NULL) { arbor_status s = emit_element(c, 3u, e); if (s.native != 0) return s; }
    if (is != NULL) {
        const arbor_view0_native_v1n3_definition *is_def = definition_for(c->options, is->value, &def_index);
        if (is_def == NULL || (is_def->flags & ARBOR_VIEW0_NATIVE_V1N3_DEFINITION_CUSTOMIZED_BUILTIN) == 0u ||
            !span_equal(is_def->local_name, e->name)) { arbor_status s = emit_attr(c, 4u, is); if (s.native != 0) return s; }
    }
    if (custom_name) {
        if (is != NULL) { arbor_status s = emit_attr(c, 5u, is); if (s.native != 0) return s; }
        for (const arbor_view0_native_v1n3_attr *a = e->attributes; a != NULL; a = a->next) {
            bool upper = false; for (uint64_t i = 0u; i < a->name.length; ++i) if (a->name.data[i] >= (uint8_t)'A' && a->name.data[i] <= (uint8_t)'Z') upper = true;
            if (upper) { arbor_status s = emit_attr(c, 5u, a); if (s.native != 0) return s; }
        }
        const bool form_attr = arbor_view0_native_v1n3_attr_find(e, "disabled") != NULL || arbor_view0_native_v1n3_attr_find(e, "form") != NULL ||
            arbor_view0_native_v1n3_attr_find(e, "name") != NULL || arbor_view0_native_v1n3_attr_find(e, "readonly") != NULL;
        if (form_attr && (def == NULL || (def->flags & ARBOR_VIEW0_NATIVE_V1N3_DEFINITION_FORM_ASSOCIATED) == 0u)) {
            arbor_status s = emit_element(c, 6u, e); if (s.native != 0) return s;
        }
    }
    return arbor_status_from_native(0);
}

arbor_status arbor_view0_native_v1n3_g13_finalize(arbor_view0_native_v1n3_context *c)
{
    for (uint64_t i = 0u; i < c->options->definition_count; ++i) {
        const arbor_view0_native_v1n3_definition *d = c->options->definitions + i;
        if ((d->flags & (ARBOR_VIEW0_NATIVE_V1N3_DEFINITION_AUTONOMOUS | ARBOR_VIEW0_NATIVE_V1N3_DEFINITION_CUSTOMIZED_BUILTIN)) == 0u) continue;
        arbor_view0_native_v1n3_ecma_result parsed = {0};
        arbor_status s = arbor_view0_native_v1n3_ecma_parse(ARBOR_VIEW0_NATIVE_V1N3_ECMA_CONSTRUCTOR_SUBSET, d->constructor_source, c->arena, &parsed);
        if (s.native != 0) return s;
        c->evaluation.frontend_parse_count += 1u;
        if (parsed.accepted == 0u) {
            s = arbor_view0_native_v1n3_emit(c, 1u, parsed.error_offset,
                parsed.error_offset < d->constructor_source.length ? 1u : 0u, (uint32_t)(i + 1u));
            if (s.native != 0) return s;
        }
    }
    return arbor_status_from_native(0);
}

arbor_status arbor_view0_native_v1n3_g13_measure(arbor_span i, const arbor_view0_native_v1n3_options *o, void *a, arbor_view0_native_v1n3_g13_evaluation *e)
{ return arbor_view0_native_v1n3_run_group(13u, i, o, a, NULL, 0u, e); }
arbor_status arbor_view0_native_v1n3_g13_collect_anchors(arbor_span i, const arbor_view0_native_v1n3_options *o, void *a, arbor_view0_native_v1n3_g13_anchor *n, uint64_t c, arbor_view0_native_v1n3_g13_evaluation *e)
{ return arbor_view0_native_v1n3_run_group(13u, i, o, a, n, c, e); }
void arbor_view0_native_v1n3_g13_materialize_anchor(const arbor_view0_native_v1n3_g13_anchor *a, uint64_t s, arbor_view0_native_diagnostic *d)
{ arbor_view0_native_v1n3_materialize(a, s, d); }
