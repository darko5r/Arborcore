#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <arborcore/config.h>

#define CONFIG_PREPARED_GUARD UINT64_C(0x434f4e4649473052)
#define CONFIG_NA UINT64_MAX

typedef enum config_namespace {
    CONFIG_NAMESPACE_KEY,
    CONFIG_NAMESPACE_FILE,
    CONFIG_NAMESPACE_ENVIRONMENT,
    CONFIG_NAMESPACE_COMMAND_LINE
} config_namespace;

typedef struct config_range {
    uintptr_t begin;
    uintptr_t end;
    bool empty;
} config_range;

typedef struct config_record {
    arbor_span name;
    arbor_span value;
    uint64_t ordinal;
    uint64_t name_offset;
    uint64_t value_offset;
} config_record;

typedef struct config_resolution {
    arbor_config_value value;
    arbor_config_provenance provenance;
    const uint8_t *utf8_source;
    uint64_t utf8_length;
    bool present;
} config_resolution;

static arbor_status config_status(arbor_status_code code, int native)
{
    arbor_status status = {code, (int64_t)native};
    return status;
}

static arbor_status config_ok(void)
{
    return config_status(ARBOR_STATUS_OK, 0);
}

static arbor_status config_invalid(void)
{
    return config_status(ARBOR_STATUS_INVALID_ARGUMENT, -EINVAL);
}

static arbor_status config_not_found(void)
{
    return config_status(ARBOR_STATUS_NOT_FOUND, -ENOENT);
}

static arbor_status config_no_space(void)
{
    return config_status(ARBOR_STATUS_NO_SPACE, -ENOSPC);
}

static arbor_status config_overflow(void)
{
    return config_status(ARBOR_STATUS_OVERFLOW, -EOVERFLOW);
}

static void config_diagnostic_clear(arbor_config_diagnostic *diagnostic)
{
    *diagnostic = (arbor_config_diagnostic){
        ARBOR_CONFIG_DIAGNOSTIC_NONE,
        ARBOR_CONFIG_SOURCE_NONE,
        CONFIG_NA,
        CONFIG_NA,
        CONFIG_NA
    };
}

static arbor_status config_fail(
    arbor_config_diagnostic *diagnostic,
    arbor_config_diagnostic_code code,
    arbor_config_source source,
    uint64_t record,
    uint64_t byte_offset,
    uint64_t key_index,
    arbor_status status)
{
    *diagnostic = (arbor_config_diagnostic){
        code,
        source,
        record,
        byte_offset,
        key_index
    };
    return status;
}

static bool config_u64_multiply(
    uint64_t left,
    uint64_t right,
    uint64_t *product_out)
{
    if (left != 0u && right > UINT64_MAX / left) {
        return false;
    }
    *product_out = left * right;
    return true;
}

static bool config_u64_add(
    uint64_t left,
    uint64_t right,
    uint64_t *sum_out)
{
    if (right > UINT64_MAX - left) {
        return false;
    }
    *sum_out = left + right;
    return true;
}

static bool config_range_make(
    const void *pointer,
    uint64_t length,
    config_range *range_out)
{
    if (length == 0u) {
        *range_out = (config_range){0u, 0u, true};
        return true;
    }
    if (pointer == NULL || length > (uint64_t)UINTPTR_MAX) {
        return false;
    }
    uintptr_t begin = (uintptr_t)pointer;
    uintptr_t size = (uintptr_t)length;
    if (begin > UINTPTR_MAX - size) {
        return false;
    }
    *range_out = (config_range){begin, begin + size, false};
    return true;
}

static bool config_array_range(
    const void *pointer,
    uint64_t count,
    uint64_t element_size,
    config_range *range_out)
{
    uint64_t bytes = 0u;
    return config_u64_multiply(count, element_size, &bytes) &&
        config_range_make(pointer, bytes, range_out);
}

static bool config_ranges_overlap(config_range left, config_range right)
{
    return !left.empty && !right.empty &&
        left.begin < right.end && right.begin < left.end;
}

static bool config_span_equal(arbor_span left, arbor_span right)
{
    return left.length == right.length &&
        (left.length == 0u ||
         memcmp(left.data, right.data, (size_t)left.length) == 0);
}

static bool config_span_literal(arbor_span span, const char *literal)
{
    size_t length = strlen(literal);
    return span.length == (uint64_t)length &&
        (length == 0u || memcmp(span.data, literal, length) == 0);
}

static bool config_reserved_zero(const uint8_t reserved[3])
{
    return reserved[0] == 0u && reserved[1] == 0u && reserved[2] == 0u;
}

static bool config_span_inactive(arbor_span span)
{
    return span.data == NULL && span.length == 0u;
}

static bool config_name_valid(
    arbor_span name,
    bool environment,
    bool enumeration,
    uint64_t *bad_offset_out)
{
    config_range range;
    if (name.length == 0u || name.length > 63u ||
        !config_range_make(name.data, name.length, &range)) {
        *bad_offset_out = 0u;
        return false;
    }
    for (uint64_t index = 0u; index < name.length; ++index) {
        uint8_t byte = name.data[index];
        bool upper = byte >= (uint8_t)'A' && byte <= (uint8_t)'Z';
        bool lower = byte >= (uint8_t)'a' && byte <= (uint8_t)'z';
        bool digit = byte >= (uint8_t)'0' && byte <= (uint8_t)'9';
        bool valid;
        if (index == 0u) {
            valid = enumeration ? upper || lower || digit :
                upper || lower || byte == (uint8_t)'_';
        } else if (environment) {
            valid = upper || lower || digit || byte == (uint8_t)'_';
        } else {
            valid = upper || lower || digit || byte == (uint8_t)'_' ||
                byte == (uint8_t)'.' || byte == (uint8_t)'-';
        }
        if (!valid) {
            *bad_offset_out = index;
            return false;
        }
    }
    return true;
}

static bool config_utf8_valid(arbor_span text, uint64_t *bad_offset_out)
{
    config_range range;
    if (!config_range_make(text.data, text.length, &range)) {
        *bad_offset_out = 0u;
        return false;
    }
    uint64_t index = 0u;
    while (index < text.length) {
        uint8_t first = text.data[index];
        if (first == 0u) {
            *bad_offset_out = index;
            return false;
        }
        if (first <= UINT8_C(0x7f)) {
            ++index;
            continue;
        }
        uint64_t needed;
        uint32_t scalar;
        uint32_t minimum;
        if (first >= UINT8_C(0xc2) && first <= UINT8_C(0xdf)) {
            needed = 2u;
            scalar = (uint32_t)(first & UINT8_C(0x1f));
            minimum = UINT32_C(0x80);
        } else if (first >= UINT8_C(0xe0) && first <= UINT8_C(0xef)) {
            needed = 3u;
            scalar = (uint32_t)(first & UINT8_C(0x0f));
            minimum = UINT32_C(0x800);
        } else if (first >= UINT8_C(0xf0) && first <= UINT8_C(0xf4)) {
            needed = 4u;
            scalar = (uint32_t)(first & UINT8_C(0x07));
            minimum = UINT32_C(0x10000);
        } else {
            *bad_offset_out = index;
            return false;
        }
        if (needed > text.length - index) {
            *bad_offset_out = index;
            return false;
        }
        for (uint64_t part = 1u; part < needed; ++part) {
            uint8_t byte = text.data[index + part];
            if ((byte & UINT8_C(0xc0)) != UINT8_C(0x80)) {
                *bad_offset_out = index + part;
                return false;
            }
            scalar = (scalar << 6u) | (uint32_t)(byte & UINT8_C(0x3f));
        }
        if (scalar < minimum || scalar > UINT32_C(0x10ffff) ||
            (scalar >= UINT32_C(0xd800) && scalar <= UINT32_C(0xdfff))) {
            *bad_offset_out = index;
            return false;
        }
        index += needed;
    }
    return true;
}

static arbor_span config_descriptor_name(
    const arbor_config_descriptor *descriptor,
    config_namespace name_space)
{
    switch (name_space) {
    case CONFIG_NAMESPACE_KEY:
        return descriptor->names.key;
    case CONFIG_NAMESPACE_FILE:
        return descriptor->names.file;
    case CONFIG_NAMESPACE_ENVIRONMENT:
        return descriptor->names.environment;
    case CONFIG_NAMESPACE_COMMAND_LINE:
        return descriptor->names.command_line;
    }
    return (arbor_span){NULL, 0u};
}

