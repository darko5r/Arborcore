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

typedef struct test_trace {
    uint64_t count;
    uint64_t events[64];
} test_trace;

static void trace_append(test_trace *trace, uint64_t event)
{
    if (trace != NULL && trace->count < UINT64_C(64)) {
        trace->events[trace->count] = event;
        trace->count += UINT64_C(1);
    }
}

static bool capability_id_equal(arbor_capability_id left, arbor_capability_id right)
{
    return left.high == right.high && left.low == right.low;
}

typedef struct test_checked_math_v1 {
    uint32_t abi_version;
    uint32_t struct_size;
    uint64_t flags;
    arbor_asm_result_u64 (*add_checked)(uint64_t left, uint64_t right);
} test_checked_math_v1;

typedef enum test_add_outcome_kind {
    TEST_ADD_APPLIED = 1,
    TEST_ADD_LIMIT_EXCEEDED = 2
} test_add_outcome_kind;

typedef struct test_add_command {
    uint64_t left;
    uint64_t right;
    uint64_t limit;
} test_add_command;

typedef struct test_add_result {
    uint32_t kind;
    uint32_t reserved0;
    uint64_t value;
} test_add_result;

typedef int64_t (*test_add_execute_fn)(
    void *provider_context,
    const test_add_command *command,
    test_add_result *result_out);

typedef struct test_add_service_v1 {
    arbor_application_service_interface_header header;
    test_add_execute_fn execute;
} test_add_service_v1;

typedef struct test_scalar_input {
    uint64_t value;
} test_scalar_input;

typedef struct test_scalar_output {
    uint64_t value;
} test_scalar_output;

typedef int64_t (*test_scalar_execute_fn)(
    void *provider_context,
    const test_scalar_input *input,
    test_scalar_output *output);

typedef struct test_scalar_service_v1 {
    arbor_application_service_interface_header header;
    test_scalar_execute_fn execute;
} test_scalar_service_v1;

typedef struct test_module_context {
    test_trace *trace;
    uint64_t id;
    arbor_capability_binding cached_dependency;
    uint64_t prepare_calls;
    uint64_t rollback_calls;
    uint64_t stop_calls;
    int64_t prepare_return;
    int64_t stop_return;
    bool prepared;
} test_module_context;

typedef struct test_service_provider_context {
    test_module_context *module;
    uint64_t bias;
} test_service_provider_context;

typedef struct test_asm_lifecycle_context {
    test_trace *trace;
    uint64_t id;
    uint64_t prepare_calls;
    uint64_t rollback_calls;
    uint64_t stop_calls;
    int64_t prepare_return;
    int64_t stop_return;
    uint64_t stack_errors;
} test_asm_lifecycle_context;

typedef struct test_asm_provider_context {
    uint64_t bias;
    int64_t native_return;
    uint64_t calls;
    uint64_t stack_errors;
} test_asm_provider_context;

extern int64_t af3_asm_prepare(
    void *module_context,
    const arbor_application_service_prepare_context *prepare_context);
extern void af3_asm_rollback(void *module_context);
extern int64_t af3_asm_stop(void *module_context);
extern int64_t af3_asm_typed_method(
    void *provider_context,
    const test_scalar_input *input,
    test_scalar_output *output);
extern int64_t af3_asm_call_c_typed(
    test_scalar_execute_fn function,
    void *provider_context,
    const test_scalar_input *input,
    test_scalar_output *output);
extern int64_t af3_asm_call_c_typed_preserve(
    test_scalar_execute_fn function,
    void *provider_context,
    const test_scalar_input *input,
    test_scalar_output *output);

_Static_assert(sizeof(test_checked_math_v1) == 24u, "test math table drift");
_Static_assert(sizeof(test_add_service_v1) == 24u, "test add service table drift");
_Static_assert(sizeof(test_scalar_service_v1) == 24u, "test scalar service table drift");
_Static_assert(sizeof(test_add_result) == 16u, "test add result drift");
_Static_assert(sizeof(test_asm_lifecycle_context) == 64u, "ASM lifecycle context drift");
_Static_assert(sizeof(test_asm_provider_context) == 32u, "ASM provider context drift");

