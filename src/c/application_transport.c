#include <errno.h>
#include <stdint.h>

#include <arborcore/application_transport.h>

/* MVC-layer internal Assembly entry. It is intentionally outside Assembly ABI v1. */
typedef int64_t (*arbor_application_transport_dispatch_fn)(
    const arbor_asm_http_request *request,
    arbor_asm_buffer *output,
    arbor_asm_arena *arena,
    void *context,
    uint64_t *keep_alive_out);

extern arbor_asm_result_u64 application_transport_handle_once(
    arbor_asm_connection *connection,
    arbor_asm_http_request *request,
    arbor_application_transport_dispatch_fn dispatch,
    void *context,
    int64_t epoll_fd);

static arbor_status invalid_argument_status(void)
{
    return arbor_status_from_native(-EINVAL);
}

static bool spans_overlap(
    const void *left,
    uint64_t left_length,
    const void *right,
    uint64_t right_length)
{
    if (left_length == 0u || right_length == 0u) {
        return false;
    }
    if (left == NULL || right == NULL) {
        return false;
    }

    arbor_asm_result_u64 overlap = range_overlaps(
        (uint64_t)(uintptr_t)left,
        left_length,
        (uint64_t)(uintptr_t)right,
        right_length);
    return overlap.status == 0 && overlap.value != 0u;
}

static bool span_representable(const void *data, uint64_t length)
{
    if (length == 0u) {
        return true;
    }
    if (data == NULL) {
        return false;
    }
    arbor_asm_result_u64 end = range_end_checked(
        (uint64_t)(uintptr_t)data,
        length);
    return end.status == 0;
}

static bool pointer_anchor_in_span(const void *pointer, const void *span, uint64_t span_length)
{
    return pointer != NULL && spans_overlap(pointer, 1u, span, span_length);
}

static arbor_status application_transport_storage_validate(
    const arbor_runtime_storage *storage,
    const arbor_application_capabilities *application,
    const uint64_t *completed_request_count)
{
    if (storage == NULL || application == NULL) {
        return invalid_argument_status();
    }

    if (storage->input.length > storage->input.capacity ||
        storage->output.length > storage->output.capacity ||
        storage->arena.offset > storage->arena.capacity ||
        !span_representable(storage->input.data, storage->input.capacity) ||
        !span_representable(storage->output.data, storage->output.capacity) ||
        !span_representable(storage->arena.base, storage->arena.capacity)) {
        return invalid_argument_status();
    }

    if (storage->connection.input_buffer != &storage->input ||
        storage->connection.output_buffer != &storage->output ||
        storage->connection.arena != &storage->arena) {
        return invalid_argument_status();
    }

    if (spans_overlap(
            storage->input.data, storage->input.capacity,
            storage->output.data, storage->output.capacity) ||
        spans_overlap(
            storage->input.data, storage->input.capacity,
            storage->arena.base, storage->arena.capacity) ||
        spans_overlap(
            storage->output.data, storage->output.capacity,
            storage->arena.base, storage->arena.capacity) ||
        spans_overlap(storage, sizeof(*storage), storage->input.data, storage->input.capacity) ||
        spans_overlap(storage, sizeof(*storage), storage->output.data, storage->output.capacity) ||
        spans_overlap(storage, sizeof(*storage), storage->arena.base, storage->arena.capacity)) {
        return invalid_argument_status();
    }

    if (spans_overlap(application, sizeof(*application), storage, sizeof(*storage)) ||
        spans_overlap(
            application, sizeof(*application),
            storage->input.data, storage->input.capacity) ||
        spans_overlap(
            application, sizeof(*application),
            storage->output.data, storage->output.capacity) ||
        spans_overlap(
            application, sizeof(*application),
            storage->arena.base, storage->arena.capacity) ||
        pointer_anchor_in_span(application->application_context, storage, sizeof(*storage)) ||
        pointer_anchor_in_span(
            application->application_context, storage->input.data, storage->input.capacity) ||
        pointer_anchor_in_span(
            application->application_context, storage->output.data, storage->output.capacity) ||
        pointer_anchor_in_span(
            application->application_context, storage->arena.base, storage->arena.capacity)) {
        return invalid_argument_status();
    }

    if (completed_request_count != NULL &&
        (spans_overlap(completed_request_count, sizeof(*completed_request_count),
                       storage, sizeof(*storage)) ||
         spans_overlap(completed_request_count, sizeof(*completed_request_count),
                       storage->input.data, storage->input.capacity) ||
         spans_overlap(completed_request_count, sizeof(*completed_request_count),
                       storage->output.data, storage->output.capacity) ||
         spans_overlap(completed_request_count, sizeof(*completed_request_count),
                       storage->arena.base, storage->arena.capacity) ||
         spans_overlap(completed_request_count, sizeof(*completed_request_count),
                       application, sizeof(*application)) ||
         (application->application_context != NULL &&
          spans_overlap(completed_request_count, sizeof(*completed_request_count),
                        application->application_context, 1u)))) {
        return invalid_argument_status();
    }

    return arbor_status_from_native(0);
}

