#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include <arborcore/ddd_support.h>

_Static_assert(sizeof(void *) == 8u, "AF4 requires x86-64 pointers");
_Static_assert(sizeof(uintptr_t) == 8u, "AF4 requires 64-bit uintptr_t");

_Static_assert(sizeof(arbor_ddd_event_record) == 32u, "AF4 event record ABI drift");
_Static_assert(offsetof(arbor_ddd_event_record, type_id) == 0u, "AF4 event type offset drift");
_Static_assert(offsetof(arbor_ddd_event_record, abi_version) == 16u, "AF4 event ABI offset drift");
_Static_assert(offsetof(arbor_ddd_event_record, flags) == 20u, "AF4 event flags offset drift");
_Static_assert(offsetof(arbor_ddd_event_record, payload_offset) == 24u, "AF4 event payload offset drift");
_Static_assert(offsetof(arbor_ddd_event_record, payload_size) == 28u, "AF4 event payload-size offset drift");

_Static_assert(sizeof(arbor_ddd_event_checkpoint) == 16u, "AF4 checkpoint ABI drift");
_Static_assert(offsetof(arbor_ddd_event_checkpoint, record_count) == 0u, "AF4 checkpoint record-count offset drift");
_Static_assert(offsetof(arbor_ddd_event_checkpoint, byte_count) == 4u, "AF4 checkpoint byte-count offset drift");
_Static_assert(offsetof(arbor_ddd_event_checkpoint, generation) == 8u, "AF4 checkpoint generation offset drift");

_Static_assert(sizeof(arbor_ddd_event_journal) == 40u, "AF4 journal ABI drift");
_Static_assert(offsetof(arbor_ddd_event_journal, records) == 0u, "AF4 journal records offset drift");
_Static_assert(offsetof(arbor_ddd_event_journal, record_capacity) == 8u, "AF4 journal record-capacity offset drift");
_Static_assert(offsetof(arbor_ddd_event_journal, record_count) == 12u, "AF4 journal record-count offset drift");
_Static_assert(offsetof(arbor_ddd_event_journal, payload_bytes) == 16u, "AF4 journal payload offset drift");
_Static_assert(offsetof(arbor_ddd_event_journal, byte_capacity) == 24u, "AF4 journal byte-capacity offset drift");
_Static_assert(offsetof(arbor_ddd_event_journal, byte_count) == 28u, "AF4 journal byte-count offset drift");
_Static_assert(offsetof(arbor_ddd_event_journal, generation) == 32u, "AF4 journal generation offset drift");

_Static_assert(sizeof(arbor_ddd_event_view) == 40u, "AF4 event view ABI drift");
_Static_assert(offsetof(arbor_ddd_event_view, type_id) == 0u, "AF4 event-view type offset drift");
_Static_assert(offsetof(arbor_ddd_event_view, abi_version) == 16u, "AF4 event-view ABI offset drift");
_Static_assert(offsetof(arbor_ddd_event_view, flags) == 20u, "AF4 event-view flags offset drift");
_Static_assert(offsetof(arbor_ddd_event_view, payload) == 24u, "AF4 event-view payload offset drift");
_Static_assert(offsetof(arbor_ddd_event_view, payload_size) == 32u, "AF4 event-view payload-size offset drift");
_Static_assert(offsetof(arbor_ddd_event_view, sequence) == 36u, "AF4 event-view sequence offset drift");

_Static_assert(sizeof(arbor_ddd_transaction_interface) == 72u, "AF4 transaction interface ABI drift");
_Static_assert(offsetof(arbor_ddd_transaction_interface, abi_version) == 0u, "AF4 transaction ABI offset drift");
_Static_assert(offsetof(arbor_ddd_transaction_interface, struct_size) == 4u, "AF4 transaction size offset drift");
_Static_assert(offsetof(arbor_ddd_transaction_interface, flags) == 8u, "AF4 transaction flags offset drift");
_Static_assert(offsetof(arbor_ddd_transaction_interface, authority_id) == 16u, "AF4 authority offset drift");
_Static_assert(offsetof(arbor_ddd_transaction_interface, state_size) == 32u, "AF4 state-size offset drift");
_Static_assert(offsetof(arbor_ddd_transaction_interface, state_alignment) == 40u, "AF4 state-alignment offset drift");
_Static_assert(offsetof(arbor_ddd_transaction_interface, begin) == 48u, "AF4 begin offset drift");
_Static_assert(offsetof(arbor_ddd_transaction_interface, commit) == 56u, "AF4 commit offset drift");
_Static_assert(offsetof(arbor_ddd_transaction_interface, rollback) == 64u, "AF4 rollback offset drift");

