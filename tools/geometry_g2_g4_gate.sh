#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C

ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
G01_COMMIT="a999bf6c05586c052b3ba192ea8f6c8d6c28739d"
EXPECTED_EXPERIMENT_SHA="e64dec13d23fce957cc1d425ab74a6e26fae2acff5f7569af98af3ad311d7337"
cd "$ROOT"

if ! git merge-base --is-ancestor "$G01_COMMIT" HEAD; then
  echo "FAIL: G2-G4 does not descend from qualified G0-G1 commit $G01_COMMIT" >&2
  exit 1
fi

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
[[ "$experiment_sha" == "$EXPECTED_EXPERIMENT_SHA" ]] || {
  echo "FAIL: accepted G0-G1 experiment source changed during G2-G4." >&2
  exit 1
}

if ! git diff --quiet "$G01_COMMIT" -- \
  src/asm \
  abi/arborcore-1.symbols \
  abi/arborcore-1.internal-symbols \
  abi/arborcore-1.layout \
  abi/arborcore-1.map \
  abi/arborcore-1.freeze \
  include/arborcore/arborcore.h \
  include/arborcore/assembly_abi.h \
  src/c/status.c \
  src/c/security.c \
  src/c/request.c \
  src/c/route.c \
  src/c/runtime.c; then
  echo "FAIL: G2-G4 modifies a qualified lower-layer file." >&2
  exit 1
fi

echo "### G2-G4: lower-layer immutability"
bash tools/geometry_lower_layer_verify.sh
printf 'g0_g1_experiment_source_sha256=%s\n' "$experiment_sha"

echo
echo "### G2: checked scalar/geometry qualification"
make geometry-check

echo
echo "### G2/G3/G4: sanitizer qualification"
make geometry-sanitize-verify

echo
echo "### G3/G4: numerical-contract source/shape verification"
make geometry-numerical-contract-verify

echo
echo "### G4: native/WASM semantic equivalence probe"
make geometry-wasm-verify

echo
echo "### G4: deterministic geometry archive"
make geometry-reproducibility-verify

echo
echo "### G2-G4: equivalent-work performance qualification"
make geometry-production-benchmark-verify

echo
echo "### G2-G4: qualified C bridge regression"
make c-runtime-check

echo
echo "### G2-G4: frozen Assembly regression suite"
make check

bash tools/geometry_lower_layer_verify.sh >/dev/null

geometry_source_sha="$({
  printf '%s\0' \
    include/arborcore/geometry.h \
    src/c/geometry.c \
    src/wasm/geometry_int128_builtins.c \
    geometry/arborcore-geometry-1.contract \
    | sort -z | xargs -0 sha256sum
} | sha256sum | awk '{print $1}')"
geometry_archive_sha="$(sha256sum build/libarborcore_geometry.a | awk '{print $1}')"
contract_sha="$(sha256sum geometry/arborcore-geometry-1.contract | awk '{print $1}')"
public_count="$(nm -g --defined-only build/libarborcore_geometry.a | awk 'NF >= 3 {print $3}' | grep '^arbor_' | sort -u | wc -l | tr -d ' ')"
wasm_result="$(awk -F= '$1=="G4_WASM_RUNTIME_RESULT" {print $2}' build/geometry-g4-wasm/result.env 2>/dev/null | tail -1 || true)"
perf_result="$(awk -F= '$1=="G2_G4_GEOMETRY_PERFORMANCE_RESULT" {print $2}' build/geometry-g2-g4-performance/result.env 2>/dev/null | tail -1 || true)"

# The make wrappers capture these in result.env; fail closed if evidence was not retained.
[[ "$wasm_result" == "PASS" ]] || { echo "FAIL: missing retained WASM PASS evidence" >&2; exit 1; }
[[ "$perf_result" == "PASS" ]] || { echo "FAIL: missing retained performance PASS evidence" >&2; exit 1; }

cat > build/geometry-numerical-contract-v1.env <<EVIDENCE
GEOMETRY_PHASE=G2-G4
GEOMETRY_STATE=NUMERICAL_FREEZE_CANDIDATE
GEOMETRY_BASE_COMMIT=$G01_COMMIT
GEOMETRY_REPRESENTATION=Q32.32
GEOMETRY_FRACTION_BITS=32
GEOMETRY_DEFAULT_ROUNDING=NEAREST_EVEN
GEOMETRY_OVERFLOW_POLICY=EXPLICIT_TRANSACTIONAL_FAILURE
GEOMETRY_DEVICE_SCALE=EXACT_REDUCED_RATIONAL
GEOMETRY_ROTATION_GENERATOR=INTEGER_CORDIC_31
GEOMETRY_PRODUCTION_SOURCE_SHA256=$geometry_source_sha
GEOMETRY_ARCHIVE_SHA256=$geometry_archive_sha
GEOMETRY_CONTRACT_SHA256=$contract_sha
GEOMETRY_C_SURFACE_SYMBOL_COUNT=$public_count
GEOMETRY_C_SURFACE_STATE=UNFROZEN_CONSTRUCTION
G4_WASM_RUNTIME_RESULT=$wasm_result
G2_G4_GEOMETRY_PERFORMANCE_RESULT=$perf_result
ASSEMBLY_PRODUCTION_SOURCE_SHA256=769b2c13a87eb8fc680236b60f916752b324d09cfc39e59a403221631c409d81
C_RUNTIME_SOURCE_SHA256=5822f2723c9272d23c6fe4ef10b85adaf70514a511bd41e9cdc8376611d11c2f
NUMERICAL_CONTRACT_STATE=FREEZE_CANDIDATE
EVIDENCE

cat build/geometry-numerical-contract-v1.env

echo
echo "### GEOMETRY PRECISION G2-G4 GATE PASSED"
echo "G2_G4_DECISION=ADMIT_NUMERICAL_FREEZE_CANDIDATE"