static uint64_t config_find_descriptor(
    const arbor_config_schema *schema,
    config_namespace name_space,
    arbor_span name)
{
    for (uint64_t index = 0u; index < schema->descriptor_count; ++index) {
        if (config_span_equal(
                config_descriptor_name(&schema->descriptors[index], name_space),
                name)) {
            return index;
        }
    }
    return CONFIG_NA;
}

static bool config_value_zero(const arbor_config_value *value)
{
    static const uint8_t zero[sizeof(arbor_config_value)] = {0};
    return memcmp(value, zero, sizeof(*value)) == 0;
}

static bool config_value_canonical(
    const arbor_config_descriptor *descriptor,
    const arbor_config_value *value,
    bool validate_utf8,
    uint64_t *bad_offset_out,
    arbor_config_diagnostic_code *failure_out)
{
    if (value->kind != descriptor->kind ||
        !config_reserved_zero(value->reserved0)) {
        *failure_out = ARBOR_CONFIG_DIAGNOSTIC_SCHEMA_DEFAULT;
        *bad_offset_out = 0u;
        return false;
    }
    switch (descriptor->kind) {
    case ARBOR_CONFIG_KIND_BOOL:
        if (value->u64_value != 0u || value->i64_value != 0 ||
            !config_span_inactive(value->utf8_value) ||
            value->enum_value != 0) {
            *failure_out = ARBOR_CONFIG_DIAGNOSTIC_SCHEMA_DEFAULT;
            *bad_offset_out = 0u;
            return false;
        }
        return true;
    case ARBOR_CONFIG_KIND_U64:
        if (value->i64_value != 0 || !config_span_inactive(value->utf8_value) ||
            value->enum_value != 0 || value->bool_value) {
            *failure_out = ARBOR_CONFIG_DIAGNOSTIC_SCHEMA_DEFAULT;
            *bad_offset_out = 0u;
            return false;
        }
        if (value->u64_value < descriptor->u64_min ||
            value->u64_value > descriptor->u64_max) {
            *failure_out = ARBOR_CONFIG_DIAGNOSTIC_VALUE_BOUNDS;
            *bad_offset_out = 0u;
            return false;
        }
        return true;
    case ARBOR_CONFIG_KIND_I64:
        if (value->u64_value != 0u || !config_span_inactive(value->utf8_value) ||
            value->enum_value != 0 || value->bool_value) {
            *failure_out = ARBOR_CONFIG_DIAGNOSTIC_SCHEMA_DEFAULT;
            *bad_offset_out = 0u;
            return false;
        }
        if (value->i64_value < descriptor->i64_min ||
            value->i64_value > descriptor->i64_max) {
            *failure_out = ARBOR_CONFIG_DIAGNOSTIC_VALUE_BOUNDS;
            *bad_offset_out = 0u;
            return false;
        }
        return true;
    case ARBOR_CONFIG_KIND_UTF8:
        if (value->u64_value != 0u || value->i64_value != 0 ||
            value->enum_value != 0 || value->bool_value) {
            *failure_out = ARBOR_CONFIG_DIAGNOSTIC_SCHEMA_DEFAULT;
            *bad_offset_out = 0u;
            return false;
        }
        if (value->utf8_value.length == 0u &&
            (descriptor->flags & ARBOR_CONFIG_DESCRIPTOR_ALLOW_EMPTY) == 0u) {
            *failure_out = ARBOR_CONFIG_DIAGNOSTIC_VALUE_EMPTY;
            *bad_offset_out = 0u;
            return false;
        }
        if (value->utf8_value.length > descriptor->utf8_max_length) {
            *failure_out = ARBOR_CONFIG_DIAGNOSTIC_VALUE_LENGTH;
            *bad_offset_out = 0u;
            return false;
        }
        if (validate_utf8 &&
            !config_utf8_valid(value->utf8_value, bad_offset_out)) {
            *failure_out = ARBOR_CONFIG_DIAGNOSTIC_VALUE_UTF8;
            return false;
        }
        return true;
    case ARBOR_CONFIG_KIND_ENUM:
        if (value->u64_value != 0u || value->i64_value != 0 ||
            !config_span_inactive(value->utf8_value) || value->bool_value) {
            *failure_out = ARBOR_CONFIG_DIAGNOSTIC_SCHEMA_DEFAULT;
            *bad_offset_out = 0u;
            return false;
        }
        for (uint64_t index = 0u; index < descriptor->enum_choice_count;
             ++index) {
            if (descriptor->enum_choices[index].value == value->enum_value) {
                return true;
            }
        }
        *failure_out = ARBOR_CONFIG_DIAGNOSTIC_VALUE_ENUM;
        *bad_offset_out = 0u;
        return false;
    }
    *failure_out = ARBOR_CONFIG_DIAGNOSTIC_SCHEMA_DEFAULT;
    *bad_offset_out = 0u;
    return false;
}

