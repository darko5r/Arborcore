#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
source tools/server_benchmark_common.sh

assembly="$(arborcore_production_source_sha256 "$ROOT")"
c_runtime="$({ find include/arborcore/arborcore.h include/arborcore/assembly_abi.h src/c/status.c src/c/security.c src/c/request.c src/c/route.c src/c/runtime.c -type f -print0 | sort -z | xargs -0 sha256sum; } | sha256sum | awk '{print $1}')"
geometry="$({ printf '%s\0' include/arborcore/geometry.h src/c/geometry.c src/wasm/geometry_int128_builtins.c geometry/arborcore-geometry-1.contract | sort -z | xargs -0 sha256sum; } | sha256sum | awk '{print $1}')"
geometry_contract="$(sha256sum geometry/arborcore-geometry-1.contract | awk '{print $1}')"
r0_r3="$({ printf '%s\0' experiments/renderer/raster_foundation_candidates.h tests/c/renderer_foundation_candidate_test.c tests/c/renderer_foundation_wasm_selftest.c bench/renderer_foundation_bench.c tools/renderer_r0_r3_benchmark_run.sh tools/renderer_r0_r3_select.sh tools/renderer_r0_r3_wasm_verify.sh tools/renderer_r0_r3_lower_layer_verify.sh tools/renderer_r0_r3_gate.sh | sort -z | xargs -0 sha256sum; } | sha256sum | awk '{print $1}')"

printf 'assembly_source_sha256=%s\n' "$assembly"
printf 'c_runtime_source_sha256=%s\n' "$c_runtime"
printf 'geometry_source_sha256=%s\n' "$geometry"
printf 'geometry_contract_sha256=%s\n' "$geometry_contract"
printf 'renderer_r0_r3_source_sha256=%s\n' "$r0_r3"

[[ "$assembly" == "769b2c13a87eb8fc680236b60f916752b324d09cfc39e59a403221631c409d81" ]]
[[ "$c_runtime" == "5822f2723c9272d23c6fe4ef10b85adaf70514a511bd41e9cdc8376611d11c2f" ]]
[[ "$geometry" == "7c32142b40e27bd4735ffa12a5106127fc2411cc101a521ed98ce88a3c2b7aaf" ]]
[[ "$geometry_contract" == "6ca4f36380327536e6dada3118caad8594145f3865fbf4e8c6241d261738148b" ]]
[[ "$r0_r3" == "03bf27beacc9797d2c034b99b8d99fc8cb3a4b8670a020585652c3e6562b7598" ]]

echo "PASS: R4-R9 production renderer remains above qualified/frozen lower layers"
