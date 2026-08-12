#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <arborcore/arborcore.h>

static int64_t ok_handler(
    const arbor_asm_http_request *request,
    void *context,
    const arbor_asm_route_param *params,
    uint64_t parameter_count)
{
    (void)request;
    (void)context;
    (void)params;
    (void)parameter_count;
    return 200;
}

static void close_if_valid(int64_t fd)
{
    if (fd >= 0 && fd <= INT32_MAX) {
        (void)close((int)fd);
    }
}

int main(void)
{
    int result_code = 1;
    int64_t listener_fd = -1;
    int64_t epoll_fd = -1;
    int64_t accepted_fd = -1;
    int client_fd = -1;
    bool accepted = false;

    struct sockaddr_in address;
    (void)memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(0u);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    arbor_status status = arbor_server_open(
        &address,
        (uint64_t)sizeof(address),
        16,
        &listener_fd);
    if (arbor_status_is_error(status) || listener_fd < 0 || listener_fd > INT32_MAX) {
        goto done;
    }

    socklen_t address_length = (socklen_t)sizeof(address);
    if (getsockname((int)listener_fd, (struct sockaddr *)&address, &address_length) != 0) {
        goto done;
    }

    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        goto done;
    }
    if (connect(client_fd, (const struct sockaddr *)&address, address_length) != 0) {
        goto done;
    }

    status = arbor_event_loop_create(listener_fd, &epoll_fd);
    if (arbor_status_is_error(status) || epoll_fd < 0 || epoll_fd > INT32_MAX) {
        goto done;
    }

    uint8_t input_bytes[4096];
    uint8_t output_bytes[4096];
    uint8_t arena_bytes[4096];
    arbor_runtime_storage storage;
    status = arbor_runtime_storage_prepare(
        &storage,
        (arbor_mut_span){input_bytes, (uint64_t)sizeof(input_bytes)},
        (arbor_mut_span){output_bytes, (uint64_t)sizeof(output_bytes)},
        (arbor_mut_span){arena_bytes, (uint64_t)sizeof(arena_bytes)});
    if (arbor_status_is_error(status)) {
        goto done;
    }

    status = arbor_server_accept(listener_fd, epoll_fd, &storage, &accepted_fd);
    if (arbor_status_is_error(status) || accepted_fd < 0 || accepted_fd > INT32_MAX) {
        goto done;
    }
    accepted = true;

    static const uint8_t request[] = "GET /hello HTTP/1.1\r\nHost: local\r\n\r\n";
    ssize_t sent = write(client_fd, request, sizeof(request) - 1u);
    if (sent != (ssize_t)(sizeof(request) - 1u)) {
        goto done;
    }

    struct pollfd readable;
    readable.fd = (int)accepted_fd;
    readable.events = POLLIN;
    readable.revents = 0;
    if (poll(&readable, 1u, 1000) <= 0) {
        goto done;
    }

    arbor_route route;
    status = arbor_route_init(
        &route,
        (arbor_span){(const uint8_t *)"GET", 3u},
        (arbor_span){(const uint8_t *)"/hello", 6u},
        ok_handler);
    if (arbor_status_is_error(status)) {
        goto done;
    }

    uint64_t completed = 0u;
    status = arbor_server_step(&storage, &route, 1u, NULL, epoll_fd, &completed);
    if (arbor_status_is_error(status) || completed != 1u) {
        goto done;
    }

    uint8_t response[512];
    ssize_t received = read(client_fd, response, sizeof(response));
    static const char prefix[] = "HTTP/1.1 200 OK\r\n";
    if (received < (ssize_t)(sizeof(prefix) - 1u) ||
        memcmp(response, prefix, sizeof(prefix) - 1u) != 0) {
        goto done;
    }

    status = arbor_server_close(epoll_fd, &storage);
    if (arbor_status_is_error(status)) {
        goto done;
    }
    accepted = false;
    accepted_fd = -1;

    result_code = 0;

done:
    if (accepted) {
        (void)arbor_server_close(epoll_fd, &storage);
    }
    close_if_valid(accepted_fd);
    close_if_valid(epoll_fd);
    close_if_valid(listener_fd);
    if (client_fd >= 0) {
        (void)close(client_fd);
    }
    return result_code;
}
