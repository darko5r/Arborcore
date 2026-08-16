#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include <arborcore/capability.h>

_Static_assert(sizeof(arbor_module_id) == 16u, "module id ABI drift");
_Static_assert(sizeof(arbor_capability_id) == 16u, "capability id ABI drift");
_Static_assert(sizeof(arbor_capability_version) == 8u, "capability version ABI drift");
_Static_assert(sizeof(arbor_capability_export) == 56u, "capability export ABI drift");
_Static_assert(sizeof(arbor_capability_requirement) == 40u, "capability requirement ABI drift");
_Static_assert(sizeof(arbor_module_descriptor) == 64u, "module descriptor ABI drift");
_Static_assert(sizeof(arbor_capability_binding) == 64u, "capability binding ABI drift");
_Static_assert(sizeof(arbor_capability_resolution) == 24u, "capability resolution ABI drift");
_Static_assert(sizeof(arbor_capability_catalog_requirements) == 24u, "catalog requirements ABI drift");
_Static_assert(sizeof(arbor_capability_catalog_storage) == 48u, "catalog storage ABI drift");
_Static_assert(sizeof(arbor_capability_workspace) == 56u, "workspace ABI drift");
_Static_assert(sizeof(arbor_capability_catalog) == 72u, "catalog ABI drift");

_Static_assert(offsetof(arbor_capability_export, interface_table) == 40u, "export interface offset drift");
_Static_assert(offsetof(arbor_capability_export, provider_context) == 48u, "export context offset drift");
_Static_assert(offsetof(arbor_module_descriptor, provides) == 32u, "module provides offset drift");
_Static_assert(offsetof(arbor_module_descriptor, consumes) == 48u, "module consumes offset drift");
_Static_assert(offsetof(arbor_capability_binding, provider_module_index) == 56u, "binding provider offset drift");
_Static_assert(offsetof(arbor_capability_catalog, module_order) == 64u, "catalog order offset drift");

static arbor_status native_status(int64_t value)
{
    return arbor_status_from_native(value);
}

static arbor_status ok_status(void)
{
    return native_status(0);
}

static bool module_id_is_zero(arbor_module_id id)
{
    return id.high == 0u && id.low == 0u;
}

static bool capability_id_is_zero(arbor_capability_id id)
{
    return id.high == 0u && id.low == 0u;
}

static bool module_id_equal(arbor_module_id left, arbor_module_id right)
{
    return left.high == right.high && left.low == right.low;
}

static bool capability_id_equal(arbor_capability_id left, arbor_capability_id right)
{
    return left.high == right.high && left.low == right.low;
}

static bool version_satisfies(
    arbor_capability_version provider,
    arbor_capability_version required)
{
    return provider.major == required.major && provider.minor >= required.minor;
}

static arbor_status checked_add_u64(uint64_t left, uint64_t right, uint64_t *out)
{
    arbor_asm_result_u64 result = u64_add_checked(left, right);
    if (result.status != 0) {
        return native_status(result.status);
    }
    *out = result.value;
    return ok_status();
}

static arbor_status module_shape_validate(const arbor_module_descriptor *module)
{
    if (module == NULL || module_id_is_zero(module->id)) {
        return native_status(-EINVAL);
    }
    if (module->abi_version != ARBOR_MODULE_DESCRIPTOR_ABI_VERSION ||
        module->struct_size != (uint32_t)sizeof(arbor_module_descriptor) ||
        module->flags != ARBOR_MODULE_FLAGS_NONE) {
        return native_status(-EINVAL);
    }
    if ((module->provides_count != 0u && module->provides == NULL) ||
        (module->consumes_count != 0u && module->consumes == NULL)) {
        return native_status(-EINVAL);
    }
    return ok_status();
}

static arbor_status export_validate(const arbor_capability_export *provided)
{
    if (provided == NULL || capability_id_is_zero(provided->id)) {
        return native_status(-EINVAL);
    }
    if (provided->version.major == 0u ||
        provided->interface_size == 0u ||
        provided->reserved0 != 0u ||
        provided->flags != ARBOR_CAPABILITY_FLAGS_NONE ||
        provided->interface_table == NULL) {
        return native_status(-EINVAL);
    }
    return ok_status();
}

