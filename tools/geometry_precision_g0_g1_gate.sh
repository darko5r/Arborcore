#!/usr/bin/env bash
set -euo pipefail

ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
BASE_COMMIT="55c95517033325a509e8a7728921742aae38cd54"
EXPECTED_ASSEMBLY_SOURCE="769b2c13a87eb8fc680236b60f916752b324d09cfc39e59a403221631c409d81"
EXPECTED_C_RUNTIME_SOURCE="5822f2723c9272d23c6fe4ef10b85adaf70514a511bd41e9cdc8376611d11c2f"
cd "$ROOT"

if ! git merge-base --is-ancestor "$BASE_COMMIT" HEAD; then
    echo "FAIL: geometry branch does not descend from qualified CR base $BASE_COMMIT" >&2
    exit 1
fi

if ! git diff --quiet "$BASE_COMMIT" -- \
    src/asm \
    abi/arborcore-1.symbols \
    abi/arborcore-1.internal-symbols \
    abi/arborcore-1.layout \
    abi/arborcore-1.map \
    abi/arborcore-1.freeze \
    include/arborcore \
    src/c; then
    echo "FAIL: G0-G1 experiment modifies a frozen/lower-layer contract." >&2
    git diff --stat "$BASE_COMMIT" -- \
      src/asm abi/arborcore-1.symbols abi/arborcore-1.internal-symbols \
      abi/arborcore-1.layout abi/arborcore-1.map abi/arborcore-1.freeze \
      include/arborcore src/c >&2 || true
    exit 1
fi

source tools/server_benchmark_common.sh
assembly_source="$(arborcore_production_source_sha256 "$ROOT")"
if [[ "$assembly_source" != "$EXPECTED_ASSEMBLY_SOURCE" ]]; then
    echo "FAIL: frozen Assembly source identity changed" >&2
    echo "expected=$EXPECTED_ASSEMBLY_SOURCE" >&2
    echo "actual=$assembly_source" >&2
    exit 1
fi

c_runtime_source="$({
    find include/arborcore src/c -type f -print0 | sort -z | xargs -0 sha256sum
} | sha256sum | awk '{print $1}')"
if [[ "$c_runtime_source" != "$EXPECTED_C_RUNTIME_SOURCE" ]]; then
    echo "FAIL: qualified C runtime source identity changed" >&2
    echo "expected=$EXPECTED_C_RUNTIME_SOURCE" >&2
    echo "actual=$c_runtime_source" >&2
    exit 1
fi

echo "### G0: lower-layer immutability"
echo "geometry_base_commit=$BASE_COMMIT"
echo "assembly_source_sha256=$assembly_source"
echo "c_runtime_source_sha256=$c_runtime_source"
echo "PASS: Geometry experiment is isolated above the qualified lower layers"

echo
echo "### G0: candidate arithmetic/property qualification"
make geometry-precision-candidate-test

echo
echo "### G0: WebAssembly representation probe"
bash tools/geometry_precision_wasm_probe.sh | tee build/geometry-precision-g0-g1/wasm-probe.txt

echo
echo "### G1: same-host candidate performance"
make geometry-precision-benchmark-run

echo
echo "### G1: evidence-backed representation recommendation"
bash tools/geometry_precision_select.sh

echo
echo "### G1: qualified C bridge regression"
make c-runtime-check

experiment_sha="$({
    find experiments/geometry \
         tests/c/geometry_precision_candidate_test.c \
         bench/geometry_precision_bench.c \
         tools/geometry_precision_benchmark_run.sh \
         tools/geometry_precision_select.sh \
         tools/geometry_precision_wasm_probe.sh \
         tools/geometry_precision_g0_g1_gate.sh \
         -type f -print0 | sort -z | xargs -0 sha256sum
} | sha256sum | awk '{print $1}')"

# shellcheck disable=SC1091
source build/geometry-precision-g0-g1/selection.env
wasm_probe="$(awk -F= '$1=="G0_WASM_COMPILE_PROBE" {print $2}' build/geometry-precision-g0-g1/wasm-probe.txt | tail -1)"

cat > build/geometry-precision-g0-g1.env <<EVIDENCE
GEOMETRY_PHASE=G0-G1
GEOMETRY_STATE=QUALIFIED_EXPERIMENT
GEOMETRY_BASE_COMMIT=$BASE_COMMIT
ASSEMBLY_PRODUCTION_SOURCE_SHA256=$assembly_source
C_RUNTIME_SOURCE_SHA256=$c_runtime_source
GEOMETRY_EXPERIMENT_SOURCE_SHA256=$experiment_sha
G0_MIN_INTEGER_RANGE=1048576
G0_MIN_FRACTION_BITS=16
G1_ELIGIBLE_CANDIDATES=Q32.32,Q24.40
Q32_32_SCORE_NS=$Q32_32_SCORE_NS
Q24_40_SCORE_NS=$Q24_40_SCORE_NS
Q32_32_VS_Q24_40_SCORE_DELTA_PCT=$Q32_32_VS_Q24_40_SCORE_DELTA_PCT
G0_WASM_COMPILE_PROBE=$wasm_probe
G1_SELECTION_DECISION=$G1_SELECTION_DECISION
G1_NUMERICAL_CONTRACT_STATE=UNFROZEN_EXPERIMENT
EVIDENCE

cat build/geometry-precision-g0-g1.env

echo
echo "### GEOMETRY PRECISION G0-G1 GATE PASSED"
echo "G0_G1_DECISION=REVIEW_RECOMMENDATION_ONLY"
