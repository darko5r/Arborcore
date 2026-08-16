#!/usr/bin/env bash
set -euo pipefail

ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
BASE="${ARBORCORE_AF2_BASE_COMMIT:-f5b98e824d367f20035823c8ef52f5bb08df9af7}"
BASE_TREE="08817700443742297371794175259424cb0b0a91"
cd "$ROOT"

[[ "$(git rev-parse "$BASE^{tree}")" == "$BASE_TREE" ]]

# AF0-AF1 R1 must remain byte-exact. Makefile is intentionally extended by AF2,
# so its frozen base blob is verified through the base commit rather than the
# working-tree file.
[[ "$(git show "$BASE:Makefile" | sha256sum | awk '{print $1}')" == d6226eb7853f8109134e576ac0e244554b155f0ce8e3be4dd9577d1afc132ba3 ]]
[[ "$(sha256sum application/arborcore-application-ddd-mvc-foundation-1.contract | awk '{print $1}')" == bb9344f5f9e90c18efcd398b5292f13e277fe289320fe8e18aaf2d61c4cc7996 ]]
[[ "$(sha256sum docs/APPLICATION_DDD_MVC_FOUNDATION.md | awk '{print $1}')" == a5f65c64d0fe8bdbcce5f5201205c0a6ffb39ca9536c39c0d5bd6ffbde5ab1c7 ]]
[[ "$(sha256sum include/arborcore/application.h | awk '{print $1}')" == f0888d89eb3e6472913211e8fe8631037a175fe97e37fd173b8954caa225a274 ]]
[[ "$(sha256sum src/c/application_foundation.c | awk '{print $1}')" == d0baac56848d460d07aba5e2d76e1aa41d9a0829c811350e79e9028fdd1d287b ]]
[[ "$(sha256sum tests/c/application_foundation_test.c | awk '{print $1}')" == 553d24a07010c6918d7e78a1a0e13c6790bcaaa5c9dce49fd9af436e457dbfcf ]]
[[ "$(sha256sum tools/application_foundation_contract_verify.sh | awk '{print $1}')" == 0e2db91db886857011aa4133fa71c22bb653f866c2335c92ebe5f2f308ab82c9 ]]
[[ "$(sha256sum tools/application_foundation_frozen_layers_verify.sh | awk '{print $1}')" == f5332063e9fb25d58c7aeb00b7b9f9f493381c3e3629f0e6d9b0b30f97aed41e ]]
[[ "$(sha256sum tools/application_foundation_gate.sh | awk '{print $1}')" == 8fe185a8bccc07b5cbb85ef8a0c4d667b062805486ead270450a64870f7cc17e ]]
[[ "$(sha256sum tools/application_foundation_native_verify.sh | awk '{print $1}')" == 3f586b160a85a770a48937a679e4ce229d1c9ef84171fac1cf6cce56d4cc226f ]]
[[ "$(sha256sum tools/application_foundation_reproducibility_verify.sh | awk '{print $1}')" == 41397d6eaf64b5bc894d206fb4646ee40d98cf7d2071c3eb1fae9ff20e298b9f ]]
[[ "$(sha256sum tools/application_foundation_scope_verify.sh | awk '{print $1}')" == 87cf240f49285bc1c99cf3f9585cdd6a86e330f2f86ad2db6dce52f6ba10604d ]]

bash tools/application_foundation_frozen_layers_verify.sh

echo 'AF2_BASE_COMMIT=f5b98e824d367f20035823c8ef52f5bb08df9af7'
echo 'AF2_BASE_TREE=08817700443742297371794175259424cb0b0a91'
echo 'AF2_AF0_AF1_R1_BASELINE=BYTE_EXACT'
echo 'AF2_LOWER_LAYER_RETROFIT_REQUIRED=NO'
echo 'PASS: AF2 starts from exact AF0-AF1 R1 and protected lower-layer baselines'