static arbor_status requirement_validate(const arbor_capability_requirement *required)
{
    if (required == NULL || capability_id_is_zero(required->id)) {
        return native_status(-EINVAL);
    }
    if (required->minimum_version.major == 0u ||
        required->minimum_interface_size == 0u ||
        required->reserved0 != 0u ||
        required->flags != ARBOR_CAPABILITY_FLAGS_NONE) {
        return native_status(-EINVAL);
    }
    return ok_status();
}

static arbor_status counts_measure(
    const arbor_module_descriptor *modules,
    uint64_t module_count,
    arbor_capability_catalog_requirements *counts)
{
    if (module_count != 0u && modules == NULL) {
        return native_status(-EINVAL);
    }

    uint64_t binding_count = 0u;
    uint64_t resolution_count = 0u;

    for (uint64_t i = 0u; i < module_count; ++i) {
        arbor_status status = module_shape_validate(&modules[i]);
        if (status.native != 0) {
            return status;
        }
        status = checked_add_u64(binding_count, modules[i].provides_count, &binding_count);
        if (status.native != 0) {
            return status;
        }
        status = checked_add_u64(resolution_count, modules[i].consumes_count, &resolution_count);
        if (status.native != 0) {
            return status;
        }
    }

    counts->module_count = module_count;
    counts->binding_count = binding_count;
    counts->resolution_count = resolution_count;
    return ok_status();
}

arbor_status arbor_capability_catalog_measure(
    const arbor_module_descriptor *modules,
    uint64_t module_count,
    arbor_capability_catalog_requirements *out)
{
    if (out == NULL) {
        return native_status(-EINVAL);
    }

    arbor_capability_catalog_requirements candidate = {0u, 0u, 0u};
    arbor_status status = counts_measure(modules, module_count, &candidate);
    if (status.native != 0) {
        return status;
    }

    *out = candidate;
    return ok_status();
}

static arbor_status storage_validate(
    const arbor_capability_catalog_requirements *counts,
    const arbor_capability_catalog_storage *storage,
    const arbor_capability_workspace *workspace)
{
    if (counts == NULL || storage == NULL || workspace == NULL) {
        return native_status(-EINVAL);
    }

    if (storage->binding_capacity < counts->binding_count ||
        storage->resolution_capacity < counts->resolution_count ||
        storage->module_order_capacity < counts->module_count ||
        workspace->resolution_capacity < counts->resolution_count ||
        workspace->module_capacity < counts->module_count) {
        return native_status(-ENOSPC);
    }

    if ((counts->binding_count != 0u && storage->bindings == NULL) ||
        (counts->resolution_count != 0u && storage->resolutions == NULL) ||
        (counts->module_count != 0u && storage->module_order == NULL) ||
        (counts->resolution_count != 0u &&
         (workspace->resolved_provider_module_indices == NULL ||
          workspace->resolved_binding_indices == NULL)) ||
        (counts->module_count != 0u &&
         (workspace->indegree == NULL ||
          workspace->selected == NULL ||
          workspace->module_order == NULL))) {
        return native_status(-EINVAL);
    }

    return ok_status();
}

typedef struct arbor_memory_region {
    const void *pointer;
    uint64_t byte_length;
} arbor_memory_region;

static arbor_status region_make(
    const void *pointer,
    uint64_t count,
    uint64_t element_size,
    arbor_memory_region *out)
{
    if (out == NULL) {
        return native_status(-EINVAL);
    }
    if (count == 0u) {
        *out = (arbor_memory_region){NULL, 0u};
        return ok_status();
    }
    if (pointer == NULL) {
        return native_status(-EINVAL);
    }

    arbor_asm_result_u64 bytes = u64_mul_checked(count, element_size);
    if (bytes.status != 0) {
        return native_status(bytes.status);
    }
    arbor_asm_result_u64 end = range_end_checked(
        (uint64_t)(uintptr_t)pointer,
        bytes.value);
    if (end.status != 0) {
        return native_status(end.status);
    }
    (void)end;

    *out = (arbor_memory_region){pointer, bytes.value};
    return ok_status();
}

