#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <arborcore/config.h>

_Static_assert(sizeof(arbor_config_source) == 4u, "config source layout");
_Static_assert(sizeof(arbor_config_kind) == 4u, "config kind layout");
_Static_assert(sizeof(arbor_config_diagnostic_code) == 4u,
    "config diagnostic layout");
_Static_assert(sizeof(arbor_config_enum_choice) == 24u, "choice layout");
_Static_assert(sizeof(arbor_config_value) == 48u, "value layout");
_Static_assert(sizeof(arbor_config_names) == 64u, "names layout");
_Static_assert(sizeof(arbor_config_descriptor) == 184u, "descriptor layout");
_Static_assert(sizeof(arbor_config_schema) == 32u, "schema layout");
_Static_assert(sizeof(arbor_config_sources) == 48u, "sources layout");
_Static_assert(sizeof(arbor_config_requirements) == 56u,
    "requirements layout");
_Static_assert(sizeof(arbor_config_provenance) == 24u,
    "provenance layout");
_Static_assert(sizeof(arbor_config_diagnostic) == 32u,
    "diagnostic layout");
_Static_assert(sizeof(arbor_config_storage) == 64u, "storage layout");
_Static_assert(sizeof(arbor_config_result) == 48u, "result layout");
_Static_assert(offsetof(arbor_config_descriptor, default_value) == 80u,
    "descriptor value offset");
_Static_assert(offsetof(arbor_config_descriptor, enum_choice_count) == 176u,
    "descriptor count offset");
_Static_assert(offsetof(arbor_config_value, kind) == 40u,
    "value kind offset");
_Static_assert(offsetof(arbor_config_result, prepared_guard) == 40u,
    "result guard offset");

int main(void)
{
    const arbor_config_schema schema = {
        ARBOR_CONFIG_ABI_VERSION,
        sizeof(arbor_config_schema),
        ARBOR_CONFIG_SCHEMA_KNOWN_FLAGS,
        NULL,
        0u
    };
    const arbor_config_sources sources = {{NULL, 0u}, NULL, 0u, NULL, 0u};
    arbor_config_requirements requirements = {0};
    arbor_status status = arbor_config_measure(
        &schema, &sources, &requirements, NULL);
    if (status.native != 0 || requirements.descriptor_count != 0u ||
        requirements.result_bytes != sizeof(arbor_config_result)) {
        return 1;
    }
    const arbor_config_storage storage = {
        NULL, 0u, NULL, 0u, {NULL, 0u}, {NULL, 0u}
    };
    arbor_config_result result = {0};
    status = arbor_config_prepare(&schema, &sources, &storage, &result, NULL);
    if (status.native != 0 ||
        arbor_config_validate(&schema, &result).native != 0) {
        return 1;
    }
    puts("PASS: CONFIG0 public C include, exact layouts and archive linkage");
    return 0;
}
