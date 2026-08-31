#ifndef ARBORCORE_VIEW0_CONFORMANCE_NATIVE_H
#define ARBORCORE_VIEW0_CONFORMANCE_NATIVE_H

#include <stdint.h>

#include <arborcore/view.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ARBOR_VIEW0_NATIVE_MAX_INPUT_BYTES UINT64_C(1048576)
#define ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS UINT64_C(4096)
#define ARBOR_VIEW0_NATIVE_RULE_SYMBOL_CAP UINT64_C(128)
#define ARBOR_VIEW0_NATIVE_MESSAGE_CAP UINT64_C(160)
#define ARBOR_VIEW0_NATIVE_SOURCE_OFFSET_NONE UINT64_MAX

#define ARBOR_VIEW0_NATIVE_RULE_UTF8_INVALID UINT64_C(0x0000000000010001)
#define ARBOR_VIEW0_NATIVE_RULE_TOKENIZER_BASE UINT64_C(0x0000000010000000)
#define ARBOR_VIEW0_NATIVE_RULE_TREE_BASE UINT64_C(0x0000000020000000)
#define ARBOR_VIEW_V1_G02_DOCTYPE_REQUIRED UINT64_C(0x0000000030020001)
#define ARBOR_VIEW_V1_G02_DOCTYPE_SYNTAX UINT64_C(0x0000000030020002)
#define ARBOR_VIEW_V1_G02_DOCTYPE_LEGACY_DISCOURAGED UINT64_C(0x0000000030020003)
#define ARBOR_VIEW_V1_G02_HEAD_TITLE_CARDINALITY UINT64_C(0x0000000030020006)
#define ARBOR_VIEW_V1_G02_HEAD_BASE_CARDINALITY UINT64_C(0x0000000030020007)
#define ARBOR_VIEW_V1_G02_BODY_SINGLETON UINT64_C(0x0000000030020008)
#define ARBOR_VIEW_V1_G03_ELEMENT_CONTEXT UINT64_C(0x0000000030030001)
#define ARBOR_VIEW_V1_G03_CONTENT_MODEL UINT64_C(0x0000000030030002)
#define ARBOR_VIEW_V1_G03_DESCENDANT_EXCLUSIONS UINT64_C(0x0000000030030003)
#define ARBOR_VIEW_V1_G03_NOTHING_MODEL UINT64_C(0x0000000030030004)
#define ARBOR_VIEW_V1_G03_EXPLICIT_HTML_ELEMENT_ALLOWANCE UINT64_C(0x0000000030030005)
#define ARBOR_VIEW_V1_G03_PALPABLE_PHRASING_NONEMPTY UINT64_C(0x0000000030030007)
#define ARBOR_VIEW_V1_G04_TRANSPARENT_PARENT_MODEL UINT64_C(0x0000000030040001)
#define ARBOR_VIEW_V1_G04_TRANSPARENT_PARENTLESS_FLOW UINT64_C(0x0000000030040002)
#define ARBOR_VIEW_V1_G05_GLOBAL_ATTRIBUTE_APPLICABILITY UINT64_C(0x0000000030050001)
#define ARBOR_VIEW_V1_G05_ELEMENT_ATTRIBUTE_APPLICABILITY UINT64_C(0x0000000030050002)
#define ARBOR_VIEW_V1_G05_CONDITIONAL_ATTRIBUTE_APPLICABILITY UINT64_C(0x0000000030050003)
#define ARBOR_VIEW_V1_G05_BODY_WINDOW_EVENT_ATTRIBUTE_APPLICABILITY UINT64_C(0x0000000030050004)
#define ARBOR_VIEW_V1_G06_BOOLEAN_ATTRIBUTE UINT64_C(0x0000000030060001)
#define ARBOR_VIEW_V1_G06_ENUMERATED_ATTRIBUTE UINT64_C(0x0000000030060002)
#define ARBOR_VIEW_V1_G06_SIGNED_INTEGER UINT64_C(0x0000000030060003)
#define ARBOR_VIEW_V1_G06_NON_NEGATIVE_INTEGER UINT64_C(0x0000000030060004)
#define ARBOR_VIEW_V1_G06_FLOATING_POINT UINT64_C(0x0000000030060005)
#define ARBOR_VIEW_V1_G06_MONTH UINT64_C(0x0000000030060006)
#define ARBOR_VIEW_V1_G06_DATE UINT64_C(0x0000000030060007)
#define ARBOR_VIEW_V1_G06_YEARLESS_DATE UINT64_C(0x0000000030060008)
#define ARBOR_VIEW_V1_G06_TIME UINT64_C(0x0000000030060009)
#define ARBOR_VIEW_V1_G06_LOCAL_DATE_TIME UINT64_C(0x000000003006000a)
#define ARBOR_VIEW_V1_G06_TIME_ZONE_OFFSET UINT64_C(0x000000003006000b)
#define ARBOR_VIEW_V1_G06_GLOBAL_DATE_TIME UINT64_C(0x000000003006000c)
#define ARBOR_VIEW_V1_G06_WEEK UINT64_C(0x000000003006000d)
#define ARBOR_VIEW_V1_G06_DURATION UINT64_C(0x000000003006000e)
#define ARBOR_VIEW_V1_G06_DATE_OPTIONAL_TIME UINT64_C(0x000000003006000f)
#define ARBOR_VIEW_V1_G06_SPACE_SEPARATED_TOKENS UINT64_C(0x0000000030060010)
#define ARBOR_VIEW_V1_G06_COMMA_SEPARATED_TOKENS UINT64_C(0x0000000030060011)
#define ARBOR_VIEW_V1_G07_HYPERLINK_ELEMENT_SEMANTICS UINT64_C(0x0000000030070001)
#define ARBOR_VIEW_V1_G07_DOWNLOAD_SEMANTICS UINT64_C(0x0000000030070002)
#define ARBOR_VIEW_V1_G07_HYPERLINK_AUDITING UINT64_C(0x0000000030070003)
#define ARBOR_VIEW_V1_G07_LINK_TYPE_APPLICABILITY UINT64_C(0x0000000030070004)
#define ARBOR_VIEW_V1_G07_LINK_RELATION_SEMANTICS UINT64_C(0x0000000030070005)
#define ARBOR_VIEW_V1_G08_RESPONSIVE_IMAGE_SOURCE_SET UINT64_C(0x0000000030080001)
#define ARBOR_VIEW_V1_G08_IMAGE_RESOURCE_DECLARATION UINT64_C(0x0000000030080002)
#define ARBOR_VIEW_V1_G08_IMAGE_TEXT_ALTERNATIVES UINT64_C(0x0000000030080003)
#define ARBOR_VIEW_V1_G08_IFRAME_AUTHORING UINT64_C(0x0000000030080004)
#define ARBOR_VIEW_V1_G08_EMBED_AUTHORING UINT64_C(0x0000000030080005)
#define ARBOR_VIEW_V1_G08_OBJECT_AUTHORING UINT64_C(0x0000000030080006)
#define ARBOR_VIEW_V1_G08_MEDIA_ELEMENT_DECLARATION UINT64_C(0x0000000030080007)
#define ARBOR_VIEW_V1_G08_TEXT_TRACK_AUTHORING UINT64_C(0x0000000030080008)
#define ARBOR_VIEW_V1_G08_MEDIA_CROSS_RESOURCE_SEMANTICS UINT64_C(0x0000000030080009)
#define ARBOR_VIEW_V1_G08_IMAGE_MAP_AUTHORING UINT64_C(0x000000003008000a)
#define ARBOR_VIEW_V1_G08_FOREIGN_EMBEDDED_CONTENT UINT64_C(0x000000003008000b)
#define ARBOR_VIEW_V1_G08_DIMENSION_ATTRIBUTE_SEMANTICS UINT64_C(0x000000003008000c)

