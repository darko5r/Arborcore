#include <arborcore/view.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int span_equals(arbor_span span, const char *text)
{
    const size_t length = strlen(text);
    return span.length == (uint64_t)length &&
           (length == 0u || memcmp(span.data, text, length) == 0);
}

int main(void)
{
    static const char dynamic_text[] = "A&B <tag> \"quoted\" 'single'";
    static const char expected[] = "<p>A&amp;B &lt;tag&gt; \"quoted\" 'single'</p>";

    arbor_view_measure measure = {0};
    if (arbor_view_measure_add(&measure, 3u).native != 0 ||
        arbor_view_html_text_measure(
            &measure,
            (arbor_span){(const uint8_t *)dynamic_text, sizeof(dynamic_text) - 1u}).native != 0 ||
        arbor_view_measure_add(&measure, 4u).native != 0 ||
        measure.length != (uint64_t)(sizeof(expected) - 1u)) {
        return 1;
    }

    uint8_t arena_bytes[256] = {0};
    arbor_asm_arena arena = {0};
    if (arena_init(&arena, arena_bytes, sizeof(arena_bytes)).status != 0) {
        return 2;
    }

    arbor_view_output output = {0};
    if (arbor_view_output_begin(&arena, measure.length, &output).native != 0 ||
        arbor_view_output_append(
            &output, (arbor_span){(const uint8_t *)"<p>", 3u}).native != 0 ||
        arbor_view_html_text_append(
            &output,
            (arbor_span){(const uint8_t *)dynamic_text, sizeof(dynamic_text) - 1u}).native != 0 ||
        arbor_view_output_append(
            &output, (arbor_span){(const uint8_t *)"</p>", 4u}).native != 0) {
        return 3;
    }

    arbor_span body = {0};
    if (arbor_view_output_commit(&output, &body).native != 0 ||
        !span_equals(body, expected)) {
        return 4;
    }

    static const char entity_like[] = "&copy; &amp; <script>x</script> > \" '";
    static const char entity_expected[] =
        "&amp;copy; &amp;amp; &lt;script&gt;x&lt;/script&gt; &gt; \" '";
    measure = (arbor_view_measure){0};
    if (arbor_view_html_text_measure(
            &measure,
            (arbor_span){(const uint8_t *)entity_like, sizeof(entity_like) - 1u}).native != 0 ||
        measure.length != (uint64_t)(sizeof(entity_expected) - 1u)) {
        return 5;
    }

    output = (arbor_view_output){0};
    if (arbor_view_output_begin(&arena, measure.length, &output).native != 0 ||
        arbor_view_html_text_append(
            &output,
            (arbor_span){(const uint8_t *)entity_like, sizeof(entity_like) - 1u}).native != 0 ||
        arbor_view_output_commit(&output, &body).native != 0 ||
        !span_equals(body, entity_expected)) {
        return 6;
    }

    const char arena_model[] = "x<y & z>q";
    arbor_asm_result_ptr model_storage = arena_alloc(&arena, sizeof(arena_model) - 1u);
    if (model_storage.status != 0) {
        return 7;
    }
    (void)memory_copy(model_storage.value, arena_model, sizeof(arena_model) - 1u);
    measure = (arbor_view_measure){0};
    if (arbor_view_html_text_measure(
            &measure,
            (arbor_span){model_storage.value, sizeof(arena_model) - 1u}).native != 0) {
        return 8;
    }
    output = (arbor_view_output){0};
    if (arbor_view_output_begin(&arena, measure.length, &output).native != 0 ||
        arbor_view_html_text_append(
            &output,
            (arbor_span){model_storage.value, sizeof(arena_model) - 1u}).native != 0 ||
        arbor_view_output_commit(&output, &body).native != 0 ||
        !span_equals(body, "x&lt;y &amp; z&gt;q")) {
        return 9;
    }

    measure = (arbor_view_measure){7u};
    if (arbor_view_html_text_measure(&measure, (arbor_span){NULL, 0u}).native != 0 ||
        measure.length != 7u) {
        return 10;
    }

    puts("PASS: VIEW0 C2 exact HTML text measurement, context escaping, trusted markup composition and same-arena non-overlap");
    return 0;
}
