#include <arborcore/view.h>

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
arbor_status view0_c4_asm_call_render_preserve(
    arbor_asm_arena *arena,
    arbor_span text,
    arbor_span *body_out);

static int span_equals(arbor_span span, const char *text)
{
    const size_t length = strlen(text);
    return span.length == (uint64_t)length &&
           (length == 0u || memcmp(span.data, text, length) == 0);
}

int main(void)
{
    static const char dangerous[] = "<Darko & \"Friends\">";
    static const char expected[] =
        "<p>&lt;Darko &amp; \"Friends\"&gt;</p>";

    uint8_t storage[256] = {0};
    arbor_asm_arena arena = {0};
    if (arena_init(&arena, storage, sizeof(storage)).status != 0) {
        return 1;
    }

    arbor_span body = {0};
    arbor_status status = view0_c4_asm_call_render_preserve(
        &arena,
        (arbor_span){(const uint8_t *)dangerous, sizeof(dangerous) - 1u},
        &body);
    if (status.code != ARBOR_STATUS_OK || status.native != 0 ||
        !span_equals(body, expected)) {
        return 2;
    }

    const uint64_t first_end = arena.offset;
    arbor_span second = {0};
    status = view0_c4_asm_render_html_text(
        &arena,
        (arbor_span){NULL, 0u},
        &second);
    if (status.code != ARBOR_STATUS_OK || status.native != 0 ||
        arena.offset <= first_end || !span_equals(second, "<p></p>") ||
        !span_equals(body, expected)) {
        return 3;
    }

    uint8_t same_arena_storage[128] = {0};
    arbor_asm_arena same_arena = {0};
    if (arena_init(&same_arena, same_arena_storage, sizeof(same_arena_storage)).status != 0) {
        return 4;
    }
    static const char prior_text[] = "A < B & C";
    arbor_asm_result_ptr copied = arena_alloc(&same_arena, sizeof(prior_text) - 1u);
    if (copied.status != 0) {
        return 5;
    }
    (void)memory_copy(copied.value, prior_text, sizeof(prior_text) - 1u);
    arbor_span same_arena_body = {0};
    status = view0_c4_asm_render_html_text(
        &same_arena,
        (arbor_span){copied.value, sizeof(prior_text) - 1u},
        &same_arena_body);
    if (status.code != ARBOR_STATUS_OK || status.native != 0 ||
        !span_equals(same_arena_body, "<p>A &lt; B &amp; C</p>") ||
        memcmp(copied.value, prior_text, sizeof(prior_text) - 1u) != 0) {
        return 6;
    }

    uint8_t abort_storage[32] = {0};
    arbor_asm_arena abort_arena = {0};
    if (arena_init(&abort_arena, abort_storage, sizeof(abort_storage)).status != 0) {
        return 7;
    }
    status = view0_c4_asm_begin_abort(&abort_arena, 16u);
    if (status.code != ARBOR_STATUS_OK || status.native != 0 ||
        abort_arena.offset != 0u) {
        return 8;
    }

    puts("PASS: VIEW0 C4 real NASM renders escaped HTML through all seven existing VIEW C functions with SysV preservation");
    return 0;
}
