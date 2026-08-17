#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include <arborcore/http.h>

#define HTTP0_BENCH_ROUNDS 9u
#define HTTP0_BENCH_ITERATIONS UINT64_C(200000)

static uint64_t monotonic_ns(void)
{
    struct timespec ts = {0, 0};
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0u;
    }
    return (uint64_t)ts.tv_sec * UINT64_C(1000000000) + (uint64_t)ts.tv_nsec;
}

static void sort_double(double *values, size_t count)
{
    for (size_t i = 1u; i < count; ++i) {
        const double value = values[i];
        size_t j = i;
        while (j > 0u && values[j - 1u] > value) {
            values[j] = values[j - 1u];
            j -= 1u;
        }
        values[j] = value;
    }
}

int main(void)
{
    static const uint8_t request_bytes[] =
        "GET /hello HTTP/1.1\r\nHost: local\r\n\r\n";
    arbor_request_view request = {0};
    uint64_t required = 0u;
    if (arbor_request_parse(
            (arbor_span){request_bytes, sizeof(request_bytes) - 1u},
            &request,
            &required).native != 0 ||
        required != sizeof(request_bytes) - 1u) {
        return 1;
    }

    static const uint8_t content_type_name[] = "Content-Type";
    static const uint8_t content_type_value[] = "text/html; charset=utf-8";
    static const uint8_t cache_name[] = "Cache-Control";
    static const uint8_t cache_value[] = "no-store";
    static const uint8_t body[] =
        "<!doctype html><html><body><h1>Hello HTTP0</h1></body></html>";
    const arbor_http_field fields[] = {
        {content_type_name, sizeof(content_type_name) - 1u,
         content_type_value, sizeof(content_type_value) - 1u},
        {cache_name, sizeof(cache_name) - 1u,
         cache_value, sizeof(cache_value) - 1u}
    };
    arbor_http_response response = {0};
    if (arbor_http_response_make(
            200u,
            fields,
            (uint64_t)(sizeof(fields) / sizeof(fields[0])),
            (arbor_span){body, sizeof(body) - 1u},
            ARBOR_HTTP_RESPONSE_FLAG_NONE,
            &response).native != 0) {
        return 1;
    }

    uint8_t bytes[1024] = {0};
    arbor_asm_buffer buffer = {0};
    if (buffer_init(&buffer, bytes, sizeof(bytes)).status != 0) {
        return 1;
    }

    double rounds[HTTP0_BENCH_ROUNDS] = {0.0};
    uint64_t checksum = 0u;
    for (size_t round = 0u; round < HTTP0_BENCH_ROUNDS; ++round) {
        const uint64_t start = monotonic_ns();
        if (start == 0u) {
            return 1;
        }
        for (uint64_t i = 0u; i < HTTP0_BENCH_ITERATIONS; ++i) {
            buffer.length = 0u;
            uint64_t written = 0u;
            bool close_after = true;
            const arbor_status status = arbor_http_response_serialize(
                &buffer, &request.native, &response, &written, &close_after);
            if (status.native != 0 || close_after || written == 0u) {
                return 1;
            }
            checksum ^= written + i;
        }
        const uint64_t end = monotonic_ns();
        if (end <= start) {
            return 1;
        }
        rounds[round] = (double)(end - start) / (double)HTTP0_BENCH_ITERATIONS;
    }

    sort_double(rounds, HTTP0_BENCH_ROUNDS);
    const double median = rounds[HTTP0_BENCH_ROUNDS / 2u];
    printf("HTTP0_RESPONSE_SERIALIZE_MEDIAN_NS_PER_OP=%.3f\n", median);
    printf("HTTP0_RESPONSE_SERIALIZE_ITERATIONS=%llu\n",
           (unsigned long long)HTTP0_BENCH_ITERATIONS);
    printf("HTTP0_RESPONSE_SERIALIZE_ROUNDS=%u\n", HTTP0_BENCH_ROUNDS);
    printf("HTTP0_RESPONSE_SERIALIZE_CHECKSUM=%llu\n", (unsigned long long)checksum);
    return median > 0.0 ? 0 : 1;
}