/* Zero tokenizer/tree parse errors; authoring diagnostics may still exist. */
#define ARBOR_VIEW0_NATIVE_RESULT_FLAG_PARSE_CLEAN UINT64_C(0x1)
/* G03 R1A is intentionally partial while the main/form accessible-name branch is deferred. */
#define ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R1_PARTIAL UINT64_C(0x2)
#define ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R1_DEFERRED_MAIN_FORM UINT64_C(0x4)
#define ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_PARTIAL UINT64_C(0x8)
#define ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_DEFERRED_STYLE UINT64_C(0x10)
#define ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_DEFERRED_SCRIPT UINT64_C(0x20)
#define ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_DEFERRED_NOSCRIPT UINT64_C(0x40)
#define ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_DEFERRED_SELECT_SIZE UINT64_C(0x80)
#define ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_DEFERRED_SELECT_PLATFORM UINT64_C(0x100)
#define ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_DEFERRED_UNCLASSIFIED UINT64_C(0x200)
#define ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_PARTIAL UINT64_C(0x400)
#define ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_DEFERRED_INPUT_TYPE UINT64_C(0x800)
#define ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_DEFERRED_LABELED_CONTROL UINT64_C(0x1000)
#define ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_DEFERRED_CANVAS_INPUT_STATE UINT64_C(0x2000)
#define ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_DEFERRED_CANVAS_SELECT_SIZE UINT64_C(0x4000)
#define ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_DEFERRED_NOSCRIPT UINT64_C(0x8000)
#define ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R4_PARTIAL UINT64_C(0x10000)
#define ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R4_DEFERRED_SELECTEDCONTENT_PROVENANCE UINT64_C(0x20000)
#define ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R5_PARTIAL UINT64_C(0x40000)
#define ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R7_PARTIAL UINT64_C(0x80000)
#define ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R7_DEFERRED_G04_TRANSPARENT UINT64_C(0x100000)
#define ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R7_DEFERRED_G13_CUSTOM UINT64_C(0x200000)
#define ARBOR_VIEW0_NATIVE_RESULT_FLAG_G04_R1_PARTIAL UINT64_C(0x400000)
/* Reserved historical R1A/R1B bit. R1C freezes the checker to scripting disabled
 * and no successful R1C path publishes this flag. */
#define ARBOR_VIEW0_NATIVE_RESULT_FLAG_G04_R1_DEFERRED_NOSCRIPT_SCRIPTING UINT64_C(0x800000)
#define ARBOR_VIEW0_NATIVE_RESULT_FLAG_G04_R1_DEFERRED_OPTION_BRANCH UINT64_C(0x1000000)
#define ARBOR_VIEW0_NATIVE_RESULT_FLAG_G04_R1_DEFERRED_G13_CUSTOM UINT64_C(0x2000000)
#define ARBOR_VIEW0_NATIVE_RESULT_FLAG_G04_R2_DEFERRED_G13_CUSTOM UINT64_C(0x4000000)

/* V1N2 G09 uses one invocation-local Lexbor mraw support arena. */
#define ARBOR_VIEW0_NATIVE_V1N2_G09_TRANSIENT_SUPPORT_ARENAS UINT64_C(1)
/* V1N2 G10 uses one invocation-local Lexbor mraw support arena. */
#define ARBOR_VIEW0_NATIVE_V1N2_G10_TRANSIENT_SUPPORT_ARENAS UINT64_C(1)
/* V1N2 G11 uses one invocation-local Lexbor mraw support arena. */
#define ARBOR_VIEW0_NATIVE_V1N2_G11_TRANSIENT_SUPPORT_ARENAS UINT64_C(1)

/* V1N3 uses one invocation-local Lexbor mraw support arena. */
#define ARBOR_VIEW0_NATIVE_V1N3_TRANSIENT_SUPPORT_ARENAS UINT64_C(1)
#define ARBOR_VIEW0_NATIVE_V1N3_OPTIONS_ABI_V1 UINT64_C(0x415256304e334f31)
#define ARBOR_VIEW0_NATIVE_V1N3_MAX_DEFINITIONS UINT64_C(4096)
#define ARBOR_VIEW0_NATIVE_V1N3_MAX_CONSTRUCTOR_BYTES UINT64_C(8388608)
#define ARBOR_VIEW0_NATIVE_V1N3_MAX_ECMA_TOKENS UINT64_C(262144)
#define ARBOR_VIEW0_NATIVE_V1N3_MAX_ECMA_NODES UINT64_C(262144)
#define ARBOR_VIEW0_NATIVE_V1N3_MAX_ECMA_NESTING UINT64_C(4096)
#define ARBOR_VIEW0_NATIVE_V1N3_MAX_ARENA_BYTES UINT64_C(134217728)