_Static_assert(sizeof(arbor_ddd_transaction_view) == 32u, "AF4 transaction view ABI drift");
_Static_assert(offsetof(arbor_ddd_transaction_view, interface_table) == 0u, "AF4 transaction-view interface offset drift");
_Static_assert(offsetof(arbor_ddd_transaction_view, provider_context) == 8u, "AF4 transaction-view context offset drift");
_Static_assert(offsetof(arbor_ddd_transaction_view, transaction_state) == 16u, "AF4 transaction-view state offset drift");
_Static_assert(offsetof(arbor_ddd_transaction_view, transaction_state_size) == 24u, "AF4 transaction-view size offset drift");

_Static_assert(sizeof(arbor_ddd_unit_of_work) == 64u, "AF4 unit-of-work ABI drift");
_Static_assert(offsetof(arbor_ddd_unit_of_work, transaction_view) == 0u, "AF4 UOW transaction-view offset drift");
_Static_assert(offsetof(arbor_ddd_unit_of_work, event_journal) == 32u, "AF4 UOW journal offset drift");
_Static_assert(offsetof(arbor_ddd_unit_of_work, event_checkpoint) == 40u, "AF4 UOW checkpoint offset drift");
_Static_assert(offsetof(arbor_ddd_unit_of_work, state) == 56u, "AF4 UOW state offset drift");
_Static_assert(offsetof(arbor_ddd_unit_of_work, flags) == 60u, "AF4 UOW flags offset drift");

typedef struct af4_region {
    const void *pointer;
    uint64_t length;
} af4_region;

static arbor_status native_status(int64_t native)
{
    return arbor_status_from_native(native);
}

static arbor_status ok_status(void)
{
    return native_status(0);
}

static int id_is_zero(arbor_capability_id id)
{
    return id.high == UINT64_C(0) && id.low == UINT64_C(0);
}

static int power_of_two_u64(uint64_t value)
{
    return value != UINT64_C(0) && (value & (value - UINT64_C(1))) == UINT64_C(0);
}

static arbor_status region_make(const void *pointer, uint64_t length, af4_region *out)
{
    if (out == NULL) {
        return native_status(-EINVAL);
    }
    if (length != UINT64_C(0) && pointer == NULL) {
        return native_status(-EINVAL);
    }
    if (length != UINT64_C(0)) {
        const uintptr_t start = (uintptr_t)pointer;
        if (length > (uint64_t)(UINTPTR_MAX - start)) {
            return native_status(-EOVERFLOW);
        }
    }
    out->pointer = pointer;
    out->length = length;
    return ok_status();
}

static int regions_overlap(af4_region left, af4_region right)
{
    if (left.length == UINT64_C(0) || right.length == UINT64_C(0)) {
        return 0;
    }

    const uintptr_t left_start = (uintptr_t)left.pointer;
    const uintptr_t right_start = (uintptr_t)right.pointer;
    const uintptr_t left_end = left_start + (uintptr_t)left.length;
    const uintptr_t right_end = right_start + (uintptr_t)right.length;
    return left_start < right_end && right_start < left_end;
}

static arbor_status provider_context_anchor_region(
    const void *provider_context,
    af4_region *out)
{
    return region_make(
        provider_context,
        provider_context == NULL ? UINT64_C(0) : UINT64_C(1),
        out);
}

static arbor_status journal_regions(
    const arbor_ddd_event_journal *journal,
    af4_region *journal_region,
    af4_region *records_region,
    af4_region *payload_region)
{
    if (journal == NULL || journal_region == NULL ||
        records_region == NULL || payload_region == NULL) {
        return native_status(-EINVAL);
    }

    arbor_status status = region_make(
        journal,
        (uint64_t)sizeof(*journal),
        journal_region);
    if (status.native != 0) {
        return status;
    }

    const uint64_t record_bytes =
        (uint64_t)journal->record_capacity * (uint64_t)sizeof(arbor_ddd_event_record);
    status = region_make(journal->records, record_bytes, records_region);
    if (status.native != 0) {
        return status;
    }

    status = region_make(
        journal->payload_bytes,
        (uint64_t)journal->byte_capacity,
        payload_region);
    if (status.native != 0) {
        return status;
    }

    if (regions_overlap(*journal_region, *records_region) ||
        regions_overlap(*journal_region, *payload_region) ||
        regions_overlap(*records_region, *payload_region)) {
        return native_status(-EINVAL);
    }

    return ok_status();
}

static arbor_status journal_shape_validate(const arbor_ddd_event_journal *journal)
{
    if (journal == NULL || journal->generation == UINT64_C(0) ||
        journal->record_count > journal->record_capacity ||
        journal->byte_count > journal->byte_capacity) {
        return native_status(-EINVAL);
    }

    if (journal->record_capacity == 0u) {
        if (journal->records != NULL) {
            return native_status(-EINVAL);
        }
    } else {
        if (journal->records == NULL ||
            ((uintptr_t)journal->records %
                (uintptr_t)_Alignof(arbor_ddd_event_record)) != 0u) {
            return native_status(-EINVAL);
        }
    }

    if (journal->byte_capacity == 0u) {
        if (journal->payload_bytes != NULL) {
            return native_status(-EINVAL);
        }
    } else if (journal->payload_bytes == NULL) {
        return native_status(-EINVAL);
    }

    af4_region journal_region;
    af4_region records_region;
    af4_region payload_region;
    return journal_regions(
        journal,
        &journal_region,
        &records_region,
        &payload_region);
}

