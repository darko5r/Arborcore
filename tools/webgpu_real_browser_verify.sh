#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
WASM_OUT="$ROOT/build/browser-b0-b6-wasm/arborcore-browser-reference.wasm"
WEB="$ROOT/build/browser-webgpu-web"
OUT="$ROOT/build/browser-webgpu-live"
MODE="${1:-serve}"

prepare_web() {
  command -v node >/dev/null 2>&1 || { echo 'W4_LIVE_BROWSER_RESULT=REVIEW_NO_NODE' >&2; exit 1; }
  [[ -f "$WASM_OUT" ]] || { echo 'FAIL: run browser-wasm-verify first.' >&2; exit 1; }
  rm -rf "$WEB"
  mkdir -p "$WEB" "$OUT"
  cp "$WASM_OUT" "$WEB/arborcore-browser-reference.wasm"
  cp "$ROOT/browser/precision_surface.js" "$WEB/precision_surface.js"
  cp "$ROOT/browser/webgpu_accelerator.js" "$WEB/webgpu_accelerator.js"
  cp "$ROOT/tests/browser/webgpu_accelerator_browser_test.html" "$WEB/webgpu_accelerator_browser_test.html"
}

verify_evidence() {
  local result="$OUT/result.env"
  [[ -s "$result" ]] || { echo 'FAIL: live-browser evidence is absent; run make webgpu-live-browser-verify first.' >&2; exit 1; }
  [[ "$(awk -F= '$1=="W4_LIVE_REAL_BROWSER_RESULT" {print $2}' "$result")" == PASS ]]
  [[ "$(awk -F= '$1=="W4_LIVE_BROWSER_COUNT" {print $2}' "$result")" == 2 ]]
  [[ "$(awk -F= '$1=="W4_LIVE_FIREFOX_SECURE_CONTEXT" {print $2}' "$result")" == true ]]
  [[ "$(awk -F= '$1=="W4_LIVE_CHROME_SECURE_CONTEXT" {print $2}' "$result")" == true ]]
  local webgpu_count
  webgpu_count="$(awk -F= '$1=="W4_LIVE_WEBGPU_BROWSER_COUNT" {print $2}' "$result")"
  cat "$result"
  if [[ "$webgpu_count" -lt 1 ]]; then
    echo 'W4_LIVE_WEBGPU_ADMISSION=REVIEW_NO_LIVE_WEBGPU_BROWSER' >&2
    exit 2
  fi
  echo 'W4_LIVE_WEBGPU_ADMISSION=PASS'
  echo 'PASS: W2-W4 normal-profile WebGPU/fallback qualification evidence'
}

case "$MODE" in
  serve)
    prepare_web
    rm -rf "$OUT"
    mkdir -p "$OUT"
    set +e
    ARBORCORE_ROOT="$ROOT" node "$ROOT/tools/webgpu_real_browser_runner.mjs"
    runner_exit=$?
    set -e
    if [[ "$runner_exit" -ne 0 ]]; then
      [[ -s "$OUT/result.env" ]] && cat "$OUT/result.env"
      exit "$runner_exit"
    fi
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
  *)
    echo "usage: $0 [serve|evidence]" >&2
    exit 64
    ;;
esac
