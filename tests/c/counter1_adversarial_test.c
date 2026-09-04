#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "counter1.h"

#define TEST_TX_GUARD UINT64_C(0x5445535454584752)

typedef struct failure_provider_context {
    uint64_t value;
    bool fail_commit;
    bool fail_rollback;
    const arbor_ddd_transaction_interface *transaction;
} failure_provider_context;

static int fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    return 1;
}

static int64_t failure_tx_begin(
    void *provider_context,
    void *transaction_state,
    uint64_t transaction_state_size)
{
    failure_provider_context *context =
        (failure_provider_context *)provider_context;
    counter1_transaction_state *state =
        (counter1_transaction_state *)transaction_state;
    if (context == NULL || state == NULL ||
        transaction_state_size != sizeof(*state)) {
        return -EINVAL;
    }
    *state = (counter1_transaction_state){TEST_TX_GUARD, 0u, 0u, 0u};
    return 0;
}

static int64_t failure_tx_commit(void *provider_context, void *transaction_state)
{
    failure_provider_context *context =
        (failure_provider_context *)provider_context;
    counter1_transaction_state *state =
        (counter1_transaction_state *)transaction_state;
    if (context == NULL || state == NULL || state->guard != TEST_TX_GUARD) {
        return -EINVAL;
    }
    if (context->fail_commit) {
        return -EIO;
    }
    if (state->has_staged_mutation != 0u) {
        context->value = state->staged_value;
    }
    state->guard = 0u;
    return 0;
}

static int64_t failure_tx_rollback(void *provider_context, void *transaction_state)
{
    failure_provider_context *context =
        (failure_provider_context *)provider_context;
    counter1_transaction_state *state =
        (counter1_transaction_state *)transaction_state;
    if (context == NULL || state == NULL || state->guard != TEST_TX_GUARD) {
        return -EINVAL;
    }
    if (context->fail_rollback) {
        return -EIO;
    }
    state->guard = 0u;
    state->has_staged_mutation = 0u;
    return 0;
}

static int failure_view_validate(
    failure_provider_context *context,
    const arbor_ddd_transaction_view *transaction)
{
    if (context == NULL || transaction == NULL ||
        arbor_ddd_transaction_view_validate(transaction).native != 0 ||
        transaction->interface_table != context->transaction ||
        transaction->provider_context != context ||
        transaction->transaction_state_size != sizeof(counter1_transaction_state)) {
        return 0;
    }
    const counter1_transaction_state *state =
        (const counter1_transaction_state *)transaction->transaction_state;
    return state != NULL && state->guard == TEST_TX_GUARD;
}

static int64_t failure_repo_get(
    void *provider_context,
    const arbor_ddd_transaction_view *transaction,
    uint64_t id,
    counter1_repository_get_result *result_out)
{
    failure_provider_context *context =
        (failure_provider_context *)provider_context;
    if (result_out == NULL || !failure_view_validate(context, transaction)) {
        return -EINVAL;
    }
    counter1_repository_get_result result = {
        id == 1u ? (uint32_t)COUNTER1_REPOSITORY_GET_FOUND :
                   (uint32_t)COUNTER1_REPOSITORY_GET_NOT_FOUND,
        0u,
        id,
        id == 1u ? context->value : 0u
    };
    *result_out = result;
    return 0;
}

static int64_t failure_repo_increment(
    void *provider_context,
    const arbor_ddd_transaction_view *transaction,
    uint64_t id,
    counter1_repository_increment_result *result_out)
{
    failure_provider_context *context =
        (failure_provider_context *)provider_context;
    if (result_out == NULL || !failure_view_validate(context, transaction)) {
        return -EINVAL;
    }
    counter1_repository_increment_result result = {
        (uint32_t)COUNTER1_REPOSITORY_INCREMENT_NOT_FOUND,
        0u,
        id,
        0u
    };
    if (id != 1u) {
        *result_out = result;
        return 0;
    }
    if (context->value == UINT64_MAX) {
        result.outcome_code =
            (uint32_t)COUNTER1_REPOSITORY_INCREMENT_LIMIT_REACHED;
        result.value = context->value;
        *result_out = result;
        return 0;
    }
    counter1_transaction_state *state =
        (counter1_transaction_state *)transaction->transaction_state;
    if (state->has_staged_mutation != 0u) {
        return -EINVAL;
    }
    state->staged_index = 0u;
    state->staged_value = context->value + 1u;
    state->has_staged_mutation = 1u;
    result.outcome_code = (uint32_t)COUNTER1_REPOSITORY_INCREMENTED;
    result.value = state->staged_value;
    *result_out = result;
    return 0;
}