static arbor_status regions_overlap(
    arbor_memory_region left,
    arbor_memory_region right,
    bool *overlap_out)
{
    if (overlap_out == NULL) {
        return native_status(-EINVAL);
    }
    if (left.byte_length == 0u || right.byte_length == 0u) {
        *overlap_out = false;
        return ok_status();
    }

    arbor_asm_result_u64 result = range_overlaps(
        (uint64_t)(uintptr_t)left.pointer,
        left.byte_length,
        (uint64_t)(uintptr_t)right.pointer,
        right.byte_length);
    if (result.status != 0) {
        return native_status(result.status);
    }
    *overlap_out = result.value != 0u;
    return ok_status();
}

static arbor_status reject_overlap(
    arbor_memory_region left,
    arbor_memory_region right)
{
    bool overlap = false;
    arbor_status status = regions_overlap(left, right, &overlap);
    if (status.native != 0) {
        return status;
    }
    return overlap ? native_status(-EINVAL) : ok_status();
}

static arbor_status publication_regions_validate(
    const arbor_module_descriptor *modules,
    uint64_t module_count,
    const arbor_capability_catalog_requirements *counts,
    const arbor_capability_catalog_storage *storage,
    const arbor_capability_workspace *workspace,
    const arbor_capability_catalog *out)
{
    arbor_memory_region writes[9] = {0};
    arbor_status status = region_make(
        storage->bindings, counts->binding_count,
        (uint64_t)sizeof(arbor_capability_binding), &writes[0]);
    if (status.native != 0) return status;
    status = region_make(
        storage->resolutions, counts->resolution_count,
        (uint64_t)sizeof(arbor_capability_resolution), &writes[1]);
    if (status.native != 0) return status;
    status = region_make(
        storage->module_order, counts->module_count,
        (uint64_t)sizeof(uint64_t), &writes[2]);
    if (status.native != 0) return status;
    status = region_make(
        workspace->resolved_provider_module_indices, counts->resolution_count,
        (uint64_t)sizeof(uint64_t), &writes[3]);
    if (status.native != 0) return status;
    status = region_make(
        workspace->resolved_binding_indices, counts->resolution_count,
        (uint64_t)sizeof(uint64_t), &writes[4]);
    if (status.native != 0) return status;
    status = region_make(
        workspace->indegree, counts->module_count,
        (uint64_t)sizeof(uint64_t), &writes[5]);
    if (status.native != 0) return status;
    status = region_make(
        workspace->selected, counts->module_count,
        (uint64_t)sizeof(uint8_t), &writes[6]);
    if (status.native != 0) return status;
    status = region_make(
        workspace->module_order, counts->module_count,
        (uint64_t)sizeof(uint64_t), &writes[7]);
    if (status.native != 0) return status;
    status = region_make(out, 1u, (uint64_t)sizeof(arbor_capability_catalog), &writes[8]);
    if (status.native != 0) return status;

    for (size_t i = 0u; i < sizeof(writes) / sizeof(writes[0]); ++i) {
        for (size_t j = 0u; j < i; ++j) {
            status = reject_overlap(writes[i], writes[j]);
            if (status.native != 0) return status;
        }
    }

    arbor_memory_region controls[3] = {0};
    status = region_make(storage, 1u, (uint64_t)sizeof(*storage), &controls[0]);
    if (status.native != 0) return status;
    status = region_make(workspace, 1u, (uint64_t)sizeof(*workspace), &controls[1]);
    if (status.native != 0) return status;
    status = region_make(
        modules, module_count, (uint64_t)sizeof(arbor_module_descriptor), &controls[2]);
    if (status.native != 0) return status;

    for (size_t write_index = 0u;
         write_index < sizeof(writes) / sizeof(writes[0]);
         ++write_index) {
        for (size_t control_index = 0u;
             control_index < sizeof(controls) / sizeof(controls[0]);
             ++control_index) {
            status = reject_overlap(writes[write_index], controls[control_index]);
            if (status.native != 0) return status;
        }
    }

    for (uint64_t module_index = 0u; module_index < module_count; ++module_index) {
        arbor_memory_region provides = {0};
        arbor_memory_region consumes = {0};
        status = region_make(
            modules[module_index].provides,
            modules[module_index].provides_count,
            (uint64_t)sizeof(arbor_capability_export),
            &provides);
        if (status.native != 0) return status;
        status = region_make(
            modules[module_index].consumes,
            modules[module_index].consumes_count,
            (uint64_t)sizeof(arbor_capability_requirement),
            &consumes);
        if (status.native != 0) return status;

        for (size_t write_index = 0u;
             write_index < sizeof(writes) / sizeof(writes[0]);
             ++write_index) {
            status = reject_overlap(writes[write_index], provides);
            if (status.native != 0) return status;
            status = reject_overlap(writes[write_index], consumes);
            if (status.native != 0) return status;
        }

        for (uint64_t export_index = 0u;
             export_index < modules[module_index].provides_count;
             ++export_index) {
            arbor_memory_region interface_region = {0};
            status = region_make(
                modules[module_index].provides[export_index].interface_table,
                modules[module_index].provides[export_index].interface_size,
                1u,
                &interface_region);
            if (status.native != 0) return status;
            for (size_t write_index = 0u;
                 write_index < sizeof(writes) / sizeof(writes[0]);
                 ++write_index) {
                status = reject_overlap(writes[write_index], interface_region);
                if (status.native != 0) return status;
            }
        }
    }

    return ok_status();
}

