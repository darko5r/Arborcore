#include <arborcore/view.h>

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

arbor_status view0_c4_asm_render_html_text(
    arbor_asm_arena *arena,
    arbor_span text,
    arbor_span *body_out);
arbor_status view0_c4_asm_begin_abort(
    arbor_asm_arena *arena,
    uint64_t required_length);

static int span_unchanged(arbor_span value, arbor_span before)
{
    return value.data == before.data && value.length == before.length;
}

int main(void)
{
    const arbor_span sentinel_before = {(const uint8_t *)"sentinel", 8u};

    uint8_t small_storage[4] = {0};
    arbor_asm_arena small_arena = {0};
    if (arena_init(&small_arena, small_storage, sizeof(small_storage)).status != 0) {
        return 1;
    }
    arbor_span sentinel = sentinel_before;
    arbor_status status = view0_c4_asm_render_html_text(
        &small_arena,
        (arbor_span){(const uint8_t *)"x", 1u},
        &sentinel);
    if (status.code != ARBOR_STATUS_NO_SPACE || status.native != -ENOSPC ||
        small_arena.offset != 0u || !span_unchanged(sentinel, sentinel_before)) {
        return 2;
    }

    uint8_t invalid_storage[64] = {0};
    arbor_asm_arena invalid_arena = {0};
    if (arena_init(&invalid_arena, invalid_storage, sizeof(invalid_storage)).status != 0) {
        return 3;
    }
    sentinel = sentinel_before;
    status = view0_c4_asm_render_html_text(
        &invalid_arena,
        (arbor_span){NULL, 1u},
        &sentinel);
    if (status.code != ARBOR_STATUS_INVALID_ARGUMENT || status.native != -EINVAL ||
        invalid_arena.offset != 0u || !span_unchanged(sentinel, sentinel_before)) {
        return 4;
    }

    uint8_t alias_storage[64] = {0};
    alias_storage[0] = (uint8_t)'x';
    arbor_asm_arena alias_arena = {0};
    if (arena_init(&alias_arena, alias_storage, sizeof(alias_storage)).status != 0) {
        return 5;
    }
    sentinel = sentinel_before;
    status = view0_c4_asm_render_html_text(
        &alias_arena,
        (arbor_span){alias_storage, 1u},
        &sentinel);
    if (status.code != ARBOR_STATUS_INVALID_ARGUMENT || status.native != -EINVAL ||
        alias_arena.offset != 0u || alias_storage[0] != (uint8_t)'x' ||
        !span_unchanged(sentinel, sentinel_before)) {
        return 6;
    }

    _Alignas(arbor_span) uint8_t result_source_storage[32] = {0};
    result_source_storage[8] = (uint8_t)'x';
    uint8_t result_arena_storage[64] = {0};
    arbor_asm_arena result_arena = {0};
    if (arena_init(&result_arena, result_arena_storage, sizeof(result_arena_storage)).status != 0) {
        return 7;
    }
    status = view0_c4_asm_render_html_text(
        &result_arena,
        (arbor_span){result_source_storage + 8u, 1u},
        (arbor_span *)(void *)result_source_storage);
    if (status.code != ARBOR_STATUS_INVALID_ARGUMENT || status.native != -EINVAL ||
        result_arena.offset != 0u || result_source_storage[8] != (uint8_t)'x') {
        return 8;
    }

    uint8_t abort_small_storage[4] = {0};
    arbor_asm_arena abort_small = {0};
    if (arena_init(&abort_small, abort_small_storage, sizeof(abort_small_storage)).status != 0) {
        return 9;
    }
    status = view0_c4_asm_begin_abort(&abort_small, 8u);
    if (status.code != ARBOR_STATUS_NO_SPACE || status.native != -ENOSPC ||
        abort_small.offset != 0u) {
        return 10;
    }

    puts("PASS: VIEW0 C4 NASM consumer preserves status-code/native ABI, capacity precedence, alias safety and rollback");
    return 0;
}
