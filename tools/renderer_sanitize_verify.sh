#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
OUT="$ROOT/build/renderer-sanitize"
mkdir -p "$OUT"
COMMON=( -I"$ROOT/include" -I"$ROOT/tests/c" -std=c17 -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=all -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef )
for name in renderer_surface_test renderer_blend_test renderer_rect_test renderer_path_test; do
  cc "${COMMON[@]}" \
    "$ROOT/tests/c/$name.c" "$ROOT/src/c/renderer.c" "$ROOT/src/c/geometry.c" \
    -fsanitize=address,undefined -o "$OUT/$name"
  "$OUT/$name" >/dev/null
done
echo "PASS: R4-R7 ASan/UBSan reference-renderer qualification"
