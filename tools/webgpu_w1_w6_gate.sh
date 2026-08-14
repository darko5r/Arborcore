#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
BASE="80395372243fcfb2995ed03beca8a3af3e873964"
BASE_TREE="b571df944b366f6ee1cdbbf3c55c12d507ba41f9"
EXPECTED_BASE_SOURCE="a970816b979dae1021853bc649f2b37ae8703403376f893d8ca4ab678725cfd3"
EXPECTED_BASE_CONTRACT="7bd78690444e7925913010e2187cab80dfb5631dc97e3b126047ede2cda5f4b7"
EXPECTED_WEBGPU_SOURCE="351cfe3e3240661a3251b25a0ecbd61dd724edfd5fbbb39dab8795199784e666"
EXPECTED_WEBGPU_CONTRACT="a0d95467817504dbdc4db38c22b10c210b208e0604857ab752aa2730e97a144b"
EXPECTED_RGBA8="a24be39e38b4fb654e79a2161177cf3670c354695f0b2fa13be677e8d1624dfd"
EXPECTED_OPAQUE="c21b35e3f28e676cedf24c13575a7346682e101a2d26aad9598d0cdbcee9ee3b"
cd "$ROOT"

[[ "$(git rev-parse HEAD)" == "$BASE" ]]
[[ "$(git rev-parse 'HEAD^{tree}')" == "$BASE_TREE" ]]

echo '### W0: qualification-host inventory'
make webgpu-w0-host-verify

echo
echo '### W0/W1: frozen B0-B6 boundary'
make browser-b0-b6-lower-layer-verify
make browser-contract-verify

echo
echo '### W1: WebGPU accelerator contract/source qualification'
make webgpu-contract-verify
make webgpu-js-check

echo
echo '### W2-W4: live normal-profile browser evidence'
make webgpu-live-browser-evidence-verify

echo
echo '### W4: isolated clean-profile reproducibility/fallback qualification'
make webgpu-isolated-browser-verify

echo
echo '### W5: live WebGPU performance baseline'
make webgpu-benchmark-verify

echo
echo '### W5: deterministic WebGPU source archive'
make webgpu-reproducibility-verify

echo
echo '### W6: frozen B0-B6 reference-browser regression'
make browser-check
make browser-sanitize-verify
make browser-wasm-verify
make browser-real-browser-verify
make browser-reproducibility-verify
make browser-benchmark-verify

echo
echo '### W6: frozen Reference Raster regression'
make renderer-check
make renderer-contract-verify
make renderer-wasm-golden-verify

echo
echo '### W6: frozen Geometry regression'
make geometry-check
make geometry-numerical-contract-verify

echo
echo '### W6: qualified C bridge regression'
make c-runtime-check

echo
echo '### W6: frozen Assembly regression suite'
make check

base_source="$({ printf '%s\0' \
  include/arborcore/browser_surface.h \
  src/c/browser_surface.c \
  browser/precision_surface.js \
  browser/linear16_srgb8_bucket12.h \
  browser/arborcore-browser-surface-1.contract \
  | LC_ALL=C sort -z | xargs -0 sha256sum; } | sha256sum | awk '{print $1}')"
base_contract="$(sha256sum browser/arborcore-browser-surface-1.contract | awk '{print $1}')"
webgpu_source="$({ printf '%s\0' \
  browser/arborcore-browser-webgpu-1.contract \
  browser/webgpu_accelerator.js \
  | LC_ALL=C sort -z | xargs -0 sha256sum; } | sha256sum | awk '{print $1}')"
webgpu_contract="$(sha256sum browser/arborcore-browser-webgpu-1.contract | awk '{print $1}')"
[[ "$base_source" == "$EXPECTED_BASE_SOURCE" ]]
[[ "$base_contract" == "$EXPECTED_BASE_CONTRACT" ]]
[[ "$webgpu_source" == "$EXPECTED_WEBGPU_SOURCE" ]]
[[ "$webgpu_contract" == "$EXPECTED_WEBGPU_CONTRACT" ]]

live_result="$ROOT/build/browser-webgpu-live/result.env"
isolated_result="$ROOT/build/browser-webgpu-isolated/result.env"
perf_result="$ROOT/build/browser-webgpu-live/performance.env"
repro_result="$ROOT/build/browser-webgpu-repro/result.env"
[[ -s "$live_result" && -s "$isolated_result" && -s "$perf_result" && -s "$repro_result" ]]
live_webgpu_count="$(awk -F= '$1=="W4_LIVE_WEBGPU_BROWSER_COUNT" {print $2}' "$live_result")"
[[ "$live_webgpu_count" -ge 1 ]]
[[ "$(awk -F= '$1=="W4_LIVE_BROWSER_COUNT" {print $2}' "$live_result")" == 2 ]]
[[ "$(awk -F= '$1=="W4_ISOLATED_BROWSER_COUNT" {print $2}' "$isolated_result")" == 2 ]]
archive_sha="$(awk -F= '$1=="WEBGPU_W1_W6_ARCHIVE_SHA256" {print $2}' "$repro_result")"
live_fallback_count="$(awk -F= '$1=="W4_LIVE_FALLBACK_BROWSER_COUNT" {print $2}' "$live_result")"
firefox_mode="$(awk -F= '$1=="W4_LIVE_FIREFOX_QUALIFIED_MODE" {print $2}' "$live_result")"
firefox_failure_class="$(awk -F= '$1=="W4_LIVE_FIREFOX_WEBGPU_RUNTIME_FAILURE_CLASS" {print $2}' "$live_result")"
chrome_mode="$(awk -F= '$1=="W4_LIVE_CHROME_QUALIFIED_MODE" {print $2}' "$live_result")"
chrome_failure_class="$(awk -F= '$1=="W4_LIVE_CHROME_WEBGPU_RUNTIME_FAILURE_CLASS" {print $2}' "$live_result")"
isolated_webgpu_count="$(awk -F= '$1=="W4_ISOLATED_WEBGPU_BROWSER_COUNT" {print $2}' "$isolated_result")"
isolated_fallback_count="$(awk -F= '$1=="W4_ISOLATED_FALLBACK_BROWSER_COUNT" {print $2}' "$isolated_result")"
perf_policy="$(awk -F= '$1=="W5_PERFORMANCE_POLICY" {print $2}' "$perf_result")"

