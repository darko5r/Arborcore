#include <arborcore/view0_conformance/native.h>

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void need(bool ok, const char *message)
{
    if (!ok) {
        (void)fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

static uint64_t count_rule(const char *body, uint64_t rule)
{
    static const char prefix[] = "<!doctype html><html><head><title>x</title></head><body>";
    static const char suffix[] = "</body></html>";
    char input[4096];
    arbor_view0_native_diagnostic diagnostics[256];
    arbor_view0_native_result result = {0};
    const int written = snprintf(input, sizeof(input), "%s%s%s", prefix, body, suffix);
    need(written > 0 && (size_t)written < sizeof(input), "fixture construction");
    (void)memset(diagnostics, 0, sizeof(diagnostics));
    arbor_status status = arbor_view0_native_check(
        (arbor_span){(const uint8_t *)input, (uint64_t)written},
        diagnostics, 256u, &result);
    if (status.native != 0) {
        (void)fprintf(stderr, "FAIL: checker status=%" PRId64 " fixture=%s\n",
                      status.native, body);
        exit(1);
    }
    uint64_t count = 0u;
    for (uint64_t i = 0u; i < result.diagnostic_count; ++i) {
        if (diagnostics[i].rule_id == rule) ++count;
    }
    return count;
}

int main(void)
{
    static const struct {
        const char *fixture;
        uint64_t rule;
        const char *name;
    } negatives[] = {
        {"<details open=false>x</details>", UINT64_C(0x0000000030060001), "R1"},
        {"<div dir=sideways>x</div>", UINT64_C(0x0000000030060002), "R2"},
        {"<ol start=+><li>x</li></ol>", UINT64_C(0x0000000030060003), "R3"},
        {"<textarea cols=0>x</textarea>", UINT64_C(0x0000000030060004), "R4"},
        {"<meter value=NaN></meter>", UINT64_C(0x0000000030060005), "R5"},
        {"<input type=month value=2026-13>", UINT64_C(0x0000000030060006), "R6"},
        {"<input type=date value=2026-02-30>", UINT64_C(0x0000000030060007), "R7"},
        {"<time datetime=02-30>x</time>", UINT64_C(0x0000000030060008), "R8"},
        {"<input type=time value=25:00>", UINT64_C(0x0000000030060009), "R9"},
        {"<input type=datetime-local value=2026-08-19T25:00>", UINT64_C(0x000000003006000a), "R10"},
        {"<time datetime=+24:00>x</time>", UINT64_C(0x000000003006000b), "R11"},
        {"<time datetime=2026-08-19T17:30+24:00>x</time>", UINT64_C(0x000000003006000c), "R12"},
        {"<input type=week value=2021-W53>", UINT64_C(0x000000003006000d), "R13"},
        {"<time datetime=P>x</time>", UINT64_C(0x000000003006000e), "R14"},
        {"<p class='a a'>x</p>", UINT64_C(0x0000000030060010), "R16"},
        {"<meta name=keywords content=','>", UINT64_C(0x0000000030060011), "R17"}
    };
    for (size_t i = 0u; i < sizeof(negatives) / sizeof(negatives[0]); ++i) {
        if (count_rule(negatives[i].fixture, negatives[i].rule) != 1u) {
            (void)fprintf(stderr, "FAIL: %s negative did not publish exactly once\n",
                          negatives[i].name);
            return 1;
        }
        (void)printf("PASS G06-%s\n", negatives[i].name);
    }
    puts("PASS G06-R15 ZERO_AUTHOR_FACING_CONSUMER");
    need(count_rule("<time datetime=2011>x</time>", UINT64_C(0x0000000030060006)) == 0u,
         "time year-only branch remains admitted without invented rule");
    need(count_rule("<time datetime=banana>x</time>", UINT64_C(0x0000000030060006)) == 1u,
         "time invalid union deterministic first-branch fallback");
    need(count_rule("<input type=text accept='image/png'>", UINT64_C(0x0000000030060011)) == 0u,
         "G05 placement owner suppresses G06 accept value");
    need(count_rule("<link rel=stylesheet disabled=false>", UINT64_C(0x0000000030060001)) == 1u,
         "conditional boolean consumer");
    need(count_rule("<link rel=preload as=worker>", UINT64_C(0x0000000030060002)) == 1u,
         "A2 preload destination policy");
    need(count_rule("<link rel=modulepreload as=worker>", UINT64_C(0x0000000030060002)) == 0u,
         "A2 modulepreload destination policy");
    need(count_rule("<input type=text checked=false>", UINT64_C(0x0000000030060001)) == 0u,
         "G05 input-state placement owner suppresses G06 boolean value");
    need(count_rule("<table><colgroup><col span=1001></colgroup></table>", UINT64_C(0x0000000030060004)) == 1u,
         "col consuming upper bound");
    need(count_rule("<link rel=icon sizes=0x16>", UINT64_C(0x0000000030060004)) == 1u,
         "sizes dimension component bound");
    need(count_rule("<meter min=2 max=1></meter>", UINT64_C(0x0000000030060005)) == 1u,
         "meter cross-attribute range");
    need(count_rule("<progress max=0></progress>", UINT64_C(0x0000000030060005)) == 1u,
         "progress positive maximum");
    need(count_rule("<meter min=1e100 max=2e100 value=1.5e100></meter>",
                    UINT64_C(0x0000000030060005)) == 0u,
         "exact decimal exponent comparison");
    need(count_rule("<time>20<!--split-->11-11</time>", UINT64_C(0x0000000030060006)) == 0u,
         "time child text concatenation");
    need(count_rule("<time><span>banana</span></time>", UINT64_C(0x0000000030060006)) == 0u,
         "G03 time descendant owner suppresses G06 union fallback");
    need(count_rule("<input type=file accept='image/png,'>", UINT64_C(0x0000000030060011)) == 1u,
         "file accept empty comma token");
    need(count_rule("<input type=file accept='image/png,IMAGE/PNG'>",
                    UINT64_C(0x0000000030060011)) == 1u,
         "file accept ASCII-case-insensitive duplicate restriction");
    need(count_rule("<input type=file accept=''>",
                    UINT64_C(0x0000000030060011)) == 0u,
         "file accept empty token set remains admitted");
    need(count_rule("<ol start='999999999999999999999999999999999999'>"
                    "<li>x</li></ol>", UINT64_C(0x0000000030060003)) == 0u,
         "valid signed-integer authoring syntax remains unbounded");
    need(count_rule("<input type=email multiple value='a@example.com,bad'>",
                    UINT64_C(0x0000000030060011)) == 1u,
         "multiple email consumer");
    puts("VIEW0_V1N1_G06_WAVE_RULE_CHECKPOINTS=R1_R14_R16_R17_PASS_R15_ZERO_CONSUMER");
    puts("PASS: VIEW0 V1N1 G06 internally checkpointed diagnostic wave");
    return 0;
}
