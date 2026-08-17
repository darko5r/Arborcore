#ifndef ARBORCORE_DDD_SUPPORT_H
#define ARBORCORE_DDD_SUPPORT_H

#include <stdint.h>

#include <arborcore/capability.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ARBOR_DDD_SUPPORT_ABI_VERSION 1u
#define ARBOR_DDD_TRANSACTION_INTERFACE_ABI_VERSION 1u

#define ARBOR_DDD_EVENT_FLAGS_NONE UINT32_C(0)
#define ARBOR_DDD_TRANSACTION_FLAGS_NONE UINT64_C(0)
#define ARBOR_DDD_UNIT_OF_WORK_FLAGS_NONE UINT32_C(0)

#define ARBOR_DDD_TRANSACTION_MAX_STATE_ALIGNMENT UINT64_C(4096)

typedef arbor_capability_id arbor_ddd_event_type_id;
typedef arbor_capability_id arbor_ddd_transaction_authority_id;

typedef struct arbor_ddd_event_record {
    arbor_ddd_event_type_id type_id;
    uint32_t abi_version;
    uint32_t flags;
    uint32_t payload_offset;
    uint32_t payload_size;
} arbor_ddd_event_record;

typedef struct arbor_ddd_event_checkpoint {
    uint32_t record_count;
    uint32_t byte_count;
    uint64_t generation;
} arbor_ddd_event_checkpoint;

typedef struct arbor_ddd_event_journal {
    arbor_ddd_event_record *records;
    uint32_t record_capacity;
    uint32_t record_count;
    uint8_t *payload_bytes;
    uint32_t byte_capacity;
    uint32_t byte_count;
    uint64_t generation;
} arbor_ddd_event_journal;

typedef struct arbor_ddd_event_view {
    arbor_ddd_event_type_id type_id;
    uint32_t abi_version;
    uint32_t flags;
    const uint8_t *payload;
    uint32_t payload_size;
    uint32_t sequence;
} arbor_ddd_event_view;

typedef int64_t (*arbor_ddd_transaction_begin_fn)(
    void *provider_context,
    void *transaction_state,
    uint64_t transaction_state_size);

typedef int64_t (*arbor_ddd_transaction_commit_fn)(
    void *provider_context,
    void *transaction_state);

typedef int64_t (*arbor_ddd_transaction_rollback_fn)(
    void *provider_context,
    void *transaction_state);

typedef struct arbor_ddd_transaction_interface {
    uint32_t abi_version;
    uint32_t struct_size;
    uint64_t flags;
    arbor_ddd_transaction_authority_id authority_id;
    uint64_t state_size;
    uint64_t state_alignment;
    arbor_ddd_transaction_begin_fn begin;
    arbor_ddd_transaction_commit_fn commit;
    arbor_ddd_transaction_rollback_fn rollback;
} arbor_ddd_transaction_interface;

typedef struct arbor_ddd_transaction_view {
    const arbor_ddd_transaction_interface *interface_table;
    void *provider_context;
    void *transaction_state;
    uint64_t transaction_state_size;
} arbor_ddd_transaction_view;

typedef enum arbor_ddd_unit_of_work_state {
    ARBOR_DDD_UNIT_OF_WORK_ZERO = 0,
    ARBOR_DDD_UNIT_OF_WORK_ACTIVE = 1,
    ARBOR_DDD_UNIT_OF_WORK_COMMITTED = 2,
    ARBOR_DDD_UNIT_OF_WORK_ROLLED_BACK = 3,
    ARBOR_DDD_UNIT_OF_WORK_FAULTED = 4
} arbor_ddd_unit_of_work_state;

typedef struct arbor_ddd_unit_of_work {
    arbor_ddd_transaction_view transaction_view;
    arbor_ddd_event_journal *event_journal;
    arbor_ddd_event_checkpoint event_checkpoint;
    uint32_t state;
    uint32_t flags;
} arbor_ddd_unit_of_work;

arbor_status arbor_ddd_event_journal_validate(
    const arbor_ddd_event_journal *journal);

arbor_status arbor_ddd_event_journal_init(
    arbor_ddd_event_record *records,
    uint32_t record_capacity,
    uint8_t *payload_bytes,
    uint32_t byte_capacity,
    arbor_ddd_event_journal *out);

arbor_status arbor_ddd_event_journal_checkpoint(
    const arbor_ddd_event_journal *journal,
    arbor_ddd_event_checkpoint *out);

arbor_status arbor_ddd_event_journal_append(
    arbor_ddd_event_journal *journal,
    arbor_ddd_event_type_id type_id,
    uint32_t abi_version,
    uint32_t flags,
    const void *payload,
    uint32_t payload_size,
    uint32_t *sequence_out);

arbor_status arbor_ddd_event_journal_rewind(
    arbor_ddd_event_journal *journal,
    const arbor_ddd_event_checkpoint *checkpoint);

arbor_status arbor_ddd_event_journal_view(
    const arbor_ddd_event_journal *journal,
    uint32_t sequence,
    arbor_ddd_event_view *out);

arbor_status arbor_ddd_event_journal_clear(
    arbor_ddd_event_journal *journal);

arbor_status arbor_ddd_transaction_interface_validate(
    const arbor_capability_binding *binding);

arbor_status arbor_ddd_transaction_view_validate(
    const arbor_ddd_transaction_view *view);

arbor_status arbor_ddd_unit_of_work_begin(
    const arbor_capability_binding *transaction_binding,
    void *transaction_state,
    uint64_t transaction_state_size,
    arbor_ddd_event_journal *event_journal,
    arbor_ddd_unit_of_work *out);

arbor_status arbor_ddd_unit_of_work_commit(
    arbor_ddd_unit_of_work *unit_of_work);

arbor_status arbor_ddd_unit_of_work_rollback(
    arbor_ddd_unit_of_work *unit_of_work);

arbor_status arbor_ddd_unit_of_work_active_transaction(
    const arbor_ddd_unit_of_work *unit_of_work,
    arbor_ddd_transaction_view *out);

arbor_status arbor_ddd_unit_of_work_reset(
    arbor_ddd_unit_of_work *unit_of_work);

#ifdef __cplusplus
}
#endif

#endif
