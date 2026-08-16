#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <arborcore/application_service.h>

static int fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    return 1;
}

typedef struct test_service_v1 {
    arbor_application_service_interface_header header;
    int64_t (*execute)(void *provider_context, const uint64_t *input, uint64_t *output);
} test_service_v1;

typedef struct test_lifecycle_context {
    uint64_t id;
    uint64_t prepare_calls;
    uint64_t rollback_calls;
    uint64_t stop_calls;
    int64_t prepare_return;
    int64_t stop_return;
    bool prepared;
    uint64_t *trace;
    uint64_t *trace_count;
} test_lifecycle_context;

static int64_t test_execute(void *provider_context, const uint64_t *input, uint64_t *output)
{
    (void)provider_context;
    if (input == NULL || output == NULL) {
        return -EINVAL;
    }
    *output = *input;
    return 0;
}

static void append_trace(test_lifecycle_context *context, uint64_t base)
{
    if (context != NULL && context->trace != NULL && context->trace_count != NULL &&
        *context->trace_count < UINT64_C(64)) {
        context->trace[*context->trace_count] = base + context->id;
        *context->trace_count += UINT64_C(1);
    }
}

static int64_t lifecycle_prepare(
    void *module_context,
    const arbor_application_service_prepare_context *prepare_context)
{
    test_lifecycle_context *context = (test_lifecycle_context *)module_context;
    if (context == NULL || prepare_context == NULL) {
        return -EINVAL;
    }
    context->prepare_calls += UINT64_C(1);
    append_trace(context, UINT64_C(100));
    context->prepared = false;
    if (context->prepare_return != 0) {
        return context->prepare_return;
    }
    context->prepared = true;
    return 0;
}

static int64_t invalid_requirement_prepare(
    void *module_context,
    const arbor_application_service_prepare_context *prepare_context)
{
    if (module_context == NULL || prepare_context == NULL) {
        return -EINVAL;
    }

    arbor_capability_binding sentinel = {
        {UINT64_C(9), UINT64_C(9)}, {9u, 9u}, 9u, 9u, 9u,
        prepare_context, module_context, 9u
    };
    arbor_capability_binding binding = sentinel;
    arbor_status status = arbor_application_service_prepare_resolve(
        prepare_context, 0u, &binding);
    if (status.native != -EINVAL || memcmp(&binding, &sentinel, sizeof(binding)) != 0) {
        return -EFAULT;
    }
    return status.native;
}

typedef struct alias_prepare_context {
    arbor_capability_binding *illegal_output;
    int64_t observed_status;
} alias_prepare_context;

static int64_t alias_output_prepare(
    void *module_context,
    const arbor_application_service_prepare_context *prepare_context)
{
    alias_prepare_context *context = (alias_prepare_context *)module_context;
    if (context == NULL || prepare_context == NULL || context->illegal_output == NULL) {
        return -EINVAL;
    }

    arbor_status status = arbor_application_service_prepare_resolve(
        prepare_context,
        0u,
        context->illegal_output);
    context->observed_status = status.native;
    return status.native;
}

static void alias_output_rollback(void *module_context)
{
    (void)module_context;
}

static int64_t alias_output_stop(void *module_context)
{
    return module_context == NULL ? -EINVAL : 0;
}

static void lifecycle_rollback(void *module_context)
{
    test_lifecycle_context *context = (test_lifecycle_context *)module_context;
    if (context == NULL) {
        return;
    }
    context->rollback_calls += UINT64_C(1);
    append_trace(context, UINT64_C(200));
    context->prepared = false;
}

static int64_t lifecycle_stop(void *module_context)
{
    test_lifecycle_context *context = (test_lifecycle_context *)module_context;
    if (context == NULL) {
        return -EINVAL;
    }
    context->stop_calls += UINT64_C(1);
    append_trace(context, UINT64_C(300));
    if (context->stop_return == 0) {
        context->prepared = false;
    }
    return context->stop_return;
}

static arbor_module_descriptor make_module(
    arbor_module_id id,
    const arbor_capability_export *provides,
    uint64_t provides_count)
{
    return (arbor_module_descriptor){
        id,
        ARBOR_MODULE_DESCRIPTOR_ABI_VERSION,
        (uint32_t)sizeof(arbor_module_descriptor),
        ARBOR_MODULE_FLAGS_NONE,
        provides,
        provides_count,
        NULL,
        0u
    };
}

static arbor_capability_export make_export(
    arbor_capability_id id,
    const void *table,
    uint32_t size,
    void *provider_context)
{
    return (arbor_capability_export){
        id,
        {1u, 0u},
        size,
        0u,
        ARBOR_CAPABILITY_FLAGS_NONE,
        table,
        provider_context
    };
}

static bool runtime_equal(arbor_application_runtime left, arbor_application_runtime right)
{
    return left.abi_version == right.abi_version &&
           left.struct_size == right.struct_size &&
           left.flags == right.flags &&
           left.catalog == right.catalog &&
           left.service_modules == right.service_modules &&
           left.managed_module_count == right.managed_module_count &&
           left.records == right.records &&
           left.state == right.state &&
           left.reserved0 == right.reserved0;
}

static bool record_equal(
    arbor_application_service_runtime_record left,
    arbor_application_service_runtime_record right)
{
    return left.descriptor_index == right.descriptor_index &&
           left.module_index == right.module_index &&
           left.state == right.state &&
           left.reserved0 == right.reserved0 &&
           left.stop_native == right.stop_native;
}

