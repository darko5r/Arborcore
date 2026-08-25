#include <arborcore/view.h>

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct tiny_model {
    arbor_span text;
} tiny_model;

static const uint8_t PREFIX[] = "<p>";
static const uint8_t SUFFIX[] = "</p>";


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

static arbor_status tiny_view_render(
    arbor_asm_arena *arena,
    const tiny_model *model,
    arbor_span *body_out)
{
    if (arena == NULL || model == NULL || body_out == NULL) {
        return arbor_status_from_native(-EINVAL);
    }

    arbor_view_measure measure = {0};
    arbor_status status = arbor_view_measure_add(
        &measure, (uint64_t)(sizeof(PREFIX) - 1u));
    if (status.native != 0) {
        return status;
    }
    status = arbor_view_html_text_measure(&measure, model->text);
    if (status.native != 0) {
        return status;
    }
    status = arbor_view_measure_add(
        &measure, (uint64_t)(sizeof(SUFFIX) - 1u));
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
        ranges_conflict(body_out, sizeof(*body_out), model->text.data, model->text.length)) {
        return arbor_status_from_native(-EINVAL);
    }

    arbor_view_output output = {0};
    status = arbor_view_output_begin(arena, measure.length, &output);
    if (status.native != 0) {
        return status;
    }
    status = arbor_view_output_append(
        &output, (arbor_span){PREFIX, sizeof(PREFIX) - 1u});
    if (status.native != 0) {
        return status;
    }
    status = arbor_view_html_text_append(&output, model->text);
    if (status.native != 0) {
        return status;
    }
    status = arbor_view_output_append(
        &output, (arbor_span){SUFFIX, sizeof(SUFFIX) - 1u});
    if (status.native != 0) {
        return status;
    }
    return arbor_view_output_commit(&output, body_out);
}

static arbor_status explicit_internal_failure_view(
    arbor_asm_arena *arena,
    arbor_span *body_out)
{
    if (arena == NULL || body_out == NULL) {
        return arbor_status_from_native(-EINVAL);
    }
    arbor_view_output output = {0};
    arbor_status status = arbor_view_output_begin(arena, 7u, &output);
    if (status.native != 0) {
        return status;
    }
    status = arbor_view_output_append(
        &output, (arbor_span){(const uint8_t *)"<p>", 3u});
    if (status.native != 0) {
        return status;
    }

    /* Application-defined compiled-view logic failed after a successful append. */
    status = arbor_view_output_abort(&output);
    if (status.native != 0) {
        return status;
    }
    return arbor_status_from_native(-EINVAL);
}

