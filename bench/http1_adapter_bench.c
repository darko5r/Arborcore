#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include <arborcore/http_mvc.h>

static uint64_t ns_now(void)
{
    struct timespec ts = {0, 0};
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0u;
    return (uint64_t)ts.tv_sec * UINT64_C(1000000000) + (uint64_t)ts.tv_nsec;
}

int main(void)
{
    arbor_mvc_catalog catalog = {
        ARBOR_MVC_ABI_VERSION, (uint32_t)sizeof(arbor_mvc_catalog),
        ARBOR_MVC_CATALOG_FLAGS_NONE, NULL, 0u, NULL, 0u
    };
    arbor_mvc_prepare_workspace workspace = {NULL, 0u};
    arbor_mvc_application mvc = {0};
    if (arbor_mvc_application_prepare(&catalog, &workspace, &mvc).native != 0) return 1;
    arbor_http_mvc_application app = {0};
    if (arbor_http_mvc_application_prepare(&mvc, 8u, &app).native != 0) return 1;

    const uint64_t iterations = UINT64_C(200000);
    uint64_t start = ns_now();
    if (start == 0u) return 1;
    uint64_t checksum = 0u;
    for (uint64_t i = 0u; i < iterations; ++i) {
        arbor_status status = arbor_http_mvc_application_validate(&app);
        if (status.native != 0) return 1;
        checksum += (uint64_t)(status.native == 0);
    }
    uint64_t end = ns_now();
    if (end <= start) return 1;
    printf("HTTP1_ADAPTER_VALIDATE_NS_TOTAL=%llu\n", (unsigned long long)(end - start));
    printf("HTTP1_ADAPTER_VALIDATE_ITERATIONS=%llu\n", (unsigned long long)iterations);
    printf("HTTP1_ADAPTER_VALIDATE_CHECKSUM=%llu\n", (unsigned long long)checksum);
    return 0;
}