typedef struct basic_fixture {
    test_service_v1 service_table;
    uint64_t raw_table;
    arbor_capability_export managed_exports[2];
    arbor_capability_export unmanaged_exports[1];
    arbor_module_descriptor modules[2];
    arbor_capability_binding bindings[3];
    uint64_t order[2];
    arbor_capability_catalog_storage storage;
    uint64_t indegree[2];
    uint8_t selected[2];
    uint64_t workspace_order[2];
    arbor_capability_workspace workspace;
    arbor_capability_catalog catalog;
    uint64_t service_indices[1];
    test_lifecycle_context context;
    arbor_application_service_module_descriptor descriptor;
} basic_fixture;

static int basic_fixture_init(basic_fixture *fixture)
{
    (void)memset(fixture, 0, sizeof(*fixture));

    static const arbor_module_id managed_id = {UINT64_C(0x510), UINT64_C(1)};
    static const arbor_module_id unmanaged_id = {UINT64_C(0x510), UINT64_C(2)};
    static const arbor_capability_id managed_service = {UINT64_C(0x520), UINT64_C(1)};
    static const arbor_capability_id managed_nonservice = {UINT64_C(0x520), UINT64_C(2)};
    static const arbor_capability_id unmanaged_cap = {UINT64_C(0x520), UINT64_C(3)};

    fixture->service_table = (test_service_v1){
        {ARBOR_APPLICATION_SERVICE_INTERFACE_ABI_VERSION,
         (uint32_t)sizeof(test_service_v1),
         ARBOR_APPLICATION_SERVICE_INTERFACE_FLAGS_NONE},
        test_execute
    };
    fixture->raw_table = UINT64_C(0x12345678);
    fixture->managed_exports[0] = make_export(
        managed_service,
        &fixture->service_table,
        (uint32_t)sizeof(fixture->service_table),
        &fixture->context);
    fixture->managed_exports[1] = make_export(
        managed_nonservice,
        &fixture->raw_table,
        (uint32_t)sizeof(fixture->raw_table),
        NULL);
    fixture->unmanaged_exports[0] = make_export(
        unmanaged_cap,
        &fixture->raw_table,
        (uint32_t)sizeof(fixture->raw_table),
        NULL);

    fixture->modules[0] = make_module(managed_id, fixture->managed_exports, 2u);
    fixture->modules[1] = make_module(unmanaged_id, fixture->unmanaged_exports, 1u);

    fixture->storage = (arbor_capability_catalog_storage){
        fixture->bindings, 3u, NULL, 0u, fixture->order, 2u
    };
    fixture->workspace = (arbor_capability_workspace){
        NULL, NULL, 0u, fixture->indegree, fixture->selected, fixture->workspace_order, 2u
    };

    arbor_status status = arbor_capability_catalog_prepare(
        fixture->modules, 2u, &fixture->storage, &fixture->workspace, &fixture->catalog);
    if (status.native != 0) {
        return fail("AF3 adversarial basic AF2 catalog");
    }

    fixture->service_indices[0] = 0u;
    fixture->context = (test_lifecycle_context){.id = 1u};
    fixture->descriptor = (arbor_application_service_module_descriptor){
        managed_id,
        ARBOR_APPLICATION_SERVICE_MODULE_DESCRIPTOR_ABI_VERSION,
        (uint32_t)sizeof(arbor_application_service_module_descriptor),
        ARBOR_APPLICATION_SERVICE_MODULE_FLAGS_NONE,
        &fixture->context,
        fixture->service_indices,
        1u,
        lifecycle_prepare,
        lifecycle_rollback,
        lifecycle_stop
    };
    return 0;
}

static int test_null_public_api(void)
{
    arbor_application_runtime_requirements requirements = {7u, 8u};
    if (arbor_application_runtime_measure(NULL, NULL, 0u, &requirements).native != -EINVAL ||
        requirements.managed_module_record_count != 7u || requirements.module_map_count != 8u) {
        return fail("AF3 NULL catalog measure rejection is transactional");
    }
    if (arbor_application_runtime_measure(NULL, NULL, 0u, NULL).native != -EINVAL ||
        arbor_application_runtime_validate(NULL).native != -EINVAL ||
        arbor_application_service_prepare_resolve(NULL, 0u, NULL).native != -EINVAL ||
        arbor_application_runtime_stop(NULL).native != -EINVAL) {
        return fail("AF3 NULL public-object rejection");
    }

    arbor_capability_binding binding = {0};
    if (arbor_application_runtime_find_ready(
            NULL, (arbor_capability_id){UINT64_C(1), UINT64_C(1)},
            (arbor_capability_version){1u, 0u}, 1u, &binding).native != -EINVAL ||
        arbor_application_runtime_find_ready(
            NULL, (arbor_capability_id){UINT64_C(1), UINT64_C(1)},
            (arbor_capability_version){1u, 0u}, 1u, NULL).native != -EINVAL) {
        return fail("AF3 NULL ready-discovery rejection");
    }
    return 0;
}

