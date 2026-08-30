#include <arborcore/view0_conformance/native.h>
#include "g05_c0.h"
#include "g05_r3a.h"
#include "g06.h"
#include "g03_r3a.h"
#include "g10.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct state_case { const char *keyword; uint64_t state; } state_case;
typedef struct fixture_case { const char *id; const char *body; size_t rule; bool present; } fixture_case;

static void need(bool condition, const char *message) {
    if (!condition) { (void)fprintf(stderr, "FAIL: %s\n", message); exit(1); }
}
static arbor_span cspan(const char *value) {
    return (arbor_span){(const uint8_t *)value, (uint64_t)strlen(value)};
}
static arbor_view0_native_v1n2_g10_evaluation evaluate_body(const char *body) {
    static const char prefix[] = "<!doctype html><html><head><title>x</title></head><body>";
    static const char suffix[] = "</body></html>";
    char input[65536];
    const int length = snprintf(input, sizeof(input), "%s%s%s", prefix, body, suffix);
    arbor_view0_native_v1n2_g10_evaluation result = {0};
    need(length > 0 && (size_t)length < sizeof(input), "fixture construction");
    need(arbor_view0_native_v1n2_g10_measure((arbor_span){(const uint8_t *)input,
        (uint64_t)length}, &result).native == 0, "G10 measurement");
    return result;
}
static void check_fixture(const fixture_case *test) {
    const arbor_view0_native_v1n2_g10_evaluation result = evaluate_body(test->body);
    need(test->rule >= 1u && test->rule <= 13u, "fixture rule ordinal");
    const bool observed = result.rule_violation_count[test->rule - 1u] != 0u;
    if (observed != test->present) {
        (void)fprintf(stderr, "FAIL: %s expected G10-R%zu=%u observed=%u diagnostics=%llu\n",
            test->id, test->rule, test->present ? 1u : 0u, observed ? 1u : 0u,
            (unsigned long long)result.diagnostic_count);
        exit(1);
    }
}
static uint64_t count_rule_id(
    const arbor_view0_native_diagnostic *diagnostics,
    uint64_t diagnostic_count, uint64_t rule_id) {
    uint64_t count = 0u;
    for (uint64_t index = 0u; index < diagnostic_count; ++index)
        if (diagnostics[index].rule_id == rule_id) count += 1u;
    return count;
}
static void check_input_states(void) {
    static const state_case states[] = {
        {"hidden", ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_HIDDEN},
        {"text", ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TEXT},
        {"search", ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_SEARCH},
        {"tel", ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TEL},
        {"url", ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_URL},
        {"email", ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_EMAIL},
        {"password", ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_PASSWORD},
        {"date", ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_DATE},
        {"month", ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_MONTH},
        {"week", ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_WEEK},
        {"time", ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TIME},
        {"datetime-local", ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_DATETIME_LOCAL},
        {"number", ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_NUMBER},
        {"range", ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_RANGE},
        {"color", ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_COLOR},
        {"checkbox", ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_CHECKBOX},
        {"radio", ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_RADIO},
        {"file", ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_FILE},
        {"submit", ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_SUBMIT},
        {"image", ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_IMAGE},
        {"reset", ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_RESET},
        {"button", ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_BUTTON}
    };
    for (size_t index = 0u; index < sizeof(states) / sizeof(states[0]); ++index)
        need(arbor_view0_native_g05_c0_input_state_from_type(cspan(states[index].keyword)) ==
             states[index].state, "T001-T022 exact input-state classification");
    need(arbor_view0_native_g05_c0_input_state_from_type(cspan("")) ==
         ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TEXT, "T023 empty type normalization");
    need(arbor_view0_native_g05_c0_input_state_from_type(cspan("unknown-state")) ==
         ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TEXT, "T024 unknown type normalization");
    need(arbor_view0_native_g05_c0_input_state_from_type(cspan("TEXT")) ==
         ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TEXT, "T025 ASCII-case normalization");
    need(arbor_view0_native_g05_c0_input_state_from_type(cspan(" text ")) ==
         ARBOR_VIEW0_NATIVE_G05_C0_INPUT_STATE_TEXT, "T026 invalid spaced type default");
}

static void check_sr2_step_case(
    const char *id, const char *body, bool expect_mismatch) {
    static const char prefix[] = "<!doctype html><html><head><title>x</title></head><body>";
    static const char suffix[] = "</body></html>";
    char input[65536];
    const int length = snprintf(input, sizeof(input), "%s%s%s", prefix, body, suffix);
    need(length > 0 && (size_t)length < sizeof(input), "SR2 fixture construction");
    const bool exact_identity_case = strcmp(id, "S201") == 0;
    const char *step = strstr(input, "step");
    need(!exact_identity_case || step != NULL, "S201 step source token");
    const uint64_t step_offset = step == NULL
        ? UINT64_C(0) : (uint64_t)(step - input);
    const arbor_span span = {(const uint8_t *)input, (uint64_t)length};
    arbor_view0_native_v1n2_g10_evaluation measured = {0};
    need(arbor_view0_native_v1n2_g10_measure(span, &measured).native == 0,
         "SR2 step measurement");
    if (!expect_mismatch) {
        if (measured.diagnostic_count != 0u) {
            (void)fprintf(stderr, "FAIL: %s expected no G10 diagnostic, observed=%llu\n",
                id, (unsigned long long)measured.diagnostic_count);
            exit(1);
        }
        return;
    }
    need(measured.diagnostic_count == UINT64_C(1),
         "S201 exact total diagnostic count");
    for (size_t rule = 0u; rule < 13u; ++rule)
        need(measured.rule_violation_count[rule] == (rule == 11u ? UINT64_C(1) : 0u),
             "S201 exact G10 rule counts");
    if (!exact_identity_case) return;
    arbor_view0_native_v1n2_g10_anchor anchor = {0};
    arbor_view0_native_v1n2_g10_evaluation collected = {0};
    need(arbor_view0_native_v1n2_g10_collect_anchors(
        span, &anchor, UINT64_C(1), &collected).native == 0,
        "S201 exact anchor collection");
    need(memcmp(&measured, &collected, sizeof(measured)) == 0,
         "S201 measure/collect identity");
    need(anchor.shared.group_ordinal == UINT16_C(10) &&
         anchor.shared.rule_ordinal == UINT16_C(12) &&
         anchor.shared.kind == (uint16_t)ARBOR_VIEW0_NATIVE_V1N2_ANCHOR_ATTRIBUTE_NAME &&
         anchor.shared.byte_offset == step_offset &&
         anchor.shared.source_length == UINT64_C(4),
         "S201 exact step-attribute anchor identity");
    arbor_view0_native_diagnostic diagnostic = {0};
    arbor_view0_native_v1n2_g10_materialize_anchor(&anchor, UINT64_C(0), &diagnostic);
    need(strcmp(diagnostic.symbolic_name,
        "ARBOR_VIEW_V1_G10_CONSTRAINT_VALIDATION_SEMANTICS") == 0 &&
        diagnostic.byte_offset == anchor.shared.byte_offset &&
        diagnostic.source_length == anchor.shared.source_length &&
        diagnostic.discovery_sequence == UINT64_C(0),
        "S201 exact materialized diagnostic identity and order");
}

static void check_sr2_exact_arithmetic(void) {
    check_sr2_step_case("S201", "<input type=number value=3 min=0 step=2 name=x>", true);
    check_sr2_step_case("S202", "<input type=number value=4 min=0 step=2 name=x>", false);
    check_sr2_step_case("S203", "<input type=number value=0.3 min=0 step=0.1 name=x>", false);
    check_sr2_step_case("S204", "<input type=number value=0.31 min=0 step=0.1 name=x>", true);
    check_sr2_step_case("S205", "<input type=number value=3e0 min=0 step=2e0 name=x>", true);
    check_sr2_step_case("S206", "<input type=number value=3 min=0 step=any name=x>", false);
    check_sr2_step_case("S207", "<input type=date value=2026-12-02 min=2026-12-01 step=2 name=x>", true);
    check_sr2_step_case("S208", "<input type=month value=2026-02 min=2026-01 step=2 name=x>", true);
    check_sr2_step_case("S209", "<input type=week value=2026-W03 min=2026-W01 step=3 name=x>", true);
    check_sr2_step_case("S210", "<input type=time value=00:00:30 min=00:00:00 name=x>", true);
    check_sr2_step_case("S211", "<input type=datetime-local value=2026-01-01T00:00:30 min=2026-01-01T00:00:00 name=x>", true);
    check_sr2_step_case("S212", "<input type=number value=3 min=x step=2 name=x>", false);
    check_sr2_step_case("S213", "<input type=number value=3 step=2 name=x>", false);
}