static arbor_status config_validate_schema(
    const arbor_config_schema *schema,
    arbor_config_diagnostic *diagnostic)
{
    config_range schema_range;
    if (!config_range_make(schema, sizeof(*schema), &schema_range)) {
        return config_fail(
            diagnostic, ARBOR_CONFIG_DIAGNOSTIC_SCHEMA_ABI,
            ARBOR_CONFIG_SOURCE_NONE, CONFIG_NA, 0u, CONFIG_NA,
            config_invalid());
    }
    if (schema->abi_version != ARBOR_CONFIG_ABI_VERSION ||
        schema->struct_size != (uint32_t)sizeof(*schema)) {
        return config_fail(
            diagnostic, ARBOR_CONFIG_DIAGNOSTIC_SCHEMA_ABI,
            ARBOR_CONFIG_SOURCE_NONE, CONFIG_NA, 0u, CONFIG_NA,
            config_invalid());
    }
    if (schema->flags != ARBOR_CONFIG_SCHEMA_KNOWN_FLAGS) {
        return config_fail(
            diagnostic, ARBOR_CONFIG_DIAGNOSTIC_SCHEMA_FLAGS,
            ARBOR_CONFIG_SOURCE_NONE, CONFIG_NA, 8u, CONFIG_NA,
            config_invalid());
    }
    config_range descriptor_range;
    if (!config_array_range(
            schema->descriptors,
            schema->descriptor_count,
            sizeof(arbor_config_descriptor),
            &descriptor_range)) {
        return config_fail(
            diagnostic, ARBOR_CONFIG_DIAGNOSTIC_OVERFLOW,
            ARBOR_CONFIG_SOURCE_NONE, CONFIG_NA, CONFIG_NA, CONFIG_NA,
            config_overflow());
    }

    for (uint64_t index = 0u; index < schema->descriptor_count; ++index) {
        const arbor_config_descriptor *descriptor =
            &schema->descriptors[index];
        uint64_t requirement = descriptor->flags &
            (ARBOR_CONFIG_DESCRIPTOR_REQUIRED |
             ARBOR_CONFIG_DESCRIPTOR_HAS_DEFAULT);
        if ((descriptor->flags & ~ARBOR_CONFIG_DESCRIPTOR_KNOWN_FLAGS) != 0u ||
            (requirement != ARBOR_CONFIG_DESCRIPTOR_REQUIRED &&
             requirement != ARBOR_CONFIG_DESCRIPTOR_HAS_DEFAULT) ||
            ((descriptor->flags & ARBOR_CONFIG_DESCRIPTOR_ALLOW_EMPTY) != 0u &&
             descriptor->kind != ARBOR_CONFIG_KIND_UTF8)) {
            return config_fail(
                diagnostic, ARBOR_CONFIG_DIAGNOSTIC_SCHEMA_FLAGS,
                ARBOR_CONFIG_SOURCE_NONE, CONFIG_NA, 72u, index,
                config_invalid());
        }
        if (descriptor->reserved0 != 0u ||
            descriptor->kind < ARBOR_CONFIG_KIND_BOOL ||
            descriptor->kind > ARBOR_CONFIG_KIND_ENUM) {
            return config_fail(
                diagnostic, ARBOR_CONFIG_DIAGNOSTIC_SCHEMA_DESCRIPTOR,
                ARBOR_CONFIG_SOURCE_NONE, CONFIG_NA, CONFIG_NA, index,
                config_invalid());
        }

        const arbor_span names[4] = {
            descriptor->names.key,
            descriptor->names.file,
            descriptor->names.environment,
            descriptor->names.command_line
        };
        for (uint64_t name_index = 0u; name_index < 4u; ++name_index) {
            uint64_t bad_offset = 0u;
            if (!config_name_valid(
                    names[name_index], name_index == 2u, false,
                    &bad_offset)) {
                return config_fail(
                    diagnostic, ARBOR_CONFIG_DIAGNOSTIC_SCHEMA_NAME,
                    ARBOR_CONFIG_SOURCE_NONE, CONFIG_NA, bad_offset, index,
                    config_invalid());
            }
            for (uint64_t earlier = 0u; earlier < index; ++earlier) {
                arbor_span prior = config_descriptor_name(
                    &schema->descriptors[earlier],
                    (config_namespace)name_index);
                if (config_span_equal(names[name_index], prior)) {
                    return config_fail(
                        diagnostic,
                        ARBOR_CONFIG_DIAGNOSTIC_SCHEMA_NAME_DUPLICATE,
                        ARBOR_CONFIG_SOURCE_NONE, CONFIG_NA, 0u, index,
                        config_invalid());
                }
            }
        }

        bool constraints_valid = false;
        switch (descriptor->kind) {
        case ARBOR_CONFIG_KIND_BOOL:
            constraints_valid = descriptor->u64_min == 0u &&
                descriptor->u64_max == 0u && descriptor->i64_min == 0 &&
                descriptor->i64_max == 0 &&
                descriptor->utf8_max_length == 0u &&
                descriptor->enum_choices == NULL &&
                descriptor->enum_choice_count == 0u;
            break;
        case ARBOR_CONFIG_KIND_U64:
            constraints_valid = descriptor->u64_min <= descriptor->u64_max &&
                descriptor->i64_min == 0 && descriptor->i64_max == 0 &&
                descriptor->utf8_max_length == 0u &&
                descriptor->enum_choices == NULL &&
                descriptor->enum_choice_count == 0u;
            break;
        case ARBOR_CONFIG_KIND_I64:
            constraints_valid = descriptor->u64_min == 0u &&
                descriptor->u64_max == 0u &&
                descriptor->i64_min <= descriptor->i64_max &&
                descriptor->utf8_max_length == 0u &&
                descriptor->enum_choices == NULL &&
                descriptor->enum_choice_count == 0u;
            break;
        case ARBOR_CONFIG_KIND_UTF8:
            constraints_valid = descriptor->u64_min == 0u &&
                descriptor->u64_max == 0u && descriptor->i64_min == 0 &&
                descriptor->i64_max == 0 &&
                descriptor->enum_choices == NULL &&
                descriptor->enum_choice_count == 0u;
            break;
        case ARBOR_CONFIG_KIND_ENUM: {
            config_range choices;
            constraints_valid = descriptor->u64_min == 0u &&
                descriptor->u64_max == 0u && descriptor->i64_min == 0 &&
                descriptor->i64_max == 0 &&
                descriptor->utf8_max_length == 0u &&
                descriptor->enum_choice_count != 0u &&
                config_array_range(
                    descriptor->enum_choices,
                    descriptor->enum_choice_count,
                    sizeof(arbor_config_enum_choice),
                    &choices);
            if (constraints_valid) {
                for (uint64_t choice = 0u;
                     choice < descriptor->enum_choice_count; ++choice) {
                    uint64_t bad_offset = 0u;
                    if (!config_name_valid(
                            descriptor->enum_choices[choice].name,
                            false, true, &bad_offset)) {
                        return config_fail(
                            diagnostic, ARBOR_CONFIG_DIAGNOSTIC_SCHEMA_NAME,
                            ARBOR_CONFIG_SOURCE_NONE, CONFIG_NA, bad_offset,
                            index, config_invalid());
                    }
                    for (uint64_t earlier = 0u; earlier < choice; ++earlier) {
                        if (config_span_equal(
                                descriptor->enum_choices[choice].name,
                                descriptor->enum_choices[earlier].name) ||
                            descriptor->enum_choices[choice].value ==
                                descriptor->enum_choices[earlier].value) {
                            return config_fail(
                                diagnostic,
                                ARBOR_CONFIG_DIAGNOSTIC_SCHEMA_DESCRIPTOR,
                                ARBOR_CONFIG_SOURCE_NONE, CONFIG_NA,
                                CONFIG_NA, index, config_invalid());
                        }
                    }
                }
            }
            break;
        }
        }
        if (!constraints_valid) {
            return config_fail(
                diagnostic, ARBOR_CONFIG_DIAGNOSTIC_SCHEMA_DESCRIPTOR,
                ARBOR_CONFIG_SOURCE_NONE, CONFIG_NA, CONFIG_NA, index,
                config_invalid());
        }

        if (requirement == ARBOR_CONFIG_DESCRIPTOR_REQUIRED) {
            if (!config_value_zero(&descriptor->default_value)) {
                return config_fail(
                    diagnostic, ARBOR_CONFIG_DIAGNOSTIC_SCHEMA_DEFAULT,
                    ARBOR_CONFIG_SOURCE_DEFAULT, index, 0u, index,
                    config_invalid());
            }
        } else {
            uint64_t bad_offset = 0u;
            arbor_config_diagnostic_code failure =
                ARBOR_CONFIG_DIAGNOSTIC_SCHEMA_DEFAULT;
            if (!config_value_canonical(
                    descriptor, &descriptor->default_value, true,
                    &bad_offset, &failure)) {
                return config_fail(
                    diagnostic, ARBOR_CONFIG_DIAGNOSTIC_SCHEMA_DEFAULT,
                    ARBOR_CONFIG_SOURCE_DEFAULT, index, bad_offset, index,
                    config_invalid());
            }
        }
    }
    return config_ok();
}

