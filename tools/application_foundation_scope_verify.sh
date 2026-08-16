#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
BASE="${ARBORCORE_AF_BASE_COMMIT:-a8d23f77bea211e31bd404230f06c2bc6e8a33d1}"
BRANCH="${ARBORCORE_AF_BRANCH:-application-ddd-mvc-foundation}"
cd "$ROOT"

[[ "$(git branch --show-current)" == "$BRANCH" ]] || { echo "FAIL: AF scope branch mismatch" >&2; exit 1; }
git rev-parse "$BASE^{commit}" >/dev/null
[[ "$(git merge-base "$BASE" HEAD)" == "$BASE" ]]

expected=(
  Makefile
  application/arborcore-application-ddd-mvc-foundation-1.contract
  docs/APPLICATION_DDD_MVC_FOUNDATION.md
  include/arborcore/application.h
  src/c/application_foundation.c
  tests/c/application_foundation_test.c
  tools/application_foundation_contract_verify.sh
  tools/application_foundation_frozen_layers_verify.sh
  tools/application_foundation_gate.sh
  tools/application_foundation_native_verify.sh
  tools/application_foundation_reproducibility_verify.sh
  tools/application_foundation_scope_verify.sh
)
mapfile -t actual < <(
  {
    git diff --name-only "$BASE" --
    git ls-files --others --exclude-standard
  } | LC_ALL=C sort -u
)
mapfile -t wanted < <(printf '%s\n' "${expected[@]}" | LC_ALL=C sort)

[[ "${#actual[@]}" -eq 12 ]] || {
  echo "FAIL: AF0-AF1 candidate path count ${#actual[@]}, expected 12" >&2
  printf '%s\n' "${actual[@]}" >&2
  exit 1
}
[[ "$(printf '%s\n' "${actual[@]}")" == "$(printf '%s\n' "${wanted[@]}")" ]] || {
  echo 'FAIL: AF0-AF1 candidate path set mismatch' >&2
  diff -u <(printf '%s\n' "${wanted[@]}") <(printf '%s\n' "${actual[@]}") >&2 || true
  exit 1
}

for protected in src/asm include/arborcore/assembly_abi.h abi geometry renderer browser; do
  if git diff --name-only "$BASE" -- "$protected" | grep -q .; then
    echo "FAIL: protected lower-layer path changed: $protected" >&2
    exit 1
  fi
done
for protected in include/arborcore/arborcore.h src/c/status.c src/c/security.c src/c/request.c src/c/route.c src/c/runtime.c \
                 include/arborcore/geometry.h src/c/geometry.c include/arborcore/renderer.h src/c/renderer.c \
                 include/arborcore/browser_hardening_v2.h include/arborcore/browser_host_v2.h include/arborcore/browser_surface.h \
                 src/c/browser_hardening_v2.c src/c/browser_host_v2.c src/c/browser_surface.c; do
  [[ -z "$(git diff --name-only "$BASE" -- "$protected")" ]] || { echo "FAIL: protected source changed: $protected" >&2; exit 1; }
done

echo 'AF0_AF1_CANDIDATE_PATH_COUNT=12'
printf '%s\n' "${actual[@]}"
echo 'PASS: AF0-AF1 construction scope exact; no lower-layer retrofit present'