static void check_sr3_select_case(
    const char *id, const char *body, uint64_t expected_r5,
    uint64_t expected_r12, uint64_t expected_total, bool exact_identity) {
    static const char prefix[] = "<!doctype html><html><head><title>x</title></head><body>";
    static const char suffix[] = "</body></html>";
    char input[65536];
    const int length = snprintf(input, sizeof(input), "%s%s%s", prefix, body, suffix);
    need(length > 0 && (size_t)length < sizeof(input), "SR3 fixture construction");
    const arbor_span span = {(const uint8_t *)input, (uint64_t)length};
    arbor_view0_native_v1n2_g10_evaluation measured = {0};
    need(arbor_view0_native_v1n2_g10_measure(span, &measured).native == 0,
         "SR3 select measurement");
    if (measured.rule_violation_count[4] != expected_r5 ||
        measured.rule_violation_count[11] != expected_r12 ||
        measured.diagnostic_count != expected_total) {
        (void)fprintf(stderr,
            "FAIL: %s expected R5=%llu R12=%llu total=%llu; observed=%llu,%llu,%llu\n",
            id, (unsigned long long)expected_r5, (unsigned long long)expected_r12,
            (unsigned long long)expected_total,
            (unsigned long long)measured.rule_violation_count[4],
            (unsigned long long)measured.rule_violation_count[11],
            (unsigned long long)measured.diagnostic_count);
        exit(1);
    }
    for (size_t rule = 0u; rule < 13u; ++rule) {
        const uint64_t expected = rule == 4u ? expected_r5 :
            (rule == 11u ? expected_r12 : UINT64_C(0));
        need(measured.rule_violation_count[rule] == expected,
             "SR3 exact per-rule diagnostic counts");
    }
    if (!exact_identity) return;
    const char *required = strstr(input, "required");
    need(required != NULL, "S301 required source token");
    arbor_view0_native_v1n2_g10_anchor anchor = {0};
    arbor_view0_native_v1n2_g10_evaluation collected = {0};
    need(arbor_view0_native_v1n2_g10_collect_anchors(
        span, &anchor, UINT64_C(1), &collected).native == 0,
        "S301 exact anchor collection");
    need(memcmp(&measured, &collected, sizeof(measured)) == 0,
         "S301 measure/collect identity");
    need(anchor.shared.group_ordinal == UINT16_C(10) &&
         anchor.shared.rule_ordinal == UINT16_C(12) &&
         anchor.shared.kind == (uint16_t)ARBOR_VIEW0_NATIVE_V1N2_ANCHOR_ATTRIBUTE_NAME &&
         anchor.shared.byte_offset == (uint64_t)(required - input) &&
         anchor.shared.source_length == UINT64_C(8),
         "S301 exact required-attribute anchor identity");
    arbor_view0_native_diagnostic diagnostic = {0};
    arbor_view0_native_v1n2_g10_materialize_anchor(&anchor, UINT64_C(0), &diagnostic);
    need(strcmp(diagnostic.symbolic_name,
        "ARBOR_VIEW_V1_G10_CONSTRAINT_VALIDATION_SEMANTICS") == 0 &&
        diagnostic.byte_offset == anchor.shared.byte_offset &&
        diagnostic.source_length == anchor.shared.source_length &&
        diagnostic.discovery_sequence == UINT64_C(0),
        "S301 exact materialized diagnostic identity and order");
}

static void check_sr3_select_placeholder_semantics(void) {
    check_sr3_select_case("S301",
        "<select required name=x><option value='' selected></option></select>",
        0u, 1u, 1u, true);
    check_sr3_select_case("S302",
        "<select required size=1 name=x><option value='' selected></option></select>",
        0u, 1u, 1u, false);
    check_sr3_select_case("S303",
        "<select required size=2 name=x><option value='' selected></option></select>",
        0u, 0u, 0u, false);
    check_sr3_select_case("S304",
        "<select required multiple name=x><option value='' selected></option></select>",
        0u, 0u, 0u, false);
    check_sr3_select_case("S305",
        "<select required multiple size=1 name=x><option value='' selected></option></select>",
        0u, 1u, 1u, false);
    check_sr3_select_case("S306",
        "<select required name=x><optgroup label=x><option value=x></option></optgroup>"
        "<option value='' selected></option></select>",
        1u, 0u, 1u, false);
    check_sr3_select_case("S307",
        "<select required name=x><optgroup label=x><option value='' selected></option>"
        "</optgroup></select>",
        1u, 0u, 1u, false);
    check_sr3_select_case("S308",
        "<select required name=x><div><option value=x></option></div>"
        "<option value='' selected></option></select>",
        1u, 0u, 1u, false);
    check_sr3_select_case("S309",
        "<select required name=x><option value='' selected></option>"
        "<option value=x selected></option></select>",
        1u, 0u, 1u, false);
    check_sr3_select_case("S310-invalid-single",
        "<select required size=bogus name=x><option value='' selected></option></select>",
        0u, 1u, 1u, false);
    check_sr3_select_case("S310-zero-single",
        "<select required size=0 name=x><option value='' selected></option></select>",
        0u, 1u, 1u, false);
    check_sr3_select_case("S310-invalid-multiple",
        "<select required multiple size=bogus name=x><option value='' selected></option></select>",
        0u, 0u, 0u, false);
    check_sr3_select_case("S310-zero-multiple",
        "<select required multiple size=0 name=x><option value='' selected></option></select>",
        0u, 0u, 0u, false);
}

static void check_sr4_case(
    const char *id, const char *body, uint16_t expected_rule,
    const char *expected_attribute, bool exact_identity) {
    static const char prefix[] = "<!doctype html><html><head><title>x</title></head><body>";
    static const char suffix[] = "</body></html>";
    char input[65536];
    const int length = snprintf(input, sizeof(input), "%s%s%s", prefix, body, suffix);
    need(length > 0 && (size_t)length < sizeof(input), "SR4 fixture construction");
    need(expected_rule <= UINT16_C(13), "SR4 expected rule ordinal");
    const arbor_span span = {(const uint8_t *)input, (uint64_t)length};
    arbor_view0_native_v1n2_g10_evaluation measured = {0};
    need(arbor_view0_native_v1n2_g10_measure(span, &measured).native == 0,
         "SR4 measurement");
    const uint64_t expected_total = expected_rule == UINT16_C(0)
        ? UINT64_C(0) : UINT64_C(1);
    if (measured.diagnostic_count != expected_total) {
        (void)fprintf(stderr,
            "FAIL: %s expected total=%llu; observed=%llu\n", id,
            (unsigned long long)expected_total,
            (unsigned long long)measured.diagnostic_count);
        exit(1);
    }
    for (uint16_t rule = UINT16_C(1); rule <= UINT16_C(13); ++rule) {
        const uint64_t expected = rule == expected_rule ? UINT64_C(1) : UINT64_C(0);
        need(measured.rule_violation_count[rule - UINT16_C(1)] == expected,
             "SR4 exact per-rule diagnostic counts");
    }
    if (!exact_identity) return;
    arbor_view0_native_v1n2_g10_anchor anchor = {0};
    arbor_view0_native_v1n2_g10_evaluation collected = {0};
    arbor_view0_native_v1n2_g10_anchor *anchors = expected_total == UINT64_C(0)
        ? NULL : &anchor;
    need(arbor_view0_native_v1n2_g10_collect_anchors(
        span, anchors, expected_total, &collected).native == 0,
        "SR4 exact anchor collection");
    need(memcmp(&measured, &collected, sizeof(measured)) == 0,
         "SR4 measure/collect identity");
    if (expected_total == UINT64_C(0)) return;
    need(expected_attribute != NULL, "SR4 expected attribute identity");
    const char *attribute = strstr(input, expected_attribute);
    need(attribute != NULL, "SR4 attribute source token");
    need(anchor.shared.group_ordinal == UINT16_C(10) &&
         anchor.shared.rule_ordinal == expected_rule &&
         anchor.shared.kind == (uint16_t)ARBOR_VIEW0_NATIVE_V1N2_ANCHOR_ATTRIBUTE_NAME &&
         anchor.shared.byte_offset == (uint64_t)(attribute - input) &&
         anchor.shared.source_length == (uint64_t)strlen(expected_attribute),
         "SR4 exact attribute anchor identity");
    arbor_view0_native_diagnostic diagnostic = {0};
    arbor_view0_native_v1n2_g10_materialize_anchor(&anchor, UINT64_C(0), &diagnostic);
    const char *symbolic_name = expected_rule == UINT16_C(4)
        ? "ARBOR_VIEW_V1_G10_BUTTON_SEMANTICS"
        : "ARBOR_VIEW_V1_G10_FORM_CONTROL_COMMON_SEMANTICS";
    need((expected_rule == UINT16_C(4) || expected_rule == UINT16_C(11)) &&
         strcmp(diagnostic.symbolic_name, symbolic_name) == 0 &&
         diagnostic.byte_offset == anchor.shared.byte_offset &&
         diagnostic.source_length == anchor.shared.source_length &&
         diagnostic.discovery_sequence == UINT64_C(0),
         "SR4 exact materialized diagnostic identity and order");
}

