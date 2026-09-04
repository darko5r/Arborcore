#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "counter1.h"

#define COUNTER1_ID_HIGH UINT64_C(0x434f554e54455231)
#define COUNTER1_TRANSACTION_CAPABILITY_LOW UINT64_C(0x0000000000001002)

static int fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    return 1;
}

static arbor_capability_binding transaction_binding(
    const counter1_repository_provider *provider)
{
    return (arbor_capability_binding){
        {COUNTER1_ID_HIGH, COUNTER1_TRANSACTION_CAPABILITY_LOW},
        {1u, 0u},
        (uint32_t)sizeof(arbor_ddd_transaction_interface),
        0u,
        ARBOR_CAPABILITY_FLAGS_NONE,
        provider->transaction,
        provider->provider_context,
        0u
    };
}

static arbor_status begin_uow(
    const counter1_repository_provider *provider,
    counter1_transaction_state *state,
    arbor_ddd_event_record *record,
    uint8_t payload[16],
    arbor_ddd_event_journal *journal,
    arbor_ddd_unit_of_work *uow)
{
    arbor_status status = arbor_ddd_event_journal_init(
        record, 1u, payload, 16u, journal);
    if (status.native != 0) {
        return status;
    }
    arbor_capability_binding binding = transaction_binding(provider);
    return arbor_ddd_unit_of_work_begin(
        &binding, state, sizeof(*state), journal, uow);
}

int main(void)
{
    counter1_in_memory_repository repository = {0};
    counter1_repository_provider provider = {0};
    arbor_status status = counter1_in_memory_repository_prepare(
        &repository, &provider);
    if (status.native != 0 ||
        counter1_in_memory_repository_validate(&repository).native != 0 ||
        counter1_repository_provider_validate(&provider).native != 0) {
        return fail("in-memory provider prepare/validate");
    }
    if (repository.records[0].value != 0u || repository.records[1].value != 41u ||
        repository.records[2].value != UINT64_MAX) {
        return fail("exact initial records");
    }

    counter1_transaction_state state = {0};
    arbor_ddd_event_record record = {0};
    uint8_t payload[16] = {0};
    arbor_ddd_event_journal journal = {0};
    arbor_ddd_unit_of_work uow = {0};
    status = begin_uow(
        &provider, &state, &record, payload, &journal, &uow);
    if (status.native != 0) {
        return fail("GET UoW begin");
    }
    arbor_ddd_transaction_view view = {0};
    status = arbor_ddd_unit_of_work_active_transaction(&uow, &view);
    if (status.native != 0) {
        return fail("GET active transaction");
    }
    counter1_repository_get_result get_result = {0};
    if (provider.repository->get(
            provider.provider_context, &view, 1u, &get_result) != 0 ||
        get_result.outcome_code != COUNTER1_REPOSITORY_GET_FOUND ||
        get_result.id != 1u || get_result.value != 0u) {
        return fail("GET existing record");
    }
    status = arbor_ddd_unit_of_work_commit(&uow);
    if (status.native != 0 || journal.record_count != 0u) {
        return fail("GET commit with zero events");
    }

    (void)memset(&state, 0, sizeof(state));
    (void)memset(&record, 0, sizeof(record));
    (void)memset(payload, 0, sizeof(payload));
    (void)memset(&journal, 0, sizeof(journal));
    (void)memset(&uow, 0, sizeof(uow));
    status = begin_uow(
        &provider, &state, &record, payload, &journal, &uow);
    if (status.native != 0 ||
        arbor_ddd_unit_of_work_active_transaction(&uow, &view).native != 0) {
        return fail("increment UoW begin/view");
    }
    counter1_repository_increment_result increment_result = {0};
    if (provider.repository->increment(
            provider.provider_context, &view, 1u, &increment_result) != 0 ||
        increment_result.outcome_code != COUNTER1_REPOSITORY_INCREMENTED ||
        increment_result.id != 1u || increment_result.value != 1u ||
        repository.records[0].value != 0u) {
        return fail("increment stages without provider mutation");
    }
    status = arbor_ddd_unit_of_work_commit(&uow);
    if (status.native != 0 || repository.records[0].value != 1u) {
        return fail("increment commit applies staged mutation");
    }

    (void)memset(&state, 0, sizeof(state));
    (void)memset(&record, 0, sizeof(record));
    (void)memset(payload, 0, sizeof(payload));
    (void)memset(&journal, 0, sizeof(journal));
    (void)memset(&uow, 0, sizeof(uow));
    status = begin_uow(
        &provider, &state, &record, payload, &journal, &uow);
    if (status.native != 0 ||
        arbor_ddd_unit_of_work_active_transaction(&uow, &view).native != 0) {
        return fail("limit UoW begin/view");
    }
    increment_result = (counter1_repository_increment_result){0};
    if (provider.repository->increment(
            provider.provider_context, &view, 3u, &increment_result) != 0 ||
        increment_result.outcome_code != COUNTER1_REPOSITORY_INCREMENT_LIMIT_REACHED ||
        increment_result.id != 3u || increment_result.value != UINT64_MAX) {
        return fail("limit reached typed outcome");
    }
    status = arbor_ddd_unit_of_work_rollback(&uow);
    if (status.native != 0 || repository.records[2].value != UINT64_MAX) {
        return fail("limit rollback preserves provider");
    }

    (void)memset(&state, 0, sizeof(state));
    (void)memset(&record, 0, sizeof(record));
    (void)memset(payload, 0, sizeof(payload));
    (void)memset(&journal, 0, sizeof(journal));
    (void)memset(&uow, 0, sizeof(uow));
    status = begin_uow(
        &provider, &state, &record, payload, &journal, &uow);
    if (status.native != 0 ||
        arbor_ddd_unit_of_work_active_transaction(&uow, &view).native != 0) {
        return fail("not-found UoW begin/view");
    }
    get_result = (counter1_repository_get_result){0};
    if (provider.repository->get(
            provider.provider_context, &view, 999u, &get_result) != 0 ||
        get_result.outcome_code != COUNTER1_REPOSITORY_GET_NOT_FOUND ||
        get_result.id != 999u || get_result.value != 0u) {
        return fail("GET not-found canonical result");
    }
    status = arbor_ddd_unit_of_work_commit(&uow);
    if (status.native != 0) {
        return fail("GET not-found commit");
    }

    counter1_repository_get_result sentinel = {
        UINT32_C(0xa5a5a5a5), UINT32_C(0x5a5a5a5a),
        UINT64_C(0x1122334455667788), UINT64_C(0x8877665544332211)
    };
    counter1_repository_get_result before = sentinel;
    arbor_ddd_transaction_view bad_view = {0};
    if (provider.repository->get(
            provider.provider_context, &bad_view, 1u, &sentinel) != -EINVAL ||
        memcmp(&sentinel, &before, sizeof(before)) != 0) {
        return fail("repository result failure atomicity");
    }

    puts("PASS: COUNTER1 core repository and AF4 transaction semantics");
    return 0;
}