static int test_interface_validation(void)
{
    test_service_v1 table = {
        {ARBOR_APPLICATION_SERVICE_INTERFACE_ABI_VERSION,
         (uint32_t)sizeof(test_service_v1),
         ARBOR_APPLICATION_SERVICE_INTERFACE_FLAGS_NONE},
        test_execute
    };
    arbor_capability_binding binding = {
        {UINT64_C(1), UINT64_C(2)}, {1u, 0u}, (uint32_t)sizeof(table), 0u,
        ARBOR_CAPABILITY_FLAGS_NONE, &table, NULL, 0u
    };

    if (arbor_application_service_interface_validate(&binding).native != 0 ||
        arbor_application_service_interface_validate(NULL).native != -EINVAL) {
        return fail("AF3 interface baseline/null validation");
    }

    arbor_capability_binding changed = binding;
    changed.id = (arbor_capability_id){0u, 0u};
    if (arbor_application_service_interface_validate(&changed).native != -EINVAL) {
        return fail("AF3 zero capability ID rejection");
    }
    changed = binding;
    changed.reserved0 = 1u;
    if (arbor_application_service_interface_validate(&changed).native != -EINVAL) {
        return fail("AF3 binding reserved rejection");
    }
    changed = binding;
    changed.flags = 1u;
    if (arbor_application_service_interface_validate(&changed).native != -EINVAL) {
        return fail("AF3 binding flags rejection");
    }
    changed = binding;
    changed.interface_size = 8u;
    if (arbor_application_service_interface_validate(&changed).native != -EINVAL) {
        return fail("AF3 too-small interface rejection");
    }

    uint32_t saved_abi = table.header.abi_version;
    table.header.abi_version = 2u;
    if (arbor_application_service_interface_validate(&binding).native != -EPROTONOSUPPORT) {
        return fail("AF3 unsupported interface ABI rejection");
    }
    table.header.abi_version = saved_abi;

    uint32_t saved_size = table.header.struct_size;
    table.header.struct_size = saved_size - 1u;
    if (arbor_application_service_interface_validate(&binding).native != -EINVAL) {
        return fail("AF3 interface size mismatch rejection");
    }
    table.header.struct_size = saved_size;

    uint64_t saved_flags = table.header.flags;
    table.header.flags = 1u;
    if (arbor_application_service_interface_validate(&binding).native != -EINVAL) {
        return fail("AF3 interface flags rejection");
    }
    table.header.flags = saved_flags;

    _Alignas(arbor_application_service_interface_header) uint8_t bytes[64] = {0};
    changed = binding;
    changed.interface_table = &bytes[1];
    if (arbor_application_service_interface_validate(&changed).native != -EINVAL) {
        return fail("AF3 unaligned interface rejection");
    }

    return 0;
}

static int test_descriptor_validation(void)
{
    basic_fixture fixture;
    if (basic_fixture_init(&fixture) != 0) {
        return 1;
    }

    arbor_application_runtime_requirements sentinel = {77u, 88u};
    arbor_application_runtime_requirements out = sentinel;
    arbor_status status = arbor_application_runtime_measure(
        &fixture.catalog, &fixture.descriptor, 1u, &out);
    if (status.native != 0 || out.managed_module_record_count != 1u || out.module_map_count != 2u) {
        return fail("AF3 descriptor validation baseline");
    }

#define EXPECT_MEASURE_FAILURE(mutator, expected, label) \
    do { \
        arbor_application_service_module_descriptor d_ = fixture.descriptor; \
        mutator; \
        out = sentinel; \
        status = arbor_application_runtime_measure(&fixture.catalog, &d_, 1u, &out); \
        if (status.native != (expected) || \
            out.managed_module_record_count != sentinel.managed_module_record_count || \
            out.module_map_count != sentinel.module_map_count) { \
            return fail(label); \
        } \
    } while (0)

    const arbor_module_id zero_module_id = {0u, 0u};
    EXPECT_MEASURE_FAILURE(d_.module_id = zero_module_id, -EINVAL,
        "AF3 zero service-module ID rejection");
    EXPECT_MEASURE_FAILURE(d_.abi_version = 2u, -EINVAL,
        "AF3 descriptor ABI rejection");
    EXPECT_MEASURE_FAILURE(d_.struct_size -= 8u, -EINVAL,
        "AF3 descriptor exact-size rejection");
    EXPECT_MEASURE_FAILURE(d_.flags = 1u, -EINVAL,
        "AF3 descriptor flags rejection");
    EXPECT_MEASURE_FAILURE(d_.service_export_count = 0u, -EINVAL,
        "AF3 zero service-export count rejection");
    EXPECT_MEASURE_FAILURE(d_.service_export_indices = NULL, -EINVAL,
        "AF3 NULL service-export index rejection");
    EXPECT_MEASURE_FAILURE(d_.prepare = NULL, -EINVAL,
        "AF3 partial lifecycle callback rejection");
    const arbor_module_id missing_module_id = {UINT64_C(9), UINT64_C(9)};
    EXPECT_MEASURE_FAILURE(d_.module_id = missing_module_id, -ENOENT,
        "AF3 missing service-module rejection");

#undef EXPECT_MEASURE_FAILURE

    uint64_t out_of_range_indices[1] = {2u};
    arbor_application_service_module_descriptor changed = fixture.descriptor;
    changed.service_export_indices = out_of_range_indices;
    out = sentinel;
    status = arbor_application_runtime_measure(&fixture.catalog, &changed, 1u, &out);
    if (status.native != -EINVAL || memcmp(&out, &sentinel, sizeof(out)) != 0) {
        return fail("AF3 out-of-range service-export index rejection");
    }

    uint64_t duplicate_indices[2] = {0u, 0u};
    changed = fixture.descriptor;
    changed.service_export_indices = duplicate_indices;
    changed.service_export_count = 2u;
    out = sentinel;
    status = arbor_application_runtime_measure(&fixture.catalog, &changed, 1u, &out);
    if (status.native != -EEXIST || memcmp(&out, &sentinel, sizeof(out)) != 0) {
        return fail("AF3 duplicate service-export index rejection");
    }

    arbor_application_service_module_descriptor duplicate_descriptors[2] = {
        fixture.descriptor, fixture.descriptor
    };
    out = sentinel;
    status = arbor_application_runtime_measure(
        &fixture.catalog, duplicate_descriptors, 2u, &out);
    if (status.native != -EEXIST || memcmp(&out, &sentinel, sizeof(out)) != 0) {
        return fail("AF3 duplicate managed module rejection");
    }

    uint32_t saved_abi = fixture.service_table.header.abi_version;
    fixture.service_table.header.abi_version = 2u;
    out = sentinel;
    status = arbor_application_runtime_measure(&fixture.catalog, &fixture.descriptor, 1u, &out);
    if (status.native != -EPROTONOSUPPORT || memcmp(&out, &sentinel, sizeof(out)) != 0) {
        return fail("AF3 managed service interface ABI rejection");
    }
    fixture.service_table.header.abi_version = saved_abi;

    uint32_t saved_size = fixture.service_table.header.struct_size;
    fixture.service_table.header.struct_size = saved_size - 1u;
    out = sentinel;
    status = arbor_application_runtime_measure(&fixture.catalog, &fixture.descriptor, 1u, &out);
    if (status.native != -EINVAL || memcmp(&out, &sentinel, sizeof(out)) != 0) {
        return fail("AF3 managed service interface publication-size mismatch");
    }
    fixture.service_table.header.struct_size = saved_size;

    return 0;
}

