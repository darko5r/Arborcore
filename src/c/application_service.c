#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include <arborcore/application_service.h>

_Static_assert(sizeof(void *) == 8u, "AF3 requires x86-64 pointers");
_Static_assert(sizeof(uintptr_t) == 8u, "AF3 requires 64-bit uintptr_t");

_Static_assert(sizeof(arbor_application_service_interface_header) == 16u,
    "AF3 interface header ABI drift");
_Static_assert(offsetof(arbor_application_service_interface_header, abi_version) == 0u,
    "AF3 interface abi_version offset drift");
_Static_assert(offsetof(arbor_application_service_interface_header, struct_size) == 4u,
    "AF3 interface struct_size offset drift");
_Static_assert(offsetof(arbor_application_service_interface_header, flags) == 8u,
    "AF3 interface flags offset drift");

_Static_assert(sizeof(arbor_application_service_module_descriptor) == 80u,
    "AF3 service-module descriptor ABI drift");
_Static_assert(offsetof(arbor_application_service_module_descriptor, module_id) == 0u,
    "AF3 service-module module_id offset drift");
_Static_assert(offsetof(arbor_application_service_module_descriptor, abi_version) == 16u,
    "AF3 service-module abi_version offset drift");
_Static_assert(offsetof(arbor_application_service_module_descriptor, struct_size) == 20u,
    "AF3 service-module struct_size offset drift");
_Static_assert(offsetof(arbor_application_service_module_descriptor, flags) == 24u,
    "AF3 service-module flags offset drift");
_Static_assert(offsetof(arbor_application_service_module_descriptor, module_context) == 32u,
    "AF3 service-module context offset drift");
_Static_assert(offsetof(arbor_application_service_module_descriptor, service_export_indices) == 40u,
    "AF3 service-module export-index offset drift");
_Static_assert(offsetof(arbor_application_service_module_descriptor, service_export_count) == 48u,
    "AF3 service-module export-count offset drift");
_Static_assert(offsetof(arbor_application_service_module_descriptor, prepare) == 56u,
    "AF3 service-module prepare offset drift");
_Static_assert(offsetof(arbor_application_service_module_descriptor, rollback) == 64u,
    "AF3 service-module rollback offset drift");
_Static_assert(offsetof(arbor_application_service_module_descriptor, stop) == 72u,
    "AF3 service-module stop offset drift");

_Static_assert(sizeof(arbor_application_service_runtime_record) == 32u,
    "AF3 runtime record ABI drift");
_Static_assert(offsetof(arbor_application_service_runtime_record, descriptor_index) == 0u,
    "AF3 record descriptor-index offset drift");
_Static_assert(offsetof(arbor_application_service_runtime_record, module_index) == 8u,
    "AF3 record module-index offset drift");
_Static_assert(offsetof(arbor_application_service_runtime_record, state) == 16u,
    "AF3 record state offset drift");
_Static_assert(offsetof(arbor_application_service_runtime_record, reserved0) == 20u,
    "AF3 record reserved offset drift");
_Static_assert(offsetof(arbor_application_service_runtime_record, stop_native) == 24u,
    "AF3 record stop-native offset drift");

_Static_assert(sizeof(arbor_application_runtime_requirements) == 16u,
    "AF3 requirements ABI drift");
_Static_assert(offsetof(arbor_application_runtime_requirements, managed_module_record_count) == 0u,
    "AF3 requirements record-count offset drift");
_Static_assert(offsetof(arbor_application_runtime_requirements, module_map_count) == 8u,
    "AF3 requirements module-map offset drift");

_Static_assert(sizeof(arbor_application_runtime_storage) == 16u,
    "AF3 storage ABI drift");
_Static_assert(offsetof(arbor_application_runtime_storage, records) == 0u,
    "AF3 storage records offset drift");
_Static_assert(offsetof(arbor_application_runtime_storage, record_capacity) == 8u,
    "AF3 storage capacity offset drift");

_Static_assert(sizeof(arbor_application_runtime_workspace) == 32u,
    "AF3 workspace ABI drift");
_Static_assert(offsetof(arbor_application_runtime_workspace, records) == 0u,
    "AF3 workspace records offset drift");
_Static_assert(offsetof(arbor_application_runtime_workspace, record_capacity) == 8u,
    "AF3 workspace record-capacity offset drift");
_Static_assert(offsetof(arbor_application_runtime_workspace, module_to_descriptor_index) == 16u,
    "AF3 workspace module-map offset drift");
_Static_assert(offsetof(arbor_application_runtime_workspace, module_capacity) == 24u,
    "AF3 workspace module-capacity offset drift");

_Static_assert(sizeof(arbor_application_runtime) == 56u,
    "AF3 runtime ABI drift");
_Static_assert(offsetof(arbor_application_runtime, abi_version) == 0u,
    "AF3 runtime abi_version offset drift");
_Static_assert(offsetof(arbor_application_runtime, struct_size) == 4u,
    "AF3 runtime struct_size offset drift");
_Static_assert(offsetof(arbor_application_runtime, flags) == 8u,
    "AF3 runtime flags offset drift");
_Static_assert(offsetof(arbor_application_runtime, catalog) == 16u,
    "AF3 runtime catalog offset drift");
_Static_assert(offsetof(arbor_application_runtime, service_modules) == 24u,
    "AF3 runtime service-modules offset drift");
_Static_assert(offsetof(arbor_application_runtime, managed_module_count) == 32u,
    "AF3 runtime managed-count offset drift");
_Static_assert(offsetof(arbor_application_runtime, records) == 40u,
    "AF3 runtime records offset drift");
_Static_assert(offsetof(arbor_application_runtime, state) == 48u,
    "AF3 runtime state offset drift");
_Static_assert(offsetof(arbor_application_runtime, reserved0) == 52u,
    "AF3 runtime reserved offset drift");

#define AF3_PREPARE_CONTEXT_MAGIC UINT64_C(0x4152463343505831)

struct arbor_application_service_prepare_context {
    uint64_t magic;
    const arbor_capability_catalog *catalog;
    const arbor_application_service_module_descriptor *service_modules;
    uint64_t managed_module_count;
    const arbor_application_runtime_workspace *workspace;
    uint64_t current_record_index;
    uint64_t consumer_module_index;
};

typedef struct af3_region {
    const void *pointer;
    uint64_t length;
} af3_region;

static arbor_status native_status(int64_t native)
{
    return arbor_status_from_native(native);
}

static arbor_status ok_status(void)
{
    return native_status(0);
}

static bool module_id_is_zero(arbor_module_id id)
{
    return id.high == 0u && id.low == 0u;
}

