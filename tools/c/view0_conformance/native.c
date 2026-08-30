#include <arborcore/view0_conformance/native.h>

#include "g03_r1a.h"
#include "g03_r2a.h"
#include "g03_r3a.h"
#include "g03_r4a.h"
#include "g03_r5a.h"
#include "g03_r7a.h"
#include "g04_r1a.h"
#include "g04_r2a.h"
#include "g05_r1a.h"
#include "g05_r2a.h"
#include "g05_r3a.h"
#include "g05_r4a.h"
#include "g06.h"
#include "g07.h"
#include "g08.h"
#include "g09.h"
#include "g10.h"
#include "g11.h"
#include "v1n2_c0.h"

#include <lexbor/core/mraw.h>

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void *arbor_view0_native_v1n2_g09_support_calloc(void *arena, size_t size) {
    return arena == NULL ? NULL : lexbor_mraw_calloc((lexbor_mraw_t *)arena, size);
}

void *arbor_view0_native_v1n2_g10_support_calloc(void *arena, size_t size) {
    return arena == NULL ? NULL : lexbor_mraw_calloc((lexbor_mraw_t *)arena, size);
}

void *arbor_view0_native_v1n2_g11_support_calloc(void *arena, size_t size) {
    return arena == NULL ? NULL : lexbor_mraw_calloc((lexbor_mraw_t *)arena, size);
}

_Static_assert(sizeof(uintptr_t) == sizeof(uint64_t), "V1N0 requires 64-bit uintptr_t");
_Static_assert(sizeof(size_t) <= sizeof(uint64_t), "V1N0 size_t must fit uint64_t");
_Static_assert(sizeof(arbor_view0_native_result) == 32u, "V1N0 result layout drift");
_Static_assert(sizeof(arbor_view0_native_diagnostic) == 352u, "V1N0 diagnostic layout drift");
_Static_assert(sizeof(arbor_view0_native_document_facts) == 184u,
               "V1N1 C0 document-facts layout drift");
_Static_assert(sizeof("ARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED") <=
                   ARBOR_VIEW0_NATIVE_RULE_SYMBOL_CAP,
               "G02 doctype-required symbolic name exceeds diagnostic capacity");
_Static_assert(sizeof("HTML document is missing the required DOCTYPE preamble") <=
                   ARBOR_VIEW0_NATIVE_MESSAGE_CAP,
               "G02 doctype-required message exceeds diagnostic capacity");
_Static_assert(sizeof("ARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX") <=
                   ARBOR_VIEW0_NATIVE_RULE_SYMBOL_CAP,
               "G02 doctype-syntax symbolic name exceeds diagnostic capacity");
_Static_assert(sizeof("HTML DOCTYPE does not match an admitted authoring form") <=
                   ARBOR_VIEW0_NATIVE_MESSAGE_CAP,
               "G02 doctype-syntax message exceeds diagnostic capacity");
_Static_assert(sizeof("ARBOR_VIEW_V1_G02_DOCTYPE_LEGACY_DISCOURAGED") <=
                   ARBOR_VIEW0_NATIVE_RULE_SYMBOL_CAP,
               "G02 legacy-doctype warning symbolic name exceeds diagnostic capacity");
_Static_assert(sizeof("Legacy DOCTYPE compatibility string should not be used unless required by a generator limitation") <=
                   ARBOR_VIEW0_NATIVE_MESSAGE_CAP,
               "G02 legacy-doctype warning message exceeds diagnostic capacity");
_Static_assert(sizeof("ARBOR_VIEW_V1_G02_HEAD_TITLE_CARDINALITY") <=
                   ARBOR_VIEW0_NATIVE_RULE_SYMBOL_CAP,
               "G02 head-title-cardinality symbolic name exceeds diagnostic capacity");
_Static_assert(sizeof("Standalone HTML document head must contain exactly one title element") <=
                   ARBOR_VIEW0_NATIVE_MESSAGE_CAP,
               "G02 head-title-cardinality message exceeds diagnostic capacity");
_Static_assert(sizeof("ARBOR_VIEW_V1_G02_HEAD_BASE_CARDINALITY") <=
                   ARBOR_VIEW0_NATIVE_RULE_SYMBOL_CAP,
               "G02 head-base-cardinality symbolic name exceeds diagnostic capacity");
_Static_assert(sizeof("HTML document head must contain no more than one base element") <=
                   ARBOR_VIEW0_NATIVE_MESSAGE_CAP,
               "G02 head-base-cardinality message exceeds diagnostic capacity");
_Static_assert(sizeof("ARBOR_VIEW_V1_G02_BODY_SINGLETON") <=
                   ARBOR_VIEW0_NATIVE_RULE_SYMBOL_CAP,
               "G02 body-singleton symbolic name exceeds diagnostic capacity");
_Static_assert(sizeof("HTML document must contain exactly one logical body element") <=
                   ARBOR_VIEW0_NATIVE_MESSAGE_CAP,
               "G02 body-singleton message exceeds diagnostic capacity");
_Static_assert(sizeof("ARBOR_VIEW_V1_G03_ELEMENT_CONTEXT") <=
                   ARBOR_VIEW0_NATIVE_RULE_SYMBOL_CAP,
               "G03 R1A element-context symbolic name exceeds diagnostic capacity");
_Static_assert(sizeof("ARBOR_VIEW_V1_G03_CONTENT_MODEL") <=
                   ARBOR_VIEW0_NATIVE_RULE_SYMBOL_CAP,
               "G03 R2A content-model symbolic name exceeds diagnostic capacity");
_Static_assert(sizeof("ARBOR_VIEW_V1_G03_DESCENDANT_EXCLUSIONS") <=
                   ARBOR_VIEW0_NATIVE_RULE_SYMBOL_CAP,
               "G03 R3A descendant-exclusions symbolic name exceeds diagnostic capacity");

static arbor_status status_from_errno_value(int value)
{
    return arbor_status_from_native(-(int64_t)value);
}

static arbor_status ok_status(void)
{
    return arbor_status_from_native(0);
}

static bool range_representable(const void *data, uint64_t length)
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

static bool overlap_known_representable(
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

static bool utf8_first_invalid_offset(arbor_span input, uint64_t *offset_out)
{
    if (offset_out == NULL) {
        return false;
    }

    uint64_t i = 0u;
    while (i < input.length) {
        const uint8_t first = input.data[i];
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
            *offset_out = i;
            return true;
        }

        if (input.length - i <= needed) {
            *offset_out = i;
            return true;
        }

        const uint8_t second = input.data[i + 1u];
        if (second < second_lower || second > second_upper) {
            *offset_out = i + 1u;
            return true;
        }

        for (uint64_t offset = 2u; offset <= needed; ++offset) {
            const uint8_t continuation = input.data[i + offset];
            if (continuation < UINT8_C(0x80) || continuation > UINT8_C(0xbf)) {
                *offset_out = i + offset;
                return true;
            }
        }

        i += needed + 1u;
    }

    return false;
}

static bool copy_literal(char *destination, uint64_t capacity, const char *literal)
{
    const size_t length = strlen(literal);
    if ((uint64_t)length >= capacity) {
        return false;
    }

    (void)memcpy(destination, literal, length + 1u);
    return true;
}


static bool ascii_whitespace(uint8_t byte)
{
    return byte == UINT8_C(0x09) || byte == UINT8_C(0x0a) ||
           byte == UINT8_C(0x0c) || byte == UINT8_C(0x0d) ||
           byte == UINT8_C(0x20);
}

static uint8_t ascii_lower(uint8_t byte)
{
    if (byte >= (uint8_t)'A' && byte <= (uint8_t)'Z') {
        return (uint8_t)(byte + ((uint8_t)'a' - (uint8_t)'A'));
    }
    return byte;
}

static bool ascii_case_equal_literal(
    arbor_span input,
    uint64_t offset,
    const char *literal)
{
    if (literal == NULL || offset > input.length) {
        return false;
    }
    const size_t literal_length_size = strlen(literal);
    if ((uint64_t)literal_length_size > input.length - offset) {
        return false;
    }
    for (uint64_t i = 0u; i < (uint64_t)literal_length_size; ++i) {
        if (ascii_lower(input.data[offset + i]) !=
            ascii_lower((uint8_t)literal[i])) {
            return false;
        }
    }
    return true;
}

typedef struct g02_doctype_syntax_issue {
    bool invalid;
    uint64_t byte_offset;
    uint64_t source_length;
} g02_doctype_syntax_issue;

static g02_doctype_syntax_issue syntax_issue_at(
    arbor_span input,
    uint64_t offset,
    uint64_t preferred_length)
{
    g02_doctype_syntax_issue issue = {true, offset, 0u};
    if (offset < input.length) {
        uint64_t available = input.length - offset;
        issue.source_length = preferred_length < available
            ? preferred_length
            : available;
        if (issue.source_length == 0u) {
            issue.source_length = 1u;
        }
    }
    return issue;
}

