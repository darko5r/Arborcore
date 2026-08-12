#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"

echo "### R0-R3: lower-layer immutability"
make renderer-r0-r3-lower-layer-verify

echo
echo "### R0-R2: candidate color/surface/bounds properties"
make renderer-r0-r3-candidate-test

echo
echo "### R3: native/WASM coverage equivalence"
make renderer-r0-r3-wasm-verify

echo
echo "### R0/R3: same-host foundation benchmark"
make renderer-r0-r3-benchmark-run
ARBORCORE_ROOT="$ROOT" bash tools/renderer_r0_r3_select.sh

echo
echo "### R0-R3: frozen geometry regression"
make geometry-check

selection="$ROOT/build/renderer-r0-r3/selection.env"
wasm_result="$ROOT/build/renderer-r0-r3/wasm/result.env"
evidence="$ROOT/build/renderer-r0-r3.env"
experiment_sha="$({ printf '%s\0' experiments/renderer/raster_foundation_candidates.h tests/c/renderer_foundation_candidate_test.c tests/c/renderer_foundation_wasm_selftest.c bench/renderer_foundation_bench.c tools/renderer_r0_r3_benchmark_run.sh tools/renderer_r0_r3_select.sh tools/renderer_r0_r3_wasm_verify.sh tools/renderer_r0_r3_lower_layer_verify.sh tools/renderer_r0_r3_gate.sh | sort -z | xargs -0 sha256sum; } | sha256sum | awk '{print $1}')"

{
  echo "RENDERER_PHASE=R0-R3"
  echo "RENDERER_STATE=QUALIFIED_FOUNDATION_EXPERIMENT"
  echo "RENDERER_BASE_COMMIT=d18bab355ecb3bcf7ad9d07479fa4dfc7c323745"
  echo "GEOMETRY_NUMERICAL_CONTRACT=Q32.32_V1_FROZEN"
  echo "RENDERER_EXPERIMENT_SOURCE_SHA256=$experiment_sha"
  cat "$selection"
  grep '^R0_R3_WASM_RUNTIME_RESULT=' "$wasm_result"
  echo "R1_SURFACE_LAYOUT=ROW_MAJOR_EXPLICIT_STRIDE"
  echo "R2_PIXEL_CELL=HALF_OPEN_UNIT_SQUARE"
  echo "R3_RECT_COVERAGE=ANALYTIC_AREA_NO_SUPERSAMPLING"
  echo "R0_R3_CONTRACT_STATE=UNFROZEN_EXPERIMENT"
} > "$evidence"
cat "$evidence"
echo
echo "### PRECISION RENDERER R0-R3 GATE PASSED"
echo "R0_R3_DECISION=REVIEW_RECOMMENDATION_ONLY"
