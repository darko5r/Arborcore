#include <arborcore/view.h>

#include <errno.h>
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
    arbor_view_measure measure = {0};
    if (arbor_view_measure_add(&measure, 6u).native != 0 ||
        arbor_view_measure_add(&measure, 5u).native != 0 ||
        measure.length != 11u) {
        return 1;
    }
    const uint64_t before_overflow = measure.length;
    measure.length = UINT64_MAX;
    if (arbor_view_measure_add(&measure, 1u).native != -EOVERFLOW ||
        measure.length != UINT64_MAX) {
        return 2;
    }
    measure.length = before_overflow;

    uint8_t arena_bytes[128] = {0};
    arbor_asm_arena arena = {0};
    if (arena_init(&arena, arena_bytes, sizeof(arena_bytes)).status != 0) {
        return 3;
    }

    arbor_asm_result_ptr prefix = arena_alloc(&arena, 7u);
    if (prefix.status != 0 || prefix.value != arena_bytes) {
        return 4;
    }
    (void)memory_copy(prefix.value, "prefix!", 7u);
    const uint64_t body_mark = arena.offset;

    arbor_view_output output = {0};
    if (arbor_view_output_begin(&arena, measure.length, &output).native != 0 ||
        output.state != ARBOR_VIEW_OUTPUT_STATE_ACTIVE ||
        output.arena_mark != body_mark || output.buffer.length != 0u ||
        output.buffer.capacity != 11u || arena.offset != body_mark + 11u) {
        return 5;
    }
    if (arbor_view_output_append(
            &output, (arbor_span){(const uint8_t *)"hello ", 6u}).native != 0 ||
        arbor_view_output_append(
            &output, (arbor_span){(const uint8_t *)"world", 5u}).native != 0) {
        return 6;
    }

    arbor_span body = {(const uint8_t *)"sentinel", 8u};
    if (arbor_view_output_commit(&output, &body).native != 0 ||
        output.state != ARBOR_VIEW_OUTPUT_STATE_COMMITTED ||
        !span_equals(body, "hello world") || arena.offset != body_mark + 11u ||
        memcmp(arena_bytes, "prefix!", 7u) != 0) {
        return 7;
    }

    arbor_view_output alias_output = {0};
    if (arbor_view_output_begin(&arena, 6u, &alias_output).native != 0 ||
        arbor_view_output_append(
            &alias_output, (arbor_span){(const uint8_t *)"abc", 3u}).native != 0) {
        return 8;
    }
    const uint8_t *alias_source = alias_output.buffer.data;
    if (arbor_view_output_append(
            &alias_output, (arbor_span){alias_source, 3u}).native != 0) {
        return 9;
    }
    arbor_span alias_body = {0};
    if (arbor_view_output_commit(&alias_output, &alias_body).native != 0 ||
        !span_equals(alias_body, "abcabc")) {
        return 10;
    }

    const uint64_t abort_mark = arena.offset;
    arbor_view_output aborted = {0};
    if (arbor_view_output_begin(&arena, 4u, &aborted).native != 0 ||
        arbor_view_output_append(
            &aborted, (arbor_span){(const uint8_t *)"xy", 2u}).native != 0 ||
        arbor_view_output_abort(&aborted).native != 0 ||
        arena.offset != abort_mark || aborted.state != ARBOR_VIEW_OUTPUT_STATE_IDLE) {
        return 11;
    }

    arbor_view_output empty = {0};
    arbor_span empty_body = {(const uint8_t *)"sentinel", 8u};
    const uint64_t empty_mark = arena.offset;
    if (arbor_view_output_begin(&arena, 0u, &empty).native != 0 ||
        arbor_view_output_commit(&empty, &empty_body).native != 0 ||
        empty_body.length != 0u || arena.offset != empty_mark) {
        return 12;
    }

    puts("PASS: VIEW0 C1 checked measurement, exact bounded rendering, alias snapshot, abort and zero-length semantics");
    return 0;
}