static arbor_status config_parse_text_value(
    const arbor_config_descriptor *descriptor,
    arbor_span text,
    arbor_config_source source,
    uint64_t ordinal,
    uint64_t value_offset,
    uint64_t key_index,
    config_resolution *resolution,
    arbor_config_diagnostic *diagnostic)
{
    arbor_config_value value = {0};
    value.kind = descriptor->kind;
    switch (descriptor->kind) {
    case ARBOR_CONFIG_KIND_BOOL:
        if (config_span_literal(text, "true")) {
            value.bool_value = true;
        } else if (!config_span_literal(text, "false")) {
            return config_fail(
                diagnostic, ARBOR_CONFIG_DIAGNOSTIC_VALUE_BOOL, source,
                ordinal, value_offset, key_index, config_invalid());
        }
        break;
    case ARBOR_CONFIG_KIND_U64: {
        if (text.length == 0u) {
            return config_fail(
                diagnostic, ARBOR_CONFIG_DIAGNOSTIC_VALUE_U64, source,
                ordinal, value_offset, key_index, config_invalid());
        }
        uint64_t number = 0u;
        for (uint64_t index = 0u; index < text.length; ++index) {
            uint8_t byte = text.data[index];
            if (byte < (uint8_t)'0' || byte > (uint8_t)'9') {
                return config_fail(
                    diagnostic, ARBOR_CONFIG_DIAGNOSTIC_VALUE_U64, source,
                    ordinal, value_offset + index, key_index,
                    config_invalid());
            }
            uint64_t digit = (uint64_t)(byte - (uint8_t)'0');
            if (number > (UINT64_MAX - digit) / 10u) {
                return config_fail(
                    diagnostic, ARBOR_CONFIG_DIAGNOSTIC_VALUE_U64, source,
                    ordinal, value_offset + index, key_index,
                    config_overflow());
            }
            number = number * 10u + digit;
        }
        if (number < descriptor->u64_min || number > descriptor->u64_max) {
            return config_fail(
                diagnostic, ARBOR_CONFIG_DIAGNOSTIC_VALUE_BOUNDS, source,
                ordinal, value_offset, key_index, config_invalid());
        }
        value.u64_value = number;
        break;
    }
    case ARBOR_CONFIG_KIND_I64: {
        if (text.length == 0u) {
            return config_fail(
                diagnostic, ARBOR_CONFIG_DIAGNOSTIC_VALUE_I64, source,
                ordinal, value_offset, key_index, config_invalid());
        }
        uint64_t index = 0u;
        bool negative = false;
        if (text.data[0] == (uint8_t)'-') {
            negative = true;
            index = 1u;
        }
        if (index == text.length) {
            return config_fail(
                diagnostic, ARBOR_CONFIG_DIAGNOSTIC_VALUE_I64, source,
                ordinal, value_offset + index, key_index, config_invalid());
        }
        uint64_t magnitude = 0u;
        uint64_t limit = negative ? UINT64_C(9223372036854775808) :
            UINT64_C(9223372036854775807);
        for (; index < text.length; ++index) {
            uint8_t byte = text.data[index];
            if (byte < (uint8_t)'0' || byte > (uint8_t)'9') {
                return config_fail(
                    diagnostic, ARBOR_CONFIG_DIAGNOSTIC_VALUE_I64, source,
                    ordinal, value_offset + index, key_index,
                    config_invalid());
            }
            uint64_t digit = (uint64_t)(byte - (uint8_t)'0');
            if (magnitude > (limit - digit) / 10u) {
                return config_fail(
                    diagnostic, ARBOR_CONFIG_DIAGNOSTIC_VALUE_I64, source,
                    ordinal, value_offset + index, key_index,
                    config_overflow());
            }
            magnitude = magnitude * 10u + digit;
        }
        int64_t number;
        if (negative && magnitude == UINT64_C(9223372036854775808)) {
            number = INT64_MIN;
        } else if (negative) {
            number = -(int64_t)magnitude;
        } else {
            number = (int64_t)magnitude;
        }
        if (number < descriptor->i64_min || number > descriptor->i64_max) {
            return config_fail(
                diagnostic, ARBOR_CONFIG_DIAGNOSTIC_VALUE_BOUNDS, source,
                ordinal, value_offset, key_index, config_invalid());
        }
        value.i64_value = number;
        break;
    }
    case ARBOR_CONFIG_KIND_UTF8: {
        if (text.length == 0u &&
            (descriptor->flags & ARBOR_CONFIG_DESCRIPTOR_ALLOW_EMPTY) == 0u) {
            return config_fail(
                diagnostic, ARBOR_CONFIG_DIAGNOSTIC_VALUE_EMPTY, source,
                ordinal, value_offset, key_index, config_invalid());
        }
        if (text.length > descriptor->utf8_max_length) {
            return config_fail(
                diagnostic, ARBOR_CONFIG_DIAGNOSTIC_VALUE_LENGTH, source,
                ordinal, value_offset, key_index, config_invalid());
        }
        uint64_t bad_offset = 0u;
        if (!config_utf8_valid(text, &bad_offset)) {
            return config_fail(
                diagnostic, ARBOR_CONFIG_DIAGNOSTIC_VALUE_UTF8, source,
                ordinal, value_offset + bad_offset, key_index,
                config_invalid());
        }
        value.utf8_value = text;
        break;
    }
    case ARBOR_CONFIG_KIND_ENUM: {
        bool matched = false;
        for (uint64_t index = 0u; index < descriptor->enum_choice_count;
             ++index) {
            if (config_span_equal(text, descriptor->enum_choices[index].name)) {
                value.enum_value = descriptor->enum_choices[index].value;
                matched = true;
                break;
            }
        }
        if (!matched) {
            return config_fail(
                diagnostic, ARBOR_CONFIG_DIAGNOSTIC_VALUE_ENUM, source,
                ordinal, value_offset, key_index, config_invalid());
        }
        break;
    }
    }
    resolution->value = value;
    resolution->provenance = (arbor_config_provenance){
        source, 0u, ordinal, value_offset
    };
    resolution->utf8_source = descriptor->kind == ARBOR_CONFIG_KIND_UTF8 ?
        text.data : NULL;
    resolution->utf8_length = descriptor->kind == ARBOR_CONFIG_KIND_UTF8 ?
        text.length : 0u;
    resolution->present = true;
    return config_ok();
}

static arbor_status config_file_record_next(
    arbor_span document,
    uint64_t *cursor,
    uint64_t *ordinal,
    config_record *record,
    bool *available,
    arbor_config_diagnostic *diagnostic)
{
    if (*cursor >= document.length) {
        *available = false;
        return config_ok();
    }
    uint64_t start = *cursor;
    uint64_t end = start;
    while (end < document.length && document.data[end] != (uint8_t)'\n' &&
           document.data[end] != (uint8_t)'\r') {
        if (document.data[end] == 0u) {
            return config_fail(
                diagnostic, ARBOR_CONFIG_DIAGNOSTIC_SOURCE_NUL,
                ARBOR_CONFIG_SOURCE_FILE, *ordinal, end, CONFIG_NA,
                config_invalid());
        }
        ++end;
    }
    uint64_t next = end;
    if (end < document.length) {
        if (document.data[end] == (uint8_t)'\r') {
            if (end + 1u >= document.length ||
                document.data[end + 1u] != (uint8_t)'\n') {
                return config_fail(
                    diagnostic, ARBOR_CONFIG_DIAGNOSTIC_SOURCE_SYNTAX,
                    ARBOR_CONFIG_SOURCE_FILE, *ordinal, end, CONFIG_NA,
                    config_invalid());
            }
            next = end + 2u;
        } else {
            next = end + 1u;
        }
    }
    arbor_span line = {document.data + start, end - start};
    *record = (config_record){line, {NULL, 0u}, *ordinal, start, start};
    *cursor = next;
    ++*ordinal;
    *available = true;
    return config_ok();
}

static arbor_status config_assignment_from_file(
    config_record *record,
    bool *assignment,
    arbor_config_diagnostic *diagnostic)
{
    if (record->name.length == 0u || record->name.data[0] == (uint8_t)'#') {
        *assignment = false;
        return config_ok();
    }
    uint64_t equals = 0u;
    while (equals < record->name.length &&
           record->name.data[equals] != (uint8_t)'=') {
        ++equals;
    }
    if (equals == 0u || equals == record->name.length) {
        return config_fail(
            diagnostic, ARBOR_CONFIG_DIAGNOSTIC_SOURCE_SYNTAX,
            ARBOR_CONFIG_SOURCE_FILE, record->ordinal,
            record->name_offset, CONFIG_NA, config_invalid());
    }
    arbor_span line = record->name;
    record->name = (arbor_span){line.data, equals};
    record->value = (arbor_span){
        line.data + equals + 1u, line.length - equals - 1u
    };
    record->value_offset = record->name_offset + equals + 1u;
    *assignment = true;
    return config_ok();
}

static arbor_status config_vector_record(
    arbor_span entry,
    uint64_t ordinal,
    arbor_config_source source,
    config_record *record,
    arbor_config_diagnostic *diagnostic)
{
    config_range range;
    if (!config_range_make(entry.data, entry.length, &range)) {
        return config_fail(
            diagnostic, ARBOR_CONFIG_DIAGNOSTIC_OVERFLOW, source, ordinal,
            CONFIG_NA, CONFIG_NA, config_overflow());
    }
    for (uint64_t index = 0u; index < entry.length; ++index) {
        if (entry.data[index] == 0u || entry.data[index] == (uint8_t)'\r' ||
            entry.data[index] == (uint8_t)'\n') {
            return config_fail(
                diagnostic, ARBOR_CONFIG_DIAGNOSTIC_SOURCE_NUL, source,
                ordinal, index, CONFIG_NA, config_invalid());
        }
    }
    uint64_t name_start = source == ARBOR_CONFIG_SOURCE_COMMAND_LINE ? 2u : 0u;
    if ((source == ARBOR_CONFIG_SOURCE_COMMAND_LINE &&
         (entry.length < 3u || entry.data[0] != (uint8_t)'-' ||
          entry.data[1] != (uint8_t)'-')) || entry.length == 0u) {
        return config_fail(
            diagnostic, ARBOR_CONFIG_DIAGNOSTIC_SOURCE_SYNTAX, source,
            ordinal, 0u, CONFIG_NA, config_invalid());
    }
    uint64_t equals = name_start;
    while (equals < entry.length && entry.data[equals] != (uint8_t)'=') {
        ++equals;
    }
    if (equals == name_start || equals == entry.length) {
        return config_fail(
            diagnostic, ARBOR_CONFIG_DIAGNOSTIC_SOURCE_SYNTAX, source,
            ordinal, name_start, CONFIG_NA, config_invalid());
    }
    record->name = (arbor_span){entry.data + name_start, equals - name_start};
    record->value = (arbor_span){
        entry.data + equals + 1u, entry.length - equals - 1u
    };
    record->ordinal = ordinal;
    record->name_offset = name_start;
    record->value_offset = equals + 1u;
    return config_ok();
}

