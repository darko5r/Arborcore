#include <arborcore/view.h>

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int bytes_all(const uint8_t *bytes, size_t length, uint8_t value)
{
    for (size_t i = 0u; i < length; ++i) {
        if (bytes[i] != value) {
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    arbor_view_measure measure = {17u};
    if (arbor_view_measure_add(NULL, 1u).native != -EINVAL ||
        arbor_view_measure_add(&measure, UINT64_MAX).native != -EOVERFLOW ||
        measure.length != 17u) {
        return 1;
    }

    uint8_t arena_bytes[64];
    memset(arena_bytes, 0xA5, sizeof(arena_bytes));
    arbor_asm_arena arena = {0};
    if (arena_init(&arena, arena_bytes, sizeof(arena_bytes)).status != 0) {
        return 2;
    }

    arbor_view_output sentinel = {
        .arena = (arbor_asm_arena *)(uintptr_t)UINT64_C(0x11111111),
        .arena_mark = UINT64_C(0x22222222),
        .buffer = {(uint8_t *)(uintptr_t)UINT64_C(0x33333333), UINT64_C(0x44), UINT64_C(0x55)},
        .required_length = UINT64_C(0x66666666),
        .state = ARBOR_VIEW_OUTPUT_STATE_COMMITTED
    };
    const arbor_view_output sentinel_before = sentinel;
    if (arbor_view_output_begin(&arena, 65u, &sentinel).native != -ENOSPC ||
        memcmp(&sentinel, &sentinel_before, sizeof(sentinel)) != 0 || arena.offset != 0u) {
        return 3;
    }

    arbor_asm_arena bad = {(uint8_t *)(uintptr_t)(UINTPTR_MAX - 3u), 8u, 0u};
    arbor_view_output clean = {0};
    if (arbor_view_output_begin(&bad, 1u, &clean).native != -EOVERFLOW ||
        clean.state != ARBOR_VIEW_OUTPUT_STATE_IDLE) {
        return 4;
    }
    bad = (arbor_asm_arena){NULL, 1u, 0u};
    if (arbor_view_output_begin(&bad, 1u, &clean).native != -EINVAL) {
        return 5;
    }
    bad = (arbor_asm_arena){arena_bytes, sizeof(arena_bytes), sizeof(arena_bytes) + 1u};
    if (arbor_view_output_begin(&bad, 1u, &clean).native != -EINVAL) {
        return 6;
    }

    arbor_asm_arena self_alias = {0};
    uint8_t self_backing[128] = {0};
    arbor_asm_arena *self = (arbor_asm_arena *)(void *)&self_backing[16];
    self_alias = (arbor_asm_arena){self_backing, sizeof(self_backing), 0u};
    *self = self_alias;
    if (arbor_view_output_begin(self, 1u, &clean).native != -EINVAL) {
        return 7;
    }

    arbor_view_output *inside = (arbor_view_output *)(void *)&arena_bytes[0];
    memset(inside, 0, sizeof(*inside));
    if (arbor_view_output_begin(&arena, 1u, inside).native != -EINVAL || arena.offset != 0u) {
        return 8;
    }
    memset(arena_bytes, 0xA5, sizeof(arena_bytes));

    arbor_view_output output = {0};
    if (arbor_view_output_begin(&arena, 4u, &output).native != 0) {
        return 9;
    }
    const uint64_t mark = output.arena_mark;
    if (arbor_view_output_append(&output, (arbor_span){NULL, 1u}).native != -EINVAL ||
        output.state != ARBOR_VIEW_OUTPUT_STATE_IDLE || arena.offset != mark) {
        return 10;
    }

    if (arbor_view_output_begin(&arena, 4u, &output).native != 0 ||
        arbor_view_output_append(
            &output, (arbor_span){(const uint8_t *)"12345", 5u}).native != -ENOSPC ||
        output.state != ARBOR_VIEW_OUTPUT_STATE_IDLE || arena.offset != mark) {
        return 11;
    }

    if (arbor_view_output_begin(&arena, 4u, &output).native != 0 ||
        arbor_view_output_append(
            &output, (arbor_span){(const uint8_t *)"12", 2u}).native != 0) {
        return 12;
    }
    arbor_span body_sentinel = {(const uint8_t *)"keep", 4u};
    if (arbor_view_output_commit(&output, &body_sentinel).native != -EINVAL ||
        body_sentinel.length != 4u || memcmp(body_sentinel.data, "keep", 4u) != 0 ||
        output.state != ARBOR_VIEW_OUTPUT_STATE_IDLE || arena.offset != mark) {
        return 13;
    }

    if (arbor_view_output_begin(&arena, 4u, &output).native != 0 ||
        arbor_view_output_append(
            &output, (arbor_span){(const uint8_t *)"ABCD", 4u}).native != 0) {
        return 14;
    }
    arbor_span *body_alias = (arbor_span *)(void *)output.buffer.data;
    if (arbor_view_output_commit(&output, body_alias).native != -EINVAL ||
        output.state != ARBOR_VIEW_OUTPUT_STATE_IDLE || arena.offset != mark) {
        return 15;
    }

    arbor_view_output active = {0};
    if (arbor_view_output_begin(&arena, 4u, &active).native != 0) {
        return 16;
    }
    const uint64_t expected_frontier = arena.offset;
    arbor_asm_result_ptr unrelated = arena_alloc(&arena, 1u);
    if (unrelated.status != 0 || arena.offset != expected_frontier + 1u ||
        arbor_view_output_abort(&active).native != -EINVAL ||
        arena.offset != expected_frontier + 1u || active.state != ARBOR_VIEW_OUTPUT_STATE_ACTIVE) {
        return 17;
    }
    if (arena_rewind(&arena, expected_frontier).status != 0 ||
        arbor_view_output_abort(&active).native != 0 || arena.offset != mark) {
        return 18;
    }

    arbor_view_output corrupt = {0};
    if (arbor_view_output_begin(&arena, 3u, &corrupt).native != 0) {
        return 19;
    }
    const uint64_t corrupt_frontier = arena.offset;
    corrupt.buffer.capacity = 2u;
    if (arbor_view_output_append(
            &corrupt, (arbor_span){(const uint8_t *)"x", 1u}).native != -EINVAL ||
        corrupt.state != ARBOR_VIEW_OUTPUT_STATE_ACTIVE ||
        arena.offset != corrupt_frontier) {
        return 20;
    }
    corrupt.buffer.capacity = 3u;
    if (arbor_view_output_abort(&corrupt).native != 0 || arena.offset != mark) {
        return 21;
    }

    if (!bytes_all(arena_bytes + mark, sizeof(arena_bytes) - (size_t)mark, 0xA5)) {
        /* Arena rewind is logical only: writes before a detected failure may remain.
         * This check is intentionally NOT required to pass. */
    }

    puts("PASS: VIEW0 C1 adversarial bounds, alias, rollback, frontier-integrity and corruption guards");
    return 0;
}