static int prepare_one(
    basic_fixture *fixture,
    arbor_application_service_runtime_record *persistent,
    arbor_application_service_runtime_record *scratch,
    uint64_t *module_map,
    arbor_application_runtime *runtime)
{
    arbor_application_runtime_storage storage = {persistent, 1u};
    arbor_application_runtime_workspace workspace = {scratch, 1u, module_map, 2u};
    arbor_status status = arbor_application_runtime_prepare(
        &fixture->catalog, &fixture->descriptor, 1u, &storage, &workspace, runtime);
    return (int)status.native;
}

static int test_capacity_alias_and_overflow(void)
{
    basic_fixture fixture;
    if (basic_fixture_init(&fixture) != 0) {
        return 1;
    }

    arbor_application_service_runtime_record persistent = {7u, 7u, 7u, 7u, 7};
    arbor_application_service_runtime_record scratch = {0};
    uint64_t module_map[2] = {0u, 0u};
    arbor_application_runtime sentinel = {9u, 9u, 9u, &fixture.catalog, NULL, 9u, NULL, 9u, 9u};
    arbor_application_runtime runtime = sentinel;

    arbor_application_runtime_storage small_storage = {&persistent, 0u};
    arbor_application_runtime_workspace workspace = {&scratch, 1u, module_map, 2u};
    arbor_status status = arbor_application_runtime_prepare(
        &fixture.catalog, &fixture.descriptor, 1u, &small_storage, &workspace, &runtime);
    if (status.native != -ENOSPC || !runtime_equal(runtime, sentinel)) {
        return fail("AF3 insufficient persistent record capacity");
    }

    arbor_application_runtime_storage storage = {&persistent, 1u};
    arbor_application_runtime_workspace small_workspace = {&scratch, 0u, module_map, 2u};
    runtime = sentinel;
    status = arbor_application_runtime_prepare(
        &fixture.catalog, &fixture.descriptor, 1u, &storage, &small_workspace, &runtime);
    if (status.native != -ENOSPC || !runtime_equal(runtime, sentinel)) {
        return fail("AF3 insufficient workspace record capacity");
    }

    small_workspace = (arbor_application_runtime_workspace){&scratch, 1u, module_map, 1u};
    runtime = sentinel;
    status = arbor_application_runtime_prepare(
        &fixture.catalog, &fixture.descriptor, 1u, &storage, &small_workspace, &runtime);
    if (status.native != -ENOSPC || !runtime_equal(runtime, sentinel)) {
        return fail("AF3 insufficient module-map capacity");
    }

    arbor_application_service_module_descriptor descriptor_before = fixture.descriptor;
    arbor_application_runtime_requirements *aliased_requirements =
        (arbor_application_runtime_requirements *)(void *)&fixture.descriptor;
    status = arbor_application_runtime_measure(
        &fixture.catalog, &fixture.descriptor, 1u, aliased_requirements);
    if (status.native != -EINVAL ||
        memcmp(&fixture.descriptor, &descriptor_before, sizeof(descriptor_before)) != 0) {
        return fail("AF3 measure output/immutable descriptor alias rejection");
    }

    arbor_application_runtime_workspace alias_workspace = {
        &persistent, 1u, module_map, 2u
    };
    runtime = sentinel;
    status = arbor_application_runtime_prepare(
        &fixture.catalog, &fixture.descriptor, 1u, &storage, &alias_workspace, &runtime);
    if (status.native != -EINVAL || !runtime_equal(runtime, sentinel)) {
        return fail("AF3 persistent/workspace record alias rejection");
    }

    _Alignas(arbor_application_service_runtime_record) uint8_t workspace_alias[64] = {0};
    alias_workspace = (arbor_application_runtime_workspace){
        (arbor_application_service_runtime_record *)(void *)workspace_alias,
        1u,
        (uint64_t *)(void *)workspace_alias,
        2u
    };
    runtime = sentinel;
    status = arbor_application_runtime_prepare(
        &fixture.catalog, &fixture.descriptor, 1u, &storage, &alias_workspace, &runtime);
    if (status.native != -EINVAL || !runtime_equal(runtime, sentinel)) {
        return fail("AF3 workspace records/module-map alias rejection");
    }

    _Alignas(arbor_application_runtime) uint8_t output_alias[128] = {0};
    arbor_application_runtime_storage alias_storage = {
        (arbor_application_service_runtime_record *)(void *)output_alias, 1u
    };
    arbor_application_runtime *aliased_runtime = (arbor_application_runtime *)(void *)output_alias;
    workspace = (arbor_application_runtime_workspace){&scratch, 1u, module_map, 2u};
    status = arbor_application_runtime_prepare(
        &fixture.catalog, &fixture.descriptor, 1u, &alias_storage, &workspace, aliased_runtime);
    if (status.native != -EINVAL) {
        return fail("AF3 runtime output/backing-storage alias rejection");
    }

    alias_storage = (arbor_application_runtime_storage){
        (arbor_application_service_runtime_record *)(void *)&fixture.descriptor, 1u
    };
    runtime = sentinel;
    status = arbor_application_runtime_prepare(
        &fixture.catalog, &fixture.descriptor, 1u, &alias_storage, &workspace, &runtime);
    if (status.native != -EINVAL || !runtime_equal(runtime, sentinel)) {
        return fail("AF3 mutable storage/immutable descriptor alias rejection");
    }

    alias_storage = (arbor_application_runtime_storage){
        (arbor_application_service_runtime_record *)(void *)&fixture.service_table, 1u
    };
    runtime = sentinel;
    status = arbor_application_runtime_prepare(
        &fixture.catalog, &fixture.descriptor, 1u, &alias_storage, &workspace, &runtime);
    if (status.native != -EINVAL || !runtime_equal(runtime, sentinel)) {
        return fail("AF3 mutable storage/service-interface-table alias rejection");
    }

    arbor_application_service_module_descriptor invalid_resolve_descriptor = fixture.descriptor;
    invalid_resolve_descriptor.prepare = invalid_requirement_prepare;
    persistent = (arbor_application_service_runtime_record){7u, 7u, 7u, 7u, 7};
    storage = (arbor_application_runtime_storage){&persistent, 1u};
    runtime = sentinel;
    status = arbor_application_runtime_prepare(
        &fixture.catalog, &invalid_resolve_descriptor, 1u, &storage, &workspace, &runtime);
    if (status.native != -EINVAL || !runtime_equal(runtime, sentinel) ||
        persistent.descriptor_index != 7u || persistent.module_index != 7u ||
        persistent.state != 7u || persistent.reserved0 != 7u || persistent.stop_native != 7) {
        return fail("AF3 prepare-resolve invalid requirement rejection/output atomicity");
    }

    storage = (arbor_application_runtime_storage){&persistent, UINT64_MAX};
    runtime = sentinel;
    status = arbor_application_runtime_prepare(
        &fixture.catalog, &fixture.descriptor, 1u, &storage, &workspace, &runtime);
    if (status.native != -EOVERFLOW || !runtime_equal(runtime, sentinel)) {
        return fail("AF3 storage byte-range multiplication overflow rejection");
    }

    return 0;
}