#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N2_G10_R1 UINT64_C(0x00000000300a0001)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N2_G10_R2 UINT64_C(0x00000000300a0002)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N2_G10_R3 UINT64_C(0x00000000300a0003)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N2_G10_R4 UINT64_C(0x00000000300a0004)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N2_G10_R5 UINT64_C(0x00000000300a0005)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N2_G10_R6 UINT64_C(0x00000000300a0006)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N2_G10_R7 UINT64_C(0x00000000300a0007)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N2_G10_R8 UINT64_C(0x00000000300a0008)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N2_G10_R9 UINT64_C(0x00000000300a0009)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N2_G10_R10 UINT64_C(0x00000000300a000a)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N2_G10_R11 UINT64_C(0x00000000300a000b)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N2_G10_R12 UINT64_C(0x00000000300a000c)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N2_G10_R13 UINT64_C(0x00000000300a000d)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N2_G11_R1 UINT64_C(0x00000000300b0001)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N2_G11_R2 UINT64_C(0x00000000300b0002)

#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N3_G12_R1 UINT64_C(0x00000000300c0001)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N3_G12_R2 UINT64_C(0x00000000300c0002)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N3_G12_R3 UINT64_C(0x00000000300c0003)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N3_G12_R4 UINT64_C(0x00000000300c0004)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N3_G12_R5 UINT64_C(0x00000000300c0005)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N3_G12_R6 UINT64_C(0x00000000300c0006)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N3_G12_R7 UINT64_C(0x00000000300c0007)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N3_G12_R8 UINT64_C(0x00000000300c0008)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N3_G13_R1 UINT64_C(0x00000000300d0001)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N3_G13_R2 UINT64_C(0x00000000300d0002)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N3_G13_R3 UINT64_C(0x00000000300d0003)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N3_G13_R4 UINT64_C(0x00000000300d0004)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N3_G13_R5 UINT64_C(0x00000000300d0005)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N3_G13_R6 UINT64_C(0x00000000300d0006)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N3_G14_R1 UINT64_C(0x00000000300e0001)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N3_G14_R2 UINT64_C(0x00000000300e0002)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N3_G14_R3 UINT64_C(0x00000000300e0003)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N3_G14_R4 UINT64_C(0x00000000300e0004)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N3_G14_R5 UINT64_C(0x00000000300e0005)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N3_G14_R6 UINT64_C(0x00000000300e0006)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N3_G15_R1 UINT64_C(0x00000000300f0001)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N3_G15_R2 UINT64_C(0x00000000300f0002)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N3_G15_R3 UINT64_C(0x00000000300f0003)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N3_G15_R4 UINT64_C(0x00000000300f0004)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N3_G15_R5 UINT64_C(0x00000000300f0005)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N3_G15_R6 UINT64_C(0x00000000300f0006)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N3_G15_R7 UINT64_C(0x00000000300f0007)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N3_G15_R8 UINT64_C(0x00000000300f0008)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N3_G16_R1 UINT64_C(0x0000000030100001)
#define ARBOR_VIEW0_NATIVE_RULE_ID_V1N3_G16_R2 UINT64_C(0x0000000030100002)

typedef enum arbor_view0_native_severity {
    ARBOR_VIEW0_NATIVE_SEVERITY_ERROR = 1,
    ARBOR_VIEW0_NATIVE_SEVERITY_WARNING = 2
} arbor_view0_native_severity;

typedef enum arbor_view0_native_origin {
    ARBOR_VIEW0_NATIVE_ORIGIN_UTF8 = 1,
    ARBOR_VIEW0_NATIVE_ORIGIN_LEXBOR_TOKENIZER = 2,
    ARBOR_VIEW0_NATIVE_ORIGIN_LEXBOR_TREE = 3,
    ARBOR_VIEW0_NATIVE_ORIGIN_AUTHORING = 4
} arbor_view0_native_origin;

typedef struct arbor_view0_native_diagnostic {
    uint64_t rule_id;
    uint64_t byte_offset;
    uint64_t source_length;
    uint64_t line;
    uint64_t column;
    uint64_t discovery_sequence;
    uint32_t severity;
    uint32_t origin;
    uint32_t external_id;
    uint32_t reserved;
    char symbolic_name[128];
    char message[160];
} arbor_view0_native_diagnostic;

typedef struct arbor_view0_native_result {
    uint64_t diagnostic_count;
    uint64_t tokenizer_error_count;
    uint64_t tree_error_count;
    uint64_t flags;
} arbor_view0_native_result;

typedef enum arbor_view0_native_v1n3_scripting_mode {
    ARBOR_VIEW0_NATIVE_V1N3_SCRIPTING_DISABLED = 0,
    ARBOR_VIEW0_NATIVE_V1N3_SCRIPTING_ENABLED = 1
} arbor_view0_native_v1n3_scripting_mode;

#define ARBOR_VIEW0_NATIVE_V1N3_DEFINITION_AUTONOMOUS UINT64_C(0x1)
#define ARBOR_VIEW0_NATIVE_V1N3_DEFINITION_CUSTOMIZED_BUILTIN UINT64_C(0x2)
#define ARBOR_VIEW0_NATIVE_V1N3_DEFINITION_FORM_ASSOCIATED UINT64_C(0x4)

typedef struct arbor_view0_native_v1n3_definition {
    arbor_span name;
    arbor_span local_name;
    arbor_span constructor_source;
    uint64_t flags;
} arbor_view0_native_v1n3_definition;

typedef struct arbor_view0_native_v1n3_options {
    uint64_t abi;
    uint64_t scripting_mode;
    const arbor_view0_native_v1n3_definition *definitions;
    uint64_t definition_count;
} arbor_view0_native_v1n3_options;

