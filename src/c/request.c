#include <errno.h>

#include <arborcore/arborcore.h>

static arbor_status invalid_argument_status(void)
{
    return arbor_status_from_native(-EINVAL);
}

static arbor_status ok_status(void)
{
    return (arbor_status){ARBOR_STATUS_OK, 0};
}

arbor_status arbor_request_parse(arbor_span bytes, arbor_request_view *out, uint64_t *required_length)
{
    if (required_length != NULL) {
        *required_length = 0u;
    }

    if (out == NULL) {
        return invalid_argument_status();
    }

    arbor_asm_result_u64 parsed = http_parse_request(bytes.data, bytes.length, &out->native);
    if (required_length != NULL) {
        *required_length = parsed.value;
    }
    if (parsed.status < 0) {
        (void)memory_zero(&out->target, (uint64_t)sizeof(out->target));
        return arbor_status_from_native(parsed.status);
    }

    arbor_asm_result_u64 target = request_target_from_request(&out->native, &out->target);
    if (target.status < 0) {
        (void)memory_zero(out, (uint64_t)sizeof(*out));
        if (required_length != NULL) {
            *required_length = 0u;
        }
        return arbor_status_from_native(target.status);
    }

    if (required_length != NULL) {
        *required_length = out->native.message_length;
    }
    return ok_status();
}

arbor_status arbor_response_serialize(arbor_asm_buffer *buffer, uint64_t status, arbor_span body, bool keep_alive, uint64_t *bytes_written)
{
    if (bytes_written != NULL) {
        *bytes_written = 0u;
    }

    arbor_asm_result_u64 result = http_response_serialize(
        buffer,
        status,
        body.data,
        body.length,
        keep_alive ? 1u : 0u);

    if (result.status == 0) {
        if (bytes_written != NULL) {
            *bytes_written = result.value;
        }
        return ok_status();
    }
    return arbor_status_from_native(result.status);
}
