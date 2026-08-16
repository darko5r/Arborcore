#!/usr/bin/env bash
set -euo pipefail

ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
BASE="${ARBORCORE_AF2_BASE_COMMIT:-f5b98e824d367f20035823c8ef52f5bb08df9af7}"
BRANCH="${ARBORCORE_AF2_BRANCH:-application-ddd-mvc-af2-capability-kernel}"
cd "$ROOT"

[[ "$(git branch --show-current)" == "$BRANCH" ]] || { echo 'FAIL: AF2 branch mismatch' >&2; exit 1; }
[[ "$(git rev-parse HEAD)" == "$BASE" ]] || { echo 'FAIL: AF2 construction HEAD moved' >&2; exit 1; }
[[ "$(git rev-parse "$BASE^{tree}")" == 08817700443742297371794175259424cb0b0a91 ]]

expected=(
  Makefile
  application/arborcore-application-capability-kernel-1.contract
  docs/APPLICATION_CAPABILITY_KERNEL_AF2.md
  include/arborcore/capability.h
  src/c/capability_kernel.c
  tests/c/application_capability_kernel_test.c
  tools/application_capability_kernel_baseline_verify.sh
  tools/application_capability_kernel_contract_verify.sh
  tools/application_capability_kernel_gate.sh
  tools/application_capability_kernel_native_verify.sh
  tools/application_capability_kernel_reproducibility_verify.sh
  tools/application_capability_kernel_scope_verify.sh
)

mapfile -t actual < <(
  {
    git diff --name-only "$BASE" --
    git ls-files --others --exclude-standard
  } | LC_ALL=C sort -u
)
mapfile -t wanted < <(printf '%s\n' "${expected[@]}" | LC_ALL=C sort)

[[ "${#actual[@]}" -eq 12 ]] || {
  echo "FAIL: AF2 candidate path count ${#actual[@]}, expected 12" >&2
  printf '%s\n' "${actual[@]}" >&2
  exit 1
}
[[ "$(printf '%s\n' "${actual[@]}")" == "$(printf '%s\n' "${wanted[@]}")" ]] || {
  echo 'FAIL: AF2 candidate path set mismatch' >&2
  diff -u <(printf '%s\n' "${wanted[@]}") <(printf '%s\n' "${actual[@]}") >&2 || true
  exit 1
}

# AF2 adds a new shared kernel; all previously qualified production source is
# protected. Makefile is the only pre-existing file intentionally extended.
protected=(
  src/asm
  abi
  geometry
  renderer
  browser
  include/arborcore/assembly_abi.h
  include/arborcore/arborcore.h
  include/arborcore/application.h
  include/arborcore/geometry.h
  include/arborcore/renderer.h
  include/arborcore/browser_hardening_v2.h
  include/arborcore/browser_host_v2.h
  include/arborcore/browser_surface.h
  src/c/status.c
  src/c/security.c
  src/c/request.c
  src/c/route.c
  src/c/runtime.c
  src/c/application_foundation.c
  src/c/geometry.c
  src/c/renderer.c
  src/c/browser_hardening_v2.c
  src/c/browser_host_v2.c
  src/c/browser_surface.c
  application/arborcore-application-ddd-mvc-foundation-1.contract
  docs/APPLICATION_DDD_MVC_FOUNDATION.md
  tests/c/application_foundation_test.c
  tools/application_foundation_contract_verify.sh
  tools/application_foundation_frozen_layers_verify.sh
  tools/application_foundation_gate.sh
  tools/application_foundation_native_verify.sh
  tools/application_foundation_reproducibility_verify.sh
  tools/application_foundation_scope_verify.sh
)

for rel in "${protected[@]}"; do
  [[ -z "$(git diff --name-only "$BASE" -- "$rel")" ]] || {
    echo "FAIL: protected pre-AF2 path changed: $rel" >&2
    exit 1
  }
done

echo 'AF2_CANDIDATE_PATH_COUNT=12'
printf '%s\n' "${actual[@]}"
echo 'AF2_LOWER_LAYER_RETROFIT_REQUIRED=NO'
echo 'PASS: AF2 construction scope exact; prior qualified layers untouched'
