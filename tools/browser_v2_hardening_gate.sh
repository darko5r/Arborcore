#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"
MODE="${1:-full}"

echo '### BV2H0: exact inventory / frozen baseline'
bash tools/browser_v2_hardening_inventory.sh

echo
echo '### BV2H1/BV2H4: frozen authority + disposition contract'
bash tools/browser_v2_hardening_contract_verify.sh

echo
echo '### BV2H2/BV2H5: native lifecycle/metrics/timing authority'
bash tools/browser_v2_hardening_native_verify.sh

echo
echo '### BV2H2/BV2H5: zero-import WASM authority'
bash tools/browser_v2_hardening_wasm_verify.sh

echo
echo '### BV2H9: frozen source/evidence reproducibility'
bash tools/browser_v2_hardening_reproducibility_verify.sh

echo
echo '### Retained Browser v1 JavaScript retirement invariant'
make browser-v1-js-retirement-verify

if [[ "$MODE" == prelive ]]; then
  echo
  echo '### BV2H R2 PRE-LIVE GATE COMPLETE'
  echo 'BV2H_PRELIVE_STATE=PASS'
  echo 'BV2H_CONTRACT_STATE=FROZEN'
  echo 'BV2H_OPT3_PRODUCTION_ADMISSION=DEFERRED'
  echo 'NO COMMIT_PERFORMED=YES'
  exit 0
fi

if [[ "$MODE" != full ]]; then
  echo "FAIL: unknown BV2H gate mode: $MODE" >&2
  exit 1
fi

echo
echo '### BV2H1/BV2H6/BV2H7/BV2H8: fresh final live evidence'
bash tools/browser_v2_hardening_live_verify.sh evidence

echo
echo '### BV2H2/BV2H3: performance/resource lifetime evidence'
python3 tools/browser_v2_hardening_performance_verify.py

echo
echo '### BV2H4: retained exactness/performance disposition'
python3 tools/browser_v2_opt3_qualification_verify.py

echo
echo '### BV2H9: retained frozen LBv2 + lower-layer regression gate'
make browser-language-boundary-v2-gate

echo
echo '### BV2H9: final diff hygiene'
git diff --check
[[ "$(sha256sum browser/arborcore_host.js | awk '{print $1}')" == c7fb40e47ec93796e1a68b44948b983b433ac67b152a628a02678eae297b9d4a ]]

echo
echo '### BROWSER V2 HARDENING / OPTIMIZATION FREEZE GATE PASSED'
echo 'BV2H_STATE=FROZEN_BV2H'
echo 'BV2H_AUTHORITATIVE_JS_LOGIC=ZERO'
echo 'BV2H_OPT2_STATE=FROZEN_EXTENDED_QUALIFICATION'
echo 'BV2H_OPT3_STATE=QUALIFIED_TEST_ONLY_PROMISING_FUTURE_CONTRACT_REVISION'
echo 'BV2H_OPT3_PRODUCTION_ADMISSION=DEFERRED'
echo 'BV2H_WEBKIT_SAFARI=NOT_FORMALLY_QUALIFIED'
echo 'BV2H_CONTRACT_STATE=FROZEN'
echo 'BV2H_DELIVERY_STATE=FROZEN_BV2H'
echo 'BV2H9_DECISION=FREEZE_BROWSER_V2_HARDENING_OPTIMIZATION'
echo 'NO COMMIT_PERFORMED=YES'
echo 'NO_REMOTE_WRITE_PERFORMED=YES'
