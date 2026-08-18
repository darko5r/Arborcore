#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include <arborcore/http_mvc.h>

#define HTTP1_EXCHANGE_MAGIC UINT64_C(0x4854545031455843)
#define HTTP1_LOCATOR_MAGIC UINT64_C(0x48545450314c4f43)
#define HTTP1_INTERNAL_ABI_VERSION UINT32_C(1)
#define HTTP1_LOCATOR_GUARD_XOR UINT64_C(0x91d7a4c52be83610)

/* Reuse the frozen MVC0 internal transport engine; this is not Assembly ABI v1. */
typedef int64_t (*http1_transport_dispatch_fn)(
    const arbor_asm_http_request *request,
    arbor_asm_buffer *output,
    arbor_asm_arena *arena,
    void *context,
    uint64_t *keep_alive_out);

extern arbor_asm_result_u64 application_transport_handle_once(
    arbor_asm_connection *connection,
    arbor_asm_http_request *request,
    http1_transport_dispatch_fn dispatch,
    void *context,
    int64_t epoll_fd);

typedef struct http1_exchange {
    uint64_t magic;
    uint32_t abi_version;
    uint32_t struct_size;
    uint64_t field_capacity;
    uint64_t field_count;
    arbor_http_field *fields;
    const arbor_asm_http_request *request;
    const arbor_http_mvc_application *application;
} http1_exchange;

typedef struct http1_locator {
    uint64_t magic;
    uint32_t abi_version;
    uint32_t struct_size;
    http1_exchange *exchange;
    arbor_asm_arena *application_arena;
    uint64_t guard;
} http1_locator;

_Static_assert(sizeof(arbor_http_mvc_requirements) == 16u, "HTTP1 requirements ABI drift");
_Static_assert(offsetof(arbor_http_mvc_requirements, arena_prefix_bytes) == 0u,
               "HTTP1 requirements prefix offset drift");
_Static_assert(offsetof(arbor_http_mvc_requirements, response_field_capacity) == 8u,
               "HTTP1 requirements capacity offset drift");

_Static_assert(sizeof(arbor_http_mvc_application) == 80u, "HTTP1 application ABI drift");
_Static_assert(offsetof(arbor_http_mvc_application, abi_version) == 0u,
               "HTTP1 application version offset drift");
_Static_assert(offsetof(arbor_http_mvc_application, struct_size) == 4u,
               "HTTP1 application size offset drift");
_Static_assert(offsetof(arbor_http_mvc_application, flags) == 8u,
               "HTTP1 application flags offset drift");
_Static_assert(offsetof(arbor_http_mvc_application, mvc_application) == 16u,
               "HTTP1 application MVC offset drift");
_Static_assert(offsetof(arbor_http_mvc_application, mvc_capabilities) == 24u,
               "HTTP1 application capabilities offset drift");
_Static_assert(offsetof(arbor_http_mvc_application, response_field_capacity) == 56u,
               "HTTP1 application capacity offset drift");
_Static_assert(offsetof(arbor_http_mvc_application, arena_prefix_bytes) == 64u,
               "HTTP1 application prefix offset drift");
_Static_assert(offsetof(arbor_http_mvc_application, prepared_guard) == 72u,
               "HTTP1 application guard offset drift");

_Static_assert(sizeof(http1_exchange) == 56u, "HTTP1 exchange layout drift");
_Static_assert(sizeof(http1_locator) == 40u, "HTTP1 locator layout drift");

static arbor_status invalid_argument_status(void)
{
    return arbor_status_from_native(-EINVAL);
}

static arbor_status no_space_status(void)
{
    return arbor_status_from_native(-ENOSPC);
}

static arbor_status overflow_status(void)
{
    return arbor_status_from_native(-EOVERFLOW);
}

static arbor_status ok_status(void)
{
    return arbor_status_from_native(0);
}

static bool object_span_valid(const void *data, uint64_t length)
{
    if (length == 0u) {
        return true;
    }
    if (data == NULL) {
        return false;
    }
    arbor_asm_result_u64 end = range_end_checked((uint64_t)(uintptr_t)data, length);
    return end.status == 0;
}

