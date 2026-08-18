#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
make -s build/http1/http1_abi_test.o http1-core-test
OBJ=build/http1/http1_abi_test.o
nm -g --defined-only "$OBJ" | awk '{print $3}' | grep -Fxq http1_asm_call_measure
readelf -W -S "$OBJ" | grep -q '\.note\.GNU-stack'
./build/http1-core-test >/dev/null
echo "HTTP1_REAL_NASM_ABI_OBJECT_SHA256=$(sha256sum "$OBJ" | awk '{print $1}')"
echo "HTTP1_REAL_NASM_ABI_TEXT_BYTES=$(size -A "$OBJ" | awk '$1==".text"{print $2}')"
echo 'HTTP1_ASSEMBLY_TO_C_API_ABI=PASS'
echo 'HTTP1_MVC0_ASSEMBLY_TRANSPORT_REUSE=PASS'
echo 'PASS: HTTP1 real NASM C-API consumer and reused transport ABI qualification'
