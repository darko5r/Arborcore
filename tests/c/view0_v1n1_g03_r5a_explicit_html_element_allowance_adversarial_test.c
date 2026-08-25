#include <arborcore/view0_conformance/native.h>

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static arbor_span span_from_cstr(const char *text)
{
    return (arbor_span){(const uint8_t *)text, (uint64_t)strlen(text)};
}


static int append_text(
    uint8_t *buffer,
    uint64_t capacity,
    uint64_t *length_io,
    const char *text)
{
    if (buffer == NULL || length_io == NULL || text == NULL) return 1;
    const size_t n = strlen(text);
    if (*length_io > capacity || (uint64_t)n > capacity - *length_io) return 1;
    (void)memcpy(buffer + *length_io, text, n);
    *length_io += (uint64_t)n;
    return 0;
}

static uint64_t count_rule(
    const arbor_view0_native_diagnostic *diagnostics,
    uint64_t diagnostic_count,
    uint64_t rule_id)
{
    uint64_t count = 0u;
    for (uint64_t i = 0u; i < diagnostic_count; ++i) {
        if (diagnostics[i].rule_id == rule_id) count += 1u;
    }
    return count;
}

static int expect_ownership(
    const char *html,
    uint64_t expected_tree_errors,
    uint64_t expected_r1,
    uint64_t expected_total)
{
    arbor_view0_native_diagnostic diagnostics[64] = {{0}};
    arbor_view0_native_result result = {0};
    arbor_status status = arbor_view0_native_check(
        span_from_cstr(html), diagnostics, 64u, &result);
    if (status.native != 0) return 1;
    const uint64_t r7_count = count_rule(
        diagnostics, result.diagnostic_count, ARBOR_VIEW_V1_G03_PALPABLE_PHRASING_NONEMPTY);
    if (result.tokenizer_error_count != 0u || result.tree_error_count != expected_tree_errors ||
        result.diagnostic_count < r7_count ||
        result.diagnostic_count - r7_count != expected_total) return 2;
    if (count_rule(diagnostics, result.diagnostic_count, ARBOR_VIEW_V1_G03_ELEMENT_CONTEXT) != expected_r1 ||
        count_rule(diagnostics, result.diagnostic_count, ARBOR_VIEW_V1_G03_CONTENT_MODEL) != 0u ||
        count_rule(diagnostics, result.diagnostic_count, ARBOR_VIEW_V1_G03_DESCENDANT_EXCLUSIONS) != 0u ||
        count_rule(diagnostics, result.diagnostic_count, ARBOR_VIEW_V1_G03_NOTHING_MODEL) != 0u ||
        count_rule(diagnostics, result.diagnostic_count, ARBOR_VIEW_V1_G03_EXPLICIT_HTML_ELEMENT_ALLOWANCE) != 0u) return 3;
    return 0;
}

