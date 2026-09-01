#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include <arborcore/http_mvc.h>

#define HELLO0_BENCH_MAX_ROUTES 256u
#define HELLO0_BENCH_PATTERN_CAPACITY 8u
#define HELLO0_BENCH_WORKSPACE_CAPACITY 8u
#define HELLO0_BENCH_BUFFER_CAPACITY 16384u
#define HELLO0_BENCH_REQUESTS_PER_ROUND 64u
#define HELLO0_BENCH_QUIESCENT_STEPS 1000u
#define HELLO0_BENCH_ROUNDS 9u

typedef struct hello0_bench_application {
    uint8_t patterns[HELLO0_BENCH_MAX_ROUTES][HELLO0_BENCH_PATTERN_CAPACITY];
    arbor_mvc_route routes[HELLO0_BENCH_MAX_ROUTES];
    arbor_mvc_catalog catalog;
    arbor_mvc_application mvc;
    arbor_http_mvc_application http;
} hello0_bench_application;

static int64_t hello0_bench_controller(
    const arbor_mvc_request *request,
    void *context,
    arbor_mvc_controller_result *result_out)
{
    (void)context;
    if (arbor_mvc_request_validate(request).native != 0 || result_out == NULL) {
        return -EINVAL;
    }
    *result_out = (arbor_mvc_controller_result){
        1u,
        ARBOR_MVC_CONTROLLER_RESULT_FLAGS_NONE,
        NULL,
        0u
    };
    return 0;
}

static int64_t hello0_bench_presenter(
    const arbor_mvc_request *request,
    void *context,
    const arbor_mvc_controller_result *result,
    arbor_response_plan *response_out)
{
    (void)context;
    if (arbor_mvc_request_validate(request).native != 0 || result == NULL ||
        response_out == NULL || result->outcome_code != 1u ||
        result->model_data != NULL || result->model_size != 0u) {
        return -EINVAL;
    }
    arbor_status status = arbor_response_plan_make(
        204u,
        (arbor_span){NULL, 0u},
        ARBOR_RESPONSE_PLAN_FLAG_KEEP_ALIVE,
        response_out);
    return status.native;
}

static int hello0_bench_prepare(
    hello0_bench_application *application,
    uint64_t route_count)
{
    static const uint8_t method[] = "GET";
    if (application == NULL || route_count == 0u ||
        route_count > HELLO0_BENCH_MAX_ROUTES) {
        return 1;
    }
    (void)memset(application, 0, sizeof(*application));
    for (uint64_t i = 0u; i < route_count; ++i) {
        int length = snprintf(
            (char *)(void *)application->patterns[i],
            HELLO0_BENCH_PATTERN_CAPACITY,
            "/r%03" PRIu64,
            i);
        if (length != 5) {
            return 1;
        }
        application->routes[i] = (arbor_mvc_route){
            method,
            (uint64_t)(sizeof(method) - 1u),
            application->patterns[i],
            (uint64_t)length,
            hello0_bench_controller,
            NULL,
            hello0_bench_presenter,
            NULL,
            NULL,
            0u
        };
    }
    application->catalog = (arbor_mvc_catalog){
        ARBOR_MVC_ABI_VERSION,
        (uint32_t)sizeof(arbor_mvc_catalog),
        ARBOR_MVC_CATALOG_FLAGS_NONE,
        application->routes,
        route_count,
        NULL,
        0u
    };
    arbor_route_param params[HELLO0_BENCH_WORKSPACE_CAPACITY] = {0};
    arbor_mvc_prepare_workspace workspace = {
        params,
        HELLO0_BENCH_WORKSPACE_CAPACITY
    };
    if (arbor_mvc_application_prepare(
            &application->catalog,
            &workspace,
            &application->mvc).native != 0 ||
        arbor_http_mvc_application_prepare(
            &application->mvc,
            0u,
            &application->http).native != 0) {
        return 1;
    }
    return 0;
}

static int hello0_bench_now(uint64_t *nanoseconds_out)
{
    struct timespec now = {0, 0};
    if (nanoseconds_out == NULL || clock_gettime(CLOCK_MONOTONIC, &now) != 0 ||
        now.tv_sec < 0 || now.tv_nsec < 0) {
        return 1;
    }
    uint64_t seconds = (uint64_t)now.tv_sec;
    if (seconds > UINT64_MAX / UINT64_C(1000000000)) {
        return 1;
    }
    *nanoseconds_out = seconds * UINT64_C(1000000000) +
        (uint64_t)now.tv_nsec;
    return 0;
}

