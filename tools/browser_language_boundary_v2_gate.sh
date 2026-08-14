#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
echo '### LB0/LB1: contract and C/WASM authority'
make browser-language-boundary-v2-contract-verify
make browser-language-boundary-v2-native-test
make browser-language-boundary-v2-wasm-verify

echo
echo '### LB2-LB7: retained live browser equivalence'
make browser-language-boundary-v2-live-evidence-verify

echo
echo '### LB8: source reproducibility'
make browser-language-boundary-v2-reproducibility-verify

echo
echo '### LB8: frozen Browser/WebGPU v1 boundaries'
make browser-contract-verify
make webgpu-contract-verify
make browser-check
make browser-sanitize-verify
make browser-wasm-verify
make browser-reproducibility-verify
make browser-benchmark-verify

echo
echo '### LB8: frozen Renderer / Geometry / C / Assembly regressions'
make renderer-r4-r9-gate
make geometry-g2-g4-gate
make c-runtime-bridge-gate
make check

source_sha="$({ printf '%s\0' \
  include/arborcore/browser_host_v2.h src/c/browser_host_v2.c \
  browser/arborcore_host.js browser/shaders/rgba8_present.wgsl \
  browser/shaders/rgba16_exact_convert.wgsl browser/arborcore-browser-language-boundary-2.contract \
  | LC_ALL=C sort -z | xargs -0 sha256sum; } | sha256sum | awk '{print $1}')"
repro_sha="$(awk -F= '$1=="LBV2_ARCHIVE_SHA256"{print $2}' build/browser-language-boundary-v2-repro/result.env)"
qualification_repro_sha="$(awk -F= '$1=="LBV2_QUALIFICATION_ARCHIVE_SHA256"{print $2}' build/browser-language-boundary-v2-repro/result.env)"
cat > build/browser-language-boundary-v2.env <<EVIDENCE
LBV2_PHASE=LB0-LB8
LBV2_STATE=FROZEN_V2
LBV2_BASE_COMMIT=66574f02102c0b5bdcc97590ddfb3b30e298cf97
LBV2_BASE_TREE=7b93cfa50f3a0d70b9e8934c187099ab29185a4b
LBV2_NATIVE_AUTHORITY=ASSEMBLY_C
LBV2_PORTABLE_BROWSER_AUTHORITY=C_WASM
LBV2_GPU_AUTHORITY=WGSL
LBV2_JS_ROLE=BROWSER_HOST_SYSCALL_SHIM_ONLY
LBV2_AUTHORITATIVE_JS_LOGIC=ZERO
LBV2_SOURCE_SHA256=$source_sha
LBV2_LIVE_BROWSER_RESULT=PASS
LBV2_LIVE_BROWSER_COUNT=2
LBV2_LIVE_WEBGPU_BROWSER_COUNT=$(awk -F= '$1=="LBV2_LIVE_WEBGPU_BROWSER_COUNT"{print $2}' build/browser-language-boundary-v2-live/result.env)
LBV2_OPT2_STATE=REIMPLEMENTED_UNDER_V2_HOST_BOUNDARY
LBV2_OPT3_STATE=TEST_ONLY_EXPERIMENT_FUTURE_CONTRACT_REVISION_REQUIRED
LBV2_BROAD_BROWSER_RELEASE_CLAIM=NOT_ADMITTED_WEBKIT_SAFARI_NOT_FORMALLY_QUALIFIED
LBV2_ARCHIVE_SHA256=$repro_sha
LBV2_ARCHIVE_FILE_COUNT=21
LBV2_QUALIFICATION_ARCHIVE_SHA256=$qualification_repro_sha
LBV2_QUALIFICATION_ARCHIVE_FILE_COUNT=23
LBV2_QUALIFICATION_ARCHIVE_ROLE=POST_FREEZE_CURRENT_QUALIFICATION
LBV2_CONTRACT_STATE=FROZEN
LBV2_DELIVERY_STATE=FROZEN_V2
EVIDENCE
cat build/browser-language-boundary-v2.env
echo
echo '### BROWSER LANGUAGE BOUNDARY V2 LB0-LB8 FREEZE GATE PASSED'
echo 'LB0_LB8_DECISION=FREEZE_BROWSER_LANGUAGE_BOUNDARY_V2'