static bool module_id_equal(arbor_module_id left, arbor_module_id right)
{
    return left.high == right.high && left.low == right.low;
}

static bool capability_id_is_zero(arbor_capability_id id)
{
    return id.high == 0u && id.low == 0u;
}

static bool capability_id_equal(arbor_capability_id left, arbor_capability_id right)
{
    return left.high == right.high && left.low == right.low;
}

static arbor_status checked_mul_u64(uint64_t left, uint64_t right, uint64_t *out)
{
    if (out == NULL) {
        return native_status(-EINVAL);
    }

    arbor_asm_result_u64 result = u64_mul_checked(left, right);
    if (result.status != 0) {
        return native_status(result.status);
    }

    *out = result.value;
    return ok_status();
}

static arbor_status checked_add_u64(uint64_t left, uint64_t right, uint64_t *out)
{
    if (out == NULL) {
        return native_status(-EINVAL);
    }

    arbor_asm_result_u64 result = u64_add_checked(left, right);
    if (result.status != 0) {
        return native_status(result.status);
    }

    *out = result.value;
    return ok_status();
}

static arbor_status region_validate(af3_region region)
{
    if (region.length == 0u) {
        return ok_status();
    }
    if (region.pointer == NULL) {
        return native_status(-EINVAL);
    }

    uint64_t end = 0u;
    return checked_add_u64((uint64_t)(uintptr_t)region.pointer, region.length, &end);
}

static arbor_status region_overlap(af3_region left, af3_region right, bool *overlap_out)
{
    if (overlap_out == NULL) {
        return native_status(-EINVAL);
    }

    arbor_status status = region_validate(left);
    if (status.native != 0) {
        return status;
    }
    status = region_validate(right);
    if (status.native != 0) {
        return status;
    }

    arbor_asm_result_u64 result = range_overlaps(
        (uint64_t)(uintptr_t)left.pointer,
        left.length,
        (uint64_t)(uintptr_t)right.pointer,
        right.length);
    if (result.status != 0) {
        return native_status(result.status);
    }

    *overlap_out = result.value != 0u;
    return ok_status();
}

static arbor_status regions_require_disjoint(af3_region left, af3_region right)
{
    bool overlap = false;
    arbor_status status = region_overlap(left, right, &overlap);
    if (status.native != 0) {
        return status;
    }
    if (overlap) {
        return native_status(-EINVAL);
    }
    return ok_status();
}

static arbor_status array_region(
    const void *pointer,
    uint64_t count,
    uint64_t element_size,
    af3_region *region_out)
{
    if (region_out == NULL) {
        return native_status(-EINVAL);
    }

    uint64_t bytes = 0u;
    arbor_status status = checked_mul_u64(count, element_size, &bytes);
    if (status.native != 0) {
        return status;
    }
    if (bytes != 0u && pointer == NULL) {
        return native_status(-EINVAL);
    }

    af3_region candidate = {pointer, bytes};
    status = region_validate(candidate);
    if (status.native != 0) {
        return status;
    }

    *region_out = candidate;
    return ok_status();
}

static arbor_status catalog_module_index_direct(
    const arbor_capability_catalog *catalog,
    arbor_module_id id,
    uint64_t *module_index_out)
{
    if (catalog == NULL || module_index_out == NULL || module_id_is_zero(id)) {
        return native_status(-EINVAL);
    }

    for (uint64_t i = 0u; i < catalog->module_count; ++i) {
        if (module_id_equal(catalog->modules[i].id, id)) {
            *module_index_out = i;
            return ok_status();
        }
    }
    return native_status(-ENOENT);
}

static arbor_status module_export_index_for_capability(
    const arbor_module_descriptor *module,
    arbor_capability_id id,
    uint64_t *export_index_out)
{
    if (module == NULL || export_index_out == NULL || capability_id_is_zero(id)) {
        return native_status(-EINVAL);
    }

    for (uint64_t i = 0u; i < module->provides_count; ++i) {
        if (capability_id_equal(module->provides[i].id, id)) {
            *export_index_out = i;
            return ok_status();
        }
    }
    return native_status(-ENOENT);
}

static bool descriptor_declares_export(
    const arbor_application_service_module_descriptor *descriptor,
    uint64_t export_index)
{
    for (uint64_t i = 0u; i < descriptor->service_export_count; ++i) {
        if (descriptor->service_export_indices[i] == export_index) {
            return true;
        }
    }
    return false;
}

static bool callbacks_are_passive(
    const arbor_application_service_module_descriptor *descriptor)
{
    return descriptor->prepare == NULL && descriptor->rollback == NULL && descriptor->stop == NULL;
}

static bool callbacks_are_active(
    const arbor_application_service_module_descriptor *descriptor)
{
    return descriptor->prepare != NULL && descriptor->rollback != NULL && descriptor->stop != NULL;
}

arbor_status arbor_application_service_status_from_native(int64_t native)
{
    if (native > 0) {
        return native_status(-EINVAL);
    }
    return native_status(native);
}

arbor_status arbor_application_service_interface_validate(
    const arbor_capability_binding *binding)
{
    if (binding == NULL || capability_id_is_zero(binding->id) ||
        binding->version.major == 0u || binding->reserved0 != 0u ||
        binding->flags != ARBOR_CAPABILITY_FLAGS_NONE || binding->interface_table == NULL ||
        binding->interface_size < (uint32_t)sizeof(arbor_application_service_interface_header)) {
        return native_status(-EINVAL);
    }

    if (((uintptr_t)binding->interface_table %
            (uintptr_t)_Alignof(arbor_application_service_interface_header)) != 0u) {
        return native_status(-EINVAL);
    }

    const arbor_application_service_interface_header *header =
        (const arbor_application_service_interface_header *)binding->interface_table;

    if (header->abi_version != ARBOR_APPLICATION_SERVICE_INTERFACE_ABI_VERSION) {
        return native_status(-EPROTONOSUPPORT);
    }
    if (header->struct_size != binding->interface_size ||
        header->struct_size < (uint32_t)sizeof(*header) ||
        header->flags != ARBOR_APPLICATION_SERVICE_INTERFACE_FLAGS_NONE) {
        return native_status(-EINVAL);
    }

    return ok_status();
}