typedef enum arbor_view0_native_v1n3_ecma_operation {
    ARBOR_VIEW0_NATIVE_V1N3_ECMA_CONSTRUCTOR_SUBSET = 1,
    ARBOR_VIEW0_NATIVE_V1N3_ECMA_FUNCTION_BODY = 2,
    ARBOR_VIEW0_NATIVE_V1N3_ECMA_PATTERN_V = 3
} arbor_view0_native_v1n3_ecma_operation;

typedef struct arbor_view0_native_v1n3_ecma_result {
    uint64_t accepted;
    uint64_t error_offset;
    uint64_t token_count;
    uint64_t node_count;
    uint64_t max_nesting;
    uint64_t constructor_super_first;
    uint64_t constructor_return_kind;
    uint64_t reserved;
} arbor_view0_native_v1n3_ecma_result;

/* Private compact staging record used by the global checker before publication. */
typedef struct arbor_view0_native_source_anchor {
    uint32_t byte_offset;
    uint32_t source_length;
} arbor_view0_native_source_anchor;

_Static_assert(sizeof(arbor_view0_native_source_anchor) == 8u,
               "native source-anchor layout drift");

/*
 * Development-tool-only parse/conformance foundation.
 *
 * This function validates M1 UTF-8 first, then uses the private Lexbor v3.0.0
 * adapter to collect tokenizer/tree-builder parse evidence and C0 document facts.
 * The Arborcore-owned rule layer enforces the frozen G02 group and the admitted
 * partial G03 R1A structural ELEMENT_CONTEXT rule, the admitted partial G03
 * R2A CONTENT_MODEL residual evaluator, the admitted partial G03 R3A
 * DESCENDANT_EXCLUSIONS evaluator, the admitted partial G03 R4A
 * NOTHING_MODEL evaluator, the admitted partial G03 R5A
 * EXPLICIT_HTML_ELEMENT_ALLOWANCE evaluator, the R6A SCALAR_VALUE_TEXT retained-owner
 * integration, the admitted partial G03 R7A PALPABLE_PHRASING_NONEMPTY warning
 * evaluator, the admitted G04 R1 transparent-parent evaluator, the four-rule G05
 * applicability partition, and the 17-rule G06 microsyntax consumer wave. G04 consumes
 * authored start-tag/attribute/text provenance from the same pinned Lexbor parse
 * where parser repair would otherwise erase the required parent-model relation.
 * V1N1 G04 R1C explicitly configures that development parser with scripting
 * disabled; the autonomous-custom-element transparency dependency remains G13-
 * owned. Documented deferred branches remain non-rejecting; complete HTML
 * conformance is not claimed. G06 R15 has no accepted author-facing consumer and
 * therefore publishes no diagnostic; its bounded validator remains available to
 * later owning rules. The `time` union admits the normative year-only branch without
 * inventing a new V1N1 rule identity. V1N2 C0 additionally validates the frozen
 * G07-G11 rule/authority/resource metadata foundation before parsing. G07 additionally
 * publishes the five frozen static link-semantics rules. G08 additionally publishes the
 * twelve frozen static embedded-content rules. G09 publishes the six frozen static
 * table-semantics rules, G10 publishes twelve static form rules plus the frozen
 * deterministic constraint-validation subset, and G11 publishes the details name-group
 * authoring rule while retaining G05/G06 ownership of dialog prohibitions and syntax.
 * WebVTT resource bodies, full SVG/MathML
 * language conformance, fetching, media selection/playback, intrinsic-resource dimensions,
 * human-language adequacy, and interactive dialog/details algorithms remain outside
 * this checkpoint.
 *
 * input is borrowed immutable and must remain live through the call. diagnostics
 * and result_out are caller-owned writable objects and must not overlap input or
 * each other. diagnostics may be NULL only when diagnostic_capacity is zero.
 * On validation/mechanism/capacity failure, result_out and diagnostics are not
 * modified. On success, the result reports zero or more document violations.
 */
arbor_status arbor_view0_native_check(
    arbor_span input,
    arbor_view0_native_diagnostic *diagnostics,
    uint64_t diagnostic_capacity,
    arbor_view0_native_result *result_out);

/*
 * V1N3 configured development-tool checker. The definition set is immutable
 * and invocation-local; no registry is created. ECMAScript inputs are parsed
 * only. No evaluation, bytecode, module loading, host binding, event dispatch,
 * regular-expression matching, or DOM mutation occurs.
 */
arbor_status arbor_view0_native_check_configured(
    arbor_span input,
    const arbor_view0_native_v1n3_options *options,
    arbor_view0_native_diagnostic *diagnostics,
    uint64_t diagnostic_capacity,
    arbor_view0_native_result *result_out);

arbor_status arbor_view0_native_v1n3_ecma_parse(
    uint64_t operation,
    arbor_span source,
    void *support_arena,
    arbor_view0_native_v1n3_ecma_result *result_out);

/*
 * Development-tool-only explicit fragment-model checker. This mode does not
 * run whole-document G02/G03 rules. It parses the supplied bytes as an HTML
 * fragment in BODY context with the same pinned Lexbor parser and the same
 * explicitly scripting-disabled mode, then evaluates the frozen G04 R2
 * parentless-transparent fallback. The synthetic Lexbor fragment wrapper is
 * not treated as an authored parent element.
 */
arbor_status arbor_view0_native_check_fragment_model(
    arbor_span input,
    arbor_view0_native_diagnostic *diagnostics,
    uint64_t diagnostic_capacity,
    arbor_view0_native_result *result_out);

/*
 * G03-C0 neutral semantic-observation vocabulary. These are private
 * development-tool values, not production VIEW ABI and not G03 rule IDs.
 * Standard-element IDs are frozen to the F1-R2 113-token inventory order so
 * they do not depend on Lexbor's private tag-number assignment.
 */
#define ARBOR_VIEW0_NATIVE_STANDARD_ELEMENT_COUNT UINT64_C(113)
#define ARBOR_VIEW0_NATIVE_ANCESTOR_WORD_COUNT UINT64_C(2)
#define ARBOR_VIEW0_NATIVE_OBSERVATION_INDEX_NONE UINT64_MAX