int main(void)
{
    const uint64_t active_partial_flags =
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R1_PARTIAL |
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R2_PARTIAL |
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R3_PARTIAL |
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R4_PARTIAL |
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R5_PARTIAL |
        ARBOR_VIEW0_NATIVE_RESULT_FLAG_G03_R7_PARTIAL;
    static const char *const clean_deferred[] = {
        "<!doctype html><title>x</title><p>x</p>",
        "<!doctype html><title>x</title><body><p>x<div>y</div></body>",
        "<!doctype html><title>x</title><body><ul><li>a<li>b</ul></body>",
        "<!doctype html><title>x</title><body><dl><dt>a<dd>b</dl></body>",
        "<!doctype html><title>x</title><body><select><option>a<option>b</select></body>",
        "<!doctype html><title>x</title><body><table><x-arbor-r5></x-arbor-r5></table></body>"
    };
    for (size_t i = 0u; i < sizeof(clean_deferred) / sizeof(clean_deferred[0]); ++i) {
        if (expect_ownership(clean_deferred[i], 0u, 0u, 0u) != 0) return (int)(10u + i);
    }
    if (expect_ownership(
            "<!doctype html><title>x</title><body><form>x<form></form></form></body>",
            2u, 0u, 2u) != 0) return 20;
    if (expect_ownership(
            "<!doctype html><title>x</title><body><h1>x<h2>y</h2></body>",
            1u, 0u, 1u) != 0) return 21;
    if (expect_ownership(
            "<!doctype html><title>x</title><body><button>x<button>y</button></body>",
            1u, 0u, 1u) != 0) return 22;
    if (expect_ownership(
            "<!doctype html><title>x</title><body><table><input type=hidden></table></body>",
            1u, 1u, 2u) != 0) return 23;
    if (expect_ownership(
            "<!doctype html><title>x</title><body><table><form></form></table></body>",
            2u, 1u, 3u) != 0) return 24;

    static const char suppressed[] =
        "<!doctype html><title>x</title><body><table><li>x</li></table></body>";
    arbor_view0_native_diagnostic suppressed_diagnostics[64] = {{0}};
    arbor_view0_native_result suppressed_result = {0};
    arbor_status status = arbor_view0_native_check(
        span_from_cstr(suppressed), suppressed_diagnostics, 64u, &suppressed_result);
    if (status.native != 0 ||
        count_rule(
            suppressed_diagnostics, suppressed_result.diagnostic_count,
            ARBOR_VIEW_V1_G03_ELEMENT_CONTEXT) != 1u ||
        count_rule(
            suppressed_diagnostics, suppressed_result.diagnostic_count,
            ARBOR_VIEW_V1_G03_EXPLICIT_HTML_ELEMENT_ALLOWANCE) != 0u) {
        return 30;
    }

    static const char foster_two[] =
        "<!doctype html><title>x</title><body><table><p>a</p><div>b</div></table></body>";
    arbor_view0_native_diagnostic ample[64] = {{0}};
    arbor_view0_native_result ample_result = {0};
    status = arbor_view0_native_check(
        span_from_cstr(foster_two), ample, 64u, &ample_result);
    if (status.native != 0 ||
        count_rule(
            ample, ample_result.diagnostic_count,
            ARBOR_VIEW_V1_G03_EXPLICIT_HTML_ELEMENT_ALLOWANCE) != 2u ||
        ample_result.diagnostic_count == 0u) {
        return 31;
    }

    arbor_view0_native_diagnostic small[64];
    arbor_view0_native_diagnostic before[64];
    (void)memset(small, 0x5a, sizeof(small));
    (void)memcpy(before, small, sizeof(small));
    arbor_view0_native_result failed = {11u, 22u, 33u, 44u};
    arbor_view0_native_result failed_before = failed;
    status = arbor_view0_native_check(
        span_from_cstr(foster_two), small,
        ample_result.diagnostic_count - 1u, &failed);
    if (status.native != -(int64_t)ENOSPC ||
        memcmp(small, before, sizeof(small)) != 0 ||
        memcmp(&failed, &failed_before, sizeof(failed)) != 0) {
        return 32;
    }

    arbor_view0_native_diagnostic exact[64] = {{0}};
    arbor_view0_native_result exact_result = {0};
    status = arbor_view0_native_check(
        span_from_cstr(foster_two), exact,
        ample_result.diagnostic_count, &exact_result);
    if (status.native != 0 ||
        memcmp(&ample_result, &exact_result, sizeof(ample_result)) != 0 ||
        memcmp(
            ample, exact,
            (size_t)(ample_result.diagnostic_count * sizeof(exact[0]))) != 0) {
        return 33;
    }

    static const uint8_t invalid_utf8[] = {
        '<','!','d','o','c','t','y','p','e',' ','h','t','m','l','>',
        '<','t','a','b','l','e','>','<','p','>',0xf0,0x80,0x80,0x80
    };
    arbor_view0_native_diagnostic utf8_diagnostics[16] = {{0}};
    arbor_view0_native_result utf8_result = {0};
    status = arbor_view0_native_check(
        (arbor_span){invalid_utf8, sizeof(invalid_utf8)},
        utf8_diagnostics, 16u, &utf8_result);
    if (status.native != 0 || utf8_result.diagnostic_count == 0u ||
        utf8_diagnostics[0].origin != (uint32_t)ARBOR_VIEW0_NATIVE_ORIGIN_UTF8 ||
        count_rule(
            utf8_diagnostics, utf8_result.diagnostic_count,
            ARBOR_VIEW_V1_G03_EXPLICIT_HTML_ELEMENT_ALLOWANCE) != 0u ||
        (utf8_result.flags & active_partial_flags) != active_partial_flags) {
        return 34;
    }

    arbor_view0_native_diagnostic empty_diagnostics[8] = {{0}};
    arbor_view0_native_result empty_result = {0};
    status = arbor_view0_native_check(
        (arbor_span){NULL, 0u}, empty_diagnostics, 8u, &empty_result);
    if (status.native != 0 || empty_result.diagnostic_count != 2u ||
        count_rule(
            empty_diagnostics, empty_result.diagnostic_count,
            ARBOR_VIEW_V1_G03_EXPLICIT_HTML_ELEMENT_ALLOWANCE) != 0u ||
        (empty_result.flags & active_partial_flags) != active_partial_flags) {
        return 36;
    }


    static uint8_t mixed_prior_overflow[262144];
    uint64_t mixed_length = 0u;
    if (append_text(
            mixed_prior_overflow, sizeof(mixed_prior_overflow), &mixed_length,
            "<!doctype html><title>x</title><body>") != 0) return 36;
    for (uint64_t i = 0u; i < UINT64_C(2100); ++i) {
        if (append_text(
                mixed_prior_overflow, sizeof(mixed_prior_overflow), &mixed_length,
                "<div><li>x</li></div>") != 0) return 37;
    }
    for (uint64_t i = 0u; i < UINT64_C(2100); ++i) {
        if (append_text(
                mixed_prior_overflow, sizeof(mixed_prior_overflow), &mixed_length,
                "<figure><figcaption>a</figcaption><figcaption>b</figcaption></figure>") != 0) return 38;
    }
    if (append_text(
            mixed_prior_overflow, sizeof(mixed_prior_overflow), &mixed_length,
            "</body>") != 0) return 39;

    static arbor_view0_native_diagnostic
        overflow_diagnostics[ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS];
    static arbor_view0_native_diagnostic
        overflow_before[ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS];
    (void)memset(overflow_diagnostics, 0xa5, sizeof(overflow_diagnostics));
    (void)memcpy(overflow_before, overflow_diagnostics, sizeof(overflow_before));
    arbor_view0_native_result overflow_result = {55u, 66u, 77u, 88u};
    const arbor_view0_native_result overflow_result_before = overflow_result;
    status = arbor_view0_native_check(
        (arbor_span){mixed_prior_overflow, mixed_length},
        overflow_diagnostics, ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS, &overflow_result);
    if (status.native != -(int64_t)ENOSPC ||
        memcmp(overflow_diagnostics, overflow_before, sizeof(overflow_before)) != 0 ||
        memcmp(&overflow_result, &overflow_result_before, sizeof(overflow_result)) != 0) {
        return 40;
    }

    (void)puts(
        "PASS: VIEW0 V1N1 G03 R5A explicit deferrals, G13 delegation, capacity failure atomicity, determinism and UTF-8 precedence");
    return 0;
}
