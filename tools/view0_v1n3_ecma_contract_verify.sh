#!/usr/bin/env bash
set -euo pipefail
ROOT=${ARBORCORE_ROOT:-$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)}
cd "$ROOT"
rg -q 'ARBOR_VIEW0_NATIVE_V1N3_ECMA_CONSTRUCTOR' tools/include/arborcore/view0_conformance/native.h
rg -q 'ARBOR_VIEW0_NATIVE_V1N3_ECMA_FUNCTION_BODY' tools/include/arborcore/view0_conformance/native.h
rg -q 'ARBOR_VIEW0_NATIVE_V1N3_ECMA_PATTERN_V' tools/include/arborcore/view0_conformance/native.h
rg -q '17\.0\.0' tools/c/view0_conformance/ecma_unicode_tables.c
! rg -n 'eval\(|bytecode|execute_script|RegExp.*match' tools/c/view0_conformance/ecma_*.c
echo "VIEW0_V1N3_ECMA_OPERATIONS=3_OF_3"
echo "VIEW0_V1N3_ECMA_EXECUTION=ZERO"
echo "PASS: parse-only ECMAScript/Unicode contract"