typedef struct lifecycle_fixture {
    test_service_v1 tables[3];
    arbor_capability_export exports[3];
    arbor_module_descriptor modules[3];
    arbor_capability_binding bindings[3];
    uint64_t order[3];
    arbor_capability_catalog_storage storage;
    uint64_t indegree[3];
    uint8_t selected[3];
    uint64_t workspace_order[3];
    arbor_capability_workspace workspace;
    arbor_capability_catalog catalog;
    uint64_t service_index[3];
    uint64_t trace[64];
    uint64_t trace_count;
    test_lifecycle_context contexts[3];
    arbor_application_service_module_descriptor descriptors[3];
} lifecycle_fixture;

static int lifecycle_fixture_init(lifecycle_fixture *fixture)
{
    (void)memset(fixture, 0, sizeof(*fixture));

    for (uint64_t i = 0u; i < 3u; ++i) {
        fixture->tables[i] = (test_service_v1){
            {ARBOR_APPLICATION_SERVICE_INTERFACE_ABI_VERSION,
             (uint32_t)sizeof(test_service_v1),
             ARBOR_APPLICATION_SERVICE_INTERFACE_FLAGS_NONE},
            test_execute
        };
        arbor_capability_id cap = {UINT64_C(0x620), i + UINT64_C(1)};
        fixture->exports[i] = make_export(
            cap, &fixture->tables[i], (uint32_t)sizeof(test_service_v1), NULL);
        arbor_module_id module = {UINT64_C(0x610), i + UINT64_C(1)};
        fixture->modules[i] = make_module(module, &fixture->exports[i], 1u);
    }

    fixture->storage = (arbor_capability_catalog_storage){
        fixture->bindings, 3u, NULL, 0u, fixture->order, 3u
    };
    fixture->workspace = (arbor_capability_workspace){
        NULL, NULL, 0u, fixture->indegree, fixture->selected, fixture->workspace_order, 3u
    };
    arbor_status status = arbor_capability_catalog_prepare(
        fixture->modules, 3u, &fixture->storage, &fixture->workspace, &fixture->catalog);
    if (status.native != 0) {
        return fail("AF3 lifecycle fixture AF2 catalog");
    }

    for (uint64_t i = 0u; i < 3u; ++i) {
        fixture->service_index[i] = 0u;
        fixture->contexts[i] = (test_lifecycle_context){
            .id = i + UINT64_C(1),
            .trace = fixture->trace,
            .trace_count = &fixture->trace_count
        };
        fixture->descriptors[i] = (arbor_application_service_module_descriptor){
            fixture->modules[i].id,
            ARBOR_APPLICATION_SERVICE_MODULE_DESCRIPTOR_ABI_VERSION,
            (uint32_t)sizeof(arbor_application_service_module_descriptor),
            ARBOR_APPLICATION_SERVICE_MODULE_FLAGS_NONE,
            &fixture->contexts[i],
            &fixture->service_index[i],
            1u,
            lifecycle_prepare,
            lifecycle_rollback,
            lifecycle_stop
        };
    }
    return 0;
}

