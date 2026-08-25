#include <arborcore/view.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static arbor_span span_from_cstr(const char *text)
{
    return (arbor_span){(const uint8_t *)text, (uint64_t)strlen(text)};
}

static int require_status_ok(arbor_status status, const char *where)
{
    if (status.native != 0) {
        fprintf(stderr, "%s: native=%lld code=%d\n",
                where, (long long)status.native, (int)status.code);
        return 1;
    }
    return 0;
}

static int basic_prepared_template_test(void)
{
    char source[] =
        "<!doctype html><html><head><title>Arborcore</title>"
        "<link rel=\"stylesheet\" href=\"/app.css\">"
        "<script src=\"/arborcore_host.js\"></script></head>"
        "<body><h1>{{ name }}</h1><p>{{message}}</p><p>{{name}}</p></body></html>";
    const arbor_span fields[] = {
        span_from_cstr("name"),
        span_from_cstr("message")
    };

    arbor_view_html_template_requirements requirements = {0};
    arbor_status status = arbor_view_html_template_measure(
        span_from_cstr(source),
        fields,
        2u,
        &requirements);
    if (require_status_ok(status, "measure") != 0 ||
        requirements.part_count != 7u ||
        requirements.literal_bytes == 0u) {
        return 1;
    }

    arbor_view_html_template_part parts[7] = {0};
    uint8_t literal_bytes[512] = {0};
    arbor_view_html_template_storage storage = {
        parts, 7u, literal_bytes, sizeof(literal_bytes)
    };
    arbor_view_html_template template_view = {0};
    status = arbor_view_html_template_prepare(
        span_from_cstr(source),
        fields,
        2u,
        &storage,
        &template_view);
    if (require_status_ok(status, "prepare") != 0 ||
        template_view.part_count != requirements.part_count ||
        template_view.literal_length != requirements.literal_bytes ||
        template_view.value_count != 2u) {
        return 2;
    }

    /*
     * T1 preparation must own all trusted literal bytes and slot indices in the
     * caller's persistent storage; source and field metadata may disappear.
     */
    memset(source, 'X', sizeof(source) - 1u);

    const arbor_span values[] = {
        span_from_cstr("<Darko & \"Friends\">"),
        span_from_cstr("5 > 3 & 2 < 4")
    };
    uint8_t arena_bytes[1024] = {0};
    arbor_asm_arena arena = {0};
    if (arena_init(&arena, arena_bytes, sizeof(arena_bytes)).status != 0) {
        return 3;
    }

    arbor_span body = {0};
    status = arbor_view_html_template_render(
        &template_view,
        values,
        2u,
        &arena,
        &body);
    if (require_status_ok(status, "render") != 0) {
        return 4;
    }

    const char *expected =
        "<!doctype html><html><head><title>Arborcore</title>"
        "<link rel=\"stylesheet\" href=\"/app.css\">"
        "<script src=\"/arborcore_host.js\"></script></head>"
        "<body><h1>&lt;Darko &amp; \"Friends\"&gt;</h1>"
        "<p>5 &gt; 3 &amp; 2 &lt; 4</p>"
        "<p>&lt;Darko &amp; \"Friends\"&gt;</p></body></html>";
    if (body.length != (uint64_t)strlen(expected) ||
        memcmp(body.data, expected, (size_t)body.length) != 0) {
        return 5;
    }

    const uint8_t first_byte = body.data[0];
    const arbor_span second_values[] = {
        span_from_cstr("Second"),
        span_from_cstr("Render")
    };
    arbor_span second_body = {0};
    status = arbor_view_html_template_render(
        &template_view,
        second_values,
        2u,
        &arena,
        &second_body);
    if (require_status_ok(status, "second render") != 0 ||
        body.data[0] != first_byte ||
        second_body.data <= body.data) {
        return 6;
    }

    return 0;
}

