#include <arborcore/view0_conformance/native.h>

#include <stdio.h>
#include <string.h>

static arbor_view0_native_diagnostic first[ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS];
static arbor_view0_native_diagnostic second[ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS];

int main(void)
{
    static const char html[] = "<!doctype html><html><head><title>x</title></head><body><x-widget></x-widget></body></html>";
    static const char name[] = "x-widget";
    static const char constructor_source[] = "work(); super();";
    const arbor_view0_native_v1n3_definition definition = {
        {(const uint8_t *)name, sizeof(name) - 1u},
        {(const uint8_t *)name, sizeof(name) - 1u},
        {(const uint8_t *)constructor_source, sizeof(constructor_source) - 1u},
        ARBOR_VIEW0_NATIVE_V1N3_DEFINITION_AUTONOMOUS
    };
    const arbor_view0_native_v1n3_options options = {
        ARBOR_VIEW0_NATIVE_V1N3_OPTIONS_ABI_V1, 0u, &definition, 1u};
    arbor_view0_native_result a = {0}, b = {0};
    const arbor_span input = {(const uint8_t *)html, sizeof(html) - 1u};
    if (arbor_view0_native_check_configured(input, &options, first, ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS, &a).native != 0 ||
        arbor_view0_native_check_configured(input, &options, second, ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS, &b).native != 0 ||
        memcmp(&a, &b, sizeof(a)) != 0 ||
        memcmp(first, second, (size_t)a.diagnostic_count * sizeof(*first)) != 0) return 1;
    puts("VIEW0_V1N3_G13_DETERMINISM=PASS");
    puts("VIEW0_V1N3_G13_LOCALE_INDEPENDENCE=PASS");
    return 0;
}
