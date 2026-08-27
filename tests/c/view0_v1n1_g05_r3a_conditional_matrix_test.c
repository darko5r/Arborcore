#include "../../tools/c/view0_conformance/g05_r3a.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct test_case {
    unsigned clause;
    const char *html;
} test_case;

static const test_case cases[] = {
    {1u, "<!doctype html><html><head><title>x</title><base></head><body></body></html>"},
    {2u, "<!doctype html><html><head><title>x</title><link rel=\"stylesheet\"></head><body></body></html>"},
    {3u, "<!doctype html><html><head><title>x</title><link href=\"/\"></head><body></body></html>"},
    {4u, "<!doctype html><html><head><title>x</title><link href=\"/\" rel=\"author\" integrity=\"x\"></head><body></body></html>"},
    {5u, "<!doctype html><html><head><title>x</title><link href=\"/\" rel=\"preload\" as=\"script\" imagesrcset=\"x\"></head><body></body></html>"},
    {6u, "<!doctype html><html><head><title>x</title><link href=\"/\" rel=\"author\" sizes=\"any\"></head><body></body></html>"},
    {7u, "<!doctype html><html><head><title>x</title><link href=\"/\" rel=\"preload\"></head><body></body></html>"},
    {8u, "<!doctype html><html><head><title>x</title><link href=\"/\" rel=\"author\" color=\"red\"></head><body></body></html>"},
    {9u, "<!doctype html><html><head><title>x</title><link href=\"/\" rel=\"author\" disabled></head><body></body></html>"},
    {10u, "<!doctype html><html><head><title>x</title><meta name=\"x\" charset=\"utf-8\"></head><body></body></html>"},
    {11u, "<!doctype html><title>x</title><body><a target=\"_blank\">x</a></body>"},
    {12u, "<!doctype html><title>x</title><body><img alt=\"x\" ismap></body>"},
    {13u, "<!doctype html><title>x</title><body><img controls></body>"},
    {14u, "<!doctype html><title>x</title><body><map name=\"m\"><area alt=\"x\"></map></body>"},
    {15u, "<!doctype html><title>x</title><body><map name=\"m\"><area target=\"_blank\"></map></body>"},
    {16u, "<!doctype html><title>x</title><body><input type=\"hidden\" name=\"_charset_\" value=\"x\"></body>"},
    {17u, "<!doctype html><title>x</title><body><input type=\"hidden\" accept=\"x\"></body>"},
    {18u, "<!doctype html><title>x</title><body><input type=\"text\" accept=\"x\"></body>"},
    {19u, "<!doctype html><title>x</title><body><input type=\"tel\" accept=\"x\"></body>"},
    {20u, "<!doctype html><title>x</title><body><input type=\"url\" accept=\"x\"></body>"},
    {21u, "<!doctype html><title>x</title><body><input type=\"email\" accept=\"x\"></body>"},
    {22u, "<!doctype html><title>x</title><body><input type=\"password\" accept=\"x\"></body>"},
    {23u, "<!doctype html><title>x</title><body><input type=\"date\" accept=\"x\"></body>"},
    {24u, "<!doctype html><title>x</title><body><input type=\"month\" accept=\"x\"></body>"},
    {25u, "<!doctype html><title>x</title><body><input type=\"week\" accept=\"x\"></body>"},
    {26u, "<!doctype html><title>x</title><body><input type=\"time\" accept=\"x\"></body>"},
    {27u, "<!doctype html><title>x</title><body><input type=\"datetime-local\" accept=\"x\"></body>"},
    {28u, "<!doctype html><title>x</title><body><input type=\"number\" accept=\"x\"></body>"},
    {29u, "<!doctype html><title>x</title><body><input type=\"range\" readonly></body>"},
    {30u, "<!doctype html><title>x</title><body><input type=\"color\" max=\"1\"></body>"},
    {31u, "<!doctype html><title>x</title><body><input type=\"checkbox\" list=\"x\"></body>"},
    {32u, "<!doctype html><title>x</title><body><input type=\"radio\" list=\"x\"></body>"},
    {33u, "<!doctype html><title>x</title><body><input type=\"file\" checked></body>"},
    {34u, "<!doctype html><title>x</title><body><input type=\"file\" value=\"x\"></body>"},
    {35u, "<!doctype html><title>x</title><body><input type=\"submit\" accept=\"x\"></body>"},
    {36u, "<!doctype html><title>x</title><body><input type=\"image\" accept=\"x\"></body>"},
    {37u, "<!doctype html><title>x</title><body><input type=\"image\" value=\"x\"></body>"},
    {38u, "<!doctype html><title>x</title><body><input type=\"reset\" accept=\"x\"></body>"},
    {39u, "<!doctype html><title>x</title><body><input type=\"button\" accept=\"x\"></body>"},
    {40u, "<!doctype html><title>x</title><body><select><optgroup><option>x</option></optgroup></select></body>"},
    {41u, "<!doctype html><title>x</title><body><textarea wrap=\"hard\"></textarea></body>"},
    {42u, "<!doctype html><title>x</title><body><meter>x</meter></body>"},
    {43u, "<!doctype html><title>x</title><body><dialog tabindex=\"0\">x</dialog></body>"},
};

static int fail(const char *msg, unsigned clause)
{
    (void)fprintf(stderr, "FAIL clause %u: %s\n", clause, msg);
    return 1;
}

int main(void)
{
    if (sizeof(cases) / sizeof(cases[0]) != 43u) {
        return fail("case count", 0u);
    }

    for (size_t i = 0u; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        const test_case *tc = &cases[i];
        arbor_view0_native_g05_r3a_evaluation ev = {0};
        arbor_span input = {(const uint8_t *)tc->html, (uint64_t)strlen(tc->html)};
        arbor_status status = arbor_view0_native_g05_r3a_measure(input, &ev);
        if (status.native != 0) return fail("measure status", tc->clause);
        if (ev.clause_violation_count[tc->clause - 1u] == 0u)
            return fail("target clause did not fire", tc->clause);
        if (ev.diagnostic_count != 1u)
            return fail("expected isolated one diagnostic", tc->clause);
        for (unsigned c = 1u; c <= 43u; ++c) {
            const uint64_t expected = c == tc->clause ? UINT64_C(1) : UINT64_C(0);
            if (ev.clause_violation_count[c - 1u] != expected)
                return fail("non-target clause fired", tc->clause);
        }
    }

    (void)printf("VIEW0_V1N1_G05_R3A_CONDITIONAL_CLAUSE_NEGATIVES=43_OF_43\n");
    (void)printf("PASS: G05 R3A isolated frozen conditional-clause predicate matrix\n");
    return 0;
}
