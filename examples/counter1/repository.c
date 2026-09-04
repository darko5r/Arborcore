#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "counter1.h"

#define COUNTER1_REPOSITORY_GUARD UINT64_C(0x434e545231524550)
#define COUNTER1_TRANSACTION_GUARD UINT64_C(0x434e54523154584e)
#define COUNTER1_ID_HIGH UINT64_C(0x434f554e54455231)
#define COUNTER1_TRANSACTION_CAPABILITY_LOW UINT64_C(0x0000000000001002)
#define COUNTER1_TRANSACTION_AUTHORITY_LOW UINT64_C(0x0000000000002001)

static int64_t counter1_transaction_begin(
    void *provider_context,
    void *transaction_state,
    uint64_t transaction_state_size);
static int64_t counter1_transaction_commit(
    void *provider_context,
    void *transaction_state);
static int64_t counter1_transaction_rollback(
    void *provider_context,
    void *transaction_state);
static int64_t counter1_repository_get(
    void *provider_context,
    const arbor_ddd_transaction_view *transaction,
    uint64_t id,
    counter1_repository_get_result *result_out);
static int64_t counter1_repository_increment(
    void *provider_context,
    const arbor_ddd_transaction_view *transaction,
    uint64_t id,
    counter1_repository_increment_result *result_out);

static const counter1_repository_v1 counter1_repository_interface = {
    COUNTER1_REPOSITORY_ABI_VERSION,
    (uint32_t)sizeof(counter1_repository_v1),
    COUNTER1_REPOSITORY_FLAGS_NONE,
    counter1_repository_get,
    counter1_repository_increment
};

static const arbor_ddd_transaction_interface counter1_transaction_interface = {
    ARBOR_DDD_TRANSACTION_INTERFACE_ABI_VERSION,
    (uint32_t)sizeof(arbor_ddd_transaction_interface),
    ARBOR_DDD_TRANSACTION_FLAGS_NONE,
    {COUNTER1_ID_HIGH, COUNTER1_TRANSACTION_AUTHORITY_LOW},
    (uint64_t)sizeof(counter1_transaction_state),
    (uint64_t)_Alignof(counter1_transaction_state),
    counter1_transaction_begin,
    counter1_transaction_commit,
    counter1_transaction_rollback
};

_Static_assert(sizeof(counter1_transaction_state) == 32u,
               "COUNTER1 transaction-state size drift");
_Static_assert(_Alignof(counter1_transaction_state) == 8u,
               "COUNTER1 transaction-state alignment drift");

static arbor_status counter1_repository_interface_validate(
    const counter1_repository_v1 *repository)
{
    if (repository == NULL ||
        repository->abi_version != COUNTER1_REPOSITORY_ABI_VERSION ||
        repository->struct_size != (uint32_t)sizeof(counter1_repository_v1) ||
        repository->flags != COUNTER1_REPOSITORY_FLAGS_NONE ||
        repository->get == NULL || repository->increment == NULL) {
        return arbor_status_from_native(-EINVAL);
    }
    return arbor_status_from_native(0);
}

static arbor_status counter1_transaction_view_matches_repository(
    const counter1_in_memory_repository *repository,
    const arbor_ddd_transaction_view *transaction)
{
    arbor_status status = arbor_ddd_transaction_view_validate(transaction);
    if (status.native != 0) {
        return status;
    }
    if (repository == NULL ||
        transaction->interface_table != &counter1_transaction_interface ||
        transaction->provider_context != repository ||
        transaction->transaction_state == NULL ||
        transaction->transaction_state_size !=
            (uint64_t)sizeof(counter1_transaction_state)) {
        return arbor_status_from_native(-EINVAL);
    }
    const counter1_transaction_state *state =
        (const counter1_transaction_state *)transaction->transaction_state;
    if (state->guard != COUNTER1_TRANSACTION_GUARD) {
        return arbor_status_from_native(-EINVAL);
    }
    return arbor_status_from_native(0);
}

static int64_t counter1_find_record(
    const counter1_in_memory_repository *repository,
    uint64_t id,
    uint64_t *index_out)
{
    if (repository == NULL || index_out == NULL) {
        return -EINVAL;
    }
    for (uint64_t index = 0u; index < repository->record_count; ++index) {
        if (repository->records[index].id == id) {
            *index_out = index;
            return 0;
        }
    }
    return -ENOENT;
}

static int64_t counter1_transaction_begin(
    void *provider_context,
    void *transaction_state,
    uint64_t transaction_state_size)
{
    counter1_in_memory_repository *repository =
        (counter1_in_memory_repository *)provider_context;
    if (counter1_in_memory_repository_validate(repository).native != 0 ||
        transaction_state == NULL ||
        transaction_state_size != (uint64_t)sizeof(counter1_transaction_state)) {
        return -EINVAL;
    }
    counter1_transaction_state *state =
        (counter1_transaction_state *)transaction_state;
    *state = (counter1_transaction_state){
        COUNTER1_TRANSACTION_GUARD, 0u, 0u, 0u
    };
    return 0;
}

static int64_t counter1_transaction_commit(
    void *provider_context,
    void *transaction_state)
{
    counter1_in_memory_repository *repository =
        (counter1_in_memory_repository *)provider_context;
    counter1_transaction_state *state =
        (counter1_transaction_state *)transaction_state;
    if (counter1_in_memory_repository_validate(repository).native != 0 ||
        state == NULL || state->guard != COUNTER1_TRANSACTION_GUARD ||
        state->has_staged_mutation > 1u) {
        return -EINVAL;
    }
    if (state->has_staged_mutation == 0u) {
        state->guard = 0u;
        return 0;
    }
    if (state->staged_index >= repository->record_count) {
        return -EINVAL;
    }
    const uint64_t current = repository->records[state->staged_index].value;
    if (current == UINT64_MAX || state->staged_value != current + UINT64_C(1)) {
        return -EINVAL;
    }
    repository->records[state->staged_index].value = state->staged_value;
    state->has_staged_mutation = 0u;
    state->guard = 0u;
    return 0;
}

