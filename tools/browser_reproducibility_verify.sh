#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
CC="${CC:-cc}"
AR="${AR:-ar}"
OUT="$ROOT/build/browser-repro"
rm -rf "$OUT"
mkdir -p "$OUT/a" "$OUT/b"
for dir in a b; do
  "$CC" -I"$ROOT/include" -D_POSIX_C_SOURCE=200809L -std=c17 -O2 -fPIC \
    -Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion -Wshadow \
    -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef \
    -c "$ROOT/src/c/browser_surface.c" -o "$OUT/$dir/browser_surface.o"
  "$AR" rcsD "$OUT/$dir/libarborcore_browser_surface.a" "$OUT/$dir/browser_surface.o"
done
cmp "$OUT/a/libarborcore_browser_surface.a" "$OUT/b/libarborcore_browser_surface.a"
sha="$(sha256sum "$OUT/a/libarborcore_browser_surface.a" | awk '{print $1}')"
printf 'browser_repro_sha256=%s\n' "$sha"
printf 'B6_BROWSER_REPRO_RESULT=PASS\nBROWSER_ARCHIVE_SHA256=%s\n' "$sha" > "$OUT/result.env"
echo 'PASS: independent Browser Precision Surface archive builds are byte-for-byte reproducible'
