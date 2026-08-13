#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
OUT="$ROOT/build/renderer-reproducibility"
rm -rf "$OUT"
mkdir -p "$OUT/a/include/arborcore" "$OUT/a/src/c" "$OUT/a/renderer" \
         "$OUT/b/include/arborcore" "$OUT/b/src/c" "$OUT/b/renderer"
for tree in a b; do
  cp "$ROOT/include/arborcore/renderer.h" "$OUT/$tree/include/arborcore/renderer.h"
  cp "$ROOT/include/arborcore/geometry.h" "$OUT/$tree/include/arborcore/geometry.h"
  cp "$ROOT/src/c/renderer.c" "$OUT/$tree/src/c/renderer.c"
  cp "$ROOT/renderer/srgb8_linear16_lut.h" "$OUT/$tree/renderer/srgb8_linear16_lut.h"
  (
    cd "$OUT/$tree"
    cc -Iinclude -std=c17 -O2 -fPIC -Wall -Wextra -Wpedantic -Werror \
      -Wconversion -Wsign-conversion -Wshadow -Wstrict-prototypes \
      -Wmissing-prototypes -Wformat=2 -Wundef \
      -c src/c/renderer.c -o renderer.o
    ar rcsD libarborcore_renderer.a renderer.o
  )
done
cmp "$OUT/a/libarborcore_renderer.a" "$OUT/b/libarborcore_renderer.a"
sha="$(sha256sum "$OUT/a/libarborcore_renderer.a" | awk '{print $1}')"
cat > "$OUT/result.env" <<EVIDENCE
RENDERER_REPRODUCIBILITY_RESULT=PASS
RENDERER_ARCHIVE_SHA256=$sha
EVIDENCE
printf 'renderer_repro_sha256=%s\n' "$sha"
echo "PASS: independent renderer archive builds are byte-for-byte reproducible"