typedef enum arbor_view0_native_standard_element_id {
    ARBOR_VIEW0_NATIVE_ELEMENT_NONE = 0,
    ARBOR_VIEW0_NATIVE_ELEMENT_HTML = 1,
    ARBOR_VIEW0_NATIVE_ELEMENT_HEAD = 2,
    ARBOR_VIEW0_NATIVE_ELEMENT_TITLE = 3,
    ARBOR_VIEW0_NATIVE_ELEMENT_BASE = 4,
    ARBOR_VIEW0_NATIVE_ELEMENT_LINK = 5,
    ARBOR_VIEW0_NATIVE_ELEMENT_META = 6,
    ARBOR_VIEW0_NATIVE_ELEMENT_STYLE = 7,
    ARBOR_VIEW0_NATIVE_ELEMENT_BODY = 8,
    ARBOR_VIEW0_NATIVE_ELEMENT_ARTICLE = 9,
    ARBOR_VIEW0_NATIVE_ELEMENT_SECTION = 10,
    ARBOR_VIEW0_NATIVE_ELEMENT_NAV = 11,
    ARBOR_VIEW0_NATIVE_ELEMENT_ASIDE = 12,
    ARBOR_VIEW0_NATIVE_ELEMENT_H1 = 13,
    ARBOR_VIEW0_NATIVE_ELEMENT_H2 = 14,
    ARBOR_VIEW0_NATIVE_ELEMENT_H3 = 15,
    ARBOR_VIEW0_NATIVE_ELEMENT_H4 = 16,
    ARBOR_VIEW0_NATIVE_ELEMENT_H5 = 17,
    ARBOR_VIEW0_NATIVE_ELEMENT_H6 = 18,
    ARBOR_VIEW0_NATIVE_ELEMENT_HGROUP = 19,
    ARBOR_VIEW0_NATIVE_ELEMENT_HEADER = 20,
    ARBOR_VIEW0_NATIVE_ELEMENT_FOOTER = 21,
    ARBOR_VIEW0_NATIVE_ELEMENT_ADDRESS = 22,
    ARBOR_VIEW0_NATIVE_ELEMENT_P = 23,
    ARBOR_VIEW0_NATIVE_ELEMENT_HR = 24,
    ARBOR_VIEW0_NATIVE_ELEMENT_PRE = 25,
    ARBOR_VIEW0_NATIVE_ELEMENT_BLOCKQUOTE = 26,
    ARBOR_VIEW0_NATIVE_ELEMENT_OL = 27,
    ARBOR_VIEW0_NATIVE_ELEMENT_UL = 28,
    ARBOR_VIEW0_NATIVE_ELEMENT_MENU = 29,
    ARBOR_VIEW0_NATIVE_ELEMENT_LI = 30,
    ARBOR_VIEW0_NATIVE_ELEMENT_DL = 31,
    ARBOR_VIEW0_NATIVE_ELEMENT_DT = 32,
    ARBOR_VIEW0_NATIVE_ELEMENT_DD = 33,
    ARBOR_VIEW0_NATIVE_ELEMENT_FIGURE = 34,
    ARBOR_VIEW0_NATIVE_ELEMENT_FIGCAPTION = 35,
    ARBOR_VIEW0_NATIVE_ELEMENT_MAIN = 36,
    ARBOR_VIEW0_NATIVE_ELEMENT_SEARCH = 37,
    ARBOR_VIEW0_NATIVE_ELEMENT_DIV = 38,
    ARBOR_VIEW0_NATIVE_ELEMENT_A = 39,
    ARBOR_VIEW0_NATIVE_ELEMENT_EM = 40,
    ARBOR_VIEW0_NATIVE_ELEMENT_STRONG = 41,
    ARBOR_VIEW0_NATIVE_ELEMENT_SMALL = 42,
    ARBOR_VIEW0_NATIVE_ELEMENT_S = 43,
    ARBOR_VIEW0_NATIVE_ELEMENT_CITE = 44,
    ARBOR_VIEW0_NATIVE_ELEMENT_Q = 45,
    ARBOR_VIEW0_NATIVE_ELEMENT_DFN = 46,
    ARBOR_VIEW0_NATIVE_ELEMENT_ABBR = 47,
    ARBOR_VIEW0_NATIVE_ELEMENT_RUBY = 48,
    ARBOR_VIEW0_NATIVE_ELEMENT_RT = 49,
    ARBOR_VIEW0_NATIVE_ELEMENT_RP = 50,
    ARBOR_VIEW0_NATIVE_ELEMENT_DATA = 51,
    ARBOR_VIEW0_NATIVE_ELEMENT_TIME = 52,
    ARBOR_VIEW0_NATIVE_ELEMENT_CODE = 53,
    ARBOR_VIEW0_NATIVE_ELEMENT_VAR = 54,
    ARBOR_VIEW0_NATIVE_ELEMENT_SAMP = 55,
    ARBOR_VIEW0_NATIVE_ELEMENT_KBD = 56,
    ARBOR_VIEW0_NATIVE_ELEMENT_SUB = 57,
    ARBOR_VIEW0_NATIVE_ELEMENT_SUP = 58,
    ARBOR_VIEW0_NATIVE_ELEMENT_I = 59,
    ARBOR_VIEW0_NATIVE_ELEMENT_B = 60,
    ARBOR_VIEW0_NATIVE_ELEMENT_U = 61,
    ARBOR_VIEW0_NATIVE_ELEMENT_MARK = 62,
    ARBOR_VIEW0_NATIVE_ELEMENT_BDI = 63,
    ARBOR_VIEW0_NATIVE_ELEMENT_BDO = 64,
    ARBOR_VIEW0_NATIVE_ELEMENT_SPAN = 65,
    ARBOR_VIEW0_NATIVE_ELEMENT_BR = 66,
    ARBOR_VIEW0_NATIVE_ELEMENT_WBR = 67,
    ARBOR_VIEW0_NATIVE_ELEMENT_INS = 68,
    ARBOR_VIEW0_NATIVE_ELEMENT_DEL = 69,
    ARBOR_VIEW0_NATIVE_ELEMENT_PICTURE = 70,
    ARBOR_VIEW0_NATIVE_ELEMENT_SOURCE = 71,
    ARBOR_VIEW0_NATIVE_ELEMENT_IMG = 72,
    ARBOR_VIEW0_NATIVE_ELEMENT_IFRAME = 73,
    ARBOR_VIEW0_NATIVE_ELEMENT_EMBED = 74,
    ARBOR_VIEW0_NATIVE_ELEMENT_OBJECT = 75,
    ARBOR_VIEW0_NATIVE_ELEMENT_VIDEO = 76,
    ARBOR_VIEW0_NATIVE_ELEMENT_AUDIO = 77,
    ARBOR_VIEW0_NATIVE_ELEMENT_TRACK = 78,
    ARBOR_VIEW0_NATIVE_ELEMENT_MAP = 79,
    ARBOR_VIEW0_NATIVE_ELEMENT_AREA = 80,
    ARBOR_VIEW0_NATIVE_ELEMENT_TABLE = 81,
    ARBOR_VIEW0_NATIVE_ELEMENT_CAPTION = 82,
    ARBOR_VIEW0_NATIVE_ELEMENT_COLGROUP = 83,
    ARBOR_VIEW0_NATIVE_ELEMENT_COL = 84,
    ARBOR_VIEW0_NATIVE_ELEMENT_TBODY = 85,
    ARBOR_VIEW0_NATIVE_ELEMENT_THEAD = 86,
    ARBOR_VIEW0_NATIVE_ELEMENT_TFOOT = 87,
    ARBOR_VIEW0_NATIVE_ELEMENT_TR = 88,
    ARBOR_VIEW0_NATIVE_ELEMENT_TD = 89,
    ARBOR_VIEW0_NATIVE_ELEMENT_TH = 90,
    ARBOR_VIEW0_NATIVE_ELEMENT_FORM = 91,
    ARBOR_VIEW0_NATIVE_ELEMENT_LABEL = 92,
    ARBOR_VIEW0_NATIVE_ELEMENT_INPUT = 93,
    ARBOR_VIEW0_NATIVE_ELEMENT_BUTTON = 94,
    ARBOR_VIEW0_NATIVE_ELEMENT_SELECT = 95,
    ARBOR_VIEW0_NATIVE_ELEMENT_DATALIST = 96,
    ARBOR_VIEW0_NATIVE_ELEMENT_OPTGROUP = 97,
    ARBOR_VIEW0_NATIVE_ELEMENT_OPTION = 98,
    ARBOR_VIEW0_NATIVE_ELEMENT_TEXTAREA = 99,
    ARBOR_VIEW0_NATIVE_ELEMENT_OUTPUT = 100,
    ARBOR_VIEW0_NATIVE_ELEMENT_PROGRESS = 101,
    ARBOR_VIEW0_NATIVE_ELEMENT_METER = 102,
    ARBOR_VIEW0_NATIVE_ELEMENT_FIELDSET = 103,
    ARBOR_VIEW0_NATIVE_ELEMENT_LEGEND = 104,
    ARBOR_VIEW0_NATIVE_ELEMENT_SELECTEDCONTENT = 105,
    ARBOR_VIEW0_NATIVE_ELEMENT_DETAILS = 106,
    ARBOR_VIEW0_NATIVE_ELEMENT_SUMMARY = 107,
    ARBOR_VIEW0_NATIVE_ELEMENT_DIALOG = 108,
    ARBOR_VIEW0_NATIVE_ELEMENT_SCRIPT = 109,
    ARBOR_VIEW0_NATIVE_ELEMENT_NOSCRIPT = 110,
    ARBOR_VIEW0_NATIVE_ELEMENT_TEMPLATE = 111,
    ARBOR_VIEW0_NATIVE_ELEMENT_SLOT = 112,
    ARBOR_VIEW0_NATIVE_ELEMENT_CANVAS = 113,
    ARBOR_VIEW0_NATIVE_ELEMENT__COUNT = 114
} arbor_view0_native_standard_element_id;

