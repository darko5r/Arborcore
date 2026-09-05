#ifndef ARBORCORE_PROFILE_H
#define ARBORCORE_PROFILE_H

#include <stdbool.h>
#include <stdint.h>

#include <arborcore/arborcore.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ARBOR_PROFILE_ABI_VERSION 1u
#define ARBOR_PROFILE_FLAGS_NONE UINT64_C(0)

typedef int64_t (*arbor_profile_clock_fn)(void *context);

typedef struct arbor_profile_region_descriptor {
    uint64_t id;
} arbor_profile_region_descriptor;

typedef struct arbor_profile_aggregate {
    uint64_t sample_count;
    uint64_t total_ns;
    uint64_t min_ns;
    uint64_t max_ns;
} arbor_profile_aggregate;

typedef struct arbor_profile_region_result {
    uint64_t id;
    arbor_profile_aggregate aggregate;
} arbor_profile_region_result;

typedef struct arbor_profile_session {
    uint32_t abi_version;
    uint32_t struct_size;
    uint64_t flags;
    const arbor_profile_region_descriptor *descriptors;
    arbor_profile_aggregate *aggregates;
    uint64_t region_count;
    arbor_profile_clock_fn clock;
    void *clock_context;
    const void *identity;
    uint64_t prepared_guard;
} arbor_profile_session;

typedef struct arbor_profile_span {
    uint32_t abi_version;
    uint32_t struct_size;
    uint64_t flags;
    arbor_profile_session *session;
    uint64_t region_index;
    int64_t start_ns;
    const void *identity;
    uint64_t prepared_guard;
    bool active;
    uint8_t reserved0[7];
} arbor_profile_span;

arbor_status arbor_profile_session_prepare(
    arbor_profile_session *session,
    const arbor_profile_region_descriptor *descriptors,
    arbor_profile_aggregate *aggregates,
    uint64_t region_count,
    arbor_profile_clock_fn clock,
    void *clock_context);

arbor_status arbor_profile_session_validate(
    const arbor_profile_session *session);

arbor_status arbor_profile_session_reset(
    arbor_profile_session *session);

arbor_status arbor_profile_region_get(
    const arbor_profile_session *session,
    uint64_t region_index,
    arbor_profile_region_result *result_out);

arbor_status arbor_profile_span_begin(
    arbor_profile_session *session,
    uint64_t region_index,
    arbor_profile_span *span_out);

arbor_status arbor_profile_span_end(
    arbor_profile_span *span);

#ifdef __cplusplus
}
#endif

#endif
