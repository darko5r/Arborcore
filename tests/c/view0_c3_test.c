#include <arborcore/view.h>

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct hello_model {
    arbor_span name;
    arbor_span message;
} hello_model;

static const uint8_t PAGE_PREFIX[] =
    "<!doctype html><html><head>"
    "<meta charset=\"utf-8\">"
    "<link rel=\"stylesheet\" href=\"/assets/site.css\">"
    "<title>Arborcore</title></head><body><h1>Hello ";
static const uint8_t PAGE_MIDDLE[] = "</h1><p>";
static const uint8_t PAGE_SUFFIX[] = "</p></body></html>";


static bool ranges_conflict(
    const void *left,
    uint64_t left_length,
    const void *right,
    uint64_t right_length)
{
    if (left_length == 0u || right_length == 0u) {
        return false;
    }
    if (left == NULL || right == NULL) {
        return true;
    }
    arbor_asm_result_u64 overlap = range_overlaps(
        (uint64_t)(uintptr_t)left,
        left_length,
        (uint64_t)(uintptr_t)right,
        right_length);
    return overlap.status != 0 || overlap.value != 0u;
}

static arbor_span literal_span(const uint8_t *data, uint64_t length)
{
    return (arbor_span){data, length};
}

static arbor_status hello_view_render(
    arbor_asm_arena *arena,
    const hello_model *model,
    arbor_span *body_out)
{
    if (arena == NULL || model == NULL || body_out == NULL) {
        return arbor_status_from_native(-EINVAL);
    }

    arbor_view_measure measure = {0};
    arbor_status status = arbor_view_measure_add(
        &measure, (uint64_t)(sizeof(PAGE_PREFIX) - 1u));
    if (status.native != 0) {
        return status;
    }
    status = arbor_view_html_text_measure(&measure, model->name);
    if (status.native != 0) {
        return status;
    }
    status = arbor_view_measure_add(
        &measure, (uint64_t)(sizeof(PAGE_MIDDLE) - 1u));
    if (status.native != 0) {
        return status;
    }
    status = arbor_view_html_text_measure(&measure, model->message);
    if (status.native != 0) {
        return status;
    }
    status = arbor_view_measure_add(
        &measure, (uint64_t)(sizeof(PAGE_SUFFIX) - 1u));
    if (status.native != 0) {
        return status;
    }

    arbor_asm_result_u64 future_start = range_end_checked(
        (uint64_t)(uintptr_t)arena->base, arena->offset);
    const bool future_reservation_fits =
        arena->offset <= arena->capacity &&
        measure.length <= arena->capacity - arena->offset;
    if (future_start.status != 0 ||
        ranges_conflict(model, sizeof(*model), arena, sizeof(*arena)) ||
        (future_reservation_fits &&
         ranges_conflict(model, sizeof(*model),
                         (const void *)(uintptr_t)future_start.value, measure.length)) ||
        ranges_conflict(body_out, sizeof(*body_out), model, sizeof(*model)) ||
        ranges_conflict(body_out, sizeof(*body_out), model->name.data, model->name.length) ||
        ranges_conflict(body_out, sizeof(*body_out), model->message.data, model->message.length)) {
        return arbor_status_from_native(-EINVAL);
    }

    arbor_view_output output = {0};
    status = arbor_view_output_begin(arena, measure.length, &output);
    if (status.native != 0) {
        return status;
    }

    status = arbor_view_output_append(
        &output,
        literal_span(PAGE_PREFIX, (uint64_t)(sizeof(PAGE_PREFIX) - 1u)));
    if (status.native != 0) {
        return status;
    }
    status = arbor_view_html_text_append(&output, model->name);
    if (status.native != 0) {
        return status;
    }
    status = arbor_view_output_append(
        &output,
        literal_span(PAGE_MIDDLE, (uint64_t)(sizeof(PAGE_MIDDLE) - 1u)));
    if (status.native != 0) {
        return status;
    }
    status = arbor_view_html_text_append(&output, model->message);
    if (status.native != 0) {
        return status;
    }
    status = arbor_view_output_append(
        &output,
        literal_span(PAGE_SUFFIX, (uint64_t)(sizeof(PAGE_SUFFIX) - 1u)));
    if (status.native != 0) {
        return status;
    }
    return arbor_view_output_commit(&output, body_out);
}

static int span_equals(arbor_span span, const char *text)
{
    const size_t length = strlen(text);
    return span.length == (uint64_t)length &&
           (length == 0u || memcmp(span.data, text, length) == 0);
}

int main(void)
{
    static const char name[] = "<Darko & Friends>";
    static const char message[] = "C + Assembly: \"native\" & safe";
    static const char expected[] =
        "<!doctype html><html><head>"
        "<meta charset=\"utf-8\">"
        "<link rel=\"stylesheet\" href=\"/assets/site.css\">"
        "<title>Arborcore</title></head><body><h1>Hello "
        "&lt;Darko &amp; Friends&gt;"
        "</h1><p>C + Assembly: \"native\" &amp; safe</p></body></html>";

    const hello_model model = {
        .name = {(const uint8_t *)name, sizeof(name) - 1u},
        .message = {(const uint8_t *)message, sizeof(message) - 1u}
    };

    uint8_t storage[512] = {0};
    arbor_asm_arena arena = {0};
    if (arena_init(&arena, storage, sizeof(storage)).status != 0) {
        return 1;
    }

    arbor_span body = {0};
    if (hello_view_render(&arena, &model, &body).native != 0 ||
        !span_equals(body, expected)) {
        return 2;
    }

    const uint64_t first_end = arena.offset;
    static const char second_name[] = "Arborcore";
    const hello_model second_model = {
        .name = {(const uint8_t *)second_name, sizeof(second_name) - 1u},
        .message = {NULL, 0u}
    };
    arbor_span second_body = {0};
    if (hello_view_render(&arena, &second_model, &second_body).native != 0 ||
        arena.offset <= first_end ||
        !span_equals(
            second_body,
            "<!doctype html><html><head><meta charset=\"utf-8\">"
            "<link rel=\"stylesheet\" href=\"/assets/site.css\">"
            "<title>Arborcore</title></head><body><h1>Hello Arborcore"
            "</h1><p></p></body></html>") ||
        !span_equals(body, expected)) {
        return 3;
    }

    uint8_t same_arena_storage[256] = {0};
    arbor_asm_arena same_arena = {0};
    if (arena_init(&same_arena, same_arena_storage, sizeof(same_arena_storage)).status != 0) {
        return 4;
    }
    static const char arena_name[] = "A < B & C";
    arbor_asm_result_ptr name_storage = arena_alloc(
        &same_arena, sizeof(arena_name) - 1u);
    if (name_storage.status != 0) {
        return 5;
    }
    (void)memory_copy(name_storage.value, arena_name, sizeof(arena_name) - 1u);
    const hello_model same_arena_model = {
        .name = {name_storage.value, sizeof(arena_name) - 1u},
        .message = {(const uint8_t *)"ok", 2u}
    };
    arbor_span same_arena_body = {0};
    if (hello_view_render(&same_arena, &same_arena_model, &same_arena_body).native != 0 ||
        !span_equals(
            same_arena_body,
            "<!doctype html><html><head><meta charset=\"utf-8\">"
            "<link rel=\"stylesheet\" href=\"/assets/site.css\">"
            "<title>Arborcore</title></head><body><h1>Hello A &lt; B &amp; C"
            "</h1><p>ok</p></body></html>")) {
        return 6;
    }

    puts("PASS: VIEW0 C3 typed native C compiled view, trusted literals, escaped model text, CSS reference and request-arena lifetime");
    return 0;
}