static int64_t application_transport_dispatch(
    const arbor_asm_http_request *request,
    arbor_asm_buffer *output,
    arbor_asm_arena *arena,
    void *context,
    uint64_t *keep_alive_out)
{
    if (request == NULL || output == NULL || arena == NULL || context == NULL ||
        keep_alive_out == NULL) {
        return -EINVAL;
    }
    if (output->length != 0u || output->length > output->capacity ||
        (output->capacity != 0u && output->data == NULL)) {
        return -EINVAL;
    }
    if (output->capacity != 0u) {
        arbor_asm_result_u64 output_end = range_end_checked(
            (uint64_t)(uintptr_t)output->data,
            output->capacity);
        if (output_end.status != 0) {
            return output_end.status;
        }
    }

    const arbor_application_capabilities *application =
        (const arbor_application_capabilities *)context;

    arbor_request_view request_view = {0};
    request_view.native = *request;

    arbor_asm_result_u64 target_result = request_target_from_request(
        request,
        &request_view.target);
    if (target_result.status < 0) {
        return target_result.status;
    }

    arbor_request_scope scope = {0};
    arbor_status status = arbor_request_scope_make(
        &request_view,
        NULL,
        0u,
        arena,
        &scope);
    if (status.native != 0) {
        return status.native;
    }

    arbor_response_plan response = {0u, NULL, 0u, 0u};
    status = arbor_application_invoke(application, &scope, &response);
    if (status.native != 0) {
        return status.native;
    }

    if (response.body_length != 0u) {
        arbor_asm_result_u64 body_end = range_end_checked(
            (uint64_t)(uintptr_t)response.body_data,
            response.body_length);
        if (body_end.status != 0) {
            return body_end.status;
        }
    }
    if (spans_overlap(
            response.body_data,
            response.body_length,
            output,
            (uint64_t)sizeof(*output)) ||
        spans_overlap(
            response.body_data,
            response.body_length,
            output->data,
            output->capacity)) {
        return -EINVAL;
    }

    uint64_t serialized = 0u;
    status = arbor_response_plan_serialize(output, &response, &serialized);
    if (status.native != 0) {
        return status.native;
    }
    if (serialized == 0u || output->length != serialized) {
        return -EIO;
    }

    *keep_alive_out =
        (response.flags & ARBOR_RESPONSE_PLAN_FLAG_KEEP_ALIVE) != 0u ? 1u : 0u;
    return 0;
}

arbor_status arbor_application_server_step(
    arbor_runtime_storage *storage,
    const arbor_application_capabilities *application,
    int64_t epoll_fd,
    uint64_t *completed_request_count)
{
    if (storage == NULL) {
        return invalid_argument_status();
    }

    arbor_status status = arbor_application_capabilities_validate(application);
    if (status.native != 0) {
        return status;
    }
    status = application_transport_storage_validate(
        storage,
        application,
        completed_request_count);
    if (status.native != 0) {
        return status;
    }

    if (completed_request_count != NULL) {
        *completed_request_count = 0u;
    }

    arbor_asm_result_u64 result = application_transport_handle_once(
        &storage->connection,
        &storage->request,
        application_transport_dispatch,
        (void *)application,
        epoll_fd);

    if (result.status >= 0 && completed_request_count != NULL) {
        *completed_request_count = result.value;
    }
    return arbor_status_from_native(result.status);
}
