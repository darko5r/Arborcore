#include <arborcore/view.h>

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    arbor_view_measure measure = {UINT64_MAX - 3u};
    const arbor_view_measure before = measure;
    if (arbor_view_html_text_measure(
            &measure, (arbor_span){(const uint8_t *)"<", 1u}).native != -EOVERFLOW ||
        measure.length != before.length) {
        return 1;
    }
    if (arbor_view_html_text_measure(&measure, (arbor_span){NULL, 1u}).native != -EINVAL ||
        measure.length != before.length) {
        return 2;
    }
    if (arbor_view_html_text_measure(NULL, (arbor_span){NULL, 0u}).native != -EINVAL) {
        return 3;
    }

    arbor_view_measure aliased_measure = {UINT64_C(0x4141414141414141)};
    const arbor_view_measure aliased_before = aliased_measure;
    if (arbor_view_html_text_measure(
            &aliased_measure,
            (arbor_span){
                (const uint8_t *)(const void *)&aliased_measure,
                sizeof(aliased_measure)}).native != -EINVAL ||
        aliased_measure.length != aliased_before.length) {
        return 24;
    }

    uint8_t arena_bytes[96] = {0};
    arbor_asm_arena arena = {0};
    if (arena_init(&arena, arena_bytes, sizeof(arena_bytes)).status != 0) {
        return 4;
    }

    arbor_view_output output = {0};
    if (arbor_view_output_begin(&arena, 3u, &output).native != 0) {
        return 5;
    }
    const uint64_t mark = output.arena_mark;
    if (arbor_view_html_text_append(
            &output, (arbor_span){(const uint8_t *)"<", 1u}).native != -ENOSPC ||
        output.state != ARBOR_VIEW_OUTPUT_STATE_IDLE || arena.offset != mark) {
        return 6;
    }

    output = (arbor_view_output){0};
    if (arbor_view_output_begin(&arena, 1u, &output).native != 0) {
        return 7;
    }
    if (arbor_view_html_text_append(
            &output, (arbor_span){output.buffer.data, 1u}).native != -EINVAL ||
        output.state != ARBOR_VIEW_OUTPUT_STATE_IDLE || arena.offset != mark) {
        return 8;
    }

    output = (arbor_view_output){0};
    if (arbor_view_output_begin(&arena, 1u, &output).native != 0) {
        return 9;
    }
    if (arbor_view_html_text_append(
            &output, (arbor_span){(const uint8_t *)(const void *)&output, 1u}).native != -EINVAL ||
        output.state != ARBOR_VIEW_OUTPUT_STATE_IDLE || arena.offset != mark) {
        return 10;
    }

    output = (arbor_view_output){0};
    if (arbor_view_output_begin(&arena, 1u, &output).native != 0) {
        return 11;
    }
    if (arbor_view_html_text_append(
            &output, (arbor_span){(const uint8_t *)(const void *)&arena, 1u}).native != -EINVAL ||
        output.state != ARBOR_VIEW_OUTPUT_STATE_IDLE || arena.offset != mark) {
        return 12;
    }

    output = (arbor_view_output){0};
    if (arbor_view_output_begin(&arena, 1u, &output).native != 0) {
        return 13;
    }
    const arbor_span impossible = {
        (const uint8_t *)(uintptr_t)(UINTPTR_MAX - 1u),
        4u
    };
    if (arbor_view_html_text_append(&output, impossible).native != -EINVAL ||
        output.state != ARBOR_VIEW_OUTPUT_STATE_IDLE || arena.offset != mark) {
        return 14;
    }

    output = (arbor_view_output){0};
    arbor_span body = {(const uint8_t *)"sentinel", 8u};
    if (arbor_view_output_begin(&arena, 0u, &output).native != 0 ||
        arbor_view_html_text_append(&output, (arbor_span){NULL, 0u}).native != 0 ||
        arbor_view_output_commit(&output, &body).native != 0 ||
        body.length != 0u) {
        return 15;
    }

    static const uint8_t arbitrary[] = {
        0x00u, 0x01u, (uint8_t)'"', (uint8_t)'\'', 0x7fu, 0x80u, 0xffu
    };
    measure = (arbor_view_measure){0};
    if (arbor_view_html_text_measure(
            &measure, (arbor_span){arbitrary, sizeof(arbitrary)}).native != 0 ||
        measure.length != (uint64_t)sizeof(arbitrary)) {
        return 16;
    }
    output = (arbor_view_output){0};
    if (arbor_view_output_begin(&arena, measure.length, &output).native != 0 ||
        arbor_view_html_text_append(
            &output, (arbor_span){arbitrary, sizeof(arbitrary)}).native != 0 ||
        arbor_view_output_commit(&output, &body).native != 0 ||
        body.length != (uint64_t)sizeof(arbitrary) ||
        memcmp(body.data, arbitrary, sizeof(arbitrary)) != 0) {
        return 17;
    }

    for (uint32_t value = 0u; value <= UINT8_MAX; ++value) {
        uint8_t one = (uint8_t)value;
        uint8_t local_bytes[8] = {0};
        arbor_asm_arena local_arena = {0};
        if (arena_init(&local_arena, local_bytes, sizeof(local_bytes)).status != 0) {
            return 18;
        }
        arbor_view_measure local_measure = {0};
        if (arbor_view_html_text_measure(
                &local_measure, (arbor_span){&one, 1u}).native != 0) {
            return 19;
        }
        uint64_t expected_length = 1u;
        const char *expected_entity = NULL;
        if (one == (uint8_t)'&') {
            expected_length = 5u;
            expected_entity = "&amp;";
        } else if (one == (uint8_t)'<' || one == (uint8_t)'>') {
            expected_length = 4u;
            expected_entity = one == (uint8_t)'<' ? "&lt;" : "&gt;";
        }
        if (local_measure.length != expected_length) {
            return 20;
        }
        arbor_view_output local_output = {0};
        arbor_span local_body = {0};
        if (arbor_view_output_begin(
                &local_arena, local_measure.length, &local_output).native != 0 ||
            arbor_view_html_text_append(
                &local_output, (arbor_span){&one, 1u}).native != 0 ||
            arbor_view_output_commit(&local_output, &local_body).native != 0 ||
            local_body.length != expected_length) {
            return 21;
        }
        if (expected_entity != NULL) {
            if (memcmp(local_body.data, expected_entity, (size_t)expected_length) != 0) {
                return 22;
            }
        } else if (local_body.data[0] != one) {
            return 23;
        }
    }

    uint8_t mutable_text = (uint8_t)'x';
    arbor_view_measure mutable_measure = {0};
    if (arbor_view_html_text_measure(
            &mutable_measure, (arbor_span){&mutable_text, 1u}).native != 0 ||
        mutable_measure.length != 1u) {
        return 25;
    }
    mutable_text = (uint8_t)'&';
    arbor_asm_arena mutable_arena = {0};
    uint8_t mutable_bytes[8] = {0};
    if (arena_init(&mutable_arena, mutable_bytes, sizeof(mutable_bytes)).status != 0) {
        return 26;
    }
    arbor_view_output mutable_output = {0};
    if (arbor_view_output_begin(
            &mutable_arena, mutable_measure.length, &mutable_output).native != 0 ||
        arbor_view_html_text_append(
            &mutable_output, (arbor_span){&mutable_text, 1u}).native != -ENOSPC ||
        mutable_output.state != ARBOR_VIEW_OUTPUT_STATE_IDLE ||
        mutable_arena.offset != 0u) {
        return 27;
    }

    mutable_text = (uint8_t)'&';
    mutable_measure = (arbor_view_measure){0};
    if (arbor_view_html_text_measure(
            &mutable_measure, (arbor_span){&mutable_text, 1u}).native != 0 ||
        mutable_measure.length != 5u) {
        return 28;
    }
    mutable_text = (uint8_t)'x';
    mutable_output = (arbor_view_output){0};
    if (arbor_view_output_begin(
            &mutable_arena, mutable_measure.length, &mutable_output).native != 0 ||
        arbor_view_html_text_append(
            &mutable_output, (arbor_span){&mutable_text, 1u}).native != 0) {
        return 29;
    }
    arbor_span mutable_body = {0};
    if (arbor_view_output_commit(&mutable_output, &mutable_body).native != -EINVAL ||
        mutable_output.state != ARBOR_VIEW_OUTPUT_STATE_IDLE ||
        mutable_arena.offset != 0u) {
        return 30;
    }

    puts("PASS: VIEW0 C2 overflow atomicity, aliases, cross-pass mutation fail-closed behavior and exhaustive byte policy");
    return 0;
}