static const arbor_module_id module_b = {UINT64_C(0x300), UINT64_C(0x01)};
static const arbor_module_id module_c = {UINT64_C(0x300), UINT64_C(0x02)};
static const arbor_module_id module_a = {UINT64_C(0x300), UINT64_C(0x03)};
static const arbor_module_id module_math = {UINT64_C(0x300), UINT64_C(0x04)};

static const arbor_capability_id cap_b = {UINT64_C(0x400), UINT64_C(0x01)};
static const arbor_capability_id cap_c = {UINT64_C(0x400), UINT64_C(0x02)};
static const arbor_capability_id cap_a_primary = {UINT64_C(0x400), UINT64_C(0x03)};
static const arbor_capability_id cap_a_secondary = {UINT64_C(0x400), UINT64_C(0x04)};
static const arbor_capability_id cap_math = {UINT64_C(0x400), UINT64_C(0x05)};

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
    const void *interface_table,
    uint32_t interface_size,
    void *provider_context)
{
    return (arbor_capability_export){
        id,
        {1u, 0u},
        interface_size,
        0u,
        ARBOR_CAPABILITY_FLAGS_NONE,
        interface_table,
        provider_context
    };
}

static arbor_capability_requirement make_requirement(
    arbor_capability_id id,
    uint32_t minimum_interface_size)
{
    return (arbor_capability_requirement){
        id,
        {1u, 0u},
        minimum_interface_size,
        0u,
        ARBOR_CAPABILITY_FLAGS_NONE
    };
}

static int64_t module_prepare(
    void *module_context,
    const arbor_application_service_prepare_context *prepare_context)
{
    test_module_context *context = (test_module_context *)module_context;
    if (context == NULL || prepare_context == NULL) {
        return -EINVAL;
    }

    context->prepare_calls += UINT64_C(1);
    trace_append(context->trace, UINT64_C(100) + context->id);
    context->prepared = false;
    (void)memset(&context->cached_dependency, 0, sizeof(context->cached_dependency));

    if (context->prepare_return != 0) {
        return context->prepare_return;
    }

    arbor_capability_binding binding = {0};
    arbor_status status = arbor_application_service_prepare_resolve(
        prepare_context,
        0u,
        &binding);
    if (status.native != 0) {
        return status.native;
    }

    context->cached_dependency = binding;
    context->prepared = true;
    return 0;
}

static void module_rollback(void *module_context)
{
    test_module_context *context = (test_module_context *)module_context;
    if (context == NULL) {
        return;
    }
    context->rollback_calls += UINT64_C(1);
    trace_append(context->trace, UINT64_C(200) + context->id);
    context->prepared = false;
    (void)memset(&context->cached_dependency, 0, sizeof(context->cached_dependency));
}

static int64_t module_stop(void *module_context)
{
    test_module_context *context = (test_module_context *)module_context;
    if (context == NULL) {
        return -EINVAL;
    }
    context->stop_calls += UINT64_C(1);
    trace_append(context->trace, UINT64_C(300) + context->id);
    if (context->stop_return == 0) {
        context->prepared = false;
        (void)memset(&context->cached_dependency, 0, sizeof(context->cached_dependency));
    }
    return context->stop_return;
}

static int64_t service_a_execute(
    void *provider_context,
    const test_add_command *command,
    test_add_result *result_out)
{
    test_service_provider_context *provider = (test_service_provider_context *)provider_context;
    if (provider == NULL || provider->module == NULL || command == NULL || result_out == NULL ||
        !provider->module->prepared) {
        return -EINVAL;
    }

    const test_checked_math_v1 *math =
        (const test_checked_math_v1 *)provider->module->cached_dependency.interface_table;
    if (math == NULL || math->add_checked == NULL) {
        return -EINVAL;
    }

    arbor_asm_result_u64 sum = math->add_checked(command->left, command->right);
    if (sum.status != 0) {
        return sum.status;
    }

    test_add_result candidate = {
        sum.value > command->limit ? TEST_ADD_LIMIT_EXCEEDED : TEST_ADD_APPLIED,
        0u,
        sum.value + provider->bias
    };
    *result_out = candidate;
    return 0;
}

