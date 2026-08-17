#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
cd "$ROOT"
OBJ='build/ddd-support/ddd_support_abi_test.o'

make -s "$OBJ" ddd-support-native-test

for sym in \
  af4_asm_transaction_begin \
  af4_asm_transaction_commit \
  af4_asm_transaction_rollback \
  af4_asm_checkpoint_call
do
  nm -g --defined-only "$OBJ" | awk '{print $3}' | grep -Fxq "$sym" ||
    { echo "FAIL: missing real NASM ABI symbol $sym" >&2; exit 1; }
done

readelf -W -S "$OBJ" | grep -q '\.note\.GNU-stack' ||
  { echo 'FAIL: AF4 ABI object lacks GNU-stack note' >&2; exit 1; }

TEXT_BYTES=$(size -A "$OBJ" | awk '$1==".text"{print $2}')
OBJ_SHA=$(sha256sum "$OBJ" | awk '{print $1}')

printf 'AF4_REAL_NASM_OBJECT_SHA256=%s\n' "$OBJ_SHA"
printf 'AF4_REAL_NASM_TEXT_BYTES=%s\n' "$TEXT_BYTES"
printf 'AF4_C_TO_ASSEMBLY_CALLBACK_ABI=PASS\n'
printf 'AF4_ASSEMBLY_TO_C_API_ABI=PASS\n'
printf 'AF4_SYSV_STACK_ALIGNMENT=PASS\n'
printf 'PASS: AF4 real NASM C/Assembly ABI qualification\n'
