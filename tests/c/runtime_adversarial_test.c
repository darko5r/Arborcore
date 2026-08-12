#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <arborcore/arborcore.h>

static int64_t dummy_handler(
    const arbor_asm_http_request *request,
    void *context,
    const arbor_asm_route_param *params,
    uint64_t parameter_count)
{
    (void)request;
    (void)context;
    (void)params;
    (void)parameter_count;
    return 200;
}

int main(void)
{
    static const uint8_t incomplete[] = "GET / HTTP/1.1\r\nHost: x\r\n";
    static const uint8_t malformed[] = "GET /bad#fragment HTTP/1.1\r\n\r\n";
    static const uint8_t pattern[] = "/:a/:b";
    static const uint8_t path[] = "/x/y";

    arbor_status status = arbor_request_parse((arbor_span){NULL, 0u}, NULL, NULL);
    if (status.code != ARBOR_STATUS_INVALID_ARGUMENT || status.native != -EINVAL) {
        return 1;
    }

    arbor_request_view view;
    uint64_t required = 0u;
    status = arbor_request_parse(
        (arbor_span){incomplete, (uint64_t)sizeof(incomplete) - 1u},
        &view,
        &required);
    if (status.code != ARBOR_STATUS_WOULD_BLOCK || required != 0u) {
        return 2;
    }

    status = arbor_request_parse(
        (arbor_span){malformed, (uint64_t)sizeof(malformed) - 1u},
        &view,
        &required);
    if (status.code != ARBOR_STATUS_INVALID_ARGUMENT) {
        return 3;
    }

    uint8_t tiny[8];
    arbor_asm_buffer output;
    if (buffer_init(&output, tiny, (uint64_t)sizeof(tiny)).status != 0) {
        return 4;
    }
    uint64_t written = 999u;
    status = arbor_response_serialize(
        &output,
        200u,
        (arbor_span){NULL, 0u},
        true,
        &written);
    if (status.code != ARBOR_STATUS_NO_SPACE || written != 0u || output.length != 0u) {
        return 5;
    }

    arbor_route route;
    status = arbor_route_init(
        &route,
        (arbor_span){(const uint8_t *)"GET", 3u},
        (arbor_span){pattern, (uint64_t)sizeof(pattern) - 1u},
        NULL);
    if (status.code != ARBOR_STATUS_INVALID_ARGUMENT) {
        return 6;
    }

    status = arbor_route_init(
        &route,
        (arbor_span){(const uint8_t *)"GET", 3u},
        (arbor_span){pattern, (uint64_t)sizeof(pattern) - 1u},
        dummy_handler);
    if (arbor_status_is_error(status)) {
        return 7;
    }

    arbor_route_param one_param[1];
    bool matched = false;
    uint64_t count = 0u;
    status = arbor_route_match(
        (arbor_span){pattern, (uint64_t)sizeof(pattern) - 1u},
        (arbor_span){path, (uint64_t)sizeof(path) - 1u},
        one_param,
        1u,
        &matched,
        &count);
    if (status.code != ARBOR_STATUS_NO_SPACE || matched || count != 0u) {
        return 8;
    }

    arbor_runtime_storage storage;
    status = arbor_runtime_storage_prepare(
        &storage,
        (arbor_mut_span){NULL, 16u},
        (arbor_mut_span){NULL, 0u},
        (arbor_mut_span){NULL, 0u});
    if (status.code != ARBOR_STATUS_INVALID_ARGUMENT) {
        return 9;
    }

    status = arbor_status_from_native(-12345);
    if (status.code != ARBOR_STATUS_NATIVE_ERROR || !arbor_status_is_error(status)) {
        return 10;
    }

    const uint8_t a[] = {1u, 2u, 3u};
    const uint8_t b[] = {1u, 2u, 4u};
    if (arbor_secure_equal(a, b, (uint64_t)sizeof(a))) {
        return 11;
    }

    return 0;
}
