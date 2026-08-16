#!/usr/bin/env bash
set -euo pipefail
ROOT="${ARBORCORE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT"

echo '### AF0: foundation contract and construction scope'
bash tools/application_foundation_contract_verify.sh
bash tools/application_foundation_scope_verify.sh

echo
echo '### AF0: qualified lower-layer baseline identities'
bash tools/application_foundation_frozen_layers_verify.sh

echo
echo '### AF1: native request/response/capability qualification'
bash tools/application_foundation_native_verify.sh

echo
echo '### AF1: deterministic candidate reproducibility'
bash tools/application_foundation_reproducibility_verify.sh

echo
echo '### AF0-AF1: source hygiene'
git diff --check

echo
echo '### APPLICATION / DDD / MVC FOUNDATION AF0-AF1 GATE PASSED'
echo 'AF0_AF1_DECISION=ADMIT_QUALIFIED_CANDIDATE_FOR_REVIEW'
echo 'AF0_AF1_LOWER_LAYER_RETROFIT_REQUIRED=NO'
echo 'AF0_AF1_CONTROLLER_IMPLEMENTATION=DEFERRED_AF3'
echo 'AF0_AF1_MIDDLEWARE_IMPLEMENTATION=DEFERRED_AF4'
echo 'AF0_AF1_REPOSITORY_IMPLEMENTATION=DEFERRED_AF7'
echo 'AF0_AF1_MARIADB=FROZEN'
echo 'AF0_AF1_R=FROZEN'
echo 'AF0_AF1_DEPLOYMENT=FROZEN'
