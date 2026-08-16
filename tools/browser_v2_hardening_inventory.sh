#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
BASE=22c5476754b6adf35fd742dfac5c208fed46d2bc
TREE=f59e3100b2eba0a76c176895d4a8ee53601a92e8
HOST_SHA=c7fb40e47ec93796e1a68b44948b983b433ac67b152a628a02678eae297b9d4a
ARCHIVE=browser-webgpu-postfreeze-opt0-opt5-archive
ARCHIVE_COMMIT=95dcc2495e3a44a415e11879ac621b1d01fb720c

echo '### BV2H0 INVENTORY'
[[ "$(git branch --show-current)" == browser-v2-hardening-optimization ]]
[[ "$(git rev-parse HEAD)" == "$BASE" ]]
[[ "$(git rev-parse 'HEAD^{tree}')" == "$TREE" ]]
[[ "$(git rev-parse main)" == "$BASE" ]]
[[ "$(git rev-parse origin/main)" == "$BASE" ]]
[[ "$(git rev-parse "$ARCHIVE")" == "$ARCHIVE_COMMIT" ]]
[[ "$(git rev-parse "origin/$ARCHIVE")" == "$ARCHIVE_COMMIT" ]]
[[ "$(sha256sum browser/arborcore_host.js | awk '{print $1}')" == "$HOST_SHA" ]]
[[ ! -e browser/precision_surface.js ]]
[[ ! -e browser/webgpu_accelerator.js ]]

printf 'BV2H0_BASE_COMMIT=%s\n' "$BASE"
printf 'BV2H0_BASE_TREE=%s\n' "$TREE"
printf 'BV2H0_PRODUCTION_HOST_SHA256=%s\n' "$HOST_SHA"
printf 'BV2H0_OPT_ARCHIVE_COMMIT=%s\n' "$ARCHIVE_COMMIT"
printf 'BV2H0_PRODUCTION_JS_FILE_COUNT=%s\n' "$(find browser -maxdepth 1 -type f -name '*.js' | wc -l)"
printf 'BV2H0_PRODUCTION_JS_ENTRY=%s\n' "$(find browser -maxdepth 1 -type f -name '*.js' -print)"
printf 'BV2H0_HARDENING_HEADER_LINES=%s\n' "$(wc -l < include/arborcore/browser_hardening_v2.h)"
printf 'BV2H0_HARDENING_C_LINES=%s\n' "$(wc -l < src/c/browser_hardening_v2.c)"
printf 'BV2H0_DIAGNOSTIC_JS_LINES=%s\n' "$(wc -l < tests/js/browser_v2_hardening_runtime.mjs)"
printf 'BV2H0_DIAGNOSTIC_HTML_LINES=%s\n' "$(wc -l < tests/browser/browser_v2_hardening_diagnostics.html)"
printf 'BV2H0_OPT_ARCHIVE_UNTRACKED_FILES=%s\n' "$(git ls-tree -r --name-only "$ARCHIVE^3" | wc -l)"
echo 'BV2H0_AUTHORITATIVE_JS_LOGIC=ZERO'
echo 'BV2H0_APPLICATION_DDD_MVC_SCOPE=FROZEN'
echo 'PASS: BV2H0 exact baseline, authority boundary and OPT archive inventory'
