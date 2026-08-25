#include <arborcore/view.h>

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

_Static_assert(sizeof(arbor_view_measure) == 8u, "VIEW0 C1 measure layout drift");
_Static_assert(offsetof(arbor_view_output, arena) == 0u, "VIEW0 C1 output.arena layout drift");
_Static_assert(offsetof(arbor_view_output, arena_mark) == 8u, "VIEW0 C1 output.arena_mark layout drift");
_Static_assert(offsetof(arbor_view_output, buffer) == 16u, "VIEW0 C1 output.buffer layout drift");
_Static_assert(offsetof(arbor_view_output, required_length) == 40u, "VIEW0 C1 output.required_length layout drift");
_Static_assert(offsetof(arbor_view_output, state) == 48u, "VIEW0 C1 output.state layout drift");
_Static_assert(sizeof(arbor_view_output) == 56u, "VIEW0 C1 output layout drift");

static arbor_status invalid_argument_status(void)
{
    return arbor_status_from_native(-EINVAL);
}

static arbor_status ok_status(void)
{
    return arbor_status_from_native(0);
}

static bool span_range_representable(const void *data, uint64_t length)
{
    if (length == 0u) {
        return true;
    }
    if (data == NULL) {
        return false;
    }
    arbor_asm_result_u64 end = range_end_checked((uint64_t)(uintptr_t)data, length);
    return end.status == 0;
}

static bool spans_overlap_known_representable(
    const void *left,
    uint64_t left_length,
    const void *right,
    uint64_t right_length)
{
    if (left_length == 0u || right_length == 0u || left == NULL || right == NULL) {
        return false;
    }
    arbor_asm_result_u64 overlap = range_overlaps(
        (uint64_t)(uintptr_t)left,
        left_length,
        (uint64_t)(uintptr_t)right,
        right_length);
    return overlap.status == 0 && overlap.value != 0u;
}

static arbor_status arena_validate(const arbor_asm_arena *arena)
{
    if (arena == NULL || !span_range_representable(arena, sizeof(*arena)) ||
        arena->offset > arena->capacity) {
        return invalid_argument_status();
    }
    if (arena->capacity != 0u && arena->base == NULL) {
        return invalid_argument_status();
    }
    arbor_asm_result_u64 end = range_end_checked(
        (uint64_t)(uintptr_t)arena->base,
        arena->capacity);
    if (end.status != 0) {
        return arbor_status_from_native(end.status);
    }
    if (spans_overlap_known_representable(arena, sizeof(*arena), arena->base, arena->capacity)) {
        return invalid_argument_status();
    }
    return ok_status();
}

static arbor_status output_state_validate_for_begin(const arbor_view_output *output)
{
    if (output->state != ARBOR_VIEW_OUTPUT_STATE_IDLE &&
        output->state != ARBOR_VIEW_OUTPUT_STATE_COMMITTED) {
        return invalid_argument_status();
    }
    return ok_status();
}

static arbor_status output_active_validate(const arbor_view_output *output)
{
    if (output == NULL || !span_range_representable(output, sizeof(*output)) ||
        output->state != ARBOR_VIEW_OUTPUT_STATE_ACTIVE || output->arena == NULL) {
        return invalid_argument_status();
    }

    arbor_status status = arena_validate(output->arena);
    if (status.native != 0) {
        return status;
    }
    if (spans_overlap_known_representable(output, sizeof(*output), output->arena, sizeof(*output->arena)) ||
        spans_overlap_known_representable(output, sizeof(*output), output->arena->base, output->arena->capacity)) {
        return invalid_argument_status();
    }
    if (output->arena_mark > output->arena->capacity ||
        output->required_length > output->arena->capacity - output->arena_mark) {
        return invalid_argument_status();
    }

    arbor_asm_result_u64 expected_end = u64_add_checked(
        output->arena_mark,
        output->required_length);
    if (expected_end.status != 0 || output->arena->offset != expected_end.value) {
        return invalid_argument_status();
    }

    arbor_asm_result_u64 expected_data = range_end_checked(
        (uint64_t)(uintptr_t)output->arena->base,
        output->arena_mark);
    if (expected_data.status != 0 ||
        output->buffer.data != (uint8_t *)(uintptr_t)expected_data.value ||
        output->buffer.capacity != output->required_length ||
        output->buffer.length > output->buffer.capacity) {
        return invalid_argument_status();
    }

    return ok_status();
}

static arbor_status output_abort_validated(arbor_view_output *output)
{
    arbor_asm_result_u64 rewind = arena_rewind(output->arena, output->arena_mark);
    if (rewind.status != 0) {
        return arbor_status_from_native(rewind.status);
    }
    *output = (arbor_view_output){0};
    return ok_status();
}

arbor_status arbor_view_measure_add(arbor_view_measure *measure, uint64_t length)
{
    if (measure == NULL || !span_range_representable(measure, sizeof(*measure))) {
        return invalid_argument_status();
    }
    arbor_asm_result_u64 total = u64_add_checked(measure->length, length);
    if (total.status != 0) {
        return arbor_status_from_native(total.status);
    }
    measure->length = total.value;
    return ok_status();
}

arbor_status arbor_view_output_begin(
    arbor_asm_arena *arena,
    uint64_t required_length,
    arbor_view_output *out)
{
    if (out == NULL || !span_range_representable(out, sizeof(*out))) {
        return invalid_argument_status();
    }

    arbor_status status = arena_validate(arena);
    if (status.native != 0) {
        return status;
    }
    if (spans_overlap_known_representable(out, sizeof(*out), arena, sizeof(*arena)) ||
        spans_overlap_known_representable(out, sizeof(*out), arena->base, arena->capacity)) {
        return invalid_argument_status();
    }
    status = output_state_validate_for_begin(out);
    if (status.native != 0) {
        return status;
    }

    const uint64_t mark = arena_mark(arena);
    arbor_asm_result_ptr allocation = arena_alloc(arena, required_length);
    if (allocation.status != 0) {
        return arbor_status_from_native(allocation.status);
    }

    arbor_view_output candidate = {
        .arena = arena,
        .arena_mark = mark,
        .buffer = {0},
        .required_length = required_length,
        .state = ARBOR_VIEW_OUTPUT_STATE_ACTIVE
    };
    arbor_asm_result_u64 initialized = buffer_init(
        &candidate.buffer,
        allocation.value,
        required_length);
    if (initialized.status != 0) {
        arbor_asm_result_u64 rewind = arena_rewind(arena, mark);
        if (rewind.status != 0) {
            return arbor_status_from_native(rewind.status);
        }
        return arbor_status_from_native(initialized.status);
    }

    *out = candidate;
    return ok_status();
}