static bool config_file_seen_before(
    const arbor_config_schema *schema,
    arbor_span document,
    uint64_t stop_offset,
    uint64_t key_index)
{
    uint64_t cursor = 0u;
    while (cursor < stop_offset) {
        uint64_t start = cursor;
        uint64_t end = start;
        while (end < document.length && document.data[end] != (uint8_t)'\n' &&
               document.data[end] != (uint8_t)'\r') {
            ++end;
        }
        cursor = end;
        if (cursor < document.length) {
            cursor += document.data[cursor] == (uint8_t)'\r' ? 2u : 1u;
        }
        if (end == start || document.data[start] == (uint8_t)'#') {
            continue;
        }
        uint64_t equals = start;
        while (equals < end && document.data[equals] != (uint8_t)'=') {
            ++equals;
        }
        if (equals > start && equals < end &&
            config_find_descriptor(
                schema, CONFIG_NAMESPACE_FILE,
                (arbor_span){document.data + start, equals - start}) ==
                key_index) {
            return true;
        }
    }
    return false;
}

static bool config_vector_seen_before(
    const arbor_config_schema *schema,
    const arbor_span *entries,
    uint64_t stop,
    arbor_config_source source,
    uint64_t key_index)
{
    config_namespace name_space = source == ARBOR_CONFIG_SOURCE_ENVIRONMENT ?
        CONFIG_NAMESPACE_ENVIRONMENT : CONFIG_NAMESPACE_COMMAND_LINE;
    for (uint64_t index = 0u; index < stop; ++index) {
        arbor_span entry = entries[index];
        uint64_t start = source == ARBOR_CONFIG_SOURCE_COMMAND_LINE ? 2u : 0u;
        uint64_t equals = start;
        while (equals < entry.length && entry.data[equals] != (uint8_t)'=') {
            ++equals;
        }
        if (config_find_descriptor(
                schema, name_space,
                (arbor_span){entry.data + start, equals - start}) == key_index) {
            return true;
        }
    }
    return false;
}

static arbor_status config_validate_file(
    const arbor_config_schema *schema,
    arbor_span document,
    arbor_config_diagnostic *diagnostic)
{
    config_range document_range;
    if (!config_range_make(document.data, document.length, &document_range)) {
        return config_fail(
            diagnostic, ARBOR_CONFIG_DIAGNOSTIC_OVERFLOW,
            ARBOR_CONFIG_SOURCE_FILE, CONFIG_NA, CONFIG_NA, CONFIG_NA,
            config_overflow());
    }
    uint64_t cursor = 0u;
    uint64_t ordinal = 0u;
    for (;;) {
        config_record record;
        bool available = false;
        arbor_status status = config_file_record_next(
            document, &cursor, &ordinal, &record, &available, diagnostic);
        if (status.native != 0 || !available) {
            return status;
        }
        bool assignment = false;
        status = config_assignment_from_file(&record, &assignment, diagnostic);
        if (status.native != 0) {
            return status;
        }
        if (!assignment) {
            continue;
        }
        uint64_t key_index = config_find_descriptor(
            schema, CONFIG_NAMESPACE_FILE, record.name);
        if (key_index == CONFIG_NA) {
            return config_fail(
                diagnostic, ARBOR_CONFIG_DIAGNOSTIC_UNKNOWN_KEY,
                ARBOR_CONFIG_SOURCE_FILE, record.ordinal,
                record.name_offset, CONFIG_NA, config_not_found());
        }
        if (config_file_seen_before(
                schema, document, record.name_offset, key_index)) {
            return config_fail(
                diagnostic, ARBOR_CONFIG_DIAGNOSTIC_DUPLICATE_KEY,
                ARBOR_CONFIG_SOURCE_FILE, record.ordinal,
                record.name_offset, key_index, config_invalid());
        }
        config_resolution parsed = {0};
        status = config_parse_text_value(
            &schema->descriptors[key_index], record.value,
            ARBOR_CONFIG_SOURCE_FILE, record.ordinal, record.value_offset,
            key_index, &parsed, diagnostic);
        if (status.native != 0) {
            return status;
        }
    }
}

static arbor_status config_validate_vector(
    const arbor_config_schema *schema,
    const arbor_span *entries,
    uint64_t count,
    arbor_config_source source,
    arbor_config_diagnostic *diagnostic)
{
    config_range vector_range;
    if (!config_array_range(entries, count, sizeof(arbor_span), &vector_range)) {
        return config_fail(
            diagnostic, ARBOR_CONFIG_DIAGNOSTIC_OVERFLOW, source,
            CONFIG_NA, CONFIG_NA, CONFIG_NA, config_overflow());
    }
    config_namespace name_space = source == ARBOR_CONFIG_SOURCE_ENVIRONMENT ?
        CONFIG_NAMESPACE_ENVIRONMENT : CONFIG_NAMESPACE_COMMAND_LINE;
    for (uint64_t ordinal = 0u; ordinal < count; ++ordinal) {
        config_record record;
        arbor_status status = config_vector_record(
            entries[ordinal], ordinal, source, &record, diagnostic);
        if (status.native != 0) {
            return status;
        }
        uint64_t key_index = config_find_descriptor(
            schema, name_space, record.name);
        if (key_index == CONFIG_NA) {
            return config_fail(
                diagnostic, ARBOR_CONFIG_DIAGNOSTIC_UNKNOWN_KEY, source,
                ordinal, record.name_offset, CONFIG_NA, config_not_found());
        }
        if (config_vector_seen_before(
                schema, entries, ordinal, source, key_index)) {
            return config_fail(
                diagnostic, ARBOR_CONFIG_DIAGNOSTIC_DUPLICATE_KEY, source,
                ordinal, record.name_offset, key_index, config_invalid());
        }
        config_resolution parsed = {0};
        status = config_parse_text_value(
            &schema->descriptors[key_index], record.value, source, ordinal,
            record.value_offset, key_index, &parsed, diagnostic);
        if (status.native != 0) {
            return status;
        }
    }
    return config_ok();
}

static arbor_status config_validate_sources(
    const arbor_config_schema *schema,
    const arbor_config_sources *sources,
    arbor_config_diagnostic *diagnostic)
{
    config_range sources_range;
    if (!config_range_make(sources, sizeof(*sources), &sources_range)) {
        return config_fail(
            diagnostic, ARBOR_CONFIG_DIAGNOSTIC_SOURCE_SYNTAX,
            ARBOR_CONFIG_SOURCE_NONE, CONFIG_NA, CONFIG_NA, CONFIG_NA,
            config_invalid());
    }
    arbor_status status = config_validate_file(
        schema, sources->file_document, diagnostic);
    if (status.native != 0) {
        return status;
    }
    status = config_validate_vector(
        schema, sources->environment_entries, sources->environment_count,
        ARBOR_CONFIG_SOURCE_ENVIRONMENT, diagnostic);
    if (status.native != 0) {
        return status;
    }
    return config_validate_vector(
        schema, sources->command_line_entries, sources->command_line_count,
        ARBOR_CONFIG_SOURCE_COMMAND_LINE, diagnostic);
}

