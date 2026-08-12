#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <arborcore/arborcore.h>

typedef struct handler_context {
    uint64_t calls;
    uint64_t observed_value_length;
} handler_context;

static int64_t item_handler(
    const arbor_asm_http_request *request,
    void *context,
    const arbor_asm_route_param *params,
    uint64_t parameter_count)
{
    handler_context *state = (handler_context *)context;
    static const uint8_t id_name[] = {'i', 'd'};
    static const uint8_t value[] = {'4', '2'};

    if (request == NULL || state == NULL || params == NULL || parameter_count != 1u) {
        return -22;
    }
    if (params[0].name_len != sizeof(id_name) ||
        memcmp(params[0].name_ptr, id_name, sizeof(id_name)) != 0) {
        return -22;
    }
    if (params[0].value_len != sizeof(value) ||
        memcmp(params[0].value_ptr, value, sizeof(value)) != 0) {
        return -22;
    }

    state->calls += 1u;
    state->observed_value_length = params[0].value_len;
    return 200;
}

int main(void)
{
    static const uint8_t request_bytes[] =
        "GET /items/42?x=1 HTTP/1.1\r\n"
        "Host: example\r\n"
        "\r\n";
    static const uint8_t expected_path[] = "/items/42";
    static const uint8_t expected_query[] = "x=1";
    static const uint8_t method[] = "GET";
    static const uint8_t pattern[] = "/items/:id";
    static const uint8_t body[] = "ok";
    static const uint8_t response_prefix[] = "HTTP/1.1 200 OK\r\n";

    arbor_request_view view;
    uint64_t required = 0u;
    arbor_status status = arbor_request_parse(
        (arbor_span){request_bytes, (uint64_t)sizeof(request_bytes) - 1u},
        &view,
        &required);
    if (arbor_status_is_error(status) || required != (uint64_t)sizeof(request_bytes) - 1u) {
        return 1;
    }
    if (view.target.path_len != (uint64_t)sizeof(expected_path) - 1u ||
        memcmp(view.target.path_ptr, expected_path, sizeof(expected_path) - 1u) != 0) {
        return 2;
    }
    if (view.target.query_len != (uint64_t)sizeof(expected_query) - 1u ||
        memcmp(view.target.query_ptr, expected_query, sizeof(expected_query) - 1u) != 0) {
        return 3;
    }
    if (view.native.method_ptr != request_bytes) {
        return 4;
    }

    arbor_route route;
    status = arbor_route_init(
        &route,
        (arbor_span){method, (uint64_t)sizeof(method) - 1u},
        (arbor_span){pattern, (uint64_t)sizeof(pattern) - 1u},
        item_handler);
    if (arbor_status_is_error(status)) {
        return 5;
    }

    arbor_route_param params[2];
    bool matched = false;
    uint64_t parameter_count = 0u;
    status = arbor_route_match(
        (arbor_span){pattern, (uint64_t)sizeof(pattern) - 1u},
        (arbor_span){view.target.path_ptr, view.target.path_len},
        params,
        2u,
        &matched,
        &parameter_count);
    if (arbor_status_is_error(status) || !matched || parameter_count != 1u) {
        return 6;
    }

    handler_context context = {0u, 0u};
    int64_t handler_result = 0;
    status = arbor_route_dispatch(
        &route,
        1u,
        &view.native,
        &context,
        params,
        2u,
        &handler_result);
    if (arbor_status_is_error(status) || handler_result != 200 ||
        context.calls != 1u || context.observed_value_length != 2u) {
        return 7;
    }

    uint8_t output_bytes[256];
    arbor_asm_buffer output;
    arbor_asm_result_u64 init = buffer_init(&output, output_bytes, (uint64_t)sizeof(output_bytes));
    if (init.status != 0) {
        return 8;
    }

    uint64_t written = 0u;
    status = arbor_response_serialize(
        &output,
        200u,
        (arbor_span){body, (uint64_t)sizeof(body) - 1u},
        true,
        &written);
    if (arbor_status_is_error(status) || written != output.length) {
        return 9;
    }
    if (output.length < (uint64_t)sizeof(response_prefix) - 1u ||
        memcmp(output.data, response_prefix, sizeof(response_prefix) - 1u) != 0) {
        return 10;
    }

    uint8_t secret_a[] = {1u, 2u, 3u, 4u};
    uint8_t secret_b[] = {1u, 2u, 3u, 4u};
    if (!arbor_secure_equal(secret_a, secret_b, (uint64_t)sizeof(secret_a))) {
        return 11;
    }
    (void)arbor_secure_clear(secret_a, (uint64_t)sizeof(secret_a));
    for (uint64_t i = 0u; i < (uint64_t)sizeof(secret_a); ++i) {
        if (secret_a[i] != 0u) {
            return 12;
        }
    }

    return 0;
}