static void check_sr4_prior_owner_semantics(void) {
    check_sr4_case("S401", "<input type=text name=x formaction=/x>",
        UINT16_C(0), NULL, true);
    check_sr4_case("S402-formaction", "<input type=text name=x formaction=/x>",
        UINT16_C(0), NULL, false);
    check_sr4_case("S402-formmethod", "<input type=text name=x formmethod=post>",
        UINT16_C(0), NULL, false);
    check_sr4_case("S402-formenctype",
        "<input type=text name=x formenctype=multipart/form-data>",
        UINT16_C(0), NULL, false);
    check_sr4_case("S402-formtarget", "<input type=text name=x formtarget=_self>",
        UINT16_C(0), NULL, false);
    check_sr4_case("S402-formnovalidate", "<input type=text name=x formnovalidate>",
        UINT16_C(0), NULL, false);
    check_sr4_case("S403-reset", "<input type=reset name=x formaction=/x>",
        UINT16_C(0), NULL, false);
    check_sr4_case("S403-button", "<input type=button name=x formaction=/x>",
        UINT16_C(0), NULL, false);
    check_sr4_case("S404-checkbox", "<input type=checkbox name=x formaction=/x>",
        UINT16_C(0), NULL, false);
    check_sr4_case("S404-file", "<input type=file name=x formaction=/x>",
        UINT16_C(0), NULL, false);
    check_sr4_case("S405", "<form id=f></form><input type=submit form=f formaction=/x>",
        UINT16_C(0), NULL, false);
    check_sr4_case("S406", "<input type=submit formaction=/x>",
        UINT16_C(11), "formaction", true);
    check_sr4_case("S407-resolved",
        "<form id=f></form><input type=image form=f formaction=/x src=x alt=x name=x>",
        UINT16_C(0), NULL, false);
    check_sr4_case("S407-absent",
        "<input type=image formaction=/x src=x alt=x name=x>",
        UINT16_C(11), "formaction", false);
    check_sr4_case("S408-missing-resolved",
        "<form id=f></form><button form=f formaction=/x>x</button>",
        UINT16_C(0), NULL, false);
    check_sr4_case("S408-invalid-resolved",
        "<form id=f></form><button type=bogus form=f formaction=/x>x</button>",
        UINT16_C(0), NULL, false);
    check_sr4_case("S408-missing-absent", "<button formaction=/x>x</button>",
        UINT16_C(11), "formaction", false);
    check_sr4_case("S408-invalid-absent",
        "<button type=bogus formaction=/x>x</button>",
        UINT16_C(11), "formaction", false);
    check_sr4_case("S409", "<button type=button formaction=/x>x</button>",
        UINT16_C(4), "formaction", true);
}

static void check_sr5_r2_case(
    const char *id, const char *body, uint64_t expected_g05,
    uint64_t expected_g06, uint64_t expected_g10_r6) {
    static const char prefix[] = "<!doctype html><html><head><title>x</title></head><body>";
    static const char suffix[] = "</body></html>";
    char input[65536];
    const int length = snprintf(input, sizeof(input), "%s%s%s", prefix, body, suffix);
    need(length > 0 && (size_t)length < sizeof(input), "SR5-R2 fixture construction");
    need(expected_g05 <= UINT64_C(1) && expected_g06 <= UINT64_C(1) &&
         expected_g10_r6 <= UINT64_C(1), "SR5-R2 expected count bounds");
    const arbor_span span = {(const uint8_t *)input, (uint64_t)length};

    arbor_view0_native_v1n2_g10_evaluation g10_measured = {0};
    need(arbor_view0_native_v1n2_g10_measure(span, &g10_measured).native == 0,
         "SR5-R2 G10 measurement");
    need(g10_measured.diagnostic_count == expected_g10_r6,
         "SR5-R2 exact G10 diagnostic count");
    for (size_t rule = 0u; rule < 13u; ++rule) {
        const uint64_t expected = rule == 5u ? expected_g10_r6 : UINT64_C(0);
        need(g10_measured.rule_violation_count[rule] == expected,
             "SR5-R2 exact G10 per-rule counts");
    }
    arbor_view0_native_v1n2_g10_anchor g10_anchor = {0};
    arbor_view0_native_v1n2_g10_evaluation g10_collected = {0};
    need(arbor_view0_native_v1n2_g10_collect_anchors(
        span, expected_g10_r6 == 0u ? NULL : &g10_anchor,
        expected_g10_r6, &g10_collected).native == 0,
        "SR5-R2 G10 anchor collection");
    need(memcmp(&g10_measured, &g10_collected, sizeof(g10_measured)) == 0,
         "SR5-R2 G10 measure/collect identity");

    arbor_view0_native_g05_r3a_evaluation g05_measured = {0};
    need(arbor_view0_native_g05_r3a_measure(span, &g05_measured).native == 0,
         "SR5-R2 G05 measurement");
    need(g05_measured.diagnostic_count == expected_g05 &&
         g05_measured.clause_violation_count[40] == expected_g05,
         "SR5-R2 exact G05 owner count");
    arbor_view0_native_source_anchor g05_anchor = {0};
    arbor_view0_native_g05_r3a_evaluation g05_collected = {0};
    need(arbor_view0_native_g05_r3a_collect_anchors(
        span, expected_g05 == 0u ? NULL : &g05_anchor,
        expected_g05, &g05_collected).native == 0,
        "SR5-R2 G05 anchor collection");
    need(memcmp(&g05_measured, &g05_collected, sizeof(g05_measured)) == 0,
         "SR5-R2 G05 measure/collect identity");

    arbor_view0_native_g06_evaluation g06_measured = {0};
    need(arbor_view0_native_g06_measure(span, &g06_measured).native == 0,
         "SR5-R2 G06 measurement");
    need(g06_measured.diagnostic_count == expected_g06 &&
         g06_measured.rule_violation_count[3] == expected_g06,
         "SR5-R2 exact G06 owner count");
    arbor_view0_native_g06_anchor g06_anchor = {0};
    arbor_view0_native_g06_evaluation g06_collected = {0};
    need(arbor_view0_native_g06_collect_anchors(
        span, expected_g06 == 0u ? NULL : &g06_anchor,
        expected_g06, &g06_collected).native == 0,
        "SR5-R2 G06 anchor collection");
    need(memcmp(&g06_measured, &g06_collected, sizeof(g06_measured)) == 0,
         "SR5-R2 G06 measure/collect identity");

    if (expected_g05 != 0u) {
        arbor_view0_native_diagnostic diagnostic = {0};
        arbor_view0_native_g05_r3a_materialize_anchor(&g05_anchor, UINT64_C(0), &diagnostic);
        need(strcmp(diagnostic.symbolic_name,
            "ARBOR_VIEW_V1_G05_CONDITIONAL_ATTRIBUTE_APPLICABILITY") == 0,
            "SR5-R2 exact G05 symbolic owner");
    }
    if (expected_g06 != 0u) {
        arbor_view0_native_diagnostic diagnostic = {0};
        need(g06_anchor.rule_ordinal == UINT16_C(4), "SR5-R2 exact G06 rule owner");
        arbor_view0_native_g06_materialize_anchor(&g06_anchor, UINT64_C(0), &diagnostic);
        need(strcmp(diagnostic.symbolic_name,
            "ARBOR_VIEW_V1_G06_NONNEGATIVE_INTEGER") == 0,
            "SR5-R2 exact G06 symbolic owner");
    }
    if (expected_g10_r6 != 0u) {
        arbor_view0_native_diagnostic diagnostic = {0};
        need(g10_anchor.shared.rule_ordinal == UINT16_C(6),
             "SR5-R2 exact retained G10 rule");
        arbor_view0_native_v1n2_g10_materialize_anchor(
            &g10_anchor, UINT64_C(0), &diagnostic);
        need(strcmp(diagnostic.symbolic_name,
            "ARBOR_VIEW_V1_G10_TEXTAREA_SEMANTICS") == 0,
            "SR5-R2 exact retained G10 symbolic owner");
    }
    (void)id;
}

static void check_sr5_r2_integrated_prior_owner_semantics(void) {
    check_sr5_r2_case("S5R101", "<textarea name=x rows=0></textarea>", 0u, 1u, 0u);
    check_sr5_r2_case("S5R102", "<textarea name=x cols=0></textarea>", 0u, 1u, 0u);
    check_sr5_r2_case("S5R103", "<textarea name=x cols=bogus></textarea>", 0u, 1u, 0u);
    check_sr5_r2_case("S5R104", "<textarea name=x wrap=hard></textarea>", 1u, 0u, 0u);
    check_sr5_r2_case("S5R105", "<textarea name=x wrap=hard cols=bogus></textarea>",
        0u, 1u, 0u);
    check_sr5_r2_case("S5R106", "<textarea name=x wrap=hard cols=0></textarea>",
        0u, 1u, 0u);
    check_sr5_r2_case("S5R107", "<textarea name=x wrap=hard cols=20></textarea>",
        0u, 0u, 0u);
    check_sr5_r2_case("S5R108", "<textarea name=x rows=bogus></textarea>", 0u, 1u, 0u);
    check_sr5_r2_case("S5R109", "<textarea name=x rows=1 cols=1></textarea>",
        0u, 0u, 0u);
    check_sr5_r2_case("S5R110", "<textarea name=x wrap=HARD></textarea>", 1u, 0u, 0u);
    check_sr5_r2_case("S5R111", "<textarea name=x wrap=soft></textarea>", 0u, 0u, 0u);
    check_sr5_r2_case("S5R112-minmax",
        "<textarea name=x minlength=5 maxlength=2></textarea>", 0u, 0u, 1u);
    check_sr5_r2_case("S5R112-placeholder",
        "<textarea name=x placeholder='a&#10;b'></textarea>", 0u, 0u, 1u);
}