static arbor_status journal_record_validate_at(
    const arbor_ddd_event_journal *journal,
    uint32_t index,
    uint32_t expected_payload_offset,
    uint32_t *end_offset_out)
{
    if (journal == NULL || end_offset_out == NULL ||
        index >= journal->record_count) {
        return native_status(-EINVAL);
    }

    const arbor_ddd_event_record *record = &journal->records[index];
    if (id_is_zero(record->type_id) ||
        record->abi_version == 0u ||
        record->flags != ARBOR_DDD_EVENT_FLAGS_NONE ||
        record->payload_offset != expected_payload_offset ||
        record->payload_offset > journal->byte_count ||
        record->payload_size > journal->byte_count - record->payload_offset) {
        return native_status(-EINVAL);
    }

    *end_offset_out = record->payload_offset + record->payload_size;
    return ok_status();
}

static arbor_status journal_checkpoint_validate(
    const arbor_ddd_event_journal *journal,
    const arbor_ddd_event_checkpoint *checkpoint)
{
    arbor_status status = arbor_ddd_event_journal_validate(journal);
    if (status.native != 0) {
        return status;
    }
    if (checkpoint == NULL) {
        return native_status(-EINVAL);
    }
    if (checkpoint->generation != journal->generation) {
        return native_status(-ESTALE);
    }
    if (checkpoint->record_count > journal->record_count ||
        checkpoint->byte_count > journal->byte_count) {
        return native_status(-EINVAL);
    }

    if (checkpoint->record_count == 0u) {
        if (checkpoint->byte_count != 0u) {
            return native_status(-EINVAL);
        }
        return ok_status();
    }

    uint32_t boundary = 0u;
    const arbor_ddd_event_record *last =
        &journal->records[checkpoint->record_count - 1u];
    if (id_is_zero(last->type_id) ||
        last->abi_version == 0u ||
        last->flags != ARBOR_DDD_EVENT_FLAGS_NONE ||
        last->payload_offset > journal->byte_count ||
        last->payload_size > journal->byte_count - last->payload_offset) {
        return native_status(-EINVAL);
    }
    boundary = last->payload_offset + last->payload_size;
    if (boundary != checkpoint->byte_count) {
        return native_status(-EINVAL);
    }
    return ok_status();
}

static arbor_status transaction_interface_shape_validate(
    const arbor_ddd_transaction_interface *interface_table)
{
    if (interface_table == NULL ||
        ((uintptr_t)interface_table %
            (uintptr_t)_Alignof(arbor_ddd_transaction_interface)) != 0u) {
        return native_status(-EINVAL);
    }

    if (interface_table->abi_version != ARBOR_DDD_TRANSACTION_INTERFACE_ABI_VERSION) {
        return native_status(-EPROTONOSUPPORT);
    }
    if (interface_table->struct_size < (uint32_t)sizeof(arbor_ddd_transaction_interface) ||
        interface_table->flags != ARBOR_DDD_TRANSACTION_FLAGS_NONE ||
        id_is_zero(interface_table->authority_id) ||
        !power_of_two_u64(interface_table->state_alignment) ||
        interface_table->state_alignment > ARBOR_DDD_TRANSACTION_MAX_STATE_ALIGNMENT ||
        interface_table->begin == NULL ||
        interface_table->commit == NULL ||
        interface_table->rollback == NULL) {
        return native_status(-EINVAL);
    }
    return ok_status();
}

static arbor_status transaction_state_validate(
    const arbor_ddd_transaction_interface *interface_table,
    void *transaction_state,
    uint64_t transaction_state_size)
{
    if (transaction_state_size != interface_table->state_size) {
        return native_status(-EINVAL);
    }

    if (transaction_state_size == UINT64_C(0)) {
        if (transaction_state != NULL) {
            return native_status(-EINVAL);
        }
        return ok_status();
    }

    if (transaction_state == NULL ||
        ((uintptr_t)transaction_state %
            (uintptr_t)interface_table->state_alignment) != 0u) {
        return native_status(-EINVAL);
    }

    af4_region state_region;
    arbor_status status =
        region_make(transaction_state, transaction_state_size, &state_region);
    if (status.native != 0) {
        return status;
    }

    af4_region interface_region;
    status = region_make(
        interface_table,
        (uint64_t)interface_table->struct_size,
        &interface_region);
    if (status.native != 0) {
        return status;
    }
    if (regions_overlap(state_region, interface_region)) {
        return native_status(-EINVAL);
    }
    return ok_status();
}

static arbor_status callback_status(int64_t native)
{
    if (native > 0) {
        return native_status(-EINVAL);
    }
    return native_status(native);
}

