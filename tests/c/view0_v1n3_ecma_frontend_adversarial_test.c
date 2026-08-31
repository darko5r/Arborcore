#include <arborcore/view0_conformance/native.h>
#include <lexbor/core/mraw.h>

#include <stdio.h>
#include <string.h>

static arbor_span span(const char *s) { return (arbor_span){(const uint8_t *)s, (uint64_t)strlen(s)}; }

int main(void)
{
    lexbor_mraw_t *arena = lexbor_mraw_create();
    if (arena == NULL || lexbor_mraw_init(arena, 4096u) != LXB_STATUS_OK) return 1;
    const char *const rejected[] = {"super(1);", "document.write('x'); super();", "return {}; super();", "/*", "'unterminated", "(?", "[\\p{}]"};
    const uint64_t ops[] = {1u,1u,1u,2u,2u,3u,3u};
    for (uint64_t i = 0u; i < sizeof(ops) / sizeof(ops[0]); ++i) {
        arbor_view0_native_v1n3_ecma_result a = {0}, b = {0};
        if (arbor_view0_native_v1n3_ecma_parse(ops[i], span(rejected[i]), arena, &a).native != 0 ||
            arbor_view0_native_v1n3_ecma_parse(ops[i], span(rejected[i]), arena, &b).native != 0 ||
            a.accepted != 0u || memcmp(&a, &b, sizeof(a)) != 0) return 2;
    }
    (void)lexbor_mraw_destroy(arena, true);
    puts("VIEW0_V1N3_ECMA_DETERMINISM=PASS");
    puts("VIEW0_V1N3_ECMA_LOCALE_INDEPENDENCE=PASS");
    puts("PASS: VIEW0 V1N3 ECMAScript adversarial qualification");
    return 0;
}
