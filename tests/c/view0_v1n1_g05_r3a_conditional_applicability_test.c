#include <arborcore/view0_conformance/native.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int count_rule(const char *html, uint64_t rule, uint64_t *offset_out, uint64_t *length_out)
{
    arbor_view0_native_diagnostic diagnostics[128] = {{0}};
    arbor_view0_native_result result = {0};
    const arbor_span input = {(const uint8_t *)html, (uint64_t)strlen(html)};
    const arbor_status status = arbor_view0_native_check(input, diagnostics, UINT64_C(128), &result);
    if (status.native != 0) return -1;
    uint64_t count = 0u;
    for (uint64_t i = 0u; i < result.diagnostic_count; ++i) {
        if (diagnostics[i].rule_id != rule) continue;
        if (count == 0u && offset_out != NULL && length_out != NULL) {
            *offset_out = diagnostics[i].byte_offset;
            *length_out = diagnostics[i].source_length;
        }
        count += 1u;
    }
    return (int)count;
}

static int require_clean(const char *html, int code)
{
    return count_rule(html, ARBOR_VIEW_V1_G05_CONDITIONAL_ATTRIBUTE_APPLICABILITY,
                      NULL, NULL) == 0 ? 0 : code;
}

int main(void)
{
    static const char positive[] =
        "<!doctype html><title>x</title><body><a href=\"/\" target=\"_blank\">x</a></body>";
    static const char negative[] =
        "<!doctype html><title>x</title><body><a target=\"_blank\">x</a></body>";
    uint64_t offset = 0u, length = 0u;
    if (require_clean(positive, 10) != 0) return 10;
    if (count_rule(negative, ARBOR_VIEW_V1_G05_CONDITIONAL_ATTRIBUTE_APPLICABILITY,
                   &offset, &length) != 1) return 11;
    const char *target = strstr(negative, "target");
    if (target == NULL || offset != (uint64_t)(target - negative) || length != UINT64_C(6))
        return 12;

    /* Missing/invalid input type defaults are both Text; maxlength remains applicable. */
    if (require_clean("<!doctype html><title>x</title><body><input maxlength=\"5\"></body>", 13) != 0)
        return 13;
    if (require_clean("<!doctype html><title>x</title><body><input type=\"not-a-state\" maxlength=\"5\"></body>", 14) != 0)
        return 14;

    /* R3 evaluates applicability only; later value-language owners remain deferred. */
    if (require_clean("<!doctype html><head><title>x</title><link href=\"/x\" rel=\"modulepreload\" as=\"not-a-destination\"></head><body></body>", 15) != 0)
        return 15;
    if (require_clean("<!doctype html><title>x</title><body><meter value=\"not-a-number\">x</meter></body>", 16) != 0)
        return 16;

    /* Cross-element/context predicates. */
    if (require_clean("<!doctype html><title>x</title><body><a href=\"/\"><img alt=\"x\" ismap></a></body>", 17) != 0)
        return 17;
    if (require_clean("<!doctype html><title>x</title><body><img alt=\"x\" controls></body>", 18) != 0)
        return 18;
    if (require_clean("<!doctype html><title>x</title><body><select><optgroup><legend>x</legend><option>y</option></optgroup></select></body>", 19) != 0)
        return 19;
    if (require_clean("<!doctype html><title>x</title><body><textarea wrap=\"hard\" cols=\"20\"></textarea></body>", 20) != 0)
        return 20;

    /* Earlier and later G05 owners must not be duplicated by R3. */
    if (require_clean("<!doctype html><title>x</title><body><p bogus=\"x\">x</p></body>", 21) != 0)
        return 21;
    if (require_clean("<!doctype html><title>x</title><body><p href=\"/\">x</p></body>", 22) != 0)
        return 22;
    if (require_clean("<!doctype html><title>x</title><body><p onpageshow=\"x()\">x</p></body>", 23) != 0)
        return 23;
    if (require_clean("<!doctype html><title>x</title><body><x-foo target=\"_blank\">x</x-foo></body>", 24) != 0)
        return 24;

    (void)puts("VIEW0_V1N1_G05_R3A_FROZEN_MATRIX_FIXTURES=2_OF_2");
    (void)puts("VIEW0_V1N1_G05_R3A_NEGATIVE_ATTRIBUTE_ANCHOR=PASS");
    (void)puts("VIEW0_V1N1_G05_R3A_INPUT_DEFAULT_STATE=TEXT_MISSING_INVALID_PASS");
    (void)puts("VIEW0_V1N1_G05_R3A_VALUE_GRAMMAR_DEFERRAL=PASS");
    (void)puts("VIEW0_V1N1_G05_R3A_ANCESTOR_CHILD_PREDICATES=PASS");
    (void)puts("VIEW0_V1N1_G05_R3A_R1_R2_R4_G13_OWNERSHIP=PASS");
    return 0;
}