static arbor_status binding_for_service_export(
    const arbor_capability_catalog *catalog,
    uint64_t module_index,
    uint64_t export_index,
    arbor_capability_binding *binding_out)
{
    if (catalog == NULL || binding_out == NULL || module_index >= catalog->module_count) {
        return native_status(-EINVAL);
    }

    const arbor_module_descriptor *module = &catalog->modules[module_index];
    if (export_index >= module->provides_count) {
        return native_status(-EINVAL);
    }

    const arbor_capability_export *provided = &module->provides[export_index];
    arbor_capability_binding binding = {0};
    arbor_status status = arbor_capability_catalog_find(
        catalog,
        provided->id,
        provided->version,
        provided->interface_size,
        &binding);
    if (status.native != 0) {
        return status;
    }

    if (binding.provider_module_index != module_index ||
        binding.interface_table != provided->interface_table ||
        binding.provider_context != provided->provider_context ||
        binding.interface_size != provided->interface_size ||
        binding.version.major != provided->version.major ||
        binding.version.minor != provided->version.minor) {
        return native_status(-EINVAL);
    }

    *binding_out = binding;
    return ok_status();
}

static arbor_status descriptor_validate(
    const arbor_capability_catalog *catalog,
    const arbor_application_service_module_descriptor *descriptor,
    uint64_t *module_index_out)
{
    if (catalog == NULL || descriptor == NULL || module_index_out == NULL ||
        module_id_is_zero(descriptor->module_id)) {
        return native_status(-EINVAL);
    }
    if (descriptor->abi_version != ARBOR_APPLICATION_SERVICE_MODULE_DESCRIPTOR_ABI_VERSION ||
        descriptor->struct_size != (uint32_t)sizeof(*descriptor) ||
        descriptor->flags != ARBOR_APPLICATION_SERVICE_MODULE_FLAGS_NONE ||
        descriptor->service_export_count == 0u ||
        descriptor->service_export_indices == NULL) {
        return native_status(-EINVAL);
    }
    if (!callbacks_are_passive(descriptor) && !callbacks_are_active(descriptor)) {
        return native_status(-EINVAL);
    }

    uint64_t module_index = 0u;
    arbor_status status = catalog_module_index_direct(catalog, descriptor->module_id, &module_index);
    if (status.native != 0) {
        return status;
    }

    const arbor_module_descriptor *module = &catalog->modules[module_index];
    if (descriptor->service_export_count > module->provides_count) {
        return native_status(-EINVAL);
    }

    for (uint64_t i = 0u; i < descriptor->service_export_count; ++i) {
        uint64_t export_index = descriptor->service_export_indices[i];
        if (export_index >= module->provides_count) {
            return native_status(-EINVAL);
        }
        for (uint64_t earlier = 0u; earlier < i; ++earlier) {
            if (descriptor->service_export_indices[earlier] == export_index) {
                return native_status(-EEXIST);
            }
        }

        arbor_capability_binding binding = {0};
        status = binding_for_service_export(catalog, module_index, export_index, &binding);
        if (status.native != 0) {
            return status;
        }
        status = arbor_application_service_interface_validate(&binding);
        if (status.native != 0) {
            return status;
        }
    }

    *module_index_out = module_index;
    return ok_status();
}

static arbor_status descriptor_set_validate(
    const arbor_capability_catalog *catalog,
    const arbor_application_service_module_descriptor *service_modules,
    uint64_t managed_module_count)
{
    arbor_status status = arbor_capability_catalog_validate(catalog);
    if (status.native != 0) {
        return status;
    }

    if (managed_module_count == 0u) {
        return ok_status();
    }
    if (service_modules == NULL || managed_module_count > catalog->module_count) {
        return native_status(-EINVAL);
    }

    uint64_t descriptor_bytes = 0u;
    status = checked_mul_u64(
        managed_module_count,
        (uint64_t)sizeof(arbor_application_service_module_descriptor),
        &descriptor_bytes);
    if (status.native != 0) {
        return status;
    }
    (void)descriptor_bytes;

    for (uint64_t i = 0u; i < managed_module_count; ++i) {
        uint64_t module_index = 0u;
        status = descriptor_validate(catalog, &service_modules[i], &module_index);
        if (status.native != 0) {
            return status;
        }
        (void)module_index;

        for (uint64_t earlier = 0u; earlier < i; ++earlier) {
            if (module_id_equal(service_modules[earlier].module_id, service_modules[i].module_id)) {
                return native_status(-EEXIST);
            }
        }
    }

    return ok_status();
}

static arbor_status immutable_inputs_require_disjoint(
    const arbor_capability_catalog *catalog,
    const arbor_application_service_module_descriptor *service_modules,
    uint64_t managed_module_count,
    af3_region mutable_region)
{
    arbor_status status = region_validate(mutable_region);
    if (status.native != 0) {
        return status;
    }

#define REQUIRE_DISJOINT_IMMUTABLE(region_value) \
    do { \
        af3_region af3_immutable_region_ = (region_value); \
        status = regions_require_disjoint(mutable_region, af3_immutable_region_); \
        if (status.native != 0) { \
            return status; \
        } \
    } while (0)

    REQUIRE_DISJOINT_IMMUTABLE(
        ((af3_region){catalog, (uint64_t)catalog->struct_size}));

    af3_region region = {0};
    status = array_region(
        catalog->modules,
        catalog->module_count,
        (uint64_t)sizeof(arbor_module_descriptor),
        &region);
    if (status.native != 0) {
        return status;
    }
    REQUIRE_DISJOINT_IMMUTABLE(region);

    status = array_region(
        catalog->bindings,
        catalog->binding_count,
        (uint64_t)sizeof(arbor_capability_binding),
        &region);
    if (status.native != 0) {
        return status;
    }
    REQUIRE_DISJOINT_IMMUTABLE(region);

    status = array_region(
        catalog->resolutions,
        catalog->resolution_count,
        (uint64_t)sizeof(arbor_capability_resolution),
        &region);
    if (status.native != 0) {
        return status;
    }
    REQUIRE_DISJOINT_IMMUTABLE(region);

    status = array_region(
        catalog->module_order,
        catalog->module_count,
        (uint64_t)sizeof(uint64_t),
        &region);
    if (status.native != 0) {
        return status;
    }
    REQUIRE_DISJOINT_IMMUTABLE(region);

    status = array_region(
        service_modules,
        managed_module_count,
        (uint64_t)sizeof(arbor_application_service_module_descriptor),
        &region);
    if (status.native != 0) {
        return status;
    }
    REQUIRE_DISJOINT_IMMUTABLE(region);

    for (uint64_t module_index = 0u; module_index < catalog->module_count; ++module_index) {
        const arbor_module_descriptor *module = &catalog->modules[module_index];

        status = array_region(
            module->provides,
            module->provides_count,
            (uint64_t)sizeof(arbor_capability_export),
            &region);
        if (status.native != 0) {
            return status;
        }
        REQUIRE_DISJOINT_IMMUTABLE(region);

        status = array_region(
            module->consumes,
            module->consumes_count,
            (uint64_t)sizeof(arbor_capability_requirement),
            &region);
        if (status.native != 0) {
            return status;
        }
        REQUIRE_DISJOINT_IMMUTABLE(region);

        for (uint64_t export_index = 0u; export_index < module->provides_count; ++export_index) {
            const arbor_capability_export *provided = &module->provides[export_index];
            region = (af3_region){provided->interface_table, provided->interface_size};
            status = region_validate(region);
            if (status.native != 0) {
                return status;
            }
            REQUIRE_DISJOINT_IMMUTABLE(region);
        }
    }

    for (uint64_t descriptor_index = 0u;
         descriptor_index < managed_module_count;
         ++descriptor_index) {
        const arbor_application_service_module_descriptor *descriptor =
            &service_modules[descriptor_index];
        status = array_region(
            descriptor->service_export_indices,
            descriptor->service_export_count,
            (uint64_t)sizeof(uint64_t),
            &region);
        if (status.native != 0) {
            return status;
        }
        REQUIRE_DISJOINT_IMMUTABLE(region);
    }

#undef REQUIRE_DISJOINT_IMMUTABLE

    return ok_status();
}

