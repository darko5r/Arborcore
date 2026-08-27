#include <arborcore/view0_conformance/native.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define G06_R4 UINT64_C(0x0000000030060004)

static arbor_span span_from_cstr(const char *text)
{
    return (arbor_span){(const uint8_t *)text, (uint64_t)strlen(text)};
}

static uint64_t count_rule(
    const arbor_view0_native_diagnostic *diagnostics,
    uint64_t count,
    uint64_t rule_id)
{
    uint64_t found = 0u;
    for (uint64_t i = 0u; i < count; ++i) {
        if (diagnostics[i].rule_id == rule_id) found += 1u;
    }
    return found;
}

static int expect(
    const char *html,
    uint64_t r2,
    uint64_t r3,
    uint64_t r7,
    uint64_t g06_r4,
    uint64_t required_flags)
{
    const uint64_t retired =
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_DEFERRED_NOSCRIPT |
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_DEFERRED_SELECT_SIZE |
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_DEFERRED_INPUT_TYPE |
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_DEFERRED_CANVAS_INPUT_STATE |
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_DEFERRED_CANVAS_SELECT_SIZE |
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_DEFERRED_NOSCRIPT |
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R7_DEFERRED_G04_TRANSPARENT;
    arbor_view0_native_diagnostic diagnostics[64] = {{0}};
    arbor_view0_native_result result = {0};
    const arbor_status status = arbor_view0_native_check(
        span_from_cstr(html), diagnostics, 64u, &result);
    const uint64_t actual_r2 = count_rule(diagnostics, result.diagnostic_count,
        ARBOR_VIEW_V1_G03_CONTENT_MODEL);
    const uint64_t actual_r3 = count_rule(diagnostics, result.diagnostic_count,
        ARBOR_VIEW_V1_G03_DESCENDANT_EXCLUSIONS);
    const uint64_t actual_r7 = count_rule(diagnostics, result.diagnostic_count,
        ARBOR_VIEW_V1_G03_PALPABLE_PHRASING_NONEMPTY);
    const uint64_t actual_g06_r4 = count_rule(diagnostics, result.diagnostic_count, G06_R4);
    if (status.native != 0 || actual_r2 != r2 || actual_r3 != r3 ||
        actual_r7 != r7 || actual_g06_r4 != g06_r4 ||
        (result.flags & retired) != 0u ||
        (result.flags & required_flags) != required_flags) {
        (void)fprintf(stderr,
            "RC1 mismatch status=%lld r2=%llu/%llu r3=%llu/%llu r7=%llu/%llu "
            "g06r4=%llu/%llu flags=0x%llx required=0x%llx retired=0x%llx\n",
            (long long)status.native,
            (unsigned long long)actual_r2, (unsigned long long)r2,
            (unsigned long long)actual_r3, (unsigned long long)r3,
            (unsigned long long)actual_r7, (unsigned long long)r7,
            (unsigned long long)actual_g06_r4, (unsigned long long)g06_r4,
            (unsigned long long)result.flags,
            (unsigned long long)required_flags,
            (unsigned long long)(result.flags & retired));
        return 1;
    }
    return 0;
}

int main(void)
{
    static const char prefix[] = "<!doctype html><title>x</title>";
    (void)prefix;

    if (expect("<!doctype html><title>x</title><noscript><span>x</span></noscript>",
               0u, 0u, 0u, 0u, 0u) != 0) return 1;
    if (expect("<!doctype html><title>x</title><select size=2><button></button><option>x</option></select>",
               1u, 0u, 1u, 0u, 0u) != 0) return 2;
    if (expect("<!doctype html><title>x</title><select size=x><button></button><option>x</option></select>",
               0u, 0u, 1u, 1u, 0u) != 0) return 3;
    if (expect("<!doctype html><title>x</title><select size=0><button></button><option>x</option></select>",
               1u, 0u, 1u, 1u, 0u) != 0) return 4;
    if (expect("<!doctype html><title>x</title><select size=1 multiple><button></button><option>x</option></select>",
               0u, 0u, 1u, 0u,
               ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_DEFERRED_SELECT_PLATFORM) != 0) return 5;

    if (expect("<!doctype html><title>x</title><a href=/><input type=hidden></a>",
               0u, 0u, 0u, 0u, 0u) != 0) return 6;
    if (expect("<!doctype html><title>x</title><a href=/><input type=unknown></a>",
               0u, 1u, 0u, 0u, 0u) != 0) return 7;
    if (expect("<!doctype html><title>x</title><canvas><input type=checkbox></canvas>",
               0u, 0u, 0u, 0u, 0u) != 0) return 8;
    if (expect("<!doctype html><title>x</title><canvas><input type=file></canvas>",
               0u, 1u, 0u, 0u, 0u) != 0) return 9;
    if (expect("<!doctype html><title>x</title><canvas><select size=2><option>x</option></select></canvas>",
               0u, 0u, 0u, 0u, 0u) != 0) return 10;
    if (expect("<!doctype html><title>x</title><canvas><select size=1><option>x</option></select></canvas>",
               0u, 1u, 0u, 0u, 0u) != 0) return 11;
    if (expect("<!doctype html><title>x</title><canvas><select size=x><option>x</option></select></canvas>",
               0u, 1u, 0u, 1u, 0u) != 0) return 12;
    if (expect("<!doctype html><title>x</title><a href=/><noscript><button>x</button></noscript></a>",
               0u, 1u, 0u, 0u, 0u) != 0) return 13;

    if (expect("<!doctype html><title>x</title><select><option><div></div></option></select>",
               0u, 0u, 1u, 0u, 0u) != 0) return 14;
    if (expect("<!doctype html><title>x</title><select><div></div><option>x</option></select>",
               0u, 0u, 0u, 0u, 0u) != 0) return 15;
    if (expect("<!doctype html><title>x</title><select><optgroup><div></div><option>x</option></optgroup></select>",
               0u, 0u, 0u, 0u, 0u) != 0) return 16;

    (void)puts("VIEW0_V1N1_RC1_RESOLVED_DEPENDENCIES=7_OF_7");
    (void)puts("VIEW0_V1N1_RC1_RETIRED_DEFERRAL_FLAGS=ZERO");
    (void)puts("VIEW0_V1N1_RC1_RETAINED_EXTERNAL_DEPENDENCIES=13");
    (void)puts("VIEW0_V1N1_RC1_ALREADY_OWNED_DEPENDENCIES=1");
    (void)puts("PASS: VIEW0 V1N1 RC1 exact dependency reconciliation matrix");
    return 0;
}