/* C0-SR1 private development-time HTML tree-builder insertion-mode vocabulary. */
typedef enum arbor_view0_native_insertion_mode_id {
    ARBOR_VIEW0_NATIVE_INSERTION_MODE_NONE = 0,
    ARBOR_VIEW0_NATIVE_INSERTION_MODE_INITIAL = 1,
    ARBOR_VIEW0_NATIVE_INSERTION_MODE_BEFORE_HTML = 2,
    ARBOR_VIEW0_NATIVE_INSERTION_MODE_BEFORE_HEAD = 3,
    ARBOR_VIEW0_NATIVE_INSERTION_MODE_IN_HEAD = 4,
    ARBOR_VIEW0_NATIVE_INSERTION_MODE_IN_HEAD_NOSCRIPT = 5,
    ARBOR_VIEW0_NATIVE_INSERTION_MODE_AFTER_HEAD = 6,
    ARBOR_VIEW0_NATIVE_INSERTION_MODE_IN_BODY = 7,
    ARBOR_VIEW0_NATIVE_INSERTION_MODE_IN_BODY_SKIP_NEW_LINE = 8,
    ARBOR_VIEW0_NATIVE_INSERTION_MODE_IN_BODY_SKIP_NEW_LINE_TEXTAREA = 9,
    ARBOR_VIEW0_NATIVE_INSERTION_MODE_TEXT = 10,
    ARBOR_VIEW0_NATIVE_INSERTION_MODE_IN_TABLE = 11,
    ARBOR_VIEW0_NATIVE_INSERTION_MODE_IN_TABLE_TEXT = 12,
    ARBOR_VIEW0_NATIVE_INSERTION_MODE_IN_CAPTION = 13,
    ARBOR_VIEW0_NATIVE_INSERTION_MODE_IN_COLUMN_GROUP = 14,
    ARBOR_VIEW0_NATIVE_INSERTION_MODE_IN_TABLE_BODY = 15,
    ARBOR_VIEW0_NATIVE_INSERTION_MODE_IN_ROW = 16,
    ARBOR_VIEW0_NATIVE_INSERTION_MODE_IN_CELL = 17,
    ARBOR_VIEW0_NATIVE_INSERTION_MODE_IN_TEMPLATE = 18,
    ARBOR_VIEW0_NATIVE_INSERTION_MODE_AFTER_BODY = 19,
    ARBOR_VIEW0_NATIVE_INSERTION_MODE_IN_FRAMESET = 20,
    ARBOR_VIEW0_NATIVE_INSERTION_MODE_AFTER_FRAMESET = 21,
    ARBOR_VIEW0_NATIVE_INSERTION_MODE_AFTER_AFTER_BODY = 22,
    ARBOR_VIEW0_NATIVE_INSERTION_MODE_AFTER_AFTER_FRAMESET = 23
} arbor_view0_native_insertion_mode_id;