static int unit_of_work_is_zero(const arbor_ddd_unit_of_work *unit_of_work)
{
    if (unit_of_work == NULL) {
        return 0;
    }
    return unit_of_work->transaction_view.interface_table == NULL &&
        unit_of_work->transaction_view.provider_context == NULL &&
        unit_of_work->transaction_view.transaction_state == NULL &&
        unit_of_work->transaction_view.transaction_state_size == UINT64_C(0) &&
        unit_of_work->event_journal == NULL &&
        unit_of_work->event_checkpoint.record_count == 0u &&
        unit_of_work->event_checkpoint.byte_count == 0u &&
        unit_of_work->event_checkpoint.generation == UINT64_C(0) &&
        unit_of_work->state == (uint32_t)ARBOR_DDD_UNIT_OF_WORK_ZERO &&
        unit_of_work->flags == ARBOR_DDD_UNIT_OF_WORK_FLAGS_NONE;
}

static arbor_status unit_of_work_common_validate(
    const arbor_ddd_unit_of_work *unit_of_work)
{
    if (unit_of_work == NULL ||
        unit_of_work->flags != ARBOR_DDD_UNIT_OF_WORK_FLAGS_NONE ||
        unit_of_work->state < (uint32_t)ARBOR_DDD_UNIT_OF_WORK_ACTIVE ||
        unit_of_work->state > (uint32_t)ARBOR_DDD_UNIT_OF_WORK_FAULTED ||
        unit_of_work->event_journal == NULL) {
        return native_status(-EINVAL);
    }

    arbor_status status =
        arbor_ddd_transaction_view_validate(&unit_of_work->transaction_view);
    if (status.native != 0) {
        return status;
    }

    return journal_shape_validate(unit_of_work->event_journal);
}

static arbor_status unit_of_work_active_validate(
    const arbor_ddd_unit_of_work *unit_of_work)
{
    arbor_status status = unit_of_work_common_validate(unit_of_work);
    if (status.native != 0) {
        return status;
    }
    if (unit_of_work->state != (uint32_t)ARBOR_DDD_UNIT_OF_WORK_ACTIVE) {
        return native_status(-EALREADY);
    }
    return journal_checkpoint_validate(
        unit_of_work->event_journal,
        &unit_of_work->event_checkpoint);
}

arbor_status arbor_ddd_event_journal_validate(
    const arbor_ddd_event_journal *journal)
{
    arbor_status status = journal_shape_validate(journal);
    if (status.native != 0) {
        return status;
    }

    uint32_t expected_payload_offset = 0u;
    for (uint32_t i = 0u; i < journal->record_count; ++i) {
        uint32_t end_offset = 0u;
        status = journal_record_validate_at(
            journal,
            i,
            expected_payload_offset,
            &end_offset);
        if (status.native != 0) {
            return status;
        }
        expected_payload_offset = end_offset;
    }

    if (expected_payload_offset != journal->byte_count) {
        return native_status(-EINVAL);
    }
    return ok_status();
}

arbor_status arbor_ddd_event_journal_init(
    arbor_ddd_event_record *records,
    uint32_t record_capacity,
    uint8_t *payload_bytes,
    uint32_t byte_capacity,
    arbor_ddd_event_journal *out)
{
    if (out == NULL) {
        return native_status(-EINVAL);
    }

    arbor_ddd_event_journal candidate;
    candidate.records = records;
    candidate.record_capacity = record_capacity;
    candidate.record_count = 0u;
    candidate.payload_bytes = payload_bytes;
    candidate.byte_capacity = byte_capacity;
    candidate.byte_count = 0u;
    candidate.generation = UINT64_C(1);

    arbor_status status = journal_shape_validate(&candidate);
    if (status.native != 0) {
        return status;
    }

    af4_region out_region;
    af4_region journal_region;
    af4_region records_region;
    af4_region payload_region;

    status = region_make(out, (uint64_t)sizeof(*out), &out_region);
    if (status.native != 0) {
        return status;
    }
    status = journal_regions(
        &candidate,
        &journal_region,
        &records_region,
        &payload_region);
    if (status.native != 0) {
        return status;
    }

    if (regions_overlap(out_region, records_region) ||
        regions_overlap(out_region, payload_region)) {
        return native_status(-EINVAL);
    }

    (void)memory_copy(out, &candidate, (uint64_t)sizeof(candidate));
    return ok_status();
}

