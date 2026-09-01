#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <arborcore/view.h>

/*
 * Canonical D1 body-rendering example.
 *
 * HTTP representation metadata is intentionally not produced here. The M1
 * presenter recipe validates the completed body as UTF-8, prepares the AF1
 * response plan, appends exactly one
 *
 *   Content-Type: text/html; charset=utf-8
 *
 * field through HTTP1, and only then publishes the plan. A controller or
 * middleware that has already added Content-Type violates that composition
 * precondition; D1 does not alter HTTP1's generic duplicate-field semantics.
 */

extern arbor_status arborcore_view0_d1_nasm_render_html_text(
    arbor_asm_arena *arena,
    arbor_span text,
    arbor_span *body_out);

static const uint8_t document_prefix[] =
    "<!doctype html><html lang=\"en\"><head><meta charset=\"utf-8\">"
    "<title>Arborcore VIEW0 D1</title></head><body><main>";
static const uint8_t document_suffix[] = "</main></body></html>\n";
static const uint8_t paragraph_prefix[] = "<p>";
static const uint8_t paragraph_suffix[] = "</p>";
static const uint8_t field_name[] = "text";
static const uint8_t model_text[] = {
    'O', 'l', UINT8_C(0xc3), UINT8_C(0xa1), ' ', '<', 'D', 'a', 'r', 'k', 'o',
    ' ', '&', ' ', 'F', 'r', 'i', 'e', 'n', 'd', 's', '>', ' ',
    UINT8_C(0xe4), UINT8_C(0xb8), UINT8_C(0x96),
    UINT8_C(0xe7), UINT8_C(0x95), UINT8_C(0x8c)
};

static int read_file_bounded(
    const char *path,
    uint8_t *storage,
    size_t capacity,
    arbor_span *out)
{
    if (path == NULL || storage == NULL || capacity == 0u || out == NULL) {
        return 1;
    }

    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return 1;
    }

    const size_t used = fread(storage, 1u, capacity, file);
    if (ferror(file) != 0) {
        (void)fclose(file);
        return 1;
    }
    if (used == capacity) {
        const int extra = fgetc(file);
        if (extra != EOF || ferror(file) != 0) {
            (void)fclose(file);
            return 1;
        }
    }
    if (fclose(file) != 0) {
        return 1;
    }

    *out = (arbor_span){storage, (uint64_t)used};
    return 0;
}

static int write_span(const char *path, arbor_span bytes)
{
    if (path == NULL || (bytes.length != 0u && bytes.data == NULL) ||
        bytes.length > (uint64_t)SIZE_MAX) {
        return 1;
    }

    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        return 1;
    }
    const size_t length = (size_t)bytes.length;
    const size_t written = length == 0u ? 0u : fwrite(bytes.data, 1u, length, file);
    if (written != length || fclose(file) != 0) {
        return 1;
    }
    return 0;
}

static int spans_equal(arbor_span left, arbor_span right)
{
    return left.length == right.length &&
        (left.length == 0u ||
         (left.data != NULL && right.data != NULL &&
          memcmp(left.data, right.data, (size_t)left.length) == 0));
}

static arbor_status append_document(
    arbor_asm_arena *arena,
    arbor_span middle,
    arbor_span *body_out)
{
    arbor_view_measure measure = {0};
    arbor_status status = arbor_view_measure_add(
        &measure, (uint64_t)(sizeof(document_prefix) - 1u));
    if (status.native != 0) return status;
    status = arbor_view_measure_add(&measure, middle.length);
    if (status.native != 0) return status;
    status = arbor_view_measure_add(
        &measure, (uint64_t)(sizeof(document_suffix) - 1u));
    if (status.native != 0) return status;

    arbor_view_output output = {0};
    status = arbor_view_output_begin(arena, measure.length, &output);
    if (status.native != 0) return status;
    status = arbor_view_output_append(
        &output,
        (arbor_span){document_prefix, (uint64_t)(sizeof(document_prefix) - 1u)});
    if (status.native != 0) return status;
    status = arbor_view_output_append(&output, middle);
    if (status.native != 0) return status;
    status = arbor_view_output_append(
        &output,
        (arbor_span){document_suffix, (uint64_t)(sizeof(document_suffix) - 1u)});
    if (status.native != 0) return status;
    return arbor_view_output_commit(&output, body_out);
}

static arbor_status render_native_document(
    arbor_asm_arena *arena,
    arbor_span text,
    arbor_span *body_out)
{
    arbor_view_measure measure = {0};
    arbor_status status = arbor_view_measure_add(
        &measure, (uint64_t)(sizeof(document_prefix) - 1u));
    if (status.native != 0) return status;
    status = arbor_view_measure_add(
        &measure, (uint64_t)(sizeof(paragraph_prefix) - 1u));
    if (status.native != 0) return status;
    status = arbor_view_html_text_measure(&measure, text);
    if (status.native != 0) return status;
    status = arbor_view_measure_add(
        &measure, (uint64_t)(sizeof(paragraph_suffix) - 1u));
    if (status.native != 0) return status;
    status = arbor_view_measure_add(
        &measure, (uint64_t)(sizeof(document_suffix) - 1u));
    if (status.native != 0) return status;

    arbor_view_output output = {0};
    status = arbor_view_output_begin(arena, measure.length, &output);
    if (status.native != 0) return status;
    status = arbor_view_output_append(
        &output,
        (arbor_span){document_prefix, (uint64_t)(sizeof(document_prefix) - 1u)});
    if (status.native != 0) return status;
    status = arbor_view_output_append(
        &output,
        (arbor_span){paragraph_prefix, (uint64_t)(sizeof(paragraph_prefix) - 1u)});
    if (status.native != 0) return status;
    status = arbor_view_html_text_append(&output, text);
    if (status.native != 0) return status;
    status = arbor_view_output_append(
        &output,
        (arbor_span){paragraph_suffix, (uint64_t)(sizeof(paragraph_suffix) - 1u)});
    if (status.native != 0) return status;
    status = arbor_view_output_append(
        &output,
        (arbor_span){document_suffix, (uint64_t)(sizeof(document_suffix) - 1u)});
    if (status.native != 0) return status;
    return arbor_view_output_commit(&output, body_out);
}