static bool spans_overlap(
    const void *left,
    uint64_t left_length,
    const void *right,
    uint64_t right_length)
{
    if (left_length == 0u || right_length == 0u || left == NULL || right == NULL) {
        return false;
    }
    arbor_asm_result_u64 overlap = range_overlaps(
        (uint64_t)(uintptr_t)left,
        left_length,
        (uint64_t)(uintptr_t)right,
        right_length);
    return overlap.status == 0 && overlap.value != 0u;
}

static bool pointer_anchor_in_span(const void *pointer, const void *span, uint64_t span_length)
{
    return pointer != NULL && spans_overlap(pointer, UINT64_C(1), span, span_length);
}

static bool request_bytes_overlap_region(
    const arbor_asm_http_request *request,
    const void *region,
    uint64_t region_length)
{
    if (request == NULL || region == NULL || region_length == 0u) {
        return false;
    }
    return spans_overlap(region, region_length, request->method_ptr, request->method_len) ||
           spans_overlap(region, region_length, request->target_ptr, request->target_len) ||
           spans_overlap(region, region_length, request->version_ptr, request->version_len) ||
           spans_overlap(region, region_length, request->headers_ptr, request->headers_len) ||
           spans_overlap(region, region_length, request->body_ptr, request->body_available);
}

static arbor_status field_array_bytes(uint64_t capacity, uint64_t *bytes_out)
{
    if (bytes_out == NULL) {
        return invalid_argument_status();
    }
    if (capacity > UINT64_MAX / (uint64_t)sizeof(arbor_http_field)) {
        return overflow_status();
    }
    *bytes_out = capacity * (uint64_t)sizeof(arbor_http_field);
    return ok_status();
}

static arbor_status prefix_bytes_for_capacity(uint64_t capacity, uint64_t *bytes_out)
{
    if (bytes_out == NULL) {
        return invalid_argument_status();
    }
    uint64_t field_bytes = 0u;
    arbor_status status = field_array_bytes(capacity, &field_bytes);
    if (status.native != 0) {
        return status;
    }
    uint64_t total = (uint64_t)sizeof(http1_exchange);
    if (UINT64_MAX - total < field_bytes) {
        return overflow_status();
    }
    total += field_bytes;
    if (UINT64_MAX - total < (uint64_t)sizeof(http1_locator)) {
        return overflow_status();
    }
    total += (uint64_t)sizeof(http1_locator);
    /* Preserve the base arena's unaligned-backing guarantee. HTTP1 reserves
     * seven extra bytes, then aligns the application boundary down to 8. */
    if (UINT64_MAX - total < UINT64_C(7)) {
        return overflow_status();
    }
    total += UINT64_C(7);
    *bytes_out = total;
    return ok_status();
}

static uint64_t guard_mix_bytes(uint64_t state, const void *data, uint64_t length)
{
    const uint8_t *bytes = (const uint8_t *)data;
    for (uint64_t i = 0u; i < length; ++i) {
        state ^= (uint64_t)bytes[i];
        state *= UINT64_C(1099511628211);
    }
    return state;
}

static uint64_t application_guard_value(
    const arbor_mvc_application *mvc_application,
    const arbor_application_capabilities *capabilities,
    uint64_t field_capacity,
    uint64_t prefix_bytes)
{
    uint64_t state = UINT64_C(1469598103934665603);
    const uintptr_t mvc_pointer = (uintptr_t)mvc_application;
    state = guard_mix_bytes(state, &mvc_pointer, (uint64_t)sizeof(mvc_pointer));
    state = guard_mix_bytes(state, capabilities, (uint64_t)sizeof(*capabilities));
    state = guard_mix_bytes(state, &field_capacity, (uint64_t)sizeof(field_capacity));
    state = guard_mix_bytes(state, &prefix_bytes, (uint64_t)sizeof(prefix_bytes));
    return ~state;
}