arbor_status arbor_ddd_event_journal_checkpoint(
    const arbor_ddd_event_journal *journal,
    arbor_ddd_event_checkpoint *out)
{
    arbor_status status = arbor_ddd_event_journal_validate(journal);
    if (status.native != 0) {
        return status;
    }
    if (out == NULL) {
        return native_status(-EINVAL);
    }

    af4_region out_region;
    af4_region journal_region;
    af4_region records_region;
    af4_region payload_region;

    status = region_make(out, (uint64_t)sizeof(*out), &out_region);
    if (status.native != 0) {
        return status;
    }
    status = journal_regions(
        journal,
        &journal_region,
        &records_region,
        &payload_region);
    if (status.native != 0) {
        return status;
    }
    if (regions_overlap(out_region, journal_region) ||
        regions_overlap(out_region, records_region) ||
        regions_overlap(out_region, payload_region)) {
        return native_status(-EINVAL);
    }

    arbor_ddd_event_checkpoint candidate;
    candidate.record_count = journal->record_count;
    candidate.byte_count = journal->byte_count;
    candidate.generation = journal->generation;
    (void)memory_copy(out, &candidate, (uint64_t)sizeof(candidate));
    return ok_status();
}

arbor_status arbor_ddd_event_journal_append(
    arbor_ddd_event_journal *journal,
    arbor_ddd_event_type_id type_id,
    uint32_t abi_version,
    uint32_t flags,
    const void *payload,
    uint32_t payload_size,
    uint32_t *sequence_out)
{
    arbor_status status = journal_shape_validate(journal);
    if (status.native != 0) {
        return status;
    }
    if (id_is_zero(type_id) || abi_version == 0u ||
        flags != ARBOR_DDD_EVENT_FLAGS_NONE ||
        (payload_size != 0u && payload == NULL) ||
        sequence_out == NULL) {
        return native_status(-EINVAL);
    }
    if (journal->record_count == journal->record_capacity ||
        payload_size > journal->byte_capacity - journal->byte_count) {
        return native_status(-ENOSPC);
    }

    af4_region journal_region;
    af4_region records_region;
    af4_region payload_region;
    af4_region sequence_region;
    af4_region source_region;

    status = journal_regions(
        journal,
        &journal_region,
        &records_region,
        &payload_region);
    if (status.native != 0) {
        return status;
    }
    status = region_make(
        sequence_out,
        (uint64_t)sizeof(*sequence_out),
        &sequence_region);
    if (status.native != 0) {
        return status;
    }
    status = region_make(payload, (uint64_t)payload_size, &source_region);
    if (status.native != 0) {
        return status;
    }

    if (regions_overlap(sequence_region, journal_region) ||
        regions_overlap(sequence_region, records_region) ||
        regions_overlap(sequence_region, payload_region) ||
        regions_overlap(sequence_region, source_region) ||
        regions_overlap(source_region, journal_region) ||
        regions_overlap(source_region, records_region) ||
        regions_overlap(source_region, payload_region)) {
        return native_status(-EINVAL);
    }

    const uint32_t sequence = journal->record_count;
    const uint32_t payload_offset = journal->byte_count;

    arbor_ddd_event_record record;
    record.type_id = type_id;
    record.abi_version = abi_version;
    record.flags = flags;
    record.payload_offset = payload_offset;
    record.payload_size = payload_size;

    if (payload_size != 0u) {
        (void)memory_copy(
            journal->payload_bytes + payload_offset,
            payload,
            (uint64_t)payload_size);
    }
    (void)memory_copy(
        &journal->records[sequence],
        &record,
        (uint64_t)sizeof(record));

    journal->byte_count = payload_offset + payload_size;
    journal->record_count = sequence + 1u;
    *sequence_out = sequence;
    return ok_status();
}

arbor_status arbor_ddd_event_journal_rewind(
    arbor_ddd_event_journal *journal,
    const arbor_ddd_event_checkpoint *checkpoint)
{
    arbor_status status = journal_checkpoint_validate(journal, checkpoint);
    if (status.native != 0) {
        return status;
    }

    af4_region checkpoint_region;
    af4_region journal_region;
    af4_region records_region;
    af4_region payload_region;

    status = region_make(
        checkpoint,
        (uint64_t)sizeof(*checkpoint),
        &checkpoint_region);
    if (status.native != 0) {
        return status;
    }
    status = journal_regions(
        journal,
        &journal_region,
        &records_region,
        &payload_region);
    if (status.native != 0) {
        return status;
    }
    if (regions_overlap(checkpoint_region, journal_region) ||
        regions_overlap(checkpoint_region, records_region) ||
        regions_overlap(checkpoint_region, payload_region)) {
        return native_status(-EINVAL);
    }

    journal->record_count = checkpoint->record_count;
    journal->byte_count = checkpoint->byte_count;
    return ok_status();
}