arbor_status arbor_view_output_append(arbor_view_output *output, arbor_span bytes)
{
    arbor_status status = output_active_validate(output);
    if (status.native != 0) {
        return status;
    }
    if (!span_range_representable(bytes.data, bytes.length)) {
        arbor_status aborted = output_abort_validated(output);
        return aborted.native != 0 ? aborted : invalid_argument_status();
    }

    arbor_asm_result_u64 appended = buffer_append(
        &output->buffer,
        bytes.data,
        bytes.length);
    if (appended.status != 0) {
        arbor_status aborted = output_abort_validated(output);
        return aborted.native != 0 ? aborted : arbor_status_from_native(appended.status);
    }
    return ok_status();
}

arbor_status arbor_view_output_commit(arbor_view_output *output, arbor_span *body_out)
{
    arbor_status status = output_active_validate(output);
    if (status.native != 0) {
        return status;
    }

    if (body_out == NULL || !span_range_representable(body_out, sizeof(*body_out)) ||
        spans_overlap_known_representable(body_out, sizeof(*body_out), output, sizeof(*output)) ||
        spans_overlap_known_representable(body_out, sizeof(*body_out), output->arena, sizeof(*output->arena)) ||
        spans_overlap_known_representable(body_out, sizeof(*body_out), output->arena->base, output->arena->capacity) ||
        output->buffer.length != output->required_length) {
        arbor_status aborted = output_abort_validated(output);
        return aborted.native != 0 ? aborted : invalid_argument_status();
    }

    const arbor_span candidate = {
        .data = output->buffer.data,
        .length = output->buffer.length
    };
    output->arena = NULL;
    output->arena_mark = 0u;
    output->state = ARBOR_VIEW_OUTPUT_STATE_COMMITTED;
    *body_out = candidate;
    return ok_status();
}

arbor_status arbor_view_output_abort(arbor_view_output *output)
{
    arbor_status status = output_active_validate(output);
    if (status.native != 0) {
        return status;
    }
    return output_abort_validated(output);
}

static arbor_status html_text_measure_additional(arbor_span text, uint64_t *additional_out)
{
    if (additional_out == NULL ||
        !span_range_representable(additional_out, sizeof(*additional_out)) ||
        !span_range_representable(text.data, text.length)) {
        return invalid_argument_status();
    }

    uint64_t additional = text.length;
    for (uint64_t i = 0u; i < text.length; ++i) {
        uint64_t expansion = 0u;
        switch (text.data[i]) {
        case (uint8_t)'&':
            expansion = 4u;
            break;
        case (uint8_t)'<':
        case (uint8_t)'>':
            expansion = 3u;
            break;
        default:
            break;
        }
        if (expansion == 0u) {
            continue;
        }
        arbor_asm_result_u64 total = u64_add_checked(additional, expansion);
        if (total.status != 0) {
            return arbor_status_from_native(total.status);
        }
        additional = total.value;
    }

    *additional_out = additional;
    return ok_status();
}

static bool html_text_source_aliases_active_output(
    const arbor_view_output *output,
    arbor_span text)
{
    return spans_overlap_known_representable(
               text.data, text.length, output, sizeof(*output)) ||
           spans_overlap_known_representable(
               text.data, text.length, output->arena, sizeof(*output->arena)) ||
           spans_overlap_known_representable(
               text.data,
               text.length,
               output->buffer.data,
               output->buffer.capacity);
}

arbor_status arbor_view_html_text_measure(
    arbor_view_measure *measure,
    arbor_span text)
{
    if (measure == NULL || !span_range_representable(measure, sizeof(*measure)) ||
        !span_range_representable(text.data, text.length) ||
        spans_overlap_known_representable(
            measure, sizeof(*measure), text.data, text.length)) {
        return invalid_argument_status();
    }

    uint64_t additional = 0u;
    arbor_status status = html_text_measure_additional(text, &additional);
    if (status.native != 0) {
        return status;
    }
    return arbor_view_measure_add(measure, additional);
}

arbor_status arbor_view_html_text_append(
    arbor_view_output *output,
    arbor_span text)
{
    arbor_status status = output_active_validate(output);
    if (status.native != 0) {
        return status;
    }
    if (!span_range_representable(text.data, text.length) ||
        html_text_source_aliases_active_output(output, text)) {
        arbor_status aborted = output_abort_validated(output);
        return aborted.native != 0 ? aborted : invalid_argument_status();
    }

    uint64_t run_start = 0u;
    for (uint64_t i = 0u; i < text.length; ++i) {
        const char *entity = NULL;
        uint64_t entity_length = 0u;
        switch (text.data[i]) {
        case (uint8_t)'&':
            entity = "&amp;";
            entity_length = 5u;
            break;
        case (uint8_t)'<':
            entity = "&lt;";
            entity_length = 4u;
            break;
        case (uint8_t)'>':
            entity = "&gt;";
            entity_length = 4u;
            break;
        default:
            break;
        }
        if (entity == NULL) {
            continue;
        }

        if (i != run_start) {
            status = arbor_view_output_append(
                output,
                (arbor_span){text.data + run_start, i - run_start});
            if (status.native != 0) {
                return status;
            }
        }
        status = arbor_view_output_append(
            output,
            (arbor_span){(const uint8_t *)entity, entity_length});
        if (status.native != 0) {
            return status;
        }
        run_start = i + 1u;
    }

    if (run_start != text.length) {
        status = arbor_view_output_append(
            output,
            (arbor_span){text.data + run_start, text.length - run_start});
    }
    return status;
}


/* VIEW0 T1 — minimal prepared HTML template mechanism. */