static void check_sr6_case(
    const char *id, const char *body, uint64_t expected_g06_r5,
    uint64_t expected_g10_r8, bool exact_g10_identity) {
    static const char prefix[] = "<!doctype html><html><head><title>x</title></head><body>";
    static const char suffix[] = "</body></html>";
    char input[65536];
    const int length = snprintf(input, sizeof(input), "%s%s%s", prefix, body, suffix);
    need(length > 0 && (size_t)length < sizeof(input), "SR6 fixture construction");
    need(expected_g06_r5 <= UINT64_C(2) && expected_g10_r8 <= UINT64_C(1),
         "SR6 expected count bounds");
    const arbor_span span = {(const uint8_t *)input, (uint64_t)length};

    arbor_view0_native_v1n2_g10_evaluation g10_measured = {0};
    need(arbor_view0_native_v1n2_g10_measure(span, &g10_measured).native == 0,
         "SR6 G10 measurement");
    need(g10_measured.diagnostic_count == expected_g10_r8,
         "SR6 exact G10 diagnostic count");
    for (size_t rule = 0u; rule < 13u; ++rule) {
        const uint64_t expected = rule == 7u ? expected_g10_r8 : UINT64_C(0);
        need(g10_measured.rule_violation_count[rule] == expected,
             "SR6 exact G10 per-rule counts");
    }
    arbor_view0_native_v1n2_g10_anchor g10_anchor = {0};
    arbor_view0_native_v1n2_g10_evaluation g10_collected = {0};
    need(arbor_view0_native_v1n2_g10_collect_anchors(
        span, expected_g10_r8 == 0u ? NULL : &g10_anchor,
        expected_g10_r8, &g10_collected).native == 0,
        "SR6 G10 anchor collection");
    need(memcmp(&g10_measured, &g10_collected, sizeof(g10_measured)) == 0,
         "SR6 G10 measure/collect identity");

    arbor_view0_native_g06_evaluation g06_measured = {0};
    need(arbor_view0_native_g06_measure(span, &g06_measured).native == 0,
         "SR6 G06 measurement");
    need(g06_measured.diagnostic_count == expected_g06_r5 &&
         g06_measured.rule_violation_count[4] == expected_g06_r5,
         "SR6 exact G06 owner count");
    for (size_t rule = 0u; rule < 17u; ++rule) {
        const uint64_t expected = rule == 4u ? expected_g06_r5 : UINT64_C(0);
        need(g06_measured.rule_violation_count[rule] == expected,
             "SR6 exact G06 per-rule counts");
    }
    arbor_view0_native_g06_anchor g06_anchors[2];
    (void)memset(g06_anchors, 0, sizeof(g06_anchors));
    arbor_view0_native_g06_evaluation g06_collected = {0};
    need(arbor_view0_native_g06_collect_anchors(
        span, expected_g06_r5 == 0u ? NULL : g06_anchors,
        expected_g06_r5, &g06_collected).native == 0,
        "SR6 G06 anchor collection");
    need(memcmp(&g06_measured, &g06_collected, sizeof(g06_measured)) == 0,
         "SR6 G06 measure/collect identity");
    for (uint64_t index = 0u; index < expected_g06_r5; ++index) {
        arbor_view0_native_diagnostic diagnostic = {0};
        need(g06_anchors[index].rule_ordinal == UINT16_C(5),
             "SR6 exact G06 rule owner");
        arbor_view0_native_g06_materialize_anchor(&g06_anchors[index], index, &diagnostic);
        need(strcmp(diagnostic.symbolic_name,
            "ARBOR_VIEW_V1_G06_FLOATING_POINT") == 0 &&
            diagnostic.discovery_sequence == index,
            "SR6 exact G06 symbolic owner and order");
    }

    if (exact_g10_identity) {
        arbor_view0_native_diagnostic diagnostic = {0};
        need(expected_g10_r8 == UINT64_C(1) &&
             g10_anchor.shared.group_ordinal == UINT16_C(10) &&
             g10_anchor.shared.rule_ordinal == UINT16_C(8) &&
             g10_anchor.shared.kind ==
                 (uint16_t)ARBOR_VIEW0_NATIVE_V1N2_ANCHOR_ELEMENT,
             "S610 exact G10-R8 anchor identity");
        arbor_view0_native_v1n2_g10_materialize_anchor(
            &g10_anchor, UINT64_C(0), &diagnostic);
        need(strcmp(diagnostic.symbolic_name,
            "ARBOR_VIEW_V1_G10_PROGRESS_METER_SEMANTICS") == 0 &&
            diagnostic.byte_offset == g10_anchor.shared.byte_offset &&
            diagnostic.source_length == g10_anchor.shared.source_length &&
            diagnostic.discovery_sequence == UINT64_C(0),
            "S610 exact materialized G10-R8 identity and order");
    }
    (void)id;
}

static void check_sr6_progress_meter_prior_owner_semantics(void) {
    check_sr6_case("S601", "<progress value=2 max=1></progress>", 1u, 0u, false);
    check_sr6_case("S602", "<progress value=0 max=0></progress>", 1u, 0u, false);
    check_sr6_case("S603", "<meter value=2 min=2 max=1></meter>", 2u, 0u, false);
    check_sr6_case("S604", "<meter value=2 min=1 max=3 low=3 high=2></meter>",
        1u, 0u, false);
    check_sr6_case("S605", "<meter value=2 min=1 max=3 optimum=4></meter>",
        1u, 0u, false);
    check_sr6_case("S606", "<meter></meter>", 0u, 1u, false);
    check_sr6_case("S607", "<meter value=bogus></meter>", 1u, 0u, false);
    check_sr6_case("S608", "<progress value=2 max=bogus></progress>", 2u, 0u, false);
    check_sr6_case("S609", "<meter value=2 min=1 max=3></meter>", 0u, 0u, false);
    check_sr6_case("S610", "<meter></meter>", 0u, 1u, true);
}

static void check_sr7_case(
    const char *id, const char *body, uint64_t expected_g03_r1,
    uint64_t expected_g03_r2, uint64_t expected_g10_r10,
    bool exact_g10_identity) {
    static const char prefix[] = "<!doctype html><html><head><title>x</title></head><body>";
    static const char suffix[] = "</body></html>";
    char input[65536];
    const int length = snprintf(input, sizeof(input), "%s%s%s", prefix, body, suffix);
    need(length > 0 && (size_t)length < sizeof(input), "SR7 fixture construction");
    need(expected_g03_r1 <= UINT64_C(1) && expected_g03_r2 <= UINT64_C(1) &&
         expected_g10_r10 <= UINT64_C(1), "SR7 expected count bounds");
    const arbor_span span = {(const uint8_t *)input, (uint64_t)length};

    arbor_view0_native_v1n2_g10_evaluation g10_measured = {0};
    need(arbor_view0_native_v1n2_g10_measure(span, &g10_measured).native == 0,
         "SR7 G10 measurement");
    need(g10_measured.diagnostic_count == expected_g10_r10,
         "SR7 exact G10 diagnostic count");
    for (size_t rule = 0u; rule < 13u; ++rule) {
        const uint64_t expected = rule == 9u ? expected_g10_r10 : UINT64_C(0);
        need(g10_measured.rule_violation_count[rule] == expected,
             "SR7 exact G10 per-rule counts");
    }
    arbor_view0_native_v1n2_g10_anchor g10_anchor = {0};
    arbor_view0_native_v1n2_g10_evaluation g10_collected = {0};
    need(arbor_view0_native_v1n2_g10_collect_anchors(
        span, expected_g10_r10 == 0u ? NULL : &g10_anchor,
        expected_g10_r10, &g10_collected).native == 0,
        "SR7 G10 anchor collection");
    need(memcmp(&g10_measured, &g10_collected, sizeof(g10_measured)) == 0,
         "SR7 G10 measure/collect identity");

    arbor_view0_native_diagnostic diagnostics[64] = {{0}};
    arbor_view0_native_result result = {0};
    need(arbor_view0_native_check(span, diagnostics, UINT64_C(64), &result).native == 0,
         "SR7 integrated check");
    need(count_rule_id(diagnostics, result.diagnostic_count,
             ARBOR_VIEW_V1_G03_ELEMENT_CONTEXT) == expected_g03_r1,
         "SR7 exact integrated G03-R1 count");
    need(count_rule_id(diagnostics, result.diagnostic_count,
             ARBOR_VIEW_V1_G03_CONTENT_MODEL) == expected_g03_r2,
         "SR7 exact integrated G03-R2 count");
    need(count_rule_id(diagnostics, result.diagnostic_count,
             ARBOR_VIEW0_NATIVE_RULE_ID_V1N2_G10_R10) == expected_g10_r10,
         "SR7 exact integrated G10-R10 count");
    need((result.flags &
             ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R4_DEFERRED_SELECTEDCONTENT_PROVENANCE) == 0u,
         "SR7 exact selectedcontent deferral retirement");

    if (exact_g10_identity) {
        const char *selectedcontent = strstr(input, "selectedcontent");
        arbor_view0_native_diagnostic diagnostic = {0};
        need(expected_g10_r10 == UINT64_C(1) && selectedcontent != NULL &&
             g10_anchor.shared.group_ordinal == UINT16_C(10) &&
             g10_anchor.shared.rule_ordinal == UINT16_C(10) &&
             g10_anchor.shared.kind ==
                 (uint16_t)ARBOR_VIEW0_NATIVE_V1N2_ANCHOR_ELEMENT &&
             g10_anchor.shared.byte_offset == (uint64_t)(selectedcontent - input) &&
             g10_anchor.shared.source_length == UINT64_C(15),
             "S704 exact G10-R10 selectedcontent anchor identity");
        arbor_view0_native_v1n2_g10_materialize_anchor(
            &g10_anchor, UINT64_C(0), &diagnostic);
        need(strcmp(diagnostic.symbolic_name,
            "ARBOR_VIEW_V1_G10_SELECTEDCONTENT_RECONCILIATION") == 0 &&
            diagnostic.byte_offset == g10_anchor.shared.byte_offset &&
            diagnostic.source_length == g10_anchor.shared.source_length &&
            diagnostic.discovery_sequence == UINT64_C(0),
            "S704 exact materialized G10-R10 identity and order");
    }
    (void)id;
}