static arbor_status descriptor_set_validate(
    const arbor_module_descriptor *modules,
    uint64_t module_count)
{
    for (uint64_t i = 0u; i < module_count; ++i) {
        arbor_status status = module_shape_validate(&modules[i]);
        if (status.native != 0) {
            return status;
        }

        for (uint64_t j = 0u; j < i; ++j) {
            if (module_id_equal(modules[i].id, modules[j].id)) {
                return native_status(-EEXIST);
            }
        }

        for (uint64_t p = 0u; p < modules[i].provides_count; ++p) {
            status = export_validate(&modules[i].provides[p]);
            if (status.native != 0) {
                return status;
            }
        }

        for (uint64_t r = 0u; r < modules[i].consumes_count; ++r) {
            status = requirement_validate(&modules[i].consumes[r]);
            if (status.native != 0) {
                return status;
            }
            for (uint64_t previous = 0u; previous < r; ++previous) {
                if (capability_id_equal(
                        modules[i].consumes[r].id,
                        modules[i].consumes[previous].id)) {
                    return native_status(-EEXIST);
                }
            }
        }
    }

    for (uint64_t mi = 0u; mi < module_count; ++mi) {
        for (uint64_t pi = 0u; pi < modules[mi].provides_count; ++pi) {
            for (uint64_t mj = 0u; mj <= mi; ++mj) {
                uint64_t limit = modules[mj].provides_count;
                if (mj == mi) {
                    limit = pi;
                }
                for (uint64_t pj = 0u; pj < limit; ++pj) {
                    if (capability_id_equal(
                            modules[mi].provides[pi].id,
                            modules[mj].provides[pj].id)) {
                        return native_status(-EEXIST);
                    }
                }
            }
        }
    }

    return ok_status();
}

static arbor_status binding_index_for_export(
    const arbor_module_descriptor *modules,
    uint64_t provider_module_index,
    uint64_t provider_export_index,
    uint64_t *binding_index_out)
{
    uint64_t index = 0u;
    for (uint64_t i = 0u; i < provider_module_index; ++i) {
        arbor_status status = checked_add_u64(index, modules[i].provides_count, &index);
        if (status.native != 0) {
            return status;
        }
    }
    return checked_add_u64(index, provider_export_index, binding_index_out);
}