arbor_status arbor_application_runtime_measure(
    const arbor_capability_catalog *catalog,
    const arbor_application_service_module_descriptor *service_modules,
    uint64_t managed_module_count,
    arbor_application_runtime_requirements *out)
{
    if (out == NULL) {
        return native_status(-EINVAL);
    }

    arbor_status status = descriptor_set_validate(catalog, service_modules, managed_module_count);
    if (status.native != 0) {
        return status;
    }

    uint64_t ignored = 0u;
    status = checked_mul_u64(
        managed_module_count,
        (uint64_t)sizeof(arbor_application_service_runtime_record),
        &ignored);
    if (status.native != 0) {
        return status;
    }

    uint64_t module_map_count = managed_module_count == 0u ? 0u : catalog->module_count;
    status = checked_mul_u64(module_map_count, (uint64_t)sizeof(uint64_t), &ignored);
    if (status.native != 0) {
        return status;
    }

    status = immutable_inputs_require_disjoint(
        catalog,
        service_modules,
        managed_module_count,
        (af3_region){out, (uint64_t)sizeof(*out)});
    if (status.native != 0) {
        return status;
    }

    arbor_application_runtime_requirements candidate = {
        managed_module_count,
        module_map_count
    };
    (void)memory_copy(out, &candidate, (uint64_t)sizeof(candidate));
    return ok_status();
}

static arbor_status storage_workspace_shape_validate(
    const arbor_application_runtime_requirements *requirements,
    const arbor_application_runtime_storage *storage,
    const arbor_application_runtime_workspace *workspace)
{
    if (requirements == NULL || storage == NULL || workspace == NULL) {
        return native_status(-EINVAL);
    }

    if (storage->record_capacity < requirements->managed_module_record_count ||
        workspace->record_capacity < requirements->managed_module_record_count ||
        workspace->module_capacity < requirements->module_map_count) {
        return native_status(-ENOSPC);
    }

    if ((storage->record_capacity != 0u && storage->records == NULL) ||
        (workspace->record_capacity != 0u && workspace->records == NULL) ||
        (workspace->module_capacity != 0u && workspace->module_to_descriptor_index == NULL)) {
        return native_status(-EINVAL);
    }

    if ((requirements->managed_module_record_count != 0u &&
            (storage->records == NULL || workspace->records == NULL)) ||
        (requirements->module_map_count != 0u && workspace->module_to_descriptor_index == NULL)) {
        return native_status(-EINVAL);
    }

    return ok_status();
}

static arbor_status mutable_regions_validate(
    const arbor_capability_catalog *catalog,
    const arbor_application_service_module_descriptor *service_modules,
    uint64_t managed_module_count,
    const arbor_application_runtime_storage *storage,
    const arbor_application_runtime_workspace *workspace,
    arbor_application_runtime *runtime_out)
{
    uint64_t storage_record_bytes = 0u;
    uint64_t workspace_record_bytes = 0u;
    uint64_t workspace_map_bytes = 0u;

    arbor_status status = checked_mul_u64(
        storage->record_capacity,
        (uint64_t)sizeof(arbor_application_service_runtime_record),
        &storage_record_bytes);
    if (status.native != 0) {
        return status;
    }
    status = checked_mul_u64(
        workspace->record_capacity,
        (uint64_t)sizeof(arbor_application_service_runtime_record),
        &workspace_record_bytes);
    if (status.native != 0) {
        return status;
    }
    status = checked_mul_u64(
        workspace->module_capacity,
        (uint64_t)sizeof(uint64_t),
        &workspace_map_bytes);
    if (status.native != 0) {
        return status;
    }

    af3_region mutable_regions[] = {
        {storage, (uint64_t)sizeof(*storage)},
        {workspace, (uint64_t)sizeof(*workspace)},
        {runtime_out, (uint64_t)sizeof(*runtime_out)},
        {storage->records, storage_record_bytes},
        {workspace->records, workspace_record_bytes},
        {workspace->module_to_descriptor_index, workspace_map_bytes}
    };
    const uint64_t mutable_count =
        (uint64_t)(sizeof(mutable_regions) / sizeof(mutable_regions[0]));

    for (uint64_t i = 0u; i < mutable_count; ++i) {
        status = region_validate(mutable_regions[i]);
        if (status.native != 0) {
            return status;
        }
        for (uint64_t j = 0u; j < i; ++j) {
            status = regions_require_disjoint(mutable_regions[i], mutable_regions[j]);
            if (status.native != 0) {
                return status;
            }
        }
    }

    for (uint64_t i = 0u; i < mutable_count; ++i) {
        status = immutable_inputs_require_disjoint(
            catalog,
            service_modules,
            managed_module_count,
            mutable_regions[i]);
        if (status.native != 0) {
            return status;
        }
    }


    return ok_status();
}

