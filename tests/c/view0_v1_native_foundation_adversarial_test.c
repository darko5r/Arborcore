#include <arborcore/view0_conformance/native.h>

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static arbor_span span_from_cstr(const char *text)
{
    return (arbor_span){(const uint8_t *)text, (uint64_t)strlen(text)};
}

static int unchanged(const void *left, const void *right, size_t length)
{
    return memcmp(left, right, length) == 0 ? 0 : 1;
}

int main(void)
{
    static const char valid[] =
        "<!doctype html><html><head><title>x</title></head>"
        "<body><p>x</p></body></html>";
    arbor_view0_native_result valid_result = {0};
    arbor_status status = arbor_view0_native_check(
        span_from_cstr(valid), NULL, 0u, &valid_result);
    if (status.native != 0 || valid_result.diagnostic_count != 0u) {
        return 1;
    }

    static const char bad[] =
        "<!doctype html><html><head><title>x</title></head>"
        "<body><?bad></body></html>";
    arbor_view0_native_result result = {
        UINT64_C(0x1111111111111111),
        UINT64_C(0x2222222222222222),
        UINT64_C(0x3333333333333333),
        UINT64_C(0x4444444444444444)
    };
    const arbor_view0_native_result result_before = result;
    status = arbor_view0_native_check(span_from_cstr(bad), NULL, 0u, &result);
    if (status.native != -(int64_t)ENOSPC ||
        unchanged(&result, &result_before, sizeof(result)) != 0) {
        return 2;
    }

    arbor_view0_native_diagnostic diagnostic = {0};
    (void)memset(&diagnostic, 0xa5, sizeof(diagnostic));
    const arbor_view0_native_diagnostic diagnostic_before = diagnostic;
    result = result_before;
    status = arbor_view0_native_check(
        span_from_cstr(bad),
        &diagnostic,
        ARBOR_VIEW0_NATIVE_MAX_DIAGNOSTICS + 1u,
        &result);
    if (status.native != -(int64_t)E2BIG ||
        unchanged(&diagnostic, &diagnostic_before, sizeof(diagnostic)) != 0 ||
        unchanged(&result, &result_before, sizeof(result)) != 0) {
        return 3;
    }

    uint8_t *oversize = (uint8_t *)malloc(
        (size_t)(ARBOR_VIEW0_NATIVE_MAX_INPUT_BYTES + 1u));
    if (oversize == NULL) {
        return 4;
    }
    (void)memset(
        oversize,
        'x',
        (size_t)(ARBOR_VIEW0_NATIVE_MAX_INPUT_BYTES + 1u));
    result = result_before;
    status = arbor_view0_native_check(
        (arbor_span){oversize, ARBOR_VIEW0_NATIVE_MAX_INPUT_BYTES + 1u},
        &diagnostic,
        1u,
        &result);
    free(oversize);
    if (status.native != -(int64_t)EFBIG ||
        unchanged(&result, &result_before, sizeof(result)) != 0) {
        return 5;
    }

    union alias_union {
        arbor_view0_native_result result;
        arbor_view0_native_diagnostic diagnostic;
    } alias = {0};
    status = arbor_view0_native_check(
        span_from_cstr(valid),
        &alias.diagnostic,
        1u,
        &alias.result);
    if (status.native != -(int64_t)EINVAL) {
        return 6;
    }

    arbor_view0_native_diagnostic deterministic_a[8] = {{0}};
    arbor_view0_native_diagnostic deterministic_b[8] = {{0}};
    arbor_view0_native_result result_a = {0};
    arbor_view0_native_result result_b = {0};
    static const char multi[] =
        "<!doctype html>\n<html><head><title>x</title></head><body>\n"
        "<?bad>\n<div a=1 a=2></div>\n<!doctype html>\n"
        "</body></html>\n";
    status = arbor_view0_native_check(
        span_from_cstr(multi), deterministic_a, 8u, &result_a);
    if (status.native != 0 || result_a.diagnostic_count < 3u) {
        return 7;
    }
    status = arbor_view0_native_check(
        span_from_cstr(multi), deterministic_b, 8u, &result_b);
    if (status.native != 0 ||
        unchanged(&result_a, &result_b, sizeof(result_a)) != 0 ||
        unchanged(
            deterministic_a,
            deterministic_b,
            (size_t)(result_a.diagnostic_count * sizeof(deterministic_a[0]))) != 0) {
        return 8;
    }

    const uint64_t repeated_errors = 32u;
    const size_t prefix_length = strlen("<!doctype html><html><head><title>x</title></head><body>");
    const size_t suffix_length = strlen("</body></html>");
    const size_t error_length = strlen("<?bad>");
    const size_t repeated_length = prefix_length +
        (size_t)repeated_errors * error_length + suffix_length;
    char *repeated = (char *)malloc(repeated_length + 1u);
    if (repeated == NULL) {
        return 9;
    }
    char *cursor = repeated;
    (void)memcpy(cursor, "<!doctype html><html><head><title>x</title></head><body>", prefix_length);
    cursor += prefix_length;
    for (uint64_t i = 0u; i < repeated_errors; ++i) {
        (void)memcpy(cursor, "<?bad>", error_length);
        cursor += error_length;
    }
    (void)memcpy(cursor, "</body></html>", suffix_length);
    cursor += suffix_length;
    *cursor = '\0';

    arbor_view0_native_diagnostic limited[31];
    (void)memset(limited, 0x5a, sizeof(limited));
    arbor_view0_native_diagnostic limited_before[31];
    (void)memcpy(limited_before, limited, sizeof(limited));
    result = result_before;
    status = arbor_view0_native_check(
        (arbor_span){(const uint8_t *)repeated, (uint64_t)repeated_length},
        limited,
        31u,
        &result);
    free(repeated);
    if (status.native != -(int64_t)ENOSPC ||
        unchanged(limited, limited_before, sizeof(limited)) != 0 ||
        unchanged(&result, &result_before, sizeof(result)) != 0) {
        return 10;
    }

    puts("PASS: VIEW0 V1N0 bounded capacity/input, alias rejection, failure atomicity and deterministic diagnostics");
    return 0;
}