static int64_t service_b_execute(
    void *provider_context,
    const test_add_command *command,
    test_add_result *result_out)
{
    test_service_provider_context *provider = (test_service_provider_context *)provider_context;
    if (provider == NULL || provider->module == NULL || command == NULL || result_out == NULL ||
        !provider->module->prepared) {
        return -EINVAL;
    }

    const test_add_service_v1 *service_a =
        (const test_add_service_v1 *)provider->module->cached_dependency.interface_table;
    if (service_a == NULL || service_a->execute == NULL) {
        return -EINVAL;
    }

    test_add_result candidate = {0u, 0u, 0u};
    int64_t native = service_a->execute(
        provider->module->cached_dependency.provider_context,
        command,
        &candidate);
    if (native != 0) {
        return native;
    }
    candidate.value += provider->bias;
    *result_out = candidate;
    return 0;
}

static int64_t c_scalar_execute(
    void *provider_context,
    const test_scalar_input *input,
    test_scalar_output *output)
{
    test_asm_provider_context *context = (test_asm_provider_context *)provider_context;
    if (context == NULL || input == NULL || output == NULL) {
        return -EINVAL;
    }
    context->calls += UINT64_C(1);
    if (context->native_return != 0) {
        return context->native_return;
    }
    output->value = input->value + context->bias;
    return 0;
}

typedef struct test_fixture {
    test_trace trace;
    test_module_context context_a;
    test_module_context context_b;
    test_asm_lifecycle_context context_c;
    test_service_provider_context provider_a_primary;
    test_service_provider_context provider_a_secondary;
    test_service_provider_context provider_b;
    test_asm_provider_context provider_c;

    test_checked_math_v1 math_interface;
    test_add_service_v1 a_primary_interface;
    test_add_service_v1 a_secondary_interface;
    test_add_service_v1 b_interface;
    test_scalar_service_v1 c_interface;

    arbor_capability_export b_exports[1];
    arbor_capability_export c_exports[1];
    arbor_capability_export a_exports[2];
    arbor_capability_export math_exports[1];
    arbor_capability_requirement b_requires[1];
    arbor_capability_requirement a_requires[1];
    arbor_module_descriptor modules[4];

    arbor_capability_binding bindings[5];
    arbor_capability_resolution resolutions[2];
    uint64_t published_order[4];
    arbor_capability_catalog_storage catalog_storage;
    uint64_t resolved_provider_modules[2];
    uint64_t resolved_binding_indices[2];
    uint64_t indegree[4];
    uint8_t selected[4];
    uint64_t workspace_order[4];
    arbor_capability_workspace capability_workspace;
    arbor_capability_catalog catalog;

    uint64_t b_service_indices[1];
    uint64_t a_service_indices[2];
    uint64_t c_service_indices[1];
    arbor_application_service_module_descriptor service_modules[3];
} test_fixture;