int main(void)
{
    uint8_t small_storage[8] = {0};
    arbor_asm_arena small_arena = {0};
    if (arena_init(&small_arena, small_storage, sizeof(small_storage)).status != 0) {
        return 1;
    }
    static const char dangerous[] = "<&>";
    const tiny_model dangerous_model = {
        .text = {(const uint8_t *)dangerous, sizeof(dangerous) - 1u}
    };
    arbor_span sentinel = {(const uint8_t *)"sentinel", 8u};
    const arbor_span sentinel_before = sentinel;
    if (tiny_view_render(&small_arena, &dangerous_model, &sentinel).native != -ENOSPC ||
        small_arena.offset != 0u ||
        sentinel.data != sentinel_before.data || sentinel.length != sentinel_before.length) {
        return 2;
    }

    uint8_t invalid_storage[64] = {0};
    arbor_asm_arena invalid_arena = {0};
    if (arena_init(&invalid_arena, invalid_storage, sizeof(invalid_storage)).status != 0) {
        return 3;
    }
    const tiny_model invalid_model = {
        .text = {NULL, 1u}
    };
    sentinel = sentinel_before;
    if (tiny_view_render(&invalid_arena, &invalid_model, &sentinel).native != -EINVAL ||
        invalid_arena.offset != 0u ||
        sentinel.data != sentinel_before.data || sentinel.length != sentinel_before.length) {
        return 4;
    }

    uint8_t alias_storage[64] = {0};
    arbor_asm_arena alias_arena = {0};
    if (arena_init(&alias_arena, alias_storage, sizeof(alias_storage)).status != 0) {
        return 5;
    }
    /* The borrowed source is representable but points at the future output frontier. */
    const tiny_model alias_model = {
        .text = {alias_storage, 1u}
    };
    sentinel = sentinel_before;
    if (tiny_view_render(&alias_arena, &alias_model, &sentinel).native != -EINVAL ||
        alias_arena.offset != 0u ||
        sentinel.data != sentinel_before.data || sentinel.length != sentinel_before.length) {
        return 6;
    }

    uint8_t abort_storage[32] = {0};
    arbor_asm_arena abort_arena = {0};
    if (arena_init(&abort_arena, abort_storage, sizeof(abort_storage)).status != 0) {
        return 7;
    }
    sentinel = sentinel_before;
    if (explicit_internal_failure_view(&abort_arena, &sentinel).native != -EINVAL ||
        abort_arena.offset != 0u ||
        sentinel.data != sentinel_before.data || sentinel.length != sentinel_before.length) {
        return 8;
    }

    _Alignas(tiny_model) uint8_t future_model_storage[64] = {0};
    arbor_asm_arena future_model_arena = {0};
    if (arena_init(&future_model_arena, future_model_storage, sizeof(future_model_storage)).status != 0) {
        return 9;
    }
    tiny_model *future_model = (tiny_model *)(void *)future_model_storage;
    *future_model = (tiny_model){
        .text = {(const uint8_t *)"x", 1u}
    };
    const tiny_model future_model_before = *future_model;
    sentinel = sentinel_before;
    if (tiny_view_render(&future_model_arena, future_model, &sentinel).native != -EINVAL ||
        future_model_arena.offset != 0u ||
        future_model->text.data != future_model_before.text.data ||
        future_model->text.length != future_model_before.text.length ||
        sentinel.data != sentinel_before.data || sentinel.length != sentinel_before.length) {
        return 10;
    }

    uint8_t result_alias_storage[64] = {0};
    arbor_asm_arena result_alias_arena = {0};
    if (arena_init(&result_alias_arena, result_alias_storage, sizeof(result_alias_storage)).status != 0) {
        return 11;
    }
    tiny_model result_alias_model = {
        .text = {(const uint8_t *)"x", 1u}
    };
    const tiny_model result_alias_before = result_alias_model;
    if (tiny_view_render(
            &result_alias_arena,
            &result_alias_model,
            (arbor_span *)(void *)&result_alias_model).native != -EINVAL ||
        result_alias_arena.offset != 0u ||
        result_alias_model.text.data != result_alias_before.text.data ||
        result_alias_model.text.length != result_alias_before.text.length) {
        return 12;
    }

    _Alignas(arbor_span) uint8_t source_result_storage[32] = {0};
    source_result_storage[8] = (uint8_t)'x';
    arbor_asm_arena source_result_arena = {0};
    uint8_t separate_body_storage[64] = {0};
    if (arena_init(&source_result_arena, separate_body_storage, sizeof(separate_body_storage)).status != 0) {
        return 13;
    }
    tiny_model source_result_model = {
        .text = {source_result_storage + 8u, 1u}
    };
    if (tiny_view_render(
            &source_result_arena,
            &source_result_model,
            (arbor_span *)(void *)source_result_storage).native != -EINVAL ||
        source_result_arena.offset != 0u || source_result_storage[8] != (uint8_t)'x') {
        return 14;
    }

    uint8_t exact_storage[32] = {0};
    arbor_asm_arena exact_arena = {0};
    if (arena_init(&exact_arena, exact_storage, sizeof(exact_storage)).status != 0) {
        return 15;
    }
    static const char safe[] = "x";
    const tiny_model safe_model = {
        .text = {(const uint8_t *)safe, sizeof(safe) - 1u}
    };
    arbor_span body = {0};
    if (tiny_view_render(&exact_arena, &safe_model, &body).native != 0 ||
        body.length != 8u || memcmp(body.data, "<p>x</p>", 8u) != 0) {
        return 16;
    }

    puts("PASS: VIEW0 C3 native compiled-view capacity, invalid-model, transform-alias, model/result alias, explicit-abort and publication atomicity");
    return 0;
}
