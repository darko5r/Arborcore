#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "echo0.h"

static int fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    return 1;
}

static bool contains_bytes(
    const uint8_t *haystack,
    size_t haystack_length,
    const uint8_t *needle,
    size_t needle_length)
{
    if (needle_length == 0u) {
        return true;
    }
    if (needle_length > haystack_length) {
        return false;
    }
    for (size_t i = 0u; i + needle_length <= haystack_length; ++i) {
        if (memcmp(haystack + i, needle, needle_length) == 0) {
            return true;
        }
    }
    return false;
}

int main(void)
{
    static const uint8_t routed_value[] = {
        'A', '&', 'B', '-', 'O', 'l', UINT8_C(0xc3), UINT8_C(0xa1),
        UINT8_C(0xf0), UINT8_C(0x9f), UINT8_C(0x98), UINT8_C(0x80)
    };
    const arbor_span value = {routed_value, sizeof(routed_value)};

    echo0_service service = {0};
    if (echo0_service_prepare(&service).native != 0 ||
        service.prepared_guard != ECHO0_SERVICE_PREPARED_GUARD) {
        return fail("prepare typed ECHO0 service");
    }

    echo0_service_result page = {0};
    echo0_service_result redirect = {0};
    if (echo0_service_execute(
            &service,
            ECHO0_ACTION_SHOW_PAGE,
            value,
            &page).native != 0 ||
        page.outcome_code != (uint32_t)ECHO0_OUTCOME_PAGE ||
        page.value.data != routed_value ||
        page.value.length != (uint64_t)sizeof(routed_value)) {
        return fail("service publishes typed page outcome with borrowed value");
    }
    if (echo0_service_execute(
            &service,
            ECHO0_ACTION_REDIRECT,
            (arbor_span){NULL, 0u},
            &redirect).native != 0 ||
        redirect.outcome_code != (uint32_t)ECHO0_OUTCOME_REDIRECT ||
        redirect.value.data != NULL || redirect.value.length != 0u) {
        return fail("service publishes typed redirect outcome");
    }

    const echo0_service_result sentinel = {
        UINT32_MAX,
        UINT32_MAX,
        {routed_value, sizeof(routed_value)}
    };
    echo0_service_result unchanged = sentinel;
    if (echo0_service_execute(
            &service,
            (echo0_action)99,
            value,
            &unchanged).native != -EINVAL ||
        memcmp(&unchanged, &sentinel, sizeof(unchanged)) != 0) {
        return fail("service invalid action is failure-atomic");
    }
    unchanged = sentinel;
    if (echo0_service_execute(
            &service,
            ECHO0_ACTION_SHOW_PAGE,
            (arbor_span){NULL, 0u},
            &unchanged).native != -EINVAL ||
        memcmp(&unchanged, &sentinel, sizeof(unchanged)) != 0) {
        return fail("service rejects empty page value atomically");
    }
    unchanged = sentinel;
    if (echo0_service_execute(
            &service,
            ECHO0_ACTION_REDIRECT,
            value,
            &unchanged).native != -EINVAL ||
        memcmp(&unchanged, &sentinel, sizeof(unchanged)) != 0) {
        return fail("service rejects redirect value atomically");
    }

    uint8_t source[] =
        "<!doctype html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "  <meta charset=\"utf-8\">\n"
        "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
        "  <title>Arborcore ECHO0</title>\n"
        "</head>\n"
        "<body>\n"
        "  <main>\n"
        "    <h1>Arborcore ECHO0</h1>\n"
        "    <p>Echo: {{value}}</p>\n"
        "  </main>\n"
        "</body>\n"
        "</html>\n";
    echo0_web_application application = {0};
    arbor_status prepare_status = echo0_web_application_prepare(
            (arbor_span){source, (uint64_t)(sizeof(source) - 1u)},
            ECHO0_RESPONSE_FIELD_CAPACITY,
            &application);
    if (prepare_status.native != 0 ||
        echo0_web_application_validate(&application).native != 0) {
        fprintf(stderr, "ECHO0_PREPARE_STATUS=%" PRId64 "\n", prepare_status.native);
        return fail("prepare complete ECHO0 web composition");
    }
    static const uint8_t page_pattern[] = "/echo/:value";
    if (application.catalog.route_count != ECHO0_ROUTE_COUNT ||
        application.routes[0].pattern_length != sizeof(page_pattern) - 1u ||
        memcmp(
            application.routes[0].pattern_data,
            page_pattern,
            sizeof(page_pattern) - 1u) != 0 ||
        application.routes[1].pattern_length != 1u ||
        application.routes[1].pattern_data[0] != (uint8_t)'/' ||
        application.mvc_application.max_route_parameter_count != 1u ||
        application.http_application.response_field_capacity !=
            ECHO0_RESPONSE_FIELD_CAPACITY) {
        return fail("prepared route order, parameter bound and field capacity");
    }

    (void)memset(source, 'X', sizeof(source));
    if (echo0_web_application_validate(&application).native != 0) {
        return fail("prepared template is independent of source lifetime");
    }

    uint8_t arena_bytes[4096] = {0};
    arbor_asm_arena arena = {0};
    if (arena_init(&arena, arena_bytes, sizeof(arena_bytes)).status != 0) {
        return fail("initialize core-test render arena");
    }
    const arbor_span values[] = {page.value};
    arbor_span rendered = {NULL, 0u};
    if (arbor_view_html_template_render(
            &application.template_view,
            values,
            1u,
            &arena,
            &rendered).native != 0 ||
        arbor_view_utf8_validate(rendered).native != 0) {
        return fail("render prepared ECHO0 template after source overwrite");
    }
    static const uint8_t escaped[] =
        "<p>Echo: A&amp;B-Ol\xc3\xa1\xf0\x9f\x98\x80</p>";
    if (!contains_bytes(
            rendered.data,
            (size_t)rendered.length,
            escaped,
            sizeof(escaped) - 1u)) {
        return fail("route-value HTML-text escaping and UTF-8 representation");
    }

    echo0_web_application capacity_one = {0};
    static const uint8_t second_source[] = "<p>{{value}}</p>";
    if (echo0_web_application_prepare(
            (arbor_span){second_source, sizeof(second_source) - 1u},
            1u,
            &capacity_one).native != 0 ||
        capacity_one.http_application.response_field_capacity != 1u) {
        return fail("composition records caller-selected field capacity");
    }

    echo0_web_application failed;
    (void)memset(&failed, 0xa5, sizeof(failed));
    static const uint8_t invalid_source[] = "<p>{{missing}}</p>";
    if (echo0_web_application_prepare(
            (arbor_span){invalid_source, sizeof(invalid_source) - 1u},
            ECHO0_RESPONSE_FIELD_CAPACITY,
            &failed).native == 0 ||
        failed.prepared_guard != 0u ||
        echo0_web_application_validate(&failed).native != -EINVAL) {
        return fail("failed composition leaves no partially prepared object");
    }

    echo0_web_application copied = application;
    if (echo0_web_application_validate(&copied).native != -EINVAL) {
        return fail("prepared application cannot be moved by value");
    }
    application.prepared_guard = 0u;
    if (echo0_web_application_validate(&application).native != -EINVAL) {
        return fail("prepared application guard rejects corruption");
    }

    puts("PASS: ECHO0 typed service, owned template and prepared composition core");
    return 0;
}
