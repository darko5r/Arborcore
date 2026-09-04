#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "counter1.h"

static int fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    return 1;
}

static int call_service(
    counter1_application *application,
    uint64_t id,
    int increment,
    counter1_service_result *result_out)
{
    uint8_t arena_bytes[4096] = {0};
    arbor_asm_arena arena = {0};
    arbor_asm_result_u64 init = arena_init(&arena, arena_bytes, sizeof(arena_bytes));
    if (init.status != 0) {
        return (int)init.status;
    }
    const counter1_service_v1 *service =
        (const counter1_service_v1 *)application->service_binding.interface_table;
    counter1_service_request request = {&arena, id};
    return (int)(increment ?
        service->increment_counter(
            application->service_binding.provider_context, &request, result_out) :
        service->get_counter(
            application->service_binding.provider_context, &request, result_out));
}

int main(void)
{
    counter1_in_memory_repository repository = {0};
    counter1_repository_provider provider = {0};
    if (counter1_in_memory_repository_prepare(&repository, &provider).native != 0) {
        return fail("repository prepare");
    }

    counter1_application application = {0};
    if (counter1_application_prepare(&provider, &application).native != 0 ||
        counter1_application_validate(&application).native != 0) {
        return fail("AF2/AF3 application prepare");
    }
    if (application.catalog.module_count != 2u ||
        application.catalog.binding_count != 3u ||
        application.catalog.resolution_count != 2u ||
        application.runtime.managed_module_count != 1u ||
        application.service_module_state.prepared_guard == 0u) {
        (void)counter1_application_stop(&application);
        return fail("exact AF2/AF3 topology");
    }

    static const uint8_t template_source[] =
        "<!doctype html>\n"
        "<html><head><meta charset=\"utf-8\"><title>COUNTER1</title></head>"
        "<body><p>Counter {{id}} = {{value}}</p></body></html>\n";
    counter1_web_application web = {0};
    arbor_span template_span = {
        template_source,
        (uint64_t)(sizeof(template_source) - 1u)
    };
    if (counter1_web_application_prepare(
            &application,
            template_span,
            COUNTER1_RESPONSE_FIELD_CAPACITY,
            &web).native != 0 ||
        counter1_web_application_validate(&web).native != 0) {
        (void)counter1_application_stop(&application);
        return fail("MVC/VIEW/HTTP application prepare");
    }
    if (web.catalog.route_count != 2u || web.catalog.middleware_count != 0u ||
        web.mvc_application.max_route_parameter_count != 1u ||
        web.template_view.value_count != 2u) {
        (void)counter1_application_stop(&application);
        return fail("exact MVC/VIEW topology");
    }

    counter1_service_result result = {0};
    if (call_service(&application, 1u, 0, &result) != 0 ||
        result.outcome_code != COUNTER1_SERVICE_FOUND || result.value != 0u) {
        (void)counter1_application_stop(&application);
        return fail("GET 1 initial through AF3/AF4");
    }
    if (call_service(&application, 1u, 1, &result) != 0 ||
        result.outcome_code != COUNTER1_SERVICE_INCREMENTED || result.value != 1u ||
        repository.records[0].value != 1u) {
        (void)counter1_application_stop(&application);
        return fail("increment 1 through AF3/AF4");
    }
    if (call_service(&application, 1u, 0, &result) != 0 ||
        result.outcome_code != COUNTER1_SERVICE_FOUND || result.value != 1u) {
        (void)counter1_application_stop(&application);
        return fail("GET 1 persistence");
    }
    if (call_service(&application, 2u, 0, &result) != 0 ||
        result.outcome_code != COUNTER1_SERVICE_FOUND || result.value != 41u) {
        (void)counter1_application_stop(&application);
        return fail("GET 2 seed");
    }
    if (call_service(&application, 3u, 1, &result) != 0 ||
        result.outcome_code != COUNTER1_SERVICE_LIMIT_REACHED ||
        result.value != UINT64_MAX || repository.records[2].value != UINT64_MAX) {
        (void)counter1_application_stop(&application);
        return fail("increment 3 limit rollback");
    }
    if (call_service(&application, 999u, 0, &result) != 0 ||
        result.outcome_code != COUNTER1_SERVICE_NOT_FOUND ||
        result.id != 999u || result.value != 0u) {
        (void)counter1_application_stop(&application);
        return fail("GET 999 typed not-found");
    }

    if (counter1_application_stop(&application).native != 0) {
        return fail("AF3 runtime stop");
    }
    puts("PASS: COUNTER1 AF2/AF3/AF4/MVC/VIEW integration");
    return 0;
}
