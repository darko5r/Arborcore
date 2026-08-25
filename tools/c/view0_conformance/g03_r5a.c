#include "g03_r5a.h"
#include "g03_r1a.h"
#include "g03_r2a.h"
#include "g03_r3a.h"
#include "g03_r4a.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define G03_R5A_PRIOR_OFFSET_CAPACITY UINT64_C(4096)
#define G03_R5A_NOINLINE __attribute__((noinline))

_Static_assert(ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS == G03_R5A_PRIOR_OFFSET_CAPACITY,
               "G03 R5A prior-owner offset bound drift");
_Static_assert(sizeof("ARBOR_VIEW_V1_G03_EXPLICIT_HTML_ELEMENT_ALLOWANCE") <=
                   ARBOR_VIEW0_NATIVE_RULE_SYMBOL_CAP,
               "G03 R5A symbolic name exceeds diagnostic capacity");
_Static_assert(sizeof("Standard HTML element is not explicitly allowed in this authored source context") <=
                   ARBOR_VIEW0_NATIVE_MESSAGE_CAP,
               "G03 R5A message exceeds diagnostic capacity");

typedef struct g03_r5a_context {
    union {
        arbor_view0_native_diagnostic *diagnostics;
        arbor_view0_native_source_anchor *anchors;
    } output;
    uint64_t diagnostic_capacity;
    uint64_t discovery_sequence_base;
    uint64_t diagnostic_count;
    uint64_t prior_owner_suppression_count;
    const uint64_t *prior_error_offsets;
    uint64_t prior_error_offset_count;
    bool publish;
    bool collect_anchors;
} g03_r5a_context;

_Static_assert(sizeof(g03_r5a_context) == 64u,
               "G03 R5A evaluator context layout drift on x86-64");

static arbor_status status_from_errno_value(int value)
{
    return arbor_status_from_native(-(int64_t)value);
}

static arbor_status ok_status(void)
{
    return arbor_status_from_native(0);
}

static bool insertion_target_is_actual_foster_family(uint64_t standard_element_id)
{
    switch (standard_element_id) {
        case ARBOR_VIEW0_NATIVE_ELEMENT_TABLE:
        case ARBOR_VIEW0_NATIVE_ELEMENT_TBODY:
        case ARBOR_VIEW0_NATIVE_ELEMENT_TFOOT:
        case ARBOR_VIEW0_NATIVE_ELEMENT_THEAD:
        case ARBOR_VIEW0_NATIVE_ELEMENT_TR:
            return true;
        default:
            return false;
    }
}

static bool prior_owner_at_offset(const g03_r5a_context *context, uint64_t offset)
{
    if (context == NULL || offset == ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE) {
        return false;
    }
    for (uint64_t i = 0u; i < context->prior_error_offset_count; ++i) {
        if (context->prior_error_offsets[i] == offset) {
            return true;
        }
    }
    return false;
}

