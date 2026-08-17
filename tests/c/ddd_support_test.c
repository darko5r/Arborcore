#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <arborcore/ddd_support.h>

typedef struct test_tx_context {
    uint64_t begin_calls;
    uint64_t commit_calls;
    uint64_t rollback_calls;
    int64_t begin_result;
    int64_t commit_result;
    int64_t rollback_result;
} test_tx_context;

typedef struct asm_tx_context {
    uint64_t begin_calls;
    uint64_t commit_calls;
    uint64_t rollback_calls;
    uint64_t expected_state_size;
    void *expected_state;
    uint64_t stack_misaligned;
} asm_tx_context;

extern int64_t af4_asm_transaction_begin(
    void *provider_context,
    void *transaction_state,
    uint64_t transaction_state_size);

extern int64_t af4_asm_transaction_commit(
    void *provider_context,
    void *transaction_state);

extern int64_t af4_asm_transaction_rollback(
    void *provider_context,
    void *transaction_state);

extern arbor_status af4_asm_checkpoint_call(
    const arbor_ddd_event_journal *journal,
    arbor_ddd_event_checkpoint *out);

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

static int64_t test_begin(
    void *provider_context,
    void *transaction_state,
    uint64_t transaction_state_size)
{
    test_tx_context *context = (test_tx_context *)provider_context;
    context->begin_calls += UINT64_C(1);
    if (transaction_state_size >= UINT64_C(8) && transaction_state != NULL) {
        *(uint64_t *)transaction_state = UINT64_C(0xaabbccddeeff0011);
    }
    return context->begin_result;
}

static int64_t test_commit(void *provider_context, void *transaction_state)
{
    test_tx_context *context = (test_tx_context *)provider_context;
    context->commit_calls += UINT64_C(1);
    if (transaction_state != NULL) {
        *(uint64_t *)transaction_state ^= UINT64_C(0x55);
    }
    return context->commit_result;
}

static int64_t test_rollback(void *provider_context, void *transaction_state)
{
    test_tx_context *context = (test_tx_context *)provider_context;
    context->rollback_calls += UINT64_C(1);
    if (transaction_state != NULL) {
        *(uint64_t *)transaction_state = UINT64_C(0);
    }
    return context->rollback_result;
}

static arbor_capability_binding make_binding(
    const arbor_ddd_transaction_interface *interface_table,
    void *provider_context)
{
    arbor_capability_binding binding;
    (void)memset(&binding, 0, sizeof(binding));
    binding.id.high = UINT64_C(0x100);
    binding.id.low = UINT64_C(0x200);
    binding.version.major = 1u;
    binding.version.minor = 0u;
    binding.interface_size = (uint32_t)sizeof(*interface_table);
    binding.interface_table = interface_table;
    binding.provider_context = provider_context;
    return binding;
}

static arbor_ddd_transaction_interface make_interface(
    arbor_ddd_transaction_begin_fn begin,
    arbor_ddd_transaction_commit_fn commit,
    arbor_ddd_transaction_rollback_fn rollback)
{
    arbor_ddd_transaction_interface interface_table;
    (void)memset(&interface_table, 0, sizeof(interface_table));
    interface_table.abi_version = ARBOR_DDD_TRANSACTION_INTERFACE_ABI_VERSION;
    interface_table.struct_size = (uint32_t)sizeof(interface_table);
    interface_table.flags = ARBOR_DDD_TRANSACTION_FLAGS_NONE;
    interface_table.authority_id.high = UINT64_C(0xabc);
    interface_table.authority_id.low = UINT64_C(0xdef);
    interface_table.state_size = UINT64_C(64);
    interface_table.state_alignment = UINT64_C(64);
    interface_table.begin = begin;
    interface_table.commit = commit;
    interface_table.rollback = rollback;
    return interface_table;
}