#define ARBOR_VIEW0_NATIVE_SOURCE_REPAIR_FLAG_FOSTER_PARENTING UINT64_C(0x1)

typedef struct arbor_view0_native_source_repair_context {
    uint64_t standard_element_id;
    uint64_t source_offset;
    uint64_t source_length;
    uint64_t initial_current_standard_element_id;
    uint64_t initial_current_source_offset;
    uint64_t initial_insertion_mode_id;
    uint64_t initial_open_elements_depth;
    uint64_t insertion_seen;
    uint64_t insertion_current_standard_element_id;
    uint64_t insertion_current_source_offset;
    uint64_t insertion_mode_id;
    uint64_t insertion_open_elements_depth;
    uint64_t insertion_flags;
} arbor_view0_native_source_repair_context;

typedef arbor_status (*arbor_view0_native_source_repair_observer_f)(
    void *context,
    const arbor_view0_native_source_repair_context *observation);

/*
 * Private authored-token observations used only where DOM repair loses the
 * authored relation needed by a frozen rule. local_name/value/text are
 * callback-lifetime borrowed spans owned by the pinned Lexbor parse.
 */
typedef struct arbor_view0_native_source_attribute_observation {
    uint64_t owner_standard_element_id;
    uint64_t owner_source_offset;
    uint64_t source_offset;
    uint64_t source_length;
    uint64_t ordinal;
    arbor_span local_name;
    arbor_span value;
} arbor_view0_native_source_attribute_observation;

typedef struct arbor_view0_native_source_text_observation {
    uint64_t source_offset;
    uint64_t source_length;
    uint64_t initial_current_standard_element_id;
    uint64_t initial_current_source_offset;
    uint64_t initial_insertion_mode_id;
    uint64_t initial_open_elements_depth;
    arbor_span text;
} arbor_view0_native_source_text_observation;

typedef arbor_status (*arbor_view0_native_source_attribute_observer_f)(
    void *context,
    const arbor_view0_native_source_attribute_observation *observation);

typedef arbor_status (*arbor_view0_native_source_text_observer_f)(
    void *context,
    const arbor_view0_native_source_text_observation *observation);

typedef enum arbor_view0_native_namespace_id {
    ARBOR_VIEW0_NATIVE_NAMESPACE_NONE = 0,
    ARBOR_VIEW0_NATIVE_NAMESPACE_HTML = 1,
    ARBOR_VIEW0_NATIVE_NAMESPACE_SVG = 2,
    ARBOR_VIEW0_NATIVE_NAMESPACE_MATHML = 3,
    ARBOR_VIEW0_NATIVE_NAMESPACE_OTHER = 4
} arbor_view0_native_namespace_id;

typedef enum arbor_view0_native_direct_child_kind {
    ARBOR_VIEW0_NATIVE_DIRECT_CHILD_ELEMENT = 1,
    ARBOR_VIEW0_NATIVE_DIRECT_CHILD_TEXT = 2
} arbor_view0_native_direct_child_kind;

#define ARBOR_VIEW0_NATIVE_ELEMENT_FLAG_SYNTHETIC UINT64_C(0x1)
#define ARBOR_VIEW0_NATIVE_CHILD_FLAG_SYNTHETIC UINT64_C(0x1)
#define ARBOR_VIEW0_NATIVE_CHILD_FLAG_FIRST_ELEMENT UINT64_C(0x2)
#define ARBOR_VIEW0_NATIVE_CHILD_FLAG_LAST_ELEMENT UINT64_C(0x4)
#define ARBOR_VIEW0_NATIVE_CHILD_FLAG_TEXT_INTER_ELEMENT_WHITESPACE UINT64_C(0x8)

typedef struct arbor_view0_native_element_observation {
    uint64_t standard_element_id;
    uint64_t namespace_id;
    uint64_t source_offset;
    uint64_t source_length;
    uint64_t parent_standard_element_id;
    uint64_t grandparent_standard_element_id;
    uint64_t depth;
    uint64_t attribute_count;
    uint64_t direct_child_count;
    uint64_t element_child_count;
    uint64_t flags;
    uint64_t ancestor_bits[2];
    arbor_span local_name;
} arbor_view0_native_element_observation;

typedef struct arbor_view0_native_attribute_observation {
    uint64_t owner_standard_element_id;
    uint64_t namespace_id;
    uint64_t ordinal;
    arbor_span local_name;
    arbor_span value;
} arbor_view0_native_attribute_observation;

typedef struct arbor_view0_native_direct_child_observation {
    uint64_t parent_standard_element_id;
    uint64_t kind;
    uint64_t sequence_index;
    uint64_t element_index;
    uint64_t element_count;
    uint64_t standard_element_id;
    uint64_t namespace_id;
    uint64_t source_offset;
    uint64_t source_length;
    uint64_t flags;
    arbor_span local_name;
    arbor_span text;
} arbor_view0_native_direct_child_observation;

typedef struct arbor_view0_native_observation_counts {
    uint64_t element_count;
    uint64_t authored_element_count;
    uint64_t synthetic_element_count;
    uint64_t attribute_count;
    uint64_t direct_child_count;
    uint64_t max_depth;
} arbor_view0_native_observation_counts;

typedef arbor_status (*arbor_view0_native_element_observer_f)(
    void *context,
    const arbor_view0_native_element_observation *observation);

typedef arbor_status (*arbor_view0_native_attribute_observer_f)(
    void *context,
    const arbor_view0_native_attribute_observation *observation);

