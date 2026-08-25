#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <arborcore/http_mvc.h>
#include <arborcore/view.h>

extern arbor_status view0_c4_asm_render_html_text(
    arbor_asm_arena *arena,
    arbor_span text,
    arbor_span *body_out);

typedef enum view_mode {
    VIEW_MODE_TEMPLATE = 0,
    VIEW_MODE_NATIVE_C = 1,
    VIEW_MODE_NASM = 2
} view_mode;

typedef struct view_context {
    view_mode mode;
    arbor_span model;
    arbor_view_html_template_part template_parts[4];
    uint8_t template_literals[32];
    arbor_view_html_template_storage template_storage;
    arbor_view_html_template template_view;
    uint64_t controller_calls;
    uint64_t presenter_calls;
} view_context;

static int fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    return 1;
}

static bool contains_bytes(
    const uint8_t *haystack,
    size_t haystack_length,
    const uint8_t *needle,
    size_t needle_length)
{
    if (needle_length == 0u) return true;
    if (needle_length > haystack_length) return false;
    for (size_t i = 0u; i + needle_length <= haystack_length; ++i) {
        if (memcmp(haystack + i, needle, needle_length) == 0) return true;
    }
    return false;
}

static size_t count_bytes(
    const uint8_t *haystack,
    size_t haystack_length,
    const uint8_t *needle,
    size_t needle_length)
{
    size_t count = 0u;
    if (needle_length == 0u || needle_length > haystack_length) return 0u;
    for (size_t i = 0u; i + needle_length <= haystack_length; ++i) {
        if (memcmp(haystack + i, needle, needle_length) == 0) count += 1u;
    }
    return count;
}

static int prepare_template(view_context *context)
{
    static const uint8_t source[] = "<p>{{text}}</p>";
    static const uint8_t field[] = "text";
    const arbor_span fields[] = {
        {field, sizeof(field) - 1u}
    };
    arbor_view_html_template_requirements requirements = {0};
    arbor_status status = arbor_view_html_template_measure(
        (arbor_span){source, sizeof(source) - 1u},
        fields,
        1u,
        &requirements);
    if (status.native != 0 || requirements.part_count > 4u ||
        requirements.literal_bytes > sizeof(context->template_literals)) {
        return 1;
    }
    context->template_storage = (arbor_view_html_template_storage){
        context->template_parts,
        4u,
        context->template_literals,
        sizeof(context->template_literals)
    };
    status = arbor_view_html_template_prepare(
        (arbor_span){source, sizeof(source) - 1u},
        fields,
        1u,
        &context->template_storage,
        &context->template_view);
    return status.native == 0 ? 0 : 1;
}

static arbor_status native_c_render(
    arbor_asm_arena *arena,
    arbor_span text,
    arbor_span *body_out)
{
    static const uint8_t prefix[] = "<p>";
    static const uint8_t suffix[] = "</p>";
    arbor_view_measure measure = {0};
    arbor_status status = arbor_view_measure_add(&measure, sizeof(prefix) - 1u);
    if (status.native != 0) return status;
    status = arbor_view_html_text_measure(&measure, text);
    if (status.native != 0) return status;
    status = arbor_view_measure_add(&measure, sizeof(suffix) - 1u);
    if (status.native != 0) return status;

    arbor_view_output output = {0};
    status = arbor_view_output_begin(arena, measure.length, &output);
    if (status.native != 0) return status;
    status = arbor_view_output_append(
        &output,
        (arbor_span){prefix, sizeof(prefix) - 1u});
    if (status.native != 0) return status;
    status = arbor_view_html_text_append(&output, text);
    if (status.native != 0) return status;
    status = arbor_view_output_append(
        &output,
        (arbor_span){suffix, sizeof(suffix) - 1u});
    if (status.native != 0) return status;
    return arbor_view_output_commit(&output, body_out);
}

static int64_t controller(
    const arbor_mvc_request *request,
    void *context_pointer,
    arbor_mvc_controller_result *out)
{
    view_context *context = (view_context *)context_pointer;
    if (arbor_mvc_request_validate(request).native != 0 ||
        context == NULL || out == NULL) {
        return -EINVAL;
    }
    context->controller_calls += 1u;
    *out = (arbor_mvc_controller_result){
        1u,
        ARBOR_MVC_CONTROLLER_RESULT_FLAGS_NONE,
        &context->model,
        sizeof(context->model)
    };
    return 0;
}