static g02_doctype_syntax_issue g02_doctype_syntax_check(
    arbor_span input,
    const arbor_view0_native_document_facts *facts)
{
    g02_doctype_syntax_issue valid = {false, 0u, 0u};
    if (facts == NULL || facts->source_doctype_count == 0u) {
        return valid;
    }

    const uint64_t keyword = facts->source_first_doctype_keyword_offset;
    if (keyword == ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE || keyword < 2u ||
        keyword > input.length) {
        return syntax_issue_at(input, 0u, 1u);
    }
    const uint64_t start = keyword - 2u;
    if (input.length - start < 2u || input.data[start] != (uint8_t)'<' ||
        input.data[start + 1u] != (uint8_t)'!') {
        return syntax_issue_at(input, start, 2u);
    }
    if (facts->source_first_doctype_keyword_length != 7u ||
        !ascii_case_equal_literal(input, keyword, "DOCTYPE")) {
        return syntax_issue_at(
            input,
            keyword,
            facts->source_first_doctype_keyword_length == 0u
                ? 1u
                : facts->source_first_doctype_keyword_length);
    }

    uint64_t cursor = keyword + 7u;
    if (cursor >= input.length || !ascii_whitespace(input.data[cursor])) {
        return syntax_issue_at(input, cursor, 1u);
    }
    do {
        cursor += 1u;
    } while (cursor < input.length && ascii_whitespace(input.data[cursor]));

    if (!ascii_case_equal_literal(input, cursor, "html")) {
        uint64_t length = facts->source_first_doctype_name_length;
        if (length == 0u) {
            length = 1u;
        }
        return syntax_issue_at(input, cursor, length);
    }
    cursor += 4u;

    const uint64_t after_name = cursor;
    while (cursor < input.length && ascii_whitespace(input.data[cursor])) {
        cursor += 1u;
    }
    if (cursor >= input.length) {
        return syntax_issue_at(input, cursor, 0u);
    }
    if (input.data[cursor] == (uint8_t)'>') {
        return valid;
    }
    if (cursor == after_name) {
        return syntax_issue_at(input, cursor, 1u);
    }

    if (!ascii_case_equal_literal(input, cursor, "SYSTEM")) {
        uint64_t length = facts->source_first_doctype_external_keyword_length;
        if (length == 0u) {
            length = 1u;
        }
        return syntax_issue_at(input, cursor, length);
    }
    cursor += 6u;
    if (cursor >= input.length || !ascii_whitespace(input.data[cursor])) {
        return syntax_issue_at(input, cursor, 1u);
    }
    do {
        cursor += 1u;
    } while (cursor < input.length && ascii_whitespace(input.data[cursor]));

    if (cursor >= input.length ||
        (input.data[cursor] != (uint8_t)'"' &&
         input.data[cursor] != (uint8_t)'\'')) {
        return syntax_issue_at(input, cursor, 1u);
    }
    const uint8_t quote = input.data[cursor];
    cursor += 1u;

    static const char legacy_value[] = "about:legacy-compat";
    const uint64_t legacy_length = (uint64_t)(sizeof(legacy_value) - 1u);
    if (legacy_length > input.length - cursor ||
        memcmp(input.data + cursor, legacy_value, (size_t)legacy_length) != 0) {
        uint64_t length = facts->source_first_doctype_system_id_length;
        if (length == 0u) {
            length = 1u;
        }
        return syntax_issue_at(input, cursor, length);
    }
    cursor += legacy_length;
    if (cursor >= input.length || input.data[cursor] != quote) {
        return syntax_issue_at(input, cursor, 1u);
    }
    cursor += 1u;

    while (cursor < input.length && ascii_whitespace(input.data[cursor])) {
        cursor += 1u;
    }
    if (cursor >= input.length || input.data[cursor] != (uint8_t)'>') {
        return syntax_issue_at(input, cursor, 1u);
    }

    return valid;
}

static bool g02_doctype_legacy_compat_anchor(
    arbor_span input,
    const arbor_view0_native_document_facts *facts,
    uint64_t *byte_offset_out,
    uint64_t *source_length_out)
{
    static const char legacy_value[] = "about:legacy-compat";
    const uint64_t legacy_length = (uint64_t)(sizeof(legacy_value) - 1u);

    if (facts == NULL || byte_offset_out == NULL || source_length_out == NULL ||
        facts->source_first_doctype_system_id_offset ==
            ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE ||
        facts->source_first_doctype_system_id_length != legacy_length) {
        return false;
    }

    const uint64_t offset = facts->source_first_doctype_system_id_offset;
    if (offset > input.length || legacy_length > input.length - offset ||
        memcmp(input.data + offset, legacy_value, (size_t)legacy_length) != 0) {
        return false;
    }

    *byte_offset_out = offset;
    *source_length_out = legacy_length;
    return true;
}

typedef struct g02_title_cardinality_issue {
    bool invalid;
    uint64_t byte_offset;
    uint64_t source_length;
} g02_title_cardinality_issue;

static arbor_status g02_head_title_cardinality_check(
    arbor_span input,
    const arbor_view0_native_document_facts *facts,
    g02_title_cardinality_issue *issue_out)
{
    if (facts == NULL || issue_out == NULL) {
        return status_from_errno_value(EINVAL);
    }

    g02_title_cardinality_issue issue = {false, 0u, 0u};
    if (facts->dom_head_title_child_count == 1u) {
        *issue_out = issue;
        return ok_status();
    }

    issue.invalid = true;
    if (facts->dom_head_title_child_count == 0u) {
        *issue_out = issue;
        return ok_status();
    }

    if (facts->source_title_start_tag_count < 2u ||
        facts->source_second_title_start_tag_offset ==
            ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE) {
        return status_from_errno_value(EIO);
    }

    const uint64_t offset = facts->source_second_title_start_tag_offset;
    if (offset > input.length || input.length - offset < 5u ||
        !ascii_case_equal_literal(input, offset, "title")) {
        return status_from_errno_value(EIO);
    }

    issue.byte_offset = offset;
    issue.source_length = 5u;
    *issue_out = issue;
    return ok_status();
}

typedef struct g02_base_cardinality_issue {
    bool invalid;
    uint64_t byte_offset;
    uint64_t source_length;
} g02_base_cardinality_issue;

static arbor_status g02_head_base_cardinality_check(
    arbor_span input,
    const arbor_view0_native_document_facts *facts,
    g02_base_cardinality_issue *issue_out)
{
    if (facts == NULL || issue_out == NULL) {
        return status_from_errno_value(EINVAL);
    }

    g02_base_cardinality_issue issue = {false, 0u, 0u};
    if (facts->dom_head_base_child_count <= 1u) {
        *issue_out = issue;
        return ok_status();
    }

    if (facts->source_base_start_tag_count < 2u ||
        facts->source_second_base_start_tag_offset ==
            ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE) {
        return status_from_errno_value(EIO);
    }

    const uint64_t offset = facts->source_second_base_start_tag_offset;
    if (offset > input.length || input.length - offset < 4u ||
        !ascii_case_equal_literal(input, offset, "base")) {
        return status_from_errno_value(EIO);
    }

    issue.invalid = true;
    issue.byte_offset = offset;
    issue.source_length = 4u;
    *issue_out = issue;
    return ok_status();
}

typedef struct g02_body_singleton_issue {
    bool invalid;
    uint64_t byte_offset;
    uint64_t source_length;
} g02_body_singleton_issue;

static arbor_status g02_body_singleton_check(
    arbor_span input,
    const arbor_view0_native_document_facts *facts,
    g02_body_singleton_issue *issue_out)
{
    if (facts == NULL || issue_out == NULL) {
        return status_from_errno_value(EINVAL);
    }

    g02_body_singleton_issue issue = {false, 0u, 0u};

    /*
     * A legal omitted <body> start tag still yields exactly one logical body
     * after text/html parsing. Multiple authored body start tags are a
     * distinct authoring violation even when the parser repairs them to the
     * same DOM body, so source evidence takes precedence for duplicates.
     */
    if (facts->source_body_start_tag_count > 1u) {
        if (facts->source_second_body_start_tag_offset ==
                ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE) {
            return status_from_errno_value(EIO);
        }

        const uint64_t offset = facts->source_second_body_start_tag_offset;
        if (offset > input.length || input.length - offset < 4u ||
            !ascii_case_equal_literal(input, offset, "body")) {
            return status_from_errno_value(EIO);
        }

        issue.invalid = true;
        issue.byte_offset = offset;
        issue.source_length = 4u;
        *issue_out = issue;
        return ok_status();
    }

    if (facts->dom_html_body_element_count == 1u) {
        *issue_out = issue;
        return ok_status();
    }

    if (facts->dom_html_body_element_count == 0u) {
        issue.invalid = true;
        *issue_out = issue;
        return ok_status();
    }

    /*
     * More than one final HTML body child with no duplicate authored body
     * token evidence is outside the admitted text/html parser model.
     */
    return status_from_errno_value(EIO);
}

static int diagnostic_compare(const void *left_void, const void *right_void)
{
    const arbor_view0_native_diagnostic *left =
        (const arbor_view0_native_diagnostic *)left_void;
    const arbor_view0_native_diagnostic *right =
        (const arbor_view0_native_diagnostic *)right_void;

    if (left->byte_offset < right->byte_offset) {
        return -1;
    }
    if (left->byte_offset > right->byte_offset) {
        return 1;
    }
    if (left->rule_id < right->rule_id) {
        return -1;
    }
    if (left->rule_id > right->rule_id) {
        return 1;
    }
    if (left->severity < right->severity) {
        return -1;
    }
    if (left->severity > right->severity) {
        return 1;
    }
    if (left->discovery_sequence < right->discovery_sequence) {
        return -1;
    }
    if (left->discovery_sequence > right->discovery_sequence) {
        return 1;
    }
    return 0;
}

static void assign_line_columns(
    arbor_span input,
    arbor_view0_native_diagnostic *diagnostics,
    uint64_t diagnostic_count)
{
    uint64_t scan_offset = 0u;
    uint64_t line = 1u;
    uint64_t column = 1u;

    for (uint64_t i = 0u; i < diagnostic_count; ++i) {
        const uint64_t target = diagnostics[i].byte_offset;

        while (scan_offset < target && scan_offset < input.length) {
            if (input.data[scan_offset] == (uint8_t)'\n') {
                line += 1u;
                column = 1u;
            } else {
                column += 1u;
            }
            scan_offset += 1u;
        }

        diagnostics[i].line = line;
        diagnostics[i].column = column;
    }
}


