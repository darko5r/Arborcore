#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <arborcore/profile.h>

#define PROFILE_SESSION_GUARD_SEED UINT64_C(0x50524f46494c4530)
#define PROFILE_SPAN_GUARD_SEED UINT64_C(0x50524f465350414e)
#define PROFILE_FNV_PRIME UINT64_C(1099511628211)

_Static_assert(sizeof(arbor_profile_region_descriptor) == 8u,
               "PROFILE0 region descriptor layout drift");
_Static_assert(_Alignof(arbor_profile_region_descriptor) == 8u,
               "PROFILE0 region descriptor alignment drift");
_Static_assert(offsetof(arbor_profile_region_descriptor, id) == 0u,
               "PROFILE0 region descriptor id offset drift");

_Static_assert(sizeof(arbor_profile_aggregate) == 32u,
               "PROFILE0 aggregate layout drift");
_Static_assert(_Alignof(arbor_profile_aggregate) == 8u,
               "PROFILE0 aggregate alignment drift");
_Static_assert(offsetof(arbor_profile_aggregate, sample_count) == 0u,
               "PROFILE0 aggregate sample_count offset drift");
_Static_assert(offsetof(arbor_profile_aggregate, total_ns) == 8u,
               "PROFILE0 aggregate total_ns offset drift");
_Static_assert(offsetof(arbor_profile_aggregate, min_ns) == 16u,
               "PROFILE0 aggregate min_ns offset drift");
_Static_assert(offsetof(arbor_profile_aggregate, max_ns) == 24u,
               "PROFILE0 aggregate max_ns offset drift");

_Static_assert(sizeof(arbor_profile_region_result) == 40u,
               "PROFILE0 region result layout drift");
_Static_assert(_Alignof(arbor_profile_region_result) == 8u,
               "PROFILE0 region result alignment drift");
_Static_assert(offsetof(arbor_profile_region_result, id) == 0u,
               "PROFILE0 region result id offset drift");
_Static_assert(offsetof(arbor_profile_region_result, aggregate) == 8u,
               "PROFILE0 region result aggregate offset drift");

_Static_assert(sizeof(arbor_profile_session) == 72u,
               "PROFILE0 session layout drift");
_Static_assert(_Alignof(arbor_profile_session) == 8u,
               "PROFILE0 session alignment drift");
_Static_assert(offsetof(arbor_profile_session, abi_version) == 0u,
               "PROFILE0 session abi offset drift");
_Static_assert(offsetof(arbor_profile_session, struct_size) == 4u,
               "PROFILE0 session size offset drift");
_Static_assert(offsetof(arbor_profile_session, flags) == 8u,
               "PROFILE0 session flags offset drift");
_Static_assert(offsetof(arbor_profile_session, descriptors) == 16u,
               "PROFILE0 session descriptors offset drift");
_Static_assert(offsetof(arbor_profile_session, aggregates) == 24u,
               "PROFILE0 session aggregates offset drift");
_Static_assert(offsetof(arbor_profile_session, region_count) == 32u,
               "PROFILE0 session region_count offset drift");
_Static_assert(offsetof(arbor_profile_session, clock) == 40u,
               "PROFILE0 session clock offset drift");
_Static_assert(offsetof(arbor_profile_session, clock_context) == 48u,
               "PROFILE0 session clock_context offset drift");
_Static_assert(offsetof(arbor_profile_session, identity) == 56u,
               "PROFILE0 session identity offset drift");
_Static_assert(offsetof(arbor_profile_session, prepared_guard) == 64u,
               "PROFILE0 session guard offset drift");

_Static_assert(sizeof(arbor_profile_span) == 64u,
               "PROFILE0 span layout drift");
_Static_assert(_Alignof(arbor_profile_span) == 8u,
               "PROFILE0 span alignment drift");
_Static_assert(offsetof(arbor_profile_span, abi_version) == 0u,
               "PROFILE0 span abi offset drift");
_Static_assert(offsetof(arbor_profile_span, struct_size) == 4u,
               "PROFILE0 span size offset drift");
_Static_assert(offsetof(arbor_profile_span, flags) == 8u,
               "PROFILE0 span flags offset drift");
_Static_assert(offsetof(arbor_profile_span, session) == 16u,
               "PROFILE0 span session offset drift");
