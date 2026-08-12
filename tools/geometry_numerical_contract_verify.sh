#!/usr/bin/env bash
set -euo pipefail

ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
HEADER="$ROOT/include/arborcore/geometry.h"
SOURCE="$ROOT/src/c/geometry.c"
WASM_HELPER="$ROOT/src/wasm/geometry_int128_builtins.c"
CONTRACT="$ROOT/geometry/arborcore-geometry-1.contract"
LIB="$ROOT/build/libarborcore_geometry.a"
OBJ="$ROOT/build/geometry/geometry.o"

for file in "$HEADER" "$SOURCE" "$WASM_HELPER" "$CONTRACT" "$LIB" "$OBJ"; do
  [[ -e "$file" ]] || { echo "FAIL: missing $file" >&2; exit 1; }
done

grep -q '^CONTRACT_VERSION=1.0$' "$CONTRACT"
grep -q '^CONTRACT_STATUS=VERSIONED_NUMERICAL_DEFINITION$' "$CONTRACT"
grep -q '^REPRESENTATION=SIGNED_Q32_32$' "$CONTRACT"
grep -q '^FRACTION_BITS=32$' "$CONTRACT"
grep -q '^ONE_RAW=4294967296$' "$CONTRACT"
grep -q '^DEFAULT_MUL_ROUNDING=NEAREST_EVEN$' "$CONTRACT"
grep -q '^DEFAULT_DIV_ROUNDING=NEAREST_EVEN$' "$CONTRACT"
grep -q '^DEVICE_SCALE=POSITIVE_REDUCED_RATIONAL$' "$CONTRACT"
grep -q '^WASM_AUTHORITATIVE_STORAGE=i64$' "$CONTRACT"
grep -q '^JAVASCRIPT_NUMBER_AUTHORITATIVE=NO$' "$CONTRACT"

grep -q '^#define ARBOR_COORD_FRACTION_BITS 32u$' "$HEADER"
grep -q '^#define ARBOR_COORD_ONE INT64_C(4294967296)$' "$HEADER"
grep -q '^typedef int64_t arbor_coord;$' "$HEADER"
grep -q '^#define ARBOR_CORDIC_ITERATIONS 31u$' "$SOURCE"
grep -q '^#define ARBOR_WASM_RUNTIME_OPAQUE __attribute__((noinline, optnone))$' "$WASM_HELPER"
grep -q '^static ARBOR_WASM_RUNTIME_OPAQUE uint64_t arbor_mul64_high' "$WASM_HELPER"
grep -q '^ARBOR_WASM_RUNTIME_OPAQUE arbor_wasm_u128 __multi3' "$WASM_HELPER"
grep -q '^ARBOR_WASM_RUNTIME_OPAQUE arbor_wasm_u128 __udivti3' "$WASM_HELPER"

if grep -Eq '#include[[:space:]]+[<"]arborcore/(arborcore|assembly_abi)\.h[>"]' "$HEADER" "$SOURCE"; then
  echo "FAIL: portable geometry depends on native Assembly/C-runtime headers." >&2
  exit 1
fi
if grep -Eq '(^|[^[:alnum:]_])(float|double|long[[:space:]]+double)([^[:alnum:]_]|$)|<math\.h>|\b(sin|cos|tan|atan2?)\s*\(' "$HEADER" "$SOURCE" "$WASM_HELPER"; then
  echo "FAIL: authoritative geometry contains floating-point/libm semantics." >&2
  exit 1
fi

public_count="$(nm -g --defined-only "$LIB" | awk 'NF >= 3 {print $3}' | grep '^arbor_' | sort -u | wc -l | tr -d ' ')"
[[ "$public_count" == "32" ]] || {
  echo "FAIL: unexpected construction-stage geometry symbol count: $public_count" >&2
  exit 1
}

undefined_symbols="$(nm -u "$LIB" 2>/dev/null | awk '{print $NF}')"
if grep -Eq '^arbor_' <<<"$undefined_symbols"; then
  echo "FAIL: portable geometry library depends on another Arborcore runtime layer." >&2
  exit 1
fi

if [[ "$(uname -m)" == "x86_64" ]]; then
  extract_symbol() {
    local symbol="$1"
    objdump -d -Mintel "$OBJ" | awk -v target="<$symbol>:" '
      index($0, target) { found=1; next }
      found && /^[[:space:]]*[[:xdigit:]]+[[:space:]]+<[^>]+>:/ { found=0 }
      found { print }
    '
  }

  for symbol in arbor_coord_mul arbor_affine_transform_point; do
    disassembly="$(extract_symbol "$symbol")"
    [[ -n "$disassembly" ]] || {
      echo "FAIL: unable to disassemble $symbol for Q32.32 hot-path verification." >&2
      exit 1
    }
    if grep -Eq '\<(div|idiv)\>' <<<"$disassembly"; then
      echo "FAIL: $symbol uses general DIV/IDIV for fixed Q32.32 scale reduction." >&2
      exit 1
    fi
    printf 'PASS: %s fixed-scale reduction contains no DIV/IDIV\n' "$symbol"
  done
fi

contract_sha="$(sha256sum "$CONTRACT" | awk '{print $1}')"
printf 'geometry_contract_sha256=%s\n' "$contract_sha"
printf 'geometry_c_surface_symbol_count=%s\n' "$public_count"
echo "PASS: Geometry Numerical Contract v1 source/shape invariants"
