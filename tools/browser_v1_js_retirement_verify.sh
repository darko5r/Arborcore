#!/usr/bin/env bash
set -euo pipefail

ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"

CONTRACT="$ROOT/browser/arborcore-browser-v1-js-retirement-1.contract"
HOST="$ROOT/browser/arborcore_host.js"
HISTORY="$ROOT/tools/browser_v1_history_verify.sh"
FIXTURE="$ROOT/tests/data/browser_v1_precision_vectors.json"

EXPECTED_HOST="c7fb40e47ec93796e1a68b44948b983b433ac67b152a628a02678eae297b9d4a"
EXPECTED_VECTOR="e730748ffe9d5cd4f0e06748b9f4e3df7617713e87091c5961329919cc87e95f"

cd "$ROOT"

[[ -f "$CONTRACT" ]]
[[ -f "$HOST" ]]
[[ -x "$HISTORY" ]]
[[ -f "$FIXTURE" ]]

[[ ! -e browser/precision_surface.js ]]
[[ ! -e browser/webgpu_accelerator.js ]]

retired_execution_files=(
  tests/js/browser_precision_unit_test.mjs
  tests/js/browser_webgpu_unit_test.mjs
  tests/browser/precision_surface_browser_test.html
  tests/browser/webgpu_accelerator_browser_test.html
  tools/browser_real_browser_runner.mjs
  tools/browser_real_browser_verify.sh
  tools/browser_b0_b6_gate.sh
  tools/webgpu_benchmark_verify.sh
  tools/webgpu_isolated_browser_runner.mjs
  tools/webgpu_isolated_browser_verify.sh
  tools/webgpu_real_browser_runner.mjs
  tools/webgpu_real_browser_verify.sh
  tools/webgpu_reproducibility_verify.sh
  tools/webgpu_w0_host_verify.sh
  tools/webgpu_w1_w6_gate.sh
)

for path in "${retired_execution_files[@]}"; do
  [[ ! -e "$path" ]] || {
    echo "FAIL: obsolete v1 JS execution file remains: $path" >&2
    exit 1
  }
done

mapfile -t browser_js < <(
  find browser \
    -maxdepth 1 \
    -type f \
    -name '*.js' \
    -print |
  LC_ALL=C sort
)

[[ "${#browser_js[@]}" -eq 1 ]]
[[ "${browser_js[0]}" == "browser/arborcore_host.js" ]]

host_sha="$(
  sha256sum "$HOST" |
  awk '{print $1}'
)"

vector_sha="$(
  sha256sum "$FIXTURE" |
  awk '{print $1}'
)"

[[ "$host_sha" == "$EXPECTED_HOST" ]]
[[ "$vector_sha" == "$EXPECTED_VECTOR" ]]

if grep -Eq \
  '^(BROWSER_JS|WEBGPU_JS)[[:space:]]*[:?+]?=' \
  Makefile
then
  echo 'FAIL: obsolete production-JS Make variable remains.' >&2
  exit 1
fi

obsolete_targets=(
  browser-real-browser-verify
  browser-b0-b6-gate
  webgpu-w0-host-verify
  webgpu-js-check
  webgpu-live-browser-verify
  webgpu-live-browser-evidence-verify
  webgpu-real-browser-verify
  webgpu-isolated-browser-verify
  webgpu-benchmark-verify
  webgpu-reproducibility-verify
  webgpu-w1-w6-gate
)

for target in "${obsolete_targets[@]}"; do
  if grep -Eq "^${target}:" Makefile; then
    echo "FAIL: obsolete Make target remains: $target" >&2
    exit 1
  fi
done

if grep -R \
  -nE \
  "(from[[:space:]]+['\"][^'\"]*(precision_surface|webgpu_accelerator)\.js|import[[:space:]].*(precision_surface|webgpu_accelerator)\.js|src=['\"][^'\"]*(precision_surface|webgpu_accelerator)\.js)" \
  browser tests tools \
  --include='*.js' \
  --include='*.mjs' \
  --include='*.html'