#define VIEW_TEMPLATE_PART_LITERAL UINT64_C(1)
#define VIEW_TEMPLATE_PART_HTML_TEXT UINT64_C(2)

typedef enum view_template_raw_kind {
    VIEW_TEMPLATE_RAW_NONE = 0,
    VIEW_TEMPLATE_RAW_SCRIPT,
    VIEW_TEMPLATE_RAW_STYLE,
    VIEW_TEMPLATE_RAW_TITLE,
    VIEW_TEMPLATE_RAW_TEXTAREA,
    VIEW_TEMPLATE_RAW_XMP,
    VIEW_TEMPLATE_RAW_IFRAME,
    VIEW_TEMPLATE_RAW_NOEMBED,
    VIEW_TEMPLATE_RAW_NOFRAMES,
    VIEW_TEMPLATE_RAW_NOSCRIPT,
    VIEW_TEMPLATE_RAW_PLAINTEXT
} view_template_raw_kind;

typedef struct view_template_scan_output {
    arbor_view_html_template_requirements requirements;
    arbor_view_html_template_part *parts;
    uint8_t *literal_bytes;
    uint64_t part_index;
    uint64_t literal_offset;
    bool emit;
} view_template_scan_output;

_Static_assert(
    sizeof(arbor_view_html_template_requirements) == 16u,
    "VIEW0 T1 requirements layout drift");
_Static_assert(
    sizeof(arbor_view_html_template_part) == 32u,
    "VIEW0 T1 part layout drift");
_Static_assert(
    sizeof(arbor_view_html_template_storage) == 32u,
    "VIEW0 T1 storage layout drift");
_Static_assert(
    sizeof(arbor_view_html_template) == 56u,
    "VIEW0 T1 template layout drift");


arbor_status arbor_view_utf8_validate(arbor_span bytes)
{
    if (!span_range_representable(bytes.data, bytes.length)) {
        return invalid_argument_status();
    }

    uint64_t i = 0u;
    while (i < bytes.length) {
        const uint8_t first = bytes.data[i];
        if (first <= UINT8_C(0x7f)) {
            i += 1u;
            continue;
        }

        uint64_t needed = 0u;
        uint8_t second_lower = UINT8_C(0x80);
        uint8_t second_upper = UINT8_C(0xbf);

        if (first >= UINT8_C(0xc2) && first <= UINT8_C(0xdf)) {
            needed = 1u;
        } else if (first == UINT8_C(0xe0)) {
            needed = 2u;
            second_lower = UINT8_C(0xa0);
        } else if (first >= UINT8_C(0xe1) && first <= UINT8_C(0xec)) {
            needed = 2u;
        } else if (first == UINT8_C(0xed)) {
            needed = 2u;
            second_upper = UINT8_C(0x9f);
        } else if (first >= UINT8_C(0xee) && first <= UINT8_C(0xef)) {
            needed = 2u;
        } else if (first == UINT8_C(0xf0)) {
            needed = 3u;
            second_lower = UINT8_C(0x90);
        } else if (first >= UINT8_C(0xf1) && first <= UINT8_C(0xf3)) {
            needed = 3u;
        } else if (first == UINT8_C(0xf4)) {
            needed = 3u;
            second_upper = UINT8_C(0x8f);
        } else {
            return arbor_status_from_native(-EILSEQ);
        }

        if (bytes.length - i <= needed) {
            return arbor_status_from_native(-EILSEQ);
        }
        const uint8_t second = bytes.data[i + 1u];
        if (second < second_lower || second > second_upper) {
            return arbor_status_from_native(-EILSEQ);
        }
        for (uint64_t offset = 2u; offset <= needed; ++offset) {
            const uint8_t continuation = bytes.data[i + offset];
            if (continuation < UINT8_C(0x80) || continuation > UINT8_C(0xbf)) {
                return arbor_status_from_native(-EILSEQ);
            }
        }
        i += needed + 1u;
    }

    return ok_status();
}

static arbor_status no_space_status(void)
{
    return arbor_status_from_native(-ENOSPC);
}

static arbor_status not_found_status(void)
{
    return arbor_status_from_native(-ENOENT);
}

static arbor_status exists_status(void)
{
    return arbor_status_from_native(-EEXIST);
}

static bool ascii_alpha(uint8_t ch)
{
    return (ch >= (uint8_t)'A' && ch <= (uint8_t)'Z') ||
           (ch >= (uint8_t)'a' && ch <= (uint8_t)'z');
}

static bool ascii_digit(uint8_t ch)
{
    return ch >= (uint8_t)'0' && ch <= (uint8_t)'9';
}

static bool ascii_identifier_start(uint8_t ch)
{
    return ascii_alpha(ch) || ch == (uint8_t)'_';
}

static bool ascii_identifier_continue(uint8_t ch)
{
    return ascii_identifier_start(ch) || ascii_digit(ch);
}

static uint8_t ascii_lower(uint8_t ch)
{
    if (ch >= (uint8_t)'A' && ch <= (uint8_t)'Z') {
        return (uint8_t)(ch + ((uint8_t)'a' - (uint8_t)'A'));
    }
    return ch;
}

static bool ascii_name_equals(arbor_span name, const char *literal, uint64_t literal_length)
{
    if (name.length != literal_length) {
        return false;
    }
    for (uint64_t i = 0u; i < name.length; ++i) {
        if (ascii_lower(name.data[i]) != (uint8_t)literal[i]) {
            return false;
        }
    }
    return true;
}

static bool span_bytes_equal(arbor_span left, arbor_span right)
{
    if (left.length != right.length) {
        return false;
    }
    for (uint64_t i = 0u; i < left.length; ++i) {
        if (left.data[i] != right.data[i]) {
            return false;
        }
    }
    return true;
}

static bool template_field_name_valid(arbor_span name)
{
    if (name.length == 0u || !span_range_representable(name.data, name.length) ||
        !ascii_identifier_start(name.data[0])) {
        return false;
    }
    for (uint64_t i = 1u; i < name.length; ++i) {
        if (!ascii_identifier_continue(name.data[i])) {
            return false;
        }
    }
    return true;
}

