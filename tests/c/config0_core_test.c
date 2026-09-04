#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <arborcore/config.h>

#define SPAN_LITERAL(text) \
    { (const uint8_t *)(text), (uint64_t)(sizeof(text) - 1u) }

static const arbor_config_enum_choice choices[] = {
    {SPAN_LITERAL("red"), 7},
    {SPAN_LITERAL("blue"), 9}
};

static const arbor_config_descriptor descriptors[] = {
    {
        {SPAN_LITERAL("flag"), SPAN_LITERAL("flag"),
         SPAN_LITERAL("CFG_FLAG"), SPAN_LITERAL("flag")},
        ARBOR_CONFIG_KIND_BOOL, 0u, ARBOR_CONFIG_DESCRIPTOR_HAS_DEFAULT,
        {0u, 0, {NULL, 0u}, 0, ARBOR_CONFIG_KIND_BOOL, false, {0u, 0u, 0u}},
        0u, 0u, 0, 0, 0u, NULL, 0u
    },
    {
        {SPAN_LITERAL("number"), SPAN_LITERAL("number"),
         SPAN_LITERAL("CFG_NUMBER"), SPAN_LITERAL("number")},
        ARBOR_CONFIG_KIND_U64, 0u, ARBOR_CONFIG_DESCRIPTOR_REQUIRED,
        {0}, 0u, 1000u, 0, 0, 0u, NULL, 0u
    },
    {
        {SPAN_LITERAL("signed"), SPAN_LITERAL("signed"),
         SPAN_LITERAL("CFG_SIGNED"), SPAN_LITERAL("signed")},
        ARBOR_CONFIG_KIND_I64, 0u, ARBOR_CONFIG_DESCRIPTOR_HAS_DEFAULT,
        {0u, -5, {NULL, 0u}, 0, ARBOR_CONFIG_KIND_I64, false, {0u, 0u, 0u}},
        0u, 0u, -10, 10, 0u, NULL, 0u
    },
    {
        {SPAN_LITERAL("text"), SPAN_LITERAL("text"),
         SPAN_LITERAL("CFG_TEXT"), SPAN_LITERAL("text")},
        ARBOR_CONFIG_KIND_UTF8, 0u, ARBOR_CONFIG_DESCRIPTOR_HAS_DEFAULT,
        {0u, 0, SPAN_LITERAL("base"), 0, ARBOR_CONFIG_KIND_UTF8, false,
         {0u, 0u, 0u}},
        0u, 0u, 0, 0, 16u, NULL, 0u
    },
    {
        {SPAN_LITERAL("mode"), SPAN_LITERAL("mode"),
         SPAN_LITERAL("CFG_MODE"), SPAN_LITERAL("mode")},
        ARBOR_CONFIG_KIND_ENUM, 0u, ARBOR_CONFIG_DESCRIPTOR_HAS_DEFAULT,
        {0u, 0, {NULL, 0u}, 7, ARBOR_CONFIG_KIND_ENUM, false,
         {0u, 0u, 0u}},
        0u, 0u, 0, 0, 0u, choices, 2u
    }
};

static const arbor_config_schema schema = {
    ARBOR_CONFIG_ABI_VERSION,
    sizeof(arbor_config_schema),
    ARBOR_CONFIG_SCHEMA_KNOWN_FLAGS,
    descriptors,
    5u
};

static bool span_equals(arbor_span value, const char *text)
{
    size_t length = strlen(text);
    return value.length == (uint64_t)length &&
        memcmp(value.data, text, length) == 0;
}

int main(void)
{
    static const uint8_t file_bytes[] =
        "number=0010\r\n# carried record\ntext=file\n";
    static const arbor_span environment[] = {
        SPAN_LITERAL("CFG_TEXT=env"),
        SPAN_LITERAL("CFG_MODE=blue")
    };
    static const arbor_span command_line[] = {
        SPAN_LITERAL("--flag=true"),
        SPAN_LITERAL("--text=cli")
    };
    const arbor_config_sources sources = {
        {file_bytes, sizeof(file_bytes) - 1u},
        environment,
        2u,
        command_line,
        2u
    };
    arbor_config_requirements requirements = {0};
    arbor_config_diagnostic untouched = {
        ARBOR_CONFIG_DIAGNOSTIC_RESULT,
        ARBOR_CONFIG_SOURCE_COMMAND_LINE,
        91u,
        92u,
        93u
    };
    arbor_status status = arbor_config_measure(
        &schema, &sources, &requirements, &untouched);
    if (status.native != 0 || requirements.descriptor_count != 5u ||
        requirements.descriptor_bytes != 5u * 184u ||
        requirements.value_bytes != 5u * 48u ||
        requirements.provenance_bytes != 5u * 24u ||
        requirements.scratch_bytes != 5u ||
        requirements.persistent_bytes != 3u ||
        requirements.result_bytes != 48u ||
        untouched.record != 91u) {
        return 1;
    }

    arbor_config_value values[7];
    arbor_config_provenance provenance[7];
    uint8_t scratch[7];
    uint8_t persistent[8];
    (void)memset(values, 0xa5, sizeof(values));
    (void)memset(provenance, 0xa5, sizeof(provenance));
    (void)memset(scratch, 0xa5, sizeof(scratch));
    (void)memset(persistent, 0xa5, sizeof(persistent));
    const arbor_config_storage storage = {
        values, 7u, provenance, 7u,
        {scratch, sizeof(scratch)},
        {persistent, sizeof(persistent)}
    };
    arbor_config_result result = {0};
    status = arbor_config_prepare(
        &schema, &sources, &storage, &result, &untouched);
    if (status.native != 0 || result.value_count != 5u ||
        !values[0].bool_value || values[1].u64_value != 10u ||
        values[2].i64_value != -5 || !span_equals(values[3].utf8_value, "cli") ||
        values[4].enum_value != 9 ||
        provenance[0].source != ARBOR_CONFIG_SOURCE_COMMAND_LINE ||
        provenance[1].source != ARBOR_CONFIG_SOURCE_FILE ||
        provenance[2].source != ARBOR_CONFIG_SOURCE_DEFAULT ||
        provenance[3].source != ARBOR_CONFIG_SOURCE_COMMAND_LINE ||
        provenance[4].source != ARBOR_CONFIG_SOURCE_ENVIRONMENT ||
        result.persistent.length != 3u || persistent[3] != UINT8_C(0xa5) ||
        values[5].kind != (arbor_config_kind)UINT32_C(0xa5a5a5a5) ||
        arbor_config_validate(&schema, &result).native != 0) {
        return 1;
    }

    arbor_config_result corrupt = result;
    corrupt.prepared_guard ^= UINT64_C(1);
    if (arbor_config_validate(&schema, &corrupt).native == 0) {
        return 1;
    }

    printf(
        "CONFIG0_PRECEDENCE=PASS_DEFAULT_FILE_ENVIRONMENT_COMMAND_LINE\n"
        "CONFIG0_MEASUREMENT=PASS_%" PRIu64 "_DESCRIPTORS_%" PRIu64
        "_PERSISTENT_BYTES\n"
        "PASS: CONFIG0 schemas, five value kinds, exact provenance and "
        "failure-atomic publication\n",
        requirements.descriptor_count,
        requirements.persistent_bytes);
    return 0;
}