_Static_assert(offsetof(arbor_profile_span, region_index) == 24u,
               "PROFILE0 span region_index offset drift");
_Static_assert(offsetof(arbor_profile_span, start_ns) == 32u,
               "PROFILE0 span start_ns offset drift");
_Static_assert(offsetof(arbor_profile_span, identity) == 40u,
               "PROFILE0 span identity offset drift");
_Static_assert(offsetof(arbor_profile_span, prepared_guard) == 48u,
               "PROFILE0 span guard offset drift");
_Static_assert(offsetof(arbor_profile_span, active) == 56u,
               "PROFILE0 span active offset drift");
_Static_assert(offsetof(arbor_profile_span, reserved0) == 57u,
               "PROFILE0 span reserved offset drift");

static arbor_status profile_status(int64_t native)
{
    return arbor_status_from_native(native);
}

static uint64_t profile_guard_mix(
    uint64_t state,
    const void *data,
    uint64_t length)
{
    const uint8_t *bytes = (const uint8_t *)data;
    for (uint64_t index = 0u; index < length; ++index) {
        state ^= (uint64_t)bytes[index];
        state *= PROFILE_FNV_PRIME;
    }
    return state;
}

static bool profile_range_valid(const void *pointer, uint64_t length)
{
    if (pointer == NULL || length == 0u) {
        return false;
    }
    arbor_asm_result_u64 result = range_end_checked(
        (uint64_t)(uintptr_t)pointer,
        length);
    return result.status == 0;
}

static bool profile_ranges_overlap(
    const void *left,
    uint64_t left_length,
    const void *right,
    uint64_t right_length)
{
    if (!profile_range_valid(left, left_length) ||
        !profile_range_valid(right, right_length)) {
        return true;
    }
    arbor_asm_result_u64 result = range_overlaps(
        (uint64_t)(uintptr_t)left,
        left_length,
        (uint64_t)(uintptr_t)right,
        right_length);
    return result.status != 0 || result.value != 0u;
}

static arbor_status profile_array_bytes(
    uint64_t count,
    uint64_t element_size,
    uint64_t *bytes_out)
{
    if (bytes_out == NULL || count == 0u || element_size == 0u) {
        return profile_status(-EINVAL);
    }
    arbor_asm_result_u64 product = u64_mul_checked(count, element_size);
    if (product.status != 0) {
        return profile_status(product.status);
    }
    if (product.value == 0u) {
        return profile_status(-EINVAL);
    }
    *bytes_out = product.value;
    return profile_status(0);
}

static bool profile_region_ids_valid(
    const arbor_profile_region_descriptor *descriptors,
    uint64_t count)
{
    for (uint64_t index = 0u; index < count; ++index) {
        if (descriptors[index].id == 0u) {
            return false;
        }
        for (uint64_t prior = 0u; prior < index; ++prior) {
            if (descriptors[prior].id == descriptors[index].id) {
                return false;
            }
        }
    }
    return true;
}

static uint64_t profile_session_guard(
    const arbor_profile_session *session)
{
    uint64_t state = PROFILE_SESSION_GUARD_SEED;
    state = profile_guard_mix(state, &session->abi_version,
                              (uint64_t)sizeof(session->abi_version));
    state = profile_guard_mix(state, &session->struct_size,
                              (uint64_t)sizeof(session->struct_size));
    state = profile_guard_mix(state, &session->flags,
                              (uint64_t)sizeof(session->flags));
    state = profile_guard_mix(state, &session->descriptors,
                              (uint64_t)sizeof(session->descriptors));
    state = profile_guard_mix(state, &session->aggregates,
                              (uint64_t)sizeof(session->aggregates));
    state = profile_guard_mix(state, &session->region_count,
                              (uint64_t)sizeof(session->region_count));
    state = profile_guard_mix(state, &session->clock,
                              (uint64_t)sizeof(session->clock));
    state = profile_guard_mix(state, &session->clock_context,
                              (uint64_t)sizeof(session->clock_context));
    state = profile_guard_mix(state, &session->identity,
                              (uint64_t)sizeof(session->identity));
    for (uint64_t index = 0u; index < session->region_count; ++index) {
        state = profile_guard_mix(
            state,
            &session->descriptors[index].id,
            (uint64_t)sizeof(session->descriptors[index].id));
    }
    return ~state;
}

