#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"

EXPECTED_SOURCE="fe943ed7c52b75f8dd5eecd41f290ac9e06c5201ffeee59a00f713c267418783"
EXPECTED_CONTRACT="1db52f652d50943753bc18a3cf487741e7ad5a494614d64fac32a2f693a5aab7"

printf '%s\n' '### R4-R9: qualified/frozen lower-layer boundary'
make renderer-r4-r9-lower-layer-verify

printf '\n%s\n' '### R4-R7: production reference rasterizer qualification'
make renderer-check

printf '\n%s\n' '### R4-R7: sanitizer qualification'
make renderer-sanitize-verify

printf '\n%s\n' '### R5-R9: renderer contract/source verification'
make renderer-contract-verify

printf '\n%s\n' '### R8: native/WASM golden raster identity'
make renderer-wasm-golden-verify

printf '\n%s\n' '### R9: deterministic renderer archive'
make renderer-reproducibility-verify

printf '\n%s\n' '### R9: production hot-path performance'
make renderer-production-benchmark-verify

printf '\n%s\n' '### R9: frozen Geometry regression'
make geometry-check
make geometry-numerical-contract-verify

printf '\n%s\n' '### R9: qualified C bridge regression'
make c-runtime-check

printf '\n%s\n' '### R9: frozen Assembly regression suite'
make check

source_sha="$({ printf '%s\0' include/arborcore/renderer.h src/c/renderer.c renderer/srgb8_linear16_lut.h src/wasm/renderer_memory_builtins.c renderer/arborcore-renderer-1.contract | sort -z | xargs -0 sha256sum; } | sha256sum | awk '{print $1}')"
contract_sha="$(sha256sum renderer/arborcore-renderer-1.contract | awk '{print $1}')"
[[ "$source_sha" == "$EXPECTED_SOURCE" ]]
[[ "$contract_sha" == "$EXPECTED_CONTRACT" ]]

repro="$ROOT/build/renderer-reproducibility/result.env"
wasm="$ROOT/build/renderer-r8-wasm/result.env"
perf="$ROOT/build/renderer-r9-performance/result.env"
[[ -f "$repro" && -f "$wasm" && -f "$perf" ]]
source "$repro"
source "$wasm"
source "$perf"

[[ "$RENDERER_REPRODUCIBILITY_RESULT" == "PASS" ]]
[[ "$R8_WASM_GOLDEN_RESULT" == "PASS" ]]
[[ "$R9_RENDERER_PERFORMANCE_RESULT" == "PASS" ]]
[[ "$BENCHMARK_OPERAND_MODE" == "RUNTIME_NONCONSTANT_SHARED_SURFACE_RECT_COLOR" ]]

public_count="$(nm -g --defined-only build/libarborcore_renderer.a | awk 'NF >= 3 {print $3}' | grep '^arbor_' | sort -u | wc -l | tr -d ' ')"
[[ "$public_count" == "15" ]]

evidence="$ROOT/build/renderer-reference-v1.env"
cat > "$evidence" <<EVIDENCE
RENDERER_PHASE=R4-R9
RENDERER_STATE=REFERENCE_CONTRACT_FREEZE_CANDIDATE
RENDERER_BASE_COMMIT=d6ba8cf69f3ec280c019985488583b21e719f96b
GEOMETRY_NUMERICAL_CONTRACT=Q32.32_V1_FROZEN
RENDERER_PIXEL_FORMAT=RGBA16_UNORM_LE_PREMULTIPLIED
RENDERER_INTERNAL_COLOR_SPACE=LINEAR_LIGHT_SRGB_PRIMARIES
RENDERER_COVERAGE_FORMAT=Q0.32
RENDERER_COMPOSITING=PORTER_DUFF_SOURCE_OVER_PREMULTIPLIED
RENDERER_RECT_COVERAGE=ANALYTICAL_AREA_NO_SUPERSAMPLING
RENDERER_LINE_AA=FIXED_POINT_WU_MIDPOINT_Q0.32
RENDERER_PATH_STROKE=ONE_DEVICE_PIXEL_HAIRLINE
RENDERER_CURVE_TOLERANCE=1_OVER_256_DEVICE_PIXEL
RENDERER_PRODUCTION_SOURCE_SHA256=$source_sha
RENDERER_CONTRACT_SHA256=$contract_sha
RENDERER_ARCHIVE_SHA256=$RENDERER_ARCHIVE_SHA256
RENDERER_C_SURFACE_SYMBOL_COUNT=$public_count
RENDERER_C_SURFACE_STATE=UNFROZEN_CONSTRUCTION
R8_WASM_GOLDEN_RESULT=$R8_WASM_GOLDEN_RESULT
R8_GOLDEN_RGBA16_SHA256=$NATIVE_GOLDEN_SHA256
R8_WASM_MODULE_SHA256=$RENDERER_WASM_MODULE_SHA256
R9_RENDERER_PERFORMANCE_RESULT=$R9_RENDERER_PERFORMANCE_RESULT
R9_BENCHMARK_OPERAND_MODE=$BENCHMARK_OPERAND_MODE
R9_LINE_MEDIAN_NS=$LINE_MEDIAN_NS
R9_PATH_MEDIAN_NS=$PATH_MEDIAN_NS
ASSEMBLY_PRODUCTION_SOURCE_SHA256=769b2c13a87eb8fc680236b60f916752b324d09cfc39e59a403221631c409d81
C_RUNTIME_SOURCE_SHA256=5822f2723c9272d23c6fe4ef10b85adaf70514a511bd41e9cdc8376611d11c2f
GEOMETRY_PRODUCTION_SOURCE_SHA256=7c32142b40e27bd4735ffa12a5106127fc2411cc101a521ed98ce88a3c2b7aaf
GEOMETRY_CONTRACT_SHA256=6ca4f36380327536e6dada3118caad8594145f3865fbf4e8c6241d261738148b
RENDERER_CONTRACT_STATE=FREEZE_CANDIDATE
EVIDENCE
cat "$evidence"
echo
echo '### PRECISION RENDERER R4-R9 GATE PASSED'
echo 'R4_R9_DECISION=ADMIT_REFERENCE_CONTRACT_FREEZE_CANDIDATE'