arbor_status arbor_ddd_event_journal_view(
    const arbor_ddd_event_journal *journal,
    uint32_t sequence,
    arbor_ddd_event_view *out)
{
    arbor_status status = journal_shape_validate(journal);
    if (status.native != 0) {
        return status;
    }
    if (out == NULL) {
        return native_status(-EINVAL);
    }
    if (sequence >= journal->record_count) {
        return native_status(-ENOENT);
    }

    const arbor_ddd_event_record *record = &journal->records[sequence];
    if (id_is_zero(record->type_id) || record->abi_version == 0u ||
        record->flags != ARBOR_DDD_EVENT_FLAGS_NONE ||
        record->payload_offset > journal->byte_count ||
        record->payload_size > journal->byte_count - record->payload_offset) {
        return native_status(-EINVAL);
    }

    af4_region out_region;
    af4_region journal_region;
    af4_region records_region;
    af4_region payload_region;

    status = region_make(out, (uint64_t)sizeof(*out), &out_region);
    if (status.native != 0) {
        return status;
    }
    status = journal_regions(
        journal,
        &journal_region,
        &records_region,
        &payload_region);
    if (status.native != 0) {
        return status;
    }
    if (regions_overlap(out_region, journal_region) ||
        regions_overlap(out_region, records_region) ||
        regions_overlap(out_region, payload_region)) {
        return native_status(-EINVAL);
    }

    arbor_ddd_event_view candidate;
    candidate.type_id = record->type_id;
    candidate.abi_version = record->abi_version;
    candidate.flags = record->flags;
    candidate.payload =
        record->payload_size == 0u
            ? NULL
            : journal->payload_bytes + record->payload_offset;
    candidate.payload_size = record->payload_size;
    candidate.sequence = sequence;

    (void)memory_copy(out, &candidate, (uint64_t)sizeof(candidate));
    return ok_status();
}

arbor_status arbor_ddd_event_journal_clear(
    arbor_ddd_event_journal *journal)
{
    arbor_status status = journal_shape_validate(journal);
    if (status.native != 0) {
        return status;
    }
    if (journal->generation == UINT64_MAX) {
        return native_status(-EOVERFLOW);
    }

    journal->record_count = 0u;
    journal->byte_count = 0u;
    journal->generation += UINT64_C(1);
    return ok_status();
}

arbor_status arbor_ddd_transaction_interface_validate(
    const arbor_capability_binding *binding)
{
    if (binding == NULL || id_is_zero(binding->id) ||
        binding->version.major == 0u ||
        binding->reserved0 != 0u ||
        binding->flags != ARBOR_CAPABILITY_FLAGS_NONE ||
        binding->interface_table == NULL ||
        binding->interface_size < (uint32_t)sizeof(arbor_ddd_transaction_interface)) {
        return native_status(-EINVAL);
    }

    const arbor_ddd_transaction_interface *interface_table =
        (const arbor_ddd_transaction_interface *)binding->interface_table;

    arbor_status status = transaction_interface_shape_validate(interface_table);
    if (status.native != 0) {
        return status;
    }
    if (interface_table->struct_size != binding->interface_size) {
        return native_status(-EINVAL);
    }

    af4_region binding_region;
    af4_region interface_region;
    af4_region provider_anchor;

    status = region_make(
        binding,
        (uint64_t)sizeof(*binding),
        &binding_region);
    if (status.native != 0) {
        return status;
    }
    status = region_make(
        interface_table,
        (uint64_t)interface_table->struct_size,
        &interface_region);
    if (status.native != 0) {
        return status;
    }
    status = provider_context_anchor_region(
        binding->provider_context,
        &provider_anchor);
    if (status.native != 0) {
        return status;
    }
    if (regions_overlap(provider_anchor, binding_region) ||
        regions_overlap(provider_anchor, interface_region)) {
        return native_status(-EINVAL);
    }

    return ok_status();
}

arbor_status arbor_ddd_transaction_view_validate(
    const arbor_ddd_transaction_view *view)
{
    if (view == NULL) {
        return native_status(-EINVAL);
    }

    arbor_status status =
        transaction_interface_shape_validate(view->interface_table);
    if (status.native != 0) {
        return status;
    }
    status = transaction_state_validate(
        view->interface_table,
        view->transaction_state,
        view->transaction_state_size);
    if (status.native != 0) {
        return status;
    }

    af4_region view_region;
    af4_region interface_region;
    af4_region state_region;
    af4_region provider_anchor;

    status = region_make(view, (uint64_t)sizeof(*view), &view_region);
    if (status.native != 0) {
        return status;
    }
    status = region_make(
        view->interface_table,
        (uint64_t)view->interface_table->struct_size,
        &interface_region);
    if (status.native != 0) {
        return status;
    }
    status = region_make(
        view->transaction_state,
        view->transaction_state_size,
        &state_region);
    if (status.native != 0) {
        return status;
    }
    status = provider_context_anchor_region(
        view->provider_context,
        &provider_anchor);
    if (status.native != 0) {
        return status;
    }
    if (regions_overlap(provider_anchor, view_region) ||
        regions_overlap(provider_anchor, interface_region) ||
        regions_overlap(provider_anchor, state_region) ||
        regions_overlap(view_region, interface_region) ||
        regions_overlap(view_region, state_region)) {
        return native_status(-EINVAL);
    }
    return ok_status();
}