static arbor_status template_field_table_validate(
    const arbor_span *field_names,
    uint64_t field_count,
    uint64_t *field_bytes_out)
{
    if (field_bytes_out == NULL ||
        !span_range_representable(field_bytes_out, sizeof(*field_bytes_out))) {
        return invalid_argument_status();
    }

    arbor_asm_result_u64 bytes = u64_mul_checked(
        field_count,
        (uint64_t)sizeof(arbor_span));
    if (bytes.status != 0) {
        return arbor_status_from_native(bytes.status);
    }
    if (!span_range_representable(field_names, bytes.value)) {
        return invalid_argument_status();
    }

    for (uint64_t i = 0u; i < field_count; ++i) {
        if (!template_field_name_valid(field_names[i])) {
            return invalid_argument_status();
        }
        for (uint64_t j = 0u; j < i; ++j) {
            if (span_bytes_equal(field_names[i], field_names[j])) {
                return exists_status();
            }
        }
    }

    *field_bytes_out = bytes.value;
    return ok_status();
}

static arbor_status template_field_resolve(
    const arbor_span *field_names,
    uint64_t field_count,
    arbor_span name,
    uint64_t *slot_out)
{
    if (slot_out == NULL ||
        !span_range_representable(slot_out, sizeof(*slot_out))) {
        return invalid_argument_status();
    }
    for (uint64_t i = 0u; i < field_count; ++i) {
        if (span_bytes_equal(field_names[i], name)) {
            *slot_out = i;
            return ok_status();
        }
    }
    return not_found_status();
}

static arbor_status template_scan_count_add(uint64_t *value)
{
    arbor_asm_result_u64 next = u64_add_checked(*value, 1u);
    if (next.status != 0) {
        return arbor_status_from_native(next.status);
    }
    *value = next.value;
    return ok_status();
}

static arbor_status template_scan_literal_add(
    view_template_scan_output *scan,
    arbor_span source,
    uint64_t start,
    uint64_t end)
{
    if (end < start) {
        return invalid_argument_status();
    }
    const uint64_t length = end - start;
    if (length == 0u) {
        return ok_status();
    }

    arbor_status status = template_scan_count_add(&scan->requirements.part_count);
    if (status.native != 0) {
        return status;
    }
    arbor_asm_result_u64 literal_total = u64_add_checked(
        scan->requirements.literal_bytes,
        length);
    if (literal_total.status != 0) {
        return arbor_status_from_native(literal_total.status);
    }

    if (scan->emit) {
        uint8_t *destination = scan->literal_bytes + scan->literal_offset;
        (void)memory_copy(destination, source.data + start, length);
        scan->parts[scan->part_index] = (arbor_view_html_template_part){
            .literal = {destination, length},
            .kind = VIEW_TEMPLATE_PART_LITERAL,
            .slot = 0u
        };
        scan->part_index += 1u;
        scan->literal_offset += length;
    }

    scan->requirements.literal_bytes = literal_total.value;
    return ok_status();
}

static arbor_status template_scan_slot_add(
    view_template_scan_output *scan,
    uint64_t slot)
{
    arbor_status status = template_scan_count_add(&scan->requirements.part_count);
    if (status.native != 0) {
        return status;
    }
    if (scan->emit) {
        scan->parts[scan->part_index] = (arbor_view_html_template_part){
            .literal = {NULL, 0u},
            .kind = VIEW_TEMPLATE_PART_HTML_TEXT,
            .slot = slot
        };
        scan->part_index += 1u;
    }
    return ok_status();
}

static bool template_char_ref_candidate_continue(uint8_t ch)
{
    return ascii_alpha(ch) || ascii_digit(ch) || ch == (uint8_t)'#';
}

static view_template_raw_kind template_raw_kind_from_name(arbor_span name)
{
    if (ascii_name_equals(name, "script", 6u)) {
        return VIEW_TEMPLATE_RAW_SCRIPT;
    }
    if (ascii_name_equals(name, "style", 5u)) {
        return VIEW_TEMPLATE_RAW_STYLE;
    }
    if (ascii_name_equals(name, "title", 5u)) {
        return VIEW_TEMPLATE_RAW_TITLE;
    }
    if (ascii_name_equals(name, "textarea", 8u)) {
        return VIEW_TEMPLATE_RAW_TEXTAREA;
    }
    if (ascii_name_equals(name, "xmp", 3u)) {
        return VIEW_TEMPLATE_RAW_XMP;
    }
    if (ascii_name_equals(name, "iframe", 6u)) {
        return VIEW_TEMPLATE_RAW_IFRAME;
    }
    if (ascii_name_equals(name, "noembed", 7u)) {
        return VIEW_TEMPLATE_RAW_NOEMBED;
    }
    if (ascii_name_equals(name, "noframes", 8u)) {
        return VIEW_TEMPLATE_RAW_NOFRAMES;
    }
    if (ascii_name_equals(name, "noscript", 8u)) {
        return VIEW_TEMPLATE_RAW_NOSCRIPT;
    }
    if (ascii_name_equals(name, "plaintext", 9u)) {
        return VIEW_TEMPLATE_RAW_PLAINTEXT;
    }
    return VIEW_TEMPLATE_RAW_NONE;
}

static arbor_span template_raw_kind_name(view_template_raw_kind kind)
{
    switch (kind) {
    case VIEW_TEMPLATE_RAW_SCRIPT:
        return (arbor_span){(const uint8_t *)"script", 6u};
    case VIEW_TEMPLATE_RAW_STYLE:
        return (arbor_span){(const uint8_t *)"style", 5u};
    case VIEW_TEMPLATE_RAW_TITLE:
        return (arbor_span){(const uint8_t *)"title", 5u};
    case VIEW_TEMPLATE_RAW_TEXTAREA:
        return (arbor_span){(const uint8_t *)"textarea", 8u};
    case VIEW_TEMPLATE_RAW_XMP:
        return (arbor_span){(const uint8_t *)"xmp", 3u};
    case VIEW_TEMPLATE_RAW_IFRAME:
        return (arbor_span){(const uint8_t *)"iframe", 6u};
    case VIEW_TEMPLATE_RAW_NOEMBED:
        return (arbor_span){(const uint8_t *)"noembed", 7u};
    case VIEW_TEMPLATE_RAW_NOFRAMES:
        return (arbor_span){(const uint8_t *)"noframes", 8u};
    case VIEW_TEMPLATE_RAW_NOSCRIPT:
        return (arbor_span){(const uint8_t *)"noscript", 8u};
    case VIEW_TEMPLATE_RAW_PLAINTEXT:
    case VIEW_TEMPLATE_RAW_NONE:
        return (arbor_span){NULL, 0u};
    }
    return (arbor_span){NULL, 0u};
}