static arbor_status requirements_resolve(
    const arbor_module_descriptor *modules,
    uint64_t module_count,
    arbor_capability_workspace *workspace)
{
    uint64_t resolution_index = 0u;

    for (uint64_t consumer = 0u; consumer < module_count; ++consumer) {
        workspace->indegree[consumer] = modules[consumer].consumes_count;
        workspace->selected[consumer] = 0u;

        for (uint64_t requirement_index = 0u;
             requirement_index < modules[consumer].consumes_count;
             ++requirement_index) {
            const arbor_capability_requirement *required =
                &modules[consumer].consumes[requirement_index];
            bool found_id = false;
            bool found_compatible = false;
            uint64_t provider_module = 0u;
            uint64_t provider_export = 0u;

            for (uint64_t candidate_module = 0u;
                 candidate_module < module_count;
                 ++candidate_module) {
                for (uint64_t candidate_export = 0u;
                     candidate_export < modules[candidate_module].provides_count;
                     ++candidate_export) {
                    const arbor_capability_export *provided =
                        &modules[candidate_module].provides[candidate_export];
                    if (!capability_id_equal(provided->id, required->id)) {
                        continue;
                    }

                    found_id = true;
                    if (version_satisfies(provided->version, required->minimum_version) &&
                        provided->interface_size >= required->minimum_interface_size) {
                        found_compatible = true;
                        provider_module = candidate_module;
                        provider_export = candidate_export;
                    }
                }
            }

            if (!found_id) {
                return native_status(-ENOENT);
            }
            if (!found_compatible) {
                return native_status(-EPROTONOSUPPORT);
            }

            uint64_t binding_index = 0u;
            arbor_status status = binding_index_for_export(
                modules,
                provider_module,
                provider_export,
                &binding_index);
            if (status.native != 0) {
                return status;
            }

            workspace->resolved_provider_module_indices[resolution_index] = provider_module;
            workspace->resolved_binding_indices[resolution_index] = binding_index;
            resolution_index += UINT64_C(1);
        }
    }

    return ok_status();
}

static arbor_status topological_order_build(
    const arbor_module_descriptor *modules,
    uint64_t module_count,
    arbor_capability_workspace *workspace)
{
    for (uint64_t output_index = 0u; output_index < module_count; ++output_index) {
        bool found = false;
        uint64_t selected_module = 0u;

        for (uint64_t candidate = 0u; candidate < module_count; ++candidate) {
            if (workspace->selected[candidate] == 0u && workspace->indegree[candidate] == 0u) {
                found = true;
                selected_module = candidate;
                break;
            }
        }

        if (!found) {
            return native_status(-ELOOP);
        }

        workspace->selected[selected_module] = 1u;
        workspace->module_order[output_index] = selected_module;

        uint64_t resolution_index = 0u;
        for (uint64_t consumer = 0u; consumer < module_count; ++consumer) {
            for (uint64_t requirement_index = 0u;
                 requirement_index < modules[consumer].consumes_count;
                 ++requirement_index) {
                (void)requirement_index;
                if (workspace->resolved_provider_module_indices[resolution_index] == selected_module) {
                    if (workspace->indegree[consumer] == 0u) {
                        return native_status(-EINVAL);
                    }
                    workspace->indegree[consumer] -= UINT64_C(1);
                }
                resolution_index += UINT64_C(1);
            }
        }
    }

    return ok_status();
}

static void publish_catalog(
    const arbor_module_descriptor *modules,
    const arbor_capability_catalog_requirements *counts,
    arbor_capability_catalog_storage *storage,
    const arbor_capability_workspace *workspace,
    arbor_capability_catalog *out)
{
    uint64_t binding_index = 0u;
    for (uint64_t module_index = 0u; module_index < counts->module_count; ++module_index) {
        for (uint64_t export_index = 0u;
             export_index < modules[module_index].provides_count;
             ++export_index) {
            const arbor_capability_export *provided = &modules[module_index].provides[export_index];
            storage->bindings[binding_index] = (arbor_capability_binding){
                provided->id,
                provided->version,
                provided->interface_size,
                0u,
                provided->flags,
                provided->interface_table,
                provided->provider_context,
                module_index
            };
            binding_index += UINT64_C(1);
        }
    }

    uint64_t resolution_index = 0u;
    for (uint64_t module_index = 0u; module_index < counts->module_count; ++module_index) {
        for (uint64_t requirement_index = 0u;
             requirement_index < modules[module_index].consumes_count;
             ++requirement_index) {
            storage->resolutions[resolution_index] = (arbor_capability_resolution){
                module_index,
                requirement_index,
                workspace->resolved_binding_indices[resolution_index]
            };
            resolution_index += UINT64_C(1);
        }
    }

    for (uint64_t i = 0u; i < counts->module_count; ++i) {
        storage->module_order[i] = workspace->module_order[i];
    }

    arbor_capability_catalog candidate = {
        ARBOR_CAPABILITY_KERNEL_ABI_VERSION,
        (uint32_t)sizeof(arbor_capability_catalog),
        ARBOR_CAPABILITY_CATALOG_FLAGS_NONE,
        modules,
        counts->module_count,
        storage->bindings,
        counts->binding_count,
        storage->resolutions,
        counts->resolution_count,
        storage->module_order
    };
    *out = candidate;
}

