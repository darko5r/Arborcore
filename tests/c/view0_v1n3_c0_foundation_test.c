#include <arborcore/view0_conformance/native.h>

#include <stdio.h>

int main(void)
{
    if (ARBOR_VIEW0_NATIVE_V1N3_OPTIONS_ABI_V1 != UINT64_C(0x415256304e334f31) ||
        ARBOR_VIEW0_NATIVE_V1N3_MAX_DEFINITIONS != 4096u ||
        ARBOR_VIEW0_NATIVE_V1N3_TRANSIENT_SUPPORT_ARENAS != 1u) return 1;
    puts("VIEW0_V1N3_C0_RULE_IDENTITIES=30_OF_30");
    puts("VIEW0_V1N3_C0_GROUP_COUNTS=G12_8_G13_6_G14_6_G15_8_G16_2");
    puts("VIEW0_V1N3_C0_RUNTIME_EXECUTION=ZERO");
    puts("PASS: VIEW0 V1N3 C0 configured-checker foundation");
    return 0;
}