static bool template_tag_name_boundary(uint8_t ch)
{
    return ch == (uint8_t)'>' || ch == (uint8_t)'/' ||
           ch == (uint8_t)' ' || ch == (uint8_t)'\t' ||
           ch == (uint8_t)'\n' || ch == (uint8_t)'\r' ||
           ch == (uint8_t)'\f';
}

static arbor_status template_scan_tag(
    arbor_span source,
    uint64_t start,
    uint64_t *end_out,
    bool *is_end_out,
    arbor_span *name_out)
{
    if (start >= source.length || source.data[start] != (uint8_t)'<' ||
        end_out == NULL || is_end_out == NULL || name_out == NULL) {
        return invalid_argument_status();
    }

    uint64_t cursor = start + 1u;
    bool is_end = false;
    if (cursor < source.length && source.data[cursor] == (uint8_t)'/') {
        is_end = true;
        cursor += 1u;
    }

    const uint64_t name_start = cursor;
    while (cursor < source.length &&
           !template_tag_name_boundary(source.data[cursor])) {
        cursor += 1u;
    }
    arbor_span name = {
        .data = cursor > name_start ? source.data + name_start : NULL,
        .length = cursor - name_start
    };

    uint8_t quote = 0u;
    for (uint64_t i = cursor; i < source.length; ++i) {
        const uint8_t ch = source.data[i];
        if (ch == (uint8_t)'{' && i + 1u < source.length &&
            source.data[i + 1u] == (uint8_t)'{') {
            return invalid_argument_status();
        }
        if (quote != 0u) {
            if (ch == quote) {
                quote = 0u;
            }
            continue;
        }
        if (ch == (uint8_t)'\'' || ch == (uint8_t)'"') {
            quote = ch;
            continue;
        }
        if (ch == (uint8_t)'>') {
            *end_out = i + 1u;
            *is_end_out = is_end;
            *name_out = name;
            return ok_status();
        }
    }

    return invalid_argument_status();
}

static arbor_status template_scan_comment(arbor_span source, uint64_t start, uint64_t *end_out)
{
    if (start + 4u > source.length ||
        source.data[start] != (uint8_t)'<' ||
        source.data[start + 1u] != (uint8_t)'!' ||
        source.data[start + 2u] != (uint8_t)'-' ||
        source.data[start + 3u] != (uint8_t)'-' ||
        end_out == NULL) {
        return invalid_argument_status();
    }

    for (uint64_t i = start + 4u; i < source.length; ++i) {
        if (source.data[i] == (uint8_t)'{' && i + 1u < source.length &&
            source.data[i + 1u] == (uint8_t)'{') {
            return invalid_argument_status();
        }
        if (i + 2u < source.length &&
            source.data[i] == (uint8_t)'-' &&
            source.data[i + 1u] == (uint8_t)'-' &&
            source.data[i + 2u] == (uint8_t)'>') {
            *end_out = i + 3u;
            return ok_status();
        }
    }
    return invalid_argument_status();
}

static bool template_raw_end_matches(
    arbor_span source,
    uint64_t start,
    view_template_raw_kind raw_kind)
{
    arbor_span name = template_raw_kind_name(raw_kind);
    if (name.length == 0u || start + 2u + name.length > source.length ||
        source.data[start] != (uint8_t)'<' ||
        source.data[start + 1u] != (uint8_t)'/') {
        return false;
    }
    for (uint64_t i = 0u; i < name.length; ++i) {
        if (ascii_lower(source.data[start + 2u + i]) != name.data[i]) {
            return false;
        }
    }
    const uint64_t boundary = start + 2u + name.length;
    return boundary == source.length ||
           template_tag_name_boundary(source.data[boundary]);
}

static arbor_status template_parse_placeholder(
    arbor_span source,
    uint64_t start,
    const arbor_span *field_names,
    uint64_t field_count,
    uint64_t *end_out,
    uint64_t *slot_out)
{
    if (start + 2u > source.length ||
        source.data[start] != (uint8_t)'{' ||
        source.data[start + 1u] != (uint8_t)'{' ||
        end_out == NULL || slot_out == NULL) {
        return invalid_argument_status();
    }

    uint64_t cursor = start + 2u;
    while (cursor < source.length &&
           (source.data[cursor] == (uint8_t)' ' ||
            source.data[cursor] == (uint8_t)'\t')) {
        cursor += 1u;
    }
    if (cursor >= source.length || !ascii_identifier_start(source.data[cursor])) {
        return invalid_argument_status();
    }

    const uint64_t name_start = cursor;
    cursor += 1u;
    while (cursor < source.length && ascii_identifier_continue(source.data[cursor])) {
        cursor += 1u;
    }
    const arbor_span name = {source.data + name_start, cursor - name_start};

    while (cursor < source.length &&
           (source.data[cursor] == (uint8_t)' ' ||
            source.data[cursor] == (uint8_t)'\t')) {
        cursor += 1u;
    }
    if (cursor + 2u > source.length ||
        source.data[cursor] != (uint8_t)'}' ||
        source.data[cursor + 1u] != (uint8_t)'}') {
        return invalid_argument_status();
    }

    arbor_status status = template_field_resolve(
        field_names,
        field_count,
        name,
        slot_out);
    if (status.native != 0) {
        return status;
    }
    *end_out = cursor + 2u;
    return ok_status();
}

