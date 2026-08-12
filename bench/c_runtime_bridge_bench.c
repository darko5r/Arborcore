#define _GNU_SOURCE
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include <arborcore/arborcore.h>

#define REQUEST_ITERATIONS 200000u
#define RESPONSE_ITERATIONS 250000u
#define ROUTE_ITERATIONS 250000u

static volatile uint64_t benchmark_sink;

static uint64_t now_ns(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC_RAW, &ts) != 0) {
        return 0u;
    }
    return ((uint64_t)ts.tv_sec * 1000000000u) + (uint64_t)ts.tv_nsec;
}

static double raw_request_bench(void)
{
    static const uint8_t bytes[] =
        "GET /items/42?x=1 HTTP/1.1\r\n"
        "Host: example\r\n"
        "Content-Length: 0\r\n\r\n";
    arbor_asm_http_request request;
    arbor_asm_request_target target;
    uint64_t start = now_ns();
    uint64_t acc = 0u;

    for (uint64_t i = 0u; i < REQUEST_ITERATIONS; ++i) {
        arbor_asm_result_u64 parsed = http_parse_request(bytes, (uint64_t)sizeof(bytes) - 1u, &request);
        if (parsed.status != 0) {
            return -1.0;
        }
        arbor_asm_result_u64 split = request_target_from_request(&request, &target);
        if (split.status != 0) {
            return -1.0;
        }
        acc += request.message_length + target.path_len;
    }

    uint64_t end = now_ns();
    benchmark_sink = acc;
    return (double)(end - start) / (double)REQUEST_ITERATIONS;
}

static double wrapper_request_bench(void)
{
    static const uint8_t bytes[] =
        "GET /items/42?x=1 HTTP/1.1\r\n"
        "Host: example\r\n"
        "Content-Length: 0\r\n\r\n";
    arbor_request_view request;
    uint64_t start = now_ns();
    uint64_t acc = 0u;

    for (uint64_t i = 0u; i < REQUEST_ITERATIONS; ++i) {
        uint64_t required = 0u;
        arbor_status status = arbor_request_parse(
            (arbor_span){bytes, (uint64_t)sizeof(bytes) - 1u},
            &request,
            &required);
        if (arbor_status_is_error(status)) {
            return -1.0;
        }
        acc += required + request.target.path_len;
    }

    uint64_t end = now_ns();
    benchmark_sink = acc;
    return (double)(end - start) / (double)REQUEST_ITERATIONS;
}

static double raw_response_bench(void)
{
    uint8_t bytes[512];
    arbor_asm_buffer buffer;
    if (buffer_init(&buffer, bytes, (uint64_t)sizeof(bytes)).status != 0) {
        return -1.0;
    }

    uint64_t start = now_ns();
    uint64_t acc = 0u;
    for (uint64_t i = 0u; i < RESPONSE_ITERATIONS; ++i) {
        if (buffer_reset(&buffer).status != 0) {
            return -1.0;
        }
        arbor_asm_result_u64 result = http_response_serialize(&buffer, 200u, NULL, 0u, 1u);
        if (result.status != 0) {
            return -1.0;
        }
        acc += result.value;
    }
    uint64_t end = now_ns();
    benchmark_sink = acc;
    return (double)(end - start) / (double)RESPONSE_ITERATIONS;
}

static double wrapper_response_bench(void)
{
    uint8_t bytes[512];
    arbor_asm_buffer buffer;
    if (buffer_init(&buffer, bytes, (uint64_t)sizeof(bytes)).status != 0) {
        return -1.0;
    }

    uint64_t start = now_ns();
    uint64_t acc = 0u;
    for (uint64_t i = 0u; i < RESPONSE_ITERATIONS; ++i) {
        if (buffer_reset(&buffer).status != 0) {
            return -1.0;
        }
        uint64_t written = 0u;
        arbor_status status = arbor_response_serialize(
            &buffer,
            200u,
            (arbor_span){NULL, 0u},
            true,
            &written);
        if (arbor_status_is_error(status)) {
            return -1.0;
        }
        acc += written;
    }
    uint64_t end = now_ns();
    benchmark_sink = acc;
    return (double)(end - start) / (double)RESPONSE_ITERATIONS;
}

static double raw_route_bench(void)
{
    static const uint8_t pattern[] = "/items/:id";
    static const uint8_t path[] = "/items/42";
    arbor_asm_route_param params[2];
    uint64_t start = now_ns();
    uint64_t acc = 0u;

    for (uint64_t i = 0u; i < ROUTE_ITERATIONS; ++i) {
        arbor_asm_match_result result = route_pattern_match(
            pattern,
            (uint64_t)sizeof(pattern) - 1u,
            path,
            (uint64_t)sizeof(path) - 1u,
            params,
            2u);
        if (result.match != 1) {
            return -1.0;
        }
        acc += result.parameter_count + params[0].value_len;
    }

    uint64_t end = now_ns();
    benchmark_sink = acc;
    return (double)(end - start) / (double)ROUTE_ITERATIONS;
}

static double wrapper_route_bench(void)
{
    static const uint8_t pattern[] = "/items/:id";
    static const uint8_t path[] = "/items/42";
    arbor_route_param params[2];
    uint64_t start = now_ns();
    uint64_t acc = 0u;

    for (uint64_t i = 0u; i < ROUTE_ITERATIONS; ++i) {
        bool matched = false;
        uint64_t count = 0u;
        arbor_status status = arbor_route_match(
            (arbor_span){pattern, (uint64_t)sizeof(pattern) - 1u},
            (arbor_span){path, (uint64_t)sizeof(path) - 1u},
            params,
            2u,
            &matched,
            &count);
        if (arbor_status_is_error(status) || !matched) {
            return -1.0;
        }
        acc += count + params[0].value_len;
    }

    uint64_t end = now_ns();
    benchmark_sink = acc;
    return (double)(end - start) / (double)ROUTE_ITERATIONS;
}

int main(void)
{
    double raw_request = raw_request_bench();
    double wrapper_request = wrapper_request_bench();
    double raw_response = raw_response_bench();
    double wrapper_response = wrapper_response_bench();
    double raw_route = raw_route_bench();
    double wrapper_route = wrapper_route_bench();

    if (raw_request < 0.0 || wrapper_request < 0.0 ||
        raw_response < 0.0 || wrapper_response < 0.0 ||
        raw_route < 0.0 || wrapper_route < 0.0) {
        return 1;
    }

    (void)printf("raw_request\t%.6f\n", raw_request);
    (void)printf("wrapper_request\t%.6f\n", wrapper_request);
    (void)printf("raw_response\t%.6f\n", raw_response);
    (void)printf("wrapper_response\t%.6f\n", wrapper_response);
    (void)printf("raw_route\t%.6f\n", raw_route);
    (void)printf("wrapper_route\t%.6f\n", wrapper_route);
    return benchmark_sink == UINT64_MAX ? 2 : 0;
}
