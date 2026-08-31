#include <arborcore/view0_conformance/native.h>

#include <stdio.h>
#include <string.h>

int main(void)
{
    static const char html[] = "<!doctype html><html><head><title>x</title></head><body><div itemtype=Thing></div></body></html>";
    const arbor_view0_native_v1n3_options options = {
        ARBOR_VIEW0_NATIVE_V1N3_OPTIONS_ABI_V1, 0u, NULL, 0u};
    arbor_view0_native_result result;
    (void)memset(&result, 0xa5, sizeof(result));
    const arbor_view0_native_result before = result;
    if (arbor_view0_native_check_configured(
            (arbor_span){(const uint8_t *)html, sizeof(html) - 1u}, &options,
            NULL, 0u, &result).native == 0 ||
        memcmp(&before, &result, sizeof(result)) != 0) return 1;
    puts("VIEW0_V1N3_G14_GLOBAL_FAILURE_ATOMICITY=PASS");
    return 0;
}