static void test_event_journal(void)
{
    arbor_ddd_event_record records[4];
    uint8_t bytes[64];
    arbor_ddd_event_journal journal;
    arbor_ddd_event_checkpoint checkpoint;
    arbor_ddd_event_view view;
    uint32_t sequence = UINT32_MAX;
    const uint8_t payload[] = {1u, 2u, 3u, 4u};
    arbor_ddd_event_type_id type_id = {
        UINT64_C(0x1111111111111111),
        UINT64_C(0x2222222222222222)
    };

    (void)memset(records, 0, sizeof(records));
    (void)memset(bytes, 0, sizeof(bytes));
    (void)memset(&journal, 0, sizeof(journal));

    CHECK(status_is(
        arbor_ddd_event_journal_init(records, 4u, bytes, 64u, &journal), 0));
    CHECK(journal.generation == UINT64_C(1));
    CHECK(status_is(arbor_ddd_event_journal_validate(&journal), 0));

    CHECK(status_is(arbor_ddd_event_journal_checkpoint(&journal, &checkpoint), 0));
    CHECK(checkpoint.record_count == 0u);
    CHECK(checkpoint.byte_count == 0u);
    CHECK(checkpoint.generation == UINT64_C(1));

    CHECK(status_is(
        arbor_ddd_event_journal_append(
            &journal,
            type_id,
            1u,
            ARBOR_DDD_EVENT_FLAGS_NONE,
            payload,
            (uint32_t)sizeof(payload),
            &sequence),
        0));
    CHECK(sequence == 0u);
    CHECK(journal.record_count == 1u);
    CHECK(journal.byte_count == (uint32_t)sizeof(payload));

    (void)memset(&view, 0, sizeof(view));
    CHECK(status_is(arbor_ddd_event_journal_view(&journal, 0u, &view), 0));
    CHECK(view.type_id.high == type_id.high);
    CHECK(view.type_id.low == type_id.low);
    CHECK(view.abi_version == 1u);
    CHECK(view.payload_size == (uint32_t)sizeof(payload));
    CHECK(view.sequence == 0u);
    CHECK(view.payload != NULL);
    CHECK(view.payload != payload);
    CHECK(memcmp(view.payload, payload, sizeof(payload)) == 0);

    CHECK(status_is(
        arbor_ddd_event_journal_append(
            &journal,
            type_id,
            2u,
            ARBOR_DDD_EVENT_FLAGS_NONE,
            NULL,
            0u,
            &sequence),
        0));
    CHECK(sequence == 1u);

    CHECK(status_is(arbor_ddd_event_journal_rewind(&journal, &checkpoint), 0));
    CHECK(journal.record_count == 0u);
    CHECK(journal.byte_count == 0u);

    CHECK(status_is(arbor_ddd_event_journal_clear(&journal), 0));
    CHECK(journal.generation == UINT64_C(2));
    CHECK(status_is(arbor_ddd_event_journal_rewind(&journal, &checkpoint), -ESTALE));

    arbor_ddd_event_checkpoint asm_checkpoint;
    (void)memset(&asm_checkpoint, 0, sizeof(asm_checkpoint));
    CHECK(status_is(af4_asm_checkpoint_call(&journal, &asm_checkpoint), 0));
    CHECK(asm_checkpoint.generation == journal.generation);
}