static bool region_overlaps_catalog(
    const void *region,
    uint64_t region_length,
    const arbor_mvc_catalog *catalog)
{
    if (catalog == NULL || region == NULL || region_length == 0u) {
        return false;
    }
    if (spans_overlap(region, region_length, catalog, sizeof(*catalog))) {
        return true;
    }
    if (catalog->route_count > UINT64_MAX / (uint64_t)sizeof(arbor_mvc_route) ||
        catalog->middleware_count > UINT64_MAX / (uint64_t)sizeof(arbor_mvc_middleware_descriptor)) {
        return true;
    }
    const uint64_t route_bytes = catalog->route_count * (uint64_t)sizeof(arbor_mvc_route);
    const uint64_t middleware_bytes =
        catalog->middleware_count * (uint64_t)sizeof(arbor_mvc_middleware_descriptor);
    if (spans_overlap(region, region_length, catalog->routes, route_bytes) ||
        spans_overlap(region, region_length, catalog->middlewares, middleware_bytes)) {
        return true;
    }
    for (uint64_t i = 0u; i < catalog->route_count; ++i) {
        const arbor_mvc_route *route = &catalog->routes[i];
        if (spans_overlap(region, region_length, route->method_data, route->method_length) ||
            spans_overlap(region, region_length, route->pattern_data, route->pattern_length)) {
            return true;
        }
        if (route->middleware_count > UINT64_MAX / (uint64_t)sizeof(uint64_t)) {
            return true;
        }
        const uint64_t index_bytes = route->middleware_count * (uint64_t)sizeof(uint64_t);
        if (spans_overlap(region, region_length, route->middleware_indices, index_bytes)) {
            return true;
        }
    }
    return false;
}

arbor_status arbor_http_mvc_application_measure(
    uint64_t response_field_capacity,
    arbor_http_mvc_requirements *out)
{
    if (out == NULL || !object_span_valid(out, sizeof(*out))) {
        return invalid_argument_status();
    }
    uint64_t prefix = 0u;
    arbor_status status = prefix_bytes_for_capacity(response_field_capacity, &prefix);
    if (status.native != 0) {
        return status;
    }
    *out = (arbor_http_mvc_requirements){prefix, response_field_capacity};
    return ok_status();
}

static arbor_status application_runtime_validate(const arbor_http_mvc_application *application)
{
    if (application == NULL ||
        application->abi_version != ARBOR_HTTP_MVC_ABI_VERSION ||
        application->struct_size != (uint32_t)sizeof(arbor_http_mvc_application) ||
        application->flags != ARBOR_HTTP_MVC_APPLICATION_FLAGS_NONE ||
        application->mvc_application == NULL) {
        return invalid_argument_status();
    }
    uint64_t prefix = 0u;
    arbor_status status = prefix_bytes_for_capacity(application->response_field_capacity, &prefix);
    if (status.native != 0 || prefix != application->arena_prefix_bytes ||
        application->prepared_guard != application_guard_value(
            application->mvc_application,
            &application->mvc_capabilities,
            application->response_field_capacity,
            application->arena_prefix_bytes)) {
        return invalid_argument_status();
    }
    status = arbor_application_capabilities_validate(&application->mvc_capabilities);
    if (status.native != 0 ||
        application->mvc_capabilities.application_context != application->mvc_application) {
        return invalid_argument_status();
    }
    return ok_status();
}

arbor_status arbor_http_mvc_application_validate(
    const arbor_http_mvc_application *application)
{
    arbor_status status = application_runtime_validate(application);
    if (status.native != 0) {
        return status;
    }
    return arbor_mvc_application_validate(application->mvc_application);
}

arbor_status arbor_http_mvc_application_prepare(
    arbor_mvc_application *mvc_application,
    uint64_t response_field_capacity,
    arbor_http_mvc_application *out)
{
    if (mvc_application == NULL || out == NULL || !object_span_valid(out, sizeof(*out))) {
        return invalid_argument_status();
    }
    arbor_status status = arbor_mvc_application_validate(mvc_application);
    if (status.native != 0) {
        return status;
    }
    if (spans_overlap(out, sizeof(*out), mvc_application, sizeof(*mvc_application)) ||
        region_overlaps_catalog(out, sizeof(*out), mvc_application->catalog)) {
        return invalid_argument_status();
    }

    arbor_http_mvc_requirements requirements = {0u, 0u};
    status = arbor_http_mvc_application_measure(response_field_capacity, &requirements);
    if (status.native != 0) {
        return status;
    }

    arbor_application_capabilities capabilities = {0};
    status = arbor_mvc_application_capabilities_make(mvc_application, &capabilities);
    if (status.native != 0) {
        return status;
    }

    const arbor_http_mvc_application candidate = {
        ARBOR_HTTP_MVC_ABI_VERSION,
        (uint32_t)sizeof(arbor_http_mvc_application),
        ARBOR_HTTP_MVC_APPLICATION_FLAGS_NONE,
        mvc_application,
        capabilities,
        response_field_capacity,
        requirements.arena_prefix_bytes,
        application_guard_value(
            mvc_application, &capabilities, response_field_capacity, requirements.arena_prefix_bytes)
    };
    *out = candidate;
    return ok_status();
}

