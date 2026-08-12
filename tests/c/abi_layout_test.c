#include <stdint.h>

#include <arborcore/arborcore.h>

int main(void)
{
    arbor_asm_result_u64 sum = u64_add_checked(2u, 3u);
    if (sum.status != 0 || sum.value != 5u) {
        return 1;
    }

    arbor_asm_result_u64 contains = range_contains_point(10u, 5u, 12u);
    if (contains.status != 0 || contains.value != 1u) {
        return 2;
    }

    static const uint8_t spaced[] = {' ', '\t', 'a', 'b', ' ', '\n'};
    arbor_asm_span_result trimmed = bytes_trim_ascii_space(spaced, (uint64_t)sizeof(spaced));
    if (trimmed.data != &spaced[2] || trimmed.length != 2u) {
        return 3;
    }

    return 0;
}