static int64_t publish_html(
    const arbor_mvc_request *request,
    arbor_span body,
    arbor_response_plan *out)
{
    if (request == NULL || out == NULL) return -EINVAL;

    arbor_status status = arbor_view_utf8_validate(body);
    if (status.native != 0) return status.native;

    arbor_response_plan candidate = {0};
    status = arbor_response_plan_make(
        200u,
        body,
        ARBOR_RESPONSE_PLAN_FLAG_NONE,
        &candidate);
    if (status.native != 0) return status.native;

    static const uint8_t name[] = "Content-Type";
    static const uint8_t value[] = "text/html; charset=utf-8";
    status = arbor_http_mvc_response_field_append(
        request,
        (arbor_span){name, sizeof(name) - 1u},
        (arbor_span){value, sizeof(value) - 1u});
    if (status.native != 0) return status.native;

    *out = candidate;
    return 0;
}

static int64_t presenter(
    const arbor_mvc_request *request,
    void *context_pointer,
    const arbor_mvc_controller_result *result,
    arbor_response_plan *out)
{
    view_context *context = (view_context *)context_pointer;
    if (request == NULL || context == NULL || result == NULL || out == NULL ||
        result->model_data == NULL || result->model_size != sizeof(arbor_span)) {
        return -EINVAL;
    }
    context->presenter_calls += 1u;
    const arbor_span text = *(const arbor_span *)result->model_data;
    arbor_span body = {0};
    arbor_status status = {0};

    switch (context->mode) {
    case VIEW_MODE_TEMPLATE: {
        const arbor_span values[] = {text};
        status = arbor_view_html_template_render(
            &context->template_view,
            values,
            1u,
            request->scope->arena,
            &body);
        break;
    }
    case VIEW_MODE_NATIVE_C:
        status = native_c_render(request->scope->arena, text, &body);
        break;
    case VIEW_MODE_NASM:
        status = view0_c4_asm_render_html_text(request->scope->arena, text, &body);
        break;
    default:
        return -EINVAL;
    }
    if (status.native != 0) return status.native;
    return publish_html(request, body, out);
}