static uint64_t locator_guard_value(
    const http1_exchange *exchange,
    const arbor_asm_arena *application_arena,
    const arbor_http_mvc_application *application)
{
    return ~(((uint64_t)(uintptr_t)exchange) ^
             ((uint64_t)(uintptr_t)application_arena) ^
             ((uint64_t)(uintptr_t)application) ^
             HTTP1_LOCATOR_GUARD_XOR);
}

static arbor_status request_exchange(
    const arbor_mvc_request *request,
    http1_exchange **exchange_out,
    arbor_asm_arena **application_arena_out,
    http1_locator **locator_out)
{
    if (exchange_out == NULL || application_arena_out == NULL || locator_out == NULL ||
        arbor_mvc_request_validate(request).native != 0 || request->scope == NULL ||
        request->scope->arena == NULL || request->scope->arena->base == NULL) {
        return invalid_argument_status();
    }

    arbor_asm_arena *application_arena = request->scope->arena;
    const uintptr_t base = (uintptr_t)application_arena->base;
    if (base < (uintptr_t)sizeof(http1_locator)) {
        return invalid_argument_status();
    }
    http1_locator *locator =
        (http1_locator *)(void *)(base - (uintptr_t)sizeof(http1_locator));
    if (!object_span_valid(locator, sizeof(*locator)) ||
        locator->magic != HTTP1_LOCATOR_MAGIC ||
        locator->abi_version != HTTP1_INTERNAL_ABI_VERSION ||
        locator->struct_size != (uint32_t)sizeof(http1_locator) ||
        locator->application_arena != application_arena ||
        locator->exchange == NULL) {
        return invalid_argument_status();
    }

    http1_exchange *exchange = locator->exchange;
    if (!object_span_valid(exchange, sizeof(*exchange)) ||
        exchange->magic != HTTP1_EXCHANGE_MAGIC ||
        exchange->abi_version != HTTP1_INTERNAL_ABI_VERSION ||
        exchange->struct_size != (uint32_t)sizeof(http1_exchange) ||
        exchange->application == NULL ||
        locator->guard != locator_guard_value(
            exchange, application_arena, exchange->application) ||
        application_runtime_validate(exchange->application).native != 0 ||
        exchange->request != request->scope->request ||
        exchange->field_count > exchange->field_capacity) {
        return invalid_argument_status();
    }
    uint64_t field_bytes = 0u;
    arbor_status status = field_array_bytes(exchange->field_capacity, &field_bytes);
    if (status.native != 0 || !object_span_valid(exchange->fields, field_bytes)) {
        return invalid_argument_status();
    }

    *exchange_out = exchange;
    *application_arena_out = application_arena;
    *locator_out = locator;
    return ok_status();
}

static arbor_status exchange_prefix_region(
    const http1_exchange *exchange,
    const arbor_asm_arena *application_arena,
    const void **base_out,
    uint64_t *length_out)
{
    if (exchange == NULL || application_arena == NULL || base_out == NULL || length_out == NULL) {
        return invalid_argument_status();
    }
    const uintptr_t begin = (uintptr_t)exchange;
    const uintptr_t end = (uintptr_t)application_arena->base;
    if (end < begin) {
        return invalid_argument_status();
    }
    const uint64_t length = (uint64_t)(end - begin);
    if (!object_span_valid(exchange, length)) {
        return invalid_argument_status();
    }
    *base_out = exchange;
    *length_out = length;
    return ok_status();
}

static arbor_status response_field_allowed(const arbor_http_field *field)
{
    arbor_status status = arbor_http_field_validate(field);
    if (status.native != 0) {
        return status;
    }
    const arbor_http_response probe = {
        ARBOR_HTTP_RESPONSE_ABI_VERSION,
        (uint32_t)sizeof(arbor_http_response),
        ARBOR_HTTP_RESPONSE_FLAG_NONE,
        UINT64_C(200),
        field,
        UINT64_C(1),
        NULL,
        UINT64_C(0)
    };
    return arbor_http_response_validate(&probe);
}

