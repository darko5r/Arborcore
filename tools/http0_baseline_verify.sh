#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
BASE='1e6b04632d971ed1d26bcf74654250f784fcca64'; TREE='bc51bdc9683bec71a3cd2247aafae1889d18e1ba'
[[ "$(git rev-parse HEAD)" == "$BASE" ]]; [[ "$(git rev-parse 'HEAD^{tree}')" == "$TREE" ]]
check(){ [[ "$(sha256sum "$1" | awk '{print $1}')" == "$2" ]] || { echo "FAIL: frozen path drift: $1" >&2; exit 1; }; }
check include/arborcore/assembly_abi.h b54ef2d2d181fcfe980441f8711991df719d84116f03cc50c6042c7ab4eec894
check src/asm/http_parser.asm 2e612c85df2ea5e08e2b1f3a497f54f2d908e80fc250068d363013e5140ccb69
check src/asm/http_response.asm 6a9d8a51f7675cadcf3ab8a578857bae64f47f3c8a7f946d388b6efccc62624e
check include/arborcore/application.h f0888d89eb3e6472913211e8fe8631037a175fe97e37fd173b8954caa225a274
check src/c/application_foundation.c d0baac56848d460d07aba5e2d76e1aa41d9a0829c811350e79e9028fdd1d287b
check include/arborcore/application_transport.h 37498924eef1b388b63b070c9ba6bba9c718eebede657caf10d7d85ffb4ccf09
check src/c/application_transport.c a7335e8d59ede9b3d4f77928ae53d3050a039ab764a8df94d046c76ed257868a
check include/arborcore/mvc.h 18e3bb5f1c102294c46a543321c9a08232c6599b7039b086af577e9b12661e1a
check src/c/mvc.c 0273d5894d95aa0a9d7bed1ffda2746c3051e987ac2e7c9396b93d7d11e45f54
bash tools/http0_scope_verify.sh >/dev/null
echo "HTTP0_BASE_COMMIT=$BASE"; echo "HTTP0_BASE_TREE=$TREE"
echo 'ASSEMBLY_ABI_V1_BYTE_EXACT=YES'; echo 'FROZEN_HTTP_PARSER_BYTE_EXACT=YES'; echo 'FROZEN_LEGACY_RESPONSE_SERIALIZER_BYTE_EXACT=YES'
echo 'AF0_AF4_BYTE_EXACT=YES'; echo 'MVC0_BYTE_EXACT=YES'; echo 'HTTP0_LOWER_LAYER_RETROFIT_REQUIRED=NO'
echo 'PASS: HTTP0 starts from exact stable post-MVC0 authority and preserves frozen layers'