static int64_t counter1_transaction_rollback(
    void *provider_context,
    void *transaction_state)
{
    counter1_in_memory_repository *repository =
        (counter1_in_memory_repository *)provider_context;
    counter1_transaction_state *state =
        (counter1_transaction_state *)transaction_state;
    if (counter1_in_memory_repository_validate(repository).native != 0 ||
        state == NULL || state->guard != COUNTER1_TRANSACTION_GUARD ||
        state->has_staged_mutation > 1u) {
        return -EINVAL;
    }
    state->staged_index = 0u;
    state->staged_value = 0u;
    state->has_staged_mutation = 0u;
    state->guard = 0u;
    return 0;
}

static int64_t counter1_repository_get(
    void *provider_context,
    const arbor_ddd_transaction_view *transaction,
    uint64_t id,
    counter1_repository_get_result *result_out)
{
    counter1_in_memory_repository *repository =
        (counter1_in_memory_repository *)provider_context;
    if (result_out == NULL ||
        counter1_in_memory_repository_validate(repository).native != 0 ||
        counter1_transaction_view_matches_repository(repository, transaction).native != 0) {
        return -EINVAL;
    }

    counter1_repository_get_result candidate = {
        (uint32_t)COUNTER1_REPOSITORY_GET_NOT_FOUND, 0u, id, 0u
    };
    uint64_t index = 0u;
    const int64_t found = counter1_find_record(repository, id, &index);
    if (found == 0) {
        candidate.outcome_code = (uint32_t)COUNTER1_REPOSITORY_GET_FOUND;
        candidate.value = repository->records[index].value;
    } else if (found != -ENOENT) {
        return found;
    }
    *result_out = candidate;
    return 0;
}

static int64_t counter1_repository_increment(
    void *provider_context,
    const arbor_ddd_transaction_view *transaction,
    uint64_t id,
    counter1_repository_increment_result *result_out)
{
    counter1_in_memory_repository *repository =
        (counter1_in_memory_repository *)provider_context;
    if (result_out == NULL ||
        counter1_in_memory_repository_validate(repository).native != 0 ||
        counter1_transaction_view_matches_repository(repository, transaction).native != 0) {
        return -EINVAL;
    }

    counter1_transaction_state *state =
        (counter1_transaction_state *)transaction->transaction_state;
    if (state->has_staged_mutation != 0u) {
        return -EINVAL;
    }

    counter1_repository_increment_result candidate = {
        (uint32_t)COUNTER1_REPOSITORY_INCREMENT_NOT_FOUND, 0u, id, 0u
    };
    uint64_t index = 0u;
    const int64_t found = counter1_find_record(repository, id, &index);
    if (found == -ENOENT) {
        *result_out = candidate;
        return 0;
    }
    if (found != 0) {
        return found;
    }

    const uint64_t current = repository->records[index].value;
    if (current == UINT64_MAX) {
        candidate.outcome_code =
            (uint32_t)COUNTER1_REPOSITORY_INCREMENT_LIMIT_REACHED;
        candidate.value = current;
        *result_out = candidate;
        return 0;
    }

    state->staged_index = index;
    state->staged_value = current + UINT64_C(1);
    state->has_staged_mutation = 1u;
    candidate.outcome_code = (uint32_t)COUNTER1_REPOSITORY_INCREMENTED;
    candidate.value = state->staged_value;
    *result_out = candidate;
    return 0;
}

arbor_status counter1_in_memory_repository_prepare(
    counter1_in_memory_repository *repository,
    counter1_repository_provider *provider_out)
{
    if (repository == NULL || provider_out == NULL) {
        return arbor_status_from_native(-EINVAL);
    }

    counter1_in_memory_repository candidate = {
        {{1u, 0u}, {2u, 41u}, {3u, UINT64_MAX}},
        3u,
        COUNTER1_REPOSITORY_GUARD
    };
    counter1_repository_provider provider = {
        &counter1_repository_interface,
        &counter1_transaction_interface,
        repository
    };

    *repository = candidate;
    *provider_out = provider;
    return counter1_repository_provider_validate(provider_out);
}

arbor_status counter1_in_memory_repository_validate(
    const counter1_in_memory_repository *repository)
{
    if (repository == NULL ||
        repository->prepared_guard != COUNTER1_REPOSITORY_GUARD ||
        repository->record_count != 3u ||
        repository->records[0].id != 1u ||
        repository->records[1].id != 2u ||
        repository->records[2].id != 3u) {
        return arbor_status_from_native(-EINVAL);
    }
    return arbor_status_from_native(0);
}

arbor_status counter1_repository_provider_validate(
    const counter1_repository_provider *provider)
{
    if (provider == NULL || provider->provider_context == NULL) {
        return arbor_status_from_native(-EINVAL);
    }
    arbor_status status = counter1_repository_interface_validate(provider->repository);
    if (status.native != 0 || provider->transaction == NULL) {
        return arbor_status_from_native(-EINVAL);
    }

    const arbor_capability_binding transaction_binding = {
        {COUNTER1_ID_HIGH, COUNTER1_TRANSACTION_CAPABILITY_LOW},
        {1u, 0u},
        (uint32_t)sizeof(arbor_ddd_transaction_interface),
        0u,
        ARBOR_CAPABILITY_FLAGS_NONE,
        provider->transaction,
        provider->provider_context,
        0u
    };
    return arbor_ddd_transaction_interface_validate(&transaction_binding);
}
