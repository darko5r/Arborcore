#include "ecma_unicode_tables.h"

#include <stdio.h>

int main(void)
{
    return printf("VIEW0_V1N3_UNICODE_TABLES=17.0.0:%016llx\n",
        (unsigned long long)arbor_view0_native_v1n3_unicode_table_identity) < 0 ? 1 : 0;
}
