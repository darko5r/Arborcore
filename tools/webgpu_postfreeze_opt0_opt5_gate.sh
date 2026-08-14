#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
BASE="66574f02102c0b5bdcc97590ddfb3b30e298cf97"
BASE_TREE="7b93cfa50f3a0d70b9e8934c187099ab29185a4b"
W_CONTRACT_SHA="a0d95467817504dbdc4db38c22b10c210b208e0604857ab752aa2730e97a144b"
W_JS_SHA="b42d48d8f30a9c3cd11c63c68219effe4ad7126d511f0233583fd1a1a59e38bb"
cd "$ROOT"

[[ "$(git rev-parse HEAD)" == "$BASE" ]]
[[ "$(git rev-parse 'HEAD^{tree}')" == "$BASE_TREE" ]]

echo '### OPT0: frozen post-W6 inspection boundary'
make webgpu-postfreeze-contract-verify

echo
echo '### OPT1: measurement and zero-import B1 oracle'
make webgpu-postfreeze-js-check
make webgpu-postfreeze-wasm-verify

echo
echo '### OPT1-OPT4: live normal-browser retained evidence'
make webgpu-postfreeze-live-browser-evidence-verify

echo
echo '### Frozen W0-W6 live evidence remains retained'
make webgpu-live-browser-evidence-verify

echo
echo '### OPT1-OPT3: benchmark/equivalence decision'
make webgpu-postfreeze-benchmark-verify

echo
echo '### OPT5: deterministic source archive'
make webgpu-postfreeze-reproducibility-verify

echo
echo '### OPT5: frozen WebGPU v1/B0-B6 regression'
make webgpu-contract-verify
make browser-contract-verify
make browser-check
make browser-sanitize-verify
make browser-wasm-verify
make browser-reproducibility-verify
make browser-benchmark-verify

echo
echo '### OPT5: frozen Reference Raster regression'
make renderer-check
make renderer-contract-verify
make renderer-wasm-golden-verify

echo
echo '### OPT5: frozen Geometry regression'
make geometry-check
make geometry-numerical-contract-verify

echo
echo '### OPT5: qualified C bridge regression'
make c-runtime-check

echo
echo '### OPT5: frozen Assembly regression suite'
make check

live="$ROOT/build/browser-webgpu-postfreeze-live/result.env"
perf="$ROOT/build/browser-webgpu-postfreeze-performance/result.env"
repro="$ROOT/build/browser-webgpu-postfreeze-repro/result.env"
wasm="$ROOT/build/browser-webgpu-postfreeze-wasm/result.env"
[[ -s "$live" && -s "$perf" && -s "$repro" && -s "$wasm" ]]
[[ "$(awk -F= '$1=="OPT4_LIVE_BROWSER_RESULT" {print $2}' "$live")" == PASS ]]
webgpu_count="$(awk -F= '$1=="OPT4_LIVE_WEBGPU_BROWSER_COUNT" {print $2}' "$live")"
[[ "$webgpu_count" -ge 1 ]]
[[ "$(awk -F= '$1=="OPT2_ADMISSION_RESULT" {print $2}' "$perf")" == PASS_OPTIONAL_V1_COMPATIBLE_IMPLEMENTATION ]]
[[ "$(awk -F= '$1=="OPT3_EQUIVALENCE_RESULT" {print $2}' "$perf")" == PASS_FROZEN_C_B1_ORACLE ]]
[[ "$(awk -F= '$1=="OPT3_CONTRACT_ADMISSION" {print $2}' "$perf")" == NONE_REQUIRES_FUTURE_CONTRACT_REVISION ]]

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
  tools/webgpu_postfreeze_opt0_opt5_gate.sh \
  tools/webgpu_postfreeze_real_browser_runner.mjs \
  tools/webgpu_postfreeze_real_browser_verify.sh \
  tools/webgpu_postfreeze_reproducibility_verify.sh \
  tools/webgpu_postfreeze_table_verify.mjs \
  tools/webgpu_postfreeze_wasm_verify.sh \
  | LC_ALL=C sort -z | xargs -0 sha256sum; } | sha256sum | awk '{print $1}')"
archive_sha="$(awk -F= '$1=="POSTFREEZE_OPT_ARCHIVE_SHA256" {print $2}' "$repro")"
vector_hash="$(awk -F= '$1=="OPT3_VECTOR_RGBA8_SHA256" {print $2}' "$wasm")"

cat > "$ROOT/build/browser-webgpu-postfreeze-opt0-opt5.env" <<EVIDENCE
POSTFREEZE_OPT_PHASE=OPT0-OPT5
POSTFREEZE_OPT_STATE=OPT2_ADMITTED_OPT3_EXPERIMENT_RETAINED
POSTFREEZE_OPT_BASE_COMMIT=$BASE
POSTFREEZE_OPT_BASE_TREE=$BASE_TREE
FROZEN_WEBGPU_V1_CONTRACT_SHA256=$W_CONTRACT_SHA
FROZEN_WEBGPU_V1_JS_SHA256=$W_JS_SHA
POSTFREEZE_OPT_SOURCE_SHA256=$postfreeze_source
OPT0_RESULT=PASS_READ_ONLY_BASELINE_RETAINED
OPT1_MEASUREMENT_RESULT=PASS
OPT1_COMPLETION_CALLBACK_POLICY=DIAGNOSTIC_NOT_GPU_EXECUTION_TIME
OPT2_SEMANTIC_RESULT=PASS_BYTE_EXACT_FROZEN_B1_RGBA8
OPT2_ADMISSION_RESULT=PASS_OPTIONAL_V1_COMPATIBLE_IMPLEMENTATION
OPT2_DELIVERY_STATE=ADMITTED_OPTIONAL_PARALLEL_IMPLEMENTATION
OPT3_EQUIVALENCE_RESULT=PASS_FROZEN_C_B1_ORACLE
OPT3_VECTOR_RGBA8_SHA256=$vector_hash
OPT3_CONTRACT_ADMISSION=NONE_REQUIRES_FUTURE_CONTRACT_REVISION
OPT3_EXPERIMENT_STATE=RETAINED_EXPERIMENTAL_FUTURE_CONTRACT_REVISION_REQUIRED
OPT4_LIVE_BROWSER_RESULT=PASS
OPT4_LIVE_BROWSER_COUNT=2
OPT4_LIVE_WEBGPU_BROWSER_COUNT=$webgpu_count
OPT4_BROAD_BROWSER_RELEASE_CLAIM=NOT_ADMITTED_WEBKIT_SAFARI_NOT_FORMALLY_QUALIFIED
POSTFREEZE_OPT_ARCHIVE_SHA256=$archive_sha
POSTFREEZE_OPT_CONTRACT_VERSION=1.0
POSTFREEZE_OPT_CONTRACT_STATE=FROZEN
POSTFREEZE_OPT_DELIVERY_STATE=FROZEN_V1
EVIDENCE
cat "$ROOT/build/browser-webgpu-postfreeze-opt0-opt5.env"
echo
echo '### BROWSER WEBGPU POST-W6 OPT0-OPT5 FREEZE GATE PASSED'
echo 'OPT0_OPT5_DECISION=FREEZE_POSTFREEZE_OPTIMIZATION_CONTRACT_V1'