for browser in FIREFOX CHROME; do
  mode="$(awk -F= -v key="W4_LIVE_${browser}_QUALIFIED_MODE" '$1==key {print $2}' "$live_result")"
  fallback_hash="$(awk -F= -v key="W4_LIVE_${browser}_FALLBACK_SHA256" '$1==key {print $2}' "$live_result")"
  secure_context="$(awk -F= -v key="W4_LIVE_${browser}_SECURE_CONTEXT" '$1==key {print $2}' "$live_result")"
  runtime_failure_class="$(awk -F= -v key="W4_LIVE_${browser}_WEBGPU_RUNTIME_FAILURE_CLASS" '$1==key {print $2}' "$live_result")"
  [[ "$secure_context" == true ]]
  if [[ "$mode" == webgpu ]]; then
    [[ "$runtime_failure_class" == NONE ]]
    upload_hash="$(awk -F= -v key="W4_LIVE_${browser}_UPLOAD_RGBA8_SHA256" '$1==key {print $2}' "$live_result")"
    opaque_hash="$(awk -F= -v key="W4_LIVE_${browser}_OPAQUE_WEBGPU_SHA256" '$1==key {print $2}' "$live_result")"
    recovery="$(awk -F= -v key="W4_LIVE_${browser}_RECOVERY_RESULT" '$1==key {print $2}' "$live_result")"
    [[ "$upload_hash" == "$EXPECTED_RGBA8" ]]
    [[ "$opaque_hash" == "$EXPECTED_OPAQUE" ]]
    [[ "$fallback_hash" == "$EXPECTED_OPAQUE" ]]
    [[ "$recovery" == true ]]
  else
    [[ "$fallback_hash" == "$EXPECTED_OPAQUE" ]]
    [[ "$runtime_failure_class" == NONE || "$runtime_failure_class" == DEVICE_LOSS_OR_PLATFORM_ALLOCATION_FAILURE ]]
  fi
done

cat > "$ROOT/build/browser-webgpu-w1-w6.env" <<EVIDENCE
WEBGPU_PHASE=W0-W6
WEBGPU_STATE=ACCELERATOR_CONTRACT_FROZEN_V1
WEBGPU_CONTRACT_VERSION=1.0
WEBGPU_BASE_COMMIT=$BASE
WEBGPU_BASE_TREE=$BASE_TREE
BASE_BROWSER_DELIVERY_CONTRACT_STATE=FROZEN_V1
BASE_BROWSER_SOURCE_SHA256=$base_source
BASE_BROWSER_CONTRACT_SHA256=$base_contract
WEBGPU_SOURCE_SHA256=$webgpu_source
WEBGPU_CONTRACT_SHA256=$webgpu_contract
WEBGPU_ACCELERATOR_INPUT_FORMAT=RGBA8_UNPREMULTIPLIED_SRGB
WEBGPU_ACCELERATOR_INPUT_GOLDEN_SHA256=$EXPECTED_RGBA8
WEBGPU_ROLE=OPTIONAL_PRESENTATION_ACCELERATOR_ONLY
WEBGPU_FALLBACK=FROZEN_CANVAS2D_REFERENCE
W4_LIVE_REAL_BROWSER_RESULT=PASS
W4_LIVE_BROWSER_COUNT=2
W4_LIVE_WEBGPU_BROWSER_COUNT=$live_webgpu_count
W4_LIVE_FALLBACK_BROWSER_COUNT=$live_fallback_count
W4_LIVE_FIREFOX_QUALIFIED_MODE=$firefox_mode
W4_LIVE_FIREFOX_WEBGPU_RUNTIME_FAILURE_CLASS=$firefox_failure_class
W4_LIVE_CHROME_QUALIFIED_MODE=$chrome_mode
W4_LIVE_CHROME_WEBGPU_RUNTIME_FAILURE_CLASS=$chrome_failure_class
W4_ISOLATED_BROWSER_RESULT=PASS
W4_ISOLATED_BROWSER_COUNT=2
W4_ISOLATED_WEBGPU_BROWSER_COUNT=$isolated_webgpu_count
W4_ISOLATED_FALLBACK_BROWSER_COUNT=$isolated_fallback_count
W5_WEBGPU_PERFORMANCE_RESULT=PASS_DIAGNOSTIC_BASELINE
W5_PERFORMANCE_POLICY=$perf_policy
WEBGPU_W1_W6_ARCHIVE_SHA256=$archive_sha
WEBGPU_JS_SURFACE_STATE=UNFROZEN_CONSTRUCTION
WEBGPU_CONTRACT_STATE=FROZEN
WEBGPU_DELIVERY_STATE=FROZEN_V1
EVIDENCE
cat "$ROOT/build/browser-webgpu-w1-w6.env"
echo
echo '### BROWSER WEBGPU ACCELERATOR W0-W6 FREEZE GATE PASSED'
echo 'W0_W6_DECISION=FREEZE_WEBGPU_ACCELERATOR_CONTRACT_V1'
