#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
WASM="$ROOT/build/browser-webgpu-postfreeze-wasm/arborcore-browser-postfreeze.wasm"
WEB="$ROOT/build/browser-webgpu-postfreeze-web"
OUT="$ROOT/build/browser-webgpu-postfreeze-live"
MODE="${1:-serve}"

prepare_web() {
  [[ -f "$WASM" ]] || { echo 'FAIL: run webgpu-postfreeze-wasm-verify first.' >&2; exit 1; }
  rm -rf "$WEB"
  mkdir -p "$WEB" "$OUT"
  cp "$WASM" "$WEB/arborcore-browser-postfreeze.wasm"
  cp "$ROOT/browser/precision_surface.js" "$WEB/precision_surface.js"
  cp "$ROOT/browser/webgpu_accelerator.js" "$WEB/webgpu_accelerator.js"
  cp "$ROOT/browser/webgpu_postfreeze_optimizer.js" "$WEB/webgpu_postfreeze_optimizer.js"
  cp "$ROOT/browser/webgpu_rgba16_exact_tables.js" "$WEB/webgpu_rgba16_exact_tables.js"
  cp "$ROOT/browser/webgpu_rgba16_experiment.js" "$WEB/webgpu_rgba16_experiment.js"
  cp "$ROOT/tests/browser/webgpu_postfreeze_browser_test.html" "$WEB/webgpu_postfreeze_browser_test.html"
}

verify_evidence() {
  local result="$OUT/result.env"
  [[ -s "$result" ]] || { echo 'FAIL: post-W6 live evidence absent; run make webgpu-postfreeze-live-browser-verify.' >&2; exit 1; }
  [[ "$(awk -F= '$1=="OPT4_LIVE_BROWSER_RESULT" {print $2}' "$result")" == PASS ]]
  [[ "$(awk -F= '$1=="OPT4_LIVE_BROWSER_COUNT" {print $2}' "$result")" == 2 ]]
  local webgpu_count
  webgpu_count="$(awk -F= '$1=="OPT4_LIVE_WEBGPU_BROWSER_COUNT" {print $2}' "$result")"
  cat "$result"
  [[ "$webgpu_count" -ge 1 ]] || { echo 'OPT4_WEBGPU_ADMISSION=REVIEW_NO_LIVE_WEBGPU_BROWSER' >&2; exit 2; }
  echo 'OPT4_WEBGPU_ADMISSION=PASS'
  echo 'PASS: OPT1-OPT4 live normal-browser measurement/equivalence evidence'
}

case "$MODE" in
  serve)
    prepare_web
    rm -rf "$OUT"
    mkdir -p "$OUT"
    ARBORCORE_ROOT="$ROOT" node "$ROOT/tools/webgpu_postfreeze_real_browser_runner.mjs"
    {
      printf 'FIREFOX_BINARY_VERSION='; firefox --version 2>/dev/null | sed -n '1p' || echo UNAVAILABLE
      printf 'CHROME_BINARY_VERSION='; google-chrome-stable --version 2>/dev/null | sed -n '1p' || echo UNAVAILABLE
    } > "$OUT/versions.env"
    cat "$OUT/versions.env"
    verify_evidence
    ;;
  evidence)
    verify_evidence
    ;;
  *) echo "usage: $0 [serve|evidence]" >&2; exit 64 ;;
esac
