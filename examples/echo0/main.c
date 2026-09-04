#include <arpa/inet.h>
#include <inttypes.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "echo0.h"
#include <arborcore/config.h>
#include <arborcore/linux_http_mvc_host.h>

#define ECHO0_EVENT_CAPACITY 16u
#define ECHO0_CONFIG_FILE_CAPACITY 8192u
#define ECHO0_CONFIG_CLI_CAPACITY 6u
#define ECHO0_CONFIG_PERSISTENT_CAPACITY 4110u
#define ECHO0_CONFIG_ENV_ENTRY_CAPACITY 8256u
#define ECHO0_TEMPLATE_PATH_CAPACITY 4096u
#define ECHO0_BIND_IPV4_CAPACITY 16u

#define SPAN_LITERAL(text) \
    { (const uint8_t *)(text), (uint64_t)(sizeof(text) - 1u) }

static volatile sig_atomic_t echo0_stop_requested = 0;

static const arbor_config_descriptor echo0_config_descriptors[] = {
    {
        {SPAN_LITERAL("template"), SPAN_LITERAL("template"),
         SPAN_LITERAL("ARBORCORE_TEMPLATE"), SPAN_LITERAL("template")},
        ARBOR_CONFIG_KIND_UTF8, 0u, ARBOR_CONFIG_DESCRIPTOR_REQUIRED,
        {0}, 0u, 0u, 0, 0, 4095u, NULL, 0u
    },
    {
        {SPAN_LITERAL("bind_ipv4"), SPAN_LITERAL("bind_ipv4"),
         SPAN_LITERAL("ARBORCORE_BIND_IPV4"), SPAN_LITERAL("bind-ipv4")},
        ARBOR_CONFIG_KIND_UTF8, 0u, ARBOR_CONFIG_DESCRIPTOR_HAS_DEFAULT,
        {0u, 0, SPAN_LITERAL("127.0.0.1"), 0, ARBOR_CONFIG_KIND_UTF8, false,
         {0u, 0u, 0u}},
        0u, 0u, 0, 0, 15u, NULL, 0u
    },
    {
        {SPAN_LITERAL("port"), SPAN_LITERAL("port"),
         SPAN_LITERAL("ARBORCORE_PORT"), SPAN_LITERAL("port")},
        ARBOR_CONFIG_KIND_U64, 0u, ARBOR_CONFIG_DESCRIPTOR_REQUIRED,
        {0}, 0u, UINT16_MAX, 0, 0, 0u, NULL, 0u
    },
    {
        {SPAN_LITERAL("backlog"), SPAN_LITERAL("backlog"),
         SPAN_LITERAL("ARBORCORE_BACKLOG"), SPAN_LITERAL("backlog")},
        ARBOR_CONFIG_KIND_I64, 0u, ARBOR_CONFIG_DESCRIPTOR_HAS_DEFAULT,
        {0u, 16, {NULL, 0u}, 0, ARBOR_CONFIG_KIND_I64, false, {0u, 0u, 0u}},
        0u, 0u, 1, INT_MAX, 0u, NULL, 0u
    },
    {
        {SPAN_LITERAL("event_wait_ms"), SPAN_LITERAL("event_wait_ms"),
         SPAN_LITERAL("ARBORCORE_EVENT_WAIT_MS"),
         SPAN_LITERAL("event-wait-ms")},
        ARBOR_CONFIG_KIND_I64, 0u, ARBOR_CONFIG_DESCRIPTOR_HAS_DEFAULT,
        {0u, 250, {NULL, 0u}, 0, ARBOR_CONFIG_KIND_I64, false,
         {0u, 0u, 0u}},
        0u, 0u, 0, INT_MAX, 0u, NULL, 0u
    },
    {
        {SPAN_LITERAL("drain_timeout_ms"), SPAN_LITERAL("drain_timeout_ms"),
         SPAN_LITERAL("ARBORCORE_DRAIN_TIMEOUT_MS"),
         SPAN_LITERAL("drain-timeout-ms")},
        ARBOR_CONFIG_KIND_U64, 0u, ARBOR_CONFIG_DESCRIPTOR_HAS_DEFAULT,
        {2000u, 0, {NULL, 0u}, 0, ARBOR_CONFIG_KIND_U64, false,
         {0u, 0u, 0u}},
        0u, UINT64_MAX, 0, 0, 0u, NULL, 0u
    }
};

static const arbor_config_schema echo0_config_schema = {
    ARBOR_CONFIG_ABI_VERSION,
    sizeof(arbor_config_schema),
    ARBOR_CONFIG_SCHEMA_KNOWN_FLAGS,
    echo0_config_descriptors,
    6u
};