static arbor_status profile_session_runtime_validate(
    const arbor_profile_session *session,
    uint64_t *descriptor_bytes_out,
    uint64_t *aggregate_bytes_out)
{
    if (session == NULL ||
        session->abi_version != ARBOR_PROFILE_ABI_VERSION ||
        session->struct_size != (uint32_t)sizeof(*session) ||
        session->flags != ARBOR_PROFILE_FLAGS_NONE ||
        session->descriptors == NULL ||
        session->aggregates == NULL ||
        session->region_count == 0u ||
        session->clock == NULL ||
        session->identity != session) {
        return profile_status(-EINVAL);
    }

    uint64_t descriptor_bytes = 0u;
    uint64_t aggregate_bytes = 0u;
    arbor_status status = profile_array_bytes(
        session->region_count,
        (uint64_t)sizeof(*session->descriptors),
        &descriptor_bytes);
    if (status.native != 0) {
        return status;
    }
    status = profile_array_bytes(
        session->region_count,
        (uint64_t)sizeof(*session->aggregates),
        &aggregate_bytes);
    if (status.native != 0) {
        return status;
    }

    if (!profile_range_valid(session, (uint64_t)sizeof(*session)) ||
        !profile_range_valid(session->descriptors, descriptor_bytes) ||
        !profile_range_valid(session->aggregates, aggregate_bytes) ||
        profile_ranges_overlap(
            session,
            (uint64_t)sizeof(*session),
            session->descriptors,
            descriptor_bytes) ||
        profile_ranges_overlap(
            session,
            (uint64_t)sizeof(*session),
            session->aggregates,
            aggregate_bytes) ||
        profile_ranges_overlap(
            session->descriptors,
            descriptor_bytes,
            session->aggregates,
            aggregate_bytes) ||
        !profile_region_ids_valid(session->descriptors, session->region_count) ||
        session->prepared_guard != profile_session_guard(session)) {
        return profile_status(-EINVAL);
    }

    if (descriptor_bytes_out != NULL) {
        *descriptor_bytes_out = descriptor_bytes;
    }
    if (aggregate_bytes_out != NULL) {
        *aggregate_bytes_out = aggregate_bytes;
    }
    return profile_status(0);
}

static bool profile_object_all_zero(const void *object, uint64_t length)
{
    const uint8_t *bytes = (const uint8_t *)object;
    for (uint64_t index = 0u; index < length; ++index) {
        if (bytes[index] != 0u) {
            return false;
        }
    }
    return true;
}

static bool profile_span_reserved_zero(const arbor_profile_span *span)
{
    for (uint64_t index = 0u; index < (uint64_t)sizeof(span->reserved0); ++index) {
        if (span->reserved0[index] != 0u) {
            return false;
        }
    }
    return true;
}

static uint64_t profile_span_guard(const arbor_profile_span *span)
{
    uint64_t state = PROFILE_SPAN_GUARD_SEED;
    state = profile_guard_mix(state, &span->abi_version,
                              (uint64_t)sizeof(span->abi_version));
    state = profile_guard_mix(state, &span->struct_size,
                              (uint64_t)sizeof(span->struct_size));
    state = profile_guard_mix(state, &span->flags,
                              (uint64_t)sizeof(span->flags));
    state = profile_guard_mix(state, &span->session,
                              (uint64_t)sizeof(span->session));
    state = profile_guard_mix(state, &span->region_index,
                              (uint64_t)sizeof(span->region_index));
    state = profile_guard_mix(state, &span->start_ns,
                              (uint64_t)sizeof(span->start_ns));
    state = profile_guard_mix(state, &span->identity,
                              (uint64_t)sizeof(span->identity));
    state = profile_guard_mix(state, &span->active,
                              (uint64_t)sizeof(span->active));
    state = profile_guard_mix(state, span->reserved0,
                              (uint64_t)sizeof(span->reserved0));
    return ~state;
}

