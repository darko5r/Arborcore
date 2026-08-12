#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
source "$ROOT/tools/server_benchmark_common.sh"
cd "$ROOT"

assembly="$(arborcore_production_source_sha256 "$ROOT")"
c_runtime="$({ find include/arborcore/arborcore.h include/arborcore/assembly_abi.h src/c/status.c src/c/security.c src/c/request.c src/c/route.c src/c/runtime.c -type f -print0 | sort -z | xargs -0 sha256sum; } | sha256sum | awk '{print $1}')"
geometry="$({ printf '%s\0' include/arborcore/geometry.h src/c/geometry.c src/wasm/geometry_int128_builtins.c geometry/arborcore-geometry-1.contract | sort -z | xargs -0 sha256sum; } | sha256sum | awk '{print $1}')"
contract="$(sha256sum geometry/arborcore-geometry-1.contract | awk '{print $1}')"

printf 'assembly_source_sha256=%s\n' "$assembly"
printf 'c_runtime_source_sha256=%s\n' "$c_runtime"
printf 'geometry_source_sha256=%s\n' "$geometry"
printf 'geometry_contract_sha256=%s\n' "$contract"

[[ "$assembly" == "769b2c13a87eb8fc680236b60f916752b324d09cfc39e59a403221631c409d81" ]]
[[ "$c_runtime" == "5822f2723c9272d23c6fe4ef10b85adaf70514a511bd41e9cdc8376611d11c2f" ]]
[[ "$geometry" == "7c32142b40e27bd4735ffa12a5106127fc2411cc101a521ed98ce88a3c2b7aaf" ]]
[[ "$contract" == "6ca4f36380327536e6dada3118caad8594145f3865fbf4e8c6241d261738148b" ]]
echo "PASS: R0-R3 experiment remains isolated above frozen Geometry Numerical Contract v1"
