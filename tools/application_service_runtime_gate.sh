#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"

echo '### AF3: exact frozen base, scope and contract'
bash tools/application_service_runtime_baseline_verify.sh
bash tools/application_service_runtime_scope_verify.sh
bash tools/application_service_runtime_contract_verify.sh

echo
echo '### AF3: AF0-AF1 native + sanitizer regression'
make application-foundation-native-test
make application-foundation-sanitize

echo
echo '### AF3: AF2 native + sanitizer regression'
make application-capability-kernel-native-test
make application-capability-kernel-sanitize

echo
echo '### AF3: Application-service runtime native/adversarial/sanitizer qualification'
bash tools/application_service_runtime_native_verify.sh

echo
echo '### AF3: handwritten Assembly/C ABI qualification'
bash tools/application_service_runtime_abi_verify.sh

echo
echo '### AF3: deterministic candidate reproducibility'
bash tools/application_service_runtime_reproducibility_verify.sh

echo
echo '### AF3: source hygiene'
git diff --check

echo
echo '### APPLICATION / DDD / MVC AF3 SERVICE RUNTIME GATE PASSED'
echo 'AF3_DECISION=ADMIT_QUALIFIED_CANDIDATE_FOR_SOURCE_REVIEW'
echo 'AF3_LOWER_LAYER_RETROFIT_REQUIRED=NO'
echo 'AF3_MVC_ROUTE_CONTROLLER_MIDDLEWARE_PRESENTER=DEFERRED_PARALLEL_TRACK'
echo 'AF3_DOMAIN_EVENT_PORT_TRANSACTION=DEFERRED_LATER_DDD_SUPPORT'
echo 'AF3_MARIADB=FROZEN'
echo 'AF3_R=FROZEN'
echo 'AF3_BROWSER_REDESIGN=FROZEN'
echo 'AF3_DEPLOYMENT=FROZEN'
