#include <arborcore/view0_conformance/native.h>
#include <lexbor/core/mraw.h>

#include <stdio.h>
#include <string.h>

static arbor_span span(const char *s) { return (arbor_span){(const uint8_t *)s, (uint64_t)strlen(s)}; }

int main(void)
{
    lexbor_mraw_t *arena = lexbor_mraw_create();
    if (arena == NULL || lexbor_mraw_init(arena, 4096u) != LXB_STATUS_OK) return 1;
    const struct { uint64_t op; const char *source; uint64_t accepted; } cases[] = {
        {ARBOR_VIEW0_NATIVE_V1N3_ECMA_CONSTRUCTOR_SUBSET, "super(); return this;", 1u},
        {ARBOR_VIEW0_NATIVE_V1N3_ECMA_CONSTRUCTOR_SUBSET, "work(); super();", 0u},
        {ARBOR_VIEW0_NATIVE_V1N3_ECMA_FUNCTION_BODY, "const x = 1\nreturn x", 1u},
        {ARBOR_VIEW0_NATIVE_V1N3_ECMA_FUNCTION_BODY, "if (", 0u},
        {ARBOR_VIEW0_NATIVE_V1N3_ECMA_PATTERN_V, "[a-z&&[^q]]+", 1u},
        {ARBOR_VIEW0_NATIVE_V1N3_ECMA_PATTERN_V, "[a-z", 0u}
    };
    for (uint64_t i = 0u; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        arbor_view0_native_v1n3_ecma_result result = {0};
        if (arbor_view0_native_v1n3_ecma_parse(cases[i].op, span(cases[i].source), arena, &result).native != 0 ||
            result.accepted != cases[i].accepted) return 2;
    }
    (void)lexbor_mraw_destroy(arena, true);
    puts("VIEW0_V1N3_ECMA_FRONTEND_OPERATIONS=3_OF_3");
    puts("VIEW0_V1N3_ECMA_FRONTEND_EXECUTION=ZERO");
    puts("PASS: VIEW0 V1N3 parse-only ECMAScript frontend");
    return 0;
}