arbor_status arbor_capability_catalog_prepare(
    const arbor_module_descriptor *modules,
    uint64_t module_count,
    arbor_capability_catalog_storage *storage,
    arbor_capability_workspace *workspace,
    arbor_capability_catalog *out)
{
    if (out == NULL) {
        return native_status(-EINVAL);
    }

    arbor_capability_catalog_requirements counts = {0u, 0u, 0u};
    arbor_status status = counts_measure(modules, module_count, &counts);
    if (status.native != 0) {
        return status;
    }
    status = storage_validate(&counts, storage, workspace);
    if (status.native != 0) {
        return status;
    }
    status = descriptor_set_validate(modules, module_count);
    if (status.native != 0) {
        return status;
    }
    status = publication_regions_validate(modules, module_count, &counts, storage, workspace, out);
    if (status.native != 0) {
        return status;
    }
    status = requirements_resolve(modules, module_count, workspace);
    if (status.native != 0) {
        return status;
    }
    status = topological_order_build(modules, module_count, workspace);
    if (status.native != 0) {
        return status;
    }

    publish_catalog(modules, &counts, storage, workspace, out);
    return ok_status();
}

static arbor_status catalog_shape_validate(const arbor_capability_catalog *catalog)
{
    if (catalog == NULL ||
        catalog->abi_version != ARBOR_CAPABILITY_KERNEL_ABI_VERSION ||
        catalog->struct_size < (uint32_t)sizeof(arbor_capability_catalog) ||
        catalog->flags != ARBOR_CAPABILITY_CATALOG_FLAGS_NONE) {
        return native_status(-EINVAL);
    }
    if ((catalog->module_count != 0u && (catalog->modules == NULL || catalog->module_order == NULL)) ||
        (catalog->binding_count != 0u && catalog->bindings == NULL) ||
        (catalog->resolution_count != 0u && catalog->resolutions == NULL)) {
        return native_status(-EINVAL);
    }
    return ok_status();
}

static arbor_status module_position(
    const arbor_capability_catalog *catalog,
    uint64_t module_index,
    uint64_t *position_out)
{
    for (uint64_t i = 0u; i < catalog->module_count; ++i) {
        if (catalog->module_order[i] == module_index) {
            *position_out = i;
            return ok_status();
        }
    }
    return native_status(-EINVAL);
}

static bool module_is_in_order_prefix(
    const arbor_capability_catalog *catalog,
    uint64_t module_index,
    uint64_t prefix_length)
{
    for (uint64_t i = 0u; i < prefix_length; ++i) {
        if (catalog->module_order[i] == module_index) {
            return true;
        }
    }
    return false;
}

