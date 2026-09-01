#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "hello0.h"

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
    hello0_service service = {0};
    if (hello0_service_prepare(&service).native != 0) {
        return fail("prepare typed HELLO0 service");
    }

    hello0_service_result page = {0};
    hello0_service_result redirect = {0};
    if (hello0_service_execute(
            &service,
            HELLO0_ACTION_SHOW_PAGE,
            &page).native != 0 ||
        page.outcome_code != (uint32_t)HELLO0_OUTCOME_PAGE ||
        page.message.data == NULL || page.message.length == 0u) {
        return fail("service publishes typed page outcome");
    }
    if (hello0_service_execute(
            &service,
            HELLO0_ACTION_REDIRECT,
            &redirect).native != 0 ||
        redirect.outcome_code != (uint32_t)HELLO0_OUTCOME_REDIRECT ||
        redirect.message.data != NULL || redirect.message.length != 0u) {
        return fail("service publishes typed redirect outcome");
    }

    const hello0_service_result sentinel = {
        UINT32_MAX,
        UINT32_MAX,
        {service.page_message.data, service.page_message.length}
    };
    hello0_service_result unchanged = sentinel;
    if (hello0_service_execute(
            &service,
            (hello0_action)99,
            &unchanged).native != -EINVAL ||
        memcmp(&unchanged, &sentinel, sizeof(unchanged)) != 0) {
        return fail("service invalid action is failure-atomic");
    }

    uint8_t source[] =
        "<!doctype html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "  <meta charset=\"utf-8\">\n"
        "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
        "  <title>Arborcore HELLO0</title>\n"
        "</head>\n"
        "<body>\n"
        "  <main>\n"
        "    <h1>Hello World</h1>\n"
        "    <p>{{message}}</p>\n"
        "  </main>\n"
        "</body>\n"
        "</html>\n";
    hello0_web_application application = {0};
    if (hello0_web_application_prepare(
            (arbor_span){source, (uint64_t)(sizeof(source) - 1u)},
            HELLO0_RESPONSE_FIELD_CAPACITY,
            &application).native != 0 ||
        hello0_web_application_validate(&application).native != 0) {
        return fail("prepare complete HELLO0 web composition");
    }
    if (application.catalog.route_count != HELLO0_ROUTE_COUNT ||
        application.routes[0].pattern_length != 6u ||
        memcmp(application.routes[0].pattern_data, "/hello", 6u) != 0 ||
        application.routes[1].pattern_length != 1u ||
        application.routes[1].pattern_data[0] != (uint8_t)'/' ||
        application.http_application.response_field_capacity !=
            HELLO0_RESPONSE_FIELD_CAPACITY) {
        return fail("prepared routes and response-field capacity");
    }

    memset(source, 'X', sizeof(source));
    if (hello0_web_application_validate(&application).native != 0) {
        return fail("prepared template is independent of source lifetime");
    }

    uint8_t arena_bytes[4096] = {0};
    arbor_asm_arena arena = {0};
    if (arena_init(&arena, arena_bytes, sizeof(arena_bytes)).status != 0) {
        return fail("initialize core-test render arena");
    }
    const arbor_span values[] = {page.message};
    arbor_span rendered = {NULL, 0u};
    if (arbor_view_html_template_render(
            &application.template_view,
            values,
            1u,
            &arena,
            &rendered).native != 0 ||
        arbor_view_utf8_validate(rendered).native != 0) {
        return fail("render prepared HELLO0 template after source overwrite");
    }
    static const uint8_t escaped[] =
        "Arborcore safely renders &lt;dynamic data&gt; &amp; UTF-8: "
        "Ol\xc3\xa1 \xf0\x9f\x98\x80";
    if (!contains_bytes(
            rendered.data,
            (size_t)rendered.length,
            escaped,
            sizeof(escaped) - 1u)) {
        return fail("HTML-text escaping and UTF-8 representation");
    }

    hello0_web_application copied = application;
    if (hello0_web_application_validate(&copied).native != -EINVAL) {
        return fail("prepared application cannot be moved by value");
    }
    application.prepared_guard = 0u;
    if (hello0_web_application_validate(&application).native != -EINVAL) {
        return fail("prepared application guard rejects corruption");
    }

    puts("PASS: HELLO0 typed service, owned template and prepared composition core");
    return 0;
}