static arbor_status template_scan(
    arbor_span source,
    const arbor_span *field_names,
    uint64_t field_count,
    view_template_scan_output *scan)
{
    if (scan == NULL || !span_range_representable(scan, sizeof(*scan)) ||
        !span_range_representable(source.data, source.length)) {
        return invalid_argument_status();
    }

    scan->requirements = (arbor_view_html_template_requirements){0u, 0u};
    scan->part_index = 0u;
    scan->literal_offset = 0u;

    uint64_t literal_start = 0u;
    uint64_t cursor = 0u;
    bool char_ref_candidate = false;
    view_template_raw_kind raw_kind = VIEW_TEMPLATE_RAW_NONE;

    while (cursor < source.length) {
        if (raw_kind != VIEW_TEMPLATE_RAW_NONE) {
            if (source.data[cursor] == (uint8_t)'{' &&
                cursor + 1u < source.length &&
                source.data[cursor + 1u] == (uint8_t)'{') {
                return invalid_argument_status();
            }
            if (raw_kind != VIEW_TEMPLATE_RAW_PLAINTEXT &&
                source.data[cursor] == (uint8_t)'<' &&
                template_raw_end_matches(source, cursor, raw_kind)) {
                uint64_t tag_end = 0u;
                bool is_end = false;
                arbor_span tag_name = {0};
                arbor_status status = template_scan_tag(
                    source,
                    cursor,
                    &tag_end,
                    &is_end,
                    &tag_name);
                if (status.native != 0 || !is_end) {
                    return invalid_argument_status();
                }
                cursor = tag_end;
                raw_kind = VIEW_TEMPLATE_RAW_NONE;
                char_ref_candidate = false;
                continue;
            }
            cursor += 1u;
            continue;
        }

        if (source.data[cursor] == (uint8_t)'<' &&
            cursor + 3u < source.length &&
            source.data[cursor + 1u] == (uint8_t)'!' &&
            source.data[cursor + 2u] == (uint8_t)'-' &&
            source.data[cursor + 3u] == (uint8_t)'-') {
            uint64_t comment_end = 0u;
            arbor_status status = template_scan_comment(source, cursor, &comment_end);
            if (status.native != 0) {
                return status;
            }
            cursor = comment_end;
            char_ref_candidate = false;
            continue;
        }

        if (source.data[cursor] == (uint8_t)'<') {
            uint64_t tag_end = 0u;
            bool is_end = false;
            arbor_span tag_name = {0};
            arbor_status status = template_scan_tag(
                source,
                cursor,
                &tag_end,
                &is_end,
                &tag_name);
            if (status.native != 0) {
                return status;
            }

            if (!is_end &&
                (ascii_name_equals(tag_name, "svg", 3u) ||
                 ascii_name_equals(tag_name, "math", 4u))) {
                return invalid_argument_status();
            }

            if (!is_end) {
                raw_kind = template_raw_kind_from_name(tag_name);
            }
            cursor = tag_end;
            char_ref_candidate = false;
            continue;
        }

        if (source.data[cursor] == (uint8_t)'{' &&
            cursor + 1u < source.length &&
            source.data[cursor + 1u] == (uint8_t)'{') {
            if (char_ref_candidate) {
                return invalid_argument_status();
            }

            uint64_t placeholder_end = 0u;
            uint64_t slot = 0u;
            arbor_status status = template_parse_placeholder(
                source,
                cursor,
                field_names,
                field_count,
                &placeholder_end,
                &slot);
            if (status.native != 0) {
                return status;
            }

            status = template_scan_literal_add(
                scan,
                source,
                literal_start,
                cursor);
            if (status.native != 0) {
                return status;
            }
            status = template_scan_slot_add(scan, slot);
            if (status.native != 0) {
                return status;
            }

            literal_start = placeholder_end;
            cursor = placeholder_end;
            char_ref_candidate = false;
            continue;
        }

        const uint8_t ch = source.data[cursor];
        if (ch == (uint8_t)'&') {
            char_ref_candidate = true;
        } else if (char_ref_candidate && !template_char_ref_candidate_continue(ch)) {
            char_ref_candidate = false;
        }
        cursor += 1u;
    }

    arbor_status status = template_scan_literal_add(
        scan,
        source,
        literal_start,
        source.length);
    if (status.native != 0) {
        return status;
    }

    if (scan->emit &&
        (scan->part_index != scan->requirements.part_count ||
         scan->literal_offset != scan->requirements.literal_bytes)) {
        return invalid_argument_status();
    }
    return ok_status();
}

static bool region_contains_span(
    const void *region,
    uint64_t region_length,
    const void *span,
    uint64_t span_length)
{
    if (span_length == 0u) {
        return true;
    }
    if (!span_range_representable(region, region_length) ||
        !span_range_representable(span, span_length)) {
        return false;
    }

    arbor_asm_result_u64 region_end = range_end_checked(
        (uint64_t)(uintptr_t)region,
        region_length);
    arbor_asm_result_u64 span_end = range_end_checked(
        (uint64_t)(uintptr_t)span,
        span_length);
    if (region_end.status != 0 || span_end.status != 0) {
        return false;
    }

    const uint64_t region_start = (uint64_t)(uintptr_t)region;
    const uint64_t span_start = (uint64_t)(uintptr_t)span;
    return span_start >= region_start && span_end.value <= region_end.value;
}

