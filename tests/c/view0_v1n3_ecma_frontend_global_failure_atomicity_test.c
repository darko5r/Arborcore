#include <arborcore/view0_conformance/native.h>

#include <stdio.h>
#include <string.h>

int main(void)
{
    const char source[] = "return 1";
    arbor_view0_native_v1n3_ecma_result result;
    (void)memset(&result, 0xa5, sizeof(result));
    const arbor_view0_native_v1n3_ecma_result before = result;
    if (arbor_view0_native_v1n3_ecma_parse(99u,
            (arbor_span){(const uint8_t *)source, sizeof(source) - 1u}, NULL, &result).native == 0 ||
        memcmp(&before, &result, sizeof(result)) != 0) return 1;
    puts("VIEW0_V1N3_ECMA_GLOBAL_FAILURE_ATOMICITY=PASS");
    return 0;
}
