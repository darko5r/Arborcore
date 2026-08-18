#include <errno.h>
#include <stddef.h>

#include <arborcore/application.h>

_Static_assert(sizeof(arbor_request_scope) == 40u, "request scope ABI drift");
_Static_assert(offsetof(arbor_request_scope, request) == 0u, "request scope request offset drift");
_Static_assert(offsetof(arbor_request_scope, target) == 8u, "request scope target offset drift");
_Static_assert(offsetof(arbor_request_scope, params) == 16u, "request scope params offset drift");
_Static_assert(offsetof(arbor_request_scope, parameter_count) == 24u, "request scope count offset drift");
_Static_assert(offsetof(arbor_request_scope, arena) == 32u, "request scope arena offset drift");

_Static_assert(sizeof(arbor_response_plan) == 32u, "response plan ABI drift");
_Static_assert(offsetof(arbor_response_plan, status) == 0u, "response plan status offset drift");
_Static_assert(offsetof(arbor_response_plan, body_data) == 8u, "response plan body pointer offset drift");
_Static_assert(offsetof(arbor_response_plan, body_length) == 16u, "response plan body length offset drift");
_Static_assert(offsetof(arbor_response_plan, flags) == 24u, "response plan flags offset drift");

_Static_assert(sizeof(arbor_application_capabilities) == 32u, "capabilities ABI drift");
_Static_assert(offsetof(arbor_application_capabilities, abi_version) == 0u, "capabilities version offset drift");
_Static_assert(offsetof(arbor_application_capabilities, struct_size) == 4u, "capabilities size offset drift");
_Static_assert(offsetof(arbor_application_capabilities, flags) == 8u, "capabilities flags offset drift");
_Static_assert(offsetof(arbor_application_capabilities, application_context) == 16u, "capabilities context offset drift");
_Static_assert(offsetof(arbor_application_capabilities, request_dispatch) == 24u, "capabilities dispatch offset drift");

static arbor_status invalid_argument_status(void)
{
    return arbor_status_from_native(-EINVAL);
}

static arbor_status ok_status(void)
{
    return arbor_status_from_native(0);
}

static bool response_status_supported(uint64_t status)
{
    return status >= UINT64_C(200) && status <= UINT64_C(599);
}

static bool response_status_legacy_serializer_supported(uint64_t status)
{
    return status == UINT64_C(200) ||
           status == UINT64_C(201) ||
           status == UINT64_C(204) ||
           status == UINT64_C(400) ||
           status == UINT64_C(404) ||
           status == UINT64_C(500);
}

arbor_status arbor_request_scope_validate(const arbor_request_scope *scope)
{
    if (scope == NULL || scope->request == NULL || scope->target == NULL || scope->arena == NULL) {
        return invalid_argument_status();
    }
    if (scope->parameter_count != 0u && scope->params == NULL) {
        return invalid_argument_status();
    }
    if (scope->arena->offset > scope->arena->capacity) {
        return invalid_argument_status();
    }
    if (scope->arena->capacity != 0u && scope->arena->base == NULL) {
        return invalid_argument_status();
    }
    return ok_status();
}

arbor_status arbor_request_scope_make(
    const arbor_request_view *request,
    const arbor_route_param *params,
    uint64_t parameter_count,
    arbor_asm_arena *arena,
    arbor_request_scope *out)
{
    if (request == NULL || out == NULL) {
        return invalid_argument_status();
    }

    arbor_request_scope candidate = {
        &request->native,
        &request->target,
        params,
        parameter_count,
        arena
    };
    arbor_status status = arbor_request_scope_validate(&candidate);
    if (status.native != 0) {
        return status;
    }

    *out = candidate;
    return ok_status();
}

