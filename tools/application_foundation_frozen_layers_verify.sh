#!/usr/bin/env bash
set -euo pipefail

ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"

aggregate() {
  for rel in "$@"; do
    [[ -f "$rel" ]] || { echo "FAIL: missing protected lower-layer path $rel" >&2; exit 1; }
    sha256sum "$rel"
  done | LC_ALL=C sort -k2 | sha256sum | awk '{print $1}'
}

mapfile -t assembly_files < <(
  find src/asm abi -type f -print | LC_ALL=C sort
  printf '%s\n' include/arborcore/assembly_abi.h
)
c_runtime_files=(
  include/arborcore/arborcore.h
  src/c/status.c
  src/c/security.c
  src/c/request.c
  src/c/route.c
  src/c/runtime.c
)
geometry_files=(
  geometry/arborcore-geometry-1.contract
  include/arborcore/geometry.h
  src/c/geometry.c
)
renderer_files=(
  renderer/arborcore-renderer-1.contract
  renderer/srgb8_linear16_lut.h
  include/arborcore/renderer.h
  src/c/renderer.c
)
mapfile -t browser_files < <(
  find browser -type f -print | LC_ALL=C sort
  printf '%s\n' \
    include/arborcore/browser_hardening_v2.h \
    include/arborcore/browser_host_v2.h \
    include/arborcore/browser_surface.h \
    src/c/browser_hardening_v2.c \
    src/c/browser_host_v2.c \
    src/c/browser_surface.c
)

assembly_sha="$(aggregate "${assembly_files[@]}")"
c_runtime_sha="$(aggregate "${c_runtime_files[@]}")"
geometry_sha="$(aggregate "${geometry_files[@]}")"
renderer_sha="$(aggregate "${renderer_files[@]}")"
browser_sha="$(aggregate "${browser_files[@]}")"

[[ "$assembly_sha" == 923d80777ddbc7fa209b339d86e3174725e4c3ecadc0ec15fd372d46ca79e8e8 ]]
[[ "$c_runtime_sha" == 6de912d94108f92d7d44828ed5ef41d25687a2fe8e19898cd95f165e8850d589 ]]
[[ "$geometry_sha" == fdf73ee2f11b566a721d41e5da06de5927311375b6384d5dffb636eccb5afc9e ]]
[[ "$renderer_sha" == 3fc704168ce8a7dcc3bf1dca8a6b28409ca79164c9d8f1e75d7f614bf36a002b ]]
[[ "$browser_sha" == c4bbfcab986546a11bd599ca1bd2acc4cfdb9d6ecd401eec17fd095328d4df09 ]]

printf 'AF_ASSEMBLY_BASELINE_AGGREGATE_SHA256=%s\n' "$assembly_sha"
printf 'AF_C_RUNTIME_BASELINE_AGGREGATE_SHA256=%s\n' "$c_runtime_sha"
printf 'AF_GEOMETRY_BASELINE_AGGREGATE_SHA256=%s\n' "$geometry_sha"
printf 'AF_RENDERER_BASELINE_AGGREGATE_SHA256=%s\n' "$renderer_sha"
printf 'AF_BROWSER_BASELINE_AGGREGATE_SHA256=%s\n' "$browser_sha"
echo 'AF0_AF1_LOWER_LAYER_RETROFIT_REQUIRED=NO'
echo 'PASS: AF0-AF1 protected lower-layer source/contract baselines are byte-exact'
