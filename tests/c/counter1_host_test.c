#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "counter1.h"
#include <arborcore/linux_http_mvc_host.h>

#define TEST_SLOT_COUNT 8u
#define TEST_BUFFER_CAPACITY 16384u
#define TEST_EVENT_CAPACITY 16u

static int fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    return 1;
}

int main(void)
{
    counter1_in_memory_repository repository = {0};
    counter1_repository_provider provider = {0};
    counter1_application application = {0};
    counter1_web_application web = {0};

    if (counter1_in_memory_repository_prepare(&repository, &provider).native != 0 ||
        counter1_application_prepare(&provider, &application).native != 0) {
        return fail("COUNTER1 application prepare");
    }

    static const uint8_t template_source[] =
        "<!doctype html>\n"
        "<html><head><meta charset=\"utf-8\"><title>COUNTER1</title></head>"
        "<body><p>Counter {{id}} = {{value}}</p></body></html>\n";
    if (counter1_web_application_prepare(
            &application,
            (arbor_span){template_source, sizeof(template_source) - 1u},
            COUNTER1_RESPONSE_FIELD_CAPACITY,
            &web).native != 0) {
        (void)counter1_application_stop(&application);
        return fail("COUNTER1 web prepare");
    }

    static arbor_linux_http_mvc_host_slot slots[TEST_SLOT_COUNT];
    static uint8_t inputs[TEST_SLOT_COUNT][TEST_BUFFER_CAPACITY];
    static uint8_t outputs[TEST_SLOT_COUNT][TEST_BUFFER_CAPACITY];
    static uint8_t arenas[TEST_SLOT_COUNT][TEST_BUFFER_CAPACITY];
    for (uint64_t index = 0u; index < TEST_SLOT_COUNT; ++index) {
        arbor_status status = arbor_linux_http_mvc_host_slot_prepare(
            &slots[index],
            (arbor_mut_span){inputs[index], sizeof(inputs[index])},
            (arbor_mut_span){outputs[index], sizeof(outputs[index])},
            (arbor_mut_span){arenas[index], sizeof(arenas[index])});
        if (status.native != 0) {
            (void)counter1_application_stop(&application);
            return fail("HOST1 slot prepare");
        }
    }

    static arbor_asm_epoll_event events[TEST_EVENT_CAPACITY];
    arbor_linux_http_mvc_host_options options = {0};
    if (arbor_linux_http_mvc_host_options_make(
            &options, 250, UINT64_C(2000), NULL, NULL, NULL, NULL).native != 0) {
        (void)counter1_application_stop(&application);
        return fail("HOST1 options");
    }

    arbor_linux_http_mvc_host host = {0};
    if (arbor_linux_http_mvc_host_prepare(
            &host,
            &web.http_application,
            slots,
            TEST_SLOT_COUNT,
            events,
            TEST_EVENT_CAPACITY,
            &options).native != 0) {
        (void)counter1_application_stop(&application);
        return fail("HOST1 prepare");
    }

    struct sockaddr_in address;
    (void)memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(0u);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (arbor_linux_http_mvc_host_open(&host, &address, sizeof(address), 16).native != 0 ||
        host.phase != ARBOR_LINUX_HTTP_MVC_HOST_PHASE_ACCEPTING) {
        (void)arbor_linux_http_mvc_host_close(&host);
        (void)counter1_application_stop(&application);
        return fail("HOST1 open");
    }

    if (arbor_linux_http_mvc_host_close(&host).native != 0 ||
        host.phase != ARBOR_LINUX_HTTP_MVC_HOST_PHASE_CLOSED) {
        (void)counter1_application_stop(&application);
        return fail("HOST1 close before AF3 stop");
    }
    arbor_linux_http_mvc_host_shutdown_result shutdown = {0};
    if (arbor_linux_http_mvc_host_shutdown_result_get(&host, &shutdown).native != 0 ||
        shutdown.first_failure != 0) {
        (void)counter1_application_stop(&application);
        return fail("HOST1 shutdown result");
    }

    if (counter1_application_validate(&application).native != 0) {
        (void)counter1_application_stop(&application);
        return fail("application remains live through HOST1 close");
    }
    if (counter1_application_stop(&application).native != 0) {
        return fail("AF3 stop after HOST1 closed");
    }

    puts("PASS: COUNTER1 HOST1 close then AF3 stop lifecycle");
    return 0;
}
