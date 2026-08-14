#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
CONTRACT="$ROOT/browser/arborcore-browser-webgpu-postfreeze-optimization-1.contract"
BASE_CONTRACT="$ROOT/browser/arborcore-browser-webgpu-1.contract"
BASE_JS="$ROOT/browser/webgpu_accelerator.js"
EXPECTED_HEAD="66574f02102c0b5bdcc97590ddfb3b30e298cf97"
EXPECTED_TREE="7b93cfa50f3a0d70b9e8934c187099ab29185a4b"
EXPECTED_W_CONTRACT="a0d95467817504dbdc4db38c22b10c210b208e0604857ab752aa2730e97a144b"
EXPECTED_W_JS="b42d48d8f30a9c3cd11c63c68219effe4ad7126d511f0233583fd1a1a59e38bb"
EXPECTED_W_SOURCE="351cfe3e3240661a3251b25a0ecbd61dd724edfd5fbbb39dab8795199784e666"
EXPECTED_B0_CONTRACT="7bd78690444e7925913010e2187cab80dfb5631dc97e3b126047ede2cda5f4b7"
EXPECTED_B0_SOURCE="a970816b979dae1021853bc649f2b37ae8703403376f893d8ca4ab678725cfd3"
cd "$ROOT"

[[ "$(git rev-parse HEAD)" == "$EXPECTED_HEAD" ]]
[[ "$(git rev-parse 'HEAD^{tree}')" == "$EXPECTED_TREE" ]]

w_contract="$(sha256sum "$BASE_CONTRACT" | awk '{print $1}')"
w_js="$(sha256sum "$BASE_JS" | awk '{print $1}')"
w_source="$({ printf '%s\0' browser/arborcore-browser-webgpu-1.contract browser/webgpu_accelerator.js | LC_ALL=C sort -z | xargs -0 sha256sum; } | sha256sum | awk '{print $1}')"
b0_contract="$(sha256sum browser/arborcore-browser-surface-1.contract | awk '{print $1}')"
b0_source="$({ printf '%s\0' include/arborcore/browser_surface.h src/c/browser_surface.c browser/precision_surface.js browser/linear16_srgb8_bucket12.h browser/arborcore-browser-surface-1.contract | LC_ALL=C sort -z | xargs -0 sha256sum; } | sha256sum | awk '{print $1}')"

printf 'frozen_webgpu_contract_sha256=%s\n' "$w_contract"
printf 'frozen_webgpu_js_sha256=%s\n' "$w_js"
printf 'frozen_webgpu_source_sha256=%s\n' "$w_source"
printf 'frozen_browser_contract_sha256=%s\n' "$b0_contract"
printf 'frozen_browser_source_sha256=%s\n' "$b0_source"
[[ "$w_contract" == "$EXPECTED_W_CONTRACT" ]]
[[ "$w_js" == "$EXPECTED_W_JS" ]]
[[ "$w_source" == "$EXPECTED_W_SOURCE" ]]
[[ "$b0_contract" == "$EXPECTED_B0_CONTRACT" ]]
[[ "$b0_source" == "$EXPECTED_B0_SOURCE" ]]