static arbor_status profile_span_runtime_validate(
    const arbor_profile_span *span)
{
    if (span == NULL ||
        !span->active ||
        span->abi_version != ARBOR_PROFILE_ABI_VERSION ||
        span->struct_size != (uint32_t)sizeof(*span) ||
        span->flags != ARBOR_PROFILE_FLAGS_NONE ||
        span->session == NULL ||
        span->start_ns < 0 ||
        span->identity != span ||
        !profile_span_reserved_zero(span) ||
        span->prepared_guard != profile_span_guard(span)) {
        return profile_status(-EINVAL);
    }

    uint64_t descriptor_bytes = 0u;
    uint64_t aggregate_bytes = 0u;
    arbor_status status = profile_session_runtime_validate(
        span->session,
        &descriptor_bytes,
        &aggregate_bytes);
    if (status.native != 0) {
        return status;
    }
    if (span->region_index >= span->session->region_count ||
        profile_ranges_overlap(
            span,
            (uint64_t)sizeof(*span),
            span->session,
            (uint64_t)sizeof(*span->session)) ||
        profile_ranges_overlap(
            span,
            (uint64_t)sizeof(*span),
            span->session->descriptors,
            descriptor_bytes) ||
        profile_ranges_overlap(
            span,
            (uint64_t)sizeof(*span),
            span->session->aggregates,
            aggregate_bytes)) {
        return profile_status(-EINVAL);
    }
    return profile_status(0);
}

arbor_status arbor_profile_session_prepare(
    arbor_profile_session *session,
    const arbor_profile_region_descriptor *descriptors,
    arbor_profile_aggregate *aggregates,
    uint64_t region_count,
    arbor_profile_clock_fn clock,
    void *clock_context)
{
    if (session == NULL || descriptors == NULL || aggregates == NULL ||
        region_count == 0u || clock == NULL) {
        return profile_status(-EINVAL);
    }

    uint64_t descriptor_bytes = 0u;
    uint64_t aggregate_bytes = 0u;
    arbor_status status = profile_array_bytes(
        region_count,
        (uint64_t)sizeof(*descriptors),
        &descriptor_bytes);
    if (status.native != 0) {
        return status;
    }
    status = profile_array_bytes(
        region_count,
        (uint64_t)sizeof(*aggregates),
        &aggregate_bytes);
    if (status.native != 0) {
        return status;
    }

    if (!profile_range_valid(session, (uint64_t)sizeof(*session)) ||
        !profile_range_valid(descriptors, descriptor_bytes) ||
        !profile_range_valid(aggregates, aggregate_bytes) ||
        profile_ranges_overlap(
            session,
            (uint64_t)sizeof(*session),
            descriptors,
            descriptor_bytes) ||
        profile_ranges_overlap(
            session,
            (uint64_t)sizeof(*session),
            aggregates,
            aggregate_bytes) ||
        profile_ranges_overlap(
            descriptors,
            descriptor_bytes,
            aggregates,
            aggregate_bytes) ||
        !profile_region_ids_valid(descriptors, region_count)) {
        return profile_status(-EINVAL);
    }

    arbor_profile_session candidate = {
        ARBOR_PROFILE_ABI_VERSION,
        (uint32_t)sizeof(arbor_profile_session),
        ARBOR_PROFILE_FLAGS_NONE,
        descriptors,
        aggregates,
        region_count,
        clock,
        clock_context,
        session,
        0u
    };
    candidate.prepared_guard = profile_session_guard(&candidate);

    for (uint64_t index = 0u; index < region_count; ++index) {
        aggregates[index] = (arbor_profile_aggregate){0u, 0u, 0u, 0u};
    }
    *session = candidate;
    return profile_status(0);
}

arbor_status arbor_profile_session_validate(
    const arbor_profile_session *session)
{
    return profile_session_runtime_validate(session, NULL, NULL);
}

arbor_status arbor_profile_session_reset(
    arbor_profile_session *session)
{
    arbor_status status = profile_session_runtime_validate(session, NULL, NULL);
    if (status.native != 0) {
        return status;
    }
    for (uint64_t index = 0u; index < session->region_count; ++index) {
        session->aggregates[index] =
            (arbor_profile_aggregate){0u, 0u, 0u, 0u};
    }
    return profile_status(0);
}