static arbor_status config_resolve_descriptor(
    const arbor_config_schema *schema,
    const arbor_config_sources *sources,
    uint64_t key_index,
    config_resolution *resolution,
    arbor_config_diagnostic *diagnostic)
{
    const arbor_config_descriptor *descriptor = &schema->descriptors[key_index];
    *resolution = (config_resolution){0};
    if ((descriptor->flags & ARBOR_CONFIG_DESCRIPTOR_HAS_DEFAULT) != 0u) {
        resolution->value = descriptor->default_value;
        resolution->provenance = (arbor_config_provenance){
            ARBOR_CONFIG_SOURCE_DEFAULT, 0u, key_index, 0u
        };
        resolution->utf8_source = descriptor->kind == ARBOR_CONFIG_KIND_UTF8 ?
            descriptor->default_value.utf8_value.data : NULL;
        resolution->utf8_length = descriptor->kind == ARBOR_CONFIG_KIND_UTF8 ?
            descriptor->default_value.utf8_value.length : 0u;
        resolution->present = true;
    }

    uint64_t cursor = 0u;
    uint64_t ordinal = 0u;
    for (;;) {
        config_record record;
        bool available = false;
        arbor_status status = config_file_record_next(
            sources->file_document, &cursor, &ordinal, &record, &available,
            diagnostic);
        if (status.native != 0 || !available) {
            if (status.native != 0) {
                return status;
            }
            break;
        }
        bool assignment = false;
        status = config_assignment_from_file(&record, &assignment, diagnostic);
        if (status.native != 0) {
            return status;
        }
        if (assignment && config_find_descriptor(
                schema, CONFIG_NAMESPACE_FILE, record.name) == key_index) {
            status = config_parse_text_value(
                descriptor, record.value, ARBOR_CONFIG_SOURCE_FILE,
                record.ordinal, record.value_offset, key_index, resolution,
                diagnostic);
            if (status.native != 0) {
                return status;
            }
        }
    }

    const arbor_span *vectors[2] = {
        sources->environment_entries,
        sources->command_line_entries
    };
    const uint64_t counts[2] = {
        sources->environment_count,
        sources->command_line_count
    };
    const arbor_config_source source_kinds[2] = {
        ARBOR_CONFIG_SOURCE_ENVIRONMENT,
        ARBOR_CONFIG_SOURCE_COMMAND_LINE
    };
    const config_namespace namespaces[2] = {
        CONFIG_NAMESPACE_ENVIRONMENT,
        CONFIG_NAMESPACE_COMMAND_LINE
    };
    for (uint64_t vector = 0u; vector < 2u; ++vector) {
        for (uint64_t item = 0u; item < counts[vector]; ++item) {
            config_record record;
            arbor_status status = config_vector_record(
                vectors[vector][item], item, source_kinds[vector], &record,
                diagnostic);
            if (status.native != 0) {
                return status;
            }
            if (config_find_descriptor(
                    schema, namespaces[vector], record.name) == key_index) {
                status = config_parse_text_value(
                    descriptor, record.value, source_kinds[vector], item,
                    record.value_offset, key_index, resolution, diagnostic);
                if (status.native != 0) {
                    return status;
                }
            }
        }
    }
    if (!resolution->present) {
        return config_fail(
            diagnostic, ARBOR_CONFIG_DIAGNOSTIC_REQUIRED,
            ARBOR_CONFIG_SOURCE_NONE, CONFIG_NA, CONFIG_NA, key_index,
            config_not_found());
    }
    return config_ok();
}

static arbor_status config_analyze(
    const arbor_config_schema *schema,
    const arbor_config_sources *sources,
    arbor_config_requirements *requirements,
    arbor_config_diagnostic *diagnostic)
{
    arbor_status status = config_validate_schema(schema, diagnostic);
    if (status.native != 0) {
        return status;
    }
    status = config_validate_sources(schema, sources, diagnostic);
    if (status.native != 0) {
        return status;
    }
    uint64_t persistent_bytes = 0u;
    for (uint64_t index = 0u; index < schema->descriptor_count; ++index) {
        config_resolution resolution;
        status = config_resolve_descriptor(
            schema, sources, index, &resolution, diagnostic);
        if (status.native != 0) {
            return status;
        }
        if (!config_u64_add(
                persistent_bytes, resolution.utf8_length,
                &persistent_bytes)) {
            return config_fail(
                diagnostic, ARBOR_CONFIG_DIAGNOSTIC_OVERFLOW,
                resolution.provenance.source, resolution.provenance.ordinal,
                resolution.provenance.byte_offset, index, config_overflow());
        }
    }
    uint64_t descriptor_bytes = 0u;
    uint64_t value_bytes = 0u;
    uint64_t provenance_bytes = 0u;
    if (!config_u64_multiply(
            schema->descriptor_count, sizeof(arbor_config_descriptor),
            &descriptor_bytes) ||
        !config_u64_multiply(
            schema->descriptor_count, sizeof(arbor_config_value),
            &value_bytes) ||
        !config_u64_multiply(
            schema->descriptor_count, sizeof(arbor_config_provenance),
            &provenance_bytes)) {
        return config_fail(
            diagnostic, ARBOR_CONFIG_DIAGNOSTIC_OVERFLOW,
            ARBOR_CONFIG_SOURCE_NONE, CONFIG_NA, CONFIG_NA, CONFIG_NA,
            config_overflow());
    }
    *requirements = (arbor_config_requirements){
        schema->descriptor_count,
        descriptor_bytes,
        value_bytes,
        provenance_bytes,
        schema->descriptor_count,
        persistent_bytes,
        sizeof(arbor_config_result)
    };
    return config_ok();
}

static bool config_range_overlaps_schema(
    config_range writable,
    const arbor_config_schema *schema)
{
    config_range input;
    if (config_range_make(schema, sizeof(*schema), &input) &&
        config_ranges_overlap(writable, input)) {
        return true;
    }
    if (config_array_range(
            schema->descriptors, schema->descriptor_count,
            sizeof(arbor_config_descriptor), &input) &&
        config_ranges_overlap(writable, input)) {
        return true;
    }
    for (uint64_t index = 0u; index < schema->descriptor_count; ++index) {
        const arbor_config_descriptor *descriptor =
            &schema->descriptors[index];
        const arbor_span names[4] = {
            descriptor->names.key,
            descriptor->names.file,
            descriptor->names.environment,
            descriptor->names.command_line
        };
        for (uint64_t name = 0u; name < 4u; ++name) {
            if (config_range_make(names[name].data, names[name].length, &input) &&
                config_ranges_overlap(writable, input)) {
                return true;
            }
        }
        if (descriptor->kind == ARBOR_CONFIG_KIND_UTF8 &&
            (descriptor->flags & ARBOR_CONFIG_DESCRIPTOR_HAS_DEFAULT) != 0u &&
            config_range_make(
                descriptor->default_value.utf8_value.data,
                descriptor->default_value.utf8_value.length, &input) &&
            config_ranges_overlap(writable, input)) {
            return true;
        }
        if (descriptor->kind == ARBOR_CONFIG_KIND_ENUM) {
            if (config_array_range(
                    descriptor->enum_choices, descriptor->enum_choice_count,
                    sizeof(arbor_config_enum_choice), &input) &&
                config_ranges_overlap(writable, input)) {
                return true;
            }
            for (uint64_t choice = 0u;
                 choice < descriptor->enum_choice_count; ++choice) {
                arbor_span name = descriptor->enum_choices[choice].name;
                if (config_range_make(name.data, name.length, &input) &&
                    config_ranges_overlap(writable, input)) {
                    return true;
                }
            }
        }
    }
    return false;
}

static bool config_range_overlaps_sources(
    config_range writable,
    const arbor_config_sources *sources)
{
    config_range input;
    if (config_range_make(sources, sizeof(*sources), &input) &&
        config_ranges_overlap(writable, input)) {
        return true;
    }
    if (config_range_make(
            sources->file_document.data, sources->file_document.length,
            &input) && config_ranges_overlap(writable, input)) {
        return true;
    }
    const arbor_span *vectors[2] = {
        sources->environment_entries,
        sources->command_line_entries
    };
    const uint64_t counts[2] = {
        sources->environment_count,
        sources->command_line_count
    };
    for (uint64_t vector = 0u; vector < 2u; ++vector) {
        if (config_array_range(
                vectors[vector], counts[vector], sizeof(arbor_span), &input) &&
            config_ranges_overlap(writable, input)) {
            return true;
        }
        for (uint64_t index = 0u; index < counts[vector]; ++index) {
            if (config_range_make(
                    vectors[vector][index].data,
                    vectors[vector][index].length, &input) &&
                config_ranges_overlap(writable, input)) {
                return true;
            }
        }
    }
    return false;
}

static bool config_writable_against_inputs(
    config_range writable,
    const arbor_config_schema *schema,
    const arbor_config_sources *sources)
{
    return config_range_overlaps_schema(writable, schema) ||
        config_range_overlaps_sources(writable, sources);
}

