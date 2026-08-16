#!/usr/bin/env bash
set -euo pipefail

ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
BASE="${ARBORCORE_AF3_BASE_COMMIT:-4337e18c4d76af4c2f11259cc3569244b3312a54}"
BASE_TREE='35398e2da67ef47efe2102165ca566fd752b594c'
cd "$ROOT"

[[ "$(git rev-parse "$BASE^{tree}")" == "$BASE_TREE" ]]
[[ "$(git show "$BASE:Makefile" | sha256sum | awk '{print $1}')" == c5db75ae2b1537411cf4d7910486069d8dcad44bcb95ad92157760da64b82960 ]]

# AF0-AF1 frozen authority.
[[ "$(sha256sum application/arborcore-application-ddd-mvc-foundation-1.contract | awk '{print $1}')" == bb9344f5f9e90c18efcd398b5292f13e277fe289320fe8e18aaf2d61c4cc7996 ]]
[[ "$(sha256sum docs/APPLICATION_DDD_MVC_FOUNDATION.md | awk '{print $1}')" == a5f65c64d0fe8bdbcce5f5201205c0a6ffb39ca9536c39c0d5bd6ffbde5ab1c7 ]]
[[ "$(sha256sum include/arborcore/application.h | awk '{print $1}')" == f0888d89eb3e6472913211e8fe8631037a175fe97e37fd173b8954caa225a274 ]]
[[ "$(sha256sum src/c/application_foundation.c | awk '{print $1}')" == d0baac56848d460d07aba5e2d76e1aa41d9a0829c811350e79e9028fdd1d287b ]]
[[ "$(sha256sum tests/c/application_foundation_test.c | awk '{print $1}')" == 553d24a07010c6918d7e78a1a0e13c6790bcaaa5c9dce49fd9af436e457dbfcf ]]

# AF2 frozen authority.
[[ "$(sha256sum application/arborcore-application-capability-kernel-1.contract | awk '{print $1}')" == 635a7ebf1c6f5bd0e3bda9d854fc206a6dd689d220a27d8baca52bec1034ef52 ]]
[[ "$(sha256sum docs/APPLICATION_CAPABILITY_KERNEL_AF2.md | awk '{print $1}')" == ff740209800d25aebcb1acecace0ce889566f458da003eef3c1ee3d31ab566f4 ]]
[[ "$(sha256sum include/arborcore/capability.h | awk '{print $1}')" == fd206458962be86cc439bb9510632e218d415f14fb0641a743c51cbfbdbafe5a ]]
[[ "$(sha256sum src/c/capability_kernel.c | awk '{print $1}')" == 22dadae3d3e9011bd9d6b9ad2142627fc73578e54f22190300d790fb32687ba2 ]]
[[ "$(sha256sum tests/c/application_capability_kernel_test.c | awk '{print $1}')" == 470b30310f9565ace8a64e74481b0aa6741f7335fdd2fb18950b3f705f781d3f ]]

# No already-qualified pre-AF3 path may differ from the frozen base except the
# Makefile, which AF3 is explicitly authorized to extend.
mapfile -t preexisting_changes < <(git diff --name-only "$BASE" -- . ':(exclude)Makefile')
for rel in "${preexisting_changes[@]}"; do
  case "$rel" in
    application/arborcore-application-service-runtime-1.contract|\
    docs/APPLICATION_SERVICE_RUNTIME_AF3.md|\
    include/arborcore/application_service.h|\
    src/c/application_service.c|\
    tests/c/application_service_runtime_test.c|\
    tests/c/application_service_runtime_adversarial_test.c|\
    tests/asm/application_service_runtime_abi_test.asm|\
    tools/application_service_runtime_baseline_verify.sh|\
    tools/application_service_runtime_contract_verify.sh|\
    tools/application_service_runtime_native_verify.sh|\
    tools/application_service_runtime_abi_verify.sh|\
    tools/application_service_runtime_scope_verify.sh|\
    tools/application_service_runtime_reproducibility_verify.sh|\
    tools/application_service_runtime_gate.sh)
      ;;
    *)
      echo "FAIL: pre-AF3 tracked path changed: $rel" >&2
      exit 1
      ;;
  esac
done

echo "AF3_BASE_COMMIT=$BASE"
echo "AF3_BASE_TREE=$BASE_TREE"
echo 'AF3_AF0_AF1_AF2_AUTHORITY=BYTE_EXACT'
echo 'AF3_LOWER_LAYER_RETROFIT_REQUIRED=NO'
echo 'PASS: AF3 starts from exact frozen AF0-AF2 main baseline'
