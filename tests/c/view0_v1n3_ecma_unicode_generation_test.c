#include "ecma_unicode.h"
#include "ecma_unicode_tables.h"

#include <stdio.h>

int main(void)
{
    if (!arbor_view0_native_v1n3_identifier_start((uint32_t)'A') ||
        !arbor_view0_native_v1n3_identifier_continue((uint32_t)'7') ||
        arbor_view0_native_v1n3_unicode_table_identity != UINT64_C(0x55434431372e302e)) return 1;
    puts("VIEW0_V1N3_UNICODE_VERSION=17.0.0");
    puts("PASS: VIEW0 V1N3 Unicode table identity");
    return 0;
}