static bool config_input_ranges_representable(
    const arbor_config_schema *schema,
    const arbor_config_sources *sources)
{
    config_range input;
    if (!config_range_make(schema, sizeof(*schema), &input) ||
        !config_range_make(sources, sizeof(*sources), &input) ||
        !config_array_range(
            schema->descriptors, schema->descriptor_count,
            sizeof(arbor_config_descriptor), &input) ||
        !config_range_make(
            sources->file_document.data, sources->file_document.length,
            &input) ||
        !config_array_range(
            sources->environment_entries, sources->environment_count,
            sizeof(arbor_span), &input) ||
        !config_array_range(
            sources->command_line_entries, sources->command_line_count,
            sizeof(arbor_span), &input)) {
        return false;
    }
    for (uint64_t index = 0u; index < schema->descriptor_count; ++index) {
        const arbor_config_descriptor *descriptor =
            &schema->descriptors[index];
        const arbor_span names[4] = {
            descriptor->names.key,
            descriptor->names.file,
            descriptor->names.environment,
            descriptor->names.command_line
        };
        for (uint64_t name = 0u; name < 4u; ++name) {
            if (!config_range_make(
                    names[name].data, names[name].length, &input)) {
                return false;
            }
        }
        if (descriptor->kind == ARBOR_CONFIG_KIND_UTF8 &&
            (descriptor->flags & ARBOR_CONFIG_DESCRIPTOR_HAS_DEFAULT) != 0u &&
            !config_range_make(
                descriptor->default_value.utf8_value.data,
                descriptor->default_value.utf8_value.length, &input)) {
            return false;
        }
        if (descriptor->kind == ARBOR_CONFIG_KIND_ENUM) {
            if (!config_array_range(
                    descriptor->enum_choices, descriptor->enum_choice_count,
                    sizeof(arbor_config_enum_choice), &input)) {
                return false;
            }
            for (uint64_t choice = 0u;
                 choice < descriptor->enum_choice_count; ++choice) {
                arbor_span choice_name =
                    descriptor->enum_choices[choice].name;
                if (!config_range_make(
                        choice_name.data, choice_name.length, &input)) {
                    return false;
                }
            }
        }
    }
    const arbor_span *vectors[2] = {
        sources->environment_entries,
        sources->command_line_entries
    };
    const uint64_t counts[2] = {
        sources->environment_count,
        sources->command_line_count
    };
    for (uint64_t vector = 0u; vector < 2u; ++vector) {
        for (uint64_t index = 0u; index < counts[vector]; ++index) {
            if (!config_range_make(
                    vectors[vector][index].data,
                    vectors[vector][index].length, &input)) {
                return false;
            }
        }
    }
    return true;
}

static bool config_diagnostic_writable_for_measure_failure(
    const arbor_config_schema *schema,
    const arbor_config_sources *sources,
    const arbor_config_requirements *requirements_out,
    arbor_config_diagnostic *diagnostic_out)
{
    config_range diagnostic_range;
    config_range requirements_range;
    return diagnostic_out != NULL &&
        config_input_ranges_representable(schema, sources) &&
        config_range_make(
            diagnostic_out, sizeof(*diagnostic_out), &diagnostic_range) &&
        config_range_make(
            requirements_out, sizeof(*requirements_out),
            &requirements_range) &&
        !config_writable_against_inputs(
            diagnostic_range, schema, sources) &&
        !config_ranges_overlap(diagnostic_range, requirements_range);
}

static bool config_diagnostic_writable_for_prepare_failure(
    const arbor_config_schema *schema,
    const arbor_config_sources *sources,
    const arbor_config_storage *storage,
    const arbor_config_result *result_out,
    arbor_config_diagnostic *diagnostic_out)
{
    config_range diagnostic_range;
    config_range output;
    if (diagnostic_out == NULL ||
        !config_input_ranges_representable(schema, sources) ||
        !config_range_make(
            diagnostic_out, sizeof(*diagnostic_out), &diagnostic_range) ||
        !config_range_make(storage, sizeof(*storage), &output) ||
        config_ranges_overlap(diagnostic_range, output) ||
        !config_range_make(result_out, sizeof(*result_out), &output) ||
        config_ranges_overlap(diagnostic_range, output) ||
        config_writable_against_inputs(
            diagnostic_range, schema, sources)) {
        return false;
    }
    if (!config_array_range(
            storage->values, storage->value_capacity,
            sizeof(arbor_config_value), &output) ||
        config_ranges_overlap(diagnostic_range, output) ||
        !config_array_range(
            storage->provenance, storage->provenance_capacity,
            sizeof(arbor_config_provenance), &output) ||
        config_ranges_overlap(diagnostic_range, output) ||
        !config_range_make(
            storage->scratch.data, storage->scratch.length, &output) ||
        config_ranges_overlap(diagnostic_range, output) ||
        !config_range_make(
            storage->persistent.data, storage->persistent.length, &output) ||
        config_ranges_overlap(diagnostic_range, output)) {
        return false;
    }
    return true;
}

static arbor_status config_measure_alias_check(
    const arbor_config_schema *schema,
    const arbor_config_sources *sources,
    arbor_config_requirements *requirements_out,
    arbor_config_diagnostic *diagnostic_out,
    bool *diagnostic_writable)
{
    config_range requirements_range;
    config_range diagnostic_range;
    *diagnostic_writable = false;
    if (!config_range_make(
            requirements_out, sizeof(*requirements_out),
            &requirements_range)) {
        return config_invalid();
    }
    if (diagnostic_out != NULL) {
        if (!config_range_make(
                diagnostic_out, sizeof(*diagnostic_out), &diagnostic_range)) {
            return config_invalid();
        }
        *diagnostic_writable =
            !config_ranges_overlap(requirements_range, diagnostic_range) &&
            !config_writable_against_inputs(
                diagnostic_range, schema, sources);
    } else {
        diagnostic_range = (config_range){0u, 0u, true};
    }
    if (config_writable_against_inputs(requirements_range, schema, sources) ||
        config_ranges_overlap(requirements_range, diagnostic_range) ||
        (diagnostic_out != NULL && !*diagnostic_writable)) {
        return config_invalid();
    }
    return config_ok();
}

static arbor_status config_prepare_ranges(
    const arbor_config_schema *schema,
    const arbor_config_sources *sources,
    const arbor_config_storage *storage,
    arbor_config_result *result_out,
    arbor_config_diagnostic *diagnostic_out,
    config_range writable[5],
    uint64_t *writable_count,
    config_range *diagnostic_range,
    bool *diagnostic_writable,
    arbor_config_diagnostic *diagnostic)
{
    config_range storage_range;
    if (!config_range_make(storage, sizeof(*storage), &storage_range) ||
        !config_range_make(result_out, sizeof(*result_out), &writable[0]) ||
        !config_array_range(
            storage->values, storage->value_capacity,
            sizeof(arbor_config_value), &writable[1]) ||
        !config_array_range(
            storage->provenance, storage->provenance_capacity,
            sizeof(arbor_config_provenance), &writable[2]) ||
        !config_range_make(
            storage->scratch.data, storage->scratch.length, &writable[3]) ||
        !config_range_make(
            storage->persistent.data, storage->persistent.length,
            &writable[4])) {
        return config_fail(
            diagnostic, ARBOR_CONFIG_DIAGNOSTIC_OVERFLOW,
            ARBOR_CONFIG_SOURCE_NONE, CONFIG_NA, CONFIG_NA, CONFIG_NA,
            config_overflow());
    }
    *writable_count = 5u;
    *diagnostic_writable = false;
    if (diagnostic_out != NULL) {
        if (!config_range_make(
                diagnostic_out, sizeof(*diagnostic_out), diagnostic_range)) {
            return config_invalid();
        }
    } else {
        *diagnostic_range = (config_range){0u, 0u, true};
    }
    if (diagnostic_out != NULL) {
        *diagnostic_writable =
            !config_writable_against_inputs(
                *diagnostic_range, schema, sources) &&
            !config_ranges_overlap(*diagnostic_range, storage_range);
        for (uint64_t index = 0u; index < *writable_count; ++index) {
            if (config_ranges_overlap(*diagnostic_range, writable[index])) {
                *diagnostic_writable = false;
            }
        }
    }
    for (uint64_t left = 0u; left < *writable_count; ++left) {
        if (config_writable_against_inputs(writable[left], schema, sources) ||
            config_ranges_overlap(writable[left], storage_range)) {
            return config_fail(
                diagnostic, ARBOR_CONFIG_DIAGNOSTIC_ALIAS,
                ARBOR_CONFIG_SOURCE_NONE, CONFIG_NA, CONFIG_NA, CONFIG_NA,
                config_invalid());
        }
        for (uint64_t right = 0u; right < left; ++right) {
            if (config_ranges_overlap(writable[left], writable[right])) {
                return config_fail(
                    diagnostic, ARBOR_CONFIG_DIAGNOSTIC_ALIAS,
                    ARBOR_CONFIG_SOURCE_NONE, CONFIG_NA, CONFIG_NA,
                    CONFIG_NA, config_invalid());
            }
        }
    }
    if (diagnostic_out != NULL) {
        if (!*diagnostic_writable) {
            return config_fail(
                diagnostic, ARBOR_CONFIG_DIAGNOSTIC_ALIAS,
                ARBOR_CONFIG_SOURCE_NONE, CONFIG_NA, CONFIG_NA, CONFIG_NA,
                config_invalid());
        }
    }
    return config_ok();
}

