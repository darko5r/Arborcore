#ifndef ARBORCORE_VIEW0_CONFORMANCE_ECMA_FRONTEND_H
#define ARBORCORE_VIEW0_CONFORMANCE_ECMA_FRONTEND_H

#include <arborcore/view0_conformance/native.h>

arbor_status arbor_view0_native_v1n3_ecma_parse(
    uint64_t operation,
    arbor_span source,
    void *support_arena,
    arbor_view0_native_v1n3_ecma_result *result_out);

#endif
