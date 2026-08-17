#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <arborcore/ddd_support.h>

typedef struct adversarial_context {
    int64_t begin_result;
    int64_t commit_result;
    int64_t rollback_result;
} adversarial_context;

static int failures = 0;

#define CHECK(expr) \
    do { \
        if (!(expr)) { \
            (void)fprintf(stderr, "FAIL:%s:%d: %s\n", __FILE__, __LINE__, #expr); \
            failures += 1; \
        } \
    } while (0)

static int status_is(arbor_status status, int64_t native)
{
    return status.native == native;
}

static int64_t adv_begin(
    void *provider_context,
    void *transaction_state,
    uint64_t transaction_state_size)
{
    adversarial_context *context = (adversarial_context *)provider_context;
    if (transaction_state != NULL && transaction_state_size >= UINT64_C(8)) {
        *(uint64_t *)transaction_state = UINT64_C(0x99);
    }
    return context->begin_result;
}

static int64_t adv_commit(void *provider_context, void *transaction_state)
{
    adversarial_context *context = (adversarial_context *)provider_context;
    (void)transaction_state;
    return context->commit_result;
}

static int64_t adv_rollback(void *provider_context, void *transaction_state)
{
    adversarial_context *context = (adversarial_context *)provider_context;
    (void)transaction_state;
    return context->rollback_result;
}

static arbor_ddd_transaction_interface valid_interface(void)
{
    arbor_ddd_transaction_interface interface_table;
    (void)memset(&interface_table, 0, sizeof(interface_table));
    interface_table.abi_version = ARBOR_DDD_TRANSACTION_INTERFACE_ABI_VERSION;
    interface_table.struct_size = (uint32_t)sizeof(interface_table);
    interface_table.authority_id.high = UINT64_C(1);
    interface_table.authority_id.low = UINT64_C(2);
    interface_table.state_size = UINT64_C(64);
    interface_table.state_alignment = UINT64_C(64);
    interface_table.begin = adv_begin;
    interface_table.commit = adv_commit;
    interface_table.rollback = adv_rollback;
    return interface_table;
}

static arbor_capability_binding valid_binding(
    const arbor_ddd_transaction_interface *interface_table,
    adversarial_context *context)
{
    arbor_capability_binding binding;
    (void)memset(&binding, 0, sizeof(binding));
    binding.id.high = UINT64_C(10);
    binding.id.low = UINT64_C(20);
    binding.version.major = 1u;
    binding.interface_size = (uint32_t)sizeof(*interface_table);
    binding.interface_table = interface_table;
    binding.provider_context = context;
    return binding;
}

static void test_journal_adversarial(void)
{
    arbor_ddd_event_record records[2];
    uint8_t payload_bytes[8];
    arbor_ddd_event_journal journal;
    arbor_ddd_event_checkpoint checkpoint;
    arbor_ddd_event_view view;
    arbor_ddd_event_type_id type_id = {UINT64_C(1), UINT64_C(3)};
    const uint8_t payload[4] = {1u, 2u, 3u, 4u};
    uint32_t sequence = UINT32_C(0xdeadbeef);

    CHECK(status_is(arbor_ddd_event_journal_validate(NULL), -EINVAL));
    CHECK(status_is(
        arbor_ddd_event_journal_init(NULL, 1u, NULL, 0u, &journal), -EINVAL));
    CHECK(status_is(
        arbor_ddd_event_journal_init(records, 0u, NULL, 0u, &journal), -EINVAL));
    CHECK(status_is(
        arbor_ddd_event_journal_init(NULL, 0u, payload_bytes, 0u, &journal), -EINVAL));

    arbor_ddd_event_journal empty;
    (void)memset(&empty, 0, sizeof(empty));
    CHECK(status_is(
        arbor_ddd_event_journal_init(NULL, 0u, NULL, 0u, &empty), 0));
    CHECK(status_is(arbor_ddd_event_journal_validate(&empty), 0));

    uint8_t misaligned_records[sizeof(arbor_ddd_event_record) + 1u];
    CHECK(status_is(
        arbor_ddd_event_journal_init(
            (arbor_ddd_event_record *)(void *)(misaligned_records + 1u),
            1u,
            NULL,
            0u,
            &journal),
        -EINVAL));

    (void)memset(&journal, 0, sizeof(journal));
    CHECK(status_is(
        arbor_ddd_event_journal_init(records, 2u, payload_bytes, 8u, &journal), 0));

    CHECK(status_is(
        arbor_ddd_event_journal_append(
            NULL, type_id, 1u, 0u, payload, 4u, &sequence), -EINVAL));

    arbor_ddd_event_type_id zero_id = {UINT64_C(0), UINT64_C(0)};
    CHECK(status_is(
        arbor_ddd_event_journal_append(
            &journal, zero_id, 1u, 0u, payload, 4u, &sequence), -EINVAL));
    CHECK(status_is(
        arbor_ddd_event_journal_append(
            &journal, type_id, 0u, 0u, payload, 4u, &sequence), -EINVAL));
    CHECK(status_is(
        arbor_ddd_event_journal_append(
            &journal, type_id, 1u, 1u, payload, 4u, &sequence), -EINVAL));
    CHECK(status_is(
        arbor_ddd_event_journal_append(
            &journal, type_id, 1u, 0u, NULL, 1u, &sequence), -EINVAL));
    CHECK(status_is(
        arbor_ddd_event_journal_append(
            &journal, type_id, 1u, 0u, payload, 4u, NULL), -EINVAL));

    uint32_t payload_and_sequence = UINT32_C(0x04030201);
    CHECK(status_is(
        arbor_ddd_event_journal_append(
            &journal,
            type_id,
            1u,
            0u,
            &payload_and_sequence,
            (uint32_t)sizeof(payload_and_sequence),
            &payload_and_sequence),
        -EINVAL));

    uint32_t old_records = journal.record_count;
    uint32_t old_bytes = journal.byte_count;
    CHECK(status_is(
        arbor_ddd_event_journal_append(
            &journal,
            type_id,
            1u,
            0u,
            payload,
            4u,
            &journal.record_count),
        -EINVAL));
    CHECK(journal.record_count == old_records);
    CHECK(journal.byte_count == old_bytes);

    CHECK(status_is(
        arbor_ddd_event_journal_append(
            &journal, type_id, 1u, 0u, payload, 4u, &sequence), 0));
    CHECK(sequence == 0u);

    arbor_ddd_event_checkpoint fabricated = {
        .record_count = 1u,
        .byte_count = 0u,
        .generation = journal.generation
    };
    old_records = journal.record_count;
    old_bytes = journal.byte_count;
    CHECK(status_is(arbor_ddd_event_journal_rewind(&journal, &fabricated), -EINVAL));
    CHECK(journal.record_count == old_records);
    CHECK(journal.byte_count == old_bytes);

    fabricated.record_count = 0u;
    fabricated.byte_count = journal.byte_count;
    CHECK(status_is(arbor_ddd_event_journal_rewind(&journal, &fabricated), -EINVAL));
    CHECK(journal.record_count == old_records);
    CHECK(journal.byte_count == old_bytes);

    const uint32_t saved_offset = journal.records[0].payload_offset;
    const uint32_t saved_size = journal.records[0].payload_size;
    journal.records[0].payload_offset = 1u;
    journal.records[0].payload_size = 3u;
    CHECK(status_is(arbor_ddd_event_journal_validate(&journal), -EINVAL));
    CHECK(status_is(arbor_ddd_event_journal_checkpoint(&journal, &checkpoint), -EINVAL));
    fabricated.record_count = 1u;
    fabricated.byte_count = 4u;
    fabricated.generation = journal.generation;
    CHECK(status_is(arbor_ddd_event_journal_rewind(&journal, &fabricated), -EINVAL));
    journal.records[0].payload_offset = saved_offset;
    journal.records[0].payload_size = saved_size;
    CHECK(status_is(arbor_ddd_event_journal_validate(&journal), 0));

    old_records = journal.record_count;
    old_bytes = journal.byte_count;
    CHECK(status_is(
        arbor_ddd_event_journal_append(
            &journal,
            type_id,
            1u,
            0u,
            journal.payload_bytes,
            1u,
            &sequence),
        -EINVAL));
    CHECK(journal.record_count == old_records);
    CHECK(journal.byte_count == old_bytes);

    CHECK(status_is(
        arbor_ddd_event_journal_append(
            &journal, type_id, 1u, 0u, payload, 4u, &sequence), 0));
    CHECK(journal.record_count == 2u);
    CHECK(journal.byte_count == 8u);

    old_records = journal.record_count;
    old_bytes = journal.byte_count;
    sequence = UINT32_C(0x12345678);
    CHECK(status_is(
        arbor_ddd_event_journal_append(
            &journal, type_id, 1u, 0u, NULL, 0u, &sequence), -ENOSPC));
    CHECK(sequence == UINT32_C(0x12345678));
    CHECK(journal.record_count == old_records);
    CHECK(journal.byte_count == old_bytes);

    CHECK(status_is(arbor_ddd_event_journal_checkpoint(&journal, &checkpoint), 0));
    CHECK(status_is(
        arbor_ddd_event_journal_checkpoint(
            &journal,
            (arbor_ddd_event_checkpoint *)(void *)&journal),
        -EINVAL));

    CHECK(status_is(arbor_ddd_event_journal_view(&journal, 2u, &view), -ENOENT));
    CHECK(status_is(
        arbor_ddd_event_journal_view(
            &journal, 0u, (arbor_ddd_event_view *)(void *)&journal),
        -EINVAL));

    arbor_ddd_event_checkpoint invalid_checkpoint = checkpoint;
    invalid_checkpoint.record_count += 1u;
    CHECK(status_is(
        arbor_ddd_event_journal_rewind(&journal, &invalid_checkpoint), -EINVAL));

    CHECK(status_is(arbor_ddd_event_journal_clear(&journal), 0));
    CHECK(status_is(arbor_ddd_event_journal_rewind(&journal, &checkpoint), -ESTALE));

    journal.generation = UINT64_MAX;
    old_records = journal.record_count;
    old_bytes = journal.byte_count;
    CHECK(status_is(arbor_ddd_event_journal_clear(&journal), -EOVERFLOW));
    CHECK(journal.generation == UINT64_MAX);
    CHECK(journal.record_count == old_records);
    CHECK(journal.byte_count == old_bytes);

    journal.generation = UINT64_C(5);
    journal.record_count = 3u;
    CHECK(status_is(arbor_ddd_event_journal_validate(&journal), -EINVAL));
}

static void test_transaction_validation(void)
{
    adversarial_context context;
    arbor_ddd_transaction_interface interface_table = valid_interface();
    arbor_capability_binding binding = valid_binding(&interface_table, &context);
    _Alignas(64) uint8_t state[65];
    arbor_ddd_transaction_view view;

    (void)memset(&context, 0, sizeof(context));
    CHECK(status_is(arbor_ddd_transaction_interface_validate(NULL), -EINVAL));
    CHECK(status_is(arbor_ddd_transaction_interface_validate(&binding), 0));

    arbor_capability_binding bad_binding = binding;
    bad_binding.id.high = 0u;
    bad_binding.id.low = 0u;
    CHECK(status_is(arbor_ddd_transaction_interface_validate(&bad_binding), -EINVAL));

    bad_binding = binding;
    bad_binding.interface_size = 8u;
    CHECK(status_is(arbor_ddd_transaction_interface_validate(&bad_binding), -EINVAL));

    arbor_ddd_transaction_interface bad_interface = interface_table;
    bad_interface.abi_version = 99u;
    bad_binding = binding;
    bad_binding.interface_table = &bad_interface;
    CHECK(status_is(
        arbor_ddd_transaction_interface_validate(&bad_binding),
        -EPROTONOSUPPORT));

    bad_interface = interface_table;
    bad_interface.struct_size += 8u;
    bad_binding = binding;
    bad_binding.interface_table = &bad_interface;
    CHECK(status_is(arbor_ddd_transaction_interface_validate(&bad_binding), -EINVAL));

    bad_interface = interface_table;
    bad_interface.flags = UINT64_C(1);
    bad_binding = binding;
    bad_binding.interface_table = &bad_interface;
    CHECK(status_is(arbor_ddd_transaction_interface_validate(&bad_binding), -EINVAL));

    bad_interface = interface_table;
    bad_interface.authority_id.high = 0u;
    bad_interface.authority_id.low = 0u;
    bad_binding = binding;
    bad_binding.interface_table = &bad_interface;
    CHECK(status_is(arbor_ddd_transaction_interface_validate(&bad_binding), -EINVAL));

    bad_interface = interface_table;
    bad_interface.state_alignment = UINT64_C(3);
    bad_binding = binding;
    bad_binding.interface_table = &bad_interface;
    CHECK(status_is(arbor_ddd_transaction_interface_validate(&bad_binding), -EINVAL));

    bad_interface = interface_table;
    bad_interface.state_alignment = UINT64_C(8192);
    bad_binding = binding;
    bad_binding.interface_table = &bad_interface;
    CHECK(status_is(arbor_ddd_transaction_interface_validate(&bad_binding), -EINVAL));

    bad_interface = interface_table;
    bad_interface.commit = NULL;
    bad_binding = binding;
    bad_binding.interface_table = &bad_interface;
    CHECK(status_is(arbor_ddd_transaction_interface_validate(&bad_binding), -EINVAL));

    bad_binding = binding;
    bad_binding.provider_context = (void *)&interface_table;
    CHECK(status_is(arbor_ddd_transaction_interface_validate(&bad_binding), -EINVAL));

    view.interface_table = &interface_table;
    view.provider_context = &context;
    view.transaction_state = state;
    view.transaction_state_size = UINT64_C(64);
    CHECK(status_is(arbor_ddd_transaction_view_validate(&view), 0));

    view.transaction_state = state + 1u;
    CHECK(status_is(arbor_ddd_transaction_view_validate(&view), -EINVAL));
    view.transaction_state = state;
    view.transaction_state_size = UINT64_C(63);
    CHECK(status_is(arbor_ddd_transaction_view_validate(&view), -EINVAL));

    view.transaction_state_size = UINT64_C(64);
    view.provider_context = state;
    CHECK(status_is(arbor_ddd_transaction_view_validate(&view), -EINVAL));
    view.provider_context = &view;
    CHECK(status_is(arbor_ddd_transaction_view_validate(&view), -EINVAL));
    view.provider_context = &context;

    union {
        _Alignas(64) uint8_t bytes[64];
        arbor_ddd_transaction_view view;
    } overlapping_view_state;
    (void)memset(&overlapping_view_state, 0, sizeof(overlapping_view_state));
    overlapping_view_state.view.interface_table = &interface_table;
    overlapping_view_state.view.provider_context = &context;
    overlapping_view_state.view.transaction_state = overlapping_view_state.bytes;
    overlapping_view_state.view.transaction_state_size = UINT64_C(64);
    CHECK(status_is(
        arbor_ddd_transaction_view_validate(&overlapping_view_state.view),
        -EINVAL));

    _Alignas(64) arbor_ddd_transaction_interface aligned_interface =
        valid_interface();
    view.interface_table = &aligned_interface;
    view.provider_context = &context;
    view.transaction_state = &aligned_interface;
    view.transaction_state_size = UINT64_C(64);
    CHECK(status_is(arbor_ddd_transaction_view_validate(&view), -EINVAL));
}

static void test_uow_adversarial(void)
{
    arbor_ddd_event_record records[4];
    uint8_t payload_bytes[32];
    arbor_ddd_event_journal journal;
    _Alignas(64) uint8_t state[65];
    adversarial_context context;
    arbor_ddd_transaction_interface interface_table = valid_interface();
    arbor_capability_binding binding = valid_binding(&interface_table, &context);
    arbor_ddd_unit_of_work uow;
    arbor_ddd_transaction_view active;
    arbor_ddd_event_type_id type_id = {UINT64_C(5), UINT64_C(6)};
    const uint8_t payload = 0x7au;
    uint32_t sequence = 0u;

    (void)memset(&context, 0, sizeof(context));
    (void)memset(&journal, 0, sizeof(journal));
    (void)memset(&uow, 0, sizeof(uow));
    CHECK(status_is(
        arbor_ddd_event_journal_init(records, 4u, payload_bytes, 32u, &journal), 0));

    CHECK(status_is(
        arbor_ddd_unit_of_work_begin(
            &binding, state + 1u, UINT64_C(64), &journal, &uow),
        -EINVAL));
    CHECK(uow.state == (uint32_t)ARBOR_DDD_UNIT_OF_WORK_ZERO);

    CHECK(status_is(
        arbor_ddd_unit_of_work_begin(
            &binding, state, UINT64_C(63), &journal, &uow),
        -EINVAL));

    arbor_ddd_unit_of_work nonzero = uow;
    nonzero.state = (uint32_t)ARBOR_DDD_UNIT_OF_WORK_COMMITTED;
    CHECK(status_is(
        arbor_ddd_unit_of_work_begin(
            &binding, state, UINT64_C(64), &journal, &nonzero),
        -EINVAL));

    arbor_capability_binding aliased_context_binding = binding;
    aliased_context_binding.provider_context = &journal;
    CHECK(status_is(
        arbor_ddd_unit_of_work_begin(
            &aliased_context_binding, state, UINT64_C(64), &journal, &uow),
        -EINVAL));

    context.begin_result = 1;
    CHECK(status_is(
        arbor_ddd_unit_of_work_begin(
            &binding, state, UINT64_C(64), &journal, &uow),
        -EINVAL));
    CHECK(uow.state == (uint32_t)ARBOR_DDD_UNIT_OF_WORK_ZERO);

    context.begin_result = 0;
    CHECK(status_is(
        arbor_ddd_event_journal_append(
            &journal, type_id, 1u, 0u, &payload, 1u, &sequence),
        0));
    const uint32_t original_offset = journal.records[0].payload_offset;
    const uint32_t original_size = journal.records[0].payload_size;
    journal.records[0].payload_offset = 1u;
    journal.records[0].payload_size = 0u;
    CHECK(status_is(
        arbor_ddd_unit_of_work_begin(
            &binding, state, UINT64_C(64), &journal, &uow),
        -EINVAL));
    CHECK(uow.state == (uint32_t)ARBOR_DDD_UNIT_OF_WORK_ZERO);
    journal.records[0].payload_offset = original_offset;
    journal.records[0].payload_size = original_size;
    CHECK(status_is(arbor_ddd_event_journal_clear(&journal), 0));

    CHECK(status_is(
        arbor_ddd_unit_of_work_begin(
            &binding, state, UINT64_C(64), &journal, &uow),
        0));
    CHECK(status_is(
        arbor_ddd_unit_of_work_begin(
            &binding, state, UINT64_C(64), &journal, &uow),
        -EINVAL));

    CHECK(status_is(
        arbor_ddd_unit_of_work_active_transaction(
            &uow, (arbor_ddd_transaction_view *)(void *)&uow),
        -EINVAL));
    CHECK(status_is(
        arbor_ddd_unit_of_work_active_transaction(
            &uow, (arbor_ddd_transaction_view *)(void *)&context),
        -EINVAL));

    CHECK(status_is(
        arbor_ddd_event_journal_append(
            &journal, type_id, 1u, 0u, &payload, 1u, &sequence),
        0));

    arbor_ddd_event_checkpoint pre_clear = uow.event_checkpoint;
    CHECK(status_is(arbor_ddd_event_journal_clear(&journal), 0));
    CHECK(status_is(arbor_ddd_unit_of_work_commit(&uow), -ESTALE));
    CHECK(uow.state == (uint32_t)ARBOR_DDD_UNIT_OF_WORK_ACTIVE);

    journal.generation = pre_clear.generation;
    journal.record_count = pre_clear.record_count;
    journal.byte_count = pre_clear.byte_count;

    context.commit_result = 1;
    CHECK(status_is(arbor_ddd_unit_of_work_commit(&uow), -EINVAL));
    CHECK(uow.state == (uint32_t)ARBOR_DDD_UNIT_OF_WORK_FAULTED);
    CHECK(journal.record_count == pre_clear.record_count);
    CHECK(status_is(arbor_ddd_unit_of_work_commit(&uow), -EALREADY));
    CHECK(status_is(arbor_ddd_unit_of_work_rollback(&uow), -EALREADY));

    uow.event_journal = NULL;
    uow.transaction_view.interface_table = NULL;
    uow.transaction_view.transaction_state = NULL;
    CHECK(status_is(arbor_ddd_unit_of_work_reset(&uow), 0));

    context.commit_result = 0;
    context.rollback_result = 1;
    CHECK(status_is(
        arbor_ddd_unit_of_work_begin(
            &binding, state, UINT64_C(64), &journal, &uow),
        0));
    CHECK(status_is(
        arbor_ddd_event_journal_append(
            &journal, type_id, 1u, 0u, &payload, 1u, &sequence),
        0));
    CHECK(status_is(arbor_ddd_unit_of_work_rollback(&uow), -EINVAL));
    CHECK(uow.state == (uint32_t)ARBOR_DDD_UNIT_OF_WORK_FAULTED);
    CHECK(status_is(arbor_ddd_unit_of_work_reset(&uow), 0));

    CHECK(status_is(arbor_ddd_unit_of_work_commit(&uow), -EINVAL));
    CHECK(status_is(arbor_ddd_unit_of_work_rollback(&uow), -EINVAL));
    CHECK(status_is(arbor_ddd_unit_of_work_active_transaction(&uow, &active), -EINVAL));
    CHECK(status_is(arbor_ddd_unit_of_work_reset(&uow), -EINVAL));

    (void)memset(&uow, 0, sizeof(uow));
    CHECK(status_is(
        arbor_ddd_unit_of_work_begin(
            &binding,
            (void *)&uow,
            UINT64_C(64),
            &journal,
            &uow),
        -EINVAL));
}

int main(void)
{
    test_journal_adversarial();
    test_transaction_validation();
    test_uow_adversarial();

    if (failures != 0) {
        (void)fprintf(stderr, "AF4 adversarial failures=%d\n", failures);
        return 1;
    }

    (void)puts("PASS: AF4 DDD support adversarial");
    return 0;
}