static int fixture_prepare_catalog(test_fixture *fixture)
{
    (void)memset(fixture, 0, sizeof(*fixture));

    fixture->context_a = (test_module_context){
        .trace = &fixture->trace, .id = 1u, .prepare_return = 0, .stop_return = 0
    };
    fixture->context_b = (test_module_context){
        .trace = &fixture->trace, .id = 2u, .prepare_return = 0, .stop_return = 0
    };
    fixture->context_c = (test_asm_lifecycle_context){
        .trace = &fixture->trace, .id = 3u, .prepare_return = 0, .stop_return = 0
    };

    fixture->provider_a_primary = (test_service_provider_context){&fixture->context_a, 0u};
    fixture->provider_a_secondary = (test_service_provider_context){&fixture->context_a, 1000u};
    fixture->provider_b = (test_service_provider_context){&fixture->context_b, 10u};
    fixture->provider_c = (test_asm_provider_context){7u, 0, 0u, 0u};

    fixture->math_interface = (test_checked_math_v1){
        1u,
        (uint32_t)sizeof(test_checked_math_v1),
        0u,
        u64_add_checked
    };
    fixture->a_primary_interface = (test_add_service_v1){
        {ARBOR_APPLICATION_SERVICE_INTERFACE_ABI_VERSION,
         (uint32_t)sizeof(test_add_service_v1),
         ARBOR_APPLICATION_SERVICE_INTERFACE_FLAGS_NONE},
        service_a_execute
    };
    fixture->a_secondary_interface = fixture->a_primary_interface;
    fixture->b_interface = (test_add_service_v1){
        {ARBOR_APPLICATION_SERVICE_INTERFACE_ABI_VERSION,
         (uint32_t)sizeof(test_add_service_v1),
         ARBOR_APPLICATION_SERVICE_INTERFACE_FLAGS_NONE},
        service_b_execute
    };
    fixture->c_interface = (test_scalar_service_v1){
        {ARBOR_APPLICATION_SERVICE_INTERFACE_ABI_VERSION,
         (uint32_t)sizeof(test_scalar_service_v1),
         ARBOR_APPLICATION_SERVICE_INTERFACE_FLAGS_NONE},
        af3_asm_typed_method
    };

    fixture->b_exports[0] = make_export(
        cap_b,
        &fixture->b_interface,
        (uint32_t)sizeof(fixture->b_interface),
        &fixture->provider_b);
    fixture->c_exports[0] = make_export(
        cap_c,
        &fixture->c_interface,
        (uint32_t)sizeof(fixture->c_interface),
        &fixture->provider_c);
    fixture->a_exports[0] = make_export(
        cap_a_primary,
        &fixture->a_primary_interface,
        (uint32_t)sizeof(fixture->a_primary_interface),
        &fixture->provider_a_primary);
    fixture->a_exports[1] = make_export(
        cap_a_secondary,
        &fixture->a_secondary_interface,
        (uint32_t)sizeof(fixture->a_secondary_interface),
        &fixture->provider_a_secondary);
    fixture->math_exports[0] = make_export(
        cap_math,
        &fixture->math_interface,
        (uint32_t)sizeof(fixture->math_interface),
        NULL);

    fixture->b_requires[0] = make_requirement(cap_a_primary, (uint32_t)sizeof(test_add_service_v1));
    fixture->a_requires[0] = make_requirement(cap_math, (uint32_t)sizeof(test_checked_math_v1));

    /* Deliberately scrambled: B -> A -> math; C and math are initially ready. */
    fixture->modules[0] = make_module(module_b, fixture->b_exports, 1u, fixture->b_requires, 1u);
    fixture->modules[1] = make_module(module_c, fixture->c_exports, 1u, NULL, 0u);
    fixture->modules[2] = make_module(module_a, fixture->a_exports, 2u, fixture->a_requires, 1u);
    fixture->modules[3] = make_module(module_math, fixture->math_exports, 1u, NULL, 0u);

    fixture->catalog_storage = (arbor_capability_catalog_storage){
        fixture->bindings,
        5u,
        fixture->resolutions,
        2u,
        fixture->published_order,
        4u
    };
    fixture->capability_workspace = (arbor_capability_workspace){
        fixture->resolved_provider_modules,
        fixture->resolved_binding_indices,
        2u,
        fixture->indegree,
        fixture->selected,
        fixture->workspace_order,
        4u
    };

    arbor_status status = arbor_capability_catalog_prepare(
        fixture->modules,
        4u,
        &fixture->catalog_storage,
        &fixture->capability_workspace,
        &fixture->catalog);
    if (status.native != 0 || arbor_capability_catalog_validate(&fixture->catalog).native != 0) {
        return fail("AF3 fixture AF2 catalog preparation");
    }
    if (fixture->published_order[0] != 1u || fixture->published_order[1] != 3u ||
        fixture->published_order[2] != 2u || fixture->published_order[3] != 0u) {
        return fail("AF3 fixture canonical AF2 tie-break order");
    }

    fixture->b_service_indices[0] = 0u;
    fixture->a_service_indices[0] = 0u;
    fixture->a_service_indices[1] = 1u;
    fixture->c_service_indices[0] = 0u;

    /* Deliberately not lifecycle order: B, A, C. */
    fixture->service_modules[0] = (arbor_application_service_module_descriptor){
        module_b,
        ARBOR_APPLICATION_SERVICE_MODULE_DESCRIPTOR_ABI_VERSION,
        (uint32_t)sizeof(arbor_application_service_module_descriptor),
        ARBOR_APPLICATION_SERVICE_MODULE_FLAGS_NONE,
        &fixture->context_b,
        fixture->b_service_indices,
        1u,
        module_prepare,
        module_rollback,
        module_stop
    };
    fixture->service_modules[1] = (arbor_application_service_module_descriptor){
        module_a,
        ARBOR_APPLICATION_SERVICE_MODULE_DESCRIPTOR_ABI_VERSION,
        (uint32_t)sizeof(arbor_application_service_module_descriptor),
        ARBOR_APPLICATION_SERVICE_MODULE_FLAGS_NONE,
        &fixture->context_a,
        fixture->a_service_indices,
        2u,
        module_prepare,
        module_rollback,
        module_stop
    };
    fixture->service_modules[2] = (arbor_application_service_module_descriptor){
        module_c,
        ARBOR_APPLICATION_SERVICE_MODULE_DESCRIPTOR_ABI_VERSION,
        (uint32_t)sizeof(arbor_application_service_module_descriptor),
        ARBOR_APPLICATION_SERVICE_MODULE_FLAGS_NONE,
        &fixture->context_c,
        fixture->c_service_indices,
        1u,
        af3_asm_prepare,
        af3_asm_rollback,
        af3_asm_stop
    };

    return 0;
}