static arbor_status build_module_map_and_order(
    const arbor_capability_catalog *catalog,
    const arbor_application_service_module_descriptor *service_modules,
    uint64_t managed_module_count,
    arbor_application_runtime_workspace *workspace)
{
    uint64_t record_bytes = 0u;
    arbor_status status = checked_mul_u64(
        managed_module_count,
        (uint64_t)sizeof(arbor_application_service_runtime_record),
        &record_bytes);
    if (status.native != 0) {
        return status;
    }

    uint64_t map_bytes = 0u;
    status = checked_mul_u64(
        catalog->module_count,
        (uint64_t)sizeof(uint64_t),
        &map_bytes);
    if (status.native != 0) {
        return status;
    }

    if (record_bytes != 0u) {
        (void)memory_zero(workspace->records, record_bytes);
    }
    if (map_bytes != 0u) {
        (void)memory_set(workspace->module_to_descriptor_index, UINT64_C(0xff), map_bytes);
    }

    for (uint64_t descriptor_index = 0u;
         descriptor_index < managed_module_count;
         ++descriptor_index) {
        uint64_t module_index = 0u;
        status = catalog_module_index_direct(
            catalog,
            service_modules[descriptor_index].module_id,
            &module_index);
        if (status.native != 0) {
            return status;
        }
        if (workspace->module_to_descriptor_index[module_index] != UINT64_MAX) {
            return native_status(-EEXIST);
        }
        workspace->module_to_descriptor_index[module_index] = descriptor_index;
    }

    uint64_t record_index = 0u;
    for (uint64_t order_index = 0u; order_index < catalog->module_count; ++order_index) {
        uint64_t module_index = catalog->module_order[order_index];
        if (module_index >= catalog->module_count) {
            return native_status(-EINVAL);
        }
        uint64_t descriptor_index = workspace->module_to_descriptor_index[module_index];
        if (descriptor_index == UINT64_MAX) {
            continue;
        }
        if (descriptor_index >= managed_module_count || record_index >= managed_module_count) {
            return native_status(-EINVAL);
        }

        workspace->records[record_index] = (arbor_application_service_runtime_record){
            descriptor_index,
            module_index,
            ARBOR_APPLICATION_SERVICE_RECORD_EMPTY,
            0u,
            0
        };
        record_index += UINT64_C(1);
    }

    if (record_index != managed_module_count) {
        return native_status(-EINVAL);
    }
    return ok_status();
}

static arbor_application_service_runtime_record *workspace_record_for_descriptor(
    const arbor_application_runtime_workspace *workspace,
    uint64_t managed_module_count,
    uint64_t descriptor_index)
{
    if (workspace == NULL || workspace->records == NULL) {
        return NULL;
    }
    for (uint64_t i = 0u; i < managed_module_count; ++i) {
        if (workspace->records[i].descriptor_index == descriptor_index) {
            return &workspace->records[i];
        }
    }
    return NULL;
}

static arbor_status validate_managed_binding_if_service_export(
    const arbor_application_service_prepare_context *prepare_context,
    const arbor_capability_binding *binding)
{
    if (prepare_context == NULL || binding == NULL ||
        binding->provider_module_index >= prepare_context->catalog->module_count) {
        return native_status(-EINVAL);
    }

    uint64_t descriptor_index =
        prepare_context->workspace->module_to_descriptor_index[binding->provider_module_index];
    if (descriptor_index == UINT64_MAX) {
        return ok_status();
    }
    if (descriptor_index >= prepare_context->managed_module_count) {
        return native_status(-EINVAL);
    }

    const arbor_application_service_module_descriptor *descriptor =
        &prepare_context->service_modules[descriptor_index];
    arbor_application_service_runtime_record *record = workspace_record_for_descriptor(
        prepare_context->workspace,
        prepare_context->managed_module_count,
        descriptor_index);
    if (record == NULL || record->state != ARBOR_APPLICATION_SERVICE_RECORD_READY) {
        return native_status(-EAGAIN);
    }

    const arbor_module_descriptor *provider_module =
        &prepare_context->catalog->modules[binding->provider_module_index];
    uint64_t export_index = 0u;
    arbor_status status = module_export_index_for_capability(
        provider_module,
        binding->id,
        &export_index);
    if (status.native != 0) {
        return status;
    }

    if (descriptor_declares_export(descriptor, export_index)) {
        return arbor_application_service_interface_validate(binding);
    }
    return ok_status();
}

static arbor_status prepare_binding_output_region_validate(
    const arbor_application_service_prepare_context *prepare_context,
    arbor_capability_binding *binding_out)
{
    af3_region output_region = {binding_out, (uint64_t)sizeof(*binding_out)};
    arbor_status status = immutable_inputs_require_disjoint(
        prepare_context->catalog,
        prepare_context->service_modules,
        prepare_context->managed_module_count,
        output_region);
    if (status.native != 0) {
        return status;
    }

    af3_region internal_regions[] = {
        {prepare_context, (uint64_t)sizeof(*prepare_context)},
        {prepare_context->workspace, (uint64_t)sizeof(*prepare_context->workspace)}
    };
    for (uint64_t i = 0u;
         i < (uint64_t)(sizeof(internal_regions) / sizeof(internal_regions[0]));
         ++i) {
        status = regions_require_disjoint(output_region, internal_regions[i]);
        if (status.native != 0) {
            return status;
        }
    }

    uint64_t record_bytes = 0u;
    status = checked_mul_u64(
        prepare_context->workspace->record_capacity,
        (uint64_t)sizeof(arbor_application_service_runtime_record),
        &record_bytes);
    if (status.native != 0) {
        return status;
    }
    status = regions_require_disjoint(
        output_region,
        (af3_region){prepare_context->workspace->records, record_bytes});
    if (status.native != 0) {
        return status;
    }

    uint64_t map_bytes = 0u;
    status = checked_mul_u64(
        prepare_context->workspace->module_capacity,
        (uint64_t)sizeof(uint64_t),
        &map_bytes);
    if (status.native != 0) {
        return status;
    }
    return regions_require_disjoint(
        output_region,
        (af3_region){prepare_context->workspace->module_to_descriptor_index, map_bytes});
}

