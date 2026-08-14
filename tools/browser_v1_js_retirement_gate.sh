#!/usr/bin/env bash
set -euo pipefail

ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"

echo '### JSR3: frozen v1 history'
make browser-v1-history-verify

echo
echo '### JSR4: retired current-tree JS surface'
make browser-v1-js-retirement-verify

echo
echo '### JSR5: current LBv2 live browser qualification'
make browser-language-boundary-v2-live-evidence-verify

echo
echo '### JSR6: complete LBv2 and lower-layer regression'
make browser-language-boundary-v2-gate

echo
echo '### JSR6: retirement invariant after complete gate'
make browser-v1-js-retirement-verify

git diff --check

mkdir -p build

host_sha="$(
  sha256sum browser/arborcore_host.js |
  awk '{print $1}'
)"

vector_sha="$(
  sha256sum tests/data/browser_v1_precision_vectors.json |
  awk '{print $1}'
)"

contract_sha="$(
  sha256sum browser/arborcore-browser-v1-js-retirement-1.contract |
  awk '{print $1}'
)"

history_sha="$(
  sha256sum tools/browser_v1_history_verify.sh |
  awk '{print $1}'
)"

lbv2_source="$(
  awk -F= \
    '$1=="LBV2_SOURCE_SHA256" {print $2}' \
    build/browser-language-boundary-v2.env
)"

lbv2_archive="$(
  awk -F= \
    '$1=="LBV2_ARCHIVE_SHA256" {print $2}' \
    build/browser-language-boundary-v2.env
)"

lbv2_qualification_archive="$(
  awk -F= \
    '$1=="LBV2_QUALIFICATION_ARCHIVE_SHA256" {print $2}' \
    build/browser-language-boundary-v2.env
)"

cat > build/browser-v1-js-retirement.env <<EVIDENCE
JSR_PHASE=JSR0-JSR7
JSR_STATE=FROZEN_RETIREMENT
JSR_BASE_COMMIT=f3cc06fc721e75fa5059ecb3b32ab54f2e2ab2fb
CURRENT_PRODUCTION_JS_FILE_COUNT=1
CURRENT_PRODUCTION_JS_ENTRY=browser/arborcore_host.js
CURRENT_PRODUCTION_JS_SHA256=$host_sha
AUTHORITATIVE_JS_LOGIC=ZERO
OBSOLETE_V1_PRODUCTION_JS_FILE_COUNT=0
RETIRED_V1_PRECISION_JS_PRESENT=NO
RETIRED_V1_WEBGPU_JS_PRESENT=NO
PRECISION_V1_HISTORY_VERIFIED=PASS
WEBGPU_V1_HISTORY_VERIFIED=PASS
PRECISION_V1_VECTOR_SHA256=$vector_sha
JSR_CONTRACT_SHA256=$contract_sha
JSR_HISTORY_VERIFIER_SHA256=$history_sha
LBV2_SOURCE_SHA256=$lbv2_source
LBV2_ARCHIVE_SHA256=$lbv2_archive
LBV2_FROZEN_ARCHIVE_SHA256=$lbv2_archive
LBV2_QUALIFICATION_ARCHIVE_SHA256=$lbv2_qualification_archive
JSR_CONTRACT_STATE=FROZEN
JSR_DELIVERY_STATE=FROZEN_RETIREMENT
EVIDENCE

cat build/browser-v1-js-retirement.env

echo
echo '### BROWSER V1 JS RETIREMENT JSR0-JSR7 FREEZE GATE PASSED'
echo 'JSR0_JSR7_DECISION=FREEZE_BROWSER_V1_JS_RETIREMENT'