static arbor_status template_prepared_validate(
    const arbor_view_html_template *template_view,
    uint64_t *parts_bytes_out)
{
    if (template_view == NULL ||
        !span_range_representable(template_view, sizeof(*template_view)) ||
        parts_bytes_out == NULL ||
        !span_range_representable(parts_bytes_out, sizeof(*parts_bytes_out)) ||
        template_view->part_count_guard != ~template_view->part_count ||
        template_view->value_count_guard != ~template_view->value_count) {
        return invalid_argument_status();
    }

    arbor_asm_result_u64 part_bytes = u64_mul_checked(
        template_view->part_count,
        (uint64_t)sizeof(arbor_view_html_template_part));
    if (part_bytes.status != 0) {
        return arbor_status_from_native(part_bytes.status);
    }
    if (!span_range_representable(template_view->parts, part_bytes.value) ||
        !span_range_representable(
            template_view->literal_bytes,
            template_view->literal_length) ||
        spans_overlap_known_representable(
            template_view,
            sizeof(*template_view),
            template_view->parts,
            part_bytes.value) ||
        spans_overlap_known_representable(
            template_view,
            sizeof(*template_view),
            template_view->literal_bytes,
            template_view->literal_length) ||
        spans_overlap_known_representable(
            template_view->parts,
            part_bytes.value,
            template_view->literal_bytes,
            template_view->literal_length)) {
        return invalid_argument_status();
    }

    uint64_t literal_offset = 0u;
    for (uint64_t i = 0u; i < template_view->part_count; ++i) {
        const arbor_view_html_template_part *part = &template_view->parts[i];
        if (part->kind == VIEW_TEMPLATE_PART_LITERAL) {
            if (part->literal.length == 0u || part->slot != 0u ||
                part->literal.data != template_view->literal_bytes + literal_offset ||
                !region_contains_span(
                    template_view->literal_bytes,
                    template_view->literal_length,
                    part->literal.data,
                    part->literal.length)) {
                return invalid_argument_status();
            }
            arbor_asm_result_u64 next = u64_add_checked(
                literal_offset,
                part->literal.length);
            if (next.status != 0) {
                return arbor_status_from_native(next.status);
            }
            literal_offset = next.value;
        } else if (part->kind == VIEW_TEMPLATE_PART_HTML_TEXT) {
            if (part->literal.data != NULL || part->literal.length != 0u ||
                part->slot >= template_view->value_count) {
                return invalid_argument_status();
            }
        } else {
            return invalid_argument_status();
        }
    }
    if (literal_offset != template_view->literal_length) {
        return invalid_argument_status();
    }

    *parts_bytes_out = part_bytes.value;
    return ok_status();
}

static bool region_overlaps_field_inputs(
    const void *region,
    uint64_t region_length,
    arbor_span source,
    const arbor_span *field_names,
    uint64_t field_count,
    uint64_t field_bytes)
{
    if (spans_overlap_known_representable(
            region, region_length, source.data, source.length) ||
        spans_overlap_known_representable(
            region, region_length, field_names, field_bytes)) {
        return true;
    }
    for (uint64_t i = 0u; i < field_count; ++i) {
        if (spans_overlap_known_representable(
                region,
                region_length,
                field_names[i].data,
                field_names[i].length)) {
            return true;
        }
    }
    return false;
}

arbor_status arbor_view_html_template_measure(
    arbor_span source,
    const arbor_span *field_names,
    uint64_t field_count,
    arbor_view_html_template_requirements *requirements_out)
{
    if (requirements_out == NULL ||
        !span_range_representable(requirements_out, sizeof(*requirements_out)) ||
        !span_range_representable(source.data, source.length)) {
        return invalid_argument_status();
    }

    uint64_t field_bytes = 0u;
    arbor_status status = template_field_table_validate(
        field_names,
        field_count,
        &field_bytes);
    if (status.native != 0) {
        return status;
    }
    if (region_overlaps_field_inputs(
            requirements_out,
            sizeof(*requirements_out),
            source,
            field_names,
            field_count,
            field_bytes)) {
        return invalid_argument_status();
    }

    view_template_scan_output scan = {0};
    status = template_scan(source, field_names, field_count, &scan);
    if (status.native != 0) {
        return status;
    }

    *requirements_out = scan.requirements;
    return ok_status();
}

arbor_status arbor_view_html_template_prepare(
    arbor_span source,
    const arbor_span *field_names,
    uint64_t field_count,
    arbor_view_html_template_storage *storage,
    arbor_view_html_template *template_out)
{
    if (storage == NULL || template_out == NULL ||
        !span_range_representable(storage, sizeof(*storage)) ||
        !span_range_representable(template_out, sizeof(*template_out)) ||
        !span_range_representable(source.data, source.length)) {
        return invalid_argument_status();
    }

    uint64_t field_bytes = 0u;
    arbor_status status = template_field_table_validate(
        field_names,
        field_count,
        &field_bytes);
    if (status.native != 0) {
        return status;
    }

    view_template_scan_output measured = {0};
    status = template_scan(source, field_names, field_count, &measured);
    if (status.native != 0) {
        return status;
    }

    arbor_asm_result_u64 part_capacity_bytes = u64_mul_checked(
        storage->part_capacity,
        (uint64_t)sizeof(arbor_view_html_template_part));
    if (part_capacity_bytes.status != 0) {
        return arbor_status_from_native(part_capacity_bytes.status);
    }
    arbor_asm_result_u64 required_part_bytes = u64_mul_checked(
        measured.requirements.part_count,
        (uint64_t)sizeof(arbor_view_html_template_part));
    if (required_part_bytes.status != 0) {
        return arbor_status_from_native(required_part_bytes.status);
    }

    if (!span_range_representable(storage->parts, part_capacity_bytes.value) ||
        !span_range_representable(storage->literal_bytes, storage->literal_capacity)) {
        return invalid_argument_status();
    }
    if (storage->part_capacity < measured.requirements.part_count ||
        storage->literal_capacity < measured.requirements.literal_bytes) {
        return no_space_status();
    }

    if (region_overlaps_field_inputs(
            storage,
            sizeof(*storage),
            source,
            field_names,
            field_count,
            field_bytes) ||
        region_overlaps_field_inputs(
            template_out,
            sizeof(*template_out),
            source,
            field_names,
            field_count,
            field_bytes) ||
        region_overlaps_field_inputs(
            storage->parts,
            required_part_bytes.value,
            source,
            field_names,
            field_count,
            field_bytes) ||
        region_overlaps_field_inputs(
            storage->literal_bytes,
            measured.requirements.literal_bytes,
            source,
            field_names,
            field_count,
            field_bytes) ||
        spans_overlap_known_representable(
            storage,
            sizeof(*storage),
            template_out,
            sizeof(*template_out)) ||
        spans_overlap_known_representable(
            storage,
            sizeof(*storage),
            storage->parts,
            required_part_bytes.value) ||
        spans_overlap_known_representable(
            storage,
            sizeof(*storage),
            storage->literal_bytes,
            measured.requirements.literal_bytes) ||
        spans_overlap_known_representable(
            template_out,
            sizeof(*template_out),
            storage->parts,
            required_part_bytes.value) ||
        spans_overlap_known_representable(
            template_out,
            sizeof(*template_out),
            storage->literal_bytes,
            measured.requirements.literal_bytes) ||
        spans_overlap_known_representable(
            storage->parts,
            required_part_bytes.value,
            storage->literal_bytes,
            measured.requirements.literal_bytes)) {
        return invalid_argument_status();
    }

    view_template_scan_output emitted = {
        .parts = storage->parts,
        .literal_bytes = storage->literal_bytes,
        .emit = true
    };
    status = template_scan(source, field_names, field_count, &emitted);
    if (status.native != 0 ||
        emitted.requirements.part_count != measured.requirements.part_count ||
        emitted.requirements.literal_bytes != measured.requirements.literal_bytes) {
        return status.native != 0 ? status : invalid_argument_status();
    }

    const arbor_view_html_template candidate = {
        .parts = measured.requirements.part_count != 0u ? storage->parts : NULL,
        .part_count = measured.requirements.part_count,
        .literal_bytes = measured.requirements.literal_bytes != 0u
            ? storage->literal_bytes
            : NULL,
        .literal_length = measured.requirements.literal_bytes,
        .value_count = field_count,
        .part_count_guard = ~measured.requirements.part_count,
        .value_count_guard = ~field_count
    };

    uint64_t prepared_part_bytes = 0u;
    status = template_prepared_validate(&candidate, &prepared_part_bytes);
    if (status.native != 0 || prepared_part_bytes != required_part_bytes.value) {
        return status.native != 0 ? status : invalid_argument_status();
    }

    *template_out = candidate;
    return ok_status();
}