static void test_uow_success_and_rollback(void)
{
    arbor_ddd_event_record records[8];
    uint8_t bytes[128];
    arbor_ddd_event_journal journal;
    _Alignas(64) uint8_t state[64];
    test_tx_context context;
    arbor_ddd_transaction_interface interface_table;
    arbor_capability_binding binding;
    arbor_ddd_unit_of_work uow;
    arbor_ddd_transaction_view active;
    arbor_ddd_event_type_id event_type = {UINT64_C(7), UINT64_C(9)};
    uint32_t sequence = UINT32_MAX;
    uint8_t payload = 0x42u;

    (void)memset(&context, 0, sizeof(context));
    interface_table = make_interface(test_begin, test_commit, test_rollback);
    binding = make_binding(&interface_table, &context);
    (void)memset(&journal, 0, sizeof(journal));
    (void)memset(&uow, 0, sizeof(uow));
    (void)memset(state, 0xa5, sizeof(state));

    CHECK(status_is(arbor_ddd_transaction_interface_validate(&binding), 0));
    CHECK(status_is(
        arbor_ddd_event_journal_init(records, 8u, bytes, 128u, &journal), 0));

    CHECK(status_is(
        arbor_ddd_unit_of_work_begin(
            &binding,
            state,
            (uint64_t)sizeof(state),
            &journal,
            &uow),
        0));
    CHECK(uow.state == (uint32_t)ARBOR_DDD_UNIT_OF_WORK_ACTIVE);
    CHECK(context.begin_calls == UINT64_C(1));
    CHECK(*(uint64_t *)state == UINT64_C(0xaabbccddeeff0011));

    (void)memset(&active, 0, sizeof(active));
    CHECK(status_is(arbor_ddd_unit_of_work_active_transaction(&uow, &active), 0));
    CHECK(active.interface_table == &interface_table);
    CHECK(active.provider_context == &context);
    CHECK(active.transaction_state == state);
    CHECK(status_is(arbor_ddd_transaction_view_validate(&active), 0));

    CHECK(status_is(
        arbor_ddd_event_journal_append(
            &journal, event_type, 1u, 0u, &payload, 1u, &sequence),
        0));
    CHECK(journal.record_count == 1u);

    CHECK(status_is(arbor_ddd_unit_of_work_commit(&uow), 0));
    CHECK(uow.state == (uint32_t)ARBOR_DDD_UNIT_OF_WORK_COMMITTED);
    CHECK(context.commit_calls == UINT64_C(1));
    CHECK(journal.record_count == 1u);
    CHECK(status_is(arbor_ddd_unit_of_work_active_transaction(&uow, &active), -EALREADY));

    CHECK(status_is(arbor_ddd_unit_of_work_reset(&uow), 0));
    CHECK(uow.state == (uint32_t)ARBOR_DDD_UNIT_OF_WORK_ZERO);

    CHECK(status_is(
        arbor_ddd_unit_of_work_begin(
            &binding,
            state,
            (uint64_t)sizeof(state),
            &journal,
            &uow),
        0));
    CHECK(status_is(
        arbor_ddd_event_journal_append(
            &journal, event_type, 1u, 0u, &payload, 1u, &sequence),
        0));
    CHECK(journal.record_count == 2u);

    CHECK(status_is(arbor_ddd_unit_of_work_rollback(&uow), 0));
    CHECK(uow.state == (uint32_t)ARBOR_DDD_UNIT_OF_WORK_ROLLED_BACK);
    CHECK(context.rollback_calls == UINT64_C(1));
    CHECK(journal.record_count == 1u);
    CHECK(status_is(arbor_ddd_unit_of_work_reset(&uow), 0));
}

