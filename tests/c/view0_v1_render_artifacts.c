#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <arborcore/view.h>

extern arbor_status view0_c4_asm_render_html_text(
    arbor_asm_arena *arena,
    arbor_span text,
    arbor_span *body_out);

static const uint8_t document_prefix[] =
    "<!doctype html><html lang=\"en\"><head><meta charset=\"utf-8\">"
    "<title>Arborcore VIEW0</title>"
    "<link rel=\"stylesheet\" href=\"/assets/site.css\">"
    "</head><body><main>";
static const uint8_t document_suffix[] = "</main></body></html>";
static const uint8_t paragraph_prefix[] = "<p>";
static const uint8_t paragraph_suffix[] = "</p>";
static const uint8_t model_text[] = {
    'O', 'l', UINT8_C(0xc3), UINT8_C(0xa1), ' ', '<', 'D', 'a', 'r', 'k', 'o',
    ' ', '&', ' ', 'F', 'r', 'i', 'e', 'n', 'd', 's', '>', ' ',
    UINT8_C(0xe4), UINT8_C(0xb8), UINT8_C(0x96),
    UINT8_C(0xe7), UINT8_C(0x95), UINT8_C(0x8c)
};

static int write_span(const char *path, arbor_span bytes)
{
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        return 1;
    }
    if (bytes.length > (uint64_t)SIZE_MAX) {
        (void)fclose(file);
        return 1;
    }
    const size_t length = (size_t)bytes.length;
    const size_t written = length == 0u ? 0u : fwrite(bytes.data, 1u, length, file);
    if (written != length || fclose(file) != 0) {
        return 1;
    }
    return 0;
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
    arbor_asm_arena *arena,
    arbor_span text,
    arbor_span *body_out)
{
    static const uint8_t source[] =
        "<!doctype html><html lang=\"en\"><head><meta charset=\"utf-8\">"
        "<title>Arborcore VIEW0</title>"
        "<link rel=\"stylesheet\" href=\"/assets/site.css\">"
        "</head><body><main><p>{{text}}</p></main></body></html>";
    static const uint8_t field_name[] = "text";
    const arbor_span fields[] = {
        {field_name, (uint64_t)(sizeof(field_name) - 1u)}
    };
    arbor_view_html_template_requirements requirements = {0};
    arbor_status status = arbor_view_html_template_measure(
        (arbor_span){source, (uint64_t)(sizeof(source) - 1u)},
        fields,
        1u,
        &requirements);
    if (status.native != 0) return status;
    if (requirements.part_count > 4u || requirements.literal_bytes > 512u) {
        return arbor_status_from_native(-EOVERFLOW);
    }

    arbor_view_html_template_part parts[4] = {0};
    uint8_t literals[512] = {0};
    arbor_view_html_template_storage storage = {
        parts,
        4u,
        literals,
        sizeof(literals)
    };
    arbor_view_html_template template_view = {0};
    status = arbor_view_html_template_prepare(
        (arbor_span){source, (uint64_t)(sizeof(source) - 1u)},
        fields,
        1u,
        &storage,
        &template_view);
    if (status.native != 0) return status;
    const arbor_span values[] = {text};
    return arbor_view_html_template_render(
        &template_view,
        values,
        1u,
        arena,
        body_out);
}

static int render_and_write_documents(void)
{
    const arbor_span text = {model_text, sizeof(model_text)};

    uint8_t template_storage[4096] = {0};
    arbor_asm_arena template_arena = {0};
    if (arena_init(&template_arena, template_storage, sizeof(template_storage)).status != 0) {
        return 1;
    }
    arbor_span template_body = {0};
    arbor_status status = render_template_document(&template_arena, text, &template_body);
    if (status.native != 0 || arbor_view_utf8_validate(template_body).native != 0 ||
        write_span("build/view0-v1/documents/template.html", template_body) != 0) {
        return 2;
    }

    uint8_t native_storage[4096] = {0};
    arbor_asm_arena native_arena = {0};
    if (arena_init(&native_arena, native_storage, sizeof(native_storage)).status != 0) {
        return 3;
    }
    arbor_span native_body = {0};
    status = render_native_document(&native_arena, text, &native_body);
    if (status.native != 0 || arbor_view_utf8_validate(native_body).native != 0 ||
        write_span("build/view0-v1/documents/native-c.html", native_body) != 0) {
        return 4;
    }

    uint8_t nasm_storage[4096] = {0};
    arbor_asm_arena nasm_arena = {0};
    if (arena_init(&nasm_arena, nasm_storage, sizeof(nasm_storage)).status != 0) {
        return 5;
    }
    arbor_span fragment = {0};
    status = view0_c4_asm_render_html_text(&nasm_arena, text, &fragment);
    if (status.native != 0 || arbor_view_utf8_validate(fragment).native != 0) {
        return 6;
    }
    arbor_span nasm_body = {0};
    status = append_document(&nasm_arena, fragment, &nasm_body);
    if (status.native != 0 || arbor_view_utf8_validate(nasm_body).native != 0 ||
        write_span("build/view0-v1/documents/nasm.html", nasm_body) != 0) {
        return 7;
    }

    return 0;
}

int main(void)
{
    const int status = render_and_write_documents();
    if (status != 0) {
        fprintf(stderr, "FAIL: VIEW0 V1 render-artifact generation (%d)\n", status);
        return status;
    }
    puts("PASS: VIEW0 V1 generated conforming-candidate HTML artifacts through template/native-C/NASM view paths");
    return 0;
}
