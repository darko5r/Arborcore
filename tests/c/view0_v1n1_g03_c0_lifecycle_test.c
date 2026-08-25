#include <arborcore/view0_conformance/native.h>

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct element_snapshot {
    uint64_t standard_element_id;
    uint64_t namespace_id;
    uint64_t source_offset;
    uint64_t source_length;
    uint64_t parent_standard_element_id;
    uint64_t grandparent_standard_element_id;
    uint64_t depth;
    uint64_t flags;
    uint64_t ancestor_bits[2];
    uint32_t began;
    uint32_t completed;
    char local_name[32];
} element_snapshot;

typedef struct lifecycle_capture {
    element_snapshot stack[64];
    uint64_t stack_size;
    uint64_t enter_count;
    uint64_t begin_count;
    uint64_t complete_count;
    uint64_t leave_count;
    uint64_t max_active;
} lifecycle_capture;

static arbor_status ok_status(void)
{
    return arbor_status_from_native(0);
}

static arbor_status invalid_status(void)
{
    return arbor_status_from_native(-(int64_t)EINVAL);
}

static arbor_span span_from_cstr(const char *text)
{
    return (arbor_span){(const uint8_t *)text, (uint64_t)strlen(text)};
}

static void copy_name(char dst[32], arbor_span name)
{
    size_t length = (size_t)name.length;
    if (length >= 32u) {
        length = 31u;
    }
    if (length != 0u) {
        (void)memcpy(dst, name.data, length);
    }
    dst[length] = '\0';
}

static int snapshot_matches(
    const element_snapshot *snapshot,
    const arbor_view0_native_element_observation *observation)
{
    if (snapshot == NULL || observation == NULL) {
        return 0;
    }
    char name[32];
    copy_name(name, observation->local_name);
    return snapshot->standard_element_id == observation->standard_element_id &&
        snapshot->namespace_id == observation->namespace_id &&
        snapshot->source_offset == observation->source_offset &&
        snapshot->source_length == observation->source_length &&
        snapshot->parent_standard_element_id == observation->parent_standard_element_id &&
        snapshot->grandparent_standard_element_id == observation->grandparent_standard_element_id &&
        snapshot->depth == observation->depth &&
        snapshot->flags == observation->flags &&
        snapshot->ancestor_bits[0] == observation->ancestor_bits[0] &&
        snapshot->ancestor_bits[1] == observation->ancestor_bits[1] &&
        strcmp(snapshot->local_name, name) == 0;
}

static arbor_status capture_enter(
    void *opaque,
    const arbor_view0_native_element_observation *observation)
{
    lifecycle_capture *capture = (lifecycle_capture *)opaque;
    if (capture == NULL || observation == NULL || capture->stack_size >= 64u ||
        observation->depth != capture->stack_size) {
        return invalid_status();
    }

    if (capture->stack_size == 0u) {
        if (observation->parent_standard_element_id != ARBOR_VIEW0_NATIVE_ELEMENT_NONE) {
            return invalid_status();
        }
    } else {
        const element_snapshot *parent = &capture->stack[capture->stack_size - 1u];
        if (parent->standard_element_id != observation->parent_standard_element_id ||
            parent->completed == 0u) {
            return invalid_status();
        }
    }

    element_snapshot *slot = &capture->stack[capture->stack_size];
    *slot = (element_snapshot){
        .standard_element_id = observation->standard_element_id,
        .namespace_id = observation->namespace_id,
        .source_offset = observation->source_offset,
        .source_length = observation->source_length,
        .parent_standard_element_id = observation->parent_standard_element_id,
        .grandparent_standard_element_id = observation->grandparent_standard_element_id,
        .depth = observation->depth,
        .flags = observation->flags,
        .ancestor_bits = {observation->ancestor_bits[0], observation->ancestor_bits[1]},
        .began = 0u,
        .completed = 0u,
        .local_name = {0}
    };
    copy_name(slot->local_name, observation->local_name);

    capture->stack_size += 1u;
    capture->enter_count += 1u;
    if (capture->stack_size > capture->max_active) {
        capture->max_active = capture->stack_size;
    }
    return ok_status();
}

static arbor_status capture_begin(
    void *opaque,
    const arbor_view0_native_element_observation *observation)
{
    lifecycle_capture *capture = (lifecycle_capture *)opaque;
    if (capture == NULL || observation == NULL || capture->stack_size == 0u) {
        return invalid_status();
    }
    element_snapshot *slot = &capture->stack[capture->stack_size - 1u];
    if (!snapshot_matches(slot, observation) || slot->began != 0u || slot->completed != 0u) {
        return invalid_status();
    }
    slot->began = 1u;
    capture->begin_count += 1u;
    return ok_status();
}

static arbor_status capture_complete(
    void *opaque,
    const arbor_view0_native_element_observation *observation)
{
    lifecycle_capture *capture = (lifecycle_capture *)opaque;
    if (capture == NULL || observation == NULL || capture->stack_size == 0u) {
        return invalid_status();
    }
    element_snapshot *slot = &capture->stack[capture->stack_size - 1u];
    if (!snapshot_matches(slot, observation) || slot->began == 0u || slot->completed != 0u) {
        return invalid_status();
    }
    slot->completed = 1u;
    capture->complete_count += 1u;
    return ok_status();
}

static arbor_status capture_leave(
    void *opaque,
    const arbor_view0_native_element_observation *observation)
{
    lifecycle_capture *capture = (lifecycle_capture *)opaque;
    if (capture == NULL || observation == NULL || capture->stack_size == 0u) {
        return invalid_status();
    }
    element_snapshot *slot = &capture->stack[capture->stack_size - 1u];
    if (!snapshot_matches(slot, observation) || slot->began == 0u || slot->completed == 0u ||
        observation->depth + 1u != capture->stack_size) {
        return invalid_status();
    }
    capture->stack_size -= 1u;
    capture->leave_count += 1u;
    return ok_status();
}

int main(void)
{
    static const char html[] =
        "<!doctype html><html><head><title>x</title></head><body>"
        "<div><span>x</span><em>y</em></div><p>z</p></body></html>";

    lifecycle_capture capture = {0};
    const arbor_view0_native_semantic_observer observer = {
        .context = &capture,
        .traversal_enter = capture_enter,
        .element_begin = capture_begin,
        .attribute = NULL,
        .direct_child = NULL,
        .element_complete = capture_complete,
        .traversal_leave = capture_leave
    };
    arbor_view0_native_parse_counts parse_counts = {0};
    arbor_view0_native_document_facts facts = {0};
    arbor_view0_native_observation_counts observation_counts = {0};

    arbor_status status = arbor_view0_native_lexbor_observe(
        span_from_cstr(html),
        &observer,
        &parse_counts,
        &facts,
        &observation_counts);
    if (status.native != 0 || parse_counts.tokenizer_error_count != 0u ||
        parse_counts.tree_error_count != 0u || capture.stack_size != 0u ||
        capture.enter_count != observation_counts.element_count ||
        capture.begin_count != observation_counts.element_count ||
        capture.complete_count != observation_counts.element_count ||
        capture.leave_count != observation_counts.element_count ||
        capture.enter_count != 8u || observation_counts.max_depth != 3u ||
        capture.max_active != observation_counts.max_depth + 1u) {
        return 1;
    }

    puts("PASS: VIEW0 V1N1 G03 C0 lifecycle true DFS enter/leave ordering preserves existing per-element callback semantics");
    return 0;
}