static int run_prepare_failure_case(uint64_t failing_index, int64_t native_failure)
{
    lifecycle_fixture fixture;
    if (lifecycle_fixture_init(&fixture) != 0) {
        return 1;
    }
    fixture.contexts[failing_index].prepare_return = native_failure;

    arbor_application_service_runtime_record persistent[3] = {
        {7u, 7u, 7u, 7u, 7}, {7u, 7u, 7u, 7u, 7}, {7u, 7u, 7u, 7u, 7}
    };
    arbor_application_service_runtime_record original[3];
    (void)memcpy(original, persistent, sizeof(original));
    arbor_application_service_runtime_record scratch[3] = {0};
    uint64_t module_map[3] = {0u, 0u, 0u};
    arbor_application_runtime_storage storage = {persistent, 3u};
    arbor_application_runtime_workspace workspace = {scratch, 3u, module_map, 3u};
    arbor_application_runtime sentinel = {9u, 9u, 9u, &fixture.catalog, NULL, 9u, NULL, 9u, 9u};
    arbor_application_runtime runtime = sentinel;

    arbor_status status = arbor_application_runtime_prepare(
        &fixture.catalog, fixture.descriptors, 3u, &storage, &workspace, &runtime);
    int64_t expected = native_failure > 0 ? -EINVAL : native_failure;
    if (status.native != expected || !runtime_equal(runtime, sentinel)) {
        return fail("AF3 preparation failure status/publication atomicity");
    }
    for (uint64_t i = 0u; i < 3u; ++i) {
        if (!record_equal(persistent[i], original[i])) {
            return fail("AF3 persistent records unchanged on prepare failure");
        }
    }

    for (uint64_t i = 0u; i < failing_index; ++i) {
        if (fixture.contexts[i].prepare_calls != 1u ||
            fixture.contexts[i].rollback_calls != 1u || fixture.contexts[i].prepared) {
            return fail("AF3 prior prepared prefix rollback");
        }
    }
    if (fixture.contexts[failing_index].prepare_calls != 1u ||
        fixture.contexts[failing_index].rollback_calls != 0u ||
        fixture.contexts[failing_index].prepared) {
        return fail("AF3 currently failing prepare is self-clean and not rollback-called");
    }
    for (uint64_t i = failing_index + UINT64_C(1); i < 3u; ++i) {
        if (fixture.contexts[i].prepare_calls != 0u || fixture.contexts[i].rollback_calls != 0u) {
            return fail("AF3 later modules not prepared after failure");
        }
    }

    fixture.contexts[failing_index].prepare_return = 0;
    fixture.trace_count = 0u;
    status = arbor_application_runtime_prepare(
        &fixture.catalog, fixture.descriptors, 3u, &storage, &workspace, &runtime);
    if (status.native != 0 || arbor_application_runtime_validate(&runtime).native != 0) {
        return fail("AF3 retry after corrected prepare failure");
    }
    if (arbor_application_runtime_stop(&runtime).native != 0) {
        return fail("AF3 cleanup after successful retry");
    }
    return 0;
}

static int test_prepare_failure_and_rollback(void)
{
    if (run_prepare_failure_case(0u, -EIO) != 0 ||
        run_prepare_failure_case(1u, -EIO) != 0 ||
        run_prepare_failure_case(2u, -EIO) != 0 ||
        run_prepare_failure_case(1u, 7) != 0) {
        return 1;
    }
    return 0;
}

static int test_shutdown_failure_continues(void)
{
    lifecycle_fixture fixture;
    if (lifecycle_fixture_init(&fixture) != 0) {
        return 1;
    }

    arbor_application_service_runtime_record persistent[3] = {0};
    arbor_application_service_runtime_record scratch[3] = {0};
    uint64_t module_map[3] = {0u, 0u, 0u};
    arbor_application_runtime_storage storage = {persistent, 3u};
    arbor_application_runtime_workspace workspace = {scratch, 3u, module_map, 3u};
    arbor_application_runtime runtime = {0};

    arbor_status status = arbor_application_runtime_prepare(
        &fixture.catalog, fixture.descriptors, 3u, &storage, &workspace, &runtime);
    if (status.native != 0) {
        return fail("AF3 stop-failure setup prepare");
    }

    fixture.trace_count = 0u;
    fixture.contexts[2].stop_return = -EIO; /* first in reverse order */
    fixture.contexts[1].stop_return = 5;    /* positive -> -EINVAL */
    fixture.contexts[0].stop_return = 0;

    status = arbor_application_runtime_stop(&runtime);
    if (status.native != -EIO || runtime.state != ARBOR_APPLICATION_RUNTIME_STOP_FAILED ||
        arbor_application_runtime_validate(&runtime).native != 0) {
        return fail("AF3 stop failure terminal state/first failure");
    }
    if (fixture.contexts[2].stop_calls != 1u || fixture.contexts[1].stop_calls != 1u ||
        fixture.contexts[0].stop_calls != 1u || fixture.trace_count != 3u ||
        fixture.trace[0] != 303u || fixture.trace[1] != 302u || fixture.trace[2] != 301u) {
        return fail("AF3 stop continues after failures in reverse order");
    }
    if (persistent[2].state != ARBOR_APPLICATION_SERVICE_RECORD_STOP_FAILED ||
        persistent[2].stop_native != -EIO ||
        persistent[1].state != ARBOR_APPLICATION_SERVICE_RECORD_STOP_FAILED ||
        persistent[1].stop_native != 5 ||
        persistent[0].state != ARBOR_APPLICATION_SERVICE_RECORD_STOPPED ||
        persistent[0].stop_native != 0) {
        return fail("AF3 stop failure raw-native record evidence");
    }

    arbor_capability_binding sentinel_binding = {
        {9u, 9u}, {9u, 9u}, 9u, 9u, 9u, &fixture, &fixture, 9u
    };
    arbor_capability_binding ready_binding = sentinel_binding;
    arbor_status ready_status = arbor_application_runtime_find_ready(
        &runtime, (arbor_capability_id){UINT64_C(0x620), UINT64_C(1)},
        (arbor_capability_version){1u, 0u}, (uint32_t)sizeof(test_service_v1),
        &ready_binding);
    if (ready_status.native != -EAGAIN ||
        memcmp(&ready_binding, &sentinel_binding, sizeof(ready_binding)) != 0) {
        return fail("AF3 ready lookup rejected after STOP_FAILED");
    }

    if (arbor_application_runtime_stop(&runtime).native != -EALREADY) {
        return fail("AF3 failed-stop retry prohibited");
    }
    return 0;
}