arbor_status arbor_profile_region_get(
    const arbor_profile_session *session,
    uint64_t region_index,
    arbor_profile_region_result *result_out)
{
    if (session == NULL || result_out == NULL) {
        return profile_status(-EINVAL);
    }

    uint64_t descriptor_bytes = 0u;
    uint64_t aggregate_bytes = 0u;
    arbor_status status = profile_session_runtime_validate(
        session,
        &descriptor_bytes,
        &aggregate_bytes);
    if (status.native != 0) {
        return status;
    }
    if (region_index >= session->region_count ||
        !profile_range_valid(result_out, (uint64_t)sizeof(*result_out)) ||
        profile_ranges_overlap(
            result_out,
            (uint64_t)sizeof(*result_out),
            session,
            (uint64_t)sizeof(*session)) ||
        profile_ranges_overlap(
            result_out,
            (uint64_t)sizeof(*result_out),
            session->descriptors,
            descriptor_bytes) ||
        profile_ranges_overlap(
            result_out,
            (uint64_t)sizeof(*result_out),
            session->aggregates,
            aggregate_bytes)) {
        return profile_status(-EINVAL);
    }

    const arbor_profile_region_result candidate = {
        session->descriptors[region_index].id,
        session->aggregates[region_index]
    };
    *result_out = candidate;
    return profile_status(0);
}

arbor_status arbor_profile_span_begin(
    arbor_profile_session *session,
    uint64_t region_index,
    arbor_profile_span *span_out)
{
    if (session == NULL || span_out == NULL) {
        return profile_status(-EINVAL);
    }

    uint64_t descriptor_bytes = 0u;
    uint64_t aggregate_bytes = 0u;
    arbor_status status = profile_session_runtime_validate(
        session,
        &descriptor_bytes,
        &aggregate_bytes);
    if (status.native != 0) {
        return status;
    }
    if (region_index >= session->region_count ||
        !profile_range_valid(span_out, (uint64_t)sizeof(*span_out)) ||
        profile_ranges_overlap(
            span_out,
            (uint64_t)sizeof(*span_out),
            session,
            (uint64_t)sizeof(*session)) ||
        profile_ranges_overlap(
            span_out,
            (uint64_t)sizeof(*span_out),
            session->descriptors,
            descriptor_bytes) ||
        profile_ranges_overlap(
            span_out,
            (uint64_t)sizeof(*span_out),
            session->aggregates,
            aggregate_bytes) ||
        !profile_object_all_zero(span_out, (uint64_t)sizeof(*span_out))) {
        return profile_status(-EINVAL);
    }

    const int64_t start_ns = session->clock(session->clock_context);
    if (start_ns < 0) {
        return profile_status(start_ns);
    }

    arbor_profile_span candidate = {
        ARBOR_PROFILE_ABI_VERSION,
        (uint32_t)sizeof(arbor_profile_span),
        ARBOR_PROFILE_FLAGS_NONE,
        session,
        region_index,
        start_ns,
        span_out,
        0u,
        true,
        {0u, 0u, 0u, 0u, 0u, 0u, 0u}
    };
    candidate.prepared_guard = profile_span_guard(&candidate);
    *span_out = candidate;
    return profile_status(0);
}

arbor_status arbor_profile_span_end(
    arbor_profile_span *span)
{
    arbor_status status = profile_span_runtime_validate(span);
    if (status.native != 0) {
        return status;
    }

    const int64_t finish_ns = span->session->clock(span->session->clock_context);
    if (finish_ns < 0) {
        return profile_status(finish_ns);
    }
    if (finish_ns < span->start_ns) {
        return profile_status(-ERANGE);
    }

    arbor_asm_result_u64 duration = u64_sub_checked(
        (uint64_t)finish_ns,
        (uint64_t)span->start_ns);
    if (duration.status != 0) {
        return profile_status(duration.status);
    }

    arbor_profile_aggregate current =
        span->session->aggregates[span->region_index];
    arbor_asm_result_u64 count = u64_add_checked(
        current.sample_count,
        UINT64_C(1));
    if (count.status != 0) {
        return profile_status(-EOVERFLOW);
    }
    arbor_asm_result_u64 total = u64_add_checked(
        current.total_ns,
        duration.value);
    if (total.status != 0) {
        return profile_status(-EOVERFLOW);
    }

    arbor_profile_aggregate candidate = current;
    candidate.sample_count = count.value;
    candidate.total_ns = total.value;
    if (current.sample_count == 0u) {
        candidate.min_ns = duration.value;
        candidate.max_ns = duration.value;
    } else {
        if (duration.value < current.min_ns) {
            candidate.min_ns = duration.value;
        }
        if (duration.value > current.max_ns) {
            candidate.max_ns = duration.value;
        }
    }

    span->session->aggregates[span->region_index] = candidate;
    (void)memset(span, 0, sizeof(*span));
    return profile_status(0);
}