arbor_status arbor_ddd_unit_of_work_begin(
    const arbor_capability_binding *transaction_binding,
    void *transaction_state,
    uint64_t transaction_state_size,
    arbor_ddd_event_journal *event_journal,
    arbor_ddd_unit_of_work *out)
{
    if (out == NULL || !unit_of_work_is_zero(out)) {
        return native_status(-EINVAL);
    }

    arbor_status status =
        arbor_ddd_transaction_interface_validate(transaction_binding);
    if (status.native != 0) {
        return status;
    }

    const arbor_ddd_transaction_interface *interface_table =
        (const arbor_ddd_transaction_interface *)transaction_binding->interface_table;

    status = transaction_state_validate(
        interface_table,
        transaction_state,
        transaction_state_size);
    if (status.native != 0) {
        return status;
    }

    status = arbor_ddd_event_journal_validate(event_journal);
    if (status.native != 0) {
        return status;
    }

    arbor_ddd_event_checkpoint checkpoint;
    checkpoint.record_count = event_journal->record_count;
    checkpoint.byte_count = event_journal->byte_count;
    checkpoint.generation = event_journal->generation;

    af4_region out_region;
    af4_region binding_region;
    af4_region interface_region;
    af4_region state_region;
    af4_region journal_region;
    af4_region records_region;
    af4_region payload_region;
    af4_region provider_anchor;

    status = region_make(out, (uint64_t)sizeof(*out), &out_region);
    if (status.native != 0) {
        return status;
    }
    status = region_make(
        transaction_binding,
        (uint64_t)sizeof(*transaction_binding),
        &binding_region);
    if (status.native != 0) {
        return status;
    }
    status = region_make(
        interface_table,
        (uint64_t)interface_table->struct_size,
        &interface_region);
    if (status.native != 0) {
        return status;
    }
    status = region_make(transaction_state, transaction_state_size, &state_region);
    if (status.native != 0) {
        return status;
    }
    status = journal_regions(
        event_journal,
        &journal_region,
        &records_region,
        &payload_region);
    if (status.native != 0) {
        return status;
    }
    status = provider_context_anchor_region(
        transaction_binding->provider_context,
        &provider_anchor);
    if (status.native != 0) {
        return status;
    }

    if (regions_overlap(provider_anchor, out_region) ||
        regions_overlap(provider_anchor, binding_region) ||
        regions_overlap(provider_anchor, interface_region) ||
        regions_overlap(provider_anchor, state_region) ||
        regions_overlap(provider_anchor, journal_region) ||
        regions_overlap(provider_anchor, records_region) ||
        regions_overlap(provider_anchor, payload_region) ||
        regions_overlap(out_region, binding_region) ||
        regions_overlap(out_region, interface_region) ||
        regions_overlap(out_region, state_region) ||
        regions_overlap(out_region, journal_region) ||
        regions_overlap(out_region, records_region) ||
        regions_overlap(out_region, payload_region) ||
        regions_overlap(state_region, binding_region) ||
        regions_overlap(state_region, interface_region) ||
        regions_overlap(state_region, journal_region) ||
        regions_overlap(state_region, records_region) ||
        regions_overlap(state_region, payload_region) ||
        regions_overlap(journal_region, binding_region) ||
        regions_overlap(journal_region, interface_region) ||
        regions_overlap(records_region, binding_region) ||
        regions_overlap(records_region, interface_region) ||
        regions_overlap(payload_region, binding_region) ||
        regions_overlap(payload_region, interface_region)) {
        return native_status(-EINVAL);
    }

    if (transaction_state_size != UINT64_C(0)) {
        (void)memory_zero(transaction_state, transaction_state_size);
    }

    const int64_t begin_native = interface_table->begin(
        transaction_binding->provider_context,
        transaction_state,
        transaction_state_size);
    status = callback_status(begin_native);
    if (status.native != 0) {
        return status;
    }

    arbor_ddd_unit_of_work candidate;
    candidate.transaction_view.interface_table = interface_table;
    candidate.transaction_view.provider_context =
        transaction_binding->provider_context;
    candidate.transaction_view.transaction_state = transaction_state;
    candidate.transaction_view.transaction_state_size = transaction_state_size;
    candidate.event_journal = event_journal;
    candidate.event_checkpoint = checkpoint;
    candidate.state = (uint32_t)ARBOR_DDD_UNIT_OF_WORK_ACTIVE;
    candidate.flags = ARBOR_DDD_UNIT_OF_WORK_FLAGS_NONE;

    (void)memory_copy(out, &candidate, (uint64_t)sizeof(candidate));
    return ok_status();
}

arbor_status arbor_ddd_unit_of_work_commit(
    arbor_ddd_unit_of_work *unit_of_work)
{
    arbor_status status = unit_of_work_active_validate(unit_of_work);
    if (status.native != 0) {
        return status;
    }

    const arbor_ddd_transaction_interface *interface_table =
        unit_of_work->transaction_view.interface_table;

    const int64_t commit_native = interface_table->commit(
        unit_of_work->transaction_view.provider_context,
        unit_of_work->transaction_view.transaction_state);
    const arbor_status commit_status = callback_status(commit_native);

    if (commit_status.native == 0) {
        unit_of_work->state = (uint32_t)ARBOR_DDD_UNIT_OF_WORK_COMMITTED;
        return commit_status;
    }

    const arbor_status rewind_status = arbor_ddd_event_journal_rewind(
        unit_of_work->event_journal,
        &unit_of_work->event_checkpoint);
    unit_of_work->state = (uint32_t)ARBOR_DDD_UNIT_OF_WORK_FAULTED;

    if (rewind_status.native != 0) {
        return commit_status;
    }
    return commit_status;
}