then
  echo 'FAIL: executable current-tree import of retired v1 JS remains.' >&2
  exit 1
fi

grep -qx \
  'ARBORCORE_BROWSER_V1_JS_RETIREMENT_VERSION=1.0' \
  "$CONTRACT"

grep -qx \
  'CURRENT_PRODUCTION_JS_ENTRY=browser/arborcore_host.js' \
  "$CONTRACT"

grep -qx \
  'CURRENT_PRODUCTION_JS_FILE_COUNT=1' \
  "$CONTRACT"

grep -qx \
  "CURRENT_PRODUCTION_JS_SHA256=$EXPECTED_HOST" \
  "$CONTRACT"

grep -qx \
  'AUTHORITATIVE_JS_LOGIC=ZERO' \
  "$CONTRACT"

grep -qx \
  'RETIRED_PRECISION_JS_HEAD_STATE=ABSENT' \
  "$CONTRACT"

grep -qx \
  'RETIRED_WEBGPU_JS_HEAD_STATE=ABSENT' \
  "$CONTRACT"

grep -qx \
  'HISTORICAL_SOURCE_RECOVERY=FROZEN_GIT_OBJECTS' \
  "$CONTRACT"

grep -qx \
  'HISTORICAL_CONTRACTS=RETAINED' \
  "$CONTRACT"

grep -qx \
  'V1_JS_EXECUTION_TESTS=RETIRED' \
  "$CONTRACT"

grep -qx \
  'V1_JS_BROWSER_RUNNERS=RETIRED' \
  "$CONTRACT"

grep -qx \
  'JSR_CONTRACT_STATE=FROZEN' \
  "$CONTRACT"

grep -qx \
  'JSR_DELIVERY_STATE=FROZEN_RETIREMENT' \
  "$CONTRACT"

if grep -q 'CANDIDATE' "$CONTRACT"; then
  echo 'FAIL: frozen retirement contract still contains CANDIDATE.' >&2
  exit 1
fi

history="$(
  ARBORCORE_ROOT="$ROOT" \
  bash "$HISTORY"
)"

printf '%s\n' "$history" |
grep -qx 'PRECISION_V1_HISTORY_VERIFIED=PASS'

printf '%s\n' "$history" |
grep -qx 'WEBGPU_V1_HISTORY_VERIFIED=PASS'

audit="$(
  python \
    tools/browser_language_boundary_v2_js_audit.py
)"

printf '%s\n' "$audit"

printf '%s\n' "$audit" |
grep -qx 'LBV2_PRODUCTION_JS_FILE_COUNT=1'

printf '%s\n' "$audit" |
grep -qx 'LBV2_AUTHORITATIVE_JS_LOGIC=ZERO'

printf 'CURRENT_PRODUCTION_JS_FILE_COUNT=1\n'
printf 'CURRENT_PRODUCTION_JS_ENTRY=browser/arborcore_host.js\n'
printf 'CURRENT_PRODUCTION_JS_SHA256=%s\n' "$host_sha"
printf 'CURRENT_PRODUCTION_JS_LINE_COUNT=%s\n' \
  "$(wc -l < "$HOST")"
printf 'OBSOLETE_V1_PRODUCTION_JS_FILE_COUNT=0\n'
printf 'RETIRED_V1_PRECISION_JS_PRESENT=NO\n'
printf 'RETIRED_V1_WEBGPU_JS_PRESENT=NO\n'
printf 'PRECISION_V1_HISTORY_VERIFIED=PASS\n'
printf 'WEBGPU_V1_HISTORY_VERIFIED=PASS\n'
printf 'AUTHORITATIVE_JS_LOGIC=ZERO\n'

echo 'PASS: current browser tree contains one production host-shim JS and no obsolete v1 production JS'