static void fill_g02_doctype_required_diagnostic(
    arbor_view0_native_diagnostic *diagnostic,
    uint64_t discovery_sequence)
{
    static const char symbolic_name[] = "ARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED";
    static const char message[] =
        "HTML document is missing the required DOCTYPE preamble";

    (void)memset(diagnostic, 0, sizeof(*diagnostic));
    diagnostic->rule_id = ARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED;
    diagnostic->byte_offset = 0u;
    diagnostic->source_length = 0u;
    diagnostic->discovery_sequence = discovery_sequence;
    diagnostic->severity = (uint32_t)ARBOR_VIEW0_NATIVE_SEVERITY_ERROR;
    diagnostic->origin = (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_AUTHORING;
    (void)memcpy(diagnostic->symbolic_name, symbolic_name, sizeof(symbolic_name));
    (void)memcpy(diagnostic->message, message, sizeof(message));
}


static void fill_g02_doctype_syntax_diagnostic(
    arbor_view0_native_diagnostic *diagnostic,
    uint64_t byte_offset,
    uint64_t source_length,
    uint64_t discovery_sequence)
{
    static const char symbolic_name[] = "ARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX";
    static const char message[] =
        "HTML DOCTYPE does not match an admitted authoring form";

    (void)memset(diagnostic, 0, sizeof(*diagnostic));
    diagnostic->rule_id = ARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX;
    diagnostic->byte_offset = byte_offset;
    diagnostic->source_length = source_length;
    diagnostic->discovery_sequence = discovery_sequence;
    diagnostic->severity = (uint32_t)ARBOR_VIEW0_NATIVE_SEVERITY_ERROR;
    diagnostic->origin = (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_AUTHORING;
    (void)memcpy(diagnostic->symbolic_name, symbolic_name, sizeof(symbolic_name));
    (void)memcpy(diagnostic->message, message, sizeof(message));
}

static void fill_g02_doctype_legacy_discouraged_diagnostic(
    arbor_view0_native_diagnostic *diagnostic,
    uint64_t byte_offset,
    uint64_t source_length,
    uint64_t discovery_sequence)
{
    static const char symbolic_name[] =
        "ARBOR_VIEW_V1_G02_DOCTYPE_LEGACY_DISCOURAGED";
    static const char message[] =
        "Legacy DOCTYPE compatibility string should not be used unless required by a generator limitation";

    (void)memset(diagnostic, 0, sizeof(*diagnostic));
    diagnostic->rule_id = ARBOR_VIEW_V1_G02_DOCTYPE_LEGACY_DISCOURAGED;
    diagnostic->byte_offset = byte_offset;
    diagnostic->source_length = source_length;
    diagnostic->discovery_sequence = discovery_sequence;
    diagnostic->severity = (uint32_t)ARBOR_VIEW0_NATIVE_SEVERITY_WARNING;
    diagnostic->origin = (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_AUTHORING;
    (void)memcpy(diagnostic->symbolic_name, symbolic_name, sizeof(symbolic_name));
    (void)memcpy(diagnostic->message, message, sizeof(message));
}

static void fill_g02_head_title_cardinality_diagnostic(
    arbor_view0_native_diagnostic *diagnostic,
    uint64_t byte_offset,
    uint64_t source_length,
    uint64_t discovery_sequence)
{
    static const char symbolic_name[] =
        "ARBOR_VIEW_V1_G02_HEAD_TITLE_CARDINALITY";
    static const char message[] =
        "Standalone HTML document head must contain exactly one title element";

    (void)memset(diagnostic, 0, sizeof(*diagnostic));
    diagnostic->rule_id = ARBOR_VIEW_V1_G02_HEAD_TITLE_CARDINALITY;
    diagnostic->byte_offset = byte_offset;
    diagnostic->source_length = source_length;
    diagnostic->discovery_sequence = discovery_sequence;
    diagnostic->severity = (uint32_t)ARBOR_VIEW0_NATIVE_SEVERITY_ERROR;
    diagnostic->origin = (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_AUTHORING;
    (void)memcpy(diagnostic->symbolic_name, symbolic_name, sizeof(symbolic_name));
    (void)memcpy(diagnostic->message, message, sizeof(message));
}

static void fill_g02_head_base_cardinality_diagnostic(
    arbor_view0_native_diagnostic *diagnostic,
    uint64_t byte_offset,
    uint64_t source_length,
    uint64_t discovery_sequence)
{
    static const char symbolic_name[] =
        "ARBOR_VIEW_V1_G02_HEAD_BASE_CARDINALITY";
    static const char message[] =
        "HTML document head must contain no more than one base element";

    (void)memset(diagnostic, 0, sizeof(*diagnostic));
    diagnostic->rule_id = ARBOR_VIEW_V1_G02_HEAD_BASE_CARDINALITY;
    diagnostic->byte_offset = byte_offset;
    diagnostic->source_length = source_length;
    diagnostic->discovery_sequence = discovery_sequence;
    diagnostic->severity = (uint32_t)ARBOR_VIEW0_NATIVE_SEVERITY_ERROR;
    diagnostic->origin = (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_AUTHORING;
    (void)memcpy(diagnostic->symbolic_name, symbolic_name, sizeof(symbolic_name));
    (void)memcpy(diagnostic->message, message, sizeof(message));
}

static void fill_g02_body_singleton_diagnostic(
    arbor_view0_native_diagnostic *diagnostic,
    uint64_t byte_offset,
    uint64_t source_length,
    uint64_t discovery_sequence)
{
    static const char symbolic_name[] =
        "ARBOR_VIEW_V1_G02_BODY_SINGLETON";
    static const char message[] =
        "HTML document must contain exactly one logical body element";

    (void)memset(diagnostic, 0, sizeof(*diagnostic));
    diagnostic->rule_id = ARBOR_VIEW_V1_G02_BODY_SINGLETON;
    diagnostic->byte_offset = byte_offset;
    diagnostic->source_length = source_length;
    diagnostic->discovery_sequence = discovery_sequence;
    diagnostic->severity = (uint32_t)ARBOR_VIEW0_NATIVE_SEVERITY_ERROR;
    diagnostic->origin = (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_AUTHORING;
    (void)memcpy(diagnostic->symbolic_name, symbolic_name, sizeof(symbolic_name));
    (void)memcpy(diagnostic->message, message, sizeof(message));
}

static arbor_status validate_output_regions(
    arbor_span input,
    arbor_view0_native_diagnostic *diagnostics,
    uint64_t diagnostic_capacity,
    arbor_view0_native_result *result_out,
    uint64_t *diagnostic_bytes_out)
{
    if (result_out == NULL || diagnostic_bytes_out == NULL ||
        !range_representable(result_out, sizeof(*result_out))) {
        return status_from_errno_value(EINVAL);
    }
    if (!range_representable(input.data, input.length)) {
        return status_from_errno_value(EINVAL);
    }
    if (input.length > ARBOR_VIEW0_NATIVE_MAX_INPUT_BYTES) {
        return status_from_errno_value(EFBIG);
    }
    if (diagnostic_capacity > ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS) {
        return status_from_errno_value(E2BIG);
    }
    if (diagnostic_capacity != 0u && diagnostics == NULL) {
        return status_from_errno_value(EINVAL);
    }

    arbor_asm_result_u64 bytes = u64_mul_checked(
        diagnostic_capacity,
        (uint64_t)sizeof(*diagnostics));
    if (bytes.status != 0) {
        return arbor_status_from_native(bytes.status);
    }
    if (!range_representable(diagnostics, bytes.value)) {
        return status_from_errno_value(EINVAL);
    }

    if (overlap_known_representable(input.data, input.length, result_out, sizeof(*result_out)) ||
        overlap_known_representable(input.data, input.length, diagnostics, bytes.value) ||
        overlap_known_representable(result_out, sizeof(*result_out), diagnostics, bytes.value)) {
        return status_from_errno_value(EINVAL);
    }

    *diagnostic_bytes_out = bytes.value;
    return ok_status();
}

arbor_status arbor_view0_native_check(
    arbor_span input,
    arbor_view0_native_diagnostic *diagnostics,
    uint64_t diagnostic_capacity,
    arbor_view0_native_result *result_out)
{
    uint64_t diagnostic_bytes = 0u;
    arbor_status status = validate_output_regions(
        input,
        diagnostics,
        diagnostic_capacity,
        result_out,
        &diagnostic_bytes);
    if (status.native != 0) {
        return status;
    }
    (void)diagnostic_bytes;

    if (!arbor_view0_native_v1n2_c0_validate()) {
        return status_from_errno_value(EIO);
    }

    arbor_status utf8 = arbor_view_utf8_validate(input);
    if (utf8.native != 0) {
        if (utf8.native != -(int64_t)EILSEQ) {
            return utf8;
        }
        if (diagnostic_capacity < 1u) {
            return status_from_errno_value(ENOSPC);
        }

        uint64_t bad_offset = 0u;
        if (!utf8_first_invalid_offset(input, &bad_offset) || bad_offset >= input.length) {
            return status_from_errno_value(EIO);
        }

        arbor_view0_native_diagnostic candidate = {0};
        candidate.rule_id = ARBOR_VIEW0_NATIVE_RULE_UTF8_INVALID;
        candidate.byte_offset = bad_offset;
        candidate.source_length = 1u;
        candidate.line = 0u;
        candidate.column = 0u;
        candidate.discovery_sequence = 0u;
        candidate.severity = (uint32_t)ARBOR_VIEW0_NATIVE_SEVERITY_ERROR;
        candidate.origin = (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_UTF8;
        candidate.external_id = 0u;
        if (!copy_literal(candidate.symbolic_name,
                          ARBOR_VIEW0_NATIVE_RULE_SYMBOL_CAP,
                          "html.utf8.invalid") ||
            !copy_literal(candidate.message,
                          ARBOR_VIEW0_NATIVE_MESSAGE_CAP,
                          "HTML input is not well-formed UTF-8")) {
            return status_from_errno_value(EOVERFLOW);
        }

        assign_line_columns(input, &candidate, 1u);
        diagnostics[0] = candidate;

        const arbor_view0_native_result result = {
            .diagnostic_count = 1u,
            .tokenizer_error_count = 0u,
            .tree_error_count = 0u,
            .flags = ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R1_PARTIAL |
                ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_PARTIAL |
                ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_PARTIAL |
                ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R4_PARTIAL |
                ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R5_PARTIAL |
                ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R7_PARTIAL |
                ARBOR_VIEW0_NATIVE_RESULT_FLAG_G04_R1_PARTIAL
        };
        *result_out = result;
        return ok_status();
    }

    if (input.length == 0u) {
        if (diagnostic_capacity < 2u) {
            return status_from_errno_value(ENOSPC);
        }

        arbor_view0_native_diagnostic candidates[2] = {{0}};
        fill_g02_doctype_required_diagnostic(&candidates[0], 0u);
        fill_g02_head_title_cardinality_diagnostic(&candidates[1], 0u, 0u, 1u);
        assign_line_columns(input, candidates, 2u);
        diagnostics[0] = candidates[0];
        diagnostics[1] = candidates[1];

        const arbor_view0_native_result result = {
            .diagnostic_count = 2u,
            .tokenizer_error_count = 0u,
            .tree_error_count = 0u,
            .flags = ARBOR_VIEW0_NATIVE_RESULT_FLAG_PARSE_CLEAN |
                ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R1_PARTIAL |
                ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_PARTIAL |
                ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_PARTIAL |
                ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R4_PARTIAL |
                ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R5_PARTIAL |
                ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R7_PARTIAL |
                ARBOR_VIEW0_NATIVE_RESULT_FLAG_G04_R1_PARTIAL
        };
        *result_out = result;
        return ok_status();
    }

    arbor_view0_native_parse_counts measured_counts = {0};
    arbor_view0_native_document_facts measured_facts = {0};
    status = arbor_view0_native_lexbor_measure(
        input,
        &measured_counts,
        &measured_facts);
    if (status.native != 0) {
        return status;
    }

    arbor_asm_result_u64 parse_total = u64_add_checked(
        measured_counts.tokenizer_error_count,
        measured_counts.tree_error_count);
    if (parse_total.status != 0) {
        return arbor_status_from_native(parse_total.status);
    }

    const uint64_t doctype_required_count =
        measured_facts.dom_doctype_node_count == 0u ? 1u : 0u;
    const g02_doctype_syntax_issue doctype_syntax_issue =
        g02_doctype_syntax_check(input, &measured_facts);
    const uint64_t doctype_syntax_count =
        doctype_syntax_issue.invalid ? 1u : 0u;

    uint64_t legacy_byte_offset = 0u;
    uint64_t legacy_source_length = 0u;
    const uint64_t doctype_legacy_discouraged_count =
        !doctype_syntax_issue.invalid &&
        g02_doctype_legacy_compat_anchor(
            input,
            &measured_facts,
            &legacy_byte_offset,
            &legacy_source_length)
            ? 1u
            : 0u;

    g02_title_cardinality_issue title_issue = {false, 0u, 0u};
    status = g02_head_title_cardinality_check(input, &measured_facts, &title_issue);
    if (status.native != 0) {
        return status;
    }
    const uint64_t head_title_cardinality_count = title_issue.invalid ? 1u : 0u;

    g02_base_cardinality_issue base_issue = {false, 0u, 0u};
    status = g02_head_base_cardinality_check(input, &measured_facts, &base_issue);
    if (status.native != 0) {
        return status;
    }
    const uint64_t head_base_cardinality_count = base_issue.invalid ? 1u : 0u;

    g02_body_singleton_issue body_issue = {false, 0u, 0u};
    status = g02_body_singleton_check(input, &measured_facts, &body_issue);
    if (status.native != 0) {
        return status;
    }
    const uint64_t body_singleton_count = body_issue.invalid ? 1u : 0u;

    arbor_view0_native_g03_r1a_evaluation r1a_measured = {0};
    status = arbor_view0_native_g03_r1a_measure(input, &r1a_measured);
    if (status.native != 0) {
        return status;
    }

    arbor_view0_native_g03_r2a_evaluation r2a_measured = {0};
    status = arbor_view0_native_g03_r2a_measure(input, &r2a_measured);
    if (status.native != 0) {
        return status;
    }

    arbor_view0_native_g03_r3a_evaluation r3a_measured = {0};
    status = arbor_view0_native_g03_r3a_measure(input, &r3a_measured);
    if (status.native != 0) {
        return status;
    }

    arbor_view0_native_g03_r4a_evaluation r4a_measured = {0};
    status = arbor_view0_native_g03_r4a_measure(input, &r4a_measured);
    if (status.native != 0) {
        return status;
    }

    arbor_asm_result_u64 r1_r2_total = u64_add_checked(
        doctype_required_count,
        doctype_syntax_count);
    if (r1_r2_total.status != 0) {
        return arbor_status_from_native(r1_r2_total.status);
    }
    arbor_asm_result_u64 r1_r3_total = u64_add_checked(
        r1_r2_total.value,
        doctype_legacy_discouraged_count);
    if (r1_r3_total.status != 0) {
        return arbor_status_from_native(r1_r3_total.status);
    }
    arbor_asm_result_u64 r1_r4_total = u64_add_checked(
        r1_r3_total.value,
        head_title_cardinality_count);
    if (r1_r4_total.status != 0) {
        return arbor_status_from_native(r1_r4_total.status);
    }
    arbor_asm_result_u64 r1_r5_total = u64_add_checked(
        r1_r4_total.value,
        head_base_cardinality_count);
    if (r1_r5_total.status != 0) {
        return arbor_status_from_native(r1_r5_total.status);
    }
    arbor_asm_result_u64 g02_total = u64_add_checked(
        r1_r5_total.value,
        body_singleton_count);
    if (g02_total.status != 0) {
        return arbor_status_from_native(g02_total.status);
    }
    arbor_asm_result_u64 g02_r1a_total = u64_add_checked(
        g02_total.value,
        r1a_measured.diagnostic_count);
    if (g02_r1a_total.status != 0) {
        return arbor_status_from_native(g02_r1a_total.status);
    }
    arbor_asm_result_u64 g02_r1a_r2a_total = u64_add_checked(
        g02_r1a_total.value,
        r2a_measured.diagnostic_count);
    if (g02_r1a_r2a_total.status != 0) {
        return arbor_status_from_native(g02_r1a_r2a_total.status);
    }
    arbor_asm_result_u64 g02_r1a_r2a_r3a_total = u64_add_checked(
        g02_r1a_r2a_total.value,
        r3a_measured.diagnostic_count);
    if (g02_r1a_r2a_r3a_total.status != 0) {
        return arbor_status_from_native(g02_r1a_r2a_r3a_total.status);
    }
    arbor_asm_result_u64 g02_r1a_r2a_r3a_r4a_total = u64_add_checked(
        g02_r1a_r2a_r3a_total.value,
        r4a_measured.diagnostic_count);
    if (g02_r1a_r2a_r3a_r4a_total.status != 0) {
        return arbor_status_from_native(g02_r1a_r2a_r3a_r4a_total.status);
    }
    arbor_asm_result_u64 pre_r5_required_total = u64_add_checked(
        parse_total.value,
        g02_r1a_r2a_r3a_r4a_total.value);
    if (pre_r5_required_total.status != 0) {
        return arbor_status_from_native(pre_r5_required_total.status);
    }
    if (pre_r5_required_total.value > ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS ||
        pre_r5_required_total.value > diagnostic_capacity) {
        return status_from_errno_value(ENOSPC);
    }

    arbor_view0_native_g03_r5a_evaluation r5a_measured = {0};
    status = arbor_view0_native_g03_r5a_measure(input, &r5a_measured);
    if (status.native != 0) {
        return status;
    }

    arbor_asm_result_u64 through_r5_total = u64_add_checked(
        g02_r1a_r2a_r3a_r4a_total.value,
        r5a_measured.diagnostic_count);
    if (through_r5_total.status != 0) {
        return arbor_status_from_native(through_r5_total.status);
    }
    arbor_asm_result_u64 pre_r7_required_total = u64_add_checked(
        parse_total.value, through_r5_total.value);
    if (pre_r7_required_total.status != 0) {
        return arbor_status_from_native(pre_r7_required_total.status);
    }
    if (pre_r7_required_total.value > ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS ||
        pre_r7_required_total.value > diagnostic_capacity) {
        return status_from_errno_value(ENOSPC);
    }

    arbor_view0_native_g03_r7a_evaluation r7a_measured = {0};
    status = arbor_view0_native_g03_r7a_measure(input, &r7a_measured);
    if (status.native != 0) {
        return status;
    }

    arbor_asm_result_u64 through_r7_total = u64_add_checked(
        through_r5_total.value,
        r7a_measured.diagnostic_count);
    if (through_r7_total.status != 0) {
        return arbor_status_from_native(through_r7_total.status);
    }
    arbor_asm_result_u64 pre_g04_r1_required_total = u64_add_checked(
        parse_total.value, through_r7_total.value);
    if (pre_g04_r1_required_total.status != 0) {
        return arbor_status_from_native(pre_g04_r1_required_total.status);
    }
    if (pre_g04_r1_required_total.value > ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS ||
        pre_g04_r1_required_total.value > diagnostic_capacity) {
        return status_from_errno_value(ENOSPC);
    }

    arbor_view0_native_g04_r1a_evaluation g04_r1a_measured = {0};
    status = arbor_view0_native_g04_r1a_measure(input, &g04_r1a_measured);
    if (status.native != 0) {
        return status;
    }

    arbor_view0_native_g05_r1a_evaluation g05_r1a_measured = {0};
    status = arbor_view0_native_g05_r1a_measure(input, &g05_r1a_measured);
    if (status.native != 0) {
        return status;
    }

    arbor_view0_native_g05_r2a_evaluation g05_r2a_measured = {0};
    status = arbor_view0_native_g05_r2a_measure(input, &g05_r2a_measured);
    if (status.native != 0) {
        return status;
    }

    arbor_view0_native_g05_r3a_evaluation g05_r3a_measured = {0};
    status = arbor_view0_native_g05_r3a_measure(input, &g05_r3a_measured);
    if (status.native != 0) {
        return status;
    }

    arbor_view0_native_g05_r4a_evaluation g05_r4a_measured = {0};
    status = arbor_view0_native_g05_r4a_measure(input, &g05_r4a_measured);
    if (status.native != 0) {
        return status;
    }

    arbor_view0_native_g06_evaluation g06_measured = {0};
    status = arbor_view0_native_g06_measure(input, &g06_measured);
    if (status.native != 0) {
        return status;
    }

    arbor_view0_native_v1n2_g07_evaluation g07_measured = {0};
    status = arbor_view0_native_v1n2_g07_measure(input, &g07_measured);
    if (status.native != 0) {
        return status;
    }

    arbor_view0_native_v1n2_g08_evaluation g08_measured = {0};
    status = arbor_view0_native_v1n2_g08_measure(input, &g08_measured);
    if (status.native != 0) {
        return status;
    }

    arbor_view0_native_v1n2_g09_evaluation g09_measured = {0};
    status = arbor_view0_native_v1n2_g09_measure(input, &g09_measured);
    if (status.native != 0) {
        return status;
    }

    arbor_view0_native_v1n2_g10_evaluation g10_measured = {0};
    status = arbor_view0_native_v1n2_g10_measure(input, &g10_measured);
    if (status.native != 0) {
        return status;
    }

    arbor_view0_native_v1n2_g11_evaluation g11_measured = {0};
    status = arbor_view0_native_v1n2_g11_measure(input, &g11_measured);
    if (status.native != 0) {
        return status;
    }

    arbor_asm_result_u64 through_g04_total = u64_add_checked(
        through_r7_total.value,
        g04_r1a_measured.diagnostic_count);
    if (through_g04_total.status != 0) {
        return arbor_status_from_native(through_g04_total.status);
    }
    arbor_asm_result_u64 through_g05_r1_total = u64_add_checked(
        through_g04_total.value,
        g05_r1a_measured.diagnostic_count);
    if (through_g05_r1_total.status != 0) {
        return arbor_status_from_native(through_g05_r1_total.status);
    }
    arbor_asm_result_u64 through_g05_r2_total = u64_add_checked(
        through_g05_r1_total.value,
        g05_r2a_measured.diagnostic_count);
    if (through_g05_r2_total.status != 0) {
        return arbor_status_from_native(through_g05_r2_total.status);
    }
    arbor_asm_result_u64 through_g05_r3_total = u64_add_checked(
        through_g05_r2_total.value,
        g05_r3a_measured.diagnostic_count);
    if (through_g05_r3_total.status != 0) {
        return arbor_status_from_native(through_g05_r3_total.status);
    }
    arbor_asm_result_u64 through_g05_total = u64_add_checked(
        through_g05_r3_total.value,
        g05_r4a_measured.diagnostic_count);
    if (through_g05_total.status != 0) {
        return arbor_status_from_native(through_g05_total.status);
    }
    arbor_asm_result_u64 through_g06_total = u64_add_checked(
        through_g05_total.value,
        g06_measured.diagnostic_count);
    if (through_g06_total.status != 0) {
        return arbor_status_from_native(through_g06_total.status);
    }
    arbor_asm_result_u64 through_g07_total = u64_add_checked(
        through_g06_total.value,
        g07_measured.diagnostic_count);
    if (through_g07_total.status != 0) {
        return arbor_status_from_native(through_g07_total.status);
    }
    arbor_asm_result_u64 through_g08_total = u64_add_checked(
        through_g07_total.value,
        g08_measured.diagnostic_count);
    if (through_g08_total.status != 0) {
        return arbor_status_from_native(through_g08_total.status);
    }
    arbor_asm_result_u64 through_g09_total = u64_add_checked(
        through_g08_total.value,
        g09_measured.diagnostic_count);
    if (through_g09_total.status != 0) {
        return arbor_status_from_native(through_g09_total.status);
    }
    arbor_asm_result_u64 through_g10_total = u64_add_checked(
        through_g09_total.value,
        g10_measured.diagnostic_count);
    if (through_g10_total.status != 0) {
        return arbor_status_from_native(through_g10_total.status);
    }
    arbor_asm_result_u64 authoring_total = u64_add_checked(
        through_g10_total.value,
        g11_measured.diagnostic_count);
    if (authoring_total.status != 0) {
        return arbor_status_from_native(authoring_total.status);
    }
    arbor_asm_result_u64 required_total = u64_add_checked(
        parse_total.value,
        authoring_total.value);
    if (required_total.status != 0) {
        return arbor_status_from_native(required_total.status);
    }
    if (required_total.value > ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS ||
        required_total.value > diagnostic_capacity) {
        return status_from_errno_value(ENOSPC);
    }

    arbor_view0_native_source_anchor authoring_anchors[ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS] = {{0}};
    arbor_view0_native_g06_anchor g06_anchors[ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS] = {0};
    arbor_view0_native_v1n2_anchor v1n2_anchors[ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS] = {0};
    uint64_t anchor_index = 0u;

    if (r1a_measured.diagnostic_count > ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS - anchor_index) {
        return status_from_errno_value(EIO);
    }
    arbor_view0_native_g03_r1a_evaluation r1a_collected = {0};
    status = arbor_view0_native_g03_r1a_collect_anchors(
        input, authoring_anchors + anchor_index, r1a_measured.diagnostic_count, &r1a_collected);
    if (status.native != 0) return status;
    if (r1a_collected.diagnostic_count != r1a_measured.diagnostic_count ||
        r1a_collected.deferred_main_form_count != r1a_measured.deferred_main_form_count) {
        return status_from_errno_value(EIO);
    }
    anchor_index += r1a_collected.diagnostic_count;

    if (r2a_measured.diagnostic_count > ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS - anchor_index) {
        return status_from_errno_value(EIO);
    }
    arbor_view0_native_g03_r2a_evaluation r2a_collected = {0};
    status = arbor_view0_native_g03_r2a_collect_anchors(
        input, authoring_anchors + anchor_index, r2a_measured.diagnostic_count, &r2a_collected);
    if (status.native != 0) return status;
    if (r2a_collected.diagnostic_count != r2a_measured.diagnostic_count ||
        r2a_collected.deferred_flags != r2a_measured.deferred_flags) {
        return status_from_errno_value(EIO);
    }
    anchor_index += r2a_collected.diagnostic_count;

    if (r3a_measured.diagnostic_count > ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS - anchor_index) {
        return status_from_errno_value(EIO);
    }
    arbor_view0_native_g03_r3a_evaluation r3a_collected = {0};
    status = arbor_view0_native_g03_r3a_collect_anchors(
        input, authoring_anchors + anchor_index, r3a_measured.diagnostic_count, &r3a_collected);
    if (status.native != 0) return status;
    if (r3a_collected.diagnostic_count != r3a_measured.diagnostic_count ||
        r3a_collected.deferred_flags != r3a_measured.deferred_flags) {
        return status_from_errno_value(EIO);
    }
    anchor_index += r3a_collected.diagnostic_count;

    if (r4a_measured.diagnostic_count > ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS - anchor_index) {
        return status_from_errno_value(EIO);
    }
    arbor_view0_native_g03_r4a_evaluation r4a_collected = {0};
    status = arbor_view0_native_g03_r4a_collect_anchors(
        input, authoring_anchors + anchor_index, r4a_measured.diagnostic_count, &r4a_collected);
    if (status.native != 0) return status;
    if (r4a_collected.diagnostic_count != r4a_measured.diagnostic_count ||
        r4a_collected.deferred_flags != r4a_measured.deferred_flags) {
        return status_from_errno_value(EIO);
    }
    anchor_index += r4a_collected.diagnostic_count;

    if (r5a_measured.diagnostic_count > ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS - anchor_index) {
        return status_from_errno_value(EIO);
    }
    arbor_view0_native_g03_r5a_evaluation r5a_collected = {0};
    status = arbor_view0_native_g03_r5a_collect_anchors(
        input, authoring_anchors + anchor_index, r5a_measured.diagnostic_count, &r5a_collected);
    if (status.native != 0) return status;
    if (r5a_collected.diagnostic_count != r5a_measured.diagnostic_count ||
        r5a_collected.prior_owner_suppression_count !=
            r5a_measured.prior_owner_suppression_count) {
        return status_from_errno_value(EIO);
    }
    anchor_index += r5a_collected.diagnostic_count;

    if (r7a_measured.diagnostic_count > ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS - anchor_index) {
        return status_from_errno_value(EIO);
    }
    arbor_view0_native_g03_r7a_evaluation r7a_collected = {0};
    status = arbor_view0_native_g03_r7a_collect_anchors(
        input, authoring_anchors + anchor_index, r7a_measured.diagnostic_count, &r7a_collected);
    if (status.native != 0) return status;
    if (r7a_collected.diagnostic_count != r7a_measured.diagnostic_count ||
        r7a_collected.deferred_flags != r7a_measured.deferred_flags ||
        r7a_collected.g04_deferred_count != r7a_measured.g04_deferred_count ||
        r7a_collected.g13_deferred_count != r7a_measured.g13_deferred_count) {
        return status_from_errno_value(EIO);
    }
    anchor_index += r7a_collected.diagnostic_count;

    if (g04_r1a_measured.diagnostic_count > ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS - anchor_index) {
        return status_from_errno_value(EIO);
    }
    arbor_view0_native_g04_r1a_evaluation g04_r1a_collected = {0};
    status = arbor_view0_native_g04_r1a_collect_anchors(
        input, authoring_anchors + anchor_index, g04_r1a_measured.diagnostic_count,
        &g04_r1a_collected);
    if (status.native != 0) return status;
    if (g04_r1a_collected.diagnostic_count != g04_r1a_measured.diagnostic_count ||
        g04_r1a_collected.prior_owner_suppression_count !=
            g04_r1a_measured.prior_owner_suppression_count ||
        g04_r1a_collected.deferred_flags != g04_r1a_measured.deferred_flags ||
        g04_r1a_collected.noscript_deferred_count !=
            g04_r1a_measured.noscript_deferred_count ||
        g04_r1a_collected.option_branch_deferred_count !=
            g04_r1a_measured.option_branch_deferred_count ||
        g04_r1a_collected.option_branch_resolved_count !=
            g04_r1a_measured.option_branch_resolved_count ||
        g04_r1a_collected.select_text_violation_count !=
            g04_r1a_measured.select_text_violation_count ||
        g04_r1a_collected.g13_custom_deferred_count !=
            g04_r1a_measured.g13_custom_deferred_count) {
        return status_from_errno_value(EIO);
    }
    anchor_index += g04_r1a_collected.diagnostic_count;

    if (g05_r1a_measured.diagnostic_count > ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS - anchor_index) {
        return status_from_errno_value(EIO);
    }
    arbor_view0_native_g05_r1a_evaluation g05_r1a_collected = {0};
    status = arbor_view0_native_g05_r1a_collect_anchors(
        input, authoring_anchors + anchor_index, g05_r1a_measured.diagnostic_count,
        &g05_r1a_collected);
    if (status.native != 0) return status;
    if (g05_r1a_collected.diagnostic_count != g05_r1a_measured.diagnostic_count ||
        g05_r1a_collected.admitted_global_count != g05_r1a_measured.admitted_global_count ||
        g05_r1a_collected.later_g05_owner_count != g05_r1a_measured.later_g05_owner_count ||
        g05_r1a_collected.nonstandard_owner_ignored_count !=
            g05_r1a_measured.nonstandard_owner_ignored_count) {
        return status_from_errno_value(EIO);
    }
    anchor_index += g05_r1a_collected.diagnostic_count;

    if (g05_r2a_measured.diagnostic_count > ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS - anchor_index) {
        return status_from_errno_value(EIO);
    }
    arbor_view0_native_g05_r2a_evaluation g05_r2a_collected = {0};
    status = arbor_view0_native_g05_r2a_collect_anchors(
        input, authoring_anchors + anchor_index, g05_r2a_measured.diagnostic_count,
        &g05_r2a_collected);
    if (status.native != 0) return status;
    if (g05_r2a_collected.diagnostic_count != g05_r2a_measured.diagnostic_count ||
        g05_r2a_collected.admitted_element_attribute_count !=
            g05_r2a_measured.admitted_element_attribute_count ||
        g05_r2a_collected.global_attribute_handoff_count !=
            g05_r2a_measured.global_attribute_handoff_count ||
        g05_r2a_collected.r1_owned_unknown_count != g05_r2a_measured.r1_owned_unknown_count ||
        g05_r2a_collected.r4_body_event_handoff_count !=
            g05_r2a_measured.r4_body_event_handoff_count ||
        g05_r2a_collected.nonstandard_owner_ignored_count !=
            g05_r2a_measured.nonstandard_owner_ignored_count) {
        return status_from_errno_value(EIO);
    }
    anchor_index += g05_r2a_collected.diagnostic_count;

    if (g05_r3a_measured.diagnostic_count > ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS - anchor_index) {
        return status_from_errno_value(EIO);
    }
    arbor_view0_native_g05_r3a_evaluation g05_r3a_collected = {0};
    status = arbor_view0_native_g05_r3a_collect_anchors(
        input, authoring_anchors + anchor_index, g05_r3a_measured.diagnostic_count,
        &g05_r3a_collected);
    if (status.native != 0) return status;
    if (g05_r3a_collected.diagnostic_count != g05_r3a_measured.diagnostic_count ||
        g05_r3a_collected.predicate_evaluation_count !=
            g05_r3a_measured.predicate_evaluation_count ||
        g05_r3a_collected.input_element_count != g05_r3a_measured.input_element_count ||
        g05_r3a_collected.missing_required_count != g05_r3a_measured.missing_required_count ||
        g05_r3a_collected.forbidden_present_count != g05_r3a_measured.forbidden_present_count ||
        g05_r3a_collected.tracked_source_attribute_count !=
            g05_r3a_measured.tracked_source_attribute_count ||
        memcmp(g05_r3a_collected.clause_violation_count,
               g05_r3a_measured.clause_violation_count,
               sizeof(g05_r3a_measured.clause_violation_count)) != 0) {
        return status_from_errno_value(EIO);
    }
    anchor_index += g05_r3a_collected.diagnostic_count;

    if (g05_r4a_measured.diagnostic_count > ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS - anchor_index) {
        return status_from_errno_value(EIO);
    }
    arbor_view0_native_g05_r4a_evaluation g05_r4a_collected = {0};
    status = arbor_view0_native_g05_r4a_collect_anchors(
        input, authoring_anchors + anchor_index, g05_r4a_measured.diagnostic_count,
        &g05_r4a_collected);
    if (status.native != 0) return status;
    if (g05_r4a_collected.diagnostic_count != g05_r4a_measured.diagnostic_count ||
        g05_r4a_collected.matched_body_window_event_count !=
            g05_r4a_measured.matched_body_window_event_count ||
        g05_r4a_collected.admitted_body_count != g05_r4a_measured.admitted_body_count ||
        g05_r4a_collected.misplaced_owner_count != g05_r4a_measured.misplaced_owner_count) {
        return status_from_errno_value(EIO);
    }
    anchor_index += g05_r4a_collected.diagnostic_count;

    arbor_view0_native_g06_evaluation g06_collected = {0};
    status = arbor_view0_native_g06_collect_anchors(
        input, g06_anchors, g06_measured.diagnostic_count, &g06_collected);
    if (status.native != 0) return status;
    if (g06_collected.diagnostic_count != g06_measured.diagnostic_count ||
        g06_collected.matched_consumer_count != g06_measured.matched_consumer_count ||
        g06_collected.prior_owner_suppression_count !=
            g06_measured.prior_owner_suppression_count ||
        g06_collected.time_union_year_admission_count !=
            g06_measured.time_union_year_admission_count ||
        g06_collected.time_union_fallback_count != g06_measured.time_union_fallback_count ||
        memcmp(g06_collected.rule_violation_count,
               g06_measured.rule_violation_count,
               sizeof(g06_measured.rule_violation_count)) != 0) {
        return status_from_errno_value(EIO);
    }

    uint64_t v1n2_anchor_index = 0u;
    const uint64_t g07_anchor_start = v1n2_anchor_index;
    if (g07_measured.diagnostic_count >
        ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS - v1n2_anchor_index) {
        return status_from_errno_value(EIO);
    }
    arbor_view0_native_v1n2_g07_evaluation g07_collected = {0};
    status = arbor_view0_native_v1n2_g07_collect_anchors(
        input,
        (arbor_view0_native_v1n2_g07_anchor *)(void *)(v1n2_anchors + g07_anchor_start),
        g07_measured.diagnostic_count,
        &g07_collected);
    if (status.native != 0) return status;
    if (g07_collected.diagnostic_count != g07_measured.diagnostic_count ||
        g07_collected.hyperlink_element_count != g07_measured.hyperlink_element_count ||
        g07_collected.rel_consumer_count != g07_measured.rel_consumer_count ||
        g07_collected.prior_owner_suppression_count !=
            g07_measured.prior_owner_suppression_count ||
        g07_collected.extension_relation_deferred_count !=
            g07_measured.extension_relation_deferred_count ||
        memcmp(g07_collected.rule_violation_count,
               g07_measured.rule_violation_count,
               sizeof(g07_measured.rule_violation_count)) != 0) {
        return status_from_errno_value(EIO);
    }
    v1n2_anchor_index += g07_collected.diagnostic_count;

    const uint64_t g08_anchor_start = v1n2_anchor_index;
    if (g08_measured.diagnostic_count >
        ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS - v1n2_anchor_index) {
        return status_from_errno_value(EIO);
    }
    arbor_view0_native_v1n2_g08_evaluation g08_collected = {0};
    status = arbor_view0_native_v1n2_g08_collect_anchors(
        input,
        (arbor_view0_native_v1n2_g08_anchor *)(void *)(v1n2_anchors + g08_anchor_start),
        g08_measured.diagnostic_count,
        &g08_collected);
    if (status.native != 0) return status;
    if (g08_collected.diagnostic_count != g08_measured.diagnostic_count ||
        g08_collected.embedded_element_count != g08_measured.embedded_element_count ||
        g08_collected.responsive_source_count != g08_measured.responsive_source_count ||
        g08_collected.media_source_count != g08_measured.media_source_count ||
        g08_collected.text_track_count != g08_measured.text_track_count ||
        g08_collected.image_map_reference_count != g08_measured.image_map_reference_count ||
        g08_collected.foreign_integration_count != g08_measured.foreign_integration_count ||
        g08_collected.prior_owner_suppression_count !=
            g08_measured.prior_owner_suppression_count ||
        g08_collected.deferred_external_semantics_count !=
            g08_measured.deferred_external_semantics_count ||
        memcmp(g08_collected.rule_violation_count,
               g08_measured.rule_violation_count,
               sizeof(g08_measured.rule_violation_count)) != 0) {
        return status_from_errno_value(EIO);
    }
    v1n2_anchor_index += g08_collected.diagnostic_count;

    const uint64_t g09_anchor_start = v1n2_anchor_index;
    if (g09_measured.diagnostic_count >
        ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS - v1n2_anchor_index) {
        return status_from_errno_value(EIO);
    }
    arbor_view0_native_v1n2_g09_evaluation g09_collected = {0};
    status = arbor_view0_native_v1n2_g09_collect_anchors(
        input,
        (arbor_view0_native_v1n2_g09_anchor *)(void *)(v1n2_anchors + g09_anchor_start),
        g09_measured.diagnostic_count,
        &g09_collected);
    if (status.native != 0) return status;
    if (g09_collected.diagnostic_count != g09_measured.diagnostic_count ||
        g09_collected.table_count != g09_measured.table_count ||
        g09_collected.row_count != g09_measured.row_count ||
        g09_collected.cell_count != g09_measured.cell_count ||
        g09_collected.column_group_count != g09_measured.column_group_count ||
        g09_collected.row_group_count != g09_measured.row_group_count ||
        g09_collected.header_token_count != g09_measured.header_token_count ||
        g09_collected.implicit_header_association_count !=
            g09_measured.implicit_header_association_count ||
        g09_collected.prior_owner_suppression_count !=
            g09_measured.prior_owner_suppression_count ||
        g09_collected.deferred_external_semantics_count !=
            g09_measured.deferred_external_semantics_count ||
        memcmp(g09_collected.rule_violation_count,
               g09_measured.rule_violation_count,
               sizeof(g09_measured.rule_violation_count)) != 0) {
        return status_from_errno_value(EIO);
    }
    v1n2_anchor_index += g09_collected.diagnostic_count;

    const uint64_t g10_anchor_start = v1n2_anchor_index;
    if (g10_measured.diagnostic_count >
        ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS - v1n2_anchor_index) {
        return status_from_errno_value(EIO);
    }
    arbor_view0_native_v1n2_g10_evaluation g10_collected = {0};
    status = arbor_view0_native_v1n2_g10_collect_anchors(
        input,
        (arbor_view0_native_v1n2_g10_anchor *)(void *)(v1n2_anchors + g10_anchor_start),
        g10_measured.diagnostic_count,
        &g10_collected);
    if (status.native != 0) return status;
    if (g10_collected.diagnostic_count != g10_measured.diagnostic_count ||
        g10_collected.form_count != g10_measured.form_count ||
        g10_collected.control_count != g10_measured.control_count ||
        g10_collected.input_count != g10_measured.input_count ||
        g10_collected.label_count != g10_measured.label_count ||
        g10_collected.option_count != g10_measured.option_count ||
        g10_collected.idref_token_count != g10_measured.idref_token_count ||
        g10_collected.prior_owner_suppression_count !=
            g10_measured.prior_owner_suppression_count ||
        g10_collected.deferred_external_semantics_count !=
            g10_measured.deferred_external_semantics_count ||
        memcmp(g10_collected.rule_violation_count,
               g10_measured.rule_violation_count,
               sizeof(g10_measured.rule_violation_count)) != 0) {
        return status_from_errno_value(EIO);
    }
    v1n2_anchor_index += g10_collected.diagnostic_count;

    const uint64_t g11_anchor_start = v1n2_anchor_index;
    if (g11_measured.diagnostic_count >
        ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS - v1n2_anchor_index) {
        return status_from_errno_value(EIO);
    }
    arbor_view0_native_v1n2_g11_evaluation g11_collected = {0};
    status = arbor_view0_native_v1n2_g11_collect_anchors(
        input,
        (arbor_view0_native_v1n2_g11_anchor *)(void *)(v1n2_anchors + g11_anchor_start),
        g11_measured.diagnostic_count,
        &g11_collected);
    if (status.native != 0) return status;
    if (g11_collected.diagnostic_count != g11_measured.diagnostic_count ||
        g11_collected.details_count != g11_measured.details_count ||
        g11_collected.dialog_count != g11_measured.dialog_count ||
        g11_collected.name_group_relation_count != g11_measured.name_group_relation_count ||
        g11_collected.prior_owner_suppression_count !=
            g11_measured.prior_owner_suppression_count ||
        g11_collected.deferred_external_semantics_count !=
            g11_measured.deferred_external_semantics_count ||
        memcmp(g11_collected.rule_violation_count,
               g11_measured.rule_violation_count,
               sizeof(g11_measured.rule_violation_count)) != 0) {
        return status_from_errno_value(EIO);
    }
    v1n2_anchor_index += g11_collected.diagnostic_count;

    if (v1n2_anchor_index != g07_measured.diagnostic_count +
            g08_measured.diagnostic_count + g09_measured.diagnostic_count +
            g10_measured.diagnostic_count + g11_measured.diagnostic_count) {
        return status_from_errno_value(EIO);
    }

    const uint64_t expected_authoring_anchor_count =
        authoring_total.value - g02_total.value - g06_measured.diagnostic_count -
        g07_measured.diagnostic_count - g08_measured.diagnostic_count -
        g09_measured.diagnostic_count - g10_measured.diagnostic_count -
        g11_measured.diagnostic_count;
    if (anchor_index != expected_authoring_anchor_count) {
        return status_from_errno_value(EIO);
    }

    /* Last fallible operation: exact parse equivalence is checked before publication. */
    status = arbor_view0_native_lexbor_collect_exact(
        input, diagnostics, parse_total.value, &measured_counts, &measured_facts);
    if (status.native != 0) {
        return status;
    }

    /* From this point to result_out publication there is no error return. */
    uint64_t authoring_index = parse_total.value;
    if (doctype_required_count != 0u) {
        fill_g02_doctype_required_diagnostic(
            diagnostics + authoring_index, authoring_index);
        authoring_index += 1u;
    }
    if (doctype_syntax_count != 0u) {
        fill_g02_doctype_syntax_diagnostic(
            diagnostics + authoring_index, doctype_syntax_issue.byte_offset,
            doctype_syntax_issue.source_length, authoring_index);
        authoring_index += 1u;
    }
    if (doctype_legacy_discouraged_count != 0u) {
        fill_g02_doctype_legacy_discouraged_diagnostic(
            diagnostics + authoring_index, legacy_byte_offset, legacy_source_length,
            authoring_index);
        authoring_index += 1u;
    }
    if (head_title_cardinality_count != 0u) {
        fill_g02_head_title_cardinality_diagnostic(
            diagnostics + authoring_index, title_issue.byte_offset, title_issue.source_length,
            authoring_index);
        authoring_index += 1u;
    }
    if (head_base_cardinality_count != 0u) {
        fill_g02_head_base_cardinality_diagnostic(
            diagnostics + authoring_index, base_issue.byte_offset, base_issue.source_length,
            authoring_index);
        authoring_index += 1u;
    }
    if (body_singleton_count != 0u) {
        fill_g02_body_singleton_diagnostic(
            diagnostics + authoring_index, body_issue.byte_offset, body_issue.source_length,
            authoring_index);
        authoring_index += 1u;
    }

    uint64_t anchor_read = 0u;
    for (uint64_t i = 0u; i < r1a_measured.diagnostic_count; ++i) {
        arbor_view0_native_g03_r1a_materialize_anchor(
            authoring_anchors + anchor_read, authoring_index, diagnostics + authoring_index);
        anchor_read += 1u; authoring_index += 1u;
    }
    for (uint64_t i = 0u; i < r2a_measured.diagnostic_count; ++i) {
        arbor_view0_native_g03_r2a_materialize_anchor(
            authoring_anchors + anchor_read, authoring_index, diagnostics + authoring_index);
        anchor_read += 1u; authoring_index += 1u;
    }
    for (uint64_t i = 0u; i < r3a_measured.diagnostic_count; ++i) {
        arbor_view0_native_g03_r3a_materialize_anchor(
            authoring_anchors + anchor_read, authoring_index, diagnostics + authoring_index);
        anchor_read += 1u; authoring_index += 1u;
    }
    for (uint64_t i = 0u; i < r4a_measured.diagnostic_count; ++i) {
        arbor_view0_native_g03_r4a_materialize_anchor(
            authoring_anchors + anchor_read, authoring_index, diagnostics + authoring_index);
        anchor_read += 1u; authoring_index += 1u;
    }
    for (uint64_t i = 0u; i < r5a_measured.diagnostic_count; ++i) {
        arbor_view0_native_g03_r5a_materialize_anchor(
            authoring_anchors + anchor_read, authoring_index, diagnostics + authoring_index);
        anchor_read += 1u; authoring_index += 1u;
    }
    for (uint64_t i = 0u; i < r7a_measured.diagnostic_count; ++i) {
        arbor_view0_native_g03_r7a_materialize_anchor(
            authoring_anchors + anchor_read, authoring_index, diagnostics + authoring_index);
        anchor_read += 1u; authoring_index += 1u;
    }
    for (uint64_t i = 0u; i < g04_r1a_measured.diagnostic_count; ++i) {
        arbor_view0_native_g04_r1a_materialize_anchor(
            authoring_anchors + anchor_read, authoring_index, diagnostics + authoring_index);
        anchor_read += 1u; authoring_index += 1u;
    }
    for (uint64_t i = 0u; i < g05_r1a_measured.diagnostic_count; ++i) {
        arbor_view0_native_g05_r1a_materialize_anchor(
            authoring_anchors + anchor_read, authoring_index, diagnostics + authoring_index);
        anchor_read += 1u; authoring_index += 1u;
    }
    for (uint64_t i = 0u; i < g05_r2a_measured.diagnostic_count; ++i) {
        arbor_view0_native_g05_r2a_materialize_anchor(
            authoring_anchors + anchor_read, authoring_index, diagnostics + authoring_index);
        anchor_read += 1u; authoring_index += 1u;
    }
    for (uint64_t i = 0u; i < g05_r3a_measured.diagnostic_count; ++i) {
        arbor_view0_native_g05_r3a_materialize_anchor(
            authoring_anchors + anchor_read, authoring_index, diagnostics + authoring_index);
        anchor_read += 1u; authoring_index += 1u;
    }
    for (uint64_t i = 0u; i < g05_r4a_measured.diagnostic_count; ++i) {
        arbor_view0_native_g05_r4a_materialize_anchor(
            authoring_anchors + anchor_read, authoring_index, diagnostics + authoring_index);
        anchor_read += 1u; authoring_index += 1u;
    }
    for (uint64_t i = 0u; i < g06_measured.diagnostic_count; ++i) {
        arbor_view0_native_g06_materialize_anchor(
            g06_anchors + i, authoring_index, diagnostics + authoring_index);
        authoring_index += 1u;
    }
    for (uint64_t i = 0u; i < g07_measured.diagnostic_count; ++i) {
        arbor_view0_native_v1n2_g07_materialize_anchor(
            (const arbor_view0_native_v1n2_g07_anchor *)(const void *)(
                v1n2_anchors + g07_anchor_start + i),
            authoring_index, diagnostics + authoring_index);
        authoring_index += 1u;
    }
    for (uint64_t i = 0u; i < g08_measured.diagnostic_count; ++i) {
        arbor_view0_native_v1n2_g08_materialize_anchor(
            (const arbor_view0_native_v1n2_g08_anchor *)(const void *)(
                v1n2_anchors + g08_anchor_start + i),
            authoring_index, diagnostics + authoring_index);
        authoring_index += 1u;
    }
    for (uint64_t i = 0u; i < g09_measured.diagnostic_count; ++i) {
        arbor_view0_native_v1n2_g09_materialize_anchor(
            (const arbor_view0_native_v1n2_g09_anchor *)(const void *)(
                v1n2_anchors + g09_anchor_start + i),
            authoring_index, diagnostics + authoring_index);
        authoring_index += 1u;
    }
    for (uint64_t i = 0u; i < g10_measured.diagnostic_count; ++i) {
        arbor_view0_native_v1n2_g10_materialize_anchor(
            (const arbor_view0_native_v1n2_g10_anchor *)(const void *)(
                v1n2_anchors + g10_anchor_start + i),
            authoring_index, diagnostics + authoring_index);
        authoring_index += 1u;
    }
    for (uint64_t i = 0u; i < g11_measured.diagnostic_count; ++i) {
        arbor_view0_native_v1n2_g11_materialize_anchor(
            (const arbor_view0_native_v1n2_g11_anchor *)(const void *)(
                v1n2_anchors + g11_anchor_start + i),
            authoring_index, diagnostics + authoring_index);
        authoring_index += 1u;
    }

    if (required_total.value > 1u) {
        qsort(diagnostics,
              (size_t)required_total.value,
              sizeof(*diagnostics),
              diagnostic_compare);
    }
    assign_line_columns(input, diagnostics, required_total.value);

    const arbor_view0_native_result result = {
        .diagnostic_count = required_total.value,
        .tokenizer_error_count = measured_counts.tokenizer_error_count,
        .tree_error_count = measured_counts.tree_error_count,
        .flags = (parse_total.value == 0u
            ? ARBOR_VIEW0_NATIVE_RESULT_FLAG_PARSE_CLEAN
            : 0u) |
            ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R1_PARTIAL |
            ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_PARTIAL |
            ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_PARTIAL |
            ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R4_PARTIAL |
            ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R5_PARTIAL |
            ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R7_PARTIAL |
            ARBOR_VIEW0_NATIVE_RESULT_FLAG_G04_R1_PARTIAL |
            (r1a_measured.deferred_main_form_count != 0u
                ? ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R1_DEFERRED_MAIN_FORM
                : 0u) |
            r2a_measured.deferred_flags |
            (r3a_measured.deferred_flags &
                ~ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_DEFERRED_LABELED_CONTROL) |
            (r4a_measured.deferred_flags &
                ~ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R4_DEFERRED_SELECTEDCONTENT_PROVENANCE) |
            r7a_measured.deferred_flags |
            g04_r1a_measured.deferred_flags
    };
    *result_out = result;

    return ok_status();
}

arbor_status arbor_view0_native_check_fragment_model(
    arbor_span input,
    arbor_view0_native_diagnostic *diagnostics,
    uint64_t diagnostic_capacity,
    arbor_view0_native_result *result_out)
{
    uint64_t diagnostic_bytes = 0u;
    arbor_status status = validate_output_regions(
        input, diagnostics, diagnostic_capacity, result_out, &diagnostic_bytes);
    if (status.native != 0) return status;
    (void)diagnostic_bytes;

    arbor_status utf8 = arbor_view_utf8_validate(input);
    if (utf8.native != 0) {
        if (utf8.native != -(int64_t)EILSEQ) return utf8;
        if (diagnostic_capacity < 1u) return status_from_errno_value(ENOSPC);
        uint64_t bad_offset = 0u;
        if (!utf8_first_invalid_offset(input, &bad_offset) || bad_offset >= input.length)
            return status_from_errno_value(EIO);
        arbor_view0_native_diagnostic candidate = {0};
        candidate.rule_id = ARBOR_VIEW0_NATIVE_RULE_UTF8_INVALID;
        candidate.byte_offset = bad_offset;
        candidate.source_length = 1u;
        candidate.discovery_sequence = 0u;
        candidate.severity = (uint32_t)ARBOR_VIEW0_NATIVE_SEVERITY_ERROR;
        candidate.origin = (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_UTF8;
        if (!copy_literal(candidate.symbolic_name, ARBOR_VIEW0_NATIVE_RULE_SYMBOL_CAP,
                          "html.utf8.invalid") ||
            !copy_literal(candidate.message, ARBOR_VIEW0_NATIVE_MESSAGE_CAP,
                          "HTML input is not well-formed UTF-8"))
            return status_from_errno_value(EOVERFLOW);
        assign_line_columns(input, &candidate, 1u);
        diagnostics[0] = candidate;
        *result_out = (arbor_view0_native_result){
            .diagnostic_count = 1u,
            .tokenizer_error_count = 0u,
            .tree_error_count = 0u,
            .flags = 0u
        };
        return ok_status();
    }

    arbor_view0_native_g04_r2a_evaluation measured = {0};
    status = arbor_view0_native_g04_r2a_measure_fragment_model(input, &measured);
    if (status.native != 0) return status;
    arbor_asm_result_u64 parse_total = u64_add_checked(
        measured.parse_counts.tokenizer_error_count,
        measured.parse_counts.tree_error_count);
    if (parse_total.status != 0) return arbor_status_from_native(parse_total.status);
    arbor_asm_result_u64 required_total = u64_add_checked(
        parse_total.value, measured.diagnostic_count);
    if (required_total.status != 0) return arbor_status_from_native(required_total.status);
    if (required_total.value > ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS ||
        required_total.value > diagnostic_capacity)
        return status_from_errno_value(ENOSPC);

    arbor_view0_native_source_anchor anchors[ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS] = {{0}};
    arbor_view0_native_g04_r2a_evaluation collected = {0};
    status = arbor_view0_native_g04_r2a_collect_fragment_anchors(
        input, anchors, measured.diagnostic_count, &collected);
    if (status.native != 0) return status;
    if (collected.diagnostic_count != measured.diagnostic_count ||
        collected.g13_custom_deferred_count != measured.g13_custom_deferred_count ||
        collected.deferred_flags != measured.deferred_flags ||
        collected.parse_counts.tokenizer_error_count != measured.parse_counts.tokenizer_error_count ||
        collected.parse_counts.tree_error_count != measured.parse_counts.tree_error_count)
        return status_from_errno_value(EIO);

    /* Last fallible operation: exact fragment parse publication. */
    status = arbor_view0_native_lexbor_fragment_collect_exact(
        input, diagnostics, parse_total.value, &measured.parse_counts);
    if (status.native != 0) return status;

    for (uint64_t i = 0u; i < measured.diagnostic_count; ++i) {
        arbor_view0_native_g04_r2a_materialize_anchor(
            &anchors[i], parse_total.value + i, &diagnostics[parse_total.value + i]);
    }
    if (required_total.value > 1u) {
        qsort(diagnostics, (size_t)required_total.value, sizeof(*diagnostics), diagnostic_compare);
    }
    assign_line_columns(input, diagnostics, required_total.value);

    uint64_t flags = measured.deferred_flags;
    if (parse_total.value == 0u) flags |= ARBOR_VIEW0_NATIVE_RESULT_FLAG_PARSE_CLEAN;
    *result_out = (arbor_view0_native_result){
        .diagnostic_count = required_total.value,
        .tokenizer_error_count = measured.parse_counts.tokenizer_error_count,
        .tree_error_count = measured.parse_counts.tree_error_count,
        .flags = flags
    };
    return ok_status();
}