grep -qx 'POSTFREEZE_OPT_CONTRACT_VERSION=1.0' "$CONTRACT"
grep -qx 'POSTFREEZE_OPT_PHASE=OPT0-OPT5' "$CONTRACT"
grep -qx "POSTFREEZE_OPT_BASE_COMMIT=$EXPECTED_HEAD" "$CONTRACT"
grep -qx "POSTFREEZE_OPT_BASE_TREE=$EXPECTED_TREE" "$CONTRACT"
grep -qx "FROZEN_WEBGPU_V1_CONTRACT_SHA256=$EXPECTED_W_CONTRACT" "$CONTRACT"
grep -qx "FROZEN_WEBGPU_V1_JS_SHA256=$EXPECTED_W_JS" "$CONTRACT"
grep -qx 'OPT1_COMPLETION_CALLBACK_POLICY=DIAGNOSTIC_NOT_GPU_EXECUTION_TIME' "$CONTRACT"
grep -qx 'OPT2_SCOPE=OPTIONAL_V1_COMPATIBLE_RGBA8_PRESENTER_IMPLEMENTATION_ONLY' "$CONTRACT"
grep -qx 'OPT3_CONTRACT_ADMISSION=NONE_REQUIRES_FUTURE_CONTRACT_REVISION' "$CONTRACT"
grep -qx 'OPT4_BROAD_BROWSER_RELEASE_CLAIM=NOT_ADMITTED_UNTIL_FORMAL_WEBKIT_SAFARI_QUALIFICATION' "$CONTRACT"
grep -qx 'FROZEN_LOWER_LAYER_POLICY=NO_MODIFICATION' "$CONTRACT"
grep -qx 'OPT2_ADMISSION_STATE=ADMITTED_OPTIONAL_V1_COMPATIBLE_IMPLEMENTATION' "$CONTRACT"
grep -qx 'OPT2_INTEGRATION_POLICY=OPTIONAL_PARALLEL_IMPLEMENTATION_FROZEN_WEBGPU_V1_REFERENCE_UNCHANGED' "$CONTRACT"
grep -qx 'OPT3_EXPERIMENT_STATE=RETAINED_EXPERIMENTAL_FUTURE_CONTRACT_REVISION_REQUIRED' "$CONTRACT"
grep -qx 'POSTFREEZE_OPT_CONTRACT_STATE=FROZEN' "$CONTRACT"
grep -qx 'POSTFREEZE_OPT_DELIVERY_STATE=FROZEN_V1' "$CONTRACT"

grep -q 'class OptimizedRgba8WebGpuPresenter' browser/webgpu_postfreeze_optimizer.js
grep -q 'class DirectRgba16WebGpuExperiment' browser/webgpu_rgba16_experiment.js
grep -q "EXPERIMENTAL_NOT_WEBGPU_V1_CONTRACT" browser/webgpu_rgba16_experiment.js
grep -q '65536_OPAQUE_LINEAR16_PLUS_16384_MIXED_ALPHA' "$CONTRACT"

ARBORCORE_ROOT="$ROOT" node tools/webgpu_postfreeze_table_verify.mjs

frozen_dirty="$(git status --short -- \
  browser/arborcore-browser-webgpu-1.contract \
  browser/webgpu_accelerator.js \
  browser/arborcore-browser-surface-1.contract \
  browser/linear16_srgb8_bucket12.h \
  browser/precision_surface.js \
  include/arborcore/browser_surface.h \
  src/c/browser_surface.c \
  geometry/arborcore-geometry-1.contract \
  renderer/arborcore-renderer-1.contract \
  src/c/geometry.c \
  src/c/renderer.c \
  src/asm)"
[[ -z "$frozen_dirty" ]] || { printf '%s\n' "$frozen_dirty" >&2; echo 'FAIL: frozen layer modified by post-W6 candidate' >&2; exit 1; }

postfreeze_source="$({ printf '%s\0' \
  browser/arborcore-browser-webgpu-postfreeze-optimization-1.contract \
  browser/webgpu_postfreeze_optimizer.js \
  browser/webgpu_rgba16_exact_tables.js \
  browser/webgpu_rgba16_experiment.js \
  docs/BROWSER_WEBGPU_POSTFREEZE_OPT0_OPT5.md \
  tests/browser/webgpu_postfreeze_browser_test.html \
  tests/c/webgpu_postfreeze_wasm_selftest.c \
  tests/js/browser_webgpu_postfreeze_unit_test.mjs \
  tools/webgpu_postfreeze_benchmark_verify.sh \
  tools/webgpu_postfreeze_contract_verify.sh \
  tools/webgpu_postfreeze_real_browser_runner.mjs \
  tools/webgpu_postfreeze_real_browser_verify.sh \
  tools/webgpu_postfreeze_table_verify.mjs \
  tools/webgpu_postfreeze_wasm_verify.sh \
  | LC_ALL=C sort -z | xargs -0 sha256sum; } | sha256sum | awk '{print $1}')"
printf 'postfreeze_opt_source_sha256=%s\n' "$postfreeze_source"
echo 'PASS: frozen W0-W6/B0-B6 identities and Post-freeze Optimization Contract v1 invariants'
