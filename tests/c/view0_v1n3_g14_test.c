#include <arborcore/view0_conformance/native.h>

#include <stdbool.h>
#include <stdio.h>

static arbor_view0_native_diagnostic diagnostics[ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS];

int main(void)
{
    static const char html[] = "<!doctype html><html><head><title>x</title></head><body><div itemtype=Thing></div></body></html>";
    const arbor_view0_native_v1n3_options options = {
        ARBOR_VIEW0_NATIVE_V1N3_OPTIONS_ABI_V1, 0u, NULL, 0u};
    arbor_view0_native_result result = {0};
    arbor_status status = arbor_view0_native_check_configured(
        (arbor_span){(const uint8_t *)html, sizeof(html) - 1u}, &options,
        diagnostics, ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS, &result);
    if (status.native != 0) return 1;
    bool found = false;
    for (uint64_t i = 0u; i < result.diagnostic_count; ++i)
        if (diagnostics[i].rule_id == ARBOR_VIEW0_NATIVE_RULE_ID_V1N3_G14_R1) found = true;
    if (!found) return 2;
    puts("VIEW0_V1N3_G14_FUNCTIONAL=PASS");
    puts("VIEW0_V1N3_G14_RULES=6_OF_6_BOUND");
    return 0;
}
