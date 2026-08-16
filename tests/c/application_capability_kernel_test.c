#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <arborcore/application.h>
#include <arborcore/capability.h>

static int fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    return 1;
}

static bool binding_equal(arbor_capability_binding left, arbor_capability_binding right)
{
    return left.id.high == right.id.high &&
           left.id.low == right.id.low &&
           left.version.major == right.version.major &&
           left.version.minor == right.version.minor &&
           left.interface_size == right.interface_size &&
           left.reserved0 == right.reserved0 &&
           left.flags == right.flags &&
           left.interface_table == right.interface_table &&
           left.provider_context == right.provider_context &&
           left.provider_module_index == right.provider_module_index;
}

static bool catalog_equal(arbor_capability_catalog left, arbor_capability_catalog right)
{
    return left.abi_version == right.abi_version &&
           left.struct_size == right.struct_size &&
           left.flags == right.flags &&
           left.modules == right.modules &&
           left.module_count == right.module_count &&
           left.bindings == right.bindings &&
           left.binding_count == right.binding_count &&
           left.resolutions == right.resolutions &&
           left.resolution_count == right.resolution_count &&
           left.module_order == right.module_order;
}

typedef struct test_checked_math_v1 {
    uint32_t abi_version;
    uint32_t struct_size;
    arbor_asm_result_u64 (*add_checked)(uint64_t left, uint64_t right);
} test_checked_math_v1;

_Static_assert(sizeof(test_checked_math_v1) == 16u, "test checked-math table drift");

static int64_t test_application_dispatch(
    const arbor_request_scope *scope,
    void *application_context,
    arbor_response_plan *response_out)
{
    uint64_t *call_count = (uint64_t *)application_context;
    static const uint8_t body[] = {'a', 'f', '2'};

    if (scope == NULL || call_count == NULL || response_out == NULL) {
        return -EINVAL;
    }

    *call_count += UINT64_C(1);
    arbor_status status = arbor_response_plan_make(
        UINT64_C(200),
        (arbor_span){body, (uint64_t)sizeof(body)},
        ARBOR_RESPONSE_PLAN_FLAG_NONE,
        response_out);
    return status.native;
}

static const arbor_module_id module_runtime = {UINT64_C(0x100), UINT64_C(0x01)};
static const arbor_module_id module_application = {UINT64_C(0x100), UINT64_C(0x02)};
static const arbor_module_id module_lower_math = {UINT64_C(0x100), UINT64_C(0x03)};
static const arbor_module_id module_other = {UINT64_C(0x100), UINT64_C(0x04)};

static const arbor_capability_id cap_checked_math = {UINT64_C(0x200), UINT64_C(0x01)};
static const arbor_capability_id cap_application_dispatch = {UINT64_C(0x200), UINT64_C(0x02)};
static const arbor_capability_id cap_missing = {UINT64_C(0x200), UINT64_C(0x03)};
static const arbor_capability_id cap_other = {UINT64_C(0x200), UINT64_C(0x04)};

static arbor_module_descriptor make_module(
    arbor_module_id id,
    const arbor_capability_export *provides,
    uint64_t provides_count,
    const arbor_capability_requirement *consumes,
    uint64_t consumes_count)
{
    return (arbor_module_descriptor){
        id,
        ARBOR_MODULE_DESCRIPTOR_ABI_VERSION,
        (uint32_t)sizeof(arbor_module_descriptor),
        ARBOR_MODULE_FLAGS_NONE,
        provides,
        provides_count,
        consumes,
        consumes_count
    };
}

static arbor_capability_export make_export(
    arbor_capability_id id,
    arbor_capability_version version,
    const void *interface_table,
    uint32_t interface_size,
    void *provider_context)
{
    return (arbor_capability_export){
        id,
        version,
        interface_size,
        0u,
        ARBOR_CAPABILITY_FLAGS_NONE,
        interface_table,
        provider_context
    };
}