arbor_status arbor_application_service_prepare_resolve(
    const arbor_application_service_prepare_context *prepare_context,
    uint64_t requirement_index,
    arbor_capability_binding *binding_out)
{
    if (prepare_context == NULL || binding_out == NULL ||
        prepare_context->magic != AF3_PREPARE_CONTEXT_MAGIC ||
        prepare_context->catalog == NULL || prepare_context->workspace == NULL) {
        return native_status(-EINVAL);
    }

    arbor_status status = descriptor_set_validate(
        prepare_context->catalog,
        prepare_context->service_modules,
        prepare_context->managed_module_count);
    if (status.native != 0) {
        return status;
    }

    if (prepare_context->managed_module_count == 0u ||
        prepare_context->consumer_module_index >= prepare_context->catalog->module_count ||
        prepare_context->current_record_index >= prepare_context->managed_module_count ||
        prepare_context->workspace->record_capacity < prepare_context->managed_module_count ||
        prepare_context->workspace->module_capacity < prepare_context->catalog->module_count ||
        prepare_context->workspace->records == NULL ||
        prepare_context->workspace->module_to_descriptor_index == NULL) {
        return native_status(-EINVAL);
    }

    const arbor_application_service_runtime_record *current_record =
        &prepare_context->workspace->records[prepare_context->current_record_index];
    if (current_record->module_index != prepare_context->consumer_module_index ||
        current_record->state != ARBOR_APPLICATION_SERVICE_RECORD_PREPARING) {
        return native_status(-EINVAL);
    }

    status = prepare_binding_output_region_validate(prepare_context, binding_out);
    if (status.native != 0) {
        return status;
    }

    arbor_capability_binding candidate = {0};
    status = arbor_capability_catalog_resolve(
        prepare_context->catalog,
        prepare_context->consumer_module_index,
        requirement_index,
        &candidate);
    if (status.native != 0) {
        return status;
    }

    status = validate_managed_binding_if_service_export(prepare_context, &candidate);
    if (status.native != 0) {
        return status;
    }

    (void)memory_copy(binding_out, &candidate, (uint64_t)sizeof(candidate));
    return ok_status();
}

static void rollback_prepared_prefix(
    const arbor_application_service_module_descriptor *service_modules,
    arbor_application_runtime_workspace *workspace,
    uint64_t failed_record_index)
{
    for (uint64_t cursor = failed_record_index; cursor > 0u; --cursor) {
        arbor_application_service_runtime_record *record = &workspace->records[cursor - UINT64_C(1)];
        if (record->state != ARBOR_APPLICATION_SERVICE_RECORD_READY) {
            continue;
        }

        const arbor_application_service_module_descriptor *descriptor =
            &service_modules[record->descriptor_index];
        record->state = ARBOR_APPLICATION_SERVICE_RECORD_ROLLING_BACK;
        if (descriptor->rollback != NULL) {
            descriptor->rollback(descriptor->module_context);
        }
        record->state = ARBOR_APPLICATION_SERVICE_RECORD_ROLLED_BACK;
        record->stop_native = 0;
    }
}

static arbor_status workspace_ready_records_validate(
    const arbor_capability_catalog *catalog,
    const arbor_application_service_module_descriptor *service_modules,
    uint64_t managed_module_count,
    const arbor_application_service_runtime_record *records)
{
    uint64_t expected_record = 0u;

    for (uint64_t order_index = 0u; order_index < catalog->module_count; ++order_index) {
        uint64_t module_index = catalog->module_order[order_index];
        uint64_t descriptor_index = UINT64_MAX;

        for (uint64_t i = 0u; i < managed_module_count; ++i) {
            if (module_id_equal(service_modules[i].module_id, catalog->modules[module_index].id)) {
                descriptor_index = i;
                break;
            }
        }

        if (descriptor_index == UINT64_MAX) {
            continue;
        }
        if (expected_record >= managed_module_count) {
            return native_status(-EINVAL);
        }

        const arbor_application_service_runtime_record *record = &records[expected_record];
        if (record->descriptor_index != descriptor_index ||
            record->module_index != module_index ||
            record->state != ARBOR_APPLICATION_SERVICE_RECORD_READY ||
            record->reserved0 != 0u || record->stop_native != 0) {
            return native_status(-EINVAL);
        }
        expected_record += UINT64_C(1);
    }

    if (expected_record != managed_module_count) {
        return native_status(-EINVAL);
    }
    return ok_status();
}

arbor_status arbor_application_runtime_prepare(
    const arbor_capability_catalog *catalog,
    const arbor_application_service_module_descriptor *service_modules,
    uint64_t managed_module_count,
    arbor_application_runtime_storage *storage,
    arbor_application_runtime_workspace *workspace,
    arbor_application_runtime *runtime_out)
{
    if (runtime_out == NULL || storage == NULL || workspace == NULL) {
        return native_status(-EINVAL);
    }

    arbor_application_runtime_requirements requirements = {0u, 0u};
    arbor_status status = arbor_application_runtime_measure(
        catalog,
        service_modules,
        managed_module_count,
        &requirements);
    if (status.native != 0) {
        return status;
    }

    status = storage_workspace_shape_validate(&requirements, storage, workspace);
    if (status.native != 0) {
        return status;
    }

    status = mutable_regions_validate(
        catalog,
        service_modules,
        managed_module_count,
        storage,
        workspace,
        runtime_out);
    if (status.native != 0) {
        return status;
    }

    if (managed_module_count != 0u) {
        status = build_module_map_and_order(
            catalog,
            service_modules,
            managed_module_count,
            workspace);
        if (status.native != 0) {
            return status;
        }

        for (uint64_t record_index = 0u; record_index < managed_module_count; ++record_index) {
            arbor_application_service_runtime_record *record = &workspace->records[record_index];
            const arbor_application_service_module_descriptor *descriptor =
                &service_modules[record->descriptor_index];

            record->state = ARBOR_APPLICATION_SERVICE_RECORD_PREPARING;
            record->stop_native = 0;

            if (descriptor->prepare != NULL) {
                arbor_application_service_prepare_context prepare_context = {
                    AF3_PREPARE_CONTEXT_MAGIC,
                    catalog,
                    service_modules,
                    managed_module_count,
                    workspace,
                    record_index,
                    record->module_index
                };

                int64_t raw = descriptor->prepare(
                    descriptor->module_context,
                    &prepare_context);
                status = arbor_application_service_status_from_native(raw);
                if (status.native != 0) {
                    record->state = ARBOR_APPLICATION_SERVICE_RECORD_PREPARE_FAILED;
                    rollback_prepared_prefix(service_modules, workspace, record_index);
                    return status;
                }
            }

            record->state = ARBOR_APPLICATION_SERVICE_RECORD_READY;
        }

        status = workspace_ready_records_validate(
            catalog,
            service_modules,
            managed_module_count,
            workspace->records);
        if (status.native != 0) {
            rollback_prepared_prefix(service_modules, workspace, managed_module_count);
            return status;
        }
    }

    uint64_t record_bytes = 0u;
    status = checked_mul_u64(
        managed_module_count,
        (uint64_t)sizeof(arbor_application_service_runtime_record),
        &record_bytes);
    if (status.native != 0) {
        if (managed_module_count != 0u) {
            rollback_prepared_prefix(service_modules, workspace, managed_module_count);
        }
        return status;
    }

    if (record_bytes != 0u) {
        (void)memory_copy(storage->records, workspace->records, record_bytes);
    }

    arbor_application_runtime candidate = {
        ARBOR_APPLICATION_RUNTIME_ABI_VERSION,
        (uint32_t)sizeof(arbor_application_runtime),
        ARBOR_APPLICATION_RUNTIME_FLAGS_NONE,
        catalog,
        managed_module_count == 0u ? NULL : service_modules,
        managed_module_count,
        managed_module_count == 0u ? NULL : storage->records,
        ARBOR_APPLICATION_RUNTIME_READY,
        0u
    };

    (void)memory_copy(runtime_out, &candidate, (uint64_t)sizeof(candidate));
    return ok_status();
}