static const counter1_repository_v1 failure_repository = {
    COUNTER1_REPOSITORY_ABI_VERSION,
    (uint32_t)sizeof(counter1_repository_v1),
    COUNTER1_REPOSITORY_FLAGS_NONE,
    failure_repo_get,
    failure_repo_increment
};

static const arbor_ddd_transaction_interface failure_transaction = {
    ARBOR_DDD_TRANSACTION_INTERFACE_ABI_VERSION,
    (uint32_t)sizeof(arbor_ddd_transaction_interface),
    ARBOR_DDD_TRANSACTION_FLAGS_NONE,
    {UINT64_C(0x434f554e54455231), UINT64_C(0x2001)},
    sizeof(counter1_transaction_state),
    _Alignof(counter1_transaction_state),
    failure_tx_begin,
    failure_tx_commit,
    failure_tx_rollback
};

static int call_increment(
    counter1_application *application,
    uint64_t id,
    counter1_service_result *result_out)
{
    uint8_t arena_bytes[4096] = {0};
    arbor_asm_arena arena = {0};
    arbor_asm_result_u64 init = arena_init(&arena, arena_bytes, sizeof(arena_bytes));
    if (init.status != 0) {
        return (int)init.status;
    }
    const counter1_service_v1 *service =
        (const counter1_service_v1 *)application->service_binding.interface_table;
    counter1_service_request request = {&arena, id};
    return (int)service->increment_counter(
        application->service_binding.provider_context,
        &request,
        result_out);
}

int main(void)
{
    counter1_repository_provider invalid = {0};
    if (counter1_repository_provider_validate(&invalid).native != -EINVAL) {
        return fail("reject invalid repository provider");
    }

    failure_provider_context context = {0u, false, false, &failure_transaction};
    counter1_repository_provider provider = {
        &failure_repository,
        &failure_transaction,
        &context
    };
    if (counter1_repository_provider_validate(&provider).native != 0) {
        return fail("custom failure provider validates");
    }

    counter1_application application = {0};
    if (counter1_application_prepare(&provider, &application).native != 0) {
        return fail("application prepare with custom provider");
    }

    context.fail_commit = true;
    counter1_service_result result = {
        UINT32_C(0xa5a5a5a5), UINT32_C(0x5a5a5a5a),
        UINT64_C(0x1111111111111111), UINT64_C(0x2222222222222222)
    };
    counter1_service_result before = result;
    int native = call_increment(&application, 1u, &result);
    if (native != -EIO || context.value != 0u ||
        memcmp(&result, &before, sizeof(result)) != 0) {
        (void)counter1_application_stop(&application);
        return fail("commit failure preserves provider and typed output");
    }

    context.fail_commit = false;
    context.value = UINT64_MAX;
    context.fail_rollback = true;
    result = before;
    native = call_increment(&application, 1u, &result);
    if (native != -EIO || context.value != UINT64_MAX ||
        memcmp(&result, &before, sizeof(result)) != 0) {
        (void)counter1_application_stop(&application);
        return fail("rollback failure overrides business outcome");
    }

    context.fail_rollback = false;
    result = (counter1_service_result){0};
    native = call_increment(&application, 1u, &result);
    if (native != 0 || result.outcome_code != COUNTER1_SERVICE_LIMIT_REACHED ||
        result.id != 1u || result.value != UINT64_MAX) {
        (void)counter1_application_stop(&application);
        return fail("limit typed outcome after successful rollback");
    }

    if (counter1_application_stop(&application).native != 0) {
        return fail("application stop");
    }

    puts("PASS: COUNTER1 adversarial commit/rollback failure atomicity");
    return 0;
}