static int hello0_bench_write_all(int fd, const uint8_t *data, size_t length)
{
    size_t offset = 0u;
    while (offset < length) {
        ssize_t written = write(fd, data + offset, length - offset);
        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }
            return 1;
        }
        if (written == 0) {
            return 1;
        }
        offset += (size_t)written;
    }
    return 0;
}

static int hello0_bench_make_pipeline(
    const char *path,
    uint8_t *buffer,
    size_t capacity,
    size_t *length_out)
{
    if (path == NULL || buffer == NULL || length_out == NULL) {
        return 1;
    }
    size_t used = 0u;
    for (uint64_t i = 0u; i < HELLO0_BENCH_REQUESTS_PER_ROUND; ++i) {
        const char *connection =
            i + 1u == HELLO0_BENCH_REQUESTS_PER_ROUND ?
                "Connection: close\r\n" : "";
        int length = snprintf(
            (char *)(void *)(buffer + used),
            capacity - used,
            "GET %s HTTP/1.1\r\nHost: local\r\n%s\r\n",
            path,
            connection);
        if (length < 0 || (size_t)length >= capacity - used) {
            return 1;
        }
        used += (size_t)length;
    }
    *length_out = used;
    return 0;
}

static int hello0_bench_pipeline_round(
    const hello0_bench_application *application,
    const char *path,
    uint64_t *nanoseconds_per_request_out)
{
    uint8_t requests[HELLO0_BENCH_BUFFER_CAPACITY] = {0};
    size_t request_length = 0u;
    if (hello0_bench_make_pipeline(
            path,
            requests,
            sizeof(requests),
            &request_length) != 0) {
        return 1;
    }

    int sockets[2] = {-1, -1};
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) != 0) {
        return 1;
    }
    uint8_t input[HELLO0_BENCH_BUFFER_CAPACITY] = {0};
    uint8_t output[HELLO0_BENCH_BUFFER_CAPACITY] = {0};
    uint8_t arena[HELLO0_BENCH_BUFFER_CAPACITY] = {0};
    arbor_runtime_storage storage = {0};
    if (arbor_runtime_storage_prepare(
            &storage,
            (arbor_mut_span){input, sizeof(input)},
            (arbor_mut_span){output, sizeof(output)},
            (arbor_mut_span){arena, sizeof(arena)}).native != 0 ||
        connection_init(
            &storage.connection,
            sockets[1],
            &storage.input,
            &storage.output,
            &storage.arena).status != 0 ||
        connection_transition(
            &storage.connection,
            ARBOR_ASM_CONNECTION_READING).status != 0 ||
        hello0_bench_write_all(sockets[0], requests, request_length) != 0) {
        (void)close(sockets[0]);
        (void)close(sockets[1]);
        return 1;
    }

    uint64_t started = 0u;
    uint64_t finished = 0u;
    uint64_t completed = 0u;
    if (hello0_bench_now(&started) != 0) {
        (void)close(sockets[0]);
        (void)close(sockets[1]);
        return 1;
    }
    for (;;) {
        arbor_status status = arbor_http_mvc_server_step(
            &storage,
            &application->http,
            -1,
            &completed);
        if (status.native == (int64_t)ARBORCORE_SERVER_MORE_WORK) {
            continue;
        }
        if (status.native != 0 ||
            storage.connection.state != ARBOR_ASM_CONNECTION_CLOSED) {
            (void)close(sockets[0]);
            if (storage.connection.state != ARBOR_ASM_CONNECTION_CLOSED) {
                (void)close(sockets[1]);
            }
            return 1;
        }
        break;
    }
    if (hello0_bench_now(&finished) != 0 || finished < started ||
        completed != HELLO0_BENCH_REQUESTS_PER_ROUND) {
        (void)close(sockets[0]);
        return 1;
    }
    (void)close(sockets[0]);
    *nanoseconds_per_request_out =
        (finished - started) / HELLO0_BENCH_REQUESTS_PER_ROUND;
    return 0;
}

