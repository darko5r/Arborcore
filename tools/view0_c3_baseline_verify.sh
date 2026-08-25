#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
BASE=08b55954139a2e8daaf254f4935c8f7a31e3aa19
TREE=eea1bab55dd90ab7f89bce5622db800a4f60e282
[[ "$(git rev-parse HEAD)" == "$BASE" ]]
[[ "$(git rev-parse HEAD^{tree})" == "$TREE" ]]
for path in \
  abi include/arborcore/assembly_abi.h src/asm \
  application mvc http \
  include/arborcore/application.h include/arborcore/capability.h include/arborcore/application_service.h include/arborcore/ddd_support.h \
  include/arborcore/mvc.h include/arborcore/http.h include/arborcore/http_mvc.h \
  src/c/application_foundation.c src/c/capability_kernel.c src/c/application_service.c src/c/ddd_support.c src/c/mvc.c src/c/http.c src/c/http_mvc.c \
  browser; do
  git diff --quiet "$BASE" -- "$path" || { echo "FAIL: frozen dependency changed: $path" >&2; exit 1; }
done
echo 'VIEW0_C3_ASSEMBLY_ABI_V1_REOPEN=NO'
echo 'VIEW0_C3_AF1_AF4_REOPEN=NO'
echo 'VIEW0_C3_MVC0_REOPEN=NO'
echo 'VIEW0_C3_HTTP0_REOPEN=NO'
echo 'VIEW0_C3_HTTP1_REOPEN=NO'
echo 'VIEW0_C3_BROWSER_AUTHORITY_REOPEN=NO'
echo 'PASS: VIEW0 C3 composes over exact frozen HTTP1 base without dependency reopen'
