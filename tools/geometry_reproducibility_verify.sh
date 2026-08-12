#!/usr/bin/env bash
set -euo pipefail

ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
CC_BIN="${CC:-cc}"
AR_BIN="${AR:-ar}"
TMP="$(mktemp -d /tmp/arborcore-geometry-repro.XXXXXX)"
trap 'rm -rf "$TMP"' EXIT

build_one() {
  local name="$1"
  local dir="$TMP/$name"
  mkdir -p "$dir/include/arborcore" "$dir/src/c" "$dir/build"
  cp "$ROOT/include/arborcore/geometry.h" "$dir/include/arborcore/geometry.h"
  cp "$ROOT/src/c/geometry.c" "$dir/src/c/geometry.c"
  (
    cd "$dir"
    "$CC_BIN" -Iinclude -std=c17 -O2 -fPIC \
      -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion \
      -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef \
      -c src/c/geometry.c -o build/geometry.o
    "$AR_BIN" rcsD build/libarborcore_geometry.a build/geometry.o
  )
}

build_one a
build_one b
sha_a="$(sha256sum "$TMP/a/build/libarborcore_geometry.a" | awk '{print $1}')"
sha_b="$(sha256sum "$TMP/b/build/libarborcore_geometry.a" | awk '{print $1}')"
printf 'geometry_repro_sha256=%s\n' "$sha_a"
[[ "$sha_a" == "$sha_b" ]] || {
  echo "FAIL: independent Geometry library builds differ." >&2
  exit 1
}
echo "PASS: independent Geometry library builds are byte-for-byte reproducible"
