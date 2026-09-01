#include <errno.h>
#include <stdint.h>

#include "hello0.h"

static const uint8_t hello0_page_message[] = {
    'A', 'r', 'b', 'o', 'r', 'c', 'o', 'r', 'e', ' ',
    's', 'a', 'f', 'e', 'l', 'y', ' ',
    'r', 'e', 'n', 'd', 'e', 'r', 's', ' ',
    '<', 'd', 'y', 'n', 'a', 'm', 'i', 'c', ' ', 'd', 'a', 't', 'a', '>', ' ',
    '&', ' ', 'U', 'T', 'F', '-', '8', ':', ' ',
    'O', 'l', UINT8_C(0xc3), UINT8_C(0xa1), ' ',
    UINT8_C(0xf0), UINT8_C(0x9f), UINT8_C(0x98), UINT8_C(0x80)
};

arbor_status hello0_service_prepare(hello0_service *service)
{
    if (service == NULL) {
        return arbor_status_from_native(-EINVAL);
    }
    *service = (hello0_service){
        .page_message = {
            hello0_page_message,
            (uint64_t)sizeof(hello0_page_message)
        }
    };
    return arbor_status_from_native(0);
}

arbor_status hello0_service_execute(
    const hello0_service *service,
    hello0_action action,
    hello0_service_result *result_out)
{
    if (service == NULL || result_out == NULL ||
        service->page_message.data == NULL || service->page_message.length == 0u) {
        return arbor_status_from_native(-EINVAL);
    }

    hello0_service_result candidate = {0};
    switch (action) {
    case HELLO0_ACTION_SHOW_PAGE:
        candidate.outcome_code = (uint32_t)HELLO0_OUTCOME_PAGE;
        candidate.message = service->page_message;
        break;
    case HELLO0_ACTION_REDIRECT:
        candidate.outcome_code = (uint32_t)HELLO0_OUTCOME_REDIRECT;
        break;
    default:
        return arbor_status_from_native(-EINVAL);
    }

    *result_out = candidate;
    return arbor_status_from_native(0);
}
