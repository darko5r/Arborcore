#include <arborcore/view.h>

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static arbor_span span_from_cstr(const char *text)
{
    return (arbor_span){(const uint8_t *)text, (uint64_t)strlen(text)};
}

static int expect_native(arbor_status status, int64_t expected)
{
    return status.native == expected ? 0 : 1;
}

static int prepare_small(
    const char *source,
    const arbor_span *fields,
    uint64_t field_count,
    arbor_view_html_template_part *parts,
    uint64_t part_capacity,
    uint8_t *bytes,
    uint64_t byte_capacity,
    arbor_view_html_template *out)
{
    arbor_view_html_template_storage storage = {
        parts, part_capacity, bytes, byte_capacity
    };
    return arbor_view_html_template_prepare(
        span_from_cstr(source),
        fields,
        field_count,
        &storage,
        out).native == 0 ? 0 : 1;
}

static int grammar_and_binding_rejection_test(void)
{
    const arbor_span one_field[] = {span_from_cstr("name")};
    arbor_view_html_template_requirements sentinel = {77u, 88u};

    const arbor_span duplicate_fields[] = {
        span_from_cstr("name"), span_from_cstr("name")
    };
    arbor_status status = arbor_view_html_template_measure(
        span_from_cstr("<p>{{name}}</p>"),
        duplicate_fields,
        2u,
        &sentinel);
    if (expect_native(status, -EEXIST) != 0 ||
        sentinel.part_count != 77u || sentinel.literal_bytes != 88u) {
        return 1;
    }

    const arbor_span invalid_fields[] = {span_from_cstr("bad-name")};
    status = arbor_view_html_template_measure(
        span_from_cstr("<p>static</p>"),
        invalid_fields,
        1u,
        &sentinel);
    if (expect_native(status, -EINVAL) != 0) {
        return 2;
    }

    status = arbor_view_html_template_measure(
        span_from_cstr("<p>{{missing}}</p>"),
        one_field,
        1u,
        &sentinel);
    if (expect_native(status, -ENOENT) != 0) {
        return 3;
    }

    const char *invalid_sources[] = {
        "<p>{{}}</p>",
        "<p>{{ name</p>",
        "<p>{{ na\nme }}</p>",
        "<p title=\"{{name}}\">x</p>",
        "<!-- {{name}} -->",
        "<title>{{name}}</title>",
        "<textarea>{{name}}</textarea>",
        "<style>{{name}}</style>",
        "<script>{{name}}</script>",
        "&{{name}};",
        "&amp{{name}};",
        "<svg><text>static</text></svg>",
        "<math><mi>x</mi></math>"
    };
    for (size_t i = 0u; i < sizeof(invalid_sources) / sizeof(invalid_sources[0]); ++i) {
        status = arbor_view_html_template_measure(
            span_from_cstr(invalid_sources[i]),
            one_field,
            1u,
            &sentinel);
        if (expect_native(status, -EINVAL) != 0 ||
            sentinel.part_count != 77u || sentinel.literal_bytes != 88u) {
            return 4;
        }
    }

    status = arbor_view_html_template_measure(
        span_from_cstr("&amp;{{name}}"),
        one_field,
        1u,
        &sentinel);
    if (status.native != 0) {
        return 5;
    }

    /*
     * Custom-element names that merely begin with a blocked/raw or foreign
     * built-in name remain ordinary HTML tags; matching is on the full name.
     */
    status = arbor_view_html_template_measure(
        span_from_cstr("<script-widget>{{name}}</script-widget><svg-icon>{{name}}</svg-icon>"),
        one_field,
        1u,
        &sentinel);
    if (status.native != 0) {
        return 6;
    }

    return 0;
}