static arbor_status render_template_document(
    arbor_span source,
    arbor_asm_arena *arena,
    arbor_span text,
    arbor_span *body_out)
{
    const arbor_span fields[] = {
        {field_name, (uint64_t)(sizeof(field_name) - 1u)}
    };
    arbor_view_html_template_requirements requirements = {0};
    arbor_status status = arbor_view_html_template_measure(
        source, fields, 1u, &requirements);
    if (status.native != 0) return status;
    if (requirements.part_count > 4u || requirements.literal_bytes > 512u) {
        return arbor_status_from_native(-EOVERFLOW);
    }

    arbor_view_html_template_part parts[4] = {0};
    uint8_t literals[512] = {0};
    arbor_view_html_template_storage storage = {
        parts, 4u, literals, sizeof(literals)
    };
    arbor_view_html_template template_view = {0};
    status = arbor_view_html_template_prepare(
        source, fields, 1u, &storage, &template_view);
    if (status.native != 0) return status;

    const arbor_span values[] = {text};
    return arbor_view_html_template_render(
        &template_view, values, 1u, arena, body_out);
}

int main(int argc, char **argv)
{
    if (argc != 5) {
        (void)fprintf(
            stderr,
            "usage: %s TEMPLATE_FILE TEMPLATE_OUT NATIVE_OUT NASM_OUT\n",
            argc > 0 ? argv[0] : "view0-d1-example");
        return 2;
    }

    uint8_t template_source_storage[4096] = {0};
    arbor_span template_source = {0};
    if (read_file_bounded(
            argv[1], template_source_storage,
            sizeof(template_source_storage), &template_source) != 0) {
        (void)fprintf(stderr, "FAIL: cannot read bounded template source: %s\n", argv[1]);
        return 3;
    }

    const arbor_span model = {model_text, sizeof(model_text)};

    uint8_t template_arena_storage[4096] = {0};
    arbor_asm_arena template_arena = {0};
    if (arena_init(
            &template_arena, template_arena_storage,
            sizeof(template_arena_storage)).status != 0) {
        return 4;
    }
    arbor_span template_body = {0};
    arbor_status status = render_template_document(
        template_source, &template_arena, model, &template_body);
    if (status.native != 0 || arbor_view_utf8_validate(template_body).native != 0) {
        return 5;
    }

    uint8_t native_arena_storage[4096] = {0};
    arbor_asm_arena native_arena = {0};
    if (arena_init(
            &native_arena, native_arena_storage,
            sizeof(native_arena_storage)).status != 0) {
        return 6;
    }
    arbor_span native_body = {0};
    status = render_native_document(&native_arena, model, &native_body);
    if (status.native != 0 || arbor_view_utf8_validate(native_body).native != 0) {
        return 7;
    }

    uint8_t nasm_arena_storage[4096] = {0};
    arbor_asm_arena nasm_arena = {0};
    if (arena_init(
            &nasm_arena, nasm_arena_storage,
            sizeof(nasm_arena_storage)).status != 0) {
        return 8;
    }
    arbor_span fragment = {0};
    status = arborcore_view0_d1_nasm_render_html_text(
        &nasm_arena, model, &fragment);
    if (status.native != 0 || arbor_view_utf8_validate(fragment).native != 0) {
        return 9;
    }
    arbor_span nasm_body = {0};
    status = append_document(&nasm_arena, fragment, &nasm_body);
    if (status.native != 0 || arbor_view_utf8_validate(nasm_body).native != 0) {
        return 10;
    }

    if (!spans_equal(template_body, native_body) ||
        !spans_equal(template_body, nasm_body)) {
        (void)fputs("FAIL: D1 renderer modes produced different document bytes\n", stderr);
        return 11;
    }

    if (write_span(argv[2], template_body) != 0 ||
        write_span(argv[3], native_body) != 0 ||
        write_span(argv[4], nasm_body) != 0) {
        (void)fputs("FAIL: cannot write D1 generated documents\n", stderr);
        return 12;
    }

    puts("VIEW0_D1_RENDERER_MODES=3_OF_3");
    puts("VIEW0_D1_RENDERER_BYTE_EQUIVALENCE=PASS");
    puts("VIEW0_D1_UTF8_VALIDATION=PASS");
    puts("PASS: VIEW0 D1 file-template/native-C/NASM runnable example rendered identical UTF-8 HTML documents");
    return 0;
}