static void echo0_request_stop(int signal_number)
{
    (void)signal_number;
    echo0_stop_requested = 1;
}

static int echo0_install_signal_handlers(void)
{
    struct sigaction action;
    (void)memset(&action, 0, sizeof(action));
    action.sa_handler = echo0_request_stop;
    if (sigemptyset(&action.sa_mask) != 0 ||
        sigaction(SIGINT, &action, NULL) != 0 ||
        sigaction(SIGTERM, &action, NULL) != 0) {
        return 1;
    }
    struct sigaction ignored;
    (void)memset(&ignored, 0, sizeof(ignored));
    ignored.sa_handler = SIG_IGN;
    if (sigemptyset(&ignored.sa_mask) != 0 ||
        sigaction(SIGPIPE, &ignored, NULL) != 0) {
        return 1;
    }
    return 0;
}

static int echo0_read_file(
    const char *path,
    uint8_t *storage,
    size_t capacity,
    bool require_nonempty,
    arbor_span *source_out)
{
    if (path == NULL || path[0] == '\0' || storage == NULL || capacity == 0u ||
        source_out == NULL) {
        return 1;
    }
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return 1;
    }
    size_t used = fread(storage, 1u, capacity, file);
    bool failed = ferror(file) != 0;
    if (!failed && used == capacity) {
        int extra = fgetc(file);
        if (extra != EOF || ferror(file) != 0) {
            failed = true;
        }
    }
    if (fclose(file) != 0) {
        failed = true;
    }
    if (failed || (require_nonempty && used == 0u)) {
        return 1;
    }
    *source_out = (arbor_span){storage, (uint64_t)used};
    return 0;
}

static void echo0_config_error(const arbor_config_diagnostic *diagnostic)
{
    fprintf(
        stderr,
        "ECHO0_CONFIG_ERROR code=%u source=%u record=%" PRIu64
        " offset=%" PRIu64 " key=%" PRIu64 "\n",
        (unsigned)diagnostic->code,
        (unsigned)diagnostic->source,
        diagnostic->record,
        diagnostic->byte_offset,
        diagnostic->key_index);
}

static int echo0_copy_text(
    arbor_span source,
    char *destination,
    size_t capacity)
{
    if (source.length >= (uint64_t)capacity) {
        return 1;
    }
    if (source.length != 0u) {
        (void)memcpy(destination, source.data, (size_t)source.length);
    }
    destination[source.length] = '\0';
    return 0;
}

static int echo0_add_environment(
    const char *name,
    char *storage,
    size_t capacity,
    arbor_span *entry_out,
    bool *present_out)
{
    const char *value = getenv(name);
    if (value == NULL) {
        *present_out = false;
        return 0;
    }
    size_t name_length = strlen(name);
    size_t value_length = strlen(value);
    if (name_length + 1u > capacity ||
        value_length > capacity - name_length - 1u) {
        return 1;
    }
    (void)memcpy(storage, name, name_length);
    storage[name_length] = '=';
    (void)memcpy(storage + name_length + 1u, value, value_length);
    *entry_out = (arbor_span){
        (const uint8_t *)storage,
        (uint64_t)(name_length + 1u + value_length)
    };
    *present_out = true;
    return 0;
}

