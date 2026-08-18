#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
BASE='5caeedb4d7c2b92bee829e27dc615efd5a658cac'
TREE='562e26268350be6e942b477c59d7ea613376cc98'
[[ "$(git rev-parse HEAD)" == "$BASE" ]]
[[ "$(git rev-parse 'HEAD^{tree}')" == "$TREE" ]]

protected=(
  include/arborcore/application.h
  include/arborcore/capability.h
  include/arborcore/application_service.h
  include/arborcore/ddd_support.h
  include/arborcore/mvc.h
  include/arborcore/application_transport.h
  src/c/capability_kernel.c
  src/c/application_service.c
  src/c/ddd_support.c
  src/c/mvc.c
  src/c/application_transport.c
  src/asm/application_transport.asm
  mvc/arborcore-mvc-core-transport-1.contract
  include/arborcore/http.h
  src/c/http.c
  src/asm/http_header.asm
  src/asm/http_response_v2.asm
  http/arborcore-http-message-semantics-1.contract
  include/arborcore/assembly_abi.h
  abi/arborcore-1.symbols
  abi/arborcore-1.map
  abi/arborcore-1.layout
  abi/arborcore-1.freeze
  src/asm/http_parser.asm
  src/asm/http_response.asm
  src/asm/server.asm
)
for p in "${protected[@]}"; do
  [[ -e "$p" ]] || { echo "FAIL: protected authority missing: $p" >&2; exit 1; }
  git diff --quiet "$BASE" -- "$p" || { echo "FAIL: protected authority changed: $p" >&2; exit 1; }
done

echo 'ASSEMBLY_ABI_V1_BYTE_EXACT=YES'
echo 'HTTP0_BYTE_EXACT=YES'
echo 'MVC0_BYTE_EXACT=YES'
echo 'AF2_AF4_BYTE_EXACT=YES'
echo 'AF1_PUBLIC_LAYOUT_HEADER_BYTE_EXACT=YES'
echo 'PASS: HTTP1 preserves frozen Assembly ABI, HTTP0, MVC0 and AF2-AF4 source authority'
