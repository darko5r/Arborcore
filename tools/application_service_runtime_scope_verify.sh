#!/usr/bin/env bash
set -euo pipefail

ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
BASE="${ARBORCORE_AF3_BASE_COMMIT:-4337e18c4d76af4c2f11259cc3569244b3312a54}"
BRANCH="${ARBORCORE_AF3_BRANCH:-application-ddd-mvc-af3-service-runtime}"
cd "$ROOT"

[[ "$(git branch --show-current)" == "$BRANCH" ]] || { echo 'FAIL: AF3 branch mismatch' >&2; exit 1; }
[[ "$(git rev-parse HEAD)" == "$BASE" ]] || { echo 'FAIL: AF3 construction HEAD moved' >&2; exit 1; }
[[ "$(git rev-parse "$BASE^{tree}")" == 35398e2da67ef47efe2102165ca566fd752b594c ]]

expected=(
  Makefile
  application/arborcore-application-service-runtime-1.contract
  docs/APPLICATION_SERVICE_RUNTIME_AF3.md
  include/arborcore/application_service.h
  src/c/application_service.c
  tests/c/application_service_runtime_test.c
  tests/c/application_service_runtime_adversarial_test.c
  tests/asm/application_service_runtime_abi_test.asm
  tools/application_service_runtime_baseline_verify.sh
  tools/application_service_runtime_contract_verify.sh
  tools/application_service_runtime_native_verify.sh
  tools/application_service_runtime_abi_verify.sh
  tools/application_service_runtime_scope_verify.sh
  tools/application_service_runtime_reproducibility_verify.sh
  tools/application_service_runtime_gate.sh
)

mapfile -t actual < <(
  {
    git diff --name-only "$BASE" --
    git ls-files --others --exclude-standard
  } | LC_ALL=C sort -u
)
mapfile -t wanted < <(printf '%s\n' "${expected[@]}" | LC_ALL=C sort)

[[ "${#actual[@]}" -eq 15 ]] || {
  echo "FAIL: AF3 candidate path count ${#actual[@]}, expected 15" >&2
  printf '%s\n' "${actual[@]}" >&2
  exit 1
}
[[ "$(printf '%s\n' "${actual[@]}")" == "$(printf '%s\n' "${wanted[@]}")" ]] || {
  echo 'FAIL: AF3 candidate path set mismatch' >&2
  diff -u <(printf '%s\n' "${wanted[@]}") <(printf '%s\n' "${actual[@]}") >&2 || true
  exit 1
}

# Every AF0-AF2 production path is protected. Makefile is the sole pre-existing
# path intentionally extended by AF3.
protected=(
  src/asm
  abi
  geometry
  renderer
  browser
  include/arborcore/assembly_abi.h
  include/arborcore/arborcore.h
  include/arborcore/application.h
  include/arborcore/capability.h
  src/c/status.c
  src/c/security.c
  src/c/request.c
  src/c/route.c
  src/c/runtime.c
  src/c/application_foundation.c
  src/c/capability_kernel.c
  application/arborcore-application-ddd-mvc-foundation-1.contract
  application/arborcore-application-capability-kernel-1.contract
  docs/APPLICATION_DDD_MVC_FOUNDATION.md
  docs/APPLICATION_CAPABILITY_KERNEL_AF2.md
  tests/c/application_foundation_test.c
  tests/c/application_capability_kernel_test.c
  tools/application_foundation_contract_verify.sh
  tools/application_foundation_frozen_layers_verify.sh
  tools/application_foundation_gate.sh
  tools/application_foundation_native_verify.sh
  tools/application_foundation_reproducibility_verify.sh
  tools/application_foundation_scope_verify.sh
  tools/application_capability_kernel_baseline_verify.sh
  tools/application_capability_kernel_contract_verify.sh
  tools/application_capability_kernel_gate.sh
  tools/application_capability_kernel_native_verify.sh
  tools/application_capability_kernel_reproducibility_verify.sh
  tools/application_capability_kernel_scope_verify.sh
)

for rel in "${protected[@]}"; do
  [[ -z "$(git diff --name-only "$BASE" -- "$rel")" ]] || {
    echo "FAIL: protected pre-AF3 path changed: $rel" >&2
    exit 1
  }
done

echo 'AF3_CANDIDATE_PATH_COUNT=15'
printf '%s\n' "${actual[@]}"
echo 'AF3_LOWER_LAYER_RETROFIT_REQUIRED=NO'
echo 'PASS: AF3 construction scope exact; AF0-AF2 and lower qualified layers untouched'