static arbor_status module_ready_at_position(
    const arbor_capability_catalog *catalog,
    uint64_t module_index,
    uint64_t position,
    bool *ready_out)
{
    if (ready_out == NULL || module_index >= catalog->module_count ||
        position > catalog->module_count) {
        return native_status(-EINVAL);
    }

    const arbor_module_descriptor *module = &catalog->modules[module_index];
    for (uint64_t requirement_index = 0u;
         requirement_index < module->consumes_count;
         ++requirement_index) {
        bool found = false;
        uint64_t binding_index = 0u;

        for (uint64_t resolution_index = 0u;
             resolution_index < catalog->resolution_count;
             ++resolution_index) {
            const arbor_capability_resolution *resolution =
                &catalog->resolutions[resolution_index];
            if (resolution->consumer_module_index == module_index &&
                resolution->requirement_index == requirement_index) {
                if (resolution->binding_index >= catalog->binding_count) {
                    return native_status(-EINVAL);
                }
                found = true;
                binding_index = resolution->binding_index;
                break;
            }
        }

        if (!found) {
            return native_status(-EINVAL);
        }

        uint64_t provider_module_index =
            catalog->bindings[binding_index].provider_module_index;
        if (provider_module_index >= catalog->module_count) {
            return native_status(-EINVAL);
        }
        if (!module_is_in_order_prefix(catalog, provider_module_index, position)) {
            *ready_out = false;
            return ok_status();
        }
    }

    *ready_out = true;
    return ok_status();
}

static arbor_status canonical_module_order_validate(
    const arbor_capability_catalog *catalog)
{
    for (uint64_t position = 0u; position < catalog->module_count; ++position) {
        uint64_t expected = UINT64_MAX;

        for (uint64_t candidate = 0u; candidate < catalog->module_count; ++candidate) {
            if (module_is_in_order_prefix(catalog, candidate, position)) {
                continue;
            }

            bool ready = false;
            arbor_status status =
                module_ready_at_position(catalog, candidate, position, &ready);
            if (status.native != 0) {
                return status;
            }
            if (ready) {
                expected = candidate;
                break;
            }
        }

        if (expected == UINT64_MAX || catalog->module_order[position] != expected) {
            return native_status(-EINVAL);
        }
    }

    return ok_status();
}

arbor_status arbor_capability_catalog_validate(const arbor_capability_catalog *catalog)
{
    arbor_status status = catalog_shape_validate(catalog);
    if (status.native != 0) {
        return status;
    }

    arbor_capability_catalog_requirements counts = {0u, 0u, 0u};
    status = counts_measure(catalog->modules, catalog->module_count, &counts);
    if (status.native != 0) {
        return status;
    }
    status = descriptor_set_validate(catalog->modules, catalog->module_count);
    if (status.native != 0) {
        return status;
    }
    if (counts.binding_count != catalog->binding_count ||
        counts.resolution_count != catalog->resolution_count) {
        return native_status(-EINVAL);
    }

    for (uint64_t i = 0u; i < catalog->module_count; ++i) {
        if (catalog->module_order[i] >= catalog->module_count) {
            return native_status(-EINVAL);
        }
        for (uint64_t j = 0u; j < i; ++j) {
            if (catalog->module_order[i] == catalog->module_order[j]) {
                return native_status(-EINVAL);
            }
        }
    }

    uint64_t binding_index = 0u;
    for (uint64_t module_index = 0u; module_index < catalog->module_count; ++module_index) {
        for (uint64_t export_index = 0u;
             export_index < catalog->modules[module_index].provides_count;
             ++export_index) {
            const arbor_capability_export *provided =
                &catalog->modules[module_index].provides[export_index];
            const arbor_capability_binding *binding = &catalog->bindings[binding_index];

            status = export_validate(provided);
            if (status.native != 0 ||
                !capability_id_equal(binding->id, provided->id) ||
                binding->version.major != provided->version.major ||
                binding->version.minor != provided->version.minor ||
                binding->interface_size != provided->interface_size ||
                binding->reserved0 != 0u ||
                binding->flags != provided->flags ||
                binding->interface_table != provided->interface_table ||
                binding->provider_context != provided->provider_context ||
                binding->provider_module_index != module_index) {
                return native_status(-EINVAL);
            }
            binding_index += UINT64_C(1);
        }
    }

    uint64_t resolution_index = 0u;
    for (uint64_t consumer = 0u; consumer < catalog->module_count; ++consumer) {
        for (uint64_t requirement_index = 0u;
             requirement_index < catalog->modules[consumer].consumes_count;
             ++requirement_index) {
            const arbor_capability_requirement *required =
                &catalog->modules[consumer].consumes[requirement_index];
            const arbor_capability_resolution *resolution =
                &catalog->resolutions[resolution_index];

            status = requirement_validate(required);
            if (status.native != 0 ||
                resolution->consumer_module_index != consumer ||
                resolution->requirement_index != requirement_index ||
                resolution->binding_index >= catalog->binding_count) {
                return native_status(-EINVAL);
            }

            const arbor_capability_binding *binding =
                &catalog->bindings[resolution->binding_index];
            if (!capability_id_equal(binding->id, required->id) ||
                !version_satisfies(binding->version, required->minimum_version) ||
                binding->interface_size < required->minimum_interface_size) {
                return native_status(-EINVAL);
            }

            uint64_t provider_position = 0u;
            uint64_t consumer_position = 0u;
            status = module_position(catalog, binding->provider_module_index, &provider_position);
            if (status.native != 0) {
                return status;
            }
            status = module_position(catalog, consumer, &consumer_position);
            if (status.native != 0 || provider_position >= consumer_position) {
                return native_status(-EINVAL);
            }

            resolution_index += UINT64_C(1);
        }
    }

    status = canonical_module_order_validate(catalog);
    if (status.native != 0) {
        return status;
    }

    return ok_status();
}

