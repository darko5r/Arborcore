#include <stddef.h>
#include <stdint.h>

#include <arborcore/profile.h>

typedef arbor_status (*session_prepare_fn)(
    arbor_profile_session *,
    const arbor_profile_region_descriptor *,
    arbor_profile_aggregate *,
    uint64_t,
    arbor_profile_clock_fn,
    void *);
typedef arbor_status (*session_validate_fn)(const arbor_profile_session *);
typedef arbor_status (*session_reset_fn)(arbor_profile_session *);
typedef arbor_status (*region_get_fn)(
    const arbor_profile_session *, uint64_t, arbor_profile_region_result *);
typedef arbor_status (*span_begin_fn)(
    arbor_profile_session *, uint64_t, arbor_profile_span *);
typedef arbor_status (*span_end_fn)(arbor_profile_span *);

_Static_assert(ARBOR_PROFILE_ABI_VERSION == 1u, "PROFILE0 ABI version");
_Static_assert(ARBOR_PROFILE_FLAGS_NONE == UINT64_C(0), "PROFILE0 flags");

_Static_assert(sizeof(arbor_profile_region_descriptor) == 8u, "descriptor size");
_Static_assert(_Alignof(arbor_profile_region_descriptor) == 8u, "descriptor align");
_Static_assert(offsetof(arbor_profile_region_descriptor, id) == 0u, "descriptor id");

_Static_assert(sizeof(arbor_profile_aggregate) == 32u, "aggregate size");
_Static_assert(_Alignof(arbor_profile_aggregate) == 8u, "aggregate align");
_Static_assert(offsetof(arbor_profile_aggregate, sample_count) == 0u, "aggregate count");
_Static_assert(offsetof(arbor_profile_aggregate, total_ns) == 8u, "aggregate total");
_Static_assert(offsetof(arbor_profile_aggregate, min_ns) == 16u, "aggregate min");
_Static_assert(offsetof(arbor_profile_aggregate, max_ns) == 24u, "aggregate max");

_Static_assert(sizeof(arbor_profile_region_result) == 40u, "result size");
_Static_assert(_Alignof(arbor_profile_region_result) == 8u, "result align");
_Static_assert(offsetof(arbor_profile_region_result, id) == 0u, "result id");
_Static_assert(offsetof(arbor_profile_region_result, aggregate) == 8u, "result aggregate");

_Static_assert(sizeof(arbor_profile_session) == 72u, "session size");
_Static_assert(_Alignof(arbor_profile_session) == 8u, "session align");
_Static_assert(offsetof(arbor_profile_session, abi_version) == 0u, "session abi");
_Static_assert(offsetof(arbor_profile_session, struct_size) == 4u, "session size field");
_Static_assert(offsetof(arbor_profile_session, flags) == 8u, "session flags");
_Static_assert(offsetof(arbor_profile_session, descriptors) == 16u, "session descriptors");
_Static_assert(offsetof(arbor_profile_session, aggregates) == 24u, "session aggregates");
_Static_assert(offsetof(arbor_profile_session, region_count) == 32u, "session count");
_Static_assert(offsetof(arbor_profile_session, clock) == 40u, "session clock");
_Static_assert(offsetof(arbor_profile_session, clock_context) == 48u, "session context");
_Static_assert(offsetof(arbor_profile_session, identity) == 56u, "session identity");
_Static_assert(offsetof(arbor_profile_session, prepared_guard) == 64u, "session guard");

_Static_assert(sizeof(arbor_profile_span) == 64u, "span size");
_Static_assert(_Alignof(arbor_profile_span) == 8u, "span align");
_Static_assert(offsetof(arbor_profile_span, abi_version) == 0u, "span abi");
_Static_assert(offsetof(arbor_profile_span, struct_size) == 4u, "span size field");
_Static_assert(offsetof(arbor_profile_span, flags) == 8u, "span flags");
_Static_assert(offsetof(arbor_profile_span, session) == 16u, "span session");
_Static_assert(offsetof(arbor_profile_span, region_index) == 24u, "span index");
_Static_assert(offsetof(arbor_profile_span, start_ns) == 32u, "span start");
_Static_assert(offsetof(arbor_profile_span, identity) == 40u, "span identity");
_Static_assert(offsetof(arbor_profile_span, prepared_guard) == 48u, "span guard");
_Static_assert(offsetof(arbor_profile_span, active) == 56u, "span active");
_Static_assert(offsetof(arbor_profile_span, reserved0) == 57u, "span reserved");

int main(void)
{
    session_prepare_fn p0 = arbor_profile_session_prepare;
    session_validate_fn p1 = arbor_profile_session_validate;
    session_reset_fn p2 = arbor_profile_session_reset;
    region_get_fn p3 = arbor_profile_region_get;
    span_begin_fn p4 = arbor_profile_span_begin;
    span_end_fn p5 = arbor_profile_span_end;
    return p0 == NULL || p1 == NULL || p2 == NULL ||
        p3 == NULL || p4 == NULL || p5 == NULL;
}