static int verify_trace_prefix(const test_trace *trace, const uint64_t *expected, uint64_t count)
{
    if (trace->count < count) {
        return fail("AF3 lifecycle trace too short");
    }
    for (uint64_t i = 0u; i < count; ++i) {
        if (trace->events[i] != expected[i]) {
            return fail("AF3 lifecycle trace order mismatch");
        }
    }
    return 0;
}

static int test_zero_managed_runtime(const arbor_capability_catalog *catalog)
{
    arbor_application_runtime_requirements requirements = {9u, 9u};
    arbor_status status = arbor_application_runtime_measure(catalog, NULL, 0u, &requirements);
    if (status.native != 0 || requirements.managed_module_record_count != 0u ||
        requirements.module_map_count != 0u) {
        return fail("AF3 zero-managed runtime measurement");
    }

    arbor_application_runtime_storage storage = {NULL, 0u};
    arbor_application_runtime_workspace workspace = {NULL, 0u, NULL, 0u};
    arbor_application_runtime runtime = {9u, 9u, 9u, catalog, NULL, 9u, NULL, 9u, 9u};

    status = arbor_application_runtime_prepare(
        catalog, NULL, 0u, &storage, &workspace, &runtime);
    if (status.native != 0 || arbor_application_runtime_validate(&runtime).native != 0 ||
        runtime.state != ARBOR_APPLICATION_RUNTIME_READY || runtime.managed_module_count != 0u ||
        runtime.service_modules != NULL || runtime.records != NULL) {
        return fail("AF3 zero-managed READY runtime");
    }

    status = arbor_application_runtime_stop(&runtime);
    if (status.native != 0 || runtime.state != ARBOR_APPLICATION_RUNTIME_STOPPED ||
        arbor_application_runtime_validate(&runtime).native != 0) {
        return fail("AF3 zero-managed stop");
    }
    if (arbor_application_runtime_stop(&runtime).native != -EALREADY) {
        return fail("AF3 zero-managed repeated stop rejection");
    }
    return 0;
}

