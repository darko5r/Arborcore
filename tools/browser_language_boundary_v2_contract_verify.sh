#!/usr/bin/env bash
set -euo pipefail

ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"

EXPECTED_WEBGPU_CONTRACT="a0d95467817504dbdc4db38c22b10c210b208e0604857ab752aa2730e97a144b"
EXPECTED_WEBGPU_JS="b42d48d8f30a9c3cd11c63c68219effe4ad7126d511f0233583fd1a1a59e38bb"
EXPECTED_BROWSER_CONTRACT="7bd78690444e7925913010e2187cab80dfb5631dc97e3b126047ede2cda5f4b7"

HISTORY="$ROOT/tools/browser_v1_history_verify.sh"

history="$(
  ARBORCORE_ROOT="$ROOT" \
    bash "$HISTORY"
)"

webgpu_js="$(
  printf '%s\n' "$history" |
    awk -F= '$1=="WEBGPU_V1_JS_SHA256" {print $2}'
)"

precision_history="$(
  printf '%s\n' "$history" |
    awk -F= '$1=="PRECISION_V1_HISTORY_VERIFIED" {print $2}'
)"

webgpu_history="$(
  printf '%s\n' "$history" |
    awk -F= '$1=="WEBGPU_V1_HISTORY_VERIFIED" {print $2}'
)"

[[ "$precision_history" == "PASS" ]]
[[ "$webgpu_history" == "PASS" ]]
[[ "$webgpu_js" == "$EXPECTED_WEBGPU_JS" ]]

[[ "$(
  sha256sum browser/arborcore-browser-webgpu-1.contract |
    awk '{print $1}'
)" == "$EXPECTED_WEBGPU_CONTRACT" ]]

[[ "$(
  sha256sum browser/arborcore-browser-surface-1.contract |
    awk '{print $1}'
)" == "$EXPECTED_BROWSER_CONTRACT" ]]

printf 'LBV2_HISTORICAL_WEBGPU_JS_SHA256=%s\n' "$webgpu_js"
printf 'LBV2_PRECISION_V1_HISTORY_VERIFIED=%s\n' "$precision_history"
printf 'LBV2_WEBGPU_V1_HISTORY_VERIFIED=%s\n' "$webgpu_history"

grep -qx \
  'LB_JS_ROLE=BROWSER_HOST_SYSCALL_SHIM_ONLY' \
  browser/arborcore-browser-language-boundary-2.contract

grep -qx \
  'LB_AUTHORITATIVE_JS_LOGIC=ZERO' \
  browser/arborcore-browser-language-boundary-2.contract

grep -qx \
  'LB_PORTABLE_BROWSER_AUTHORITY=C_WASM' \
  browser/arborcore-browser-language-boundary-2.contract

grep -qx \
  'LB_GPU_AUTHORITY=WGSL' \
  browser/arborcore-browser-language-boundary-2.contract

grep -qx \
  'ARBORCORE_BROWSER_LANGUAGE_BOUNDARY_VERSION=2.0' \
  browser/arborcore-browser-language-boundary-2.contract

grep -qx \
  'LB_CONTRACT_STATE=FROZEN' \
  browser/arborcore-browser-language-boundary-2.contract

grep -qx \
  'LB_DELIVERY_STATE=FROZEN_V2' \
  browser/arborcore-browser-language-boundary-2.contract

! grep -q \
  'CANDIDATE' \
  browser/arborcore-browser-language-boundary-2.contract

python tools/browser_language_boundary_v2_js_audit.py

for book in \
  'Advanced Topics in C' \
  'Extreme C' \
  'Clean Code'
do
  grep -q "$book" docs/SOURCES.md
done

! grep -qE \
  '@vertex|@fragment|@compute|linear16_to_srgb8|unpremultiply16' \
  browser/arborcore_host.js

[[ -s browser/shaders/rgba8_present.wgsl ]]
[[ -s browser/shaders/rgba16_exact_convert.wgsl ]]

source_sha="$({
  printf '%s\0' \
    include/arborcore/browser_host_v2.h \
    src/c/browser_host_v2.c \
    browser/arborcore_host.js \
    browser/shaders/rgba8_present.wgsl \
    browser/shaders/rgba16_exact_convert.wgsl \
    browser/arborcore-browser-language-boundary-2.contract |
    LC_ALL=C sort -z |
    xargs -0 sha256sum
} | sha256sum | awk '{print $1}')"

printf 'LBV2_SOURCE_SHA256=%s\n' "$source_sha"

echo 'PASS: Browser Language Boundary v2 contract and language-authority invariants'
echo 'PASS: frozen Browser/WebGPU v1 JavaScript identities are verified from Git history'