static void fill_diagnostic(
    arbor_view0_native_diagnostic *diagnostic,
    uint64_t source_offset,
    uint64_t source_length,
    uint64_t discovery_sequence)
{
    static const char symbolic[] =
        "ARBOR_VIEW_V1_G03_EXPLICIT_HTML_ELEMENT_ALLOWANCE";
    static const char message[] =
        "Standard HTML element is not explicitly allowed in this authored source context";
    (void)memset(diagnostic, 0, sizeof(*diagnostic));
    diagnostic->rule_id = ARBOR_VIEW_V1_G03_EXPLICIT_HTML_ELEMENT_ALLOWANCE;
    diagnostic->byte_offset = source_offset;
    diagnostic->source_length = source_length;
    diagnostic->discovery_sequence = discovery_sequence;
    diagnostic->severity = (uint32_t)ARBOR_VIEW0_NATIVE_SEVERITY_ERROR;
    diagnostic->origin = (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_AUTHORING;
    (void)memcpy(diagnostic->symbolic_name, symbolic, sizeof(symbolic));
    (void)memcpy(diagnostic->message, message, sizeof(message));
}

void arbor_view0_native_g03_r5a_materialize_anchor(
    const arbor_view0_native_source_anchor *anchor,
    uint64_t discovery_sequence,
    arbor_view0_native_diagnostic *diagnostic)
{
    fill_diagnostic(
        diagnostic, (uint64_t)anchor->byte_offset,
        (uint64_t)anchor->source_length, discovery_sequence);
}

static arbor_status report_invalid(
    g03_r5a_context *context,
    const arbor_view0_native_source_repair_context *record)
{
    if (context == NULL || record == NULL ||
        record->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_NONE ||
        record->source_offset == ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE ||
        record->source_length == 0u) {
        return status_from_errno_value(EIO);
    }
    if (context->diagnostic_count == UINT64_MAX) {
        return status_from_errno_value(EOVERFLOW);
    }
    if (context->publish || context->collect_anchors) {
        if (context->diagnostic_count >= context->diagnostic_capacity) {
            return status_from_errno_value(ENOSPC);
        }
        if (context->publish) {
            if (context->output.diagnostics == NULL) {
                return status_from_errno_value(ENOSPC);
            }
            if (context->discovery_sequence_base > UINT64_MAX - context->diagnostic_count) {
                return status_from_errno_value(EOVERFLOW);
            }
            fill_diagnostic(
                context->output.diagnostics + context->diagnostic_count,
                record->source_offset, record->source_length,
                context->discovery_sequence_base + context->diagnostic_count);
        } else if (context->output.anchors != NULL) {
            if (record->source_offset > UINT32_MAX || record->source_length > UINT32_MAX) {
                return status_from_errno_value(EOVERFLOW);
            }
            context->output.anchors[context->diagnostic_count] =
                (arbor_view0_native_source_anchor){
                    .byte_offset = (uint32_t)record->source_offset,
                    .source_length = (uint32_t)record->source_length
                };
        } else {
            return status_from_errno_value(ENOSPC);
        }
    }
    context->diagnostic_count += 1u;
    return ok_status();
}

static arbor_status source_repair(
    void *context_void,
    const arbor_view0_native_source_repair_context *record)
{
    g03_r5a_context *context = (g03_r5a_context *)context_void;
    if (context == NULL || record == NULL) {
        return status_from_errno_value(EINVAL);
    }
    if ((record->insertion_flags & ~ARBOR_VIEW0_NATIVE_SOURCE_REPAIR_FLAG_FOSTER_PARENTING) != 0u) {
        return status_from_errno_value(EIO);
    }
    if (record->standard_element_id == ARBOR_VIEW0_NATIVE_ELEMENT_NONE ||
        record->insertion_seen != 1u ||
        (record->insertion_flags & ARBOR_VIEW0_NATIVE_SOURCE_REPAIR_FLAG_FOSTER_PARENTING) == 0u ||
        !insertion_target_is_actual_foster_family(
            record->insertion_current_standard_element_id)) {
        return ok_status();
    }
    if (prior_owner_at_offset(context, record->source_offset)) {
        if (context->prior_owner_suppression_count == UINT64_MAX) {
            return status_from_errno_value(EOVERFLOW);
        }
        context->prior_owner_suppression_count += 1u;
        return ok_status();
    }
    return report_invalid(context, record);
}

static G03_R5A_NOINLINE arbor_status collect_prior_error_offsets(
    arbor_span input,
    uint64_t *offsets,
    uint64_t *count_out)
{
    if (offsets == NULL || count_out == NULL) {
        return status_from_errno_value(EINVAL);
    }
    uint64_t total = 0u;

    arbor_view0_native_g03_r1a_evaluation r1m = {0};
    arbor_status status = arbor_view0_native_g03_r1a_measure(input, &r1m);
    if (status.native != 0) return status;
    if (r1m.diagnostic_count > G03_R5A_PRIOR_OFFSET_CAPACITY - total) {
        return status_from_errno_value(E2BIG);
    }
    arbor_view0_native_g03_r1a_evaluation r1c = {0};
    status = arbor_view0_native_g03_r1a_collect_offsets(
        input, offsets + total, r1m.diagnostic_count, &r1c);
    if (status.native != 0) return status;
    if (r1c.diagnostic_count != r1m.diagnostic_count ||
        r1c.deferred_main_form_count != r1m.deferred_main_form_count) {
        return status_from_errno_value(EIO);
    }
    total += r1c.diagnostic_count;

    arbor_view0_native_g03_r2a_evaluation r2m = {0};
    status = arbor_view0_native_g03_r2a_measure(input, &r2m);
    if (status.native != 0) return status;
    if (r2m.diagnostic_count > G03_R5A_PRIOR_OFFSET_CAPACITY - total) {
        return status_from_errno_value(E2BIG);
    }
    arbor_view0_native_g03_r2a_evaluation r2c = {0};
    status = arbor_view0_native_g03_r2a_collect_offsets(
        input, offsets + total, r2m.diagnostic_count, &r2c);
    if (status.native != 0) return status;
    if (r2c.diagnostic_count != r2m.diagnostic_count ||
        r2c.deferred_flags != r2m.deferred_flags) {
        return status_from_errno_value(EIO);
    }
    total += r2c.diagnostic_count;

    arbor_view0_native_g03_r3a_evaluation r3m = {0};
    status = arbor_view0_native_g03_r3a_measure(input, &r3m);
    if (status.native != 0) return status;
    if (r3m.diagnostic_count > G03_R5A_PRIOR_OFFSET_CAPACITY - total) {
        return status_from_errno_value(E2BIG);
    }
    arbor_view0_native_g03_r3a_evaluation r3c = {0};
    status = arbor_view0_native_g03_r3a_collect_offsets(
        input, offsets + total, r3m.diagnostic_count, &r3c);
    if (status.native != 0) return status;
    if (r3c.diagnostic_count != r3m.diagnostic_count ||
        r3c.deferred_flags != r3m.deferred_flags) {
        return status_from_errno_value(EIO);
    }
    total += r3c.diagnostic_count;

    arbor_view0_native_g03_r4a_evaluation r4m = {0};
    status = arbor_view0_native_g03_r4a_measure(input, &r4m);
    if (status.native != 0) return status;
    if (r4m.diagnostic_count > G03_R5A_PRIOR_OFFSET_CAPACITY - total) {
        return status_from_errno_value(E2BIG);
    }
    arbor_view0_native_g03_r4a_evaluation r4c = {0};
    status = arbor_view0_native_g03_r4a_collect_offsets(
        input, offsets + total, r4m.diagnostic_count, &r4c);
    if (status.native != 0) return status;
    if (r4c.diagnostic_count != r4m.diagnostic_count ||
        r4c.deferred_flags != r4m.deferred_flags) {
        return status_from_errno_value(EIO);
    }
    total += r4c.diagnostic_count;

    *count_out = total;
    return ok_status();
}

static arbor_status evaluate(
    arbor_span input,
    arbor_view0_native_diagnostic *diagnostics,
    arbor_view0_native_source_anchor *anchors,
    uint64_t diagnostic_capacity,
    uint64_t discovery_sequence_base,
    bool publish,
    bool collect_anchors,
    arbor_view0_native_g03_r5a_evaluation *evaluation_out)
{
    if (evaluation_out == NULL || (publish && collect_anchors) ||
        (publish && diagnostic_capacity != 0u && diagnostics == NULL) ||
        (collect_anchors && diagnostic_capacity != 0u && anchors == NULL)) {
        return status_from_errno_value(EINVAL);
    }

    uint64_t prior_offsets[4096] = {0u};
    uint64_t prior_count = 0u;
    arbor_status status = collect_prior_error_offsets(input, prior_offsets, &prior_count);
    if (status.native != 0) return status;

    g03_r5a_context context = {
        .output = {.diagnostics = diagnostics},
        .diagnostic_capacity = diagnostic_capacity,
        .discovery_sequence_base = discovery_sequence_base,
        .diagnostic_count = 0u,
        .prior_owner_suppression_count = 0u,
        .prior_error_offsets = prior_offsets,
        .prior_error_offset_count = prior_count,
        .publish = publish,
        .collect_anchors = collect_anchors
    };
    if (collect_anchors) {
        context.output.anchors = anchors;
    }
    const arbor_view0_native_semantic_observer observer = {
        .context = &context,
        .source_repair = source_repair
    };
    arbor_view0_native_parse_counts parse_counts = {0};
    arbor_view0_native_document_facts facts = {0};
    arbor_view0_native_observation_counts observations = {0};
    status = arbor_view0_native_lexbor_observe(
        input, &observer, &parse_counts, &facts, &observations);
    if (status.native != 0) return status;
    if ((publish || collect_anchors) &&
        context.diagnostic_count != diagnostic_capacity) {
        return status_from_errno_value(EIO);
    }

    *evaluation_out = (arbor_view0_native_g03_r5a_evaluation){
        .diagnostic_count = context.diagnostic_count,
        .prior_owner_suppression_count = context.prior_owner_suppression_count
    };
    return ok_status();
}

arbor_status arbor_view0_native_g03_r5a_measure(
    arbor_span input,
    arbor_view0_native_g03_r5a_evaluation *evaluation_out)
{
    return evaluate(input, NULL, NULL, 0u, 0u, false, false, evaluation_out);
}

arbor_status arbor_view0_native_g03_r5a_collect(
    arbor_span input,
    arbor_view0_native_diagnostic *diagnostics,
    uint64_t diagnostic_capacity,
    uint64_t discovery_sequence_base,
    arbor_view0_native_g03_r5a_evaluation *evaluation_out)
{
    return evaluate(
        input, diagnostics, NULL, diagnostic_capacity, discovery_sequence_base,
        true, false, evaluation_out);
}

arbor_status arbor_view0_native_g03_r5a_collect_anchors(
    arbor_span input,
    arbor_view0_native_source_anchor *anchors,
    uint64_t anchor_capacity,
    arbor_view0_native_g03_r5a_evaluation *evaluation_out)
{
    return evaluate(
        input, NULL, anchors, anchor_capacity, 0u, false, true, evaluation_out);
}
