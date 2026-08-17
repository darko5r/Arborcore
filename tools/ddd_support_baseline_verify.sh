#!/usr/bin/env bash
set -euo pipefail

ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
cd "$ROOT"

BASE_COMMIT='848f475ed536bb01133daa81b0888a40588fe0ee'
BASE_TREE='ebdd494a98de08b1ba62c1a309b6f1496769bf99'
AF4_BRANCH='application-ddd-mvc-af4-ddd-support'

fail(){ printf 'FAIL: %s\n' "$*" >&2; exit 1; }
eq(){ [[ "$2" == "$3" ]] || fail "$1 expected $3 got $2"; }

eq branch "$(git branch --show-current)" "$AF4_BRANCH"
eq HEAD "$(git rev-parse HEAD)" "$BASE_COMMIT"
eq HEAD-tree "$(git rev-parse 'HEAD^{tree}')" "$BASE_TREE"
eq local-main "$(git rev-parse refs/heads/main)" "$BASE_COMMIT"

declare -A EXPECTED=(
["application/arborcore-application-ddd-mvc-foundation-1.contract"]="bb9344f5f9e90c18efcd398b5292f13e277fe289320fe8e18aaf2d61c4cc7996"
["application/arborcore-application-capability-kernel-1.contract"]="635a7ebf1c6f5bd0e3bda9d854fc206a6dd689d220a27d8baca52bec1034ef52"
["application/arborcore-application-service-runtime-1.contract"]="acd1ae3719ea5e567e50ad58f0a9e75f54a75247a615104157f83214efa409d0"
["docs/APPLICATION_DDD_MVC_FOUNDATION.md"]="a5f65c64d0fe8bdbcce5f5201205c0a6ffb39ca9536c39c0d5bd6ffbde5ab1c7"
["docs/APPLICATION_CAPABILITY_KERNEL_AF2.md"]="ff740209800d25aebcb1acecace0ce889566f458da003eef3c1ee3d31ab566f4"
["docs/APPLICATION_SERVICE_RUNTIME_AF3.md"]="46d802f632cbe4a66b8c667f40d222e849de01fd649b0bcb283f2df772d38126"
["include/arborcore/application.h"]="f0888d89eb3e6472913211e8fe8631037a175fe97e37fd173b8954caa225a274"
["include/arborcore/capability.h"]="fd206458962be86cc439bb9510632e218d415f14fb0641a743c51cbfbdbafe5a"
["include/arborcore/application_service.h"]="4ac545c25f72ef19b9c006c8580c53db240f6af944c809738a85516d80fa6708"
["src/c/application_foundation.c"]="d0baac56848d460d07aba5e2d76e1aa41d9a0829c811350e79e9028fdd1d287b"
["src/c/capability_kernel.c"]="22dadae3d3e9011bd9d6b9ad2142627fc73578e54f22190300d790fb32687ba2"
["src/c/application_service.c"]="b45623f1ad7a5f3a7ba4827a0d6e080a9dfaba58a0c06a78561ff5df9c7211b6"
["tests/c/application_foundation_test.c"]="553d24a07010c6918d7e78a1a0e13c6790bcaaa5c9dce49fd9af436e457dbfcf"
["tests/c/application_capability_kernel_test.c"]="470b30310f9565ace8a64e74481b0aa6741f7335fdd2fb18950b3f705f781d3f"
["tests/c/application_service_runtime_test.c"]="becf5a84c33b6ff3b531841c1caa19178285c8381b01c36d51badf7bb20e1a1c"
["tests/c/application_service_runtime_adversarial_test.c"]="a67d5f86f6f56440b7e55826a13a4e04b46b66ccec043936c3c3dfa47dc3ee6d"
["tests/asm/application_service_runtime_abi_test.asm"]="70363155fef8aae83480a7bd33da561b13c89b458c848890cf2a56a0f2bae24e"
)

for p in "${!EXPECTED[@]}"; do
    [[ -f "$p" ]] || fail "missing frozen AF0-AF3 path: $p"
    eq "$p SHA-256" "$(sha256sum "$p" | awk '{print $1}')" "${EXPECTED[$p]}"
done

printf 'AF4_BASE_COMMIT=%s\n' "$BASE_COMMIT"
printf 'AF4_BASE_TREE=%s\n' "$BASE_TREE"
printf 'AF0_AF3_AUTHORITY=BYTE_EXACT\n'
printf 'AF4_LOWER_LAYER_RETROFIT_REQUIRED=NO\n'
printf 'PASS: AF4 starts from exact frozen AF0-AF3 authority\n'