static int echo0_acquire_configuration(
    int argc,
    char **argv,
    arbor_config_sources *sources_out,
    uint8_t file_storage[ECHO0_CONFIG_FILE_CAPACITY],
    arbor_span environment_entries[6],
    char environment_storage[6][ECHO0_CONFIG_ENV_ENTRY_CAPACITY],
    arbor_span command_line_entries[ECHO0_CONFIG_CLI_CAPACITY],
    char legacy_template[ECHO0_CONFIG_ENV_ENTRY_CAPACITY],
    char legacy_port[96])
{
    bool legacy = argc == 3 && argv[1][0] != '-' && argv[2][0] != '-';
    if (legacy) {
        size_t template_length = strlen(argv[1]);
        size_t port_length = strlen(argv[2]);
        static const char template_prefix[] = "--template=";
        static const char port_prefix[] = "--port=";
        if (template_length >
                ECHO0_CONFIG_ENV_ENTRY_CAPACITY - sizeof(template_prefix) ||
            port_length > 96u - sizeof(port_prefix)) {
            return 1;
        }
        (void)memcpy(
            legacy_template, template_prefix, sizeof(template_prefix) - 1u);
        (void)memcpy(
            legacy_template + sizeof(template_prefix) - 1u,
            argv[1], template_length);
        legacy_template[sizeof(template_prefix) - 1u + template_length] = '\0';
        (void)memcpy(legacy_port, port_prefix, sizeof(port_prefix) - 1u);
        (void)memcpy(
            legacy_port + sizeof(port_prefix) - 1u, argv[2], port_length);
        legacy_port[sizeof(port_prefix) - 1u + port_length] = '\0';
        command_line_entries[0] = (arbor_span){
            (const uint8_t *)legacy_template,
            (uint64_t)(sizeof(template_prefix) - 1u + template_length)
        };
        command_line_entries[1] = (arbor_span){
            (const uint8_t *)legacy_port,
            (uint64_t)(sizeof(port_prefix) - 1u + port_length)
        };
        *sources_out = (arbor_config_sources){
            {NULL, 0u}, NULL, 0u, command_line_entries, 2u
        };
        return 0;
    }

    const char *config_path = NULL;
    uint64_t command_line_count = 0u;
    for (int index = 1; index < argc; ++index) {
        static const char prefix[] = "--config=";
        if (strncmp(argv[index], prefix, sizeof(prefix) - 1u) == 0) {
            if (config_path != NULL || argv[index][sizeof(prefix) - 1u] == '\0') {
                return 1;
            }
            config_path = argv[index] + sizeof(prefix) - 1u;
        } else {
            if (strncmp(argv[index], "--", 2u) != 0 ||
                command_line_count >= ECHO0_CONFIG_CLI_CAPACITY) {
                return 1;
            }
            command_line_entries[command_line_count] = (arbor_span){
                (const uint8_t *)argv[index],
                (uint64_t)strlen(argv[index])
            };
            ++command_line_count;
        }
    }

    arbor_span file_document = {NULL, 0u};
    if (config_path != NULL &&
        echo0_read_file(
            config_path, file_storage, ECHO0_CONFIG_FILE_CAPACITY, false,
            &file_document) != 0) {
        fputs("ECHO0_CONFIG_FILE_ERROR\n", stderr);
        return 1;
    }

    static const char *const environment_names[6] = {
        "ARBORCORE_TEMPLATE",
        "ARBORCORE_BIND_IPV4",
        "ARBORCORE_PORT",
        "ARBORCORE_BACKLOG",
        "ARBORCORE_EVENT_WAIT_MS",
        "ARBORCORE_DRAIN_TIMEOUT_MS"
    };
    uint64_t environment_count = 0u;
    for (uint64_t index = 0u; index < 6u; ++index) {
        bool present = false;
        arbor_span entry;
        if (echo0_add_environment(
                environment_names[index], environment_storage[index],
                ECHO0_CONFIG_ENV_ENTRY_CAPACITY, &entry, &present) != 0) {
            fputs("ECHO0_CONFIG_ENVIRONMENT_ERROR\n", stderr);
            return 1;
        }
        if (present) {
            environment_entries[environment_count] = entry;
            ++environment_count;
        }
    }
    *sources_out = (arbor_config_sources){
        file_document, environment_entries, environment_count,
        command_line_entries, command_line_count
    };
    return 0;
}

static void echo0_host_diagnostic(
    void *context,
    arbor_linux_http_mvc_host_diagnostic diagnostic,
    int64_t native_status)
{
    (void)context;
    switch (diagnostic) {
    case ARBOR_LINUX_HTTP_MVC_HOST_DIAGNOSTIC_LISTEN:
        fprintf(stderr, "ECHO0_LISTEN_ERROR=%" PRId64 "\n", native_status);
        break;
    case ARBOR_LINUX_HTTP_MVC_HOST_DIAGNOSTIC_EVENT_LOOP_CREATE:
        fprintf(stderr, "ECHO0_EVENT_LOOP_ERROR=%" PRId64 "\n", native_status);
        break;
    case ARBOR_LINUX_HTTP_MVC_HOST_DIAGNOSTIC_EVENT_LOOP:
        fprintf(stderr, "ECHO0_EPOLL_ERROR=%" PRId64 "\n", native_status);
        break;
    case ARBOR_LINUX_HTTP_MVC_HOST_DIAGNOSTIC_ACCEPT:
        fprintf(stderr, "ECHO0_ACCEPT_ERROR=%" PRId64 "\n", native_status);
        break;
    case ARBOR_LINUX_HTTP_MVC_HOST_DIAGNOSTIC_CONNECTION:
        fprintf(stderr, "ECHO0_CONNECTION_ERROR=%" PRId64 "\n", native_status);
        break;
    case ARBOR_LINUX_HTTP_MVC_HOST_DIAGNOSTIC_CLOCK:
        fprintf(stderr, "ECHO0_CLOCK_ERROR=%" PRId64 "\n", native_status);
        break;
    case ARBOR_LINUX_HTTP_MVC_HOST_DIAGNOSTIC_CLOSE:
        fprintf(stderr, "ECHO0_CLOSE_ERROR=%" PRId64 "\n", native_status);
        break;
    default:
        break;
    }
}

