#ifndef ARBORCORE_VIEW0_CONFORMANCE_ECMA_UNICODE_H
#define ARBORCORE_VIEW0_CONFORMANCE_ECMA_UNICODE_H

#include <arborcore/view0_conformance/native.h>

#include <stdbool.h>
#include <stdint.h>

bool arbor_view0_native_v1n3_utf8_scalar(
    arbor_span source, uint64_t offset, uint32_t *scalar_out, uint64_t *width_out);
bool arbor_view0_native_v1n3_identifier_start(uint32_t scalar);
bool arbor_view0_native_v1n3_identifier_continue(uint32_t scalar);

#endif