arbor_status arbor_response_plan_validate(const arbor_response_plan *plan)
{
    if (plan == NULL) {
        return invalid_argument_status();
    }
    if (!response_status_supported(plan->status)) {
        return invalid_argument_status();
    }
    if ((plan->flags & ~ARBOR_RESPONSE_PLAN_KNOWN_FLAGS) != 0u) {
        return invalid_argument_status();
    }
    if (plan->body_length != 0u && plan->body_data == NULL) {
        return invalid_argument_status();
    }
    if ((plan->status == UINT64_C(204) ||
         plan->status == UINT64_C(205) ||
         plan->status == UINT64_C(304)) &&
        plan->body_length != 0u) {
        return invalid_argument_status();
    }
    return ok_status();
}

arbor_status arbor_response_plan_make(
    uint64_t status,
    arbor_span body,
    uint64_t flags,
    arbor_response_plan *out)
{
    if (out == NULL) {
        return invalid_argument_status();
    }

    arbor_response_plan candidate = {
        status,
        body.data,
        body.length,
        flags
    };
    arbor_status result = arbor_response_plan_validate(&candidate);
    if (result.native != 0) {
        return result;
    }

    *out = candidate;
    return ok_status();
}

arbor_status arbor_response_plan_serialize(
    arbor_asm_buffer *buffer,
    const arbor_response_plan *plan,
    uint64_t *bytes_written)
{
    if (bytes_written != NULL) {
        *bytes_written = 0u;
    }

    arbor_status valid = arbor_response_plan_validate(plan);
    if (valid.native != 0) {
        return valid;
    }
    if (!response_status_legacy_serializer_supported(plan->status)) {
        return invalid_argument_status();
    }

    return arbor_response_serialize(
        buffer,
        plan->status,
        (arbor_span){plan->body_data, plan->body_length},
        (plan->flags & ARBOR_RESPONSE_PLAN_FLAG_KEEP_ALIVE) != 0u,
        bytes_written);
}

arbor_status arbor_application_capabilities_validate(
    const arbor_application_capabilities *capabilities)
{
    if (capabilities == NULL) {
        return invalid_argument_status();
    }
    if (capabilities->abi_version != ARBOR_APPLICATION_CAPABILITIES_ABI_VERSION) {
        return invalid_argument_status();
    }
    if (capabilities->struct_size < (uint32_t)sizeof(arbor_application_capabilities)) {
        return invalid_argument_status();
    }
    if (capabilities->flags != ARBOR_APPLICATION_CAPABILITIES_FLAG_NONE) {
        return invalid_argument_status();
    }
    if (capabilities->request_dispatch == NULL) {
        return invalid_argument_status();
    }
    return ok_status();
}

arbor_status arbor_application_capabilities_make(
    arbor_application_request_dispatch_fn request_dispatch,
    void *application_context,
    arbor_application_capabilities *out)
{
    if (out == NULL) {
        return invalid_argument_status();
    }

    arbor_application_capabilities candidate = {
        ARBOR_APPLICATION_CAPABILITIES_ABI_VERSION,
        (uint32_t)sizeof(arbor_application_capabilities),
        ARBOR_APPLICATION_CAPABILITIES_FLAG_NONE,
        application_context,
        request_dispatch
    };
    arbor_status status = arbor_application_capabilities_validate(&candidate);
    if (status.native != 0) {
        return status;
    }

    *out = candidate;
    return ok_status();
}

arbor_status arbor_application_invoke(
    const arbor_application_capabilities *capabilities,
    const arbor_request_scope *scope,
    arbor_response_plan *response_out)
{
    if (response_out == NULL) {
        return invalid_argument_status();
    }

    arbor_status status = arbor_application_capabilities_validate(capabilities);
    if (status.native != 0) {
        return status;
    }
    status = arbor_request_scope_validate(scope);
    if (status.native != 0) {
        return status;
    }

    arbor_response_plan candidate = {0u, NULL, 0u, 0u};
    int64_t native_status = capabilities->request_dispatch(
        scope,
        capabilities->application_context,
        &candidate);

    if (native_status < 0) {
        return arbor_status_from_native(native_status);
    }
    if (native_status > 0) {
        return invalid_argument_status();
    }

    status = arbor_response_plan_validate(&candidate);
    if (status.native != 0) {
        return status;
    }

    *response_out = candidate;
    return ok_status();
}