static void check_sr7_selectedcontent_provenance_reconciliation(void) {
    check_sr7_case("S701",
        "<select name=x><button><span><selectedcontent></selectedcontent></span></button>"
        "<option>x</option></select>", 0u, 0u, 0u, false);
    check_sr7_case("S702", "<selectedcontent></selectedcontent>",
        1u, 0u, 0u, false);
    check_sr7_case("S703",
        "<select name=x><button><selectedcontent></selectedcontent></button>"
        "<option>x</option></select>", 0u, 0u, 0u, false);
    check_sr7_case("S704",
        "<select name=x><button><selectedcontent>x</selectedcontent></button>"
        "<option>x</option></select>", 0u, 0u, 1u, true);
    check_sr7_case("S705",
        "<select name=x><button><selectedcontent><span>x</span></selectedcontent></button>"
        "<option>x</option></select>", 0u, 0u, 1u, false);
    check_sr7_case("S706",
        "<select name=x><button><selectedcontent> \n\t\r\f&#x20;</selectedcontent></button>"
        "<option>x</option></select>", 0u, 0u, 0u, false);
    check_sr7_case("S707",
        "<select name=x><button><selectedcontent><!--x--></selectedcontent></button>"
        "<option>x</option></select>", 0u, 0u, 0u, false);
    check_sr7_case("S708",
        "<select name=x><button><selectedcontent>x<span>y</span>z</selectedcontent></button>"
        "<option>x</option></select>", 0u, 0u, 1u, false);
    check_sr7_case("S709",
        "<select name=x><button><selectedcontent></selectedcontent>"
        "<selectedcontent></selectedcontent></button><option>x</option></select>",
        0u, 1u, 0u, false);
}

static void check_sr8_case(
    const char *id, const char *body, uint64_t expected_g03_r3,
    uint64_t expected_g10_r2, bool expected_labeled_control_deferral,
    bool exact_attribute_anchor,
    bool exact_element_anchor) {
    static const char prefix[] = "<!doctype html><html><head><title>x</title></head><body>";
    static const char suffix[] = "</body></html>";
    char input[65536];
    const int length = snprintf(input, sizeof(input), "%s%s%s", prefix, body, suffix);
    need(length > 0 && (size_t)length < sizeof(input), "SR8 fixture construction");
    need(expected_g03_r3 <= UINT64_C(1) && expected_g10_r2 <= UINT64_C(1),
         "SR8 expected count bounds");
    const arbor_span span = {(const uint8_t *)input, (uint64_t)length};

    arbor_view0_native_v1n2_g10_evaluation g10_measured = {0};
    need(arbor_view0_native_v1n2_g10_measure(span, &g10_measured).native == 0,
         "SR8 G10 measurement");
    need(g10_measured.diagnostic_count == expected_g10_r2,
         "SR8 exact G10 diagnostic count");
    for (size_t rule = 0u; rule < 13u; ++rule) {
        const uint64_t expected = rule == 1u ? expected_g10_r2 : UINT64_C(0);
        need(g10_measured.rule_violation_count[rule] == expected,
             "SR8 exact G10 per-rule counts");
    }
    arbor_view0_native_v1n2_g10_anchor g10_anchor = {0};
    arbor_view0_native_v1n2_g10_evaluation g10_collected = {0};
    need(arbor_view0_native_v1n2_g10_collect_anchors(
        span, expected_g10_r2 == 0u ? NULL : &g10_anchor,
        expected_g10_r2, &g10_collected).native == 0,
        "SR8 G10 anchor collection");
    need(memcmp(&g10_measured, &g10_collected, sizeof(g10_measured)) == 0,
         "SR8 G10 measure/collect identity");

    arbor_view0_native_g03_r3a_evaluation g03_measured = {0};
    need(arbor_view0_native_g03_r3a_measure(span, &g03_measured).native == 0,
         "SR8 G03-R3 measurement");
    const bool labeled_control_deferred = (g03_measured.deferred_flags &
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_DEFERRED_LABELED_CONTROL) != 0u;
    if (g03_measured.diagnostic_count != expected_g03_r3 ||
        labeled_control_deferred != expected_labeled_control_deferral) {
        (void)fprintf(stderr,
            "FAIL: %s frozen G03-R3 count=%llu flags=0x%llx\n", id,
            (unsigned long long)g03_measured.diagnostic_count,
            (unsigned long long)g03_measured.deferred_flags);
        exit(1);
    }

    arbor_view0_native_diagnostic diagnostics[64] = {{0}};
    arbor_view0_native_result result = {0};
    need(arbor_view0_native_check(span, diagnostics, UINT64_C(64), &result).native == 0,
         "SR8 integrated check");
    const uint64_t integrated_g03_r3 = count_rule_id(
        diagnostics, result.diagnostic_count, ARBOR_VIEW_V1_G03_DESCENDANT_EXCLUSIONS);
    if (integrated_g03_r3 != expected_g03_r3) {
        (void)fprintf(stderr,
            "FAIL: %s expected integrated G03-R3=%llu observed=%llu total=%llu\n",
            id, (unsigned long long)expected_g03_r3,
            (unsigned long long)integrated_g03_r3,
            (unsigned long long)result.diagnostic_count);
        exit(1);
    }
    need(count_rule_id(diagnostics, result.diagnostic_count,
             ARBOR_VIEW0_NATIVE_RULE_ID_V1N2_G10_R2) == expected_g10_r2,
         "SR8 exact integrated G10-R2 count");
    need((result.flags &
             ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_DEFERRED_LABELED_CONTROL) == 0u,
         "SR8 exact labeled-control deferral retirement");
    need((result.flags & g03_measured.deferred_flags) ==
             (g03_measured.deferred_flags &
                ~ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_DEFERRED_LABELED_CONTROL),
         "SR8 every other G03-R3 deferral flag retained");

    if (exact_attribute_anchor || exact_element_anchor) {
        const char *token = strstr(input, exact_attribute_anchor ? "for" : "label");
        need(expected_g10_r2 == UINT64_C(1) && token != NULL &&
             g10_anchor.shared.group_ordinal == UINT16_C(10) &&
             g10_anchor.shared.rule_ordinal == UINT16_C(2) &&
             g10_anchor.shared.kind == (exact_attribute_anchor
                 ? (uint16_t)ARBOR_VIEW0_NATIVE_V1N2_ANCHOR_ATTRIBUTE_NAME
                 : (uint16_t)ARBOR_VIEW0_NATIVE_V1N2_ANCHOR_ELEMENT) &&
             g10_anchor.shared.byte_offset == (uint64_t)(token - input) &&
             g10_anchor.shared.source_length ==
                 (exact_attribute_anchor ? UINT64_C(3) : UINT64_C(5)),
             "SR8 exact G10-R2 anchor identity");
        arbor_view0_native_diagnostic diagnostic = {0};
        arbor_view0_native_v1n2_g10_materialize_anchor(
            &g10_anchor, UINT64_C(0), &diagnostic);
        need(strcmp(diagnostic.symbolic_name,
            "ARBOR_VIEW_V1_G10_LABEL_ASSOCIATION") == 0 &&
            diagnostic.byte_offset == g10_anchor.shared.byte_offset &&
            diagnostic.source_length == g10_anchor.shared.source_length &&
            diagnostic.discovery_sequence == UINT64_C(0),
            "SR8 exact materialized G10-R2 identity and order");
    }
    (void)id;
}

static void check_sr8_label_descendant_reconciliation(void) {
    check_sr8_case("S801", "<label></label>", 0u, 0u, false, false, false);
    check_sr8_case("S802", "<label for=x>x</label><div id=x></div>",
        0u, 1u, false, true, false);
    check_sr8_case("S803", "<label><input name=x></label>",
        0u, 0u, true, false, false);
    check_sr8_case("S804", "<label><input name=x><input name=y></label>",
        0u, 1u, true, false, true);
    check_sr8_case("S805", "<label for=x><input id=x name=x><input name=y></label>",
        0u, 1u, true, false, false);
    check_sr8_case("S806", "<label for=x><input name=y></label><input id=x name=x>",
        0u, 1u, true, false, false);
    check_sr8_case("S807", "<label><input name=x><input name=y><button>z</button></label>",
        0u, 1u, true, false, false);
    check_sr8_case("S808", "<label><input type=hidden name=x></label>",
        0u, 0u, true, false, false);
    check_sr8_case("S809", "<label><input type=hidden name=x><input name=y></label>",
        0u, 0u, true, false, false);
    check_sr8_case("S810", "<label for=x>x</label><input id=x type=hidden name=x>",
        0u, 1u, false, false, false);
    check_sr8_case("S811", "<label for=x>a</label><label for=x>b</label><input id=x name=x>",
        0u, 0u, false, false, false);
    check_sr8_case("S812", "<label><label></label></label>",
        1u, 0u, false, false, false);
}

