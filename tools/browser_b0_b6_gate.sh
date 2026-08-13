#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
BASE="71cad04e01e4885f94acdfa5befd732147f89ea3"
EXPECTED_SOURCE="a970816b979dae1021853bc649f2b37ae8703403376f893d8ca4ab678725cfd3"
EXPECTED_CONTRACT="7bd78690444e7925913010e2187cab80dfb5631dc97e3b126047ede2cda5f4b7"
EXPECTED_RGBA8="a24be39e38b4fb654e79a2161177cf3670c354695f0b2fa13be677e8d1624dfd"
EXPECTED_RGBA16="fda03aa982372e8bb181ecf4e65910478bd8ad66ae08cf12cc1a5f89288673ba"
EXPECTED_ACCEL="a7e7d15e382f7c3ed91ffcf0b8a3567e9994d6a01afd0b329343775f4d3426ba"
cd "$ROOT"
[[ "$(git rev-parse HEAD)" == "$BASE" ]]

echo '### B0-B6: frozen lower-layer boundary'
make browser-b0-b6-lower-layer-verify

echo
echo '### B0/B1: native browser-export qualification'
make browser-check

echo
echo '### B0/B1: sanitizer qualification'
make browser-sanitize-verify

echo
echo '### B0-B6: browser contract/source verification'
make browser-contract-verify

echo
echo '### B0/B1/B3/B4: zero-import WASM memory/export qualification'
make browser-wasm-verify

echo
echo '### B2-B5: Firefox and Chrome real-browser qualification'
make browser-real-browser-verify

echo
echo '### B6: deterministic browser bridge archive'
make browser-reproducibility-verify

echo
echo '### B6: browser export performance'
make browser-benchmark-verify

echo
echo '### B6: frozen Reference Raster regression'
make renderer-check
make renderer-contract-verify
make renderer-wasm-golden-verify

echo
echo '### B6: frozen Geometry regression'
make geometry-check
make geometry-numerical-contract-verify

echo
echo '### B6: qualified C bridge regression'
make c-runtime-check

echo
echo '### B6: frozen Assembly regression suite'
make check

source_hash="$({ printf '%s\0' \
  include/arborcore/browser_surface.h \
  src/c/browser_surface.c \
  browser/precision_surface.js \
  browser/linear16_srgb8_bucket12.h \
  browser/arborcore-browser-surface-1.contract \
  | sort -z | xargs -0 sha256sum; } | sha256sum | awk '{print $1}')"
contract_hash="$(sha256sum "$ROOT/browser/arborcore-browser-surface-1.contract" | awk '{print $1}')"
accel_hash="$(sha256sum "$ROOT/browser/linear16_srgb8_bucket12.h" | awk '{print $1}')"
[[ "$source_hash" == "$EXPECTED_SOURCE" ]]
[[ "$contract_hash" == "$EXPECTED_CONTRACT" ]]
[[ "$accel_hash" == "$EXPECTED_ACCEL" ]]
archive_sha="$(awk -F= '$1=="BROWSER_ARCHIVE_SHA256" {print $2}' "$ROOT/build/browser-repro/result.env")"
wasm_sha="$(awk -F= '$1=="BROWSER_WASM_MODULE_SHA256" {print $2}' "$ROOT/build/browser-b0-b6-wasm/result.env")"
small_ns="$(awk -F= '$1=="EXPORT_MIXED_640X360_MEDIAN_NS" {print $2}' "$ROOT/build/browser-b6-performance/result.env")"
large_ns="$(awk -F= '$1=="EXPORT_OPAQUE_1280X720_MEDIAN_NS" {print $2}' "$ROOT/build/browser-b6-performance/result.env")"
firefox_present="$(awk -F= '$1=="B6_FIREFOX_PRESENTATION_640X360_MEDIAN_MS" {print $2}' "$ROOT/build/browser-b0-b6-browser/result.env")"
chrome_present="$(awk -F= '$1=="B6_CHROME_PRESENTATION_640X360_MEDIAN_MS" {print $2}' "$ROOT/build/browser-b0-b6-browser/result.env")"

cat > "$ROOT/build/browser-precision-surface-v1.env" <<EVIDENCE
BROWSER_PHASE=B0-B6
BROWSER_STATE=DELIVERY_CONTRACT_FROZEN_V1
BROWSER_CONTRACT_VERSION=1.0
BROWSER_BASE_COMMIT=$BASE
REFERENCE_RASTER_CONTRACT=V1_FROZEN
BROWSER_AUTHORITATIVE_PIXEL_FORMAT=RGBA16_UNORM_LE_PREMULTIPLIED_LINEAR_LIGHT
BROWSER_PRESENTATION_EXPORT_FORMAT=RGBA8_UNPREMULTIPLIED_SRGB
BROWSER_IMAGE_DATA_PIXEL_FORMAT=rgba-unorm8
BROWSER_IMAGE_DATA_COLOR_SPACE=srgb
BROWSER_CANVAS_PRESENTATION=CANVAS2D_PUTIMAGEDATA_REPLACE
BROWSER_WASM_IMPORT_POLICY=ZERO_IMPORT
BROWSER_WASM_MEMORY_VIEW_POLICY=REACQUIRE_AFTER_MEMORY_BUFFER_IDENTITY_CHANGE
BROWSER_DPR_POLICY=DEVICE_PIXEL_CONTENT_BOX_PREFERRED_WITH_DEFINED_FALLBACK
BROWSER_HYBRID_COORDINATE_POLICY=Q32_32_BIGINT_TO_CSS_DECIMAL_9_NEAREST_EVEN
BROWSER_SOURCE_SHA256=$source_hash
BROWSER_CONTRACT_SHA256=$contract_hash
B1_EXPORT_ACCELERATOR_SHA256=$accel_hash
B1_EXPORT_IMPLEMENTATION=EXACT_BUCKET12_FROZEN_RENDERER_EQUIVALENT
B1_RENDERER_SYMBOL_DEPENDENCY_COUNT=0
BROWSER_ARCHIVE_SHA256=$archive_sha
BROWSER_C_SURFACE_SYMBOL_COUNT=2
BROWSER_C_SURFACE_STATE=UNFROZEN_CONSTRUCTION
BROWSER_JS_SURFACE_STATE=UNFROZEN_CONSTRUCTION
B0_B1_WASM_RESULT=PASS
B1_EXPORT_RGBA8_GOLDEN_SHA256=$EXPECTED_RGBA8
REFERENCE_RGBA16_GOLDEN_SHA256=$EXPECTED_RGBA16
B5_REAL_BROWSER_RESULT=PASS
B5_BROWSER_COUNT=2
B6_BROWSER_EXPORT_PERFORMANCE_RESULT=PASS
B6_EXPORT_MIXED_640X360_MEDIAN_NS=$small_ns
B6_EXPORT_OPAQUE_1280X720_MEDIAN_NS=$large_ns
B6_FIREFOX_PRESENTATION_640X360_MEDIAN_MS=$firefox_present
B6_CHROME_PRESENTATION_640X360_MEDIAN_MS=$chrome_present
BROWSER_WASM_MODULE_SHA256=$wasm_sha
BROWSER_DELIVERY_CONTRACT_STATE=FROZEN
EVIDENCE
cat "$ROOT/build/browser-precision-surface-v1.env"
echo
echo '### BROWSER PRECISION SURFACE B0-B6 FREEZE GATE PASSED'
echo 'B0_B6_DECISION=FREEZE_BROWSER_DELIVERY_CONTRACT_V1'
