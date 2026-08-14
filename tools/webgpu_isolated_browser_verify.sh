#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
WASM_OUT="$ROOT/build/browser-b0-b6-wasm/arborcore-browser-reference.wasm"
WEB="$ROOT/build/browser-webgpu-web"
OUT="$ROOT/build/browser-webgpu-isolated"
SELECTION="${ARBORCORE_WEBGPU_ISOLATED_BROWSER:-all}"
case "$SELECTION" in
  all) expected_count=2 ;;
  firefox|chrome) expected_count=1 ;;
  *) echo "FAIL: invalid ARBORCORE_WEBGPU_ISOLATED_BROWSER=$SELECTION" >&2; exit 1 ;;
esac
command -v node >/dev/null 2>&1 || { echo 'W4_ISOLATED_BROWSER_RESULT=REVIEW_NO_NODE' >&2; exit 1; }
[[ -f "$WASM_OUT" ]] || { echo 'FAIL: run browser-wasm-verify first.' >&2; exit 1; }
if [[ "$SELECTION" == all || "$SELECTION" == firefox ]]; then
  command -v firefox >/dev/null 2>&1 || { echo 'W4_ISOLATED_BROWSER_RESULT=REVIEW_NO_FIREFOX' >&2; exit 1; }
fi
if [[ "$SELECTION" == all || "$SELECTION" == chrome ]]; then
  command -v google-chrome-stable >/dev/null 2>&1 || { echo 'W4_ISOLATED_BROWSER_RESULT=REVIEW_NO_CHROME' >&2; exit 1; }
fi
rm -rf "$WEB" "$OUT"
mkdir -p "$WEB" "$OUT"
cp "$WASM_OUT" "$WEB/arborcore-browser-reference.wasm"
cp "$ROOT/browser/precision_surface.js" "$WEB/precision_surface.js"
cp "$ROOT/browser/webgpu_accelerator.js" "$WEB/webgpu_accelerator.js"
cp "$ROOT/tests/browser/webgpu_accelerator_browser_test.html" "$WEB/webgpu_accelerator_browser_test.html"
FIREFOX_PATH="$(command -v firefox)" \
GOOGLE_CHROME_STABLE_PATH="$(command -v google-chrome-stable)" \
ARBORCORE_WEBGPU_ISOLATED_BROWSER="$SELECTION" \
ARBORCORE_ROOT="$ROOT" \
  node "$ROOT/tools/webgpu_isolated_browser_runner.mjs"
[[ "$(awk -F= '$1=="W4_ISOLATED_BROWSER_RESULT" {print $2}' "$OUT/result.env")" == PASS ]]
[[ "$(awk -F= '$1=="W4_ISOLATED_BROWSER_COUNT" {print $2}' "$OUT/result.env")" == "$expected_count" ]]
cat "$OUT/result.env"
firefox_launch_mode=headed-clean-profile
if [[ "${ARBORCORE_WEBGPU_ISOLATED_FIREFOX_HEADLESS:-0}" == 1 ]]; then
  firefox_launch_mode=headless-clean-profile-diagnostic
fi
{
  printf 'FIREFOX_BINARY_VERSION='; firefox --version 2>/dev/null | sed -n '1p'
  printf 'CHROME_BINARY_VERSION='; google-chrome-stable --version 2>/dev/null | sed -n '1p'
  printf 'ISOLATED_BROWSER_SELECTION=%s\n' "$SELECTION"
  printf 'ISOLATED_FIREFOX_LAUNCH_MODE=%s\n' "$firefox_launch_mode"
} > "$OUT/versions.env"
cat "$OUT/versions.env"
echo 'PASS: W4 isolated clean-profile reproducibility/fallback qualification'