static void test_uow_failures(void)
{
    arbor_ddd_event_record records[8];
    uint8_t bytes[128];
    arbor_ddd_event_journal journal;
    _Alignas(64) uint8_t state[64];
    test_tx_context context;
    arbor_ddd_transaction_interface interface_table;
    arbor_capability_binding binding;
    arbor_ddd_unit_of_work uow;
    arbor_ddd_event_type_id event_type = {UINT64_C(3), UINT64_C(4)};
    uint32_t sequence = 0u;
    const uint8_t payload[2] = {0x10u, 0x20u};

    (void)memset(&context, 0, sizeof(context));
    interface_table = make_interface(test_begin, test_commit, test_rollback);
    binding = make_binding(&interface_table, &context);
    (void)memset(&journal, 0, sizeof(journal));
    (void)memset(&uow, 0, sizeof(uow));
    CHECK(status_is(
        arbor_ddd_event_journal_init(records, 8u, bytes, 128u, &journal), 0));

    context.commit_result = -EIO;
    CHECK(status_is(
        arbor_ddd_unit_of_work_begin(
            &binding, state, (uint64_t)sizeof(state), &journal, &uow),
        0));
    CHECK(status_is(
        arbor_ddd_event_journal_append(
            &journal, event_type, 1u, 0u, payload, 2u, &sequence),
        0));
    CHECK(journal.record_count == 1u);
    CHECK(status_is(arbor_ddd_unit_of_work_commit(&uow), -EIO));
    CHECK(uow.state == (uint32_t)ARBOR_DDD_UNIT_OF_WORK_FAULTED);
    CHECK(journal.record_count == 0u);
    CHECK(journal.byte_count == 0u);
    CHECK(status_is(arbor_ddd_unit_of_work_reset(&uow), 0));

    context.commit_result = 0;
    context.rollback_result = -ECONNRESET;
    CHECK(status_is(
        arbor_ddd_unit_of_work_begin(
            &binding, state, (uint64_t)sizeof(state), &journal, &uow),
        0));
    CHECK(status_is(
        arbor_ddd_event_journal_append(
            &journal, event_type, 1u, 0u, payload, 2u, &sequence),
        0));
    CHECK(status_is(arbor_ddd_unit_of_work_rollback(&uow), -ECONNRESET));
    CHECK(uow.state == (uint32_t)ARBOR_DDD_UNIT_OF_WORK_FAULTED);
    CHECK(journal.record_count == 0u);
    CHECK(status_is(arbor_ddd_unit_of_work_reset(&uow), 0));

    context.rollback_result = 0;
    context.begin_result = -EIO;
    CHECK(status_is(
        arbor_ddd_unit_of_work_begin(
            &binding, state, (uint64_t)sizeof(state), &journal, &uow),
        -EIO));
    CHECK(uow.state == (uint32_t)ARBOR_DDD_UNIT_OF_WORK_ZERO);
    CHECK(uow.event_journal == NULL);
}

static void test_assembly_callbacks(void)
{
    arbor_ddd_event_record records[2];
    uint8_t bytes[16];
    arbor_ddd_event_journal journal;
    _Alignas(64) uint8_t state[64];
    asm_tx_context context;
    arbor_ddd_transaction_interface interface_table;
    arbor_capability_binding binding;
    arbor_ddd_unit_of_work uow;

    (void)memset(&context, 0, sizeof(context));
    context.expected_state_size = (uint64_t)sizeof(state);
    context.expected_state = state;

    interface_table = make_interface(
        af4_asm_transaction_begin,
        af4_asm_transaction_commit,
        af4_asm_transaction_rollback);
    binding = make_binding(&interface_table, &context);

    (void)memset(&journal, 0, sizeof(journal));
    (void)memset(&uow, 0, sizeof(uow));

    CHECK(status_is(
        arbor_ddd_event_journal_init(records, 2u, bytes, 16u, &journal), 0));
    CHECK(status_is(
        arbor_ddd_unit_of_work_begin(
            &binding, state, (uint64_t)sizeof(state), &journal, &uow),
        0));
    CHECK(context.begin_calls == UINT64_C(1));
    CHECK(*(uint64_t *)state == UINT64_C(0x11223344));
    CHECK(status_is(arbor_ddd_unit_of_work_commit(&uow), 0));
    CHECK(context.commit_calls == UINT64_C(1));
    CHECK(context.stack_misaligned == UINT64_C(0));
    CHECK(status_is(arbor_ddd_unit_of_work_reset(&uow), 0));

    CHECK(status_is(
        arbor_ddd_unit_of_work_begin(
            &binding, state, (uint64_t)sizeof(state), &journal, &uow),
        0));
    CHECK(status_is(arbor_ddd_unit_of_work_rollback(&uow), 0));
    CHECK(context.rollback_calls == UINT64_C(1));
    CHECK(context.stack_misaligned == UINT64_C(0));
}

int main(void)
{
    test_event_journal();
    test_uow_success_and_rollback();
    test_uow_failures();
    test_assembly_callbacks();

    if (failures != 0) {
        (void)fprintf(stderr, "AF4 functional failures=%d\n", failures);
        return 1;
    }

    (void)puts("PASS: AF4 DDD support native functional + real NASM ABI");
    return 0;
}