static int preparation_atomicity_alias_test(void)
{
    char source[] = "<p>{{name}}</p>";
    const arbor_span fields[] = {span_from_cstr("name")};
    arbor_view_html_template_requirements requirements = {0};
    arbor_status status = arbor_view_html_template_measure(
        span_from_cstr(source), fields, 1u, &requirements);
    if (status.native != 0 || requirements.part_count != 3u) {
        return 1;
    }

    arbor_view_html_template_part parts[3];
    memset(parts, 0xA5, sizeof(parts));
    uint8_t bytes[32];
    memset(bytes, 0x5A, sizeof(bytes));
    arbor_view_html_template out;
    memset(&out, 0x3C, sizeof(out));
    const arbor_view_html_template before_out = out;
    const arbor_view_html_template_part before_part = parts[0];
    const uint8_t before_byte = bytes[0];

    arbor_view_html_template_storage too_few_parts = {
        parts, 2u, bytes, sizeof(bytes)
    };
    status = arbor_view_html_template_prepare(
        span_from_cstr(source),
        fields,
        1u,
        &too_few_parts,
        &out);
    if (expect_native(status, -ENOSPC) != 0 ||
        memcmp(&out, &before_out, sizeof(out)) != 0 ||
        memcmp(&parts[0], &before_part, sizeof(before_part)) != 0 ||
        bytes[0] != before_byte) {
        return 2;
    }

    arbor_view_html_template_storage too_few_bytes = {
        parts, 3u, bytes, requirements.literal_bytes - 1u
    };
    status = arbor_view_html_template_prepare(
        span_from_cstr(source),
        fields,
        1u,
        &too_few_bytes,
        &out);
    if (expect_native(status, -ENOSPC) != 0 ||
        memcmp(&out, &before_out, sizeof(out)) != 0 ||
        memcmp(&parts[0], &before_part, sizeof(before_part)) != 0 ||
        bytes[0] != before_byte) {
        return 3;
    }

    arbor_view_html_template_storage source_alias = {
        parts,
        3u,
        (uint8_t *)source,
        sizeof(source)
    };
    const char source_before[sizeof(source)] = "<p>{{name}}</p>";
    status = arbor_view_html_template_prepare(
        span_from_cstr(source),
        fields,
        1u,
        &source_alias,
        &out);
    if (expect_native(status, -EINVAL) != 0 ||
        memcmp(source, source_before, sizeof(source)) != 0) {
        return 4;
    }

    uint8_t alias_measure_buffer[64] = {0};
    memcpy(alias_measure_buffer, "<p>static</p>", 13u);
    arbor_view_html_template_requirements *alias_requirements =
        (arbor_view_html_template_requirements *)(void *)alias_measure_buffer;
    status = arbor_view_html_template_measure(
        (arbor_span){alias_measure_buffer, 13u},
        NULL,
        0u,
        alias_requirements);
    if (expect_native(status, -EINVAL) != 0) {
        return 5;
    }

    return 0;
}

