#ifndef ARBORCORE_VIEW0_CONFORMANCE_ECMA_EARLY_ERRORS_H
#define ARBORCORE_VIEW0_CONFORMANCE_ECMA_EARLY_ERRORS_H

#include <arborcore/view0_conformance/native.h>

void arbor_view0_native_v1n3_apply_early_errors(
    uint64_t operation, arbor_span source,
    arbor_view0_native_v1n3_ecma_result *result);

#endif
