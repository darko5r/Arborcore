#ifndef ARBORCORE_CONFIG_H
#define ARBORCORE_CONFIG_H

#include <stdbool.h>
#include <stdint.h>

#include <arborcore/arborcore.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ARBOR_CONFIG_ABI_VERSION 1u
#define ARBOR_CONFIG_SCHEMA_KNOWN_FLAGS UINT64_C(0)
#define ARBOR_CONFIG_DESCRIPTOR_REQUIRED UINT64_C(1)
#define ARBOR_CONFIG_DESCRIPTOR_HAS_DEFAULT UINT64_C(2)
#define ARBOR_CONFIG_DESCRIPTOR_ALLOW_EMPTY UINT64_C(4)
#define ARBOR_CONFIG_DESCRIPTOR_KNOWN_FLAGS UINT64_C(7)

typedef enum arbor_config_source {
    ARBOR_CONFIG_SOURCE_NONE = 0,
    ARBOR_CONFIG_SOURCE_DEFAULT = 1,
    ARBOR_CONFIG_SOURCE_FILE = 2,
    ARBOR_CONFIG_SOURCE_ENVIRONMENT = 3,
    ARBOR_CONFIG_SOURCE_COMMAND_LINE = 4
} arbor_config_source;

typedef enum arbor_config_kind {
    ARBOR_CONFIG_KIND_BOOL = 1,
    ARBOR_CONFIG_KIND_U64 = 2,
    ARBOR_CONFIG_KIND_I64 = 3,
    ARBOR_CONFIG_KIND_UTF8 = 4,
    ARBOR_CONFIG_KIND_ENUM = 5
} arbor_config_kind;

typedef enum arbor_config_diagnostic_code {
    ARBOR_CONFIG_DIAGNOSTIC_NONE = 0,
    ARBOR_CONFIG_DIAGNOSTIC_SCHEMA_ABI = 1,
    ARBOR_CONFIG_DIAGNOSTIC_SCHEMA_FLAGS = 2,
    ARBOR_CONFIG_DIAGNOSTIC_SCHEMA_DESCRIPTOR = 3,
    ARBOR_CONFIG_DIAGNOSTIC_SCHEMA_NAME = 4,
    ARBOR_CONFIG_DIAGNOSTIC_SCHEMA_NAME_DUPLICATE = 5,
    ARBOR_CONFIG_DIAGNOSTIC_SCHEMA_DEFAULT = 6,
    ARBOR_CONFIG_DIAGNOSTIC_SOURCE_SYNTAX = 7,
    ARBOR_CONFIG_DIAGNOSTIC_SOURCE_NUL = 8,
    ARBOR_CONFIG_DIAGNOSTIC_UNKNOWN_KEY = 9,
    ARBOR_CONFIG_DIAGNOSTIC_DUPLICATE_KEY = 10,
    ARBOR_CONFIG_DIAGNOSTIC_VALUE_BOOL = 11,
    ARBOR_CONFIG_DIAGNOSTIC_VALUE_U64 = 12,
    ARBOR_CONFIG_DIAGNOSTIC_VALUE_I64 = 13,
    ARBOR_CONFIG_DIAGNOSTIC_VALUE_UTF8 = 14,
    ARBOR_CONFIG_DIAGNOSTIC_VALUE_ENUM = 15,
    ARBOR_CONFIG_DIAGNOSTIC_VALUE_BOUNDS = 16,
    ARBOR_CONFIG_DIAGNOSTIC_VALUE_LENGTH = 17,
    ARBOR_CONFIG_DIAGNOSTIC_VALUE_EMPTY = 18,
    ARBOR_CONFIG_DIAGNOSTIC_REQUIRED = 19,
    ARBOR_CONFIG_DIAGNOSTIC_CAPACITY = 20,
    ARBOR_CONFIG_DIAGNOSTIC_ALIAS = 21,
    ARBOR_CONFIG_DIAGNOSTIC_OVERFLOW = 22,
    ARBOR_CONFIG_DIAGNOSTIC_RESULT = 23
} arbor_config_diagnostic_code;

typedef struct arbor_config_enum_choice {
    arbor_span name;
    int64_t value;
} arbor_config_enum_choice;

typedef struct arbor_config_value {
    uint64_t u64_value;
    int64_t i64_value;
    arbor_span utf8_value;
    int64_t enum_value;
    arbor_config_kind kind;
    bool bool_value;
    uint8_t reserved0[3];
} arbor_config_value;

typedef struct arbor_config_names {
    arbor_span key;
    arbor_span file;
    arbor_span environment;
    arbor_span command_line;
} arbor_config_names;

typedef struct arbor_config_descriptor {
    arbor_config_names names;
    arbor_config_kind kind;
    uint32_t reserved0;
    uint64_t flags;
    arbor_config_value default_value;
    uint64_t u64_min;
    uint64_t u64_max;
    int64_t i64_min;
    int64_t i64_max;
    uint64_t utf8_max_length;
    const arbor_config_enum_choice *enum_choices;
    uint64_t enum_choice_count;
} arbor_config_descriptor;

typedef struct arbor_config_schema {
    uint32_t abi_version;
    uint32_t struct_size;
    uint64_t flags;
    const arbor_config_descriptor *descriptors;
    uint64_t descriptor_count;
} arbor_config_schema;

typedef struct arbor_config_sources {
    arbor_span file_document;
    const arbor_span *environment_entries;
    uint64_t environment_count;
    const arbor_span *command_line_entries;
    uint64_t command_line_count;
} arbor_config_sources;

typedef struct arbor_config_requirements {
    uint64_t descriptor_count;
    uint64_t descriptor_bytes;
    uint64_t value_bytes;
    uint64_t provenance_bytes;
    uint64_t scratch_bytes;
    uint64_t persistent_bytes;
    uint64_t result_bytes;
} arbor_config_requirements;

typedef struct arbor_config_provenance {
    arbor_config_source source;
    uint32_t reserved0;
    uint64_t ordinal;
    uint64_t byte_offset;
} arbor_config_provenance;

typedef struct arbor_config_diagnostic {
    arbor_config_diagnostic_code code;
    arbor_config_source source;
    uint64_t record;
    uint64_t byte_offset;
    uint64_t key_index;
} arbor_config_diagnostic;

typedef struct arbor_config_storage {
    arbor_config_value *values;
    uint64_t value_capacity;
    arbor_config_provenance *provenance;
    uint64_t provenance_capacity;
    arbor_mut_span scratch;
    arbor_mut_span persistent;
} arbor_config_storage;

typedef struct arbor_config_result {
    const arbor_config_value *values;
    const arbor_config_provenance *provenance;
    arbor_span persistent;
    uint64_t value_count;
    uint64_t prepared_guard;
} arbor_config_result;

arbor_status arbor_config_measure(
    const arbor_config_schema *schema,
    const arbor_config_sources *sources,
    arbor_config_requirements *requirements_out,
    arbor_config_diagnostic *diagnostic_out);

arbor_status arbor_config_prepare(
    const arbor_config_schema *schema,
    const arbor_config_sources *sources,
    const arbor_config_storage *storage,
    arbor_config_result *result_out,
    arbor_config_diagnostic *diagnostic_out);

arbor_status arbor_config_validate(
    const arbor_config_schema *schema,
    const arbor_config_result *result);

#ifdef __cplusplus
}
#endif

#endif