static bool echo0_should_stop(void *context)
{
    (void)context;
    return echo0_stop_requested != 0;
}

int main(int argc, char **argv)
{
    static uint8_t config_file[ECHO0_CONFIG_FILE_CAPACITY];
    static arbor_span environment_entries[6];
    static char environment_storage[6][ECHO0_CONFIG_ENV_ENTRY_CAPACITY];
    static arbor_span command_line_entries[ECHO0_CONFIG_CLI_CAPACITY];
    static char legacy_template[ECHO0_CONFIG_ENV_ENTRY_CAPACITY];
    static char legacy_port[96];
    arbor_config_sources config_sources;
    if (echo0_acquire_configuration(
            argc, argv, &config_sources, config_file, environment_entries,
            environment_storage, command_line_entries, legacy_template,
            legacy_port) != 0) {
        fputs("ECHO0_CONFIG_ARGUMENT_ERROR\n", stderr);
        return 2;
    }

    arbor_config_diagnostic config_diagnostic = {0};
    arbor_config_requirements requirements;
    arbor_status status = arbor_config_measure(
        &echo0_config_schema, &config_sources, &requirements,
        &config_diagnostic);
    if (status.native != 0) {
        echo0_config_error(&config_diagnostic);
        return 2;
    }
    static arbor_config_value config_values[6];
    static arbor_config_provenance config_provenance[6];
    static uint8_t config_scratch[6];
    static uint8_t config_persistent[ECHO0_CONFIG_PERSISTENT_CAPACITY];
    const arbor_config_storage config_storage = {
        config_values, 6u, config_provenance, 6u,
        {config_scratch, sizeof(config_scratch)},
        {config_persistent, sizeof(config_persistent)}
    };
    arbor_config_result config_result;
    status = arbor_config_prepare(
        &echo0_config_schema, &config_sources, &config_storage,
        &config_result, &config_diagnostic);
    if (status.native != 0 ||
        arbor_config_validate(&echo0_config_schema, &config_result).native !=
            0) {
        echo0_config_error(&config_diagnostic);
        return 2;
    }
    if (requirements.persistent_bytes > sizeof(config_persistent)) {
        fputs("ECHO0_CONFIG_CAPACITY_ERROR\n", stderr);
        return 2;
    }

    char template_path[ECHO0_TEMPLATE_PATH_CAPACITY];
    char bind_ipv4[ECHO0_BIND_IPV4_CAPACITY];
    if (echo0_copy_text(
            config_values[0].utf8_value, template_path,
            sizeof(template_path)) != 0 ||
        echo0_copy_text(
            config_values[1].utf8_value, bind_ipv4, sizeof(bind_ipv4)) != 0) {
        fputs("ECHO0_CONFIG_MAPPING_ERROR\n", stderr);
        return 2;
    }
    struct sockaddr_in address;
    (void)memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons((uint16_t)config_values[2].u64_value);
    if (inet_pton(AF_INET, bind_ipv4, &address.sin_addr) != 1) {
        fputs("ECHO0_CONFIG_IPV4_ERROR\n", stderr);
        return 2;
    }

    static uint8_t template_source[ECHO0_TEMPLATE_SOURCE_CAPACITY];
    arbor_span source = {NULL, 0u};
    if (echo0_read_file(
            template_path, template_source, sizeof(template_source), true,
            &source) != 0) {
        fputs("ECHO0_TEMPLATE_READ_ERROR\n", stderr);
        return 1;
    }
    static echo0_web_application application;
    status = echo0_web_application_prepare(
        source, ECHO0_RESPONSE_FIELD_CAPACITY, &application);
    if (status.native != 0) {
        fprintf(stderr, "ECHO0_PREPARE_ERROR=%" PRId64 "\n", status.native);
        return 1;
    }
    (void)arbor_secure_clear(template_source, sizeof(template_source));

    if (echo0_install_signal_handlers() != 0) {
        fputs("ECHO0_SIGNAL_SETUP_ERROR\n", stderr);
        return 1;
    }
    static arbor_linux_http_mvc_host_slot slots[ECHO0_CONNECTION_SLOT_COUNT];
    static uint8_t slot_inputs
        [ECHO0_CONNECTION_SLOT_COUNT][ECHO0_CONNECTION_BUFFER_CAPACITY];
    static uint8_t slot_outputs
        [ECHO0_CONNECTION_SLOT_COUNT][ECHO0_CONNECTION_BUFFER_CAPACITY];
    static uint8_t slot_arenas
        [ECHO0_CONNECTION_SLOT_COUNT][ECHO0_CONNECTION_BUFFER_CAPACITY];
    for (size_t index = 0u; index < ECHO0_CONNECTION_SLOT_COUNT; ++index) {
        status = arbor_linux_http_mvc_host_slot_prepare(
            &slots[index],
            (arbor_mut_span){slot_inputs[index], sizeof(slot_inputs[index])},
            (arbor_mut_span){slot_outputs[index], sizeof(slot_outputs[index])},
            (arbor_mut_span){slot_arenas[index], sizeof(slot_arenas[index])});
        if (status.native != 0) {
            fputs("ECHO0_STORAGE_PREPARE_ERROR\n", stderr);
            return 1;
        }
    }

    static arbor_asm_epoll_event events[ECHO0_EVENT_CAPACITY];
    arbor_linux_http_mvc_host_options host_options;
    status = arbor_linux_http_mvc_host_options_make(
        &host_options, config_values[4].i64_value,
        config_values[5].u64_value, NULL, NULL,
        echo0_host_diagnostic, NULL);
    if (status.native != 0) {
        fprintf(stderr, "ECHO0_HOST_OPTIONS_ERROR=%" PRId64 "\n", status.native);
        return 1;
    }
    arbor_linux_http_mvc_host host = {0};
    status = arbor_linux_http_mvc_host_prepare(
        &host, &application.http_application, slots,
        ECHO0_CONNECTION_SLOT_COUNT, events, ECHO0_EVENT_CAPACITY,
        &host_options);
    if (status.native != 0) {
        fprintf(stderr, "ECHO0_HOST_PREPARE_ERROR=%" PRId64 "\n", status.native);
        return 1;
    }
    status = arbor_linux_http_mvc_host_open(
        &host, &address, sizeof(address), config_values[3].i64_value);
    if (status.native != 0) {
        return 1;
    }

    socklen_t address_length = (socklen_t)sizeof(address);
    if (getsockname(
            (int)host.listener_fd, (struct sockaddr *)(void *)&address,
            &address_length) != 0 ||
        address_length != (socklen_t)sizeof(address)) {
        fputs("ECHO0_LISTENER_ADDRESS_ERROR\n", stderr);
        (void)arbor_linux_http_mvc_host_close(&host);
        return 1;
    }
    uint16_t port = ntohs(address.sin_port);
    printf(
        "ECHO0_READY=http://%s:%" PRIu16 "/echo/Arborcore\n",
        bind_ipv4,
        port);
    (void)fflush(stdout);
    arbor_status run_status = arbor_linux_http_mvc_host_run(
        &host, echo0_should_stop, NULL);
    arbor_status close_status = arbor_linux_http_mvc_host_close(&host);
    arbor_linux_http_mvc_host_shutdown_result shutdown_result = {0};
    arbor_status result_status =
        arbor_linux_http_mvc_host_shutdown_result_get(&host, &shutdown_result);
    printf(
        "ECHO0_STOPPED middleware=%" PRIu64 " controller=%" PRIu64
        " service=%" PRIu64 " presenter=%" PRIu64 "\n",
        application.metrics.middleware_calls,
        application.metrics.controller_calls,
        application.metrics.service_calls,
        application.metrics.presenter_calls);
    if (result_status.native == 0) {
        printf(
            "ECHO0_LIFE0 phase=CLOSED active_at_drain_start=%" PRIu64
            " inactive_before_deadline=%" PRIu64
            " forced_at_deadline=%" PRIu64
            " deadline_expired=%u drain_start_ms=%" PRIu64
            " drain_finish_ms=%" PRIu64 " first_failure=%" PRId64 "\n",
            shutdown_result.active_at_drain_start,
            shutdown_result.inactive_before_deadline,
            shutdown_result.forced_at_deadline,
            shutdown_result.deadline_expired ? 1u : 0u,
            shutdown_result.drain_start_ms,
            shutdown_result.drain_finish_ms,
            shutdown_result.first_failure);
    }
    return run_status.native == 0 && close_status.native == 0 &&
        result_status.native == 0 && shutdown_result.first_failure == 0 ? 0 : 1;
}
