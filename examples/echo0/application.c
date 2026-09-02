#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include "echo0.h"

static bool echo0_span_valid(arbor_span value)
{
    if (value.length == 0u) {
        return value.data == NULL;
    }
    if (value.data == NULL) {
        return false;
    }
    return range_end_checked(
               (uint64_t)(uintptr_t)value.data,
               value.length).status == 0;
}

arbor_status echo0_service_prepare(echo0_service *service)
{
    if (service == NULL) {
        return arbor_status_from_native(-EINVAL);
    }
    *service = (echo0_service){ECHO0_SERVICE_PREPARED_GUARD};
    return arbor_status_from_native(0);
}

arbor_status echo0_service_execute(
    const echo0_service *service,
    echo0_action action,
    arbor_span value,
    echo0_service_result *result_out)
{
    if (service == NULL || result_out == NULL ||
        service->prepared_guard != ECHO0_SERVICE_PREPARED_GUARD ||
        !echo0_span_valid(value)) {
        return arbor_status_from_native(-EINVAL);
    }

    echo0_service_result candidate = {0};
    switch (action) {
    case ECHO0_ACTION_SHOW_PAGE:
        if (value.length == 0u) {
            return arbor_status_from_native(-EINVAL);
        }
        candidate.outcome_code = (uint32_t)ECHO0_OUTCOME_PAGE;
        candidate.value = value;
        break;
    case ECHO0_ACTION_REDIRECT:
        if (value.data != NULL || value.length != 0u) {
            return arbor_status_from_native(-EINVAL);
        }
        candidate.outcome_code = (uint32_t)ECHO0_OUTCOME_REDIRECT;
        break;
    default:
        return arbor_status_from_native(-EINVAL);
    }

    *result_out = candidate;
    return arbor_status_from_native(0);
}
