#include <arborcore/view0_conformance/native.h>

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    static const char html[] = "<!doctype html><title>x</title><p>x</p>";
    const arbor_view0_native_semantic_observer observer = {0};
    arbor_view0_native_parse_counts parse_counts;
    arbor_view0_native_document_facts facts;
    arbor_view0_native_observation_counts observations;
    (void)memset(&parse_counts, 0x11, sizeof(parse_counts));
    (void)memset(&facts, 0x22, sizeof(facts));
    (void)memset(&observations, 0x33, sizeof(observations));
    const arbor_view0_native_parse_counts parse_before = parse_counts;
    const arbor_view0_native_document_facts facts_before = facts;
    const arbor_view0_native_observation_counts observations_before = observations;

    const arbor_status status = arbor_view0_native_lexbor_observe(
        (arbor_span){(const uint8_t *)html, (uint64_t)strlen(html)},
        &observer,
        &parse_counts,
        &facts,
        &observations);
    if (status.native != -(int64_t)EIO ||
        memcmp(&parse_counts, &parse_before, sizeof(parse_counts)) != 0 ||
        memcmp(&facts, &facts_before, sizeof(facts)) != 0 ||
        memcmp(&observations, &observations_before, sizeof(observations)) != 0) {
        return 1;
    }

    puts("PASS: VIEW0 V1N1 G03 C0 observation fails closed when the qualified provenance wrapper is absent");
    return 0;
}