static int test_prepare_resolve_output_alias(void)
{
    static const arbor_module_id provider_id = {UINT64_C(0x610), UINT64_C(1)};
    static const arbor_module_id consumer_id = {UINT64_C(0x610), UINT64_C(2)};
    static const arbor_capability_id dependency_id = {UINT64_C(0x620), UINT64_C(1)};
    static const arbor_capability_id service_id = {UINT64_C(0x620), UINT64_C(2)};

    uint64_t provider_table = UINT64_C(0x1234);
    test_service_v1 consumer_table = {
        {ARBOR_APPLICATION_SERVICE_INTERFACE_ABI_VERSION,
         (uint32_t)sizeof(test_service_v1),
         ARBOR_APPLICATION_SERVICE_INTERFACE_FLAGS_NONE},
        test_execute
    };

    arbor_capability_export provider_export =
        make_export(dependency_id, &provider_table, (uint32_t)sizeof(provider_table), NULL);
    arbor_capability_export consumer_export =
        make_export(service_id, &consumer_table, (uint32_t)sizeof(consumer_table), NULL);
    arbor_capability_requirement consumer_requirement = {
        dependency_id,
        {1u, 0u},
        (uint32_t)sizeof(provider_table),
        0u,
        ARBOR_CAPABILITY_FLAGS_NONE
    };

    arbor_module_descriptor modules[2] = {
        make_module(provider_id, &provider_export, 1u),
        {
            consumer_id,
            ARBOR_MODULE_DESCRIPTOR_ABI_VERSION,
            (uint32_t)sizeof(arbor_module_descriptor),
            ARBOR_MODULE_FLAGS_NONE,
            &consumer_export,
            1u,
            &consumer_requirement,
            1u
        }
    };

    arbor_capability_binding bindings[2] = {0};
    arbor_capability_resolution resolutions[1] = {0};
    uint64_t order[2] = {0u, 0u};
    arbor_capability_catalog_storage catalog_storage = {
        bindings, 2u, resolutions, 1u, order, 2u
    };
    uint64_t resolved_provider[1] = {0u};
    uint64_t resolved_binding[1] = {0u};
    uint64_t indegree[2] = {0u, 0u};
    uint8_t selected[2] = {0u, 0u};
    uint64_t workspace_order[2] = {0u, 0u};
    arbor_capability_workspace catalog_workspace = {
        resolved_provider,
        resolved_binding,
        1u,
        indegree,
        selected,
        workspace_order,
        2u
    };
    arbor_capability_catalog catalog = {0};
    arbor_status status = arbor_capability_catalog_prepare(
        modules, 2u, &catalog_storage, &catalog_workspace, &catalog);
    if (status.native != 0) {
        return fail("AF3 prepare-resolve alias AF2 catalog setup");
    }

    arbor_capability_binding binding_one_before = bindings[1];
    alias_prepare_context context = {&bindings[1], 0};
    uint64_t service_index = 0u;
    arbor_application_service_module_descriptor descriptor = {
        consumer_id,
        ARBOR_APPLICATION_SERVICE_MODULE_DESCRIPTOR_ABI_VERSION,
        (uint32_t)sizeof(arbor_application_service_module_descriptor),
        ARBOR_APPLICATION_SERVICE_MODULE_FLAGS_NONE,
        &context,
        &service_index,
        1u,
        alias_output_prepare,
        alias_output_rollback,
        alias_output_stop
    };

    arbor_application_service_runtime_record persistent = {7u, 7u, 7u, 7u, 7};
    arbor_application_service_runtime_record persistent_before = persistent;
    arbor_application_service_runtime_record scratch = {0};
    uint64_t module_map[2] = {0u, 0u};
    arbor_application_runtime_storage storage = {&persistent, 1u};
    arbor_application_runtime_workspace workspace = {&scratch, 1u, module_map, 2u};
    arbor_application_runtime sentinel = {9u, 9u, 9u, &catalog, NULL, 9u, NULL, 9u, 9u};
    arbor_application_runtime runtime = sentinel;

    status = arbor_application_runtime_prepare(
        &catalog, &descriptor, 1u, &storage, &workspace, &runtime);
    if (status.native != -EINVAL || context.observed_status != -EINVAL ||
        memcmp(&bindings[1], &binding_one_before, sizeof(binding_one_before)) != 0 ||
        !record_equal(persistent, persistent_before) || !runtime_equal(runtime, sentinel) ||
        arbor_capability_catalog_validate(&catalog).native != 0) {
        return fail("AF3 prepare-resolve output/catalog alias rejection");
    }

    return 0;
}