static void check_sr9_case(
    const char *id, const char *body, uint64_t expected_g10_r11,
    const size_t *expected_autofocus_ordinals) {
    static const char prefix[] = "<!doctype html><html><head><title>x</title></head><body>";
    static const char suffix[] = "</body></html>";
    char input[65536];
    const int length = snprintf(input, sizeof(input), "%s%s%s", prefix, body, suffix);
    need(length > 0 && (size_t)length < sizeof(input), "SR9 fixture construction");
    need(expected_g10_r11 <= UINT64_C(4), "SR9 expected count bounds");
    const arbor_span span = {(const uint8_t *)input, (uint64_t)length};

    uint64_t autofocus_offsets[8] = {0};
    size_t autofocus_count = 0u;
    const char *cursor = input;
    while ((cursor = strstr(cursor, "autofocus")) != NULL) {
        need(autofocus_count < sizeof(autofocus_offsets) / sizeof(autofocus_offsets[0]),
             "SR9 autofocus token capacity");
        autofocus_offsets[autofocus_count++] = (uint64_t)(cursor - input);
        cursor += strlen("autofocus");
    }

    arbor_view0_native_v1n2_g10_evaluation measured = {0};
    need(arbor_view0_native_v1n2_g10_measure(span, &measured).native == 0,
         "SR9 G10 measurement");
    need(measured.diagnostic_count == expected_g10_r11,
         "SR9 exact G10 diagnostic count");
    for (size_t rule = 0u; rule < 13u; ++rule) {
        const uint64_t expected = rule == 10u ? expected_g10_r11 : UINT64_C(0);
        need(measured.rule_violation_count[rule] == expected,
             "SR9 exact G10 per-rule counts");
    }

    arbor_view0_native_v1n2_g10_anchor anchors[4];
    (void)memset(anchors, 0, sizeof(anchors));
    arbor_view0_native_v1n2_g10_evaluation collected = {0};
    need(arbor_view0_native_v1n2_g10_collect_anchors(
        span, expected_g10_r11 == 0u ? NULL : anchors,
        expected_g10_r11, &collected).native == 0,
        "SR9 G10 anchor collection");
    need(memcmp(&measured, &collected, sizeof(measured)) == 0,
         "SR9 G10 measure/collect identity");

    for (uint64_t index = 0u; index < expected_g10_r11; ++index) {
        const size_t ordinal = expected_autofocus_ordinals[index];
        need(ordinal < autofocus_count &&
             anchors[index].shared.group_ordinal == UINT16_C(10) &&
             anchors[index].shared.rule_ordinal == UINT16_C(11) &&
             anchors[index].shared.kind ==
                 (uint16_t)ARBOR_VIEW0_NATIVE_V1N2_ANCHOR_ATTRIBUTE_NAME &&
             anchors[index].shared.byte_offset == autofocus_offsets[ordinal] &&
             anchors[index].shared.source_length == UINT64_C(9),
             "SR9 exact autofocus anchor identity and order");
    }

    arbor_view0_native_diagnostic diagnostics[64] = {{0}};
    arbor_view0_native_result result = {0};
    need(arbor_view0_native_check(span, diagnostics, UINT64_C(64), &result).native == 0,
         "SR9 integrated check");
    need(count_rule_id(diagnostics, result.diagnostic_count,
             ARBOR_VIEW0_NATIVE_RULE_ID_V1N2_G10_R11) == expected_g10_r11,
         "SR9 exact integrated G10-R11 count");

    if (expected_g10_r11 != 0u) {
        arbor_view0_native_diagnostic diagnostic = {0};
        arbor_view0_native_v1n2_g10_materialize_anchor(
            anchors, UINT64_C(0), &diagnostic);
        need(strcmp(diagnostic.symbolic_name,
            "ARBOR_VIEW_V1_G10_FORM_CONTROL_COMMON_SEMANTICS") == 0 &&
            diagnostic.byte_offset == anchors[0].shared.byte_offset &&
            diagnostic.source_length == anchors[0].shared.source_length &&
            diagnostic.discovery_sequence == UINT64_C(0),
            "SR9 exact materialized G10-R11 identity");
    }
    (void)id;
}

static void check_sr9_capacity_and_identity(void) {
    static const char prefix[] = "<!doctype html><html><head><title>x</title></head><body>";
    static const char token[] = "<br autofocus>";
    static const char suffix[] = "</body></html>";
    static char input[65536];
    static arbor_view0_native_v1n2_g10_anchor anchors[4096];
    size_t length = 0u;
    (void)memcpy(input + length, prefix, sizeof(prefix) - 1u);
    length += sizeof(prefix) - 1u;
    for (size_t index = 0u; index < 4098u; ++index) {
        need(sizeof(token) - 1u <= sizeof(input) - length,
             "S913 generated input capacity");
        (void)memcpy(input + length, token, sizeof(token) - 1u);
        length += sizeof(token) - 1u;
    }
    need(sizeof(suffix) - 1u <= sizeof(input) - length,
         "S913 generated suffix capacity");
    (void)memcpy(input + length, suffix, sizeof(suffix) - 1u);
    length += sizeof(suffix) - 1u;

    const arbor_span span = {(const uint8_t *)input, (uint64_t)length};
    arbor_view0_native_v1n2_g10_evaluation measured = {0};
    arbor_view0_native_v1n2_g10_evaluation collected = {0};
    need(arbor_view0_native_v1n2_g10_measure(span, &measured).native == 0 &&
         measured.diagnostic_count == UINT64_C(4096) &&
         measured.rule_violation_count[10] == UINT64_C(4096),
         "S913 exact diagnostic cap");
    need(arbor_view0_native_v1n2_g10_collect_anchors(
        span, anchors, UINT64_C(4096), &collected).native == 0 &&
         memcmp(&measured, &collected, sizeof(measured)) == 0,
         "S913 capped measure/collect identity");
    const uint64_t prefix_length = (uint64_t)(sizeof(prefix) - 1u);
    const uint64_t token_length = (uint64_t)(sizeof(token) - 1u);
    need(anchors[0].shared.byte_offset == prefix_length + token_length + UINT64_C(4) &&
         anchors[4095].shared.byte_offset ==
             prefix_length + token_length * UINT64_C(4096) + UINT64_C(4) &&
         anchors[0].shared.rule_ordinal == UINT16_C(11) &&
         anchors[4095].shared.rule_ordinal == UINT16_C(11),
         "S913 capped anchor order and endpoint identity");
}

static void check_sr9_autofocus_scoping_root_reconciliation(void) {
    static const size_t second[] = {1u};
    static const size_t two_root_seconds[] = {1u, 3u};
    static const size_t second_and_third[] = {1u, 2u};
    check_sr9_case("S901", "<input autofocus><button autofocus>x</button>",
        1u, second);
    check_sr9_case("S902",
        "<dialog><input autofocus><button autofocus>x</button></dialog>",
        1u, second);
    check_sr9_case("S903",
        "<dialog><input autofocus></dialog><dialog><button autofocus>x</button></dialog>",
        0u, NULL);
    check_sr9_case("S904",
        "<input autofocus><dialog><button autofocus>x</button></dialog>",
        0u, NULL);
    check_sr9_case("S905",
        "<div popover><input autofocus><button autofocus>x</button></div>",
        1u, second);
    check_sr9_case("S906",
        "<div popover><input autofocus></div><div popover><button autofocus>x</button></div>",
        0u, NULL);
    check_sr9_case("S907",
        "<input autofocus><div popover><button autofocus>x</button></div>",
        0u, NULL);
    check_sr9_case("S908",
        "<dialog><input autofocus><div popover><button autofocus>x</button></div></dialog>",
        0u, NULL);
    check_sr9_case("S909", "<div autofocus></div><input autofocus>",
        1u, second);
    check_sr9_case("S910", "<div autofocus></div><span autofocus></span>",
        1u, second);
    check_sr9_case("S911",
        "<span autofocus></span><dialog><div autofocus></div></dialog>",
        0u, NULL);
    check_sr9_case("S912",
        "<div popover><span autofocus></span><b autofocus></b></div>"
        "<section popover=bogus><i autofocus></i><em autofocus></em></section>",
        2u, two_root_seconds);
    check_sr9_case("S913-order",
        "<div autofocus></div><span autofocus></span><b autofocus></b>",
        2u, second_and_third);
    check_sr9_capacity_and_identity();
}

static uint64_t sr10_token_offset(
    const char *input, const char *token, size_t occurrence) {
    const char *cursor = input;
    for (size_t index = 0u; index <= occurrence; ++index) {
        cursor = strstr(cursor, token);
        need(cursor != NULL, "SR10 expected anchor token");
        if (index != occurrence) cursor += strlen(token);
    }
    return (uint64_t)(cursor - input);
}

