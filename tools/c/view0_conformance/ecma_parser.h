#ifndef ARBORCORE_VIEW0_CONFORMANCE_ECMA_PARSER_H
#define ARBORCORE_VIEW0_CONFORMANCE_ECMA_PARSER_H

#include "ecma_lexer.h"

arbor_status arbor_view0_native_v1n3_parse_function_body(
    arbor_span source, arbor_view0_native_v1n3_ecma_result *result_out);
arbor_status arbor_view0_native_v1n3_parse_constructor_subset(
    arbor_span source, arbor_view0_native_v1n3_ecma_result *result_out);

#endif