arbor_status arbor_http_mvc_response_field_append(
    const arbor_mvc_request *request,
    arbor_span name,
    arbor_span value)
{
    http1_exchange *exchange = NULL;
    arbor_asm_arena *application_arena = NULL;
    http1_locator *locator = NULL;
    arbor_status status = request_exchange(
        request, &exchange, &application_arena, &locator);
    if (status.native != 0) {
        return status;
    }
    if (exchange == NULL || application_arena == NULL || locator == NULL) {
        return invalid_argument_status();
    }

    const arbor_http_field candidate = {
        name.data, name.length, value.data, value.length
    };
    status = response_field_allowed(&candidate);
    if (status.native != 0) {
        return status;
    }
    if (exchange->field_count >= exchange->field_capacity) {
        return no_space_status();
    }
    const void *prefix_region = NULL;
    uint64_t prefix_region_length = 0u;
    status = exchange_prefix_region(
        exchange, application_arena, &prefix_region, &prefix_region_length);
    if (status.native != 0 ||
        spans_overlap(candidate.name_data, candidate.name_length,
                      prefix_region, prefix_region_length) ||
        spans_overlap(candidate.value_data, candidate.value_length,
                      prefix_region, prefix_region_length)) {
        return invalid_argument_status();
    }

    arbor_http_field *slot = &exchange->fields[exchange->field_count];
    if (spans_overlap(slot, sizeof(*slot), candidate.name_data, candidate.name_length) ||
        spans_overlap(slot, sizeof(*slot), candidate.value_data, candidate.value_length) ||
        spans_overlap(slot, sizeof(*slot), request, sizeof(*request)) ||
        spans_overlap(slot, sizeof(*slot), request->scope, sizeof(*request->scope)) ||
        spans_overlap(slot, sizeof(*slot), application_arena, sizeof(*application_arena)) ||
        spans_overlap(slot, sizeof(*slot), locator, sizeof(*locator)) ||
        spans_overlap(slot, sizeof(*slot), exchange, sizeof(*exchange))) {
        return invalid_argument_status();
    }

    *slot = candidate;
    exchange->field_count += UINT64_C(1);
    return ok_status();
}

arbor_status arbor_http_mvc_response_fields_mark(
    const arbor_mvc_request *request,
    uint64_t *mark_out)
{
    if (mark_out == NULL || !object_span_valid(mark_out, sizeof(*mark_out))) {
        return invalid_argument_status();
    }
    http1_exchange *exchange = NULL;
    arbor_asm_arena *application_arena = NULL;
    http1_locator *locator = NULL;
    arbor_status status = request_exchange(
        request, &exchange, &application_arena, &locator);
    if (status.native != 0) {
        return status;
    }
    if (exchange == NULL || application_arena == NULL || locator == NULL) {
        return invalid_argument_status();
    }
    const void *prefix_region = NULL;
    uint64_t prefix_region_length = 0u;
    status = exchange_prefix_region(
        exchange, application_arena, &prefix_region, &prefix_region_length);
    if (status.native != 0 ||
        exchange->application == NULL ||
        exchange->application->mvc_application == NULL ||
        spans_overlap(mark_out, sizeof(*mark_out), prefix_region, prefix_region_length) ||
        spans_overlap(mark_out, sizeof(*mark_out), application_arena, sizeof(*application_arena)) ||
        spans_overlap(mark_out, sizeof(*mark_out), application_arena->base, application_arena->capacity) ||
        spans_overlap(mark_out, sizeof(*mark_out), request, sizeof(*request)) ||
        spans_overlap(mark_out, sizeof(*mark_out), request->scope, sizeof(*request->scope)) ||
        spans_overlap(mark_out, sizeof(*mark_out),
                      exchange->application, sizeof(*exchange->application)) ||
        spans_overlap(mark_out, sizeof(*mark_out),
                      exchange->application->mvc_application,
                      sizeof(*exchange->application->mvc_application)) ||
        region_overlaps_catalog(
            mark_out, sizeof(*mark_out), exchange->application->mvc_application->catalog) ||
        request_bytes_overlap_region(request->scope->request, mark_out, sizeof(*mark_out))) {
        return invalid_argument_status();
    }
    *mark_out = exchange->field_count;
    return ok_status();
}