static int hello0_bench_quiescent_round(
    const hello0_bench_application *application,
    uint64_t *nanoseconds_per_step_out)
{
    int sockets[2] = {-1, -1};
    if (socketpair(
            AF_UNIX,
            SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
            0,
            sockets) != 0) {
        return 1;
    }
    uint8_t input[HELLO0_BENCH_BUFFER_CAPACITY] = {0};
    uint8_t output[HELLO0_BENCH_BUFFER_CAPACITY] = {0};
    uint8_t arena[HELLO0_BENCH_BUFFER_CAPACITY] = {0};
    arbor_runtime_storage storage = {0};
    if (arbor_runtime_storage_prepare(
            &storage,
            (arbor_mut_span){input, sizeof(input)},
            (arbor_mut_span){output, sizeof(output)},
            (arbor_mut_span){arena, sizeof(arena)}).native != 0 ||
        connection_init(
            &storage.connection,
            sockets[1],
            &storage.input,
            &storage.output,
            &storage.arena).status != 0 ||
        connection_transition(
            &storage.connection,
            ARBOR_ASM_CONNECTION_READING).status != 0) {
        (void)close(sockets[0]);
        (void)close(sockets[1]);
        return 1;
    }

    uint64_t started = 0u;
    uint64_t finished = 0u;
    if (hello0_bench_now(&started) != 0) {
        (void)close(sockets[0]);
        (void)close(sockets[1]);
        return 1;
    }
    for (uint64_t i = 0u; i < HELLO0_BENCH_QUIESCENT_STEPS; ++i) {
        arbor_status status = arbor_http_mvc_server_step(
            &storage,
            &application->http,
            -1,
            NULL);
        if (status.native != -EAGAIN) {
            (void)close(sockets[0]);
            (void)close(sockets[1]);
            return 1;
        }
    }
    if (hello0_bench_now(&finished) != 0 || finished < started) {
        (void)close(sockets[0]);
        (void)close(sockets[1]);
        return 1;
    }
    (void)close(sockets[0]);
    (void)close(sockets[1]);
    *nanoseconds_per_step_out =
        (finished - started) / HELLO0_BENCH_QUIESCENT_STEPS;
    return 0;
}

static uint64_t hello0_bench_median(uint64_t values[HELLO0_BENCH_ROUNDS])
{
    for (size_t i = 1u; i < HELLO0_BENCH_ROUNDS; ++i) {
        uint64_t value = values[i];
        size_t position = i;
        while (position != 0u && values[position - 1u] > value) {
            values[position] = values[position - 1u];
            position -= 1u;
        }
        values[position] = value;
    }
    return values[HELLO0_BENCH_ROUNDS / 2u];
}

static int hello0_bench_report_pipeline(
    const hello0_bench_application *application,
    uint64_t route_count,
    const char *case_name,
    const char *path)
{
    uint64_t samples[HELLO0_BENCH_ROUNDS] = {0};
    for (size_t round = 0u; round < HELLO0_BENCH_ROUNDS; ++round) {
        if (hello0_bench_pipeline_round(
                application,
                path,
                &samples[round]) != 0) {
            return 1;
        }
    }
    printf(
        "%" PRIu64 ",%s,%" PRIu64 "\n",
        route_count,
        case_name,
        hello0_bench_median(samples));
    return 0;
}

static int hello0_bench_report_quiescent(
    const hello0_bench_application *application,
    uint64_t route_count)
{
    uint64_t samples[HELLO0_BENCH_ROUNDS] = {0};
    for (size_t round = 0u; round < HELLO0_BENCH_ROUNDS; ++round) {
        if (hello0_bench_quiescent_round(application, &samples[round]) != 0) {
            return 1;
        }
    }
    printf(
        "%" PRIu64 ",quiescent,%" PRIu64 "\n",
        route_count,
        hello0_bench_median(samples));
    return 0;
}

int main(void)
{
    static const uint64_t route_counts[] = {2u, 16u, 64u, 256u};
    puts("HELLO0_ROUTE_SCALE_DIAGNOSTIC");
    puts("routes,case,median_ns_per_operation");
    for (size_t i = 0u; i < sizeof(route_counts) / sizeof(route_counts[0]); ++i) {
        uint64_t route_count = route_counts[i];
        hello0_bench_application application = {0};
        if (hello0_bench_prepare(&application, route_count) != 0) {
            fputs("FAIL: prepare route-scale application\n", stderr);
            return 1;
        }
        char last_path[HELLO0_BENCH_PATTERN_CAPACITY] = {0};
        int length = snprintf(
            last_path,
            sizeof(last_path),
            "/r%03" PRIu64,
            route_count - 1u);
        if (length != 5 ||
            hello0_bench_report_pipeline(
                &application,
                route_count,
                "first",
                "/r000") != 0 ||
            hello0_bench_report_pipeline(
                &application,
                route_count,
                "last",
                last_path) != 0 ||
            hello0_bench_report_pipeline(
                &application,
                route_count,
                "miss",
                "/miss") != 0 ||
            hello0_bench_report_quiescent(&application, route_count) != 0) {
            fputs("FAIL: route-scale measurement\n", stderr);
            return 1;
        }
    }
    puts("THRESHOLD_GATE=NONE_DIAGNOSTIC_BASELINE_ONLY");
    puts("PASS: HELLO0 route-scale server-step diagnostic completed");
    return 0;
}