static int static_and_empty_template_test(void)
{
    const char static_source[] = "<p>static</p>";
    arbor_view_html_template_requirements static_requirements = {0};
    arbor_status status = arbor_view_html_template_measure(
        span_from_cstr(static_source),
        NULL,
        0u,
        &static_requirements);
    if (require_status_ok(status, "static measure") != 0 ||
        static_requirements.part_count != 1u ||
        static_requirements.literal_bytes != (uint64_t)strlen(static_source)) {
        return 1;
    }

    arbor_view_html_template_part static_parts[1] = {0};
    uint8_t static_bytes[32] = {0};
    arbor_view_html_template_storage static_storage = {
        static_parts, 1u, static_bytes, sizeof(static_bytes)
    };
    arbor_view_html_template static_template = {0};
    status = arbor_view_html_template_prepare(
        span_from_cstr(static_source),
        NULL,
        0u,
        &static_storage,
        &static_template);
    if (require_status_ok(status, "static prepare") != 0) {
        return 2;
    }

    uint8_t arena_bytes[64] = {0};
    arbor_asm_arena arena = {0};
    if (arena_init(&arena, arena_bytes, sizeof(arena_bytes)).status != 0) {
        return 3;
    }
    arbor_span body = {0};
    status = arbor_view_html_template_render(
        &static_template,
        NULL,
        0u,
        &arena,
        &body);
    if (require_status_ok(status, "static render") != 0 ||
        body.length != (uint64_t)strlen(static_source) ||
        memcmp(body.data, static_source, (size_t)body.length) != 0) {
        return 4;
    }

    arbor_view_html_template_requirements empty_requirements = {99u, 99u};
    status = arbor_view_html_template_measure(
        (arbor_span){NULL, 0u},
        NULL,
        0u,
        &empty_requirements);
    if (require_status_ok(status, "empty measure") != 0 ||
        empty_requirements.part_count != 0u ||
        empty_requirements.literal_bytes != 0u) {
        return 5;
    }

    arbor_view_html_template_storage empty_storage = {0};
    arbor_view_html_template empty_template = {0};
    status = arbor_view_html_template_prepare(
        (arbor_span){NULL, 0u},
        NULL,
        0u,
        &empty_storage,
        &empty_template);
    if (require_status_ok(status, "empty prepare") != 0) {
        return 6;
    }

    arbor_span empty_body = {(const uint8_t *)"sentinel", 8u};
    const uint64_t before = arena.offset;
    status = arbor_view_html_template_render(
        &empty_template,
        NULL,
        0u,
        &arena,
        &empty_body);
    if (require_status_ok(status, "empty render") != 0 ||
        empty_body.length != 0u || arena.offset != before) {
        return 7;
    }

    return 0;
}

static int same_arena_prior_value_test(void)
{
    const char source[] = "<p>{{value}}</p>";
    const arbor_span fields[] = {span_from_cstr("value")};
    arbor_view_html_template_part parts[3] = {0};
    uint8_t literal_bytes[32] = {0};
    arbor_view_html_template_storage storage = {
        parts, 3u, literal_bytes, sizeof(literal_bytes)
    };
    arbor_view_html_template template_view = {0};
    arbor_status status = arbor_view_html_template_prepare(
        span_from_cstr(source), fields, 1u, &storage, &template_view);
    if (require_status_ok(status, "same-arena prepare") != 0) {
        return 1;
    }

    uint8_t arena_bytes[128] = {0};
    arbor_asm_arena arena = {0};
    if (arena_init(&arena, arena_bytes, sizeof(arena_bytes)).status != 0) {
        return 2;
    }
    const char value_text[] = "arena & value";
    arbor_asm_result_ptr allocation = arena_alloc(
        &arena,
        (uint64_t)strlen(value_text));
    if (allocation.status != 0) {
        return 3;
    }
    (void)memory_copy(
        allocation.value,
        value_text,
        (uint64_t)strlen(value_text));
    const arbor_span values[] = {
        {(const uint8_t *)allocation.value, (uint64_t)strlen(value_text)}
    };

    arbor_span body = {0};
    status = arbor_view_html_template_render(
        &template_view, values, 1u, &arena, &body);
    const char expected[] = "<p>arena &amp; value</p>";
    if (require_status_ok(status, "same-arena render") != 0 ||
        body.length != (uint64_t)strlen(expected) ||
        memcmp(body.data, expected, sizeof(expected) - 1u) != 0 ||
        memcmp(allocation.value, value_text, sizeof(value_text) - 1u) != 0) {
        return 4;
    }
    return 0;
}

int main(void)
{
    if (basic_prepared_template_test() != 0) {
        return 1;
    }
    if (static_and_empty_template_test() != 0) {
        return 2;
    }
    if (same_arena_prior_value_test() != 0) {
        return 3;
    }

    puts("PASS: VIEW0 T1 prepared HTML templates, source-independent persistent literals, resolved slots and C1/C2 rendering");
    return 0;
}