arbor_status arbor_http_mvc_response_fields_rewind(
    const arbor_mvc_request *request,
    uint64_t mark)
{
    http1_exchange *exchange = NULL;
    arbor_asm_arena *application_arena = NULL;
    http1_locator *locator = NULL;
    arbor_status status = request_exchange(
        request, &exchange, &application_arena, &locator);
    if (status.native != 0) {
        return status;
    }
    if (exchange == NULL || application_arena == NULL || locator == NULL) {
        return invalid_argument_status();
    }
    (void)application_arena;
    (void)locator;
    if (mark > exchange->field_count) {
        return invalid_argument_status();
    }
    for (uint64_t i = mark; i < exchange->field_count; ++i) {
        exchange->fields[i] = (arbor_http_field){NULL, 0u, NULL, 0u};
    }
    exchange->field_count = mark;
    return ok_status();
}

static arbor_status transport_storage_validate(
    const arbor_runtime_storage *storage,
    const arbor_http_mvc_application *application,
    const uint64_t *completed_request_count)
{
    if (storage == NULL || application == NULL) {
        return invalid_argument_status();
    }
    if (storage->input.length > storage->input.capacity ||
        storage->output.length > storage->output.capacity ||
        storage->arena.offset > storage->arena.capacity ||
        !object_span_valid(storage->input.data, storage->input.capacity) ||
        !object_span_valid(storage->output.data, storage->output.capacity) ||
        !object_span_valid(storage->arena.base, storage->arena.capacity)) {
        return invalid_argument_status();
    }
    if (storage->connection.input_buffer != &storage->input ||
        storage->connection.output_buffer != &storage->output ||
        storage->connection.arena != &storage->arena) {
        return invalid_argument_status();
    }
    if (spans_overlap(storage->input.data, storage->input.capacity,
                      storage->output.data, storage->output.capacity) ||
        spans_overlap(storage->input.data, storage->input.capacity,
                      storage->arena.base, storage->arena.capacity) ||
        spans_overlap(storage->output.data, storage->output.capacity,
                      storage->arena.base, storage->arena.capacity) ||
        spans_overlap(storage, sizeof(*storage), storage->input.data, storage->input.capacity) ||
        spans_overlap(storage, sizeof(*storage), storage->output.data, storage->output.capacity) ||
        spans_overlap(storage, sizeof(*storage), storage->arena.base, storage->arena.capacity)) {
        return invalid_argument_status();
    }
    if (spans_overlap(application, sizeof(*application), storage, sizeof(*storage)) ||
        spans_overlap(application, sizeof(*application), storage->input.data, storage->input.capacity) ||
        spans_overlap(application, sizeof(*application), storage->output.data, storage->output.capacity) ||
        spans_overlap(application, sizeof(*application), storage->arena.base, storage->arena.capacity) ||
        pointer_anchor_in_span(application->mvc_application, storage, sizeof(*storage)) ||
        pointer_anchor_in_span(application->mvc_application, storage->input.data, storage->input.capacity) ||
        pointer_anchor_in_span(application->mvc_application, storage->output.data, storage->output.capacity) ||
        pointer_anchor_in_span(application->mvc_application, storage->arena.base, storage->arena.capacity)) {
        return invalid_argument_status();
    }
    if (spans_overlap(application->mvc_application, sizeof(*application->mvc_application),
                      storage, sizeof(*storage)) ||
        spans_overlap(application->mvc_application, sizeof(*application->mvc_application),
                      storage->input.data, storage->input.capacity) ||
        spans_overlap(application->mvc_application, sizeof(*application->mvc_application),
                      storage->output.data, storage->output.capacity) ||
        spans_overlap(application->mvc_application, sizeof(*application->mvc_application),
                      storage->arena.base, storage->arena.capacity) ||
        region_overlaps_catalog(storage, sizeof(*storage), application->mvc_application->catalog) ||
        region_overlaps_catalog(storage->input.data, storage->input.capacity, application->mvc_application->catalog) ||
        region_overlaps_catalog(storage->output.data, storage->output.capacity, application->mvc_application->catalog) ||
        region_overlaps_catalog(storage->arena.base, storage->arena.capacity, application->mvc_application->catalog)) {
        return invalid_argument_status();
    }
    if (completed_request_count != NULL &&
        (spans_overlap(completed_request_count, sizeof(*completed_request_count), storage, sizeof(*storage)) ||
         spans_overlap(completed_request_count, sizeof(*completed_request_count), application, sizeof(*application)) ||
         spans_overlap(completed_request_count, sizeof(*completed_request_count), application->mvc_application,
                       sizeof(*application->mvc_application)) ||
         region_overlaps_catalog(completed_request_count, sizeof(*completed_request_count),
                                 application->mvc_application->catalog) ||
         spans_overlap(completed_request_count, sizeof(*completed_request_count), storage->input.data, storage->input.capacity) ||
         spans_overlap(completed_request_count, sizeof(*completed_request_count), storage->output.data, storage->output.capacity) ||
         spans_overlap(completed_request_count, sizeof(*completed_request_count), storage->arena.base, storage->arena.capacity))) {
        return invalid_argument_status();
    }
    return ok_status();
}

