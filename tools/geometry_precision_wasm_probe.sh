#!/usr/bin/env bash
set -euo pipefail

ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
OUT_DIR="$ROOT/build/geometry-precision-g0-g1"
mkdir -p "$OUT_DIR"

if ! command -v clang >/dev/null 2>&1; then
    echo "G0_WASM_COMPILE_PROBE=SKIP_NO_CLANG"
    echo "G0_WASM_NOTE=Representation remains compatible with wasm i32/i64; compiler lowering of wide fixed-point intermediates must be re-qualified in the browser toolchain phase."
    exit 0
fi

probe="$OUT_DIR/wasm_fixed_probe.c"
object="$OUT_DIR/wasm_fixed_probe.o"
cat > "$probe" <<'PROBE'
typedef signed int i32;
typedef signed long long i64;
typedef __int128 i128;

__attribute__((used)) i32 q16_add(i32 a, i32 b) { return a + b; }
__attribute__((used)) i32 q26_add(i32 a, i32 b) { return a + b; }
__attribute__((used)) i64 q32_add(i64 a, i64 b) { return a + b; }
__attribute__((used)) i64 q24_add(i64 a, i64 b) { return a + b; }
__attribute__((used)) i64 q32_mul(i64 a, i64 b) {
    i128 p = (i128)a * (i128)b;
    return (i64)(p / (((i128)1) << 32));
}
__attribute__((used)) i64 q24_mul(i64 a, i64 b) {
    i128 p = (i128)a * (i128)b;
    return (i64)(p / (((i128)1) << 40));
}
PROBE

clang --target=wasm32 -O2 -c "$probe" -o "$object"
if ! file "$object" | grep -q 'WebAssembly'; then
    echo "FAIL: clang wasm32 probe did not produce a WebAssembly object" >&2
    exit 1
fi

echo "G0_WASM_COMPILE_PROBE=PASS"
echo "G0_WASM_OBJECT=$object"
echo "G0_WASM_NOTE=Q32.32/Q24.40 storage maps to wasm i64; 128-bit intermediates compile but generated browser code remains subject to later WASM performance qualification."
