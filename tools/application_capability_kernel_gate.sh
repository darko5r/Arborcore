#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"

echo '### AF2: exact base, scope and contract'
bash tools/application_capability_kernel_baseline_verify.sh
bash tools/application_capability_kernel_scope_verify.sh
bash tools/application_capability_kernel_contract_verify.sh

echo
echo '### AF2: AF0-AF1 R1 native regression'
make application-foundation-native-test

echo
echo '### AF2: capability/module native and adversarial qualification'
bash tools/application_capability_kernel_native_verify.sh

echo
echo '### AF2: deterministic candidate reproducibility'
bash tools/application_capability_kernel_reproducibility_verify.sh

echo
echo '### AF2: source hygiene'
git diff --check

echo
echo '### APPLICATION / DDD / MVC AF2 CAPABILITY KERNEL GATE PASSED'
echo 'AF2_DECISION=ADMIT_QUALIFIED_CANDIDATE_FOR_SOURCE_REVIEW'
echo 'AF2_LOWER_LAYER_RETROFIT_REQUIRED=NO'
echo 'AF2_APPLICATION_USE_CASE_MODEL=DEFERRED_AF3'
echo 'AF2_DDD_EVENT_PORT_TRANSACTION_MODEL=DEFERRED_AF4'
echo 'AF2_MVC_ROUTE_CONTROLLER_MIDDLEWARE=DEFERRED_PARALLEL_TRACK'
echo 'AF2_MARIADB=FROZEN'
echo 'AF2_R=FROZEN'
echo 'AF2_DEPLOYMENT=FROZEN'