static int run_case(
    view_mode mode,
    arbor_span model,
    uint64_t field_capacity,
    int64_t expected_native)
{
    view_context context = {0};
    context.mode = mode;
    context.model = model;
    if (mode == VIEW_MODE_TEMPLATE && prepare_template(&context) != 0) {
        return fail("prepare T1 template for M1 integration");
    }

    static const uint8_t method[] = "GET";
    static const uint8_t pattern[] = "/view";
    arbor_mvc_route route = {
        method, sizeof(method) - 1u,
        pattern, sizeof(pattern) - 1u,
        controller, &context,
        presenter, &context,
        NULL, 0u
    };
    arbor_mvc_catalog catalog = {
        ARBOR_MVC_ABI_VERSION,
        (uint32_t)sizeof(arbor_mvc_catalog),
        ARBOR_MVC_CATALOG_FLAGS_NONE,
        &route, 1u,
        NULL, 0u
    };
    arbor_route_param workspace_params[8] = {0};
    arbor_mvc_prepare_workspace workspace = {workspace_params, 8u};
    arbor_mvc_application mvc = {0};
    if (arbor_mvc_application_prepare(&catalog, &workspace, &mvc).native != 0) {
        return fail("prepare MVC application for M1");
    }
    arbor_http_mvc_application application = {0};
    if (arbor_http_mvc_application_prepare(&mvc, field_capacity, &application).native != 0) {
        return fail("prepare HTTP1 application for M1");
    }

    int sv[2] = {-1, -1};
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0) return fail("M1 socketpair");

    uint8_t input[4096] = {0};
    uint8_t output[4096] = {0};
    uint8_t arena[4096] = {0};
    arbor_runtime_storage storage = {0};
    if (arbor_runtime_storage_prepare(
            &storage,
            (arbor_mut_span){input, sizeof(input)},
            (arbor_mut_span){output, sizeof(output)},
            (arbor_mut_span){arena, sizeof(arena)}).native != 0) {
        close(sv[0]); close(sv[1]);
        return fail("M1 runtime storage");
    }
    arbor_asm_result_u64 initialized = connection_init(
        &storage.connection,
        sv[1],
        &storage.input,
        &storage.output,
        &storage.arena);
    if (initialized.status != 0 ||
        connection_transition(
            &storage.connection,
            ARBOR_ASM_CONNECTION_READING).status != 0) {
        close(sv[0]); close(sv[1]);
        return fail("M1 connection initialization");
    }

    static const uint8_t request[] =
        "GET /view HTTP/1.1\r\n"
        "Host: local\r\n"
        "Connection: close\r\n"
        "\r\n";
    if (write(sv[0], request, sizeof(request) - 1u) !=
        (ssize_t)(sizeof(request) - 1u)) {
        close(sv[0]); close(sv[1]);
        return fail("M1 write request");
    }

    uint64_t completed = 0u;
    arbor_status status = arbor_http_mvc_server_step(
        &storage,
        &application,
        -1,
        &completed);

    if (expected_native != 0) {
        const bool ok = status.native == expected_native && completed == 0u &&
            context.controller_calls == 1u && context.presenter_calls == 1u &&
            storage.output.length == 0u;
        close(sv[0]);
        if (!ok) return fail("M1 presenter failure blocked HTTP response publication");
        return 0;
    }

    if (status.native != 0 || completed != 1u || context.controller_calls != 1u ||
        context.presenter_calls != 1u ||
        storage.connection.state != ARBOR_ASM_CONNECTION_CLOSED) {
        close(sv[0]);
        return fail("M1 HTTP request completion");
    }

    uint8_t response[4096] = {0};
    ssize_t used = read(sv[0], response, sizeof(response));
    close(sv[0]);
    if (used <= 0) return fail("M1 response read");

    static const uint8_t status_line[] = "HTTP/1.1 200 OK\r\n";
    static const uint8_t content_type[] = "Content-Type: text/html; charset=utf-8\r\n";
    static const uint8_t content_length[] = "Content-Length: 30\r\n";
    static const uint8_t expected_body[] = {
        '<','p','>',
        'O','l',0xc3u,0xa1u,' ',
        '&','l','t',';','&','a','m','p',';','&','g','t',';',' ',
        0xf0u,0x9fu,0x98u,0x80u,
        '<','/','p','>'
    };
    const size_t response_length = (size_t)used;
    if (!contains_bytes(response, response_length, status_line, sizeof(status_line) - 1u) ||
        !contains_bytes(response, response_length, content_type, sizeof(content_type) - 1u) ||
        count_bytes(response, response_length, content_type, sizeof(content_type) - 1u) != 1u ||
        !contains_bytes(response, response_length, content_length, sizeof(content_length) - 1u) ||
        !contains_bytes(response, response_length, expected_body, sizeof(expected_body))) {
        return fail("M1 serialized UTF-8 HTML response");
    }
    return 0;
}

int main(void)
{
    static const uint8_t valid_model[] = {
        'O','l',0xc3u,0xa1u,' ','<','&','>',' ',0xf0u,0x9fu,0x98u,0x80u
    };
    const arbor_span valid = {valid_model, sizeof(valid_model)};
    if (run_case(VIEW_MODE_TEMPLATE, valid, 1u, 0) != 0 ||
        run_case(VIEW_MODE_NATIVE_C, valid, 1u, 0) != 0 ||
        run_case(VIEW_MODE_NASM, valid, 1u, 0) != 0) {
        return 1;
    }

    static const uint8_t invalid_model[] = {0xc0u, 0xafu};
    if (run_case(
            VIEW_MODE_TEMPLATE,
            (arbor_span){invalid_model, sizeof(invalid_model)},
            1u,
            -EILSEQ) != 0) {
        return 1;
    }

    if (run_case(VIEW_MODE_TEMPLATE, valid, 0u, -ENOSPC) != 0) {
        return 1;
    }

    puts("PASS: VIEW0 M1 template/native-C/NASM presenters serialize safe UTF-8 HTML through HTTP1 and fail invalid UTF-8/metadata capacity before response publication");
    return 0;
}
