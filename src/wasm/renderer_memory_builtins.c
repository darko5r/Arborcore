#include <stddef.h>
#include <stdint.h>

void *memcpy(void *destination, const void *source, size_t count)
{
    uint8_t *dst = (uint8_t *)destination;
    const uint8_t *src = (const uint8_t *)source;
    size_t i;
    for (i = 0u; i < count; ++i) {
        dst[i] = src[i];
    }
    return destination;
}

void *memmove(void *destination, const void *source, size_t count)
{
    uint8_t *dst = (uint8_t *)destination;
    const uint8_t *src = (const uint8_t *)source;
    if (dst < src) {
        size_t i;
        for (i = 0u; i < count; ++i) dst[i] = src[i];
    } else if (dst > src) {
        size_t i = count;
        while (i != 0u) {
            --i;
            dst[i] = src[i];
        }
    }
    return destination;
}

void *memset(void *destination, int value, size_t count)
{
    uint8_t *dst = (uint8_t *)destination;
    uint8_t byte = (uint8_t)value;
    size_t i;
    for (i = 0u; i < count; ++i) {
        dst[i] = byte;
    }
    return destination;
}
