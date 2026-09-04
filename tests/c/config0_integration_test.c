#include <arpa/inet.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <arborcore/config.h>

#define SPAN_LITERAL(text) \
    { (const uint8_t *)(text), (uint64_t)(sizeof(text) - 1u) }

static const arbor_config_descriptor application_descriptors[] = {
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

static const arbor_config_schema application_schema = {
    ARBOR_CONFIG_ABI_VERSION,
    sizeof(arbor_config_schema),
    0u,
    application_descriptors,
    6u
};

static uint64_t next_random(uint64_t *state)
{
    uint64_t value = *state;
    value ^= value << 13u;
    value ^= value >> 7u;
    value ^= value << 17u;
    *state = value;
    return value;
}

static bool span_text(arbor_span span, const char *text)
{
    size_t length = strlen(text);
    return span.length == (uint64_t)length &&
        memcmp(span.data, text, length) == 0;
}

int main(void)
{
    uint64_t random_state = UINT64_C(0x434f4e4649473052);
    for (uint64_t iteration = 0u; iteration < 200u; ++iteration) {
        uint64_t bits = next_random(&random_state);
        uint64_t file_port = bits % 60000u;
        int64_t file_backlog = (int64_t)(bits % 64u) + 1;
        uint64_t environment_port = (bits >> 8u) % 60000u;
        int64_t environment_backlog = (int64_t)((bits >> 16u) % 64u) + 1;
        uint64_t command_port = (bits >> 24u) % 60000u;
        int64_t command_backlog = (int64_t)((bits >> 32u) % 64u) + 1;

        char file[256];
        int file_count = snprintf(
            file, sizeof(file),
            "template=examples/hello0/page.html\nport=%" PRIu64
            "\nbacklog=%" PRId64 "\n",
            file_port, file_backlog);
        if (file_count < 0 || (size_t)file_count >= sizeof(file)) {
            return 1;
        }

        char environment_port_text[64];
        char environment_backlog_text[64];
        int environment_port_count = snprintf(
            environment_port_text, sizeof(environment_port_text),
            "ARBORCORE_PORT=%" PRIu64, environment_port);
        int environment_backlog_count = snprintf(
            environment_backlog_text, sizeof(environment_backlog_text),
            "ARBORCORE_BACKLOG=%" PRId64, environment_backlog);
        if (environment_port_count < 0 || environment_backlog_count < 0) {
            return 1;
        }
        arbor_span environment[2] = {
            {(const uint8_t *)environment_port_text,
             (uint64_t)environment_port_count},
            {(const uint8_t *)environment_backlog_text,
             (uint64_t)environment_backlog_count}
        };
        uint64_t environment_count = (bits & UINT64_C(1)) != 0u ? 2u : 0u;

        char command_port_text[64];
        char command_backlog_text[64];
        int command_port_count = snprintf(
            command_port_text, sizeof(command_port_text),
            "--port=%" PRIu64, command_port);
        int command_backlog_count = snprintf(
            command_backlog_text, sizeof(command_backlog_text),
            "--backlog=%" PRId64, command_backlog);
        if (command_port_count < 0 || command_backlog_count < 0) {
            return 1;
        }
        arbor_span command_line[2] = {
            {(const uint8_t *)command_port_text, (uint64_t)command_port_count},
            {(const uint8_t *)command_backlog_text,
             (uint64_t)command_backlog_count}
        };
        uint64_t command_count = (bits & UINT64_C(2)) != 0u ? 2u : 0u;
        const arbor_config_sources sources = {
            {(const uint8_t *)file, (uint64_t)file_count},
            environment, environment_count,
            command_line, command_count
        };

        arbor_config_requirements requirements = {0};
        arbor_config_diagnostic diagnostic = {
            ARBOR_CONFIG_DIAGNOSTIC_RESULT,
            ARBOR_CONFIG_SOURCE_NONE,
            77u, 78u, 79u
        };
        arbor_status status = arbor_config_measure(
            &application_schema, &sources, &requirements, &diagnostic);
        if (status.native != 0 || diagnostic.record != 77u ||
            requirements.descriptor_count != 6u) {
            return 1;
        }
        arbor_config_value values[6];
        arbor_config_provenance provenance[6];
        uint8_t scratch[6];
        uint8_t persistent[4110];
        const arbor_config_storage storage = {
            values, 6u, provenance, 6u,
            {scratch, sizeof(scratch)},
            {persistent, sizeof(persistent)}
        };
        arbor_config_result result = {0};
        status = arbor_config_prepare(
            &application_schema, &sources, &storage, &result, &diagnostic);
        uint64_t expected_port = command_count != 0u ? command_port :
            environment_count != 0u ? environment_port : file_port;
        int64_t expected_backlog = command_count != 0u ? command_backlog :
            environment_count != 0u ? environment_backlog : file_backlog;
        arbor_config_source expected_source = command_count != 0u ?
            ARBOR_CONFIG_SOURCE_COMMAND_LINE :
            environment_count != 0u ? ARBOR_CONFIG_SOURCE_ENVIRONMENT :
            ARBOR_CONFIG_SOURCE_FILE;
        if (status.native != 0 ||
            !span_text(values[0].utf8_value, "examples/hello0/page.html") ||
            !span_text(values[1].utf8_value, "127.0.0.1") ||
            values[2].u64_value != expected_port ||
            values[3].i64_value != expected_backlog ||
            values[4].i64_value != 250 ||
            values[5].u64_value != 2000u ||
            provenance[2].source != expected_source ||
            provenance[3].source != expected_source ||
            arbor_config_validate(&application_schema, &result).native != 0) {
            return 1;
        }
        char address_text[16];
        if (values[1].utf8_value.length >= sizeof(address_text)) {
            return 1;
        }
        (void)memcpy(
            address_text, values[1].utf8_value.data,
            (size_t)values[1].utf8_value.length);
        address_text[values[1].utf8_value.length] = '\0';
        struct in_addr address;
        if (inet_pton(AF_INET, address_text, &address) != 1) {
            return 1;
        }
    }

    static const arbor_span bad_cli[] = {SPAN_LITERAL("--port=65536")};
    const arbor_config_sources bad_sources = {
        SPAN_LITERAL("template=examples/hello0/page.html"),
        NULL, 0u, bad_cli, 1u
    };
    arbor_config_requirements requirements = {0};
    arbor_config_diagnostic diagnostic = {0};
    arbor_status status = arbor_config_measure(
        &application_schema, &bad_sources, &requirements, &diagnostic);
    if (status.native == 0 ||
        diagnostic.code != ARBOR_CONFIG_DIAGNOSTIC_VALUE_BOUNDS ||
        diagnostic.source != ARBOR_CONFIG_SOURCE_COMMAND_LINE) {
        return 1;
    }

    puts("CONFIG0_APPLICATION_SCHEMA=PASS_6_EXACT_SHARED_KEYS");
    puts("CONFIG0_FIXED_SEED_DIFFERENTIAL=PASS_200_OF_200");
    puts("CONFIG0_IPV4_PRE_RESOURCE_MAPPING=PASS_NUMERIC_ONLY");
    puts("PASS: CONFIG0 HELLO0/ECHO0 schema, precedence and fixed-seed reference evidence");
    return 0;
}
