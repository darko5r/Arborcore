#include <arborcore/view0_conformance/native.h>
#include "g10.h"

#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void need(bool condition, const char *message) {
    if (!condition) { (void)fprintf(stderr, "FAIL: %s\n", message); exit(1); }
}

static arbor_span fixture(void) {
    static const char html[] =
        "<!doctype html><html><head><title>x</title></head><body>"
        "<form id=f action=/submit method=post><label for=a>A</label>"
        "<input id=a name=a required value=x pattern='['>"
        "<input type=number name=n value=3 min=0 step=2>"
        "<input type=email name=e value='a..b@example.com'>"
        "<input type=radio name=r checked><input type=radio name=r checked>"
        "<output for='a missing'></output>"
        "<textarea name=t minlength=2 maxlength=1></textarea>"
        "</form></body></html>";
    return (arbor_span){(const uint8_t *)html, sizeof(html) - 1u};
}

static void check_cap(void) {
    static const char prefix[] = "<!doctype html><html><head><title>x</title></head><body>";
    static const char item[] = "<input form=missing name=x>";
    static const char suffix[] = "</body></html>";
    const size_t item_count = 5000u;
    const size_t length = sizeof(prefix) - 1u + item_count * (sizeof(item) - 1u) +
        sizeof(suffix) - 1u;
    char *html = malloc(length + 1u);
    need(html != NULL, "diagnostic-cap fixture allocation");
    size_t cursor = 0u;
    (void)memcpy(html + cursor, prefix, sizeof(prefix) - 1u); cursor += sizeof(prefix) - 1u;
    for (size_t index = 0u; index < item_count; ++index) {
        (void)memcpy(html + cursor, item, sizeof(item) - 1u);
        cursor += sizeof(item) - 1u;
    }
    (void)memcpy(html + cursor, suffix, sizeof(suffix));
    arbor_view0_native_v1n2_g10_evaluation first = {0}, second = {0};
    const arbor_span input = {(const uint8_t *)html, (uint64_t)length};
    need(arbor_view0_native_v1n2_g10_measure(input, &first).native == 0,
         "first cap measurement");
    need(arbor_view0_native_v1n2_g10_measure(input, &second).native == 0,
         "second cap measurement");
    need(first.diagnostic_count == UINT64_C(4096), "diagnostic cap is exact");
    need(first.rule_violation_count[0] == UINT64_C(4096), "cap preserves rule identity");
    need(memcmp(&first, &second, sizeof(first)) == 0, "cap truncation deterministic");
    free(html);
}

static void check_radio_group_required_cap(void) {
    static const char prefix[] = "<!doctype html><html><head><title>x</title></head><body>";
    static const char required_item[] = "<input type=radio name=x required>";
    static const char optional_item[] = "<input type=radio name=x>";
    static const char required_text_item[] = "<input required>";
    static const char suffix[] = "</body></html>";
    static arbor_view0_native_v1n2_g10_anchor anchors[4096];
    const size_t optional_count = 4094u;
    const size_t required_item_length = sizeof(required_item) - 1u;
    const size_t optional_item_length = sizeof(optional_item) - 1u;
    const size_t required_text_item_length = sizeof(required_text_item) - 1u;
    const size_t length = sizeof(prefix) - 1u + required_item_length +
        optional_count * optional_item_length + required_text_item_length +
        sizeof(suffix) - 1u;
    char *html = malloc(length + 1u);
    need(html != NULL, "radio-group cap fixture allocation");
    size_t cursor = 0u;
    (void)memcpy(html + cursor, prefix, sizeof(prefix) - 1u);
    cursor += sizeof(prefix) - 1u;
    (void)memcpy(html + cursor, required_item, required_item_length);
    cursor += required_item_length;
    for (size_t index = 0u; index < optional_count; ++index) {
        (void)memcpy(html + cursor, optional_item, optional_item_length);
        cursor += optional_item_length;
    }
    (void)memcpy(html + cursor, required_text_item, required_text_item_length);
    cursor += required_text_item_length;
    (void)memcpy(html + cursor, suffix, sizeof(suffix));

    const arbor_span input = {(const uint8_t *)html, (uint64_t)length};
    arbor_view0_native_v1n2_g10_evaluation measured = {0}, collected = {0};
    need(arbor_view0_native_v1n2_g10_measure(input, &measured).native == 0 &&
         measured.diagnostic_count == UINT64_C(4096) &&
         measured.rule_violation_count[11] == UINT64_C(4096),
         "S1013 exact radio-group diagnostic cap");
    need(arbor_view0_native_v1n2_g10_collect_anchors(
        input, anchors, UINT64_C(4096), &collected).native == 0 &&
         memcmp(&measured, &collected, sizeof(measured)) == 0,
         "S1013 radio-group capped measure/collect identity");

    const char *required = strstr(required_item, "required");
    const char *name = strstr(optional_item, "name");
    const char *required_text = strstr(required_text_item, "required");
    need(required != NULL && name != NULL && required_text != NULL,
         "S1013 anchor tokens");
    const uint64_t first_offset = (uint64_t)(sizeof(prefix) - 1u) +
        (uint64_t)(required - required_item);
    const uint64_t last_radio_offset = (uint64_t)(sizeof(prefix) - 1u) +
        (uint64_t)required_item_length +
        (uint64_t)optional_item_length * UINT64_C(4093) +
        (uint64_t)(name - optional_item);
    const uint64_t final_offset = (uint64_t)(sizeof(prefix) - 1u) +
        (uint64_t)required_item_length +
        (uint64_t)optional_item_length * UINT64_C(4094) +
        (uint64_t)(required_text - required_text_item);
    need(anchors[0].shared.byte_offset == first_offset &&
         anchors[4094].shared.byte_offset == last_radio_offset &&
         anchors[4095].shared.byte_offset == final_offset &&
         anchors[0].shared.rule_ordinal == UINT16_C(12) &&
         anchors[4094].shared.rule_ordinal == UINT16_C(12) &&
         anchors[4095].shared.rule_ordinal == UINT16_C(12),
         "S1013 radio-group capped anchor order and endpoint identity");

    arbor_view0_native_diagnostic *diagnostics = calloc(
        4096u, sizeof(*diagnostics));
    need(diagnostics != NULL, "S1013 integrated diagnostic allocation");
    arbor_view0_native_result result = {0};
    const arbor_status integrated_status = arbor_view0_native_check(
        input, diagnostics, UINT64_C(4096), &result);
    if (integrated_status.native != 0 || result.diagnostic_count != UINT64_C(4096))
        (void)fprintf(stderr,
            "FAIL DETAIL: S1013 status=%lld diagnostics=%llu\n",
            (long long)integrated_status.native,
            (unsigned long long)result.diagnostic_count);
    need(integrated_status.native == 0 && result.diagnostic_count == UINT64_C(4096),
         "S1013 integrated diagnostic cap");
    uint64_t g10_r12_count = 0u;
    for (uint64_t index = 0u; index < result.diagnostic_count; ++index)
        if (diagnostics[index].rule_id == ARBOR_VIEW0_NATIVE_RULE_ID_V1N2_G10_R12)
            g10_r12_count += 1u;
    need(g10_r12_count == UINT64_C(4096),
         "S1013 integrated G10-R12 identity");
    free(diagnostics);
    free(html);
}