static arbor_status runtime_shape_validate(const arbor_application_runtime *runtime)
{
    if (runtime == NULL || runtime->catalog == NULL ||
        runtime->abi_version != ARBOR_APPLICATION_RUNTIME_ABI_VERSION ||
        runtime->struct_size < (uint32_t)sizeof(arbor_application_runtime) ||
        runtime->flags != ARBOR_APPLICATION_RUNTIME_FLAGS_NONE ||
        runtime->reserved0 != 0u) {
        return native_status(-EINVAL);
    }

    if (runtime->managed_module_count == 0u) {
        if (runtime->service_modules != NULL || runtime->records != NULL) {
            return native_status(-EINVAL);
        }
    } else if (runtime->service_modules == NULL || runtime->records == NULL) {
        return native_status(-EINVAL);
    }

    if (runtime->state != ARBOR_APPLICATION_RUNTIME_READY &&
        runtime->state != ARBOR_APPLICATION_RUNTIME_STOPPING &&
        runtime->state != ARBOR_APPLICATION_RUNTIME_STOPPED &&
        runtime->state != ARBOR_APPLICATION_RUNTIME_STOP_FAILED) {
        return native_status(-EINVAL);
    }

    return ok_status();
}

static arbor_status runtime_record_state_validate(
    const arbor_application_runtime *runtime,
    const arbor_application_service_runtime_record *record,
    bool *stop_failed_seen)
{
    if (runtime == NULL || record == NULL || stop_failed_seen == NULL || record->reserved0 != 0u) {
        return native_status(-EINVAL);
    }

    switch (runtime->state) {
    case ARBOR_APPLICATION_RUNTIME_READY:
        if (record->state != ARBOR_APPLICATION_SERVICE_RECORD_READY || record->stop_native != 0) {
            return native_status(-EINVAL);
        }
        break;

    case ARBOR_APPLICATION_RUNTIME_STOPPING:
        if (record->state != ARBOR_APPLICATION_SERVICE_RECORD_READY &&
            record->state != ARBOR_APPLICATION_SERVICE_RECORD_STOPPING &&
            record->state != ARBOR_APPLICATION_SERVICE_RECORD_STOPPED &&
            record->state != ARBOR_APPLICATION_SERVICE_RECORD_STOP_FAILED) {
            return native_status(-EINVAL);
        }
        if (record->state == ARBOR_APPLICATION_SERVICE_RECORD_STOP_FAILED) {
            if (record->stop_native == 0) {
                return native_status(-EINVAL);
            }
            *stop_failed_seen = true;
        } else if (record->stop_native != 0) {
            return native_status(-EINVAL);
        }
        break;

    case ARBOR_APPLICATION_RUNTIME_STOPPED:
        if (record->state != ARBOR_APPLICATION_SERVICE_RECORD_STOPPED || record->stop_native != 0) {
            return native_status(-EINVAL);
        }
        break;

    case ARBOR_APPLICATION_RUNTIME_STOP_FAILED:
        if (record->state == ARBOR_APPLICATION_SERVICE_RECORD_STOP_FAILED) {
            if (record->stop_native == 0) {
                return native_status(-EINVAL);
            }
            *stop_failed_seen = true;
        } else if (record->state == ARBOR_APPLICATION_SERVICE_RECORD_STOPPED) {
            if (record->stop_native != 0) {
                return native_status(-EINVAL);
            }
        } else {
            return native_status(-EINVAL);
        }
        break;

    default:
        return native_status(-EINVAL);
    }

    return ok_status();
}

static arbor_status runtime_regions_validate(const arbor_application_runtime *runtime)
{
    uint64_t record_bytes = 0u;
    arbor_status status = checked_mul_u64(
        runtime->managed_module_count,
        (uint64_t)sizeof(arbor_application_service_runtime_record),
        &record_bytes);
    if (status.native != 0) {
        return status;
    }

    af3_region runtime_region = {runtime, (uint64_t)runtime->struct_size};
    af3_region records_region = {runtime->records, record_bytes};

    status = regions_require_disjoint(runtime_region, records_region);
    if (status.native != 0) {
        return status;
    }

    status = immutable_inputs_require_disjoint(
        runtime->catalog,
        runtime->service_modules,
        runtime->managed_module_count,
        runtime_region);
    if (status.native != 0) {
        return status;
    }

    return immutable_inputs_require_disjoint(
        runtime->catalog,
        runtime->service_modules,
        runtime->managed_module_count,
        records_region);
}

static arbor_status runtime_output_region_validate(
    const arbor_application_runtime *runtime,
    af3_region output_region)
{
    arbor_status status = immutable_inputs_require_disjoint(
        runtime->catalog,
        runtime->service_modules,
        runtime->managed_module_count,
        output_region);
    if (status.native != 0) {
        return status;
    }

    status = regions_require_disjoint(
        output_region,
        (af3_region){runtime, (uint64_t)runtime->struct_size});
    if (status.native != 0) {
        return status;
    }

    uint64_t record_bytes = 0u;
    status = checked_mul_u64(
        runtime->managed_module_count,
        (uint64_t)sizeof(arbor_application_service_runtime_record),
        &record_bytes);
    if (status.native != 0) {
        return status;
    }

    return regions_require_disjoint(
        output_region,
        (af3_region){runtime->records, record_bytes});
}

