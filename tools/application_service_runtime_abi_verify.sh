#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"

make application-service-runtime-native-test

OBJ=build/application-service-runtime/application_service_runtime_abi_test.o
TEST=build/application-service-runtime-test
[[ -f "$OBJ" && -x "$TEST" ]]

for symbol in \
  af3_asm_prepare \
  af3_asm_rollback \
  af3_asm_stop \
  af3_asm_typed_method \
  af3_asm_call_c_typed \
  af3_asm_call_c_typed_preserve; do
  nm -g --defined-only "$OBJ" | awk '{print $3}' | grep -Fxq "$symbol"
done

readelf -W -S "$OBJ" | grep -Fq '.note.GNU-stack'

# A handwritten NASM ELF object must not silently be replaced by a compiler
# generated C stand-in. Arborcore's qualified NASM objects have neither GCC/
# Clang .comment metadata nor compiler unwind .eh_frame sections.
if readelf -W -S "$OBJ" | grep -Eq '\.(comment|eh_frame)([[:space:]]|$)'; then
  echo 'FAIL: AF3 ABI object appears compiler-generated rather than handwritten NASM' >&2
  exit 1
fi
if readelf -W -l "$TEST" | grep -E 'GNU_STACK.*RWE' >/dev/null; then
  echo 'FAIL: AF3 test executable has executable GNU_STACK' >&2
  exit 1
fi

grep -Fq 'RDI_PROVIDER_CONTEXT_RSI_TYPED_INPUT_RDX_TYPED_OUTPUT_RAX_NATIVE_STATUS' \
  application/arborcore-application-service-runtime-1.contract
grep -Fq 'RDI_MODULE_CONTEXT_RSI_PREPARE_CONTEXT_RAX_NATIVE_STATUS' \
  application/arborcore-application-service-runtime-1.contract
grep -Fq 'RDI_MODULE_CONTEXT_RAX_NATIVE_STATUS' \
  application/arborcore-application-service-runtime-1.contract

echo 'AF3_C_TO_ASSEMBLY_PREPARE=PASS'
echo 'AF3_C_TO_ASSEMBLY_STOP=PASS'
echo 'AF3_C_TO_ASSEMBLY_TYPED_METHOD=PASS'
echo 'AF3_ASSEMBLY_TO_C_TYPED_METHOD=PASS'
echo 'AF3_SYSV_STACK_ALIGNMENT=PASS'
echo 'AF3_SYSV_CALLEE_SAVED=PASS'
echo 'PASS: AF3 handwritten x86-64 Assembly/C ABI qualification'