static int test_runtime_corruption_and_ready_lookup(void)
{
    basic_fixture fixture;
    if (basic_fixture_init(&fixture) != 0) {
        return 1;
    }

    arbor_application_service_runtime_record persistent = {0};
    arbor_application_service_runtime_record scratch = {0};
    uint64_t module_map[2] = {0u, 0u};
    arbor_application_runtime runtime = {0};
    if (prepare_one(&fixture, &persistent, &scratch, module_map, &runtime) != 0) {
        return fail("AF3 corruption-test runtime setup");
    }

    arbor_application_runtime changed = runtime;
    changed.abi_version = 2u;
    if (arbor_application_runtime_validate(&changed).native != -EINVAL) {
        return fail("AF3 runtime ABI corruption detection");
    }
    changed = runtime;
    changed.struct_size = (uint32_t)sizeof(arbor_application_runtime) - 1u;
    if (arbor_application_runtime_validate(&changed).native != -EINVAL) {
        return fail("AF3 runtime struct-size corruption detection");
    }
    changed = runtime;
    changed.flags = 1u;
    if (arbor_application_runtime_validate(&changed).native != -EINVAL) {
        return fail("AF3 runtime flags corruption detection");
    }
    changed = runtime;
    changed.reserved0 = 1u;
    if (arbor_application_runtime_validate(&changed).native != -EINVAL) {
        return fail("AF3 runtime reserved corruption detection");
    }

    arbor_application_service_runtime_record saved = persistent;
    persistent.descriptor_index = 9u;
    if (arbor_application_runtime_validate(&runtime).native != -EINVAL) {
        return fail("AF3 record descriptor-index corruption detection");
    }
    persistent = saved;
    persistent.state = ARBOR_APPLICATION_SERVICE_RECORD_PREPARING;
    if (arbor_application_runtime_validate(&runtime).native != -EINVAL) {
        return fail("AF3 transient workspace state rejected in published READY runtime");
    }
    persistent = saved;
    persistent.reserved0 = 1u;
    if (arbor_application_runtime_validate(&runtime).native != -EINVAL) {
        return fail("AF3 record reserved corruption detection");
    }
    persistent = saved;

    typedef struct extended_runtime_alias {
        arbor_application_runtime runtime;
        arbor_application_service_runtime_record extension_record;
    } extended_runtime_alias;

    extended_runtime_alias extended = {
        runtime,
        persistent
    };
    extended.runtime.struct_size = (uint32_t)sizeof(extended);
    extended.runtime.records = &extended.extension_record;
    if (arbor_application_runtime_validate(&extended.runtime).native != -EINVAL) {
        return fail("AF3 prefix-extended runtime/record overlap rejection");
    }

    changed = runtime;
    changed.catalog = NULL;
    changed.state = ARBOR_APPLICATION_RUNTIME_STOPPED;
    if (arbor_application_runtime_stop(&changed).native != -EINVAL) {
        return fail("AF3 malformed terminal runtime validation precedes EALREADY");
    }

    static const arbor_capability_id managed_nonservice = {UINT64_C(0x520), UINT64_C(2)};
    static const arbor_capability_id unmanaged_cap = {UINT64_C(0x520), UINT64_C(3)};
    arbor_capability_binding sentinel = {
        {9u, 9u}, {9u, 9u}, 9u, 9u, 9u, &fixture, &fixture, 9u
    };
    typedef union runtime_binding_alias {
        arbor_application_runtime runtime;
        arbor_capability_binding binding;
    } runtime_binding_alias;

    runtime_binding_alias output_alias = {0};
    output_alias.runtime = runtime;
    static const arbor_capability_id managed_service = {UINT64_C(0x520), UINT64_C(1)};
    arbor_status status = arbor_application_runtime_find_ready(
        &output_alias.runtime,
        managed_service,
        (arbor_capability_version){1u, 0u},
        (uint32_t)sizeof(test_service_v1),
        &output_alias.binding);
    if (status.native != -EINVAL || !runtime_equal(output_alias.runtime, runtime)) {
        return fail("AF3 ready-lookup output/runtime alias rejection");
    }

    arbor_capability_binding binding = sentinel;
    status = arbor_application_runtime_find_ready(
        &runtime, unmanaged_cap, (arbor_capability_version){1u, 0u}, 1u, &binding);
    if (status.native != -ENOENT || memcmp(&binding, &sentinel, sizeof(binding)) != 0) {
        return fail("AF3 unmanaged capability rejected by ready-service discovery");
    }
    binding = sentinel;
    status = arbor_application_runtime_find_ready(
        &runtime, managed_nonservice, (arbor_capability_version){1u, 0u}, 1u, &binding);
    if (status.native != -ENOENT || memcmp(&binding, &sentinel, sizeof(binding)) != 0) {
        return fail("AF3 undeclared managed export rejected by ready-service discovery");
    }

    runtime.state = ARBOR_APPLICATION_RUNTIME_STOPPING;
    if (arbor_application_runtime_stop(&runtime).native != -EBUSY) {
        return fail("AF3 reentrant/concurrent STOPPING stop rejection");
    }
    binding = sentinel;
    status = arbor_application_runtime_find_ready(
        &runtime, managed_service, (arbor_capability_version){1u, 0u},
        (uint32_t)sizeof(test_service_v1), &binding);
    if (status.native != -EAGAIN || memcmp(&binding, &sentinel, sizeof(binding)) != 0) {
        return fail("AF3 ready lookup rejects STOPPING runtime transactionally");
    }
    runtime.state = ARBOR_APPLICATION_RUNTIME_READY;

    if (arbor_application_runtime_stop(&runtime).native != 0) {
        return fail("AF3 corruption-test cleanup stop");
    }
    return 0;
}

int main(void)
{
    if (test_null_public_api() != 0 ||
        test_interface_validation() != 0 ||
        test_descriptor_validation() != 0 ||
        test_capacity_alias_and_overflow() != 0 ||
        test_prepare_failure_and_rollback() != 0 ||
        test_shutdown_failure_continues() != 0 ||
        test_prepare_resolve_output_alias() != 0 ||
        test_runtime_corruption_and_ready_lookup() != 0) {
        return 1;
    }

    puts("PASS: AF3 adversarial lifecycle, publication, aliasing, failure and corruption qualification");
    return 0;
}