static arbor_capability_requirement make_requirement(
    arbor_capability_id id,
    arbor_capability_version minimum_version,
    uint32_t minimum_interface_size)
{
    return (arbor_capability_requirement){
        id,
        minimum_version,
        minimum_interface_size,
        0u,
        ARBOR_CAPABILITY_FLAGS_NONE
    };
}

int main(void)
{
    if (sizeof(arbor_module_id) != 16u ||
        sizeof(arbor_capability_id) != 16u ||
        sizeof(arbor_capability_export) != 56u ||
        sizeof(arbor_capability_requirement) != 40u ||
        sizeof(arbor_module_descriptor) != 64u ||
        sizeof(arbor_capability_binding) != 64u ||
        sizeof(arbor_capability_resolution) != 24u ||
        sizeof(arbor_capability_catalog_storage) != 48u ||
        sizeof(arbor_capability_workspace) != 56u ||
        sizeof(arbor_capability_catalog) != 72u) {
        return fail("AF2 stable capability/module layouts");
    }

    uint64_t application_calls = 0u;
    arbor_application_capabilities application_interface = {0};
    arbor_status status = arbor_application_capabilities_make(
        test_application_dispatch,
        &application_calls,
        &application_interface);
    if (status.native != 0) {
        return fail("AF1 application capability construction for AF2 export");
    }

    const test_checked_math_v1 checked_math_interface = {
        1u,
        (uint32_t)sizeof(test_checked_math_v1),
        u64_add_checked
    };

    arbor_capability_export lower_exports[1] = {
        make_export(
            cap_checked_math,
            (arbor_capability_version){1u, 2u},
            &checked_math_interface,
            (uint32_t)sizeof(checked_math_interface),
            NULL)
    };
    arbor_capability_requirement application_requires[1] = {
        make_requirement(
            cap_checked_math,
            (arbor_capability_version){1u, 1u},
            (uint32_t)sizeof(test_checked_math_v1))
    };
    arbor_capability_export application_exports[1] = {
        make_export(
            cap_application_dispatch,
            (arbor_capability_version){1u, 0u},
            &application_interface,
            (uint32_t)sizeof(application_interface),
            NULL)
    };
    arbor_capability_requirement runtime_requires[1] = {
        make_requirement(
            cap_application_dispatch,
            (arbor_capability_version){1u, 0u},
            (uint32_t)sizeof(arbor_application_capabilities))
    };

    /* Deliberately reverse dependency order: runtime -> application -> lower. */
    arbor_module_descriptor modules[3] = {
        make_module(module_runtime, NULL, 0u, runtime_requires, 1u),
        make_module(module_application, application_exports, 1u, application_requires, 1u),
        make_module(module_lower_math, lower_exports, 1u, NULL, 0u)
    };

    arbor_capability_catalog_requirements measured = {UINT64_C(99), UINT64_C(99), UINT64_C(99)};
    status = arbor_capability_catalog_measure(modules, 3u, &measured);
    if (status.native != 0 || measured.module_count != 3u ||
        measured.binding_count != 2u || measured.resolution_count != 2u) {
        return fail("AF2 deterministic catalog measurement");
    }

    arbor_module_descriptor oversized_descriptor = modules[0];
    oversized_descriptor.struct_size =
        (uint32_t)sizeof(arbor_module_descriptor) + UINT32_C(8);
    arbor_capability_catalog_requirements unchanged_descriptor_measure =
        {UINT64_C(7), UINT64_C(8), UINT64_C(9)};
    arbor_capability_catalog_requirements descriptor_measure =
        unchanged_descriptor_measure;
    status = arbor_capability_catalog_measure(
        &oversized_descriptor, 1u, &descriptor_measure);
    if (status.native != -EINVAL ||
        descriptor_measure.module_count != unchanged_descriptor_measure.module_count ||
        descriptor_measure.binding_count != unchanged_descriptor_measure.binding_count ||
        descriptor_measure.resolution_count != unchanged_descriptor_measure.resolution_count) {
        return fail("AF2 v1 module descriptor size is exact and transactional");
    }

    arbor_capability_binding bindings[2] = {0};
    arbor_capability_resolution resolutions[2] = {0};
    uint64_t published_order[3] = {UINT64_MAX, UINT64_MAX, UINT64_MAX};
    arbor_capability_catalog_storage storage = {
        bindings, 2u, resolutions, 2u, published_order, 3u
    };

    uint64_t workspace_provider_modules[2] = {0u, 0u};
    uint64_t workspace_binding_indices[2] = {0u, 0u};
    uint64_t workspace_indegree[3] = {0u, 0u, 0u};
    uint8_t workspace_selected[3] = {0u, 0u, 0u};
    uint64_t workspace_order[3] = {0u, 0u, 0u};
    arbor_capability_workspace workspace = {
        workspace_provider_modules,
        workspace_binding_indices,
        2u,
        workspace_indegree,
        workspace_selected,
        workspace_order,
        3u
    };

    arbor_capability_catalog catalog = {0};
    status = arbor_capability_catalog_prepare(modules, 3u, &storage, &workspace, &catalog);
    if (status.native != 0 || arbor_capability_catalog_validate(&catalog).native != 0) {
        return fail("AF2 valid immutable composition publication");
    }
    if (catalog.module_count != 3u || catalog.binding_count != 2u ||
        catalog.resolution_count != 2u ||
        published_order[0] != 2u || published_order[1] != 1u || published_order[2] != 0u) {
        return fail("AF2 deterministic provider-before-consumer module order");
    }

    uint64_t module_index = UINT64_MAX;
    status = arbor_capability_catalog_find_module(&catalog, module_application, &module_index);
    if (status.native != 0 || module_index != 1u) {
        return fail("AF2 module identity lookup");
    }
    uint64_t unchanged_module_index = UINT64_C(77);
    module_index = unchanged_module_index;
    status = arbor_capability_catalog_find_module(&catalog, module_other, &module_index);
    if (status.native != -ENOENT || module_index != unchanged_module_index) {
        return fail("AF2 missing module lookup is transactional");
    }

    arbor_capability_binding binding = {
        cap_other, {9u, 9u}, 9u, 0u, 0u, &catalog, &catalog, 9u
    };
    arbor_capability_binding unchanged_binding = binding;
    status = arbor_capability_catalog_find(
        &catalog,
        cap_checked_math,
        (arbor_capability_version){1u, 1u},
        (uint32_t)sizeof(test_checked_math_v1),
        &binding);
    if (status.native != 0 || binding.provider_module_index != 2u ||
        binding.interface_table != &checked_math_interface || binding.version.minor != 2u) {
        return fail("AF2 compatible versioned capability lookup");
    }

    const test_checked_math_v1 *resolved_math =
        (const test_checked_math_v1 *)binding.interface_table;
    arbor_asm_result_u64 sum = resolved_math->add_checked(UINT64_C(40), UINT64_C(2));
    if (sum.status != 0 || sum.value != UINT64_C(42)) {
        return fail("AF2 upper layer consumes qualified lower checked arithmetic capability");
    }

    binding = unchanged_binding;
    status = arbor_capability_catalog_find(
        &catalog,
        cap_checked_math,
        (arbor_capability_version){1u, 3u},
        (uint32_t)sizeof(test_checked_math_v1),
        &binding);
    if (status.native != -EPROTONOSUPPORT || !binding_equal(binding, unchanged_binding)) {
        return fail("AF2 incompatible minor version rejection is transactional");
    }

    binding = unchanged_binding;
    status = arbor_capability_catalog_find(
        &catalog,
        cap_missing,
        (arbor_capability_version){1u, 0u},
        1u,
        &binding);
    if (status.native != -ENOENT || !binding_equal(binding, unchanged_binding)) {
        return fail("AF2 missing capability lookup is transactional");
    }

    status = arbor_capability_catalog_resolve(&catalog, 1u, 0u, &binding);
    if (status.native != 0 || binding.provider_module_index != 2u) {
        return fail("AF2 application module resolves lower capability");
    }
    status = arbor_capability_catalog_resolve(&catalog, 0u, 0u, &binding);
    if (status.native != 0 || binding.provider_module_index != 1u ||
        binding.interface_table != &application_interface) {
        return fail("AF2 lower runtime module resolves higher application capability");
    }

    const arbor_application_capabilities *resolved_application =
        (const arbor_application_capabilities *)binding.interface_table;
    arbor_asm_http_request native_request = {0};
    arbor_asm_request_target target = {0};
    uint8_t arena_bytes[64] = {0};
    arbor_asm_arena arena = {arena_bytes, (uint64_t)sizeof(arena_bytes), 0u};
    arbor_request_scope request_scope = {&native_request, &target, NULL, 0u, &arena};
    arbor_response_plan response = {0};
    status = arbor_application_invoke(resolved_application, &request_scope, &response);
    if (status.native != 0 || application_calls != 1u || response.status != UINT64_C(200) ||
        response.body_length != 3u) {
        return fail("AF2 lower framework can invoke registered higher AF1 capability");
    }

    /* Duplicate module identity. */
    arbor_module_descriptor duplicate_modules[2] = {modules[1], modules[1]};
    arbor_capability_catalog sentinel_catalog = {
        9u, 9u, 9u, modules, 9u, bindings, 9u, resolutions, 9u, published_order
    };
    arbor_capability_catalog unchanged_catalog = sentinel_catalog;
    status = arbor_capability_catalog_prepare(
        duplicate_modules, 2u, &storage, &workspace, &sentinel_catalog);
    if (status.native != -EEXIST || !catalog_equal(sentinel_catalog, unchanged_catalog)) {
        return fail("AF2 duplicate module identity rejection is transactional");
    }

    /* Duplicate capability provider. */
    arbor_capability_export duplicate_export[1] = {lower_exports[0]};
    arbor_module_descriptor duplicate_provider_modules[2] = {
        make_module(module_lower_math, lower_exports, 1u, NULL, 0u),
        make_module(module_other, duplicate_export, 1u, NULL, 0u)
    };
    status = arbor_capability_catalog_prepare(
        duplicate_provider_modules, 2u, &storage, &workspace, &sentinel_catalog);
    if (status.native != -EEXIST || !catalog_equal(sentinel_catalog, unchanged_catalog)) {
        return fail("AF2 duplicate capability provider rejection is transactional");
    }

    /* Duplicate requirement in one module. */
    arbor_capability_requirement duplicate_requirements[2] = {
        application_requires[0], application_requires[0]
    };
    arbor_module_descriptor duplicate_requirement_modules[2] = {
        make_module(module_application, NULL, 0u, duplicate_requirements, 2u),
        make_module(module_lower_math, lower_exports, 1u, NULL, 0u)
    };
    arbor_capability_binding bindings3[2] = {0};
    arbor_capability_resolution resolutions3[2] = {0};
    uint64_t order3[2] = {0u, 0u};
    arbor_capability_catalog_storage storage3 = {bindings3, 2u, resolutions3, 2u, order3, 2u};
    uint64_t rpm3[2] = {0u, 0u};
    uint64_t rbi3[2] = {0u, 0u};
    uint64_t indegree3[2] = {0u, 0u};
    uint8_t selected3[2] = {0u, 0u};
    uint64_t worder3[2] = {0u, 0u};
    arbor_capability_workspace workspace3 = {rpm3, rbi3, 2u, indegree3, selected3, worder3, 2u};
    status = arbor_capability_catalog_prepare(
        duplicate_requirement_modules, 2u, &storage3, &workspace3, &sentinel_catalog);
    if (status.native != -EEXIST || !catalog_equal(sentinel_catalog, unchanged_catalog)) {
        return fail("AF2 duplicate requirement rejection is transactional");
    }

    /* Missing provider. */
    arbor_capability_requirement missing_requirement[1] = {
        make_requirement(cap_missing, (arbor_capability_version){1u, 0u}, 1u)
    };
    arbor_module_descriptor missing_modules[1] = {
        make_module(module_application, NULL, 0u, missing_requirement, 1u)
    };
    arbor_capability_resolution one_resolution[1] = {{9u, 9u, 9u}};
    uint64_t one_order[1] = {UINT64_C(9)};
    arbor_capability_catalog_storage one_storage = {NULL, 0u, one_resolution, 1u, one_order, 1u};
    uint64_t one_rpm[1] = {0u};
    uint64_t one_rbi[1] = {0u};
    uint64_t one_indegree[1] = {0u};
    uint8_t one_selected[1] = {0u};
    uint64_t one_worder[1] = {0u};
    arbor_capability_workspace one_workspace = {one_rpm, one_rbi, 1u, one_indegree, one_selected, one_worder, 1u};
    status = arbor_capability_catalog_prepare(
        missing_modules, 1u, &one_storage, &one_workspace, &sentinel_catalog);
    if (status.native != -ENOENT || !catalog_equal(sentinel_catalog, unchanged_catalog) ||
        one_resolution[0].consumer_module_index != 9u || one_order[0] != 9u) {
        return fail("AF2 missing dependency rejection preserves publication storage");
    }

    /* Incompatible version and interface size. */
    arbor_capability_requirement incompatible_requirement[1] = {
        make_requirement(cap_checked_math, (arbor_capability_version){2u, 0u}, 1u)
    };
    arbor_module_descriptor incompatible_modules[2] = {
        make_module(module_application, NULL, 0u, incompatible_requirement, 1u),
        make_module(module_lower_math, lower_exports, 1u, NULL, 0u)
    };
    status = arbor_capability_catalog_prepare(
        incompatible_modules, 2u, &storage, &workspace, &sentinel_catalog);
    if (status.native != -EPROTONOSUPPORT || !catalog_equal(sentinel_catalog, unchanged_catalog)) {
        return fail("AF2 incompatible major version rejection");
    }
    incompatible_requirement[0] = make_requirement(
        cap_checked_math,
        (arbor_capability_version){1u, 0u},
        (uint32_t)sizeof(test_checked_math_v1) + 1u);
    status = arbor_capability_catalog_prepare(
        incompatible_modules, 2u, &storage, &workspace, &sentinel_catalog);
    if (status.native != -EPROTONOSUPPORT || !catalog_equal(sentinel_catalog, unchanged_catalog)) {
        return fail("AF2 incompatible interface-size rejection");
    }

    /* Cycle: A consumes B and B consumes A. */
    static const uint64_t dummy_interface = UINT64_C(1);
    arbor_capability_export cycle_a_export[1] = {
        make_export(cap_checked_math, (arbor_capability_version){1u, 0u}, &dummy_interface, 8u, NULL)
    };
    arbor_capability_export cycle_b_export[1] = {
        make_export(cap_application_dispatch, (arbor_capability_version){1u, 0u}, &dummy_interface, 8u, NULL)
    };
    arbor_capability_requirement cycle_a_requires[1] = {
        make_requirement(cap_application_dispatch, (arbor_capability_version){1u, 0u}, 8u)
    };
    arbor_capability_requirement cycle_b_requires[1] = {
        make_requirement(cap_checked_math, (arbor_capability_version){1u, 0u}, 8u)
    };
    arbor_module_descriptor cycle_modules[2] = {
        make_module(module_application, cycle_a_export, 1u, cycle_a_requires, 1u),
        make_module(module_other, cycle_b_export, 1u, cycle_b_requires, 1u)
    };
    status = arbor_capability_catalog_prepare(
        cycle_modules, 2u, &storage3, &workspace3, &sentinel_catalog);
    if (status.native != -ELOOP || !catalog_equal(sentinel_catalog, unchanged_catalog)) {
        return fail("AF2 cyclic dependency rejection is transactional");
    }

    /* Insufficient publication capacity must fail before any storage write. */
    arbor_capability_binding storage_sentinel_binding = unchanged_binding;
    arbor_capability_resolution storage_sentinel_resolution = {7u, 7u, 7u};
    uint64_t storage_sentinel_order = UINT64_C(7);
    arbor_capability_catalog_storage small_storage = {
        &storage_sentinel_binding, 1u,
        &storage_sentinel_resolution, 1u,
        &storage_sentinel_order, 1u
    };
    status = arbor_capability_catalog_prepare(
        modules, 3u, &small_storage, &workspace, &sentinel_catalog);
    if (status.native != -ENOSPC || !catalog_equal(sentinel_catalog, unchanged_catalog) ||
        !binding_equal(storage_sentinel_binding, unchanged_binding) ||
        storage_sentinel_resolution.consumer_module_index != 7u ||
        storage_sentinel_order != 7u) {
        return fail("AF2 publication capacity failure is transactional");
    }

    /* Publication/workspace aliasing is rejected before either region is written. */
    _Alignas(arbor_capability_binding) uint8_t alias_bytes[256] = {0};
    arbor_capability_catalog_storage alias_storage = {
        (arbor_capability_binding *)(void *)alias_bytes,
        2u,
        resolutions,
        2u,
        published_order,
        3u
    };
    arbor_capability_workspace alias_workspace = {
        workspace_provider_modules,
        (uint64_t *)(void *)alias_bytes,
        2u,
        workspace_indegree,
        workspace_selected,
        workspace_order,
        3u
    };
    status = arbor_capability_catalog_prepare(
        modules, 3u, &alias_storage, &alias_workspace, &sentinel_catalog);
    if (status.native != -EINVAL || !catalog_equal(sentinel_catalog, unchanged_catalog)) {
        return fail("AF2 overlapping publication/workspace regions are rejected transactionally");
    }

    /* Count overflow is delegated to the qualified lower checked-add primitive. */
    arbor_module_descriptor overflow_modules[2] = {
        make_module(module_application, lower_exports, UINT64_MAX, NULL, 0u),
        make_module(module_other, lower_exports, 1u, NULL, 0u)
    };
    arbor_capability_catalog_requirements unchanged_measure = {7u, 8u, 9u};
    measured = unchanged_measure;
    status = arbor_capability_catalog_measure(overflow_modules, 2u, &measured);
    if (status.native != -EOVERFLOW ||
        measured.module_count != unchanged_measure.module_count ||
        measured.binding_count != unchanged_measure.binding_count ||
        measured.resolution_count != unchanged_measure.resolution_count) {
        return fail("AF2 checked count overflow is transactional");
    }

    /* Published catalog validation detects post-publication mutation/corruption. */
    uint64_t saved_order = published_order[0];
    published_order[0] = published_order[1];
    if (arbor_capability_catalog_validate(&catalog).native != -EINVAL) {
        return fail("AF2 immutable publication corruption detection");
    }
    published_order[0] = saved_order;
    if (arbor_capability_catalog_validate(&catalog).native != 0) {
        return fail("AF2 catalog remains valid after restoring immutable storage");
    }

    arbor_module_descriptor independent_modules[2] = {
        make_module(module_application, NULL, 0u, NULL, 0u),
        make_module(module_other, NULL, 0u, NULL, 0u)
    };
    uint64_t independent_order[2] = {1u, 0u};
    arbor_capability_catalog noncanonical_catalog = {
        ARBOR_CAPABILITY_KERNEL_ABI_VERSION,
        (uint32_t)sizeof(arbor_capability_catalog),
        ARBOR_CAPABILITY_CATALOG_FLAGS_NONE,
        independent_modules,
        2u,
        NULL,
        0u,
        NULL,
        0u,
        independent_order
    };
    if (arbor_capability_catalog_validate(&noncanonical_catalog).native != -EINVAL) {
        return fail("AF2 catalog validation rejects non-canonical topological order");
    }

    puts("PASS: AF2 immutable bidirectional capability/module kernel");
    return 0;
}
