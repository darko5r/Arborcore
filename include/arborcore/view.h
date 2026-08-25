#ifndef ARBORCORE_VIEW_H
#define ARBORCORE_VIEW_H

#include <stdint.h>

#include <arborcore/arborcore.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ARBOR_VIEW_CORE_VERSION_MAJOR 0u
#define ARBOR_VIEW_CORE_VERSION_MINOR 1u
#define ARBOR_VIEW_CORE_VERSION_PATCH 0u

#define ARBOR_VIEW_OUTPUT_STATE_IDLE UINT64_C(0)
#define ARBOR_VIEW_OUTPUT_STATE_ACTIVE UINT64_C(1)
#define ARBOR_VIEW_OUTPUT_STATE_COMMITTED UINT64_C(2)

typedef struct arbor_view_measure {
    uint64_t length;
} arbor_view_measure;

/*
 * Caller-owned transaction metadata. Zero-initialize before first use.
 * Fields are framework-private by contract: after begin succeeds, callers must
 * not inspect or mutate them until commit/abort returns. Rendered bytes are
 * owned by arena storage; this object owns no heap memory.
 */
typedef struct arbor_view_output {
    arbor_asm_arena *arena;
    uint64_t arena_mark;
    arbor_asm_buffer buffer;
    uint64_t required_length;
    uint64_t state;
} arbor_view_output;

/* Transactionally add a byte count to a valid writable measurement object. */
arbor_status arbor_view_measure_add(arbor_view_measure *measure, uint64_t length);

/*
 * Begin an exact-size rendering transaction. arena and out must be valid live
 * writable objects; non-empty arena backing must be valid live writable storage.
 * The arena allocation frontier must not be mutated by other code until
 * commit/abort completes.
 */
arbor_status arbor_view_output_begin(
    arbor_asm_arena *arena,
    uint64_t required_length,
    arbor_view_output *out);

/*
 * Append bytes with snapshot semantics, including overlap with body bytes.
 * For nonzero length, bytes.data must designate a valid live readable span.
 * A failure after active transaction validation rewinds automatically; a
 * transaction-metadata/frontier validation failure fails closed without an
 * unsafe rewind.
 */
arbor_status arbor_view_output_append(arbor_view_output *output, arbor_span bytes);

/*
 * Publish the exact rendered body only when required_length bytes were written.
 * body_out must be a valid live writable non-aliasing object. A failure after
 * active transaction validation rewinds automatically; validation failure
 * before safe rewind preconditions are established fails closed without rewind.
 */
arbor_status arbor_view_output_commit(arbor_view_output *output, arbor_span *body_out);

/*
 * Rewind the arena to the transaction mark and return output to idle state.
 * Rewind occurs only after active metadata/frontier integrity validates.
 */
arbor_status arbor_view_output_abort(arbor_view_output *output);

/*
 * Measure dynamic text for ordinary HTML Data/text context.
 * This is not an attribute, URL, CSS, JavaScript, raw-text, comment, or XML
 * escaper. For nonzero length, text.data must designate a valid live readable
 * span that does not overlap the writable measurement object. The measurement
 * is failure-atomic and does not snapshot text for a later render pass.
 */
arbor_status arbor_view_html_text_measure(
    arbor_view_measure *measure,
    arbor_span text);

/*
 * Append dynamic text for ordinary HTML Data/text context. '&', '<', and '>'
 * are serialized as "&amp;", "&lt;", and "&gt;" respectively; all other
 * bytes, including quote bytes, are preserved. text must not overlap active
 * VIEW transaction metadata or the reserved output body. A source in an
 * earlier non-overlapping region of the same request arena is allowed.
 *
 * A failure after active transaction validation rewinds the VIEW transaction.
 * This function does not validate Unicode/UTF-8. Higher-level HTML integration
 * must establish UTF-8 representation validity. For a coherent two-pass render,
 * callers must keep text stable from measurement through the corresponding
 * append; C2 provides no cross-call snapshot.
 */
arbor_status arbor_view_html_text_append(
    arbor_view_output *output,
    arbor_span text);


/*
 * Validate that bytes are a well-formed UTF-8 byte sequence. This is a
 * representation-validity primitive: it does not normalize, strip a BOM,
 * validate HTML structure/content models, or change bytes. For nonzero length,
 * bytes.data must designate a valid live readable span.
 */
arbor_status arbor_view_utf8_validate(arbor_span bytes);


/*
 * T1 minimal prepared HTML template support.
 *
 * Template source and field-name arrays are borrowed only during measurement
 * and preparation. Successful preparation copies every trusted literal byte
 * into caller-owned application-lifetime storage and resolves each named
 * substitution to a numeric value slot. The prepared representation and its
 * backing storage become immutable by contract until no render can reference
 * them.
 */
typedef struct arbor_view_html_template_requirements {
    uint64_t part_count;
    uint64_t literal_bytes;
} arbor_view_html_template_requirements;

/*
 * Caller-allocated persistent backing for a prepared template.
 * Fields written by preparation are framework-private thereafter.
 */
typedef struct arbor_view_html_template_part {
    arbor_span literal;
    uint64_t kind;
    uint64_t slot;
} arbor_view_html_template_part;

typedef struct arbor_view_html_template_storage {
    arbor_view_html_template_part *parts;
    uint64_t part_capacity;
    uint8_t *literal_bytes;
    uint64_t literal_capacity;
} arbor_view_html_template_storage;

typedef struct arbor_view_html_template {
    const arbor_view_html_template_part *parts;
    uint64_t part_count;
    const uint8_t *literal_bytes;
    uint64_t literal_length;
    uint64_t value_count;
    uint64_t part_count_guard;
    uint64_t value_count_guard;
} arbor_view_html_template;

/*
 * Validate and measure a template source using the T1 grammar.
 *
 * Named placeholders use {{name}} or {{ name }} with ASCII space/tab around
 * an identifier [A-Za-z_][A-Za-z0-9_]*. Placeholders are admitted only in the
 * scanner's ordinary HTML Data/text context; tag/attribute/comment and
 * raw/RCDATA-like contexts are rejected for dynamic substitution. Named fields
 * must be unique valid identifiers. requirements_out is failure-atomic.
 */
arbor_status arbor_view_html_template_measure(
    arbor_span source,
    const arbor_span *field_names,
    uint64_t field_count,
    arbor_view_html_template_requirements *requirements_out);

/*
 * Prepare an immutable template in caller-owned application-lifetime storage.
 * The source and field names are preparation-only borrows and need not remain
 * alive after success. On expected validation/capacity failure, storage backing
 * and template_out are not modified. The backing arrays must not alias source,
 * field metadata/names, storage metadata, or template_out.
 */
arbor_status arbor_view_html_template_prepare(
    arbor_span source,
    const arbor_span *field_names,
    uint64_t field_count,
    arbor_view_html_template_storage *storage,
    arbor_view_html_template *template_out);

/*
 * Render a prepared template synchronously into request-arena storage.
 * values contains exactly template.value_count stable HTML-text values indexed
 * by slots resolved during preparation. No parsing or name lookup occurs here.
 * Successful publication is through the C1 exact transaction only.
 */
arbor_status arbor_view_html_template_render(
    const arbor_view_html_template *template_view,
    const arbor_span *values,
    uint64_t value_count,
    arbor_asm_arena *request_arena,
    arbor_span *body_out);

#ifdef __cplusplus
}
#endif

#endif