static void check_sr10_case(
    const char *id, const char *body, uint64_t expected_g10_r12,
    const char *const *anchor_tokens, const size_t *anchor_occurrences) {
    static const char prefix[] = "<!doctype html><html><head><title>x</title></head><body>";
    static const char suffix[] = "</body></html>";
    char input[65536];
    const int length = snprintf(input, sizeof(input), "%s%s%s", prefix, body, suffix);
    need(length > 0 && (size_t)length < sizeof(input), "SR10 fixture construction");
    need(expected_g10_r12 <= UINT64_C(4), "SR10 expected count bounds");
    const arbor_span span = {(const uint8_t *)input, (uint64_t)length};

    arbor_view0_native_v1n2_g10_evaluation measured = {0};
    need(arbor_view0_native_v1n2_g10_measure(span, &measured).native == 0,
         "SR10 G10 measurement");
    need(measured.diagnostic_count == expected_g10_r12,
         "SR10 exact G10 diagnostic count");
    for (size_t rule = 0u; rule < 13u; ++rule) {
        const uint64_t expected = rule == 11u ? expected_g10_r12 : UINT64_C(0);
        need(measured.rule_violation_count[rule] == expected,
             "SR10 exact G10 per-rule counts");
    }

    arbor_view0_native_v1n2_g10_anchor anchors[4];
    (void)memset(anchors, 0, sizeof(anchors));
    arbor_view0_native_v1n2_g10_evaluation collected = {0};
    need(arbor_view0_native_v1n2_g10_collect_anchors(
        span, expected_g10_r12 == 0u ? NULL : anchors,
        expected_g10_r12, &collected).native == 0,
        "SR10 G10 anchor collection");
    need(memcmp(&measured, &collected, sizeof(measured)) == 0,
         "SR10 G10 measure/collect identity");

    for (uint64_t index = 0u; index < expected_g10_r12; ++index) {
        const char *token = anchor_tokens[index];
        const uint64_t offset = sr10_token_offset(
            input, token, anchor_occurrences[index]);
        need(anchors[index].shared.group_ordinal == UINT16_C(10) &&
             anchors[index].shared.rule_ordinal == UINT16_C(12) &&
             anchors[index].shared.kind ==
                 (uint16_t)ARBOR_VIEW0_NATIVE_V1N2_ANCHOR_ATTRIBUTE_NAME &&
             anchors[index].shared.byte_offset == offset &&
             anchors[index].shared.source_length == (uint64_t)strlen(token),
             "SR10 exact radio-group anchor identity and order");
        arbor_view0_native_diagnostic materialized = {0};
        arbor_view0_native_v1n2_g10_materialize_anchor(
            &anchors[index], index, &materialized);
        need(strcmp(materialized.symbolic_name,
                 "ARBOR_VIEW_V1_G10_CONSTRAINT_VALIDATION_SEMANTICS") == 0 &&
             materialized.byte_offset == offset &&
             materialized.source_length == (uint64_t)strlen(token) &&
             materialized.discovery_sequence == index,
             "SR10 exact materialized G10-R12 identity");
    }

    arbor_view0_native_diagnostic diagnostics[64] = {{0}};
    arbor_view0_native_result result = {0};
    need(arbor_view0_native_check(span, diagnostics, UINT64_C(64), &result).native == 0,
         "SR10 integrated check");
    need(count_rule_id(diagnostics, result.diagnostic_count,
             ARBOR_VIEW0_NATIVE_RULE_ID_V1N2_G10_R12) == expected_g10_r12,
         "SR10 exact integrated G10-R12 count");
    (void)id;
}

static void check_sr10_radio_group_required_propagation(void) {
    static const char *const required_then_name[] = {"required", "name"};
    static const size_t required_then_name_occurrences[] = {0u, 1u};
    static const char *const name_then_required[] = {"name", "required"};
    static const size_t name_then_required_occurrences[] = {0u, 0u};
    static const char *const three_member[] = {"name", "required", "name"};
    static const size_t three_member_occurrences[] = {0u, 0u, 2u};
    static const char *const two_required[] = {"required", "required"};
    static const size_t two_required_occurrences[] = {0u, 1u};
    static const char *const second_name[] = {"name"};
    static const size_t second_name_occurrences[] = {1u};
    static const char *const local_required[] = {"required"};
    static const size_t local_required_occurrences[] = {0u};

    check_sr10_case("S1001",
        "<input type=radio name=x required><input type=radio name=x>",
        2u, required_then_name, required_then_name_occurrences);
    check_sr10_case("S1002",
        "<input type=radio name=x><input type=radio name=x required>",
        2u, name_then_required, name_then_required_occurrences);
    check_sr10_case("S1003",
        "<input type=radio name=x><input type=radio name=x required>"
        "<input type=radio name=x>",
        3u, three_member, three_member_occurrences);
    check_sr10_case("S1004",
        "<input type=radio name=x required><input type=radio name=x required>",
        2u, two_required, two_required_occurrences);
    check_sr10_case("S1005",
        "<input type=radio name=x required disabled><input type=radio name=x>",
        1u, second_name, second_name_occurrences);
    check_sr10_case("S1006",
        "<form><fieldset disabled><input type=radio name=x required></fieldset>"
        "<input type=radio name=x></form>",
        1u, second_name, second_name_occurrences);
    check_sr10_case("S1007",
        "<input type=radio name=x required><input type=radio name=x checked>",
        0u, NULL, NULL);
    check_sr10_case("S1008",
        "<input type=radio name=x required>"
        "<input type=radio name=x checked disabled>",
        0u, NULL, NULL);
    check_sr10_case("S1009-name",
        "<input type=radio name=x required><input type=radio name=y>",
        1u, local_required, local_required_occurrences);
    check_sr10_case("S1009-owner",
        "<form id=a></form><form id=b></form>"
        "<input type=radio form=a name=x required>"
        "<input type=radio form=b name=x>",
        1u, local_required, local_required_occurrences);
    check_sr10_case("S1010",
        "<form id=a></form><input type=radio form=a name=x required>"
        "<input type=radio form=a name=x>",
        2u, required_then_name, required_then_name_occurrences);
}