arbor_status arbor_capability_catalog_find_module(
    const arbor_capability_catalog *catalog,
    arbor_module_id id,
    uint64_t *module_index_out)
{
    if (module_index_out == NULL || module_id_is_zero(id)) {
        return native_status(-EINVAL);
    }

    arbor_status status = arbor_capability_catalog_validate(catalog);
    if (status.native != 0) {
        return status;
    }

    for (uint64_t i = 0u; i < catalog->module_count; ++i) {
        if (module_id_equal(catalog->modules[i].id, id)) {
            *module_index_out = i;
            return ok_status();
        }
    }

    return native_status(-ENOENT);
}

arbor_status arbor_capability_catalog_find(
    const arbor_capability_catalog *catalog,
    arbor_capability_id id,
    arbor_capability_version minimum_version,
    uint32_t minimum_interface_size,
    arbor_capability_binding *binding_out)
{
    if (binding_out == NULL || capability_id_is_zero(id) ||
        minimum_version.major == 0u || minimum_interface_size == 0u) {
        return native_status(-EINVAL);
    }

    arbor_status status = arbor_capability_catalog_validate(catalog);
    if (status.native != 0) {
        return status;
    }

    for (uint64_t i = 0u; i < catalog->binding_count; ++i) {
        const arbor_capability_binding *binding = &catalog->bindings[i];
        if (!capability_id_equal(binding->id, id)) {
            continue;
        }
        if (!version_satisfies(binding->version, minimum_version) ||
            binding->interface_size < minimum_interface_size) {
            return native_status(-EPROTONOSUPPORT);
        }
        *binding_out = *binding;
        return ok_status();
    }

    return native_status(-ENOENT);
}

arbor_status arbor_capability_catalog_resolve(
    const arbor_capability_catalog *catalog,
    uint64_t consumer_module_index,
    uint64_t requirement_index,
    arbor_capability_binding *binding_out)
{
    if (binding_out == NULL) {
        return native_status(-EINVAL);
    }

    arbor_status status = arbor_capability_catalog_validate(catalog);
    if (status.native != 0) {
        return status;
    }
    if (consumer_module_index >= catalog->module_count ||
        requirement_index >= catalog->modules[consumer_module_index].consumes_count) {
        return native_status(-EINVAL);
    }

    for (uint64_t i = 0u; i < catalog->resolution_count; ++i) {
        const arbor_capability_resolution *resolution = &catalog->resolutions[i];
        if (resolution->consumer_module_index == consumer_module_index &&
            resolution->requirement_index == requirement_index) {
            if (resolution->binding_index >= catalog->binding_count) {
                return native_status(-EINVAL);
            }
            *binding_out = catalog->bindings[resolution->binding_index];
            return ok_status();
        }
    }

    return native_status(-ENOENT);
}
