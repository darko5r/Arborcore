#include <arborcore/arborcore.h>

void *arbor_secure_clear(void *destination, uint64_t length)
{
    return memory_secure_clear(destination, length);
}

bool arbor_secure_equal(const void *left, const void *right, uint64_t length)
{
    return memory_equal_constant_time(left, right, length) == 1u;
}
