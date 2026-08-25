#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"; cd "$ROOT"
BASE=08b55954139a2e8daaf254f4935c8f7a31e3aa19
[[ "$(git rev-parse HEAD)" == "$BASE" ]]
for path in \
  include/arborcore/view.h src/c/view.c \
  include/arborcore/http_mvc.h src/c/http_mvc.c \
  include/arborcore/mvc.h src/c/mvc.c; do
  git diff --quiet "$BASE" -- "$path" || {
    case "$path" in
      include/arborcore/view.h|src/c/view.c) : ;;
      *) echo "FAIL: V1 unexpectedly reopens frozen dependency: $path" >&2; exit 1 ;;
    esac
  }
done
# VIEW source/header must be byte-exact to the accepted M1 candidate.
sha256sum -c <<'HASHES'
be5a824ea52a542234f6c6d4016700d651c7068c04ade9bc6654e26724ed19bb  include/arborcore/view.h
35d5b9576e5a03552153ca1a1c08e6eb7fbad3852556e0ace461612505a478fa  src/c/view.c
HASHES
echo 'VIEW0_V1_PRODUCTION_VIEW_SOURCE_CHANGE=NO'
echo 'VIEW0_V1_PRODUCTION_VIEW_HEADER_CHANGE=NO'
echo 'VIEW0_V1_MVC0_HTTP0_HTTP1_REOPEN=NO'
echo 'PASS: V1 is development-time tooling over the exact M1 production boundary'
