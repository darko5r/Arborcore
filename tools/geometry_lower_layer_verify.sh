#!/usr/bin/env bash
set -euo pipefail

ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
EXPECTED_ASSEMBLY="769b2c13a87eb8fc680236b60f916752b324d09cfc39e59a403221631c409d81"
EXPECTED_CR="5822f2723c9272d23c6fe4ef10b85adaf70514a511bd41e9cdc8376611d11c2f"

source "$ROOT/tools/server_benchmark_common.sh"
assembly="$(arborcore_production_source_sha256 "$ROOT" | tr -d '\n')"
cr="$({
  printf '%s\0' \
    "$ROOT/include/arborcore/arborcore.h" \
    "$ROOT/include/arborcore/assembly_abi.h" \
    "$ROOT/src/c/status.c" \
    "$ROOT/src/c/security.c" \
    "$ROOT/src/c/request.c" \
    "$ROOT/src/c/route.c" \
    "$ROOT/src/c/runtime.c" \
    | sort -z | xargs -0 sha256sum
} | sed "s#${ROOT}/##g" | sha256sum | awk '{print $1}')"

printf 'assembly_source_sha256=%s\n' "$assembly"
printf 'c_runtime_source_sha256=%s\n' "$cr"

[[ "$assembly" == "$EXPECTED_ASSEMBLY" ]] || {
  echo "FAIL: frozen Assembly source changed under Geometry Precision." >&2
  exit 1
}
[[ "$cr" == "$EXPECTED_CR" ]] || {
  echo "FAIL: qualified CR0-CR8 source changed under Geometry Precision." >&2
  exit 1
}

echo "PASS: Geometry production remains isolated above the qualified lower layers"