static bool render_input_overlaps(
    const void *region,
    uint64_t region_length,
    const arbor_view_html_template *template_view,
    uint64_t parts_bytes,
    const arbor_span *values,
    uint64_t value_count,
    uint64_t values_bytes)
{
    if (spans_overlap_known_representable(
            region, region_length, template_view, sizeof(*template_view)) ||
        spans_overlap_known_representable(
            region, region_length, template_view->parts, parts_bytes) ||
        spans_overlap_known_representable(
            region,
            region_length,
            template_view->literal_bytes,
            template_view->literal_length) ||
        spans_overlap_known_representable(
            region, region_length, values, values_bytes)) {
        return true;
    }
    for (uint64_t i = 0u; i < value_count; ++i) {
        if (spans_overlap_known_representable(
                region, region_length, values[i].data, values[i].length)) {
            return true;
        }
    }
    return false;
}

arbor_status arbor_view_html_template_render(
    const arbor_view_html_template *template_view,
    const arbor_span *values,
    uint64_t value_count,
    arbor_asm_arena *request_arena,
    arbor_span *body_out)
{
    uint64_t parts_bytes = 0u;
    arbor_status status = template_prepared_validate(
        template_view,
        &parts_bytes);
    if (status.native != 0 || value_count != template_view->value_count) {
        return status.native != 0 ? status : invalid_argument_status();
    }

    arbor_asm_result_u64 values_bytes_result = u64_mul_checked(
        value_count,
        (uint64_t)sizeof(arbor_span));
    if (values_bytes_result.status != 0) {
        return arbor_status_from_native(values_bytes_result.status);
    }
    const uint64_t values_bytes = values_bytes_result.value;
    if (!span_range_representable(values, values_bytes) ||
        body_out == NULL ||
        !span_range_representable(body_out, sizeof(*body_out))) {
        return invalid_argument_status();
    }
    for (uint64_t i = 0u; i < value_count; ++i) {
        if (!span_range_representable(values[i].data, values[i].length)) {
            return invalid_argument_status();
        }
    }

    if (render_input_overlaps(
            body_out,
            sizeof(*body_out),
            template_view,
            parts_bytes,
            values,
            value_count,
            values_bytes)) {
        return invalid_argument_status();
    }

    status = arena_validate(request_arena);
    if (status.native != 0) {
        return status;
    }
    if (render_input_overlaps(
            request_arena,
            sizeof(*request_arena),
            template_view,
            parts_bytes,
            values,
            value_count,
            values_bytes)) {
        return invalid_argument_status();
    }

    arbor_view_measure measure = {0u};
    for (uint64_t i = 0u; i < template_view->part_count; ++i) {
        const arbor_view_html_template_part *part = &template_view->parts[i];
        if (part->kind == VIEW_TEMPLATE_PART_LITERAL) {
            status = arbor_view_measure_add(&measure, part->literal.length);
        } else {
            status = arbor_view_html_text_measure(
                &measure,
                values[part->slot]);
        }
        if (status.native != 0) {
            return status;
        }
    }

    if (measure.length <= request_arena->capacity - request_arena->offset) {
        arbor_asm_result_u64 future_address = range_end_checked(
            (uint64_t)(uintptr_t)request_arena->base,
            request_arena->offset);
        if (future_address.status != 0) {
            return arbor_status_from_native(future_address.status);
        }
        const void *future_body = (const void *)(uintptr_t)future_address.value;
        if (render_input_overlaps(
                future_body,
                measure.length,
                template_view,
                parts_bytes,
                values,
                value_count,
                values_bytes) ||
            spans_overlap_known_representable(
                future_body,
                measure.length,
                body_out,
                sizeof(*body_out))) {
            return invalid_argument_status();
        }
    }

    arbor_view_output output = {0};
    status = arbor_view_output_begin(
        request_arena,
        measure.length,
        &output);
    if (status.native != 0) {
        return status;
    }

    for (uint64_t i = 0u; i < template_view->part_count; ++i) {
        const arbor_view_html_template_part *part = &template_view->parts[i];
        if (part->kind == VIEW_TEMPLATE_PART_LITERAL) {
            status = arbor_view_output_append(&output, part->literal);
        } else {
            status = arbor_view_html_text_append(
                &output,
                values[part->slot]);
        }
        if (status.native != 0) {
            return status;
        }
    }

    return arbor_view_output_commit(&output, body_out);
}