arbor_status arbor_ddd_unit_of_work_rollback(
    arbor_ddd_unit_of_work *unit_of_work)
{
    arbor_status status = unit_of_work_active_validate(unit_of_work);
    if (status.native != 0) {
        return status;
    }

    const arbor_ddd_transaction_interface *interface_table =
        unit_of_work->transaction_view.interface_table;

    const int64_t rollback_native = interface_table->rollback(
        unit_of_work->transaction_view.provider_context,
        unit_of_work->transaction_view.transaction_state);
    const arbor_status rollback_status = callback_status(rollback_native);

    const arbor_status rewind_status = arbor_ddd_event_journal_rewind(
        unit_of_work->event_journal,
        &unit_of_work->event_checkpoint);

    if (rollback_status.native == 0 && rewind_status.native == 0) {
        unit_of_work->state = (uint32_t)ARBOR_DDD_UNIT_OF_WORK_ROLLED_BACK;
        return rollback_status;
    }

    unit_of_work->state = (uint32_t)ARBOR_DDD_UNIT_OF_WORK_FAULTED;
    if (rollback_status.native != 0) {
        return rollback_status;
    }
    return rewind_status;
}

arbor_status arbor_ddd_unit_of_work_active_transaction(
    const arbor_ddd_unit_of_work *unit_of_work,
    arbor_ddd_transaction_view *out)
{
    arbor_status status = unit_of_work_active_validate(unit_of_work);
    if (status.native != 0) {
        return status;
    }
    if (out == NULL) {
        return native_status(-EINVAL);
    }

    af4_region out_region;
    af4_region unit_region;
    af4_region interface_region;
    af4_region state_region;
    af4_region journal_region;
    af4_region records_region;
    af4_region payload_region;
    af4_region provider_anchor;

    status = region_make(out, (uint64_t)sizeof(*out), &out_region);
    if (status.native != 0) {
        return status;
    }
    status = region_make(
        unit_of_work,
        (uint64_t)sizeof(*unit_of_work),
        &unit_region);
    if (status.native != 0) {
        return status;
    }
    status = region_make(
        unit_of_work->transaction_view.interface_table,
        (uint64_t)unit_of_work->transaction_view.interface_table->struct_size,
        &interface_region);
    if (status.native != 0) {
        return status;
    }
    status = region_make(
        unit_of_work->transaction_view.transaction_state,
        unit_of_work->transaction_view.transaction_state_size,
        &state_region);
    if (status.native != 0) {
        return status;
    }
    status = journal_regions(
        unit_of_work->event_journal,
        &journal_region,
        &records_region,
        &payload_region);
    if (status.native != 0) {
        return status;
    }
    status = provider_context_anchor_region(
        unit_of_work->transaction_view.provider_context,
        &provider_anchor);
    if (status.native != 0) {
        return status;
    }

    if (regions_overlap(out_region, provider_anchor) ||
        regions_overlap(out_region, unit_region) ||
        regions_overlap(out_region, interface_region) ||
        regions_overlap(out_region, state_region) ||
        regions_overlap(out_region, journal_region) ||
        regions_overlap(out_region, records_region) ||
        regions_overlap(out_region, payload_region)) {
        return native_status(-EINVAL);
    }

    arbor_ddd_transaction_view candidate = unit_of_work->transaction_view;
    (void)memory_copy(out, &candidate, (uint64_t)sizeof(candidate));
    return ok_status();
}

arbor_status arbor_ddd_unit_of_work_reset(
    arbor_ddd_unit_of_work *unit_of_work)
{
    if (unit_of_work == NULL ||
        unit_of_work->flags != ARBOR_DDD_UNIT_OF_WORK_FLAGS_NONE) {
        return native_status(-EINVAL);
    }
    if (unit_of_work->state == (uint32_t)ARBOR_DDD_UNIT_OF_WORK_ACTIVE) {
        return native_status(-EBUSY);
    }
    if (unit_of_work->state != (uint32_t)ARBOR_DDD_UNIT_OF_WORK_COMMITTED &&
        unit_of_work->state != (uint32_t)ARBOR_DDD_UNIT_OF_WORK_ROLLED_BACK &&
        unit_of_work->state != (uint32_t)ARBOR_DDD_UNIT_OF_WORK_FAULTED) {
        return native_status(-EINVAL);
    }

    (void)memory_zero(unit_of_work, (uint64_t)sizeof(*unit_of_work));
    return ok_status();
}