static void config_publish_diagnostic(
    arbor_config_diagnostic *destination,
    bool writable,
    const arbor_config_diagnostic *diagnostic)
{
    if (destination != NULL && writable &&
        diagnostic->code != ARBOR_CONFIG_DIAGNOSTIC_NONE) {
        *destination = *diagnostic;
    }
}

arbor_status arbor_config_measure(
    const arbor_config_schema *schema,
    const arbor_config_sources *sources,
    arbor_config_requirements *requirements_out,
    arbor_config_diagnostic *diagnostic_out)
{
    arbor_config_diagnostic diagnostic;
    config_diagnostic_clear(&diagnostic);
    arbor_config_requirements requirements;
    arbor_status status = config_analyze(
        schema, sources, &requirements, &diagnostic);
    if (status.native != 0) {
        config_publish_diagnostic(
            diagnostic_out,
            config_diagnostic_writable_for_measure_failure(
                schema, sources, requirements_out, diagnostic_out),
            &diagnostic);
        return status;
    }
    bool diagnostic_writable = false;
    status = config_measure_alias_check(
        schema, sources, requirements_out, diagnostic_out,
        &diagnostic_writable);
    if (status.native != 0) {
        (void)config_fail(
            &diagnostic, ARBOR_CONFIG_DIAGNOSTIC_ALIAS,
            ARBOR_CONFIG_SOURCE_NONE, CONFIG_NA, CONFIG_NA, CONFIG_NA,
            status);
        config_publish_diagnostic(
            diagnostic_out, diagnostic_writable, &diagnostic);
        return status;
    }
    *requirements_out = requirements;
    return config_ok();
}

arbor_status arbor_config_prepare(
    const arbor_config_schema *schema,
    const arbor_config_sources *sources,
    const arbor_config_storage *storage,
    arbor_config_result *result_out,
    arbor_config_diagnostic *diagnostic_out)
{
    arbor_config_diagnostic diagnostic;
    config_diagnostic_clear(&diagnostic);
    arbor_config_requirements requirements;
    arbor_status status = config_analyze(
        schema, sources, &requirements, &diagnostic);
    if (status.native != 0) {
        config_publish_diagnostic(
            diagnostic_out,
            config_diagnostic_writable_for_prepare_failure(
                schema, sources, storage, result_out, diagnostic_out),
            &diagnostic);
        return status;
    }

    config_range writable[5];
    uint64_t writable_count = 0u;
    config_range diagnostic_range;
    bool diagnostic_writable = false;
    status = config_prepare_ranges(
        schema, sources, storage, result_out, diagnostic_out, writable,
        &writable_count, &diagnostic_range, &diagnostic_writable, &diagnostic);
    if (status.native != 0) {
        config_publish_diagnostic(
            diagnostic_out, diagnostic_writable, &diagnostic);
        return status;
    }
    if (storage->value_capacity < requirements.descriptor_count ||
        storage->provenance_capacity < requirements.descriptor_count ||
        storage->scratch.length < requirements.scratch_bytes ||
        storage->persistent.length < requirements.persistent_bytes) {
        status = config_fail(
            &diagnostic, ARBOR_CONFIG_DIAGNOSTIC_CAPACITY,
            ARBOR_CONFIG_SOURCE_NONE, CONFIG_NA,
            requirements.persistent_bytes, CONFIG_NA, config_no_space());
        config_publish_diagnostic(
            diagnostic_out, diagnostic_writable, &diagnostic);
        return status;
    }

    uint64_t persistent_offset = 0u;
    for (uint64_t index = 0u; index < schema->descriptor_count; ++index) {
        config_resolution resolution;
        status = config_resolve_descriptor(
            schema, sources, index, &resolution, &diagnostic);
        if (status.native != 0) {
            config_publish_diagnostic(
                diagnostic_out, diagnostic_writable, &diagnostic);
            return status;
        }
        if (resolution.value.kind == ARBOR_CONFIG_KIND_UTF8) {
            if (resolution.utf8_length != 0u) {
                (void)memcpy(
                    storage->persistent.data + persistent_offset,
                    resolution.utf8_source,
                    (size_t)resolution.utf8_length);
                resolution.value.utf8_value = (arbor_span){
                    storage->persistent.data + persistent_offset,
                    resolution.utf8_length
                };
            } else {
                resolution.value.utf8_value = (arbor_span){NULL, 0u};
            }
            persistent_offset += resolution.utf8_length;
        }
        storage->values[index] = resolution.value;
        storage->provenance[index] = resolution.provenance;
        storage->scratch.data[index] = 1u;
    }
    *result_out = (arbor_config_result){
        storage->values,
        storage->provenance,
        {storage->persistent.data, requirements.persistent_bytes},
        requirements.descriptor_count,
        CONFIG_PREPARED_GUARD
    };
    return config_ok();
}

static bool config_span_contained(arbor_span outer, arbor_span inner)
{
    if (inner.length == 0u) {
        return inner.data == NULL;
    }
    config_range outer_range;
    config_range inner_range;
    return config_range_make(outer.data, outer.length, &outer_range) &&
        config_range_make(inner.data, inner.length, &inner_range) &&
        inner_range.begin >= outer_range.begin &&
        inner_range.end <= outer_range.end;
}

arbor_status arbor_config_validate(
    const arbor_config_schema *schema,
    const arbor_config_result *result)
{
    arbor_config_diagnostic ignored;
    config_diagnostic_clear(&ignored);
    arbor_status status = config_validate_schema(schema, &ignored);
    if (status.native != 0) {
        return status;
    }
    config_range result_range;
    config_range values_range;
    config_range provenance_range;
    config_range persistent_range;
    if (!config_range_make(result, sizeof(*result), &result_range) ||
        result->prepared_guard != CONFIG_PREPARED_GUARD ||
        result->value_count != schema->descriptor_count ||
        !config_array_range(
            result->values, result->value_count, sizeof(arbor_config_value),
            &values_range) ||
        !config_array_range(
            result->provenance, result->value_count,
            sizeof(arbor_config_provenance), &provenance_range) ||
        !config_range_make(
            result->persistent.data, result->persistent.length,
            &persistent_range)) {
        return config_invalid();
    }
    uint64_t persistent_offset = 0u;
    for (uint64_t index = 0u; index < result->value_count; ++index) {
        const arbor_config_descriptor *descriptor =
            &schema->descriptors[index];
        const arbor_config_value *value = &result->values[index];
        uint64_t bad_offset = 0u;
        arbor_config_diagnostic_code failure =
            ARBOR_CONFIG_DIAGNOSTIC_RESULT;
        if (!config_value_canonical(
                descriptor, value, true, &bad_offset, &failure)) {
            return config_invalid();
        }
        const arbor_config_provenance *provenance =
            &result->provenance[index];
        if (provenance->reserved0 != 0u ||
            provenance->source < ARBOR_CONFIG_SOURCE_DEFAULT ||
            provenance->source > ARBOR_CONFIG_SOURCE_COMMAND_LINE ||
            (provenance->source == ARBOR_CONFIG_SOURCE_DEFAULT &&
             (provenance->ordinal != index ||
              provenance->byte_offset != 0u))) {
            return config_invalid();
        }
        if (value->kind == ARBOR_CONFIG_KIND_UTF8) {
            if (!config_span_contained(result->persistent, value->utf8_value)) {
                return config_invalid();
            }
            if (value->utf8_value.length != 0u &&
                value->utf8_value.data !=
                    result->persistent.data + persistent_offset) {
                return config_invalid();
            }
            if (!config_u64_add(
                    persistent_offset, value->utf8_value.length,
                    &persistent_offset)) {
                return config_overflow();
            }
        }
    }
    if (persistent_offset != result->persistent.length) {
        return config_invalid();
    }
    return config_ok();
}
