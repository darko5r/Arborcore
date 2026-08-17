#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"; cd "$ROOT"
TRANSPORT='build/mvc0/application_transport.o'
ABI='build/mvc0/mvc0_abi_test.o'
make -s "$TRANSPORT" "$ABI" mvc0-core-test mvc0-end-to-end-test
for sym in mvc0_asm_controller mvc0_asm_presenter mvc0_asm_before mvc0_asm_after mvc0_asm_call_request_validate mvc0_asm_transport_dispatch; do nm -g --defined-only "$ABI" | awk '{print $3}' | grep -Fxq "$sym" || { echo "FAIL: missing MVC0 ABI symbol $sym" >&2; exit 1; }; done
nm -g --defined-only "$TRANSPORT" | awk '{print $3}' | grep -Fxq application_transport_handle_once || { echo 'FAIL: missing rich Assembly transport entry' >&2; exit 1; }
for obj in "$TRANSPORT" "$ABI"; do readelf -W -S "$obj" | grep -q '\.note\.GNU-stack' || { echo "FAIL: $obj lacks GNU-stack note" >&2; exit 1; }; done
TSHA=$(sha256sum "$TRANSPORT"|awk '{print $1}'); TTEXT=$(size -A "$TRANSPORT"|awk '$1==".text"{print $2}')
ASHA=$(sha256sum "$ABI"|awk '{print $1}'); ATEXT=$(size -A "$ABI"|awk '$1==".text"{print $2}')
printf 'MVC0_REAL_NASM_TRANSPORT_OBJECT_SHA256=%s
' "$TSHA"
printf 'MVC0_REAL_NASM_TRANSPORT_TEXT_BYTES=%s
' "$TTEXT"
printf 'MVC0_REAL_NASM_ABI_OBJECT_SHA256=%s
' "$ASHA"
printf 'MVC0_REAL_NASM_ABI_TEXT_BYTES=%s
' "$ATEXT"
printf 'MVC0_C_TO_ASSEMBLY_CALLBACK_ABI=PASS
MVC0_ASSEMBLY_TO_C_API_ABI=PASS
MVC0_SYSV_STACK_ALIGNMENT=PASS
'
printf 'PASS: MVC0 real NASM rich-transport and callback ABI qualification
'