typedef arbor_status (*arbor_view0_native_direct_child_observer_f)(
    void *context,
    const arbor_view0_native_direct_child_observation *observation);

typedef struct arbor_view0_native_semantic_observer {
    void *context;
    arbor_view0_native_element_observer_f element_begin;
    arbor_view0_native_attribute_observer_f attribute;
    arbor_view0_native_direct_child_observer_f direct_child;
    arbor_view0_native_element_observer_f element_complete;
    arbor_view0_native_element_observer_f traversal_enter;
    arbor_view0_native_element_observer_f traversal_leave;
    arbor_view0_native_source_repair_observer_f source_repair;
    arbor_view0_native_source_attribute_observer_f source_attribute;
    arbor_view0_native_source_text_observer_f source_text;
} arbor_view0_native_semantic_observer;

/* Private adapter boundary; no Lexbor type crosses this header. */
typedef struct arbor_view0_native_parse_counts {
    uint64_t tokenizer_error_count;
    uint64_t tree_error_count;
} arbor_view0_native_parse_counts;

/*
 * V1N1 C0 facts are caller-local value data copied/derived while the Lexbor
 * document is live. No field is a pointer and no Lexbor object survives the
 * adapter call. SOURCE_OFFSET_NONE marks an unavailable first/second source
 * anchor. These facts are substrate only: they do not themselves decide G02.
 */
typedef struct arbor_view0_native_document_facts {
    uint64_t source_doctype_count;
    uint64_t source_first_doctype_keyword_offset;
    uint64_t source_first_doctype_keyword_length;
    uint64_t source_first_doctype_name_offset;
    uint64_t source_first_doctype_name_length;
    uint64_t source_first_doctype_external_keyword_offset;
    uint64_t source_first_doctype_external_keyword_length;
    uint64_t source_first_doctype_public_id_offset;
    uint64_t source_first_doctype_public_id_length;
    uint64_t source_first_doctype_system_id_offset;
    uint64_t source_first_doctype_system_id_length;
    uint64_t source_title_start_tag_count;
    uint64_t source_second_title_start_tag_offset;
    uint64_t source_base_start_tag_count;
    uint64_t source_second_base_start_tag_offset;
    uint64_t source_body_start_tag_count;
    uint64_t source_second_body_start_tag_offset;
    uint64_t dom_doctype_node_count;
    uint64_t dom_html_document_element_count;
    uint64_t dom_html_head_element_count;
    uint64_t dom_html_body_element_count;
    uint64_t dom_head_title_child_count;
    uint64_t dom_head_base_child_count;
} arbor_view0_native_document_facts;


/*
 * Private measurement pass used by the Arborcore-owned authoring-rule layer to
 * learn parse-error cardinality and C0 document facts before any diagnostic
 * bytes are published. This preserves whole-check failure atomicity when parser
 * diagnostics and authoring diagnostics share one caller-owned bounded array.
 * No Lexbor type or object survives the call.
 */
arbor_status arbor_view0_native_lexbor_measure(
    arbor_span input,
    arbor_view0_native_parse_counts *counts_out,
    arbor_view0_native_document_facts *facts_out);

/*
 * Private synchronous adapter. input is borrowed immutable; diagnostics,
 * counts_out and facts_out are caller-owned writable regions and must be
 * mutually disjoint and disjoint from input. Publication is failure-atomic.
 */
arbor_status arbor_view0_native_lexbor_collect(
    arbor_span input,
    arbor_view0_native_diagnostic *diagnostics,
    uint64_t diagnostic_capacity,
    arbor_view0_native_parse_counts *counts_out,
    arbor_view0_native_document_facts *facts_out);

/*
 * Private global-transaction final parser pass. The parse counts and document
 * facts must match the earlier measurement exactly before any diagnostic byte
 * is published. On mismatch or mechanism failure diagnostics remain unchanged.
 */
arbor_status arbor_view0_native_lexbor_collect_exact(
    arbor_span input,
    arbor_view0_native_diagnostic *diagnostics,
    uint64_t diagnostic_capacity,
    const arbor_view0_native_parse_counts *expected_counts,
    const arbor_view0_native_document_facts *expected_facts);

/*
 * G03-C0 synchronous neutral observation pass. The observer receives borrowed
 * callback-lifetime views only; it must not retain any pointer after a callback
 * returns. Lexbor objects/types remain private to the adapter. Template
 * contents are intentionally not traversed: the pinned HTML source gives those
 * DocumentFragment contents no conformance requirements.
 *
 * The optional traversal_enter/traversal_leave callbacks expose true iterative
 * DFS lifetime around the existing per-element callback sequence without
 * changing element_begin/element_complete semantics. traversal_enter runs
 * immediately before element_begin. On normal traversal, traversal_leave runs
 * after all descendant element traversal. If any callback fails, later leave
 * callbacks are not guaranteed.
 *
 * This function requires the qualified static-link provenance wrapper to be
 * active. On adapter/mechanism/observer failure, parse_counts_out, facts_out
 * and observation_counts_out remain unchanged. The consumer-owned observer
 * context is not transactional and may have changed before a callback failure.
 */
arbor_status arbor_view0_native_lexbor_observe(
    arbor_span input,
    const arbor_view0_native_semantic_observer *observer,
    arbor_view0_native_parse_counts *parse_counts_out,
    arbor_view0_native_document_facts *facts_out,
    arbor_view0_native_observation_counts *observation_counts_out);

/* Private explicit-fragment counterparts used only by G04 R2. */
arbor_status arbor_view0_native_lexbor_observe_fragment_model(
    arbor_span input,
    const arbor_view0_native_semantic_observer *observer,
    arbor_view0_native_parse_counts *parse_counts_out,
    arbor_view0_native_observation_counts *observation_counts_out);

arbor_status arbor_view0_native_lexbor_fragment_collect_exact(
    arbor_span input,
    arbor_view0_native_diagnostic *diagnostics,
    uint64_t diagnostic_capacity,
    const arbor_view0_native_parse_counts *expected_counts);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ARBORCORE_VIEW0_CONFORMANCE_NATIVE_H */