arbor_status arbor_application_runtime_validate(const arbor_application_runtime *runtime)
{
    arbor_status status = runtime_shape_validate(runtime);
    if (status.native != 0) {
        return status;
    }

    status = descriptor_set_validate(
        runtime->catalog,
        runtime->service_modules,
        runtime->managed_module_count);
    if (status.native != 0) {
        return status;
    }

    status = runtime_regions_validate(runtime);
    if (status.native != 0) {
        return status;
    }

    uint64_t expected_record = 0u;
    bool stop_failed_seen = false;

    for (uint64_t order_index = 0u;
         order_index < runtime->catalog->module_count;
         ++order_index) {
        uint64_t module_index = runtime->catalog->module_order[order_index];
        if (module_index >= runtime->catalog->module_count) {
            return native_status(-EINVAL);
        }

        uint64_t descriptor_index = UINT64_MAX;
        for (uint64_t i = 0u; i < runtime->managed_module_count; ++i) {
            if (module_id_equal(
                    runtime->service_modules[i].module_id,
                    runtime->catalog->modules[module_index].id)) {
                descriptor_index = i;
                break;
            }
        }

        if (descriptor_index == UINT64_MAX) {
            continue;
        }
        if (expected_record >= runtime->managed_module_count) {
            return native_status(-EINVAL);
        }

        const arbor_application_service_runtime_record *record =
            &runtime->records[expected_record];
        if (record->descriptor_index != descriptor_index || record->module_index != module_index) {
            return native_status(-EINVAL);
        }

        status = runtime_record_state_validate(runtime, record, &stop_failed_seen);
        if (status.native != 0) {
            return status;
        }
        expected_record += UINT64_C(1);
    }

    if (expected_record != runtime->managed_module_count) {
        return native_status(-EINVAL);
    }
    if (runtime->state == ARBOR_APPLICATION_RUNTIME_STOP_FAILED && !stop_failed_seen) {
        return native_status(-EINVAL);
    }
    if (runtime->state != ARBOR_APPLICATION_RUNTIME_STOP_FAILED &&
        runtime->state != ARBOR_APPLICATION_RUNTIME_STOPPING && stop_failed_seen) {
        return native_status(-EINVAL);
    }

    return ok_status();
}

static arbor_status runtime_record_for_module(
    const arbor_application_runtime *runtime,
    uint64_t module_index,
    const arbor_application_service_runtime_record **record_out,
    const arbor_application_service_module_descriptor **descriptor_out)
{
    if (runtime == NULL || record_out == NULL || descriptor_out == NULL) {
        return native_status(-EINVAL);
    }

    for (uint64_t i = 0u; i < runtime->managed_module_count; ++i) {
        if (runtime->records[i].module_index == module_index) {
            uint64_t descriptor_index = runtime->records[i].descriptor_index;
            if (descriptor_index >= runtime->managed_module_count) {
                return native_status(-EINVAL);
            }
            *record_out = &runtime->records[i];
            *descriptor_out = &runtime->service_modules[descriptor_index];
            return ok_status();
        }
    }
    return native_status(-ENOENT);
}

arbor_status arbor_application_runtime_find_ready(
    const arbor_application_runtime *runtime,
    arbor_capability_id id,
    arbor_capability_version minimum_version,
    uint32_t minimum_interface_size,
    arbor_capability_binding *binding_out)
{
    if (binding_out == NULL || capability_id_is_zero(id)) {
        return native_status(-EINVAL);
    }

    arbor_status status = arbor_application_runtime_validate(runtime);
    if (status.native != 0) {
        return status;
    }
    if (runtime->state != ARBOR_APPLICATION_RUNTIME_READY) {
        return native_status(-EAGAIN);
    }

    status = runtime_output_region_validate(
        runtime,
        (af3_region){binding_out, (uint64_t)sizeof(*binding_out)});
    if (status.native != 0) {
        return status;
    }

    arbor_capability_binding candidate = {0};
    status = arbor_capability_catalog_find(
        runtime->catalog,
        id,
        minimum_version,
        minimum_interface_size,
        &candidate);
    if (status.native != 0) {
        return status;
    }

    const arbor_application_service_runtime_record *record = NULL;
    const arbor_application_service_module_descriptor *descriptor = NULL;
    status = runtime_record_for_module(
        runtime,
        candidate.provider_module_index,
        &record,
        &descriptor);
    if (status.native != 0) {
        return status;
    }
    if (record == NULL || descriptor == NULL) {
        return native_status(-EINVAL);
    }
    if (record->state != ARBOR_APPLICATION_SERVICE_RECORD_READY) {
        return native_status(-EAGAIN);
    }

    const arbor_module_descriptor *module =
        &runtime->catalog->modules[candidate.provider_module_index];
    uint64_t export_index = 0u;
    status = module_export_index_for_capability(module, candidate.id, &export_index);
    if (status.native != 0) {
        return status;
    }
    if (!descriptor_declares_export(descriptor, export_index)) {
        return native_status(-ENOENT);
    }

    status = arbor_application_service_interface_validate(&candidate);
    if (status.native != 0) {
        return status;
    }

    (void)memory_copy(binding_out, &candidate, (uint64_t)sizeof(candidate));
    return ok_status();
}

arbor_status arbor_application_runtime_stop(arbor_application_runtime *runtime)
{
    arbor_status status = arbor_application_runtime_validate(runtime);
    if (status.native != 0) {
        return status;
    }

    if (runtime->state == ARBOR_APPLICATION_RUNTIME_STOPPING) {
        return native_status(-EBUSY);
    }
    if (runtime->state == ARBOR_APPLICATION_RUNTIME_STOPPED ||
        runtime->state == ARBOR_APPLICATION_RUNTIME_STOP_FAILED) {
        return native_status(-EALREADY);
    }

    runtime->state = ARBOR_APPLICATION_RUNTIME_STOPPING;
    arbor_status first_failure = ok_status();
    bool failed = false;

    for (uint64_t cursor = runtime->managed_module_count; cursor > 0u; --cursor) {
        arbor_application_service_runtime_record *record =
            &runtime->records[cursor - UINT64_C(1)];
        if (record->descriptor_index >= runtime->managed_module_count) {
            runtime->state = ARBOR_APPLICATION_RUNTIME_STOP_FAILED;
            return native_status(-EINVAL);
        }

        const arbor_application_service_module_descriptor *descriptor =
            &runtime->service_modules[record->descriptor_index];
        record->state = ARBOR_APPLICATION_SERVICE_RECORD_STOPPING;
        record->stop_native = 0;

        int64_t raw = 0;
        if (descriptor->stop != NULL) {
            raw = descriptor->stop(descriptor->module_context);
        }

        arbor_status normalized = arbor_application_service_status_from_native(raw);
        if (normalized.native == 0) {
            record->state = ARBOR_APPLICATION_SERVICE_RECORD_STOPPED;
            record->stop_native = 0;
        } else {
            record->state = ARBOR_APPLICATION_SERVICE_RECORD_STOP_FAILED;
            record->stop_native = raw;
            if (!failed) {
                first_failure = normalized;
                failed = true;
            }
        }
    }

    runtime->state = failed ?
        ARBOR_APPLICATION_RUNTIME_STOP_FAILED : ARBOR_APPLICATION_RUNTIME_STOPPED;

    return failed ? first_failure : ok_status();
}
