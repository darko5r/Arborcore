#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
CONTRACT="$ROOT/browser/arborcore-browser-webgpu-1.contract"
JS="$ROOT/browser/webgpu_accelerator.js"
BASE_CONTRACT="$ROOT/browser/arborcore-browser-surface-1.contract"
EXPECTED_BASE_CONTRACT="7bd78690444e7925913010e2187cab80dfb5631dc97e3b126047ede2cda5f4b7"
EXPECTED_BASE_SOURCE="a970816b979dae1021853bc649f2b37ae8703403376f893d8ca4ab678725cfd3"
EXPECTED_WEBGPU_CONTRACT="a0d95467817504dbdc4db38c22b10c210b208e0604857ab752aa2730e97a144b"
EXPECTED_WEBGPU_JS="b42d48d8f30a9c3cd11c63c68219effe4ad7126d511f0233583fd1a1a59e38bb"
EXPECTED_WEBGPU_SOURCE="351cfe3e3240661a3251b25a0ecbd61dd724edfd5fbbb39dab8795199784e666"
cd "$ROOT"

base_source="$({ printf '%s\0' \
  include/arborcore/browser_surface.h \
  src/c/browser_surface.c \
  browser/precision_surface.js \
  browser/linear16_srgb8_bucket12.h \
  browser/arborcore-browser-surface-1.contract \
  | LC_ALL=C sort -z | xargs -0 sha256sum; } | sha256sum | awk '{print $1}')"
base_contract="$(sha256sum "$BASE_CONTRACT" | awk '{print $1}')"
webgpu_contract="$(sha256sum "$CONTRACT" | awk '{print $1}')"
webgpu_js="$(sha256sum "$JS" | awk '{print $1}')"
webgpu_source="$({ printf '%s\0' \
  browser/arborcore-browser-webgpu-1.contract \
  browser/webgpu_accelerator.js \
  | LC_ALL=C sort -z | xargs -0 sha256sum; } | sha256sum | awk '{print $1}')"

printf 'base_browser_source_sha256=%s\n' "$base_source"
printf 'base_browser_contract_sha256=%s\n' "$base_contract"
printf 'webgpu_contract_sha256=%s\n' "$webgpu_contract"
printf 'webgpu_js_sha256=%s\n' "$webgpu_js"
printf 'webgpu_source_sha256=%s\n' "$webgpu_source"

[[ "$base_source" == "$EXPECTED_BASE_SOURCE" ]]
[[ "$base_contract" == "$EXPECTED_BASE_CONTRACT" ]]
[[ "$webgpu_contract" == "$EXPECTED_WEBGPU_CONTRACT" ]]
[[ "$webgpu_js" == "$EXPECTED_WEBGPU_JS" ]]
[[ "$webgpu_source" == "$EXPECTED_WEBGPU_SOURCE" ]]

grep -qx 'WEBGPU_CONTRACT_VERSION=1.0' "$CONTRACT"
grep -qx 'BASE_BROWSER_FREEZE_COMMIT=80395372243fcfb2995ed03beca8a3af3e873964' "$CONTRACT"
grep -qx 'BASE_BROWSER_FREEZE_TREE=b571df944b366f6ee1cdbbf3c55c12d507ba41f9' "$CONTRACT"
grep -qx "BASE_BROWSER_CONTRACT_SHA256=$EXPECTED_BASE_CONTRACT" "$CONTRACT"
grep -qx "BASE_BROWSER_SOURCE_SHA256=$EXPECTED_BASE_SOURCE" "$CONTRACT"
grep -qx 'ACCELERATOR_ROLE=OPTIONAL_PRESENTATION_ACCELERATOR_ONLY' "$CONTRACT"
grep -qx 'WEBGPU_UPLOAD_TEXTURE_FORMAT=rgba8unorm' "$CONTRACT"
grep -qx 'WEBGPU_CANVAS_FORMAT_POLICY=NAVIGATOR_GPU_PREFERRED_CANVAS_FORMAT' "$CONTRACT"
grep -qx 'WEBGPU_CANVAS_COLOR_SPACE=srgb' "$CONTRACT"
grep -qx 'WEBGPU_TEXTURE_SAMPLING_POLICY=NONE_TEXTURE_LOAD_EXACT_TEXEL' "$CONTRACT"
grep -qx 'WEBGPU_DEVICE_LOSS_POLICY=IMMEDIATE_FROZEN_CANVAS2D_FALLBACK_REACQUIRE_ADAPTER_ON_RECOVERY' "$CONTRACT"
grep -qx 'WEBGPU_SECURE_CONTEXT_POLICY=LOOPBACK_127_0_0_1_AND_WINDOW_ISSECURECONTEXT_REQUIRED' "$CONTRACT"
grep -qx 'WEBGPU_BROWSER_ADMISSION=LIVE_USER_REFERENCE_BROWSERS_PASS_AND_AT_LEAST_ONE_LIVE_WEBGPU_ACCELERATOR_PASS' "$CONTRACT"
grep -qx 'WEBGPU_LIVE_BROWSER_POLICY=LIVE_USER_BROWSER_ENVIRONMENT_MANUAL_OPEN_NO_PROFILE_INJECTION' "$CONTRACT"
grep -qx 'WEBGPU_ISOLATED_BROWSER_POLICY=CLEAN_PROFILE_ADDITIONAL_REPRODUCIBILITY_AND_FALLBACK_EVIDENCE' "$CONTRACT"
grep -qx 'WEBGPU_ISOLATED_WEBGPU_REQUIREMENT=NONE' "$CONTRACT"
grep -qx 'WEBGPU_UNAVAILABLE_BROWSER_POLICY=QUALIFIED_FROZEN_CANVAS2D_FALLBACK' "$CONTRACT"
grep -qx 'WEBGPU_RUNTIME_FAILURE_QUALIFICATION_POLICY=DEVICE_LOSS_OR_PLATFORM_ALLOCATION_FAILURE_MAY_FALL_BACK_NONLOSS_PROGRAMMING_FAILURE_FATAL' "$CONTRACT"
grep -qx 'WEBGPU_PERFORMANCE_POLICY=DIAGNOSTIC_UNTIL_REAL_HOST_BASELINE_ACCEPTED' "$CONTRACT"
grep -qx 'HTML_CSS_POLICY=PARALLEL_INDEPENDENT_RENDERING_PATH' "$CONTRACT"
grep -qx 'WEBGPU_CONTRACT_STATE=FROZEN' "$CONTRACT"

grep -q "textureLoad(source_texture" "$JS"
grep -q "navigator.gpu.getPreferredCanvasFormat\|gpu.getPreferredCanvasFormat" "$JS"
grep -q "device.lost" "$JS"
grep -q "context.unconfigure" "$JS"
grep -q "queue.writeTexture" "$JS"
grep -q "copyTextureToBuffer" "$JS"
grep -q "PrecisionSurfacePresenter" "$JS"
echo 'PASS: frozen B0-B6 browser delivery identities remain byte-exact'
echo 'PASS: WebGPU Accelerator Contract v1 frozen source and qualification invariants'
