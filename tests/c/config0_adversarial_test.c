#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <arborcore/config.h>

#define SPAN_LITERAL(text) \
    { (const uint8_t *)(text), (uint64_t)(sizeof(text) - 1u) }
#define TEST_FAILURE() \
    do { fprintf(stderr, "CONFIG0_ADVERSARIAL_FAILURE=%d\n", __LINE__); return 1; } while (false)

static arbor_config_descriptor required_u64(void)
{
    const arbor_config_descriptor descriptor = {
        {SPAN_LITERAL("count"), SPAN_LITERAL("count"),
         SPAN_LITERAL("COUNT"), SPAN_LITERAL("count")},
        ARBOR_CONFIG_KIND_U64,
        0u,
        ARBOR_CONFIG_DESCRIPTOR_REQUIRED,
        {0},
        0u,
        UINT64_MAX,
        0,
        0,
        0u,
        NULL,
        0u
    };
    return descriptor;
}

static bool bytes_are(const void *bytes, size_t length, uint8_t expected)
{
    const uint8_t *cursor = bytes;
    for (size_t index = 0u; index < length; ++index) {
        if (cursor[index] != expected) {
            return false;
        }
    }
    return true;
}

int main(void)
{
    arbor_config_descriptor descriptor = required_u64();
    arbor_config_schema schema = {
        ARBOR_CONFIG_ABI_VERSION,
        sizeof(arbor_config_schema),
        0u,
        &descriptor,
        1u
    };
    arbor_config_sources sources = {{NULL, 0u}, NULL, 0u, NULL, 0u};
    arbor_config_requirements requirements;
    (void)memset(&requirements, 0x5a, sizeof(requirements));
    arbor_config_diagnostic diagnostic = {0};
    arbor_status status = arbor_config_measure(
        &schema, &sources, &requirements, &diagnostic);
    if (status.code != ARBOR_STATUS_NOT_FOUND ||
        diagnostic.code != ARBOR_CONFIG_DIAGNOSTIC_REQUIRED ||
        diagnostic.key_index != 0u ||
        !bytes_are(&requirements, sizeof(requirements), UINT8_C(0x5a))) {
        TEST_FAILURE();
    }

    arbor_config_schema schema_snapshot = schema;
    status = arbor_config_measure(
        &schema, &sources, &requirements,
        (arbor_config_diagnostic *)(void *)&schema);
    if (status.code != ARBOR_STATUS_NOT_FOUND ||
        memcmp(&schema, &schema_snapshot, sizeof(schema)) != 0 ||
        !bytes_are(&requirements, sizeof(requirements), UINT8_C(0x5a))) {
        TEST_FAILURE();
    }

    static const arbor_span valid_cli[] = {SPAN_LITERAL("--count=42")};
    sources.command_line_entries = valid_cli;
    sources.command_line_count = 1u;
    status = arbor_config_measure(&schema, &sources, &requirements, NULL);
    if (status.native != 0) {
        TEST_FAILURE();
    }

    arbor_config_value value;
    arbor_config_provenance provenance;
    uint8_t scratch = UINT8_C(0x7c);
    uint8_t persistent = UINT8_C(0x7c);
    arbor_config_storage storage = {
        &value, 0u, &provenance, 1u,
        {&scratch, 1u}, {&persistent, 1u}
    };
    arbor_config_result result;
    (void)memset(&value, 0x7c, sizeof(value));
    (void)memset(&provenance, 0x7c, sizeof(provenance));
    (void)memset(&result, 0x7c, sizeof(result));
    status = arbor_config_prepare(
        &schema, &sources, &storage, &result, &diagnostic);
    if (status.code != ARBOR_STATUS_NO_SPACE ||
        diagnostic.code != ARBOR_CONFIG_DIAGNOSTIC_CAPACITY ||
        !bytes_are(&value, sizeof(value), UINT8_C(0x7c)) ||
        !bytes_are(&result, sizeof(result), UINT8_C(0x7c)) ||
        scratch != UINT8_C(0x7c) || persistent != UINT8_C(0x7c)) {
        TEST_FAILURE();
    }

    storage.value_capacity = 1u;
    storage.provenance = (arbor_config_provenance *)(void *)&value;
    status = arbor_config_prepare(
        &schema, &sources, &storage, &result, &diagnostic);
    if (status.code != ARBOR_STATUS_INVALID_ARGUMENT ||
        diagnostic.code != ARBOR_CONFIG_DIAGNOSTIC_ALIAS) {
        TEST_FAILURE();
    }
    storage.provenance = &provenance;

    static const arbor_span malformed[] = {
        SPAN_LITERAL("--count=+1"),
        SPAN_LITERAL("--count=18446744073709551616"),
        SPAN_LITERAL("count=1"),
        SPAN_LITERAL("--unknown=1")
    };
    const arbor_config_diagnostic_code expected[] = {
        ARBOR_CONFIG_DIAGNOSTIC_VALUE_U64,
        ARBOR_CONFIG_DIAGNOSTIC_VALUE_U64,
        ARBOR_CONFIG_DIAGNOSTIC_SOURCE_SYNTAX,
        ARBOR_CONFIG_DIAGNOSTIC_UNKNOWN_KEY
    };
    for (uint64_t index = 0u; index < 4u; ++index) {
        sources.command_line_entries = &malformed[index];
        status = arbor_config_measure(
            &schema, &sources, &requirements, &diagnostic);
        if (status.native == 0 || diagnostic.code != expected[index]) {
            TEST_FAILURE();
        }
    }

    union {
        uint8_t bytes[64];
        arbor_config_diagnostic alignment;
    } aliased_source = {{0}};
    static const char bad_value[] = "--count=+1";
    (void)memcpy(
        aliased_source.bytes, bad_value, sizeof(bad_value) - 1u);
    uint8_t aliased_snapshot[sizeof(aliased_source.bytes)];
    (void)memcpy(
        aliased_snapshot, aliased_source.bytes, sizeof(aliased_snapshot));
    const arbor_span aliased_entry = {
        aliased_source.bytes, sizeof(bad_value) - 1u
    };
    sources.command_line_entries = &aliased_entry;
    sources.command_line_count = 1u;
    status = arbor_config_measure(
        &schema, &sources, &requirements,
        (arbor_config_diagnostic *)(void *)aliased_source.bytes);
    if (status.native == 0 ||
        memcmp(
            aliased_source.bytes, aliased_snapshot,
            sizeof(aliased_snapshot)) != 0) {
        TEST_FAILURE();
    }

    static const uint8_t nul_file[] = {'c','o','u','n','t','=',0,'1'};
    sources.command_line_entries = NULL;
    sources.command_line_count = 0u;
    sources.file_document = (arbor_span){nul_file, sizeof(nul_file)};
    status = arbor_config_measure(
        &schema, &sources, &requirements, &diagnostic);
    if (status.native == 0 ||
        diagnostic.code != ARBOR_CONFIG_DIAGNOSTIC_SOURCE_NUL ||
        diagnostic.byte_offset != 6u) {
        TEST_FAILURE();
    }

    descriptor.flags = ARBOR_CONFIG_DESCRIPTOR_REQUIRED |
        ARBOR_CONFIG_DESCRIPTOR_HAS_DEFAULT;
    sources.file_document = (arbor_span){NULL, 0u};
    status = arbor_config_measure(
        &schema, &sources, &requirements, &diagnostic);
    if (status.native == 0 ||
        diagnostic.code != ARBOR_CONFIG_DIAGNOSTIC_SCHEMA_FLAGS) {
        TEST_FAILURE();
    }

    descriptor = required_u64();
    static const arbor_span duplicate[] = {
        SPAN_LITERAL("COUNT=1"), SPAN_LITERAL("COUNT=2")
    };
    sources.environment_entries = duplicate;
    sources.environment_count = 2u;
    status = arbor_config_measure(
        &schema, &sources, &requirements, &diagnostic);
    if (status.native == 0 ||
        diagnostic.code != ARBOR_CONFIG_DIAGNOSTIC_DUPLICATE_KEY ||
        diagnostic.record != 1u) {
        TEST_FAILURE();
    }

    descriptor = (arbor_config_descriptor){
        {SPAN_LITERAL("text"), SPAN_LITERAL("text"),
         SPAN_LITERAL("TEXT"), SPAN_LITERAL("text")},
        ARBOR_CONFIG_KIND_UTF8,
        0u,
        ARBOR_CONFIG_DESCRIPTOR_REQUIRED | ARBOR_CONFIG_DESCRIPTOR_ALLOW_EMPTY,
        {0},
        0u,
        0u,
        0,
        0,
        4u,
        NULL,
        0u
    };
    sources.environment_entries = NULL;
    sources.environment_count = 0u;
    uint8_t byte_document[6] = {'t','e','x','t','=',0};
    for (uint64_t byte = 0u; byte <= UINT8_MAX; ++byte) {
        byte_document[5] = (uint8_t)byte;
        sources.file_document = (arbor_span){byte_document, sizeof(byte_document)};
        arbor_config_diagnostic first = {0};
        arbor_config_diagnostic second = {0};
        arbor_config_requirements first_requirements = {0};
        arbor_config_requirements second_requirements = {0};
        arbor_status first_status = arbor_config_measure(
            &schema, &sources, &first_requirements, &first);
        arbor_status second_status = arbor_config_measure(
            &schema, &sources, &second_requirements, &second);
        if (first_status.native != second_status.native ||
            (first_status.native == 0 &&
             memcmp(
                 &first_requirements, &second_requirements,
                 sizeof(first_requirements)) != 0) ||
            (first_status.native != 0 &&
             memcmp(&first, &second, sizeof(first)) != 0)) {
            TEST_FAILURE();
        }
    }

    static const uint8_t unicode_document[] = {
        't','e','x','t','=',UINT8_C(0xf0),UINT8_C(0x9f),
        UINT8_C(0x98),UINT8_C(0x80),'\n'
    };
    for (uint64_t length = 0u; length <= sizeof(unicode_document); ++length) {
        sources.file_document = (arbor_span){unicode_document, length};
        arbor_config_diagnostic first = {0};
        arbor_config_diagnostic second = {0};
        arbor_status first_status = arbor_config_measure(
            &schema, &sources, &requirements, &first);
        arbor_status second_status = arbor_config_measure(
            &schema, &sources, &requirements, &second);
        if (first_status.native != second_status.native ||
            (first_status.native != 0 &&
             memcmp(&first, &second, sizeof(first)) != 0)) {
            TEST_FAILURE();
        }
    }

    puts("CONFIG0_GRAMMAR_BOUNDARIES=PASS_NUL_SYNTAX_DUPLICATE_UNKNOWN");
    puts("CONFIG0_GRAMMAR_BYTE_CORPUS=PASS_256_OF_256");
    puts("CONFIG0_TRUNCATION_CORPUS=PASS_ALL_BOUNDED_PREFIXES");
    puts("CONFIG0_FAILURE_ATOMICITY=PASS_REQUIREMENTS_RESULT_VALUES_STORAGE_DIAGNOSTIC");
    puts("PASS: CONFIG0 malformed schema, grammar, capacity, alias and overflow evidence");
    return 0;
}