int main(void)
{
    if (sizeof(arbor_application_service_interface_header) != 16u ||
        sizeof(arbor_application_service_module_descriptor) != 80u ||
        sizeof(arbor_application_service_runtime_record) != 32u ||
        sizeof(arbor_application_runtime_requirements) != 16u ||
        sizeof(arbor_application_runtime_storage) != 16u ||
        sizeof(arbor_application_runtime_workspace) != 32u ||
        sizeof(arbor_application_runtime) != 56u) {
        return fail("AF3 frozen public layout sizes");
    }

    if (arbor_application_service_status_from_native(0).native != 0 ||
        arbor_application_service_status_from_native(-EIO).native != -EIO ||
        arbor_application_service_status_from_native(7).native != -EINVAL) {
        return fail("AF3 native status normalization");
    }

    test_fixture fixture;
    if (fixture_prepare_catalog(&fixture) != 0) {
        return 1;
    }

    if (fixture.provider_c.calls != 0u ||
        (void *)&fixture.context_c == (void *)&fixture.provider_c) {
        return fail("AF3 module/provider context separation precondition");
    }

    arbor_application_runtime_requirements requirements = {UINT64_C(99), UINT64_C(99)};
    arbor_status status = arbor_application_runtime_measure(
        &fixture.catalog,
        fixture.service_modules,
        3u,
        &requirements);
    if (status.native != 0 || requirements.managed_module_record_count != 3u ||
        requirements.module_map_count != 4u) {
        return fail("AF3 deterministic runtime measurement");
    }

    arbor_application_service_runtime_record persistent_records[3] = {
        {9u, 9u, 9u, 9u, 9}, {9u, 9u, 9u, 9u, 9}, {9u, 9u, 9u, 9u, 9}
    };
    arbor_application_service_runtime_record workspace_records[3] = {0};
    uint64_t module_map[4] = {0u, 0u, 0u, 0u};
    arbor_application_runtime_storage storage = {persistent_records, 3u};
    arbor_application_runtime_workspace workspace = {workspace_records, 3u, module_map, 4u};
    arbor_application_runtime runtime = {9u, 9u, 9u, &fixture.catalog, NULL, 9u, NULL, 9u, 9u};

    status = arbor_application_runtime_prepare(
        &fixture.catalog,
        fixture.service_modules,
        3u,
        &storage,
        &workspace,
        &runtime);
    if (status.native != 0 || arbor_application_runtime_validate(&runtime).native != 0) {
        return fail("AF3 full runtime preparation");
    }

    const uint64_t expected_prepare[] = {103u, 101u, 102u};
    if (verify_trace_prefix(&fixture.trace, expected_prepare, 3u) != 0) {
        return 1;
    }
    if (persistent_records[0].module_index != 1u || persistent_records[0].descriptor_index != 2u ||
        persistent_records[1].module_index != 2u || persistent_records[1].descriptor_index != 1u ||
        persistent_records[2].module_index != 0u || persistent_records[2].descriptor_index != 0u) {
        return fail("AF3 filtered canonical lifecycle record order");
    }
    if (!fixture.context_a.prepared || !fixture.context_b.prepared ||
        fixture.context_c.prepare_calls != 1u || fixture.context_c.stack_errors != 0u) {
        return fail("AF3 active C/Assembly module preparation state");
    }
    if (!capability_id_equal(fixture.context_a.cached_dependency.id, cap_math) ||
        !capability_id_equal(fixture.context_b.cached_dependency.id, cap_a_primary)) {
        return fail("AF3 prepare-time cached dependency bindings");
    }

    arbor_capability_binding binding = {0};
    status = arbor_application_runtime_find_ready(
        &runtime, cap_a_primary, (arbor_capability_version){1u, 0u},
        (uint32_t)sizeof(test_add_service_v1), &binding);
    if (status.native != 0 || binding.provider_context != &fixture.provider_a_primary) {
        return fail("AF3 ready primary typed service lookup");
    }
    const test_add_service_v1 *service_a = (const test_add_service_v1 *)binding.interface_table;

    arbor_capability_binding secondary = {0};
    status = arbor_application_runtime_find_ready(
        &runtime, cap_a_secondary, (arbor_capability_version){1u, 0u},
        (uint32_t)sizeof(test_add_service_v1), &secondary);
    if (status.native != 0 || secondary.provider_context != &fixture.provider_a_secondary ||
        secondary.provider_context == binding.provider_context) {
        return fail("AF3 one module exports multiple typed services with distinct provider contexts");
    }

    arbor_capability_binding b_binding = {0};
    status = arbor_application_runtime_find_ready(
        &runtime, cap_b, (arbor_capability_version){1u, 0u},
        (uint32_t)sizeof(test_add_service_v1), &b_binding);
    if (status.native != 0 || b_binding.provider_context != &fixture.provider_b) {
        return fail("AF3 ready dependent typed service lookup");
    }
    const test_add_service_v1 *service_b = (const test_add_service_v1 *)b_binding.interface_table;

    test_add_command command = {40u, 2u, 100u};
    test_add_result result = {0u, 0u, 0u};
    int64_t native = service_a->execute(binding.provider_context, &command, &result);
    if (native != 0 || result.kind != TEST_ADD_APPLIED || result.value != 42u) {
        return fail("AF3 typed service successful application outcome");
    }

    command = (test_add_command){80u, 30u, 100u};
    result = (test_add_result){0u, 0u, 0u};
    native = service_a->execute(binding.provider_context, &command, &result);
    if (native != 0 || result.kind != TEST_ADD_LIMIT_EXCEEDED || result.value != 110u) {
        return fail("AF3 business rejection remains typed non-errno outcome");
    }

    command = (test_add_command){UINT64_MAX, 1u, UINT64_MAX};
    test_add_result unchanged = {77u, 88u, UINT64_C(99)};
    result = unchanged;
    native = service_a->execute(binding.provider_context, &command, &result);
    if (native != -EOVERFLOW || memcmp(&result, &unchanged, sizeof(result)) != 0) {
        return fail("AF3 typed result output unchanged on mechanism failure");
    }

    command = (test_add_command){1u, 2u, 100u};
    for (uint64_t i = 0u; i < UINT64_C(1000); ++i) {
        result = (test_add_result){0u, 0u, 0u};
        native = service_b->execute(b_binding.provider_context, &command, &result);
        if (native != 0 || result.kind != TEST_ADD_APPLIED || result.value != 13u) {
            return fail("AF3 repeated cached typed service hot-path calls");
        }
    }

    arbor_capability_binding c_binding = {0};
    status = arbor_application_runtime_find_ready(
        &runtime, cap_c, (arbor_capability_version){1u, 0u},
        (uint32_t)sizeof(test_scalar_service_v1), &c_binding);
    if (status.native != 0 || c_binding.provider_context != &fixture.provider_c) {
        return fail("AF3 Assembly typed service discovery");
    }
    const test_scalar_service_v1 *service_c =
        (const test_scalar_service_v1 *)c_binding.interface_table;
    test_scalar_input scalar_input = {5u};
    test_scalar_output scalar_output = {UINT64_C(999)};
    native = service_c->execute(c_binding.provider_context, &scalar_input, &scalar_output);
    if (native != 0 || scalar_output.value != 12u || fixture.provider_c.calls != 1u ||
        fixture.provider_c.stack_errors != 0u) {
        return fail("AF3 C-to-Assembly typed SysV call");
    }

    fixture.provider_c.native_return = 9;
    scalar_output.value = UINT64_C(777);
    native = service_c->execute(c_binding.provider_context, &scalar_input, &scalar_output);
    if (native != 9 || scalar_output.value != UINT64_C(777) ||
        arbor_application_service_status_from_native(native).native != -EINVAL) {
        return fail("AF3 positive Assembly typed return rejection and output atomicity");
    }
    fixture.provider_c.native_return = 0;

    test_asm_provider_context c_provider = {20u, 0, 0u, 0u};
    scalar_output.value = 0u;
    native = af3_asm_call_c_typed(c_scalar_execute, &c_provider, &scalar_input, &scalar_output);
    if (native != 0 || scalar_output.value != 25u || c_provider.calls != 1u) {
        return fail("AF3 Assembly-to-C typed SysV call");
    }
    scalar_output.value = 0u;
    native = af3_asm_call_c_typed_preserve(
        c_scalar_execute, &c_provider, &scalar_input, &scalar_output);
    if (native != 0 || scalar_output.value != 25u || c_provider.calls != 2u) {
        return fail("AF3 C typed callee-saved register compliance under Assembly caller");
    }

    status = arbor_application_runtime_stop(&runtime);
    if (status.native != 0 || runtime.state != ARBOR_APPLICATION_RUNTIME_STOPPED ||
        arbor_application_runtime_validate(&runtime).native != 0) {
        return fail("AF3 reverse successful shutdown");
    }
    const uint64_t expected_all[] = {103u, 101u, 102u, 302u, 301u, 303u};
    if (fixture.trace.count != 6u || verify_trace_prefix(&fixture.trace, expected_all, 6u) != 0) {
        return fail("AF3 preparation/shutdown exact lifecycle trace");
    }
    if (fixture.context_c.stop_calls != 1u || fixture.context_c.stack_errors != 0u) {
        return fail("AF3 C-to-Assembly stop ABI");
    }

    arbor_capability_binding unchanged_binding = binding;
    status = arbor_application_runtime_find_ready(
        &runtime, cap_a_primary, (arbor_capability_version){1u, 0u},
        (uint32_t)sizeof(test_add_service_v1), &binding);
    if (status.native != -EAGAIN || memcmp(&binding, &unchanged_binding, sizeof(binding)) != 0) {
        return fail("AF3 ready lookup rejected after stop with transactional output");
    }
    if (arbor_application_runtime_stop(&runtime).native != -EALREADY) {
        return fail("AF3 stop retry prohibited");
    }

    if (test_zero_managed_runtime(&fixture.catalog) != 0) {
        return 1;
    }

    puts("PASS: AF3 deterministic Application-service runtime, typed use cases, lifecycle and C/Assembly ABI");
    return 0;
}