static int render_rejection_and_precedence_test(void)
{
    const char source[] = "<p>{{name}}</p>";
    const arbor_span fields[] = {span_from_cstr("name")};
    arbor_view_html_template_part parts[3] = {0};
    uint8_t literal_bytes[32] = {0};
    arbor_view_html_template template_view = {0};
    if (prepare_small(
            source, fields, 1u, parts, 3u, literal_bytes,
            sizeof(literal_bytes), &template_view) != 0) {
        return 1;
    }

    uint8_t arena_bytes[128] = {0};
    arbor_asm_arena arena = {0};
    if (arena_init(&arena, arena_bytes, sizeof(arena_bytes)).status != 0) {
        return 2;
    }
    const arbor_span values[] = {span_from_cstr("name")};
    arbor_span body = {(const uint8_t *)"sentinel", 8u};
    const arbor_span body_before = body;

    arbor_status status = arbor_view_html_template_render(
        &template_view, values, 0u, &arena, &body);
    if (expect_native(status, -EINVAL) != 0 ||
        arena.offset != 0u || memcmp(&body, &body_before, sizeof(body)) != 0) {
        return 3;
    }

    arbor_view_html_template corrupt = template_view;
    corrupt.part_count_guard ^= 1u;
    status = arbor_view_html_template_render(
        &corrupt, values, 1u, &arena, &body);
    if (expect_native(status, -EINVAL) != 0 || arena.offset != 0u) {
        return 4;
    }

    corrupt = template_view;
    ((arbor_view_html_template_part *)corrupt.parts)[1].kind = UINT64_C(99);
    status = arbor_view_html_template_render(
        &corrupt, values, 1u, &arena, &body);
    ((arbor_view_html_template_part *)corrupt.parts)[1].kind = UINT64_C(2);
    if (expect_native(status, -EINVAL) != 0 || arena.offset != 0u) {
        return 5;
    }

    /*
     * Put a value exactly at the future body frontier. A fitting render must
     * reject before any trusted literal overwrites that borrowed value.
     */
    arena = (arbor_asm_arena){0};
    if (arena_init(&arena, arena_bytes, sizeof(arena_bytes)).status != 0) {
        return 6;
    }
    memcpy(arena_bytes, "X", 1u);
    const arbor_span future_alias_values[] = {
        {arena_bytes, 1u}
    };
    status = arbor_view_html_template_render(
        &template_view,
        future_alias_values,
        1u,
        &arena,
        &body);
    if (expect_native(status, -EINVAL) != 0 ||
        arena.offset != 0u || arena_bytes[0] != (uint8_t)'X') {
        return 7;
    }

    /*
     * When the body cannot fit at all, C1 capacity remains authoritative; do
     * not invent a speculative future-body alias failure.
     */
    uint8_t tiny_bytes[2] = {'Y', 0};
    arbor_asm_arena tiny = {0};
    if (arena_init(&tiny, tiny_bytes, sizeof(tiny_bytes)).status != 0) {
        return 8;
    }
    const arbor_span tiny_values[] = {{tiny_bytes, 1u}};
    status = arbor_view_html_template_render(
        &template_view,
        tiny_values,
        1u,
        &tiny,
        &body);
    if (expect_native(status, -ENOSPC) != 0 ||
        tiny.offset != 0u || tiny_bytes[0] != (uint8_t)'Y') {
        return 9;
    }

    uint8_t result_value[32] = "result-alias";
    const arbor_span alias_values[] = {
        {result_value, 12u}
    };
    status = arbor_view_html_template_render(
        &template_view,
        alias_values,
        1u,
        &arena,
        (arbor_span *)(void *)result_value);
    if (expect_native(status, -EINVAL) != 0) {
        return 10;
    }

    return 0;
}

static int prepared_storage_future_body_alias_test(void)
{
    const char source[] = "<p>{{name}}</p>";
    const arbor_span fields[] = {span_from_cstr("name")};

    uint8_t arena_bytes[256] = {0};
    arbor_asm_arena arena = {0};
    if (arena_init(&arena, arena_bytes, sizeof(arena_bytes)).status != 0) {
        return 1;
    }

    arbor_view_html_template_part *parts =
        (arbor_view_html_template_part *)(void *)arena_bytes;
    uint8_t *literals = arena_bytes + 96u;
    arbor_view_html_template_storage storage = {
        parts, 3u, literals, 32u
    };
    arbor_view_html_template template_view = {0};
    arbor_status status = arbor_view_html_template_prepare(
        span_from_cstr(source),
        fields,
        1u,
        &storage,
        &template_view);
    if (status.native != 0) {
        return 2;
    }

    /*
     * The request arena still has offset zero, so rendering here would reserve
     * over the prepared template backing itself. T1 must reject it.
     */
    const arbor_span values[] = {span_from_cstr("x")};
    arbor_span body = {0};
    status = arbor_view_html_template_render(
        &template_view,
        values,
        1u,
        &arena,
        &body);
    if (expect_native(status, -EINVAL) != 0 || arena.offset != 0u) {
        return 3;
    }

    return 0;
}

int main(void)
{
    if (grammar_and_binding_rejection_test() != 0) {
        return 1;
    }
    if (preparation_atomicity_alias_test() != 0) {
        return 2;
    }
    if (render_rejection_and_precedence_test() != 0) {
        return 3;
    }
    if (prepared_storage_future_body_alias_test() != 0) {
        return 4;
    }

    puts("PASS: VIEW0 T1 grammar/context rejection, preparation atomicity, corruption/alias safety and C1 capacity precedence");
    return 0;
}
