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

arbor_status arbor_route_init(arbor_route *route, arbor_span method, arbor_span pattern, arbor_route_handler handler)
{
    if (route == NULL || handler == NULL) {
        return invalid_argument_status();
    }
    if ((method.length != 0u && method.data == NULL) ||
        (pattern.length != 0u && pattern.data == NULL)) {
        return invalid_argument_status();
    }

    route->method_ptr = method.data;
    route->method_len = method.length;
    route->pattern_ptr = pattern.data;
    route->pattern_len = pattern.length;
    route->handler = handler;
    return ok_status();
}

arbor_status arbor_route_match(arbor_span pattern, arbor_span path, arbor_route_param *params, uint64_t params_capacity, bool *matched, uint64_t *parameter_count)
{
    if (matched != NULL) {
        *matched = false;
    }
    if (parameter_count != NULL) {
        *parameter_count = 0u;
    }

    arbor_asm_match_result result = route_pattern_match(
        pattern.data,
        pattern.length,
        path.data,
        path.length,
        params,
        params_capacity);

    if (result.match < 0) {
        return arbor_status_from_native(result.match);
    }

    if (matched != NULL) {
        *matched = result.match == 1;
    }
    if (parameter_count != NULL && result.match == 1) {
        *parameter_count = result.parameter_count;
    }
    return ok_status();
}

arbor_status arbor_route_dispatch(const arbor_route *routes, uint64_t route_count, const arbor_asm_http_request *request, void *context, arbor_route_param *params, uint64_t params_capacity, int64_t *handler_result)
{
    if (handler_result != NULL) {
        *handler_result = 0;
    }
    if (request == NULL) {
        return invalid_argument_status();
    }

    int64_t result = route_pattern_dispatch(
        routes,
        route_count,
        request,
        context,
        params,
        params_capacity);

    if (result < 0) {
        return arbor_status_from_native(result);
    }
    if (handler_result != NULL) {
        *handler_result = result;
    }
    return ok_status();
}