static int64_t serialize_bad_request(
    const arbor_asm_http_request *request,
    arbor_asm_buffer *output,
    uint64_t *keep_alive_out)
{
    arbor_http_response response = {0};
    arbor_status status = arbor_http_response_make(
        UINT64_C(400), NULL, UINT64_C(0), (arbor_span){NULL, UINT64_C(0)},
        ARBOR_HTTP_RESPONSE_FLAG_CLOSE, &response);
    if (status.native != 0) {
        return status.native;
    }
    uint64_t written = 0u;
    bool close_after = false;
    status = arbor_http_response_serialize(
        output, request, &response, &written, &close_after);
    if (status.native != 0) {
        return status.native;
    }
    if (written == 0u || output->length != written || !close_after) {
        return -EIO;
    }
    *keep_alive_out = UINT64_C(0);
    return 0;
}

static int64_t http1_dispatch(
    const arbor_asm_http_request *request,
    arbor_asm_buffer *output,
    arbor_asm_arena *arena,
    void *context,
    uint64_t *keep_alive_out)
{
    if (request == NULL || output == NULL || arena == NULL || context == NULL ||
        keep_alive_out == NULL || output->length != 0u || output->length > output->capacity ||
        (output->capacity != 0u && output->data == NULL) ||
        !object_span_valid(output->data, output->capacity)) {
        return -EINVAL;
    }
    const arbor_http_mvc_application *application =
        (const arbor_http_mvc_application *)context;
    arbor_status status = application_runtime_validate(application);
    if (status.native != 0) {
        return status.native;
    }
    if (arena->offset != 0u || arena->capacity < application->arena_prefix_bytes ||
        (arena->capacity != 0u && arena->base == NULL)) {
        return -EINVAL;
    }

    arbor_span host = {NULL, 0u};
    status = arbor_http_request_host_validate(request, &host);
    if (status.native != 0) {
        return status.native == -EINVAL ? serialize_bad_request(request, output, keep_alive_out)
                                       : status.native;
    }
    (void)host;

    bool request_close = false;
    status = arbor_http_request_connection_close(request, &request_close);
    if (status.native != 0) {
        return status.native == -EINVAL ? serialize_bad_request(request, output, keep_alive_out)
                                       : status.native;
    }

    uint64_t field_bytes = 0u;
    status = field_array_bytes(application->response_field_capacity, &field_bytes);
    if (status.native != 0) {
        return status.native;
    }
    uint64_t layout_bytes = (uint64_t)sizeof(http1_exchange);
    if (UINT64_MAX - layout_bytes < field_bytes) return -EOVERFLOW;
    layout_bytes += field_bytes;
    if (UINT64_MAX - layout_bytes < (uint64_t)sizeof(http1_locator)) return -EOVERFLOW;
    layout_bytes += (uint64_t)sizeof(http1_locator);

    const uintptr_t prefix_base_address = (uintptr_t)arena->base;
    arbor_asm_result_u64 reserved_end = range_end_checked(
        (uint64_t)prefix_base_address, application->arena_prefix_bytes);
    if (reserved_end.status != 0) return reserved_end.status;
    const uintptr_t application_address =
        (uintptr_t)(reserved_end.value & ~UINT64_C(7));
    if (application_address < prefix_base_address ||
        (uint64_t)(application_address - prefix_base_address) < layout_bytes) {
        return -EOVERFLOW;
    }
    const uintptr_t exchange_address = application_address - (uintptr_t)layout_bytes;
    if ((exchange_address & (uintptr_t)UINT64_C(7)) != 0u ||
        (application_address & (uintptr_t)UINT64_C(7)) != 0u) {
        return -EINVAL;
    }
    const uint64_t actual_prefix_bytes = (uint64_t)(application_address - prefix_base_address);
    if (actual_prefix_bytes > arena->capacity) return -ENOSPC;

    http1_exchange *exchange = (http1_exchange *)(void *)exchange_address;
    arbor_http_field *fields =
        (arbor_http_field *)(void *)(exchange_address + (uintptr_t)sizeof(*exchange));
    http1_locator *locator =
        (http1_locator *)(void *)(application_address - (uintptr_t)sizeof(http1_locator));
    uint8_t *application_base = (uint8_t *)(void *)application_address;
    const uint64_t application_capacity = arena->capacity - actual_prefix_bytes;
    arbor_asm_arena application_arena = {application_base, application_capacity, UINT64_C(0)};

    *exchange = (http1_exchange){
        HTTP1_EXCHANGE_MAGIC,
        HTTP1_INTERNAL_ABI_VERSION,
        (uint32_t)sizeof(http1_exchange),
        application->response_field_capacity,
        UINT64_C(0),
        fields,
        NULL,
        application
    };
    for (uint64_t i = 0u; i < application->response_field_capacity; ++i) {
        fields[i] = (arbor_http_field){NULL, 0u, NULL, 0u};
    }
    *locator = (http1_locator){
        HTTP1_LOCATOR_MAGIC,
        HTTP1_INTERNAL_ABI_VERSION,
        (uint32_t)sizeof(http1_locator),
        exchange,
        &application_arena,
        locator_guard_value(exchange, &application_arena, application)
    };

    arbor_request_view request_view = {0};
    request_view.native = *request;
    arbor_asm_result_u64 target = request_target_from_request(request, &request_view.target);
    if (target.status != 0) {
        return target.status;
    }
    arbor_request_scope scope = {0};
    status = arbor_request_scope_make(&request_view, NULL, UINT64_C(0), &application_arena, &scope);
    if (status.native != 0) {
        return status.native;
    }
    /* Bind sidecar access to the exact AF1 request-scope identity. AF1 borrows
     * request_view.native rather than the original transport request object. */
    exchange->request = scope.request;

    arbor_response_plan plan = {0u, NULL, 0u, 0u};
    status = arbor_application_invoke(&application->mvc_capabilities, &scope, &plan);
    if (status.native != 0) {
        return status.native;
    }
    if (exchange->field_count > exchange->field_capacity) {
        return -EINVAL;
    }

    uint64_t response_flags = ARBOR_HTTP_RESPONSE_FLAG_NONE;
    if ((plan.flags & ARBOR_RESPONSE_PLAN_FLAG_KEEP_ALIVE) == 0u) {
        response_flags |= ARBOR_HTTP_RESPONSE_FLAG_CLOSE;
    }

    arbor_http_response response = {0};
    status = arbor_http_response_make(
        plan.status,
        exchange->fields,
        exchange->field_count,
        (arbor_span){plan.body_data, plan.body_length},
        response_flags,
        &response);
    if (status.native != 0) {
        return status.native;
    }

    uint64_t written = 0u;
    bool close_after = false;
    status = arbor_http_response_serialize(
        output, request, &response, &written, &close_after);
    if (status.native != 0) {
        return status.native;
    }
    if (written == 0u || output->length != written) {
        return -EIO;
    }
    if (request_close && !close_after) {
        return -EIO;
    }
    *keep_alive_out = close_after ? UINT64_C(0) : UINT64_C(1);
    return 0;
}

arbor_status arbor_http_mvc_server_step(
    arbor_runtime_storage *storage,
    const arbor_http_mvc_application *application,
    int64_t epoll_fd,
    uint64_t *completed_request_count)
{
    arbor_status status = application_runtime_validate(application);
    if (status.native != 0) {
        return status;
    }
    status = transport_storage_validate(storage, application, completed_request_count);
    if (status.native != 0) {
        return status;
    }
    if (storage->arena.capacity < application->arena_prefix_bytes) {
        return no_space_status();
    }
    if (completed_request_count != NULL) {
        *completed_request_count = UINT64_C(0);
    }

    arbor_asm_result_u64 result = application_transport_handle_once(
        &storage->connection,
        &storage->request,
        http1_dispatch,
        (void *)application,
        epoll_fd);
    if (result.status >= 0 && completed_request_count != NULL) {
        *completed_request_count = result.value;
    }
    return arbor_status_from_native(result.status);
}
