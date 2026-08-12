#!/usr/bin/env bash
set -euo pipefail

ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
CC_BIN="${CC:-cc}"
OUT="$ROOT/build/geometry-sanitize"
mkdir -p "$OUT"

common=(
  -I"$ROOT/include"
  -std=c17 -O1 -g -fno-omit-frame-pointer
  -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion
  -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef
  -fsanitize=address,undefined
)

for name in geometry_scalar_test geometry_rect_property_test geometry_affine_test geometry_device_test geometry_semantic_vector_test; do
  "$CC_BIN" "${common[@]}" \
    "$ROOT/tests/c/${name}.c" "$ROOT/src/c/geometry.c" \
    -fsanitize=address,undefined -o "$OUT/$name"
  ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
  UBSAN_OPTIONS=halt_on_error=1 \
    "$OUT/$name"
done

echo "PASS: G2-G4 ASan/UBSan geometry qualification"