int main(void) {
    static const fixture_case rm0_cases[] = {
        {"T027", "<form id=f></form><form id=f></form><input form=f name=x>", 1u, false},
        {"T028", "<div id=f></div><form id=f></form><input form=f name=x>", 1u, true},
        {"T029", "<input form=missing name=x>", 1u, true},
        {"T030", "<form><input name=x></form>", 1u, false},
        {"T031", "<form accept-charset=UTF-8></form>", 1u, false},
        {"T032", "<form accept-charset=iso-8859-1></form>", 1u, true},
        {"T033", "<label for=x>x</label><input id=x name=x>", 2u, false},
        {"T034", "<label for=x>x</label><div id=x></div>", 2u, true},
        {"T035", "<label><input name=x></label>", 2u, false},
        {"T036", "<label for=x>a</label><label for=x>b</label><input id=x name=x>", 2u, false},
        {"T037", "<input type=number min=10 max=2 name=x>", 3u, true},
        {"T038", "<input type=number step=0 name=x>", 3u, true},
        {"T039", "<form><input type=radio name=x checked><input type=radio name=x checked></form>", 3u, true},
        {"T040", "<div id=l></div><datalist id=l></datalist><input list=l name=x>", 3u, true},
        {"T041", "<input name=x pattern='[' value=x>", 3u, false},
        {"T042", "<input name=x placeholder='a&#10;b'>", 3u, true},
        {"T043", "<form id=f></form><input type=submit form=f formaction=/x>", 3u, false},
        {"T044", "<input type=text name=x formaction=/x>", 3u, false},
        {"T045", "<input type=image name=x src=x>", 3u, true},
        {"T046", "<form id=f></form><button form=f formaction=/x>x</button>", 4u, false},
        {"T047", "<button type=button formaction=/x>x</button>", 4u, true},
        {"T048", "<button type=button commandfor=missing>x</button>", 4u, true},
        {"T049", "<select required name=x><option value='' selected></option></select>", 5u, false},
        {"T050", "<select name=x><option selected>a</option><option selected>b</option></select>", 5u, true},
        {"T051", "<datalist><option value=x></option></datalist>", 5u, false},
        {"T052", "<select name=x><optgroup label=x><option>x</option></optgroup></select>", 5u, false},
        {"T053", "<textarea name=x minlength=5 maxlength=2></textarea>", 6u, true},
        {"T054", "<textarea name=x rows=0></textarea>", 6u, false},
        {"T055", "<textarea name=x wrap=hard></textarea>", 6u, false},
        {"T056", "<textarea name=x placeholder='a&#10;b'></textarea>", 6u, true},
        {"T057", "<input id=x name=x><output for='x x'></output>", 7u, false},
        {"T058", "<output for=missing></output>", 7u, true},
        {"T059", "<progress value=2 max=1></progress>", 8u, false},
        {"T060", "<progress value=0 max=0></progress>", 8u, false},
        {"T061", "<meter value=2 min=2 max=1></meter>", 8u, false},
        {"T062", "<meter value=2 min=1 max=3 low=3 high=2></meter>", 8u, false},
        {"T063", "<meter value=2 min=1 max=3 optimum=4></meter>", 8u, false},
        {"T064", "<fieldset disabled><legend><input required value=x name=x></legend></fieldset>", 9u, false},
        {"T065", "<select name=x><button><span><selectedcontent></selectedcontent></span></button></select>", 10u, false},
        {"T066", "<select name=x><button><selectedcontent>x</selectedcontent></button></select>", 10u, true},
        {"T067", "<input name=x autofocus><button autofocus>x</button>", 11u, true},
        {"T068", "<input name=x dirname=''>", 11u, true},
        {"T069", "<input name=x autocomplete='section-a shipping email'>", 11u, false},
        {"T070", "<input type=checkbox name=x required>", 12u, true},
        {"T071", "<input type=radio name=x required>", 12u, true},
        {"T072", "<select required name=x><option value='' selected></option></select>", 12u, true},
        {"T073", "<input type=url name=x value='not a url'>", 12u, true},
        {"T074", "<input type=email name=x value='a..b@example.com'>", 12u, true},
        {"T075", "<input type=number name=x value=1 min=2>", 12u, true},
        {"T076", "<input type=date name=x value=2026-12-01 max=2026-01-01>", 12u, true},
        {"T077", "<input type=number name=x value=3 min=0 step=2>", 12u, true},
        {"T078", "<input name=x value=x>", 12u, false},
        {"T079", "<form enctype=multipart/form-data></form>", 13u, true},
        {"T080", "<form><input type=hidden name=_charset_></form>", 13u, false},
        {"T081", "<form><input value=x></form>", 13u, true},
        {"T082", "<form><input type=file name=x></form>", 13u, false},
        {"T083", "<form method=post enctype=multipart/form-data><input name=x></form>", 13u, false},
        {"T084", "<form action=https://example.test/ method=post><input name=x></form>", 13u, false}
    };
    static const fixture_case cr1_cases[] = {
        {"step-mismatch-number", "<input type=number value=3 min=0 step=2 name=x>", 12u, true},
        {"date-min-greater-max", "<input type=date min=2026-12-01 max=2026-01-01 name=x>", 3u, true},
        {"file-invalid-accept", "<input type=file accept=bogus name=x>", 3u, true},
        {"successful-control-missing-name", "<form><input value=x></form>", 13u, true},
        {"fieldset-disabled-required", "<fieldset disabled><input required></fieldset>", 12u, false},
        {"datalist-descendant-required", "<datalist><input required></datalist>", 12u, false},
        {"readonly-checkbox-required", "<input type=checkbox readonly required name=x>", 12u, true},
        {"text-inapplicable-min-max", "<input type=text min=10 max=2 name=x>", 3u, false},
        {"invalid-button-type-override", "<button type=bogus formaction=/x>x</button>", 4u, false},
        {"email-invalid-dot-atom", "<input type=email value='a..b@example.com' name=x>", 12u, true},
        {"default-get-multipart", "<form enctype=multipart/form-data></form>", 13u, true},
        {"submit-override-without-owner", "<input type=submit formaction=/x>", 11u, true},
        {"optgroup-missing-label", "<select name=x><optgroup><option>x</option></optgroup></select>", 5u, true},
        {"optgroup-placeholder", "<select required name=x><optgroup label=x><option value=''></option></optgroup></select>", 5u, true},
        {"image-missing-src", "<input type=image name=x alt=x>", 3u, true}
    };
    static const fixture_case complete_closure_cases[] = {
        {"C1101-required-select-missing-placeholder",
            "<select required name=x><option value=x selected>x</option></select>", 5u, true},
        {"C1102-required-select-direct-placeholder",
            "<select required name=x><option value='' selected></option></select>", 5u, false},
        {"C1103-time-reversed-valid-value-r3",
            "<input type=time name=x min=23:00 max=01:00 value=00:30>", 3u, false},
        {"C1103-time-reversed-valid-value-r12",
            "<input type=time name=x min=23:00 max=01:00 value=00:30>", 12u, false},
        {"C1104-time-reversed-gap-r3",
            "<input type=time name=x min=23:00 max=01:00 value=12:00>", 3u, false},
        {"C1104-time-reversed-gap-r12",
            "<input type=time name=x min=23:00 max=01:00 value=12:00>", 12u, true},
        {"C1105-form-empty-name", "<form name=''></form>", 1u, true},
        {"C1106-form-duplicate-name", "<form name=x></form><form name=x></form>", 1u, true},
        {"C1107-form-unique-names", "<form name=x></form><form name=y></form>", 1u, false},
        {"C1108-optgroup-legend-label", "<select name=x><optgroup><legend>x</legend>"
            "<option>x</option></optgroup></select>", 5u, false},
        {"C1109-optgroup-missing-label", "<select name=x><optgroup>"
            "<option>x</option></optgroup></select>", 5u, true},
        {"C1110-option-empty-label", "<select name=x><option label=''>x</option></select>", 5u, true},
        {"C1111-option-nonempty-label", "<select name=x><option label=x>x</option></select>", 5u, false},
        {"C1112-hidden-autocomplete-on", "<input type=hidden name=x autocomplete=on>", 11u, true},
        {"C1113-hidden-autocomplete-field", "<input type=hidden name=x "
            "autocomplete=transaction-currency>", 11u, false},
        {"C1114-autocomplete-contact-order", "<input type=tel name=x autocomplete='tel home'>", 11u, true},
        {"C1115-autocomplete-valid-contact", "<input type=tel name=x autocomplete='home tel'>", 11u, false},
        {"C1116-autocomplete-inappropriate-number", "<input type=number name=x autocomplete=name>", 11u, true},
        {"C1117-autocomplete-valid-number", "<input type=number name=x "
            "autocomplete=transaction-amount>", 11u, false},
        {"C1118-autocomplete-full-order", "<input type=tel name=x "
            "autocomplete='section-a billing home tel'>", 11u, false},
        {"C1119-autocomplete-webauthn-not-last", "<input name=x autocomplete='webauthn username'>", 11u, true},
        {"C1120-autocomplete-on-with-field", "<input name=x autocomplete='on name'>", 11u, true},
        {"C1121-autocomplete-unknown", "<input name=x autocomplete=unknown-token>", 11u, true},
        {"C1122-empty-formaction", "<form id=f method=post></form>"
            "<button form=f formaction=''>x</button>", 13u, true},
        {"C1123-formenctype-effective-get", "<form id=f method=get></form>"
            "<button form=f formenctype=multipart/form-data>x</button>", 13u, true},
        {"C1124-formenctype-effective-post", "<form id=f method=get></form>"
            "<button form=f formmethod=post formenctype=multipart/form-data>x</button>", 13u, false},
        {"C1125-invalid-form-method", "<form method=bogus></form>", 1u, true},
        {"C1126-invalid-form-enctype", "<form method=post enctype=bogus></form>", 1u, true},
        {"C1127-invalid-form-target", "<form target=_bad></form>", 1u, true},
        {"C1128-valid-form-target", "<form target=_blank></form>", 1u, false},
        {"C1129-reserved-control-name", "<input name=isindex>", 11u, true},
        {"C1130-empty-output-name", "<output name=''></output>", 11u, true},
        {"C1131-duplicate-accept-token", "<input type=file name=x accept='.png,.PNG'>", 3u, true},
        {"C1132-invalid-image-url", "<input type=image name=x alt=x src='x%'>", 3u, true},
        {"C1133-multiline-autocomplete-on-text", "<input name=x autocomplete=street-address>", 11u, true},
        {"C1134-multiline-autocomplete-on-textarea", "<textarea name=x "
            "autocomplete=street-address></textarea>", 11u, false}
    };
    check_input_states();
    check_sr2_exact_arithmetic();
    check_sr3_select_placeholder_semantics();
    check_sr4_prior_owner_semantics();
    check_sr5_r2_integrated_prior_owner_semantics();
    check_sr6_progress_meter_prior_owner_semantics();
    check_sr7_selectedcontent_provenance_reconciliation();
    check_sr8_label_descendant_reconciliation();
    check_sr9_autofocus_scoping_root_reconciliation();
    check_sr10_radio_group_required_propagation();
    for (size_t index = 0u; index < sizeof(rm0_cases) / sizeof(rm0_cases[0]); ++index)
        check_fixture(&rm0_cases[index]);
    for (size_t index = 0u; index < sizeof(cr1_cases) / sizeof(cr1_cases[0]); ++index)
        check_fixture(&cr1_cases[index]);
    for (size_t index = 0u;
         index < sizeof(complete_closure_cases) / sizeof(complete_closure_cases[0]); ++index)
        check_fixture(&complete_closure_cases[index]);
    for (size_t rule = 1u; rule <= 13u; ++rule) {
        bool seen = false;
        for (size_t index = 0u; index < sizeof(rm0_cases) / sizeof(rm0_cases[0]); ++index)
            if (rm0_cases[index].rule == rule && rm0_cases[index].present) seen = true;
        if (rule == 9u)
            seen = evaluate_body("<fieldset name=''></fieldset>")
                .rule_violation_count[rule - 1u] != 0u;
        if (rule == 10u)
            seen = evaluate_body(
                "<select name=x><button><selectedcontent>x</selectedcontent></button></select>")
                .rule_violation_count[rule - 1u] != 0u;
        if (rule == 8u)
            seen = evaluate_body("<meter></meter>")
                .rule_violation_count[rule - 1u] != 0u;
        need(seen, "each G10 rule has an executable positive case");
        (void)printf("PASS G10-R%zu\n", rule);
    }
    puts("VIEW0_V1N2_G10_SR1_CR1_BLOCKERS=15_OF_15_CORRECTED");
    puts("VIEW0_V1N2_G10_SR1_FUNCTIONAL_CASES=T001_T084_EXECUTED");
    puts("VIEW0_V1N2_G10_SR2_EXACT_ARITHMETIC_CASES=S201_S213_EXECUTED");
    puts("VIEW0_V1N2_G10_SR3_SELECT_PLACEHOLDER_CASES=S301_S310_EXECUTED");
    puts("VIEW0_V1N2_G10_SR4_PRIOR_OWNER_CASES=S401_S410_EXECUTED");
    puts("VIEW0_V1N2_G10_SR5_R2_INTEGRATED_PRIOR_OWNER_CASES=S5R101_S5R112_EXECUTED");
    puts("VIEW0_V1N2_G10_SR6_PROGRESS_METER_PRIOR_OWNER_CASES=S601_S612_EXECUTED");
    puts("VIEW0_V1N2_G10_SR7_SELECTEDCONTENT_PROVENANCE_CASES=S701_S712_EXECUTED");
    puts("VIEW0_V1N2_G10_SR8_LABEL_DESCENDANT_CASES=S801_S814_EXECUTED");
    puts("VIEW0_V1N2_G10_SR9_AUTOFOCUS_SCOPING_ROOT_CASES=S901_S914_EXECUTED");
    puts("VIEW0_V1N2_G10_SR10_RADIO_GROUP_REQUIRED_PROPAGATION_CASES=S1001_S1014_EXECUTED");
    puts("VIEW0_V1N2_G10_COMPLETE_CLOSURE_CASES=C1101_C1134_EXECUTED");
    puts("VIEW0_V1N2_G10_RM0_TEST_MATRIX=98_OF_98_EXECUTABLE_BOUND");
    puts("VIEW0_V1N2_G10_RULES=13_OF_13");
    puts("PASS: VIEW0 V1N2 G10 exhaustive corrected static form semantics closure");
    return 0;
}
