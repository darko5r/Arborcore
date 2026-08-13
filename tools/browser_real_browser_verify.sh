#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
WASM_OUT="$ROOT/build/browser-b0-b6-wasm/arborcore-browser-reference.wasm"
WEB="$ROOT/build/browser-b0-b6-web"
OUT="$ROOT/build/browser-b0-b6-browser"
command -v node >/dev/null 2>&1 || { echo 'B5_BROWSER_RESULT=REVIEW_NO_NODE' >&2; exit 1; }
[[ -f "$WASM_OUT" ]] || { echo 'FAIL: run browser-wasm-verify first.' >&2; exit 1; }
command -v firefox >/dev/null 2>&1 || { echo 'B5_BROWSER_RESULT=REVIEW_NO_FIREFOX' >&2; exit 1; }
command -v google-chrome-stable >/dev/null 2>&1 || { echo 'B5_BROWSER_RESULT=REVIEW_NO_CHROME' >&2; exit 1; }
rm -rf "$WEB" "$OUT"
mkdir -p "$WEB" "$OUT"
cp "$WASM_OUT" "$WEB/arborcore-browser-reference.wasm"
cp "$ROOT/browser/precision_surface.js" "$WEB/precision_surface.js"
cp "$ROOT/tests/browser/precision_surface_browser_test.html" "$WEB/precision_surface_browser_test.html"
FIREFOX_PATH="$(command -v firefox)" \
GOOGLE_CHROME_STABLE_PATH="$(command -v google-chrome-stable)" \
ARBORCORE_ROOT="$ROOT" \
  node "$ROOT/tools/browser_real_browser_runner.mjs"
[[ "$(awk -F= '$1=="B5_REAL_BROWSER_RESULT" {print $2}' "$OUT/result.env")" == PASS ]]
[[ "$(awk -F= '$1=="B5_BROWSER_COUNT" {print $2}' "$OUT/result.env")" == 2 ]]
{
  printf 'FIREFOX_VERSION='; firefox --version 2>/dev/null | sed -n '1p'
  printf 'CHROME_VERSION='; google-chrome-stable --version 2>/dev/null | sed -n '1p'
} > "$OUT/versions.env"
cat "$OUT/result.env"
cat "$OUT/versions.env"
echo 'PASS: B2-B5 Firefox/Chrome Canvas, memory-growth, DPR, and Hybrid qualification'