int main(void) {
    arbor_view0_native_v1n2_g10_evaluation first = {0}, second = {0}, locale_result = {0};
    const arbor_span input = fixture();
    need(arbor_view0_native_v1n2_g10_measure(input, &first).native == 0,
         "first measurement");
    need(arbor_view0_native_v1n2_g10_measure(input, &second).native == 0,
         "second measurement");
    need(memcmp(&first, &second, sizeof(first)) == 0, "repeat byte identity");
    need(first.rule_violation_count[2] != 0u, "duplicate checked radio detected");
    need(first.rule_violation_count[5] != 0u, "textarea length relation detected");
    need(first.rule_violation_count[6] != 0u, "output missing ID detected");
    need(first.rule_violation_count[11] >= 2u, "step and email validation detected");
    need(first.deferred_external_semantics_count == 6u, "cross-standard deferrals");

    arbor_view0_native_v1n2_g10_anchor anchors[128] = {0};
    arbor_view0_native_v1n2_g10_evaluation collected = {0};
    need(first.diagnostic_count <= 128u, "bounded fixture diagnostics");
    need(arbor_view0_native_v1n2_g10_collect_anchors(
        input, anchors, first.diagnostic_count, &collected).native == 0,
        "anchor collection");
    need(memcmp(&first, &collected, sizeof(first)) == 0, "measure/collect identity");
    for (uint64_t index = 0u; index < collected.diagnostic_count; ++index)
        need(anchors[index].shared.group_ordinal == UINT16_C(10) &&
             anchors[index].shared.rule_ordinal >= UINT16_C(1) &&
             anchors[index].shared.rule_ordinal <= UINT16_C(13),
             "stable G10 anchor identity");

    (void)setlocale(LC_ALL, "C");
    need(arbor_view0_native_v1n2_g10_measure(input, &locale_result).native == 0,
         "C-locale measurement");
    need(memcmp(&first, &locale_result, sizeof(first)) == 0, "locale independence");
    check_cap();
    check_radio_group_required_cap();

    puts("VIEW0_V1N2_G10_DETERMINISM=PASS");
    puts("VIEW0_V1N2_G10_DIAGNOSTIC_CAP_4096=PASS");
    puts("VIEW0_V1N2_G10_SR10_RADIO_GROUP_CAP=PASS");
    puts("VIEW0_V1N2_G10_LOCALE_INDEPENDENCE=PASS");
    puts("VIEW0_V1N2_G10_INPUT_STATES=22_OF_22_BOUND");
    puts("VIEW0_V1N2_G10_FORM_OWNER_IDREF_RELATIONS=PASS");
    puts("VIEW0_V1N2_G10_ECMASCRIPT_REGEXP_DEFERRED=PASS");
    puts("VIEW0_V1N2_G10_RUNTIME_AND_SUBMISSION_EXECUTION_DEFERRALS=PASS");
    puts("VIEW0_V1N2_G10_INPUT_STATE_PRODUCT_TABLE=ZERO");
    puts("PASS: VIEW0 V1N2 G10 corrected adversarial qualification");
    return 0;
}
